#include "player.h"
#include "input.h"
#include "collision.h"
#include "raylib.h"
#include "combat.h"
#include "items.h"
#include "events.h"

#include <math.h>

static Player player;

#define PLAYER_MAX_HEALTH 100
#define HEALTH_POTION_HEAL 25

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
     * Reduce damage invulnerability timer.
     */
    if (player.damage_timer > 0.0f)
    {
        player.damage_timer -= dt;

        if (player.damage_timer < 0.0f)
            player.damage_timer = 0.0f;
    }

    /*
     * Calculate movement direction.
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
     *
     * Without normalization:
     *
     *   W  = speed
     *   W+D = speed * sqrt(2)
     *
     * This would make diagonal movement approximately
     * 41% faster.
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
     * -----------------------------------------------------
     * Player Movement Event
     * -----------------------------------------------------
     *
     * Record the direction in which the player is moving.
     *
     * The Behavior Analyzer will later use these events
     * to determine movement preferences and positioning
     * behavior.
     */
    if (direction_x != 0.0f ||
        direction_y != 0.0f)
    {
        GameEvent move_event;

        move_event.type =
            EVENT_PLAYER_MOVE;

        move_event.dodge_direction =
            DODGE_NONE;

        move_event.move_direction =
            MOVE_NONE;

        /*
         * Convert the normalized movement vector
         * into a primary movement direction.
         */
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
        20.0f))
    {
        player.x += move_x;
    }

    /*
     * Move vertically.
     */
    if (Collision_PlayerCanMove(
        player.x,
        player.y + move_y,
        20.0f))
    {
        player.y += move_y;
    }

    /*
     * Reduce attack cooldown.
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
     *
     * H consumes one potion and restores
     * up to 25 HP.
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

            /*
             * Only consume the potion if
             * the player's health actually
             * increased.
             */
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

        Combat_PlayerAttack();
    }
}

void Player_Render(void)
{
    /*
     * Change appearance when dead.
     */
    if (player.dead)
    {
        DrawCircle(
            (int)player.x,
            (int)player.y,
            20.0f,
            DARKGRAY
        );

        DrawText(
            "DEAD",
            (int)player.x - 18,
            (int)player.y - 35,
            14,
            RED
        );

        return;
    }

    /*
     * Player body.
     */
    DrawCircle(
        (int)player.x,
        (int)player.y,
        20.0f,
        BLUE
    );

    /*
     * Health display.
     */
    DrawText(
        TextFormat(
            "HP: %d",
            player.health
        ),
        (int)player.x - 25,
        (int)player.y + 30,
        14,
        WHITE
    );

    /*
     * Display melee attack.
     */
    if (player.attack_timer >
        player.attack_cooldown - 0.10f)
    {
        DrawRectangle(
            (int)player.x + 20,
            (int)player.y - 15,
            35,
            30,
            RED
        );
    }
}

Player *Player_Get(void)
{
    return &player;
}

void Player_TakeDamage(int damage)
{
    /*
     * Dead players cannot receive more damage.
     */
    if (player.dead)
        return;

    /*
     * Invulnerability period.
     */
    if (player.damage_timer > 0.0f)
        return;

    if (player.health <= 0)
        return;

    player.health -= damage;

    if (player.health < 0)
        player.health = 0;

    /*
     * Record damage received.
     *
     * This event is generated regardless of whether
     * the damage kills the player.
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