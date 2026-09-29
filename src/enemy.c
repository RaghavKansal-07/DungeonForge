#include "enemy.h"
#include "enemy_fsm.h"
#include "collision.h"
#include "tilemap.h"
#include "player.h"
#include "item_drop.h"
#include "raylib.h"

#include <math.h>
#include <stddef.h>

static Enemy enemies[MAX_ENEMIES];

static bool debug_enabled = true;


/*
 * ---------------------------------------------------------
 * Normal Enemy Creation
 * ---------------------------------------------------------
 */

static void Enemy_Create(
    int index,
    float x,
    float y
)
{
    enemies[index].active = true;

    enemies[index].x = x;
    enemies[index].y = y;

    enemies[index].speed = 140.0f;

    enemies[index].max_health = 100;
    enemies[index].health = 100;

    enemies[index].radius = 20.0f;

    enemies[index].state =
        ENEMY_STATE_IDLE;

    /*
     * Attack system.
     *
     * Every enemy has its own attack timer.
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
     *
     * Assign normal enemy archetypes during creation.
     *
     * The boss will override this later.
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

    /*
     * Adaptive AI does not need to make a
     * decision every frame.
     *
     * This timer controls how frequently
     * an enemy reevaluates the player's
     * behavior.
     */
    enemies[index].adaptive_timer =
        1.0f;

    /*
     * -----------------------------------------------------
     * Boss AI
     * -----------------------------------------------------
     *
     * Every enemy starts as a normal enemy.
     *
     * A boss is explicitly configured later.
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
}


/*
 * ---------------------------------------------------------
 * Boss Configuration
 * ---------------------------------------------------------
 *
 * Convert an already-created enemy into the boss.
 *
 * Keeping this separate from Enemy_Create() means
 * normal enemies continue to use exactly the same
 * creation logic.
 */
static void Enemy_ConfigureBoss(
    int index
)
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
        28.0f;

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
}


/*
 * ---------------------------------------------------------
 * Find Floor Position
 * ---------------------------------------------------------
 *
 * Find a floor position that is sufficiently
 * far away from the supplied position and
 * from every enemy that has already spawned.
 */
static bool Enemy_FindFloorPosition(
    float avoid_x,
    float avoid_y,
    float minimum_distance,
    float *result_x,
    float *result_y
)
{
    if (result_x == NULL ||
        result_y == NULL)
    {
        return false;
    }

    float minimum_distance_squared =
        minimum_distance *
        minimum_distance;

    /*
     * Scan the generated tile map.
     *
     * Only floor tiles are valid spawn locations.
     */
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
             * Check the explicitly supplied position.
             *
             * For Enemy 0 this is the player.
             * For later enemies this is the
             * previously spawned enemy.
             */
            float dx =
                world_x - avoid_x;

            float dy =
                world_y - avoid_y;

            float distance_squared =
                dx * dx +
                dy * dy;

            if (distance_squared <
                minimum_distance_squared)
            {
                continue;
            }

            /*
             * Also check every enemy that has
             * already been spawned.
             */
            bool too_close_to_enemy = false;

            for (int i = 0;
                 i < MAX_ENEMIES;
                 i++)
            {
                if (!enemies[i].active)
                    continue;

                float enemy_dx =
                    world_x -
                    enemies[i].x;

                float enemy_dy =
                    world_y -
                    enemies[i].y;

                float enemy_distance_squared =
                    enemy_dx * enemy_dx +
                    enemy_dy * enemy_dy;

                if (enemy_distance_squared <
                    minimum_distance_squared)
                {
                    too_close_to_enemy = true;
                    break;
                }
            }

            if (too_close_to_enemy)
            {
                continue;
            }

            /*
             * Make sure the complete enemy circle
             * is inside a walkable location.
             */
            if (!Collision_CanMove(
                    world_x,
                    world_y,
                    20.0f))
            {
                continue;
            }

            *result_x = world_x;
            *result_y = world_y;

            return true;
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
    /*
     * First deactivate every enemy slot.
     */
    for (int i = 0;
         i < MAX_ENEMIES;
         i++)
    {
        enemies[i].active = false;
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

    /*
     * Find enemy spawn positions from the
     * generated dungeon instead of using
     * hardcoded world coordinates.
     */
    float spawn_x;
    float spawn_y;

    /*
     * -----------------------------------------------------
     * Enemy 0
     * -----------------------------------------------------
     */
    if (Enemy_FindFloorPosition(
            player_x,
            player_y,
            250.0f,
            &spawn_x,
            &spawn_y))
    {
        Enemy_Create(
            0,
            spawn_x,
            spawn_y
        );
    }

    /*
     * -----------------------------------------------------
     * Enemy 1
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
            &spawn_x,
            &spawn_y))
    {
        Enemy_Create(
            1,
            spawn_x,
            spawn_y
        );
    }

    /*
     * -----------------------------------------------------
     * Enemy 2
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
            &spawn_x,
            &spawn_y))
    {
        Enemy_Create(
            2,
            spawn_x,
            spawn_y
        );
    }

    /*
     * -----------------------------------------------------
     * Boss
     * -----------------------------------------------------
     *
     * The boss occupies slot 3.
     *
     * It is intentionally spawned farther away
     * from the player than normal enemies so that
     * the player has time to encounter the normal
     * enemies before reaching the boss.
     */
    float boss_avoid_x =
        player_x;

    float boss_avoid_y =
        player_y;

    /*
     * Prefer to keep the boss away from Enemy 2.
     */
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
            &spawn_x,
            &spawn_y))
    {
        Enemy_Create(
            3,
            spawn_x,
            spawn_y
        );

        Enemy_ConfigureBoss(
            3
        );
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
         *
         * Adaptive decisions are evaluated periodically,
         * not every frame.
         */
        if (enemies[i].adaptive_timer > 0.0f)
        {
            enemies[i].adaptive_timer -= dt;
        }

        if (enemies[i].adaptive_timer <= 0.0f)
        {
            AdaptiveDecisionResult result =
                AdaptiveAI_Decide(
                    enemies[i].adaptive_type
                );

            enemies[i].adaptive_decision =
                result.decision;

            enemies[i].adaptation_level =
                result.adaptation_level;

            /*
             * Evaluate again after one second.
             */
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
 * Enemy Rendering
 * ---------------------------------------------------------
 */

void Enemy_Render(void)
{
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
         *
         * Bosses are rendered differently so the player
         * can immediately identify the boss.
         */
        if (enemy->is_boss)
        {
            DrawCircle(
                (int)enemy->x,
                (int)enemy->y,
                enemy->radius,
                PURPLE
            );

            DrawCircleLines(
                (int)enemy->x,
                (int)enemy->y,
                enemy->radius + 4.0f,
                GOLD
            );
        }
        else
        {
            DrawCircle(
                (int)enemy->x,
                (int)enemy->y,
                enemy->radius,
                RED
            );
        }

        /*
         * -------------------------------------------------
         * Health bar background
         * -------------------------------------------------
         */
        DrawRectangle(
            (int)enemy->x - 25,
            (int)enemy->y - 35,
            50,
            6,
            DARKGRAY
        );

        /*
         * Health ratio.
         */
        float health_ratio =
            0.0f;

        if (enemy->max_health > 0)
        {
            health_ratio =
                (float)enemy->health /
                (float)enemy->max_health;
        }

        /*
         * Clamp health ratio.
         */
        if (health_ratio < 0.0f)
            health_ratio = 0.0f;

        if (health_ratio > 1.0f)
            health_ratio = 1.0f;

        /*
         * Health bar.
         */
        DrawRectangle(
            (int)enemy->x - 25,
            (int)enemy->y - 35,
            (int)(50 * health_ratio),
            6,
            enemy->is_boss
                ? PURPLE
                : GREEN
        );

        /*
         * -------------------------------------------------
         * Debug information
         * -------------------------------------------------
         */
        if (!debug_enabled)
            continue;

        const char *state_text =
            "UNKNOWN";

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

        /*
         * Include enemy ID so that when multiple
         * enemies are close together we can tell
         * them apart.
         */
        int label_offset =
            (i % 4) * 14;

        /*
         * -------------------------------------------------
         * Normal enemy debug
         * -------------------------------------------------
         */
        if (!enemy->is_boss)
        {
            DrawText(
                TextFormat(
                    "#%d %s",
                    i,
                    state_text
                ),
                (int)enemy->x - 30,
                (int)enemy->y +
                    25 +
                    label_offset,
                12,
                WHITE
            );

            /*
             * Adaptive AI debug information.
             */
            DrawText(
                TextFormat(
                    "%s",
                    AdaptiveAI_GetTypeName(
                        enemy->adaptive_type
                    )
                ),
                (int)enemy->x - 30,
                (int)enemy->y +
                    40 +
                    label_offset,
                10,
                YELLOW
            );

            DrawText(
                TextFormat(
                    "%s %.0f%%",
                    AdaptiveAI_GetDecisionName(
                        enemy->adaptive_decision
                    ),
                    enemy->adaptation_level *
                        100.0f
                ),
                (int)enemy->x - 30,
                (int)enemy->y +
                    52 +
                    label_offset,
                10,
                ORANGE
            );

            continue;
        }

        /*
         * -------------------------------------------------
         * Boss debug information
         * -------------------------------------------------
         */

        const char *boss_state_text =
            "UNKNOWN";

        switch (enemy->boss_state)
        {
            case BOSS_STATE_IDLE:
                boss_state_text = "IDLE";
                break;

            case BOSS_STATE_CHASE:
                boss_state_text = "CHASE";
                break;

            case BOSS_STATE_ATTACK:
                boss_state_text = "ATTACK";
                break;

            case BOSS_STATE_SPECIAL:
                boss_state_text = "SPECIAL";
                break;

            case BOSS_STATE_RECOVER:
                boss_state_text = "RECOVER";
                break;

            case BOSS_STATE_ENRAGED:
                boss_state_text = "ENRAGED";
                break;
        }

        const char *boss_phase_text =
            "UNKNOWN";

        switch (enemy->boss_phase)
        {
            case BOSS_PHASE_ONE:
                boss_phase_text = "PHASE 1";
                break;

            case BOSS_PHASE_TWO:
                boss_phase_text = "PHASE 2";
                break;

            case BOSS_PHASE_THREE:
                boss_phase_text = "PHASE 3";
                break;
        }

        int boss_label_x =
            (int)enemy->x - 45;

        int boss_label_y =
            (int)enemy->y + 38;

        DrawText(
            "BOSS",
            boss_label_x,
            boss_label_y,
            14,
            GOLD
        );

        DrawText(
            TextFormat(
                "%s",
                boss_state_text
            ),
            boss_label_x,
            boss_label_y + 16,
            11,
            WHITE
        );

        DrawText(
            TextFormat(
                "%s",
                boss_phase_text
            ),
            boss_label_x,
            boss_label_y + 30,
            11,
            SKYBLUE
        );

        DrawText(
            TextFormat(
                "%s %.0f%%",
                AdaptiveAI_GetDecisionName(
                    enemy->adaptive_decision
                ),
                enemy->adaptation_level *
                    100.0f
            ),
            boss_label_x,
            boss_label_y + 44,
            10,
            ORANGE
        );
    }
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
    int damage
)
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
     * Apply damage.
     */
    enemy->health -= damage;

    /*
     * Enemy dies.
     */
    if (enemy->health <= 0)
    {
        enemy->health = 0;

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
            1
        );

        return;
    }

    /*
     * Enemy survives and enters HURT.
     */
    enemy->hurt_timer =
        0.15f;

    enemy->state =
        ENEMY_STATE_HURT;
}


/*
 * ---------------------------------------------------------
 * Enemy Restore
 * ---------------------------------------------------------
 *
 * Restore one enemy from saved game state.
 *
 * Only the persistent runtime state is restored.
 * Navigation-related state is intentionally reset
 * because the dungeon is regenerated before this
 * function is called during loading.
 */

bool Enemy_Restore(
    int index,
    const Enemy *saved_enemy
)
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
        return true;
    }

    enemies[index] = *saved_enemy;

    /*
     * Reset transient navigation state.
     *
     * A* will calculate a fresh route after loading.
     */
    enemies[index].path_timer = 0.0f;

    enemies[index].waypoint_x =
        enemies[index].x;

    enemies[index].waypoint_y =
        enemies[index].y;

    enemies[index].path_valid = false;

    /*
     * Reset adaptive timer so the restored enemy
     * can evaluate the current behavior profile.
     */
    enemies[index].adaptive_timer =
        1.0f;

    /*
     * The enemy must not be restored as DEAD.
     * Dead enemies are removed from the active
     * enemy list by the FSM.
     */
    if (enemies[index].state ==
        ENEMY_STATE_DEAD)
    {
        enemies[index].active = false;
    }

    return true;
}