#include "combat.h"
#include "enemy.h"
#include "player.h"
#include "collision.h"

#include <math.h>

#define PLAYER_ATTACK_RANGE 55.0f
#define PLAYER_ATTACK_DAMAGE 25

void Combat_PlayerAttack(void)
{
    Player *player =
        Player_Get();

    if (player == 0)
        return;

    if (Player_IsDead())
        return;

    /*
     * Temporary attack direction:
     *
     * The player currently attacks to the right.
     *
     * Directional attacks will be added later
     * when the player facing system is implemented.
     */
    float attack_x =
        player->x + PLAYER_ATTACK_RANGE;

    float attack_y =
        player->y;

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
         * Calculate distance from the center
         * of the attack area to the enemy.
         */
        float dx =
            enemy->x - attack_x;

        float dy =
            enemy->y - attack_y;

        float distance =
            sqrtf(
                dx * dx +
                dy * dy
            );

        /*
         * Enemy radius is included so that
         * the attack can hit the enemy's body.
         */
        if (distance <=
            enemy->radius + 20.0f)
        {
            /*
             * All enemy damage goes through
             * Enemy_TakeDamage().
             *
             * This allows the enemy system to
             * handle HURT and DEAD states.
             */
            Enemy_TakeDamage(
                i,
                PLAYER_ATTACK_DAMAGE
            );
        }
    }
}