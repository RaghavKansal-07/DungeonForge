#include "enemy.h"
#include "adaptive_ai.h"
#include "enemy_fsm.h"
#include "collision.h"
#include "tilemap.h"
#include "player.h"
#include "item_drop.h"
#include "raylib.h"
#include "audio.h"
#include "ui.h"
#include "effects.h"

#include <math.h>
#include <stddef.h>

static Enemy enemies[MAX_ENEMIES];

#define NORMAL_ENEMY_RADIUS 20.0f
#define BOSS_RADIUS 28.0f

#define ENEMY_SPAWN_PLAYER_SAFE_DISTANCE 320.0f
#define ENEMY_SPAWN_PLAYER_MIN_DISTANCE 160.0f

static bool debug_enabled = false;

/*
 * ---------------------------------------------------------
 * Visual Effect State
 * ---------------------------------------------------------
 *
 * These effects are intentionally kept outside the Enemy
 * structure.
 *
 * This means visual effects do not become part of the
 * persistent enemy/save-game state.
 */

#define ENEMY_HIT_EFFECT_DURATION 0.12f
#define ENEMY_DEATH_EFFECT_DURATION 0.35f

static float enemy_hit_effect_timer[MAX_ENEMIES];

static float enemy_death_effect_timer[MAX_ENEMIES];

static float enemy_death_effect_x[MAX_ENEMIES];
static float enemy_death_effect_y[MAX_ENEMIES];

static float enemy_death_effect_radius[MAX_ENEMIES];

static bool enemy_death_effect_boss[MAX_ENEMIES];

/*
 * ---------------------------------------------------------
 * Normal Enemy Creation
 * ---------------------------------------------------------
 */

static void Enemy_Create(
    int index,
    float x,
    float y)
{
    enemies[index].active = true;

    enemies[index].x = x;
    enemies[index].y = y;

    enemies[index].speed = 140.0f;

    enemies[index].max_health = 100;
    enemies[index].health = 100;

    enemies[index].radius = NORMAL_ENEMY_RADIUS;

    enemies[index].state =
        ENEMY_STATE_IDLE;

    /*
     * Attack system.
     */
    enemies[index].attack_cooldown =
        0.8f;

    enemies[index].attack_timer =
        0.0f;

    /*
     * Hurt system.
     */
    enemies[index].hurt_timer =
        0.0f;

    /*
     * Target / awareness system.
     */
    enemies[index].target_x = x;
    enemies[index].target_y = y;

    enemies[index].awareness_timer =
        0.0f;

    /*
     * Pathfinding system.
     */
    enemies[index].path_timer =
        0.0f;

    enemies[index].waypoint_x = x;
    enemies[index].waypoint_y = y;

    enemies[index].path_valid =
        false;

    /*
     * -----------------------------------------------------
     * Adaptive AI
     * -----------------------------------------------------
     */
    switch (index % 3)
    {
    case 0:
        enemies[index].adaptive_type =
            ADAPTIVE_AI_HUNTER;
        break;

    case 1:
        enemies[index].adaptive_type =
            ADAPTIVE_AI_GUARDIAN;
        break;

    case 2:
    default:
        enemies[index].adaptive_type =
            ADAPTIVE_AI_ASSASSIN;
        break;
    }

    enemies[index].adaptive_decision =
        ADAPTIVE_DECISION_NONE;

    enemies[index].adaptation_level =
        0.0f;

    enemies[index].adaptive_timer =
        1.0f;

    /*
     * -----------------------------------------------------
     * Boss AI
     * -----------------------------------------------------
     */

    enemies[index].is_boss =
        false;

    enemies[index].boss_state =
        BOSS_STATE_IDLE;

    enemies[index].boss_phase =
        BOSS_PHASE_ONE;

    enemies[index].boss_state_timer =
        0.0f;

    enemies[index].boss_special_timer =
        0.0f;

    enemies[index].boss_recovery_timer =
        0.0f;

    /*
     * Clear visual effects for this slot.
     */
    enemy_hit_effect_timer[index] =
        0.0f;

    enemy_death_effect_timer[index] =
        0.0f;

    enemy_death_effect_x[index] =
        x;

    enemy_death_effect_y[index] =
        y;

    enemy_death_effect_radius[index] =
        20.0f;

    enemy_death_effect_boss[index] =
        false;
}

/*
 * ---------------------------------------------------------
 * Boss Configuration
 * ---------------------------------------------------------
 */

static void Enemy_ConfigureBoss(
    int index)
{
    enemies[index].is_boss =
        true;

    /*
     * Boss adaptive archetype.
     */
    enemies[index].adaptive_type =
        ADAPTIVE_AI_BOSS;

    enemies[index].adaptive_decision =
        ADAPTIVE_DECISION_NONE;

    enemies[index].adaptation_level =
        0.0f;

    enemies[index].adaptive_timer =
        1.0f;

    /*
     * Boss statistics.
     */
    enemies[index].speed =
        150.0f;

    enemies[index].max_health =
        500;

    enemies[index].health =
        500;

    enemies[index].radius =
        BOSS_RADIUS;

    /*
     * Boss attack.
     */
    enemies[index].attack_cooldown =
        0.9f;

    enemies[index].attack_timer =
        0.0f;

    /*
     * Boss state.
     */
    enemies[index].state =
        ENEMY_STATE_IDLE;

    enemies[index].boss_state =
        BOSS_STATE_IDLE;

    enemies[index].boss_phase =
        BOSS_PHASE_ONE;

    enemies[index].boss_state_timer =
        0.0f;

    enemies[index].boss_special_timer =
        0.0f;

    enemies[index].boss_recovery_timer =
        0.0f;

    /*
     * Navigation state.
     */
    enemies[index].path_timer =
        0.0f;

    enemies[index].waypoint_x =
        enemies[index].x;

    enemies[index].waypoint_y =
        enemies[index].y;

    enemies[index].path_valid =
        false;

    /*
     * Boss visual effect state.
     */
    enemy_death_effect_boss[index] =
        true;
}

/*
 * ---------------------------------------------------------
 * Find Floor Position
 * ---------------------------------------------------------
 */

static bool Enemy_FindFloorPosition(
    float avoid_x,
    float avoid_y,
    float minimum_distance,
    float radius,
    float *result_x,
    float *result_y)
{
    if (result_x == NULL ||
        result_y == NULL)
    {
        return false;
    }

    Player *player =
        Player_Get();

    float minimum_distance_squared =
        minimum_distance *
        minimum_distance;

    /*
     * Enemies must never spawn near the player.
     *
     * First try a comfortable distance (farther than the
     * enemy detection range). If the map has no such
     * spot, relax it once rather than failing to spawn.
     */
    const float player_distances[] =
        {
            ENEMY_SPAWN_PLAYER_SAFE_DISTANCE,
            ENEMY_SPAWN_PLAYER_MIN_DISTANCE};

    for (int pass = 0;
         pass < 2;
         pass++)
    {
        float player_distance_squared =
            player_distances[pass] *
            player_distances[pass];

        for (int y = 0;
             y < MAP_HEIGHT;
             y++)
        {
            for (int x = 0;
                 x < MAP_WIDTH;
                 x++)
            {
                if (TileMap_GetTile(
                        x,
                        y) != TILE_FLOOR)
                {
                    continue;
                }

                float world_x =
                    x * TILE_SIZE +
                    TILE_SIZE * 0.5f;

                float world_y =
                    y * TILE_SIZE +
                    TILE_SIZE * 0.5f;

                /*
                 * Distance from the supplied position.
                 */
                float dx =
                    world_x - avoid_x;

                float dy =
                    world_y - avoid_y;

                if (dx * dx + dy * dy <
                    minimum_distance_squared)
                {
                    continue;
                }

                /*
                 * Distance from the player.
                 */
                if (player != NULL)
                {
                    float player_dx =
                        world_x - player->x;

                    float player_dy =
                        world_y - player->y;

                    if (player_dx * player_dx +
                            player_dy * player_dy <
                        player_distance_squared)
                    {
                        continue;
                    }
                }

                /*
                 * Distance from every enemy already spawned.
                 */
                bool too_close_to_enemy = false;

                for (int i = 0;
                     i < MAX_ENEMIES;
                     i++)
                {
                    if (!enemies[i].active)
                        continue;

                    float enemy_dx =
                        world_x - enemies[i].x;

                    float enemy_dy =
                        world_y - enemies[i].y;

                    if (enemy_dx * enemy_dx +
                            enemy_dy * enemy_dy <
                        minimum_distance_squared)
                    {
                        too_close_to_enemy = true;
                        break;
                    }
                }

                if (too_close_to_enemy)
                    continue;

                /*
                 * The complete enemy circle must fit.
                 */
                if (!Collision_CanMove(
                        world_x,
                        world_y,
                        radius))
                {
                    continue;
                }

                *result_x = world_x;
                *result_y = world_y;

                return true;
            }
        }
    }

    return false;
}

/*
 * ---------------------------------------------------------
 * Enemy Initialization
 * ---------------------------------------------------------
 */

void Enemy_Init(void)
{

    EnemyFSM_Reset();

    /*
     * First deactivate every enemy slot.
     */
    for (int i = 0;
         i < MAX_ENEMIES;
         i++)
    {
        enemies[i].active = false;

        enemy_hit_effect_timer[i] =
            0.0f;

        enemy_death_effect_timer[i] =
            0.0f;

        enemy_death_effect_x[i] =
            0.0f;

        enemy_death_effect_y[i] =
            0.0f;

        enemy_death_effect_radius[i] =
            20.0f;

        enemy_death_effect_boss[i] =
            false;
    }

    /*
     * Get the player's generated spawn position.
     */
    Player *player =
        Player_Get();

    float player_x =
        player->x;

    float player_y =
        player->y;

    float spawn_x;
    float spawn_y;

    /*
     * -----------------------------------------------------
     * Enemy 0 - Hunter
     * -----------------------------------------------------
     */
    if (Enemy_FindFloorPosition(
            player_x,
            player_y,
            250.0f,
            NORMAL_ENEMY_RADIUS,
            &spawn_x,
            &spawn_y))
    {
        Enemy_Create(
            0,
            spawn_x,
            spawn_y);
    }

    /*
     * -----------------------------------------------------
     * Enemy 1 - Guardian
     * -----------------------------------------------------
     */
    float enemy0_x = player_x;
    float enemy0_y = player_y;

    if (enemies[0].active)
    {
        enemy0_x = enemies[0].x;
        enemy0_y = enemies[0].y;
    }

    if (Enemy_FindFloorPosition(
            enemy0_x,
            enemy0_y,
            200.0f,
            NORMAL_ENEMY_RADIUS,
            &spawn_x,
            &spawn_y))
    {
        Enemy_Create(
            1,
            spawn_x,
            spawn_y);
    }

    /*
     * -----------------------------------------------------
     * Enemy 2 - Assassin
     * -----------------------------------------------------
     */
    float enemy1_x = player_x;
    float enemy1_y = player_y;

    if (enemies[1].active)
    {
        enemy1_x = enemies[1].x;
        enemy1_y = enemies[1].y;
    }

    if (Enemy_FindFloorPosition(
            enemy1_x,
            enemy1_y,
            200.0f,
            NORMAL_ENEMY_RADIUS,
            &spawn_x,
            &spawn_y))
    {
        Enemy_Create(
            2,
            spawn_x,
            spawn_y);
    }

    /*
     * -----------------------------------------------------
     * Boss
     * -----------------------------------------------------
     */
    float boss_avoid_x =
        player_x;

    float boss_avoid_y =
        player_y;

    if (enemies[2].active)
    {
        boss_avoid_x =
            enemies[2].x;

        boss_avoid_y =
            enemies[2].y;
    }

    if (Enemy_FindFloorPosition(
            boss_avoid_x,
            boss_avoid_y,
            350.0f,
            BOSS_RADIUS,
            &spawn_x,
            &spawn_y))
    {
        Enemy_Create(
            3,
            spawn_x,
            spawn_y);

        Enemy_ConfigureBoss(
            3);
    }
}

/*
 * ---------------------------------------------------------
 * Visual Effect Update
 * ---------------------------------------------------------
 */

static void Enemy_UpdateVisualEffects(
    float dt)
{
    for (int i = 0;
         i < MAX_ENEMIES;
         i++)
    {
        if (enemy_hit_effect_timer[i] > 0.0f)
        {
            enemy_hit_effect_timer[i] -= dt;

            if (enemy_hit_effect_timer[i] < 0.0f)
            {
                enemy_hit_effect_timer[i] = 0.0f;
            }
        }

        if (enemy_death_effect_timer[i] > 0.0f)
        {
            enemy_death_effect_timer[i] -= dt;

            if (enemy_death_effect_timer[i] < 0.0f)
            {
                enemy_death_effect_timer[i] = 0.0f;
            }
        }
    }
}

/*
 * ---------------------------------------------------------
 * Enemy Update
 * ---------------------------------------------------------
 */

void Enemy_Update(void)
{
    float dt =
        GetFrameTime();

    /*
     * Update temporary visual effects first.
     */
    Enemy_UpdateVisualEffects(dt);

    /*
     * F3 toggles debug information.
     */
    if (IsKeyPressed(KEY_F3))
    {
        debug_enabled =
            !debug_enabled;
    }

    /*
     * Update every active enemy.
     */
    for (int i = 0;
         i < MAX_ENEMIES;
         i++)
    {
        if (!enemies[i].active)
            continue;

        /*
         * -------------------------------------------------
         * Adaptive AI update
         * -------------------------------------------------
         */
        if (enemies[i].adaptive_timer > 0.0f)
        {
            enemies[i].adaptive_timer -= dt;
        }

        if (enemies[i].adaptive_timer <= 0.0f)
        {
            AdaptiveDecisionResult result =
                AdaptiveAI_Decide(
                    enemies[i].adaptive_type);

            enemies[i].adaptive_decision =
                result.decision;

            enemies[i].adaptation_level =
                result.adaptation_level;

            enemies[i].adaptive_timer =
                1.0f;
        }

        /*
         * Existing FSM remains responsible for
         * actual enemy movement and combat.
         */
        EnemyFSM_Update(i);
    }

    /*
     * After all enemies have moved,
     * resolve enemy-enemy overlaps.
     */
    Collision_SeparateEnemies();
}

/*
 * ---------------------------------------------------------
 * Enemy Rendering Helpers
 * ---------------------------------------------------------
 */

static Color Enemy_GetArchetypeColor(
    AdaptiveAIType adaptive_type)
{
    switch (adaptive_type)
    {
    case ADAPTIVE_AI_HUNTER:
        return (Color){235, 70, 80, 255};

    case ADAPTIVE_AI_GUARDIAN:
        return (Color){245, 165, 55, 255};

    case ADAPTIVE_AI_ASSASSIN:
        return (Color){215, 70, 235, 255};

    case ADAPTIVE_AI_BOSS:
    default:
        return (Color){170, 80, 235, 255};
    }
}

/*
 * ---------------------------------------------------------
 * Boss Phase Colors
 * ---------------------------------------------------------
 */

static Color Enemy_GetBossPhaseColor(
    BossPhase phase)
{
    switch (phase)
    {
    case BOSS_PHASE_ONE:
        return (Color){175, 85, 245, 255};

    case BOSS_PHASE_TWO:
        return (Color){245, 145, 50, 255};

    case BOSS_PHASE_THREE:
        return (Color){240, 55, 65, 255};

    default:
        return (Color){175, 85, 245, 255};
    }
}

/*
 * ---------------------------------------------------------
 * Enemy Shadow
 * ---------------------------------------------------------
 */

static void Enemy_RenderShadow(
    const Enemy *enemy)
{
    float shadow_width =
        enemy->is_boss
            ? enemy->radius * 1.55f
            : enemy->radius * 1.25f;

    float shadow_height =
        enemy->is_boss
            ? enemy->radius * 0.62f
            : enemy->radius * 0.52f;

    DrawEllipse(
        (int)enemy->x + 2,
        (int)enemy->y + 7,
        shadow_width,
        shadow_height,
        Fade(
            BLACK,
            0.35f));
}

/*
 * ---------------------------------------------------------
 * Hunter Visual
 * ---------------------------------------------------------
 *
 * Hunter:
 * - pointed silhouette
 * - forward-facing eye
 * - directional fins
 * - red combat identity
 *
 * The geometry is intentionally different from the
 * Guardian and Assassin.
 */

static void Enemy_RenderHunter(
    const Enemy *enemy,
    Color body_color)
{
    int x =
        (int)enemy->x;

    int y =
        (int)enemy->y;

    float radius =
        enemy->radius;

    Color outline =
        Fade(
            WHITE,
            0.70f);

    /*
     * Main triangular body.
     */
    DrawPoly(
        (Vector2){
            (float)x,
            (float)(y - radius)},
        3,
        radius,
        0.0f,
        body_color);

    /*
     * Dark inner triangle gives the Hunter
     * more visual depth.
     */
    DrawPoly(
        (Vector2){
            (float)x,
            (float)(y - 2.0f)},
        3,
        radius * 0.52f,
        0.0f,
        Fade(
            MAROON,
            0.55f));

    /*
     * Outer outline.
     */
    DrawPolyLines(
        (Vector2){
            (float)x,
            (float)y},
        3,
        radius,
        0.0f,
        outline);

    /*
     * Hunter eye / targeting core.
     */
    DrawCircle(
        x,
        y - 5,
        4.0f,
        WHITE);

    DrawCircle(
        x,
        y - 5,
        2.0f,
        RED);

    /*
     * Small side fins communicate speed.
     */
    DrawLineEx(
        (Vector2){
            enemy->x - 10.0f,
            enemy->y + 7.0f},
        (Vector2){
            enemy->x - 17.0f,
            enemy->y + 13.0f},
        2.5f,
        Fade(
            body_color,
            0.85f));

    DrawLineEx(
        (Vector2){
            enemy->x + 10.0f,
            enemy->y + 7.0f},
        (Vector2){
            enemy->x + 17.0f,
            enemy->y + 13.0f},
        2.5f,
        Fade(
            body_color,
            0.85f));
}

/*
 * ---------------------------------------------------------
 * Guardian Visual
 * ---------------------------------------------------------
 *
 * Guardian:
 * - heavy square/shield silhouette
 * - reinforced corners
 * - central core
 * - orange/gold identity
 */

static void Enemy_RenderGuardian(
    const Enemy *enemy,
    Color body_color)
{
    int x =
        (int)enemy->x;

    int y =
        (int)enemy->y;

    int size =
        (int)(enemy->radius * 2.0f);

    int left =
        x - size / 2;

    int top =
        y - size / 2;

    /*
     * Outer shield body.
     */
    DrawRectangle(
        left,
        top,
        size,
        size,
        body_color);

    /*
     * Inner darker core.
     */
    DrawRectangle(
        left + 5,
        top + 5,
        size - 10,
        size - 10,
        Fade(
            BROWN,
            0.55f));

    /*
     * Strong outer border.
     */
    DrawRectangleLinesEx(
        (Rectangle){
            (float)left,
            (float)top,
            (float)size,
            (float)size},
        3.0f,
        Fade(
            WHITE,
            0.75f));

    /*
     * Shield cross.
     */
    DrawRectangle(
        x - 3,
        top + 6,
        6,
        size - 12,
        Fade(
            GOLD,
            0.85f));

    DrawRectangle(
        left + 6,
        y - 3,
        size - 12,
        6,
        Fade(
            GOLD,
            0.85f));

    /*
     * Central defensive core.
     */
    DrawCircle(
        x,
        y,
        5.0f,
        GOLD);

    DrawCircleLines(
        x,
        y,
        7.0f,
        WHITE);

    /*
     * Reinforced corner markers.
     */
    DrawCircle(
        left + 4,
        top + 4,
        3.0f,
        GOLD);

    DrawCircle(
        left + size - 4,
        top + 4,
        3.0f,
        GOLD);

    DrawCircle(
        left + 4,
        top + size - 4,
        3.0f,
        GOLD);

    DrawCircle(
        left + size - 4,
        top + size - 4,
        3.0f,
        GOLD);
}

/*
 * ---------------------------------------------------------
 * Assassin Visual
 * ---------------------------------------------------------
 *
 * Assassin:
 * - sharp diamond silhouette
 * - inner blade
 * - central eye
 * - small trailing blades
 */

static void Enemy_RenderAssassin(
    const Enemy *enemy,
    Color body_color)
{
    int x =
        (int)enemy->x;

    int y =
        (int)enemy->y;

    float radius =
        enemy->radius;

    /*
     * Main diamond.
     */
    DrawPoly(
        (Vector2){
            (float)x,
            (float)y},
        4,
        radius,
        45.0f,
        body_color);

    /*
     * Inner dark diamond.
     */
    DrawPoly(
        (Vector2){
            (float)x,
            (float)y},
        4,
        radius * 0.58f,
        45.0f,
        Fade(
            PURPLE,
            0.55f));

    /*
     * Outer sharp outline.
     */
    DrawPolyLines(
        (Vector2){
            (float)x,
            (float)y},
        4,
        radius,
        45.0f,
        Fade(
            WHITE,
            0.78f));

    /*
     * Inner blade running vertically.
     */
    DrawLineEx(
        (Vector2){
            enemy->x,
            enemy->y - 11.0f},
        (Vector2){
            enemy->x,
            enemy->y + 11.0f},
        2.0f,
        Fade(
            WHITE,
            0.70f));

    /*
     * Assassin eye/core.
     */
    DrawCircle(
        x,
        y,
        3.5f,
        WHITE);

    DrawCircle(
        x,
        y,
        1.5f,
        MAGENTA);

    /*
     * Small trailing blades.
     */
    DrawLineEx(
        (Vector2){
            enemy->x - 9.0f,
            enemy->y + 9.0f},
        (Vector2){
            enemy->x - 16.0f,
            enemy->y + 16.0f},
        2.5f,
        Fade(
            body_color,
            0.85f));

    DrawLineEx(
        (Vector2){
            enemy->x + 9.0f,
            enemy->y + 9.0f},
        (Vector2){
            enemy->x + 16.0f,
            enemy->y + 16.0f},
        2.5f,
        Fade(
            body_color,
            0.85f));
}

/*
 * ---------------------------------------------------------
 * Normal Enemy Body
 * ---------------------------------------------------------
 */

static void Enemy_RenderNormalBody(
    const Enemy *enemy,
    Color body_color)
{
    int x =
        (int)enemy->x;

    int y =
        (int)enemy->y;

    float radius =
        enemy->radius;

    Color render_color =
        body_color;

    /*
     * Hit flash.
     */
    if (enemy->hurt_timer > 0.0f)
    {
        render_color = WHITE;
    }

    /*
     * Shadow.
     */
    Enemy_RenderShadow(
        enemy);

    /*
     * Archetype-specific rendering.
     */
    switch (enemy->adaptive_type)
    {
    case ADAPTIVE_AI_HUNTER:
        Enemy_RenderHunter(
            enemy,
            render_color);
        break;

    case ADAPTIVE_AI_GUARDIAN:
        Enemy_RenderGuardian(
            enemy,
            render_color);
        break;

    case ADAPTIVE_AI_ASSASSIN:
        Enemy_RenderAssassin(
            enemy,
            render_color);
        break;

    default:
        DrawCircle(
            x,
            y,
            radius,
            render_color);

        DrawCircleLines(
            x,
            y,
            radius,
            Fade(
                WHITE,
                0.45f));

        break;
    }

    /*
     * -----------------------------------------------------
     * Attack indicator
     * -----------------------------------------------------
     */
    if (enemy->state ==
        ENEMY_STATE_ATTACK)
    {
        float pulse =
            sinf(
                GetTime() * 10.0f);

        float attack_radius =
            radius +
            5.0f +
            (pulse + 1.0f) * 2.0f;

        DrawCircleLines(
            x,
            y,
            attack_radius,
            RED);

        /*
         * Inner attack pulse.
         */
        DrawCircleLines(
            x,
            y,
            attack_radius - 3.0f,
            Fade(
                ORANGE,
                0.55f));
    }

    /*
     * -----------------------------------------------------
     * Hit impact effect
     * -----------------------------------------------------
     */
    int index =
        (int)(enemy - enemies);

    if (index >= 0 &&
        index < MAX_ENEMIES &&
        enemy_hit_effect_timer[index] > 0.0f)
    {
        float timer =
            enemy_hit_effect_timer[index];

        float progress =
            1.0f -
            timer /
                ENEMY_HIT_EFFECT_DURATION;

        float impact_radius =
            radius +
            4.0f +
            progress * 12.0f;

        float alpha =
            1.0f -
            progress;

        DrawCircleLines(
            x,
            y,
            impact_radius,
            Fade(
                WHITE,
                alpha));

        /*
         * Four directional sparks.
         */
        float spark_length =
            8.0f +
            progress * 8.0f;

        DrawLineEx(
            (Vector2){
                enemy->x - spark_length,
                enemy->y},
            (Vector2){
                enemy->x - spark_length - 5.0f,
                enemy->y},
            2.0f,
            Fade(
                WHITE,
                alpha));

        DrawLineEx(
            (Vector2){
                enemy->x + spark_length,
                enemy->y},
            (Vector2){
                enemy->x + spark_length + 5.0f,
                enemy->y},
            2.0f,
            Fade(
                WHITE,
                alpha));
    }
}

/*
 * ---------------------------------------------------------
 * Boss Body
 * ---------------------------------------------------------
 */

static void Enemy_RenderBossBody(
    const Enemy *enemy)
{
    int x =
        (int)enemy->x;

    int y =
        (int)enemy->y;

    Color boss_color =
        Enemy_GetBossPhaseColor(
            enemy->boss_phase);

    /*
     * Hurt flash takes priority.
     */
    if (enemy->hurt_timer > 0.0f)
    {
        boss_color = WHITE;
    }

    /*
     * Shadow.
     */
    Enemy_RenderShadow(
        enemy);

    /*
     * -----------------------------------------------------
     * Boss outer aura
     * -----------------------------------------------------
     */
    float aura_pulse =
        (sinf(
             GetTime() * 3.0f) +
         1.0f) *
        0.5f;

    DrawCircle(
        x,
        y,
        enemy->radius + 10.0f,
        Fade(
            boss_color,
            0.08f +
                aura_pulse * 0.07f));

    /*
     * Main boss body.
     */
    DrawCircle(
        x,
        y,
        enemy->radius,
        boss_color);

    /*
     * Dark inner core.
     */
    DrawCircle(
        x,
        y,
        enemy->radius * 0.62f,
        Fade(
            BLACK,
            0.22f));

    /*
     * Strong outer ring.
     */
    DrawCircleLines(
        x,
        y,
        enemy->radius + 5.0f,
        GOLD);

    /*
     * Second ring.
     */
    DrawCircleLines(
        x,
        y,
        enemy->radius + 9.0f,
        Fade(
            boss_color,
            0.60f));

    /*
     * Rotating-looking inner markers.
     */
    float rotation =
        GetTime() * 1.8f;

    for (int i = 0;
         i < 4;
         i++)
    {
        float angle =
            rotation +
            (float)i *
                (PI / 2.0f);

        float marker_distance =
            enemy->radius * 0.78f;

        float marker_x =
            enemy->x +
            cosf(angle) *
                marker_distance;

        float marker_y =
            enemy->y +
            sinf(angle) *
                marker_distance;

        DrawCircle(
            (int)marker_x,
            (int)marker_y,
            2.5f,
            GOLD);
    }

    /*
     * Boss central core.
     */
    DrawCircle(
        x,
        y,
        5.0f,
        WHITE);

    DrawCircle(
        x,
        y,
        2.5f,
        boss_color);

    /*
     * -----------------------------------------------------
     * Boss attack indicator
     * -----------------------------------------------------
     */
    if (enemy->boss_state ==
        BOSS_STATE_ATTACK)
    {
        float pulse =
            sinf(
                GetTime() * 12.0f);

        float attack_radius =
            enemy->radius +
            12.0f +
            (pulse + 1.0f) * 3.0f;

        DrawCircleLines(
            x,
            y,
            attack_radius,
            RED);

        DrawCircleLines(
            x,
            y,
            attack_radius + 5.0f,
            Fade(
                RED,
                0.45f));
    }

    /*
     * -----------------------------------------------------
     * Special / enraged indicator
     * -----------------------------------------------------
     */
    if (enemy->boss_state ==
            BOSS_STATE_SPECIAL ||
        enemy->boss_state ==
            BOSS_STATE_ENRAGED)
    {
        float pulse =
            sinf(
                GetTime() * 8.0f);

        float special_radius =
            enemy->radius +
            15.0f +
            (pulse + 1.0f) * 3.0f;

        DrawCircleLines(
            x,
            y,
            special_radius,
            ORANGE);

        DrawCircleLines(
            x,
            y,
            special_radius + 5.0f,
            Fade(
                GOLD,
                0.35f));
    }

    /*
     * -----------------------------------------------------
     * Boss hit impact
     * -----------------------------------------------------
     */
    int index =
        (int)(enemy - enemies);

    if (index >= 0 &&
        index < MAX_ENEMIES &&
        enemy_hit_effect_timer[index] > 0.0f)
    {
        float timer =
            enemy_hit_effect_timer[index];

        float progress =
            1.0f -
            timer /
                ENEMY_HIT_EFFECT_DURATION;

        float impact_radius =
            enemy->radius +
            6.0f +
            progress * 18.0f;

        float alpha =
            1.0f -
            progress;

        DrawCircleLines(
            x,
            y,
            impact_radius,
            Fade(
                WHITE,
                alpha));

        DrawCircleLines(
            x,
            y,
            impact_radius + 4.0f,
            Fade(
                GOLD,
                alpha * 0.65f));
    }
}

/*
 * ---------------------------------------------------------
 * Death Effect
 * ---------------------------------------------------------
 */

static void Enemy_RenderDeathEffects(void)
{
    for (int i = 0;
         i < MAX_ENEMIES;
         i++)
    {
        if (enemy_death_effect_timer[i] <= 0.0f)
            continue;

        float timer =
            enemy_death_effect_timer[i];

        float progress =
            1.0f -
            timer /
                ENEMY_DEATH_EFFECT_DURATION;

        float x =
            enemy_death_effect_x[i];

        float y =
            enemy_death_effect_y[i];

        float base_radius =
            enemy_death_effect_radius[i];

        /*
         * Expanding ring.
         */
        float ring_radius =
            base_radius +
            progress * 28.0f;

        float alpha =
            1.0f -
            progress;

        Color effect_color =
            enemy_death_effect_boss[i]
                ? GOLD
                : RED;

        DrawCircleLines(
            (int)x,
            (int)y,
            ring_radius,
            Fade(
                effect_color,
                alpha));

        /*
         * Second expanding ring for bosses.
         */
        if (enemy_death_effect_boss[i])
        {
            DrawCircleLines(
                (int)x,
                (int)y,
                ring_radius + 8.0f,
                Fade(
                    ORANGE,
                    alpha * 0.55f));
        }

        /*
         * Small fading center.
         */
        float center_radius =
            base_radius *
            (1.0f - progress * 0.65f);

        DrawCircle(
            (int)x,
            (int)y,
            center_radius,
            Fade(
                effect_color,
                alpha * 0.55f));
    }
}

/*
 * ---------------------------------------------------------
 * Adaptive AI Cues
 * ---------------------------------------------------------
 *
 * Draws what an enemy has decided to do, so the adaptive
 * AI can be seen without any debug text.
 */

static void Enemy_DrawDashedLine(
    float x1,
    float y1,
    float x2,
    float y2,
    Color color)
{
    float dx = x2 - x1;
    float dy = y2 - y1;

    float length =
        sqrtf(dx * dx + dy * dy);

    if (length < 1.0f)
        return;

    dx /= length;
    dy /= length;

    const float dash = 8.0f;
    const float gap = 6.0f;
    const float period = dash + gap;

    /* Dashes slowly travel toward the target. */
    float offset =
        fmodf((float)GetTime() * 40.0f, period);

    for (float d = offset - period;
         d < length;
         d += period)
    {
        float start = d < 0.0f ? 0.0f : d;
        float end = d + dash;

        if (end > length)
            end = length;

        if (end <= start)
            continue;

        DrawLineEx(
            (Vector2){x1 + dx * start, y1 + dy * start},
            (Vector2){x1 + dx * end, y1 + dy * end},
            2.0f,
            color);
    }
}

static void Enemy_RenderShield(
    const Enemy *enemy,
    float pulse)
{
    Color shield = (Color){90, 200, 255, 255};

    Vector2 center = {enemy->x, enemy->y};

    float inner = enemy->radius + 6.0f;
    float outer = inner + 3.0f;

    DrawCircle(
        (int)enemy->x,
        (int)enemy->y,
        outer + 2.0f,
        Fade(shield, 0.07f + pulse * 0.05f));

    DrawRing(
        center, inner, outer,
        0.0f, 360.0f, 36,
        Fade(shield, 0.35f));

    float spin = (float)GetTime() * 90.0f;

    for (int i = 0; i < 3; i++)
    {
        float start = spin + (float)i * 120.0f;

        DrawRing(
            center, inner - 1.0f, outer + 1.0f,
            start, start + 55.0f, 12,
            Fade(WHITE, 0.85f));
    }
}

static void Enemy_RenderAggression(
    const Enemy *enemy,
    float pulse)
{
    Color rage = (Color){255, 85, 60, 255};

    float spin = (float)GetTime() * 2.0f;

    for (int i = 0; i < 8; i++)
    {
        float angle =
            spin + (float)i * (PI / 4.0f);

        float inner =
            enemy->radius + 6.0f + pulse * 2.0f;

        float outer = inner + 7.0f;

        DrawLineEx(
            (Vector2){
                enemy->x + cosf(angle) * inner,
                enemy->y + sinf(angle) * inner},
            (Vector2){
                enemy->x + cosf(angle) * outer,
                enemy->y + sinf(angle) * outer},
            2.5f,
            Fade(rage, 0.85f));
    }
}

static void Enemy_RenderAdaptiveCue(Enemy *enemy)
{
    if (enemy->state == ENEMY_STATE_DEAD)
        return;

    AdaptiveDecision decision =
        enemy->adaptive_decision;

    if (decision == ADAPTIVE_DECISION_NONE)
        return;

    float pulse =
        (sinf((float)GetTime() * 6.0f) + 1.0f) * 0.5f;

    /* Guardian stances. */
    if (decision == ADAPTIVE_DECISION_ATTACK_DEFENSIVE)
    {
        Enemy_RenderShield(enemy, pulse);
        return;
    }

    if (decision == ADAPTIVE_DECISION_ATTACK_AGGRESSIVE)
    {
        if (enemy->state != ENEMY_STATE_IDLE)
            Enemy_RenderAggression(enemy, pulse);

        return;
    }

    /* Movement decisions matter only while hunting. */
    if (enemy->state != ENEMY_STATE_CHASE)
        return;

    float target_x = 0.0f;
    float target_y = 0.0f;

    if (!EnemyFSM_GetAdaptiveCue(
            enemy, &target_x, &target_y))
    {
        return;
    }

    Color color;

    switch (decision)
    {
    case ADAPTIVE_DECISION_PREDICT_LEFT:
    case ADAPTIVE_DECISION_PREDICT_RIGHT:
    case ADAPTIVE_DECISION_PREDICT_UP:
    case ADAPTIVE_DECISION_PREDICT_DOWN:
        color = (Color){255, 90, 100, 255};
        break;

    case ADAPTIVE_DECISION_FLANK_LEFT:
    case ADAPTIVE_DECISION_FLANK_RIGHT:
        color = (Color){225, 100, 255, 255};
        break;

    case ADAPTIVE_DECISION_CLOSE_DISTANCE:
        color = (Color){255, 200, 60, 255};
        break;

    default:
        return;
    }

    float strength = enemy->adaptation_level;

    if (strength < 0.0f)
        strength = 0.0f;
    if (strength > 1.0f)
        strength = 1.0f;

    float alpha = 0.35f + 0.5f * strength;

    /* Aura around the enemy. */
    DrawCircleLines(
        (int)enemy->x,
        (int)enemy->y,
        enemy->radius + 5.0f + pulse * 2.0f,
        Fade(color, alpha));

    /* Line to the point it is steering toward. */
    Enemy_DrawDashedLine(
        enemy->x, enemy->y,
        target_x, target_y,
        Fade(color, alpha * 0.8f));

    /* Crosshair at that point. */
    float r = 9.0f + pulse * 3.0f;

    DrawCircleLines(
        (int)target_x, (int)target_y,
        r, Fade(color, alpha));

    DrawCircle(
        (int)target_x, (int)target_y,
        2.5f, Fade(color, alpha));

    DrawLineEx(
        (Vector2){target_x - r - 5.0f, target_y},
        (Vector2){target_x - r + 3.0f, target_y},
        2.0f, Fade(color, alpha));

    DrawLineEx(
        (Vector2){target_x + r - 3.0f, target_y},
        (Vector2){target_x + r + 5.0f, target_y},
        2.0f, Fade(color, alpha));

    DrawLineEx(
        (Vector2){target_x, target_y - r - 5.0f},
        (Vector2){target_x, target_y - r + 3.0f},
        2.0f, Fade(color, alpha));

    DrawLineEx(
        (Vector2){target_x, target_y + r - 3.0f},
        (Vector2){target_x, target_y + r + 5.0f},
        2.0f, Fade(color, alpha));
}

/*
 * ---------------------------------------------------------
 * Enemy Rendering
 * ---------------------------------------------------------
 */

void Enemy_Render(void)
{
    /*
     * Render active enemies first.
     */
    for (int i = 0;
         i < MAX_ENEMIES;
         i++)
    {
        Enemy *enemy =
            &enemies[i];

        if (!enemy->active)
            continue;

        /*
         * -------------------------------------------------
         * Enemy body
         * -------------------------------------------------
         */
        if (enemy->is_boss)
        {
            Enemy_RenderBossBody(
                enemy);
        }
        else
        {
            Color body_color =
                Enemy_GetArchetypeColor(
                    enemy->adaptive_type);

            Enemy_RenderNormalBody(
                enemy,
                body_color);
        }

        if (enemy->is_boss)
        {
            Enemy_RenderBossBody(
                enemy);
        }
        else
        {
            Color body_color =
                Enemy_GetArchetypeColor(
                    enemy->adaptive_type);

            Enemy_RenderNormalBody(
                enemy,
                body_color);
        }

        Enemy_RenderAdaptiveCue(enemy);

        /*
         * -------------------------------------------------
         * Health bar
         * -------------------------------------------------
         */

        int bar_width =
            enemy->is_boss
                ? 64
                : 50;

        int bar_height =
            enemy->is_boss
                ? 7
                : 6;

        int bar_x =
            (int)enemy->x -
            bar_width / 2;

        int bar_y =
            (int)enemy->y -
            (int)enemy->radius -
            13;

        /*
         * Background.
         */
        DrawRectangle(
            bar_x,
            bar_y,
            bar_width,
            bar_height,
            Fade(
                BLACK,
                0.80f));

        float health_ratio =
            0.0f;

        if (enemy->max_health > 0)
        {
            health_ratio =
                (float)enemy->health /
                (float)enemy->max_health;
        }

        if (health_ratio < 0.0f)
            health_ratio = 0.0f;

        if (health_ratio > 1.0f)
            health_ratio = 1.0f;

        Color health_color =
            enemy->is_boss
                ? Enemy_GetBossPhaseColor(
                      enemy->boss_phase)
                : GREEN;

        if (!enemy->is_boss &&
            health_ratio <= 0.25f)
        {
            health_color = RED;
        }

        int health_width =
            (int)((float)bar_width *
                  health_ratio);

        if (health_width > 0)
        {
            DrawRectangle(
                bar_x,
                bar_y,
                health_width,
                bar_height,
                health_color);
        }

        DrawRectangleLines(
            bar_x,
            bar_y,
            bar_width,
            bar_height,
            Fade(
                WHITE,
                0.65f));

        /*
         * -------------------------------------------------
         * Debug information
         * -------------------------------------------------
         */
        if (!debug_enabled)
            continue;

        /*
         * Compact label: two short lines under the enemy.
         */
        const char *state_text = "UNKNOWN";

        if (enemy->is_boss)
        {
            switch (enemy->boss_state)
            {
            case BOSS_STATE_IDLE:
                state_text = "IDLE";
                break;
            case BOSS_STATE_CHASE:
                state_text = "CHASE";
                break;
            case BOSS_STATE_ATTACK:
                state_text = "ATTACK";
                break;
            case BOSS_STATE_SPECIAL:
                state_text = "SPECIAL";
                break;
            case BOSS_STATE_RECOVER:
                state_text = "RECOVER";
                break;
            case BOSS_STATE_ENRAGED:
                state_text = "ENRAGED";
                break;
            }
        }
        else
        {
            switch (enemy->state)
            {
            case ENEMY_STATE_IDLE:
                state_text = "IDLE";
                break;
            case ENEMY_STATE_CHASE:
                state_text = "CHASE";
                break;
            case ENEMY_STATE_ATTACK:
                state_text = "ATTACK";
                break;
            case ENEMY_STATE_HURT:
                state_text = "HURT";
                break;
            case ENEMY_STATE_DEAD:
                state_text = "DEAD";
                break;
            }
        }

        const char *line1 =
            enemy->is_boss
                ? TextFormat(
                      "BOSS P%d  %s",
                      (int)enemy->boss_phase + 1,
                      state_text)
                : TextFormat(
                      "%s  %s",
                      AdaptiveAI_GetTypeName(
                          enemy->adaptive_type),
                      state_text);

        bool has_decision =
            enemy->adaptive_decision !=
                ADAPTIVE_DECISION_NONE &&
            enemy->state != ENEMY_STATE_IDLE;

        const char *line2 =
            has_decision
                ? TextFormat(
                      "%s %.0f%%",
                      AdaptiveAI_GetDecisionName(
                          enemy->adaptive_decision),
                      enemy->adaptation_level *
                          100.0f)
                : "";

        int label_y =
            (int)(enemy->y + enemy->radius) + 10;

        int width1 =
            UI_TextWidth(line1, 11);

        UI_TextShadow(
            line1,
            (int)enemy->x - width1 / 2,
            label_y,
            11,
            enemy->is_boss ? GOLD : WHITE);

        if (has_decision)
        {
            int width2 =
                UI_TextWidth(line2, 10);

            UI_TextShadow(
                line2,
                (int)enemy->x - width2 / 2,
                label_y + 14,
                10,
                ORANGE);
        }
    }

    /*
     * Death effects must be rendered after active
     * enemies because the enemy itself may already
     * have been deactivated by the FSM.
     */
    Enemy_RenderDeathEffects();
}

/*
 * ---------------------------------------------------------
 * Enemy Access
 * ---------------------------------------------------------
 */

Enemy *Enemy_Get(int index)
{
    if (index < 0 ||
        index >= MAX_ENEMIES)
    {
        return 0;
    }

    return &enemies[index];
}

int Enemy_GetCount(void)
{
    int count = 0;

    for (int i = 0;
         i < MAX_ENEMIES;
         i++)
    {
        if (enemies[i].active)
            count++;
    }

    return count;
}

/*
 * ---------------------------------------------------------
 * Enemy Damage
 * ---------------------------------------------------------
 */

void Enemy_TakeDamage(
    int index,
    int damage)
{
    Enemy *enemy =
        Enemy_Get(index);

    if (enemy == 0)
        return;

    if (!enemy->active)
        return;

    if (enemy->state ==
        ENEMY_STATE_DEAD)
    {
        return;
    }

    /*
     * A Guardian in its defensive stance blocks part
     * of the damage.
     */
    if (enemy->adaptive_type ==
            ADAPTIVE_AI_GUARDIAN &&
        enemy->adaptive_decision ==
            ADAPTIVE_DECISION_ATTACK_DEFENSIVE)
    {
        damage = (damage * 6) / 10;

        if (damage < 1)
            damage = 1;
    }

    /*
     * Apply damage.
     */
    enemy->health -= damage;

    /*
     * Play the enemy hit sound after damage
     * has actually been applied.
     */
    Audio_PlayEnemyHit();

    /*
     * Trigger a short hit effect.
     */
    enemy_hit_effect_timer[index] =
        ENEMY_HIT_EFFECT_DURATION;

    Effects_SpawnNumber(
        enemy->x,
        enemy->y - enemy->radius - 34.0f,
        damage,
        '\0',
        enemy->is_boss
            ? GOLD
            : WHITE);

    Effects_AddShake(
        enemy->health <= 0
            ? 4.0f
            : 1.5f);

    /*
     * Enemy dies.
     */
    if (enemy->health <= 0)
    {
        enemy->health = 0;

        /*
         * Play the death sound once when the
         * enemy actually dies.
         */
        Audio_PlayEnemyDeath();

        /*
         * Store death-effect information BEFORE
         * the FSM removes the enemy from the
         * active enemy list.
         */
        enemy_death_effect_timer[index] =
            ENEMY_DEATH_EFFECT_DURATION;

        enemy_death_effect_x[index] =
            enemy->x;

        enemy_death_effect_y[index] =
            enemy->y;

        enemy_death_effect_radius[index] =
            enemy->radius;

        enemy_death_effect_boss[index] =
            enemy->is_boss;

        enemy->state =
            ENEMY_STATE_DEAD;

        /*
         * Drop a health potion at the
         * enemy's death position.
         */
        ItemDrop_Spawn(
            enemy->x,
            enemy->y,
            ITEM_ID_HEALTH_POTION,
            1);

        return;
    }

    /*
     * The hurt timer drives the white hit flash for
     * every enemy.
     */
    enemy->hurt_timer =
        0.15f;

    /*
     * The boss is stagger-resistant. It flashes when hit
     * but keeps its current state, so its attacks and
     * special cannot be cancelled by rapid hits.
     */
    if (enemy->is_boss)
        return;

    /*
     * Normal enemies flinch.
     */
    enemy->state =
        ENEMY_STATE_HURT;
}

/*
 * ---------------------------------------------------------
 * Enemy Restore
 * ---------------------------------------------------------
 *
 * Restore one enemy from saved game state.
 */

bool Enemy_Restore(
    int index,
    const Enemy *saved_enemy)
{
    if (saved_enemy == NULL)
        return false;

    if (index < 0 ||
        index >= MAX_ENEMIES)
    {
        return false;
    }

    /*
     * Only restore an enemy that was actually
     * active when the game was saved.
     */
    if (!saved_enemy->active)
    {
        enemies[index].active = false;

        enemy_hit_effect_timer[index] =
            0.0f;

        enemy_death_effect_timer[index] =
            0.0f;

        return true;
    }

    enemies[index] = *saved_enemy;

    /*
     * Reset transient navigation state.
     */
    enemies[index].path_timer =
        0.0f;

    enemies[index].waypoint_x =
        enemies[index].x;

    enemies[index].waypoint_y =
        enemies[index].y;

    enemies[index].path_valid =
        false;

    /*
     * Reset adaptive timer.
     */
    enemies[index].adaptive_timer =
        1.0f;

    /*
     * Clear transient visual effects.
     *
     * Effects should not survive a save/load operation.
     */
    enemy_hit_effect_timer[index] =
        0.0f;

    enemy_death_effect_timer[index] =
        0.0f;

    /*
     * The enemy must not be restored as DEAD.
     */
    if (enemies[index].state ==
        ENEMY_STATE_DEAD)
    {
        enemies[index].active = false;
    }

    return true;
}