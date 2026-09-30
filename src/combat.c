#include "combat.h"
#include "enemy.h"
#include "player.h"
#include "collision.h"

#include <math.h>

#define PLAYER_ATTACK_RANGE 55.0f
#define PLAYER_ATTACK_DAMAGE 25

/*
 * ---------------------------------------------------------
 * Melee Attack Shape
 * ---------------------------------------------------------
 *
 * The player currently faces right.
 *
 * The attack is treated as a forward-facing melee arc:
 *
 *              ENEMY
 *                *
 *              /
 *             /
 * PLAYER  ---->
 *
 * The visual attack effect in player.c uses the same
 * general direction.
 */
#define PLAYER_ATTACK_FORWARD_X 1.0f
#define PLAYER_ATTACK_FORWARD_Y 0.0f

/*
 * Cosine of the half-angle of the attack cone.
 *
 * 55 degrees gives a reasonably wide melee attack
 * without turning it into a full circular hit area.
 */
#define PLAYER_ATTACK_HALF_ANGLE_COS 0.573576f


/*
 * ---------------------------------------------------------
 * Combat Feedback State
 * ---------------------------------------------------------
 *
 * These values describe the result of the most recent
 * player attack.
 *
 * They are intentionally kept outside Player so combat
 * feedback does not become part of the player's persistent
 * state.
 */
static int last_hit_count = 0;


/*
 * ---------------------------------------------------------
 * Player Melee Attack
 * ---------------------------------------------------------
 */

void Combat_PlayerAttack(void)
{
    Player *player =
        Player_Get();

    if (player == 0)
        return;

    if (Player_IsDead())
        return;

    /*
     * Reset the result of the current attack.
     */
    last_hit_count = 0;

    /*
     * -----------------------------------------------------
     * Attack Direction
     * -----------------------------------------------------
     *
     * Directional facing will be added later.
     *
     * For now the player attacks to the right.
     */
    const float attack_direction_x =
        PLAYER_ATTACK_FORWARD_X;

    const float attack_direction_y =
        PLAYER_ATTACK_FORWARD_Y;

    /*
     * -----------------------------------------------------
     * Check Every Enemy
     * -----------------------------------------------------
     */
    for (int i = 0;
         i < MAX_ENEMIES;
         i++)
    {
        Enemy *enemy =
            Enemy_Get(i);

        if (enemy == 0)
            continue;

        if (!enemy->active)
            continue;

        if (enemy->health <= 0)
            continue;

        /*
         * -------------------------------------------------
         * Vector from player to enemy
         * -------------------------------------------------
         */
        float dx =
            enemy->x - player->x;

        float dy =
            enemy->y - player->y;

        float distance_squared =
            dx * dx +
            dy * dy;

        /*
         * -------------------------------------------------
         * Range Check
         * -------------------------------------------------
         *
         * Include the enemy radius so that the player's
         * attack can hit the edge of an enemy rather than
         * requiring the enemy center to be inside the
         * attack range.
         */
        float maximum_distance =
            PLAYER_ATTACK_RANGE +
            enemy->radius;

        if (distance_squared >
            maximum_distance *
            maximum_distance)
        {
            continue;
        }

        /*
         * -------------------------------------------------
         * Distance
         * -------------------------------------------------
         */
        float distance =
            sqrtf(
                distance_squared
            );

        /*
         * Extremely close enemies are considered
         * automatically inside the attack area.
         */
        if (distance <= 0.001f)
        {
            Enemy_TakeDamage(
                i,
                PLAYER_ATTACK_DAMAGE
            );

            last_hit_count++;

            continue;
        }

        /*
         * -------------------------------------------------
         * Normalize Player -> Enemy Direction
         * -------------------------------------------------
         */
        float enemy_direction_x =
            dx / distance;

        float enemy_direction_y =
            dy / distance;

        /*
         * -------------------------------------------------
         * Forward-Facing Attack Cone
         * -------------------------------------------------
         *
         * Dot product:
         *
         *   1.0  = directly in front
         *   0.0  = 90 degrees away
         *  -1.0  = directly behind
         *
         * The enemy must be inside the configured
         * forward-facing attack cone.
         */
        float dot =
            attack_direction_x *
                enemy_direction_x +
            attack_direction_y *
                enemy_direction_y;

        if (dot <
            PLAYER_ATTACK_HALF_ANGLE_COS)
        {
            continue;
        }

        /*
         * -------------------------------------------------
         * Enemy Hit
         * -------------------------------------------------
         */
        Enemy_TakeDamage(
            i,
            PLAYER_ATTACK_DAMAGE
        );

        last_hit_count++;
    }
}


/*
 * ---------------------------------------------------------
 * Combat Feedback
 * ---------------------------------------------------------
 *
 * Returns the number of enemies hit by the most recent
 * player attack.
 *
 * This will allow later systems to trigger:
 *
 *   - hit sounds
 *   - stronger impact effects
 *   - combo feedback
 *   - screen shake
 *   - hit counters
 *
 * without modifying the actual damage calculation.
 */
int Combat_GetLastHitCount(void)
{
    return last_hit_count;
}