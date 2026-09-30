#include "player.h"
#include "input.h"
#include "collision.h"
#include "raylib.h"
#include "combat.h"
#include "items.h"
#include "events.h"
#include "audio.h"

#include <math.h>

static Player player;

#define PLAYER_MAX_HEALTH 100
#define HEALTH_POTION_HEAL 25

/*
 * ---------------------------------------------------------
 * Player Combat Visuals
 * ---------------------------------------------------------
 *
 * The current combat system attacks toward the right.
 *
 * Keep these values visually aligned with combat.c.
 */
#define PLAYER_ATTACK_RANGE 55.0f
#define PLAYER_RADIUS 20.0f

/*
 * ---------------------------------------------------------
 * Player Dodge
 * ---------------------------------------------------------
 *
 * The dodge is a short directional movement burst.
 *
 * The player becomes temporarily invulnerable during
 * the dodge and cannot immediately dodge again because
 * of the cooldown.
 */
#define PLAYER_DODGE_DISTANCE 90.0f
#define PLAYER_DODGE_DURATION 0.15f
#define PLAYER_DODGE_COOLDOWN 0.80f
#define PLAYER_DODGE_INVULNERABILITY 0.20f

#define PLAYER_DODGE_SPEED \
    (PLAYER_DODGE_DISTANCE / PLAYER_DODGE_DURATION)

/*
 * ---------------------------------------------------------
 * Player Visual Colors
 * ---------------------------------------------------------
 */

#define PLAYER_COLOR             (Color){45, 145, 245, 255}
#define PLAYER_COLOR_LIGHT       (Color){95, 190, 255, 255}
#define PLAYER_COLOR_DARK        (Color){20, 75, 155, 255}

#define PLAYER_OUTLINE           (Color){170, 225, 255, 255}

#define PLAYER_DODGE_COLOR       (Color){75, 215, 255, 255}
#define PLAYER_DODGE_GLOW        (Color){75, 190, 255, 255}

#define PLAYER_DAMAGE_COLOR      (Color){255, 65, 75, 255}

#define PLAYER_ATTACK_COLOR      (Color){255, 155, 45, 255}
#define PLAYER_ATTACK_GLOW       (Color){255, 205, 80, 255}

#define PLAYER_SHADOW_COLOR      (Color){0, 0, 0, 90}


/*
 * =========================================================
 * PLAYER INITIALIZATION
 * =========================================================
 */

void Player_Init(void)
{
    /*
     * Default position.
     *
     * Game_Init will later move the player to the
     * procedural dungeon spawn point using
     * Player_SetPosition().
     */
    player.x = 640.0f;
    player.y = 360.0f;

    player.speed = 200.0f;

    player.health = PLAYER_MAX_HEALTH;

    player.attack_cooldown = 0.35f;
    player.attack_timer = 0.0f;

    player.damage_timer = 0.0f;

    /*
     * Dodge state.
     */
    player.dodge_timer = 0.0f;

    player.dodge_cooldown_timer = 0.0f;

    player.dodge_invulnerability_timer = 0.0f;

    /*
     * Default facing/movement direction is right.
     *
     * This means pressing SHIFT immediately after
     * spawning will produce a predictable dodge.
     */
    player.last_move_x = 1.0f;
    player.last_move_y = 0.0f;

    player.dodging = false;

    player.dead = false;

    /*
     * Initialize the player's inventory.
     *
     * This also clears all previous items when
     * a new dungeon run begins.
     */
    Inventory_Init(
        &player.inventory
    );
}


void Player_SetPosition(
    float x,
    float y
)
{
    player.x = x;
    player.y = y;
}


/*
 * =========================================================
 * PLAYER UPDATE
 * =========================================================
 */

void Player_Update(void)
{
    /*
     * Dead players cannot move or attack.
     */
    if (player.dead)
        return;

    InputState input =
        Input_GetState();

    float dt =
        GetFrameTime();

    /*
     * -----------------------------------------------------
     * Timers
     * -----------------------------------------------------
     */

    if (player.damage_timer > 0.0f)
    {
        player.damage_timer -= dt;

        if (player.damage_timer < 0.0f)
            player.damage_timer = 0.0f;
    }

    if (player.dodge_cooldown_timer > 0.0f)
    {
        player.dodge_cooldown_timer -= dt;

        if (player.dodge_cooldown_timer < 0.0f)
            player.dodge_cooldown_timer = 0.0f;
    }

    if (player.dodge_invulnerability_timer > 0.0f)
    {
        player.dodge_invulnerability_timer -= dt;

        if (player.dodge_invulnerability_timer < 0.0f)
            player.dodge_invulnerability_timer = 0.0f;
    }

    /*
     * -----------------------------------------------------
     * Calculate Movement Direction
     * -----------------------------------------------------
     */

    float direction_x = 0.0f;
    float direction_y = 0.0f;

    if (input.move_up)
        direction_y -= 1.0f;

    if (input.move_down)
        direction_y += 1.0f;

    if (input.move_left)
        direction_x -= 1.0f;

    if (input.move_right)
        direction_x += 1.0f;

    /*
     * Normalize diagonal movement.
     */
    float direction_length =
        sqrtf(
            direction_x * direction_x +
            direction_y * direction_y
        );

    if (direction_length > 0.0f)
    {
        direction_x /=
            direction_length;

        direction_y /=
            direction_length;
    }

    /*
     * Remember the most recent movement direction.
     */
    if (direction_x != 0.0f ||
        direction_y != 0.0f)
    {
        player.last_move_x =
            direction_x;

        player.last_move_y =
            direction_y;
    }

    /*
     * -----------------------------------------------------
     * Start Dodge
     * -----------------------------------------------------
     */

    if (input.dodge &&
        !player.dodging &&
        player.dodge_cooldown_timer <= 0.0f)
    {
        float dodge_x =
            direction_x;

        float dodge_y =
            direction_y;

        /*
         * If no movement key is currently held,
         * use the last movement direction.
         */
        if (dodge_x == 0.0f &&
            dodge_y == 0.0f)
        {
            dodge_x =
                player.last_move_x;

            dodge_y =
                player.last_move_y;
        }

        /*
         * Safety fallback.
         */
        float dodge_length =
            sqrtf(
                dodge_x * dodge_x +
                dodge_y * dodge_y
            );

        if (dodge_length <= 0.0f)
        {
            dodge_x = 1.0f;
            dodge_y = 0.0f;
            dodge_length = 1.0f;
        }

        dodge_x /=
            dodge_length;

        dodge_y /=
            dodge_length;

        player.last_move_x =
            dodge_x;

        player.last_move_y =
            dodge_y;

        player.dodging = true;

        player.dodge_timer =
            PLAYER_DODGE_DURATION;

        player.dodge_cooldown_timer =
            PLAYER_DODGE_COOLDOWN;

        player.dodge_invulnerability_timer =
            PLAYER_DODGE_INVULNERABILITY;

        /*
         * -------------------------------------------------
         * Dodge Event
         * -------------------------------------------------
         */

        GameEvent dodge_event;

        dodge_event.type =
            EVENT_PLAYER_DODGE;

        dodge_event.dodge_direction =
            DODGE_NONE;

        dodge_event.move_direction =
            MOVE_NONE;

        /*
         * Convert dodge vector into primary
         * cardinal direction.
         */
        if (fabsf(dodge_x) >
            fabsf(dodge_y))
        {
            if (dodge_x < 0.0f)
            {
                dodge_event.dodge_direction =
                    DODGE_LEFT;
            }
            else
            {
                dodge_event.dodge_direction =
                    DODGE_RIGHT;
            }
        }
        else
        {
            if (dodge_y < 0.0f)
            {
                dodge_event.dodge_direction =
                    DODGE_UP;
            }
            else
            {
                dodge_event.dodge_direction =
                    DODGE_DOWN;
            }
        }

        dodge_event.value =
            PLAYER_DODGE_DISTANCE;

        dodge_event.x =
            player.x;

        dodge_event.y =
            player.y;

        Events_Push(
            dodge_event
        );
    }

    /*
     * -----------------------------------------------------
     * Active Dodge
     * -----------------------------------------------------
     */

    if (player.dodging)
    {
        float dodge_move =
            PLAYER_DODGE_SPEED *
            dt;

        float move_x =
            player.last_move_x *
            dodge_move;

        float move_y =
            player.last_move_y *
            dodge_move;

        /*
         * Move horizontally.
         */
        if (Collision_PlayerCanMove(
                player.x + move_x,
                player.y,
                PLAYER_RADIUS))
        {
            player.x += move_x;
        }

        /*
         * Move vertically.
         */
        if (Collision_PlayerCanMove(
                player.x,
                player.y + move_y,
                PLAYER_RADIUS))
        {
            player.y += move_y;
        }

        player.dodge_timer -= dt;

        if (player.dodge_timer <= 0.0f)
        {
            player.dodge_timer = 0.0f;
            player.dodging = false;
        }
    }
    else
    {
        /*
         * -------------------------------------------------
         * Normal Movement
         * -------------------------------------------------
         */

        float move_x =
            direction_x *
            player.speed *
            dt;

        float move_y =
            direction_y *
            player.speed *
            dt;

        /*
         * Move horizontally.
         */
        if (Collision_PlayerCanMove(
                player.x + move_x,
                player.y,
                PLAYER_RADIUS))
        {
            player.x += move_x;
        }

        /*
         * Move vertically.
         */
        if (Collision_PlayerCanMove(
                player.x,
                player.y + move_y,
                PLAYER_RADIUS))
        {
            player.y += move_y;
        }
    }

    /*
     * -----------------------------------------------------
     * Player Movement Event
     * -----------------------------------------------------
     */

    if (!player.dodging &&
        (direction_x != 0.0f ||
         direction_y != 0.0f))
    {
        GameEvent move_event;

        move_event.type =
            EVENT_PLAYER_MOVE;

        move_event.dodge_direction =
            DODGE_NONE;

        move_event.move_direction =
            MOVE_NONE;

        if (fabsf(direction_x) >
            fabsf(direction_y))
        {
            if (direction_x < 0.0f)
            {
                move_event.move_direction =
                    MOVE_LEFT;
            }
            else
            {
                move_event.move_direction =
                    MOVE_RIGHT;
            }
        }
        else
        {
            if (direction_y < 0.0f)
            {
                move_event.move_direction =
                    MOVE_UP;
            }
            else
            {
                move_event.move_direction =
                    MOVE_DOWN;
            }
        }

        move_event.value =
            direction_length;

        move_event.x =
            player.x;

        move_event.y =
            player.y;

        Events_Push(
            move_event
        );
    }

    /*
     * -----------------------------------------------------
     * Reduce attack cooldown
     * -----------------------------------------------------
     */

    if (player.attack_timer > 0.0f)
    {
        player.attack_timer -= dt;

        if (player.attack_timer < 0.0f)
            player.attack_timer = 0.0f;
    }

    /*
     * -----------------------------------------------------
     * Health Potion
     * -----------------------------------------------------
     */

    if (IsKeyPressed(KEY_H) &&
        player.health < PLAYER_MAX_HEALTH)
    {
        int potion_quantity =
            Inventory_GetQuantity(
                &player.inventory,
                ITEM_ID_HEALTH_POTION
            );

        if (potion_quantity > 0)
        {
            int old_health =
                player.health;

            player.health +=
                HEALTH_POTION_HEAL;

            if (player.health >
                PLAYER_MAX_HEALTH)
            {
                player.health =
                    PLAYER_MAX_HEALTH;
            }

            if (player.health > old_health)
            {
                Inventory_RemoveItem(
                    &player.inventory,
                    ITEM_ID_HEALTH_POTION,
                    1
                );

                /*
                 * Record the healing event.
                 */
                GameEvent heal_event;

                heal_event.type =
                    EVENT_PLAYER_HEAL;

                heal_event.dodge_direction =
                    DODGE_NONE;

                heal_event.move_direction =
                    MOVE_NONE;

                heal_event.value =
                    (float)(
                        player.health -
                        old_health
                    );

                heal_event.x =
                    player.x;

                heal_event.y =
                    player.y;

                Events_Push(
                    heal_event
                );
            }
        }
    }

    /*
     * -----------------------------------------------------
     * Melee Attack
     * -----------------------------------------------------
     */

    if (IsKeyPressed(KEY_SPACE) &&
        player.attack_timer <= 0.0f)
    {
        player.attack_timer =
            player.attack_cooldown;

        /*
         * Record the attack before executing
         * the actual combat operation.
         */
        GameEvent attack_event;

        attack_event.type =
            EVENT_PLAYER_ATTACK;

        attack_event.dodge_direction =
            DODGE_NONE;

        attack_event.move_direction =
            MOVE_NONE;

        attack_event.value =
            1.0f;

        attack_event.x =
            player.x;

        attack_event.y =
            player.y;

        Events_Push(
            attack_event
        );

        /*
         * Play the attack sound only after the
         * attack has passed the cooldown check.
         */
        Audio_PlayPlayerAttack();

        Combat_PlayerAttack();
    }
}


/*
 * =========================================================
 * PLAYER RENDER
 * =========================================================
 *
 * Phase 17 visual polish.
 *
 * This function does not affect gameplay.
 */

void Player_Render(void)
{
    /*
     * -----------------------------------------------------
     * Dead Player
     * -----------------------------------------------------
     */

    if (player.dead)
    {
        /*
         * Ground shadow.
         */
        DrawEllipse(
            (int)player.x,
            (int)player.y + 13,
            25.0f,
            8.0f,
            Fade(
                BLACK,
                0.35f
            )
        );

        /*
         * Dead body.
         */
        DrawCircle(
            (int)player.x,
            (int)player.y,
            PLAYER_RADIUS,
            (Color){55, 60, 70, 255}
        );

        DrawCircleLines(
            (int)player.x,
            (int)player.y,
            PLAYER_RADIUS + 1.5f,
            Fade(
                WHITE,
                0.35f
            )
        );

        /*
         * Red death ring.
         */
        DrawCircleLines(
            (int)player.x,
            (int)player.y,
            PLAYER_RADIUS + 5.0f,
            Fade(
                RED,
                0.65f
            )
        );

        /*
         * Red X communicates the dead state.
         */
        DrawLineEx(
            (Vector2){
                player.x - 9.0f,
                player.y - 9.0f
            },
            (Vector2){
                player.x + 9.0f,
                player.y + 9.0f
            },
            3.0f,
            RED
        );

        DrawLineEx(
            (Vector2){
                player.x + 9.0f,
                player.y - 9.0f
            },
            (Vector2){
                player.x - 9.0f,
                player.y + 9.0f
            },
            3.0f,
            RED
        );

        DrawText(
            "DEAD",
            (int)player.x - 18,
            (int)player.y - 38,
            14,
            RED
        );

        return;
    }

    /*
     * -----------------------------------------------------
     * Ground Shadow
     * -----------------------------------------------------
     *
     * Gives the player a stronger sense of presence
     * against the tile map.
     */
    DrawEllipse(
        (int)player.x,
        (int)player.y + 13,
        24.0f,
        8.0f,
        PLAYER_SHADOW_COLOR
    );

    /*
     * -----------------------------------------------------
     * Dodge Trail
     * -----------------------------------------------------
     *
     * Draw before the player body so the body remains
     * visually dominant.
     */
    if (player.dodging)
    {
        float dodge_progress =
            1.0f -
            (
                player.dodge_timer /
                PLAYER_DODGE_DURATION
            );

        if (dodge_progress < 0.0f)
            dodge_progress = 0.0f;

        if (dodge_progress > 1.0f)
            dodge_progress = 1.0f;

        /*
         * Trail extends opposite the dodge direction.
         */
        float trail_length =
            30.0f +
            dodge_progress * 14.0f;

        float trail_x =
            player.x -
            player.last_move_x *
            trail_length;

        float trail_y =
            player.y -
            player.last_move_y *
            trail_length;

        /*
         * Outer glow.
         */
        DrawLineEx(
            (Vector2){
                trail_x,
                trail_y
            },
            (Vector2){
                player.x,
                player.y
            },
            12.0f,
            Fade(
                PLAYER_DODGE_GLOW,
                0.18f
            )
        );

        /*
         * Main trail.
         */
        DrawLineEx(
            (Vector2){
                trail_x,
                trail_y
            },
            (Vector2){
                player.x,
                player.y
            },
            5.0f,
            Fade(
                PLAYER_DODGE_COLOR,
                0.50f
            )
        );

        /*
         * Dodge ring.
         */
        float ring_radius =
            PLAYER_RADIUS +
            6.0f +
            dodge_progress * 12.0f;

        DrawCircleLines(
            (int)player.x,
            (int)player.y,
            ring_radius,
            Fade(
                PLAYER_DODGE_COLOR,
                0.85f
            )
        );

        /*
         * Secondary ring.
         */
        DrawCircleLines(
            (int)player.x,
            (int)player.y,
            ring_radius + 5.0f,
            Fade(
                PLAYER_DODGE_COLOR,
                0.30f
            )
        );
    }

    /*
     * -----------------------------------------------------
     * Player Color
     * -----------------------------------------------------
     */

    Color player_color =
        PLAYER_COLOR;

    /*
     * Damage flash.
     */
    if (player.damage_timer > 0.0f)
    {
        int flash_phase =
            (int)(
                player.damage_timer *
                18.0f
            );

        if ((flash_phase % 2) == 0)
        {
            player_color =
                WHITE;
        }
        else
        {
            player_color =
                PLAYER_DAMAGE_COLOR;
        }
    }

    /*
     * Dodge overrides normal body color.
     */
    if (player.dodging)
    {
        player_color =
            PLAYER_DODGE_COLOR;
    }

    /*
     * -----------------------------------------------------
     * Player Outer Glow
     * -----------------------------------------------------
     */

    DrawCircle(
        (int)player.x,
        (int)player.y,
        PLAYER_RADIUS + 5.0f,
        Fade(
            player_color,
            0.10f
        )
    );

    /*
     * -----------------------------------------------------
     * Player Outer Body
     * -----------------------------------------------------
     */

    DrawCircle(
        (int)player.x,
        (int)player.y,
        PLAYER_RADIUS + 1.5f,
        PLAYER_OUTLINE
    );

    /*
     * Main body.
     */
    DrawCircle(
        (int)player.x,
        (int)player.y,
        PLAYER_RADIUS,
        player_color
    );

    /*
     * Lower dark shading.
     */
    DrawCircleSector(
        (Vector2){
            player.x,
            player.y
        },
        PLAYER_RADIUS - 1.0f,
        20.0f,
        160.0f,
        20,
        Fade(
            PLAYER_COLOR_DARK,
            0.55f
        )
    );

    /*
     * -----------------------------------------------------
     * Player Highlight
     * -----------------------------------------------------
     */

    DrawCircle(
        (int)player.x - 6,
        (int)player.y - 7,
        5.0f,
        Fade(
            WHITE,
            0.38f
        )
    );

    DrawCircle(
        (int)player.x - 8,
        (int)player.y - 9,
        2.0f,
        WHITE
    );

    /*
     * -----------------------------------------------------
     * Facing / Weapon Direction Indicator
     * -----------------------------------------------------
     *
     * The current combat system attacks to the right.
     */

    /*
     * Small dark connector.
     */
    DrawLineEx(
        (Vector2){
            player.x + 12.0f,
            player.y
        },
        (Vector2){
            player.x + 19.0f,
            player.y
        },
        5.0f,
        Fade(
            BLACK,
            0.35f
        )
    );

    /*
     * Bright direction marker.
     */
    DrawCircle(
        (int)player.x + 11,
        (int)player.y,
        4.5f,
        PLAYER_COLOR_LIGHT
    );

    /*
     * Tiny weapon-direction tip.
     */
    DrawTriangle(
        (Vector2){
            player.x + 18.0f,
            player.y
        },
        (Vector2){
            player.x + 13.0f,
            player.y - 3.5f
        },
        (Vector2){
            player.x + 13.0f,
            player.y + 3.5f
        },
        WHITE
    );

    /*
     * -----------------------------------------------------
     * Damage Feedback Ring
     * -----------------------------------------------------
     */

    if (player.damage_timer > 0.0f &&
        !player.dodging)
    {
        float damage_progress =
            player.damage_timer /
            0.75f;

        if (damage_progress < 0.0f)
            damage_progress = 0.0f;

        if (damage_progress > 1.0f)
            damage_progress = 1.0f;

        DrawCircleLines(
            (int)player.x,
            (int)player.y,
            PLAYER_RADIUS +
                4.0f +
                (1.0f - damage_progress) * 4.0f,
            Fade(
                PLAYER_DAMAGE_COLOR,
                0.85f
            )
        );
    }

    /*
     * -----------------------------------------------------
     * Melee Attack Visual
     * -----------------------------------------------------
     *
     * Purely visual.
     *
     * Combat_PlayerAttack() remains responsible for
     * actual hit detection.
     *
     * Attack lasts for the first 0.12 seconds
     * of the 0.35 second attack cooldown.
     */

    if (player.attack_timer >
        player.attack_cooldown - 0.12f)
    {
        float attack_progress =
            1.0f -
            (
                (
                    player.attack_timer -
                    (
                        player.attack_cooldown -
                        0.12f
                    )
                ) /
                0.12f
            );

        if (attack_progress < 0.0f)
            attack_progress = 0.0f;

        if (attack_progress > 1.0f)
            attack_progress = 1.0f;

        /*
         * -------------------------------------------------
         * Attack Glow
         * -------------------------------------------------
         */

        float glow_radius =
            35.0f +
            attack_progress * 8.0f;

        DrawCircle(
            (int)player.x + 32,
            (int)player.y,
            glow_radius * 0.55f,
            Fade(
                PLAYER_ATTACK_GLOW,
                0.10f
            )
        );

        /*
         * -------------------------------------------------
         * Attack Arc
         * -------------------------------------------------
         */

        float inner_radius =
            22.0f +
            attack_progress * 3.0f;

        float outer_radius =
            34.0f +
            attack_progress * 10.0f;

        /*
         * Outer soft arc.
         */
        DrawRing(
            (Vector2){
                player.x,
                player.y
            },
            inner_radius - 2.0f,
            outer_radius + 3.0f,
            300.0f,
            60.0f,
            18,
            Fade(
                PLAYER_ATTACK_GLOW,
                0.25f
            )
        );

        /*
         * Main orange attack arc.
         */
        DrawRing(
            (Vector2){
                player.x,
                player.y
            },
            inner_radius,
            outer_radius,
            300.0f,
            60.0f,
            18,
            Fade(
                PLAYER_ATTACK_COLOR,
                0.92f
            )
        );

        /*
         * Bright inner edge.
         */
        DrawRing(
            (Vector2){
                player.x,
                player.y
            },
            inner_radius + 1.0f,
            inner_radius + 3.0f,
            300.0f,
            60.0f,
            18,
            Fade(
                YELLOW,
                0.90f
            )
        );

        /*
         * -------------------------------------------------
         * Slash Lines
         * -------------------------------------------------
         */

        float slash_length =
            PLAYER_ATTACK_RANGE +
            8.0f;

        /*
         * Main white slash.
         */
        DrawLineEx(
            (Vector2){
                player.x + 16.0f,
                player.y - 13.0f
            },
            (Vector2){
                player.x + slash_length,
                player.y - 3.0f
            },
            7.0f,
            WHITE
        );

        /*
         * Orange secondary edge.
         */
        DrawLineEx(
            (Vector2){
                player.x + 19.0f,
                player.y + 7.0f
            },
            (Vector2){
                player.x + slash_length - 2.0f,
                player.y + 13.0f
            },
            4.0f,
            PLAYER_ATTACK_COLOR
        );

        /*
         * -------------------------------------------------
         * Impact Point
         * -------------------------------------------------
         */

        int impact_x =
            (int)player.x +
            (int)slash_length;

        int impact_y =
            (int)player.y + 4;

        DrawCircle(
            impact_x,
            impact_y,
            8.0f,
            Fade(
                YELLOW,
                0.20f
            )
        );

        DrawCircle(
            impact_x,
            impact_y,
            5.0f,
            YELLOW
        );

        DrawCircle(
            impact_x - 1,
            impact_y - 1,
            2.0f,
            WHITE
        );

        /*
         * Small spark.
         */
        DrawCircle(
            impact_x - 5,
            impact_y - 8,
            2.5f,
            WHITE
        );

        DrawCircle(
            impact_x + 3,
            impact_y + 7,
            2.0f,
            PLAYER_ATTACK_GLOW
        );
    }
}


/*
 * =========================================================
 * PLAYER ACCESS
 * =========================================================
 */

Player *Player_Get(void)
{
    return &player;
}


/*
 * =========================================================
 * PLAYER DAMAGE
 * =========================================================
 */

void Player_TakeDamage(int damage)
{
    /*
     * Dead players cannot receive more damage.
     */
    if (player.dead)
        return;

    /*
     * Dodge invulnerability.
     */
    if (player.dodge_invulnerability_timer > 0.0f)
        return;

    /*
     * Normal damage invulnerability period.
     */
    if (player.damage_timer > 0.0f)
        return;

    if (player.health <= 0)
        return;

    player.health -= damage;

    if (player.health < 0)
        player.health = 0;

    /*
     * Play the damage sound after damage has
     * actually been applied.
     */
    Audio_PlayPlayerDamage();

    /*
     * Record damage received.
     */
    GameEvent damage_event;

    damage_event.type =
        EVENT_PLAYER_DAMAGE;

    damage_event.dodge_direction =
        DODGE_NONE;

    damage_event.move_direction =
        MOVE_NONE;

    damage_event.value =
        (float)damage;

    damage_event.x =
        player.x;

    damage_event.y =
        player.y;

    Events_Push(
        damage_event
    );

    /*
     * Player has died.
     */
    if (player.health == 0)
    {
        player.dead = true;

        player.attack_timer = 0.0f;

        player.dodging = false;

        player.dodge_timer = 0.0f;

        return;
    }

    /*
     * Prevent damage every frame.
     */
    player.damage_timer = 0.75f;
}


bool Player_IsDead(void)
{
    return player.dead;
}