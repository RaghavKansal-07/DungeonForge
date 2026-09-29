#include "collision.h"
#include "tilemap.h"
#include "enemy.h"
#include "player.h"

#include <math.h>
#include <stddef.h>

static float ClampFloat(
    float value,
    float min,
    float max
)
{
    if (value < min)
        return min;

    if (value > max)
        return max;

    return value;
}


bool Collision_CanMove(
    float x,
    float y,
    float radius
)
{
    /*
     * Determine which tiles can possibly
     * intersect the entity.
     */
    int min_tile_x =
        (int)floorf((x - radius) / TILE_SIZE);

    int max_tile_x =
        (int)floorf((x + radius) / TILE_SIZE);

    int min_tile_y =
        (int)floorf((y - radius) / TILE_SIZE);

    int max_tile_y =
        (int)floorf((y + radius) / TILE_SIZE);

    for (int tile_y = min_tile_y;
         tile_y <= max_tile_y;
         tile_y++)
    {
        for (int tile_x = min_tile_x;
             tile_x <= max_tile_x;
             tile_x++)
        {
            if (TileMap_GetTile(
                    tile_x,
                    tile_y) != TILE_WALL)
            {
                continue;
            }

            float wall_left =
                tile_x * TILE_SIZE;

            float wall_top =
                tile_y * TILE_SIZE;

            float wall_right =
                wall_left + TILE_SIZE;

            float wall_bottom =
                wall_top + TILE_SIZE;

            float closest_x =
                ClampFloat(
                    x,
                    wall_left,
                    wall_right
                );

            float closest_y =
                ClampFloat(
                    y,
                    wall_top,
                    wall_bottom
                );

            float dx =
                x - closest_x;

            float dy =
                y - closest_y;

            float distance_squared =
                dx * dx + dy * dy;

            if (distance_squared <=
                radius * radius)
            {
                return false;
            }
        }
    }

    return true;
}


/*
 * -------------------------------------------------
 * Check whether a proposed position is moving
 * away from another entity.
 *
 * This is important for avoiding a deadlock where
 * two entities are already touching in a narrow
 * corridor.
 *
 * A movement is considered "away" when the proposed
 * position has a greater distance from the other
 * entity than the current position.
 * -------------------------------------------------
 */
static bool IsMovingAway(
    float current_x,
    float current_y,
    float proposed_x,
    float proposed_y,
    float other_x,
    float other_y
)
{
    float current_dx =
        current_x - other_x;

    float current_dy =
        current_y - other_y;

    float proposed_dx =
        proposed_x - other_x;

    float proposed_dy =
        proposed_y - other_y;

    float current_distance_squared =
        current_dx * current_dx +
        current_dy * current_dy;

    float proposed_distance_squared =
        proposed_dx * proposed_dx +
        proposed_dy * proposed_dy;

    return proposed_distance_squared >
           current_distance_squared;
}


bool Collision_PlayerCanMove(
    float x,
    float y,
    float radius
)
{
    /*
     * Check walls first.
     */
    if (!Collision_CanMove(
            x,
            y,
            radius))
    {
        return false;
    }

    /*
     * Check every active enemy.
     */
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

        if (enemy->state ==
            ENEMY_STATE_DEAD)
        {
            continue;
        }

        float dx =
            x - enemy->x;

        float dy =
            y - enemy->y;

        float minimum_distance =
            radius + enemy->radius;

        float minimum_distance_squared =
            minimum_distance *
            minimum_distance;

        /*
         * Normal collision.
         */
        if (dx * dx + dy * dy <
            minimum_distance_squared)
        {
            /*
             * -------------------------------------------------
             * Deadlock escape:
             *
             * If the player is already blocked by the enemy
             * but is moving away from it, allow the movement.
             *
             * This lets the player escape from a narrow
             * corridor instead of becoming permanently trapped.
             * -------------------------------------------------
             */
            Player *player =
                Player_Get();

            if (player != NULL &&
                IsMovingAway(
                    player->x,
                    player->y,
                    x,
                    y,
                    enemy->x,
                    enemy->y))
            {
                continue;
            }

            return false;
        }
    }

    return true;
}


bool Collision_EnemyCanMove(
    int enemy_index,
    float x,
    float y,
    float radius
)
{
    /*
     * -------------------------------------------------
     * 1. Wall collision
     * -------------------------------------------------
     */
    if (!Collision_CanMove(
            x,
            y,
            radius))
    {
        return false;
    }


    /*
     * -------------------------------------------------
     * 2. Enemy cannot move into the player
     * -------------------------------------------------
     */
    Player *player =
        Player_Get();

    if (player != NULL &&
        !player->dead)
    {
        float dx =
            x - player->x;

        float dy =
            y - player->y;

        float minimum_distance =
            radius + 20.0f;

        float minimum_distance_squared =
            minimum_distance *
            minimum_distance;

        if (dx * dx + dy * dy <
            minimum_distance_squared)
        {
            /*
             * -------------------------------------------------
             * Deadlock escape:
             *
             * If the enemy is moving away from the player,
             * allow it to move even though the entities have
             * not completely separated yet.
             *
             * This is especially important for a boss trapped
             * behind the player in a narrow corridor.
             * -------------------------------------------------
             */
            Enemy *enemy =
                Enemy_Get(enemy_index);

            if (enemy != NULL &&
                IsMovingAway(
                    enemy->x,
                    enemy->y,
                    x,
                    y,
                    player->x,
                    player->y))
            {
                /*
                 * The enemy is deliberately moving away
                 * from the player, so do not create a
                 * permanent movement deadlock.
                 */
            }
            else
            {
                return false;
            }
        }
    }


    /*
     * -------------------------------------------------
     * 3. Enemy cannot move into another enemy
     * -------------------------------------------------
     *
     * Enemy collision normally remains a hard constraint.
     *
     * The important exception is when an enemy is already
     * extremely close/overlapping and its proposed movement
     * is demonstrably moving AWAY from that other enemy.
     *
     * Without this exception, Collision_SeparateEnemies()
     * cannot perform a small progressive correction:
     *
     *     25% overlap correction -> rejected
     *     50% overlap correction -> rejected
     *     75% overlap correction -> rejected
     *     100% correction        -> accepted
     *
     * That all-or-nothing behavior can make two enemies
     * fight each other every frame in a narrow corridor.
     *
     * Allowing only outward movement lets the separation
     * solver converge smoothly without allowing enemies
     * to push through one another.
     */
    for (int i = 0;
         i < MAX_ENEMIES;
         i++)
    {
        if (i == enemy_index)
            continue;

        Enemy *other =
            Enemy_Get(i);

        if (other == NULL)
            continue;

        if (!other->active)
            continue;

        if (other->state ==
            ENEMY_STATE_DEAD)
        {
            continue;
        }

        float dx =
            x - other->x;

        float dy =
            y - other->y;

        float minimum_distance =
            radius + other->radius;

        float distance_squared =
            dx * dx + dy * dy;

        if (distance_squared <
            minimum_distance *
            minimum_distance)
        {
            Enemy *enemy =
                Enemy_Get(enemy_index);

            if (enemy != NULL &&
                IsMovingAway(
                    enemy->x,
                    enemy->y,
                    x,
                    y,
                    other->x,
                    other->y))
            {
                /*
                 * This is a controlled separation movement.
                 * Allow it even though the entities are still
                 * slightly overlapping so the solver can converge.
                 */
                continue;
            }

            return false;
        }
    }

    return true;
}


bool Collision_HasLineOfSight(
    float x1,
    float y1,
    float x2,
    float y2
)
{
    float dx =
        x2 - x1;

    float dy =
        y2 - y1;

    float distance =
        sqrtf(
            dx * dx +
            dy * dy
        );

    if (distance <= 0.0f)
        return true;

    int steps =
        (int)(distance / 4.0f);

    if (steps < 1)
        steps = 1;

    for (int i = 0;
         i <= steps;
         i++)
    {
        float t =
            (float)i /
            (float)steps;

        float x =
            x1 + dx * t;

        float y =
            y1 + dy * t;

        int tile_x =
            (int)floorf(
                x / TILE_SIZE
            );

        int tile_y =
            (int)floorf(
                y / TILE_SIZE
            );

        if (TileMap_GetTile(
                tile_x,
                tile_y) == TILE_WALL)
        {
            return false;
        }
    }

    return true;
}


void Collision_SeparateEnemies(void)
{
    /*
     * Resolve enemy overlaps after movement.
     *
     * The important difference from the old
     * implementation is that we do not immediately
     * try to move both enemies by half of the entire
     * overlap.
     *
     * Instead, the lower-index enemy has priority
     * and the other enemy yields first.
     *
     * This makes the resolution deterministic and
     * prevents two enemies from repeatedly blocking
     * each other.
     */
    for (int iteration = 0;
         iteration < 8;
         iteration++)
    {
        bool any_overlap = false;

        for (int i = 0;
             i < MAX_ENEMIES;
             i++)
        {
            Enemy *a =
                Enemy_Get(i);

            if (a == NULL ||
                !a->active ||
                a->state == ENEMY_STATE_DEAD)
            {
                continue;
            }

            for (int j = i + 1;
                 j < MAX_ENEMIES;
                 j++)
            {
                Enemy *b =
                    Enemy_Get(j);

                if (b == NULL ||
                    !b->active ||
                    b->state == ENEMY_STATE_DEAD)
                {
                    continue;
                }

                float dx =
                    b->x - a->x;

                float dy =
                    b->y - a->y;

                float distance_squared =
                    dx * dx +
                    dy * dy;

                float minimum_distance =
                    a->radius + b->radius;

                float minimum_distance_squared =
                    minimum_distance *
                    minimum_distance;

                if (distance_squared >=
                    minimum_distance_squared)
                {
                    continue;
                }

                any_overlap = true;

                float distance;

                /*
                 * Handle exact overlap.
                 */
                if (distance_squared <
                    0.0001f)
                {
                    dx = 1.0f;
                    dy = 0.0f;
                    distance = 1.0f;
                }
                else
                {
                    distance =
                        sqrtf(
                            distance_squared
                        );
                }

                float overlap =
                    minimum_distance -
                    distance;

                float normal_x =
                    dx / distance;

                float normal_y =
                    dy / distance;


                /*
                 * -------------------------------------------------
                 * First: try to move B away.
                 *
                 * B has to yield because A has the lower index.
                 * -------------------------------------------------
                 *
                 * Use a sequence of progressively larger pushes.
                 *
                 * This is deliberately conservative so that
                 * separation does not suddenly teleport an enemy
                 * through a corner.
                 */
                bool separated = false;

                const float push_factors[] =
                {
                    0.25f,
                    0.50f,
                    0.75f,
                    1.00f
                };

                const int push_count =
                    sizeof(push_factors) /
                    sizeof(push_factors[0]);

                for (int p = 0;
                     p < push_count;
                     p++)
                {
                    float push =
                        overlap *
                        push_factors[p];

                    float new_bx =
                        b->x +
                        normal_x *
                        push;

                    float new_by =
                        b->y +
                        normal_y *
                        push;

                    if (Collision_EnemyCanMove(
                            j,
                            new_bx,
                            new_by,
                            b->radius))
                    {
                        b->x =
                            new_bx;

                        b->y =
                            new_by;

                        separated = true;

                        break;
                    }
                }


                if (separated)
                    continue;


                /*
                 * -------------------------------------------------
                 * B could not move.
                 *
                 * Try moving A instead.
                 * -------------------------------------------------
                 */
                for (int p = 0;
                     p < push_count;
                     p++)
                {
                    float push =
                        overlap *
                        push_factors[p];

                    float new_ax =
                        a->x -
                        normal_x *
                        push;

                    float new_ay =
                        a->y -
                        normal_y *
                        push;

                    if (Collision_EnemyCanMove(
                            i,
                            new_ax,
                            new_ay,
                            a->radius))
                    {
                        a->x =
                            new_ax;

                        a->y =
                            new_ay;

                        separated = true;

                        break;
                    }
                }


                /*
                 * If neither enemy can move, leave the
                 * positions untouched for this iteration.
                 *
                 * This can happen legitimately when the
                 * enemies are trapped between walls/player.
                 */
                if (!separated)
                    continue;
            }
        }

        /*
         * Stop early when no overlaps remain.
         */
        if (!any_overlap)
            break;
    }
}
