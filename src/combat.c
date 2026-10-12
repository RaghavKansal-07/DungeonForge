#include "combat.h"

#include "attack_cone.h"
#include "enemy.h"
#include "player.h"
#include "collision.h"
#include "equipment.h"

#include <math.h>
#include <stddef.h>

/*
 * ---------------------------------------------------------
 * Combat Feedback State
 * ---------------------------------------------------------
 *
 * The result of the most recent player attack.
 *
 * It is kept outside Player so combat feedback does not
 * become part of the player's persistent state.
 */
static int last_hit_count = 0;

/*
 * ---------------------------------------------------------
 * Player Melee Attack
 * ---------------------------------------------------------
 *
 * The attack is a cone in the direction the player aims.
 * An enemy is hit when:
 *
 *   1. it is inside the cone (see attack_cone.c), and
 *   2. there is a clear line between the player and it,
 *      so attacks cannot pass through walls.
 *
 * Damage includes the Iron Sword bonus when the player owns one.
 */

void Combat_PlayerAttack(
    float direction_x,
    float direction_y)
{
    Player *player =
        Player_Get();

    if (player == NULL)
        return;

    if (Player_IsDead())
        return;

    last_hit_count = 0;

    int damage =
        Equipment_GetAttackDamage(
            &player->inventory,
            PLAYER_ATTACK_DAMAGE);

    for (int i = 0;
         i < MAX_ENEMIES;
         i++)
    {
        Enemy *enemy =
            Enemy_Get(i);

        if (enemy == NULL)
            continue;

        if (!enemy->active)
            continue;

        if (enemy->health <= 0)
            continue;

        if (!AttackCone_Contains(
                player->x,
                player->y,
                direction_x,
                direction_y,
                enemy->x,
                enemy->y,
                enemy->radius,
                PLAYER_ATTACK_RANGE,
                PLAYER_ATTACK_HALF_ANGLE_DEGREES))
        {
            continue;
        }

        if (!Collision_HasLineOfSight(
                player->x,
                player->y,
                enemy->x,
                enemy->y))
        {
            continue;
        }

        Enemy_TakeDamage(
            i,
            damage);

        last_hit_count++;
    }
}

/*
 * ---------------------------------------------------------
 * Combat Feedback
 * ---------------------------------------------------------
 */
int Combat_GetLastHitCount(void)
{
    return last_hit_count;
}

float Combat_GetNearestEnemyDistance(
    float max_distance)
{
    Player *player =
        Player_Get();

    if (player == NULL)
        return -1.0f;

    float nearest = -1.0f;

    for (int i = 0;
         i < MAX_ENEMIES;
         i++)
    {
        Enemy *enemy =
            Enemy_Get(i);

        if (enemy == NULL ||
            !enemy->active ||
            enemy->health <= 0 ||
            enemy->state == ENEMY_STATE_DEAD)
        {
            continue;
        }

        float dx = enemy->x - player->x;
        float dy = enemy->y - player->y;

        float distance =
            sqrtf(dx * dx + dy * dy);

        if (distance > max_distance)
            continue;

        if (nearest < 0.0f ||
            distance < nearest)
        {
            nearest = distance;
        }
    }

    return nearest;
}