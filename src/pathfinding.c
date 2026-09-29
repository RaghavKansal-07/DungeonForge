#include "pathfinding.h"
#include "tilemap.h"

#include <math.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

#define MAX_NODES (MAP_WIDTH * MAP_HEIGHT)

typedef struct
{
    int x;
    int y;

    float g_cost;
    float h_cost;
    float f_cost;

    int parent_x;
    int parent_y;

    bool opened;
    bool closed;
} PathNode;


/*
 * ------------------------------------------------------------
 * Basic helpers
 * ------------------------------------------------------------
 */

static float Heuristic(
    int x1,
    int y1,
    int x2,
    int y2
)
{
    float dx =
        (float)(x2 - x1);

    float dy =
        (float)(y2 - y1);

    return sqrtf(
        dx * dx +
        dy * dy
    );
}


static bool IsValidTile(
    int x,
    int y
)
{
    return x >= 0 &&
           x < MAP_WIDTH &&
           y >= 0 &&
           y < MAP_HEIGHT;
}


static bool IsWalkable(
    int tile_x,
    int tile_y
)
{
    if (!IsValidTile(
            tile_x,
            tile_y))
    {
        return false;
    }

    return TileMap_GetTile(
        tile_x,
        tile_y
    ) == TILE_FLOOR;
}


/*
 * ------------------------------------------------------------
 * Physical waypoint validation
 * ------------------------------------------------------------
 */

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


static bool IsPositionSafe(
    float x,
    float y,
    float radius
)
{
    int min_tile_x =
        (int)floorf(
            (x - radius) /
            TILE_SIZE
        );

    int max_tile_x =
        (int)floorf(
            (x + radius) /
            TILE_SIZE
        );

    int min_tile_y =
        (int)floorf(
            (y - radius) /
            TILE_SIZE
        );

    int max_tile_y =
        (int)floorf(
            (y + radius) /
            TILE_SIZE
        );

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
                dx * dx +
                dy * dy;

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
 * ------------------------------------------------------------
 * Safe waypoint generation
 * ------------------------------------------------------------
 *
 * A* works with tiles.
 *
 * The actual enemy works with continuous world coordinates.
 *
 * Therefore we convert the A* tile into a physically safe
 * world-space waypoint.
 *
 * The search is deliberately more robust than the previous
 * implementation:
 *
 *     1. tile center
 *     2. points toward neighboring floor tiles
 *     3. several points around the tile
 *
 * This is important for larger enemies such as the boss.
 */

static bool FindSafeWaypoint(
    int tile_x,
    int tile_y,
    float radius,
    float goal_x,
    float goal_y,
    float *result_x,
    float *result_y
)
{
    if (!IsValidTile(
            tile_x,
            tile_y))
    {
        return false;
    }

    if (!IsWalkable(
            tile_x,
            tile_y))
    {
        return false;
    }

    float center_x =
        tile_x * TILE_SIZE +
        TILE_SIZE * 0.5f;

    float center_y =
        tile_y * TILE_SIZE +
        TILE_SIZE * 0.5f;

    /*
     * --------------------------------------------------------
     * Candidate positions
     * --------------------------------------------------------
     *
     * We search around the tile center rather than relying on
     * only a handful of hard-coded points.
     *
     * The offset is kept conservative so the point remains
     * inside the requested tile.
     */

    float candidate_x[49];
    float candidate_y[49];

    int candidate_count = 0;

    /*
     * Candidate 0:
     *
     * Exact tile center.
     */
    candidate_x[candidate_count] =
        center_x;

    candidate_y[candidate_count] =
        center_y;

    candidate_count++;

    /*
     * Generate a small grid around the center.
     *
     * This gives us more options in corridors and around
     * corners, especially for the boss radius.
     */
    const float offsets[] =
    {
        -24.0f,
        -16.0f,
        -8.0f,
         8.0f,
        16.0f,
        24.0f
    };

    const int offset_count =
        sizeof(offsets) /
        sizeof(offsets[0]);

    for (int y = 0;
         y < offset_count;
         y++)
    {
        for (int x = 0;
             x < offset_count;
             x++)
        {
            if (candidate_count >= 49)
                break;

            candidate_x[candidate_count] =
                center_x +
                offsets[x];

            candidate_y[candidate_count] =
                center_y +
                offsets[y];

            candidate_count++;
        }
    }

    /*
     * Add the four neighboring tile midpoints.
     *
     * These are particularly useful when travelling through
     * corridors.
     */
    static const int directions[4][2] =
    {
        {  1,  0 },
        { -1,  0 },
        {  0,  1 },
        {  0, -1 }
    };

    for (int i = 0;
         i < 4;
         i++)
    {
        int neighbor_x =
            tile_x +
            directions[i][0];

        int neighbor_y =
            tile_y +
            directions[i][1];

        if (!IsWalkable(
                neighbor_x,
                neighbor_y))
        {
            continue;
        }

        float neighbor_center_x =
            neighbor_x * TILE_SIZE +
            TILE_SIZE * 0.5f;

        float neighbor_center_y =
            neighbor_y * TILE_SIZE +
            TILE_SIZE * 0.5f;

        float midpoint_x =
            (center_x +
             neighbor_center_x) *
            0.5f;

        float midpoint_y =
            (center_y +
             neighbor_center_y) *
            0.5f;

        if (candidate_count < 49)
        {
            candidate_x[candidate_count] =
                midpoint_x;

            candidate_y[candidate_count] =
                midpoint_y;

            candidate_count++;
        }
    }

    /*
     * --------------------------------------------------------
     * Select the safest useful candidate.
     * --------------------------------------------------------
     *
     * We want the point closest to the actual navigation
     * goal, but only among physically valid points.
     */
    bool found = false;

    float best_distance_squared =
        INFINITY;

    for (int i = 0;
         i < candidate_count;
         i++)
    {
        float candidate_world_x =
            candidate_x[i];

        float candidate_world_y =
            candidate_y[i];

        /*
         * Keep candidates physically inside the tile.
         *
         * Neighbor midpoint candidates are allowed because
         * they are intentionally placed on the boundary between
         * two walkable tiles.
         */
        float tile_left =
            tile_x * TILE_SIZE;

        float tile_top =
            tile_y * TILE_SIZE;

        float tile_right =
            tile_left + TILE_SIZE;

        float tile_bottom =
            tile_top + TILE_SIZE;

        bool inside_tile =
            candidate_world_x >= tile_left &&
            candidate_world_x <= tile_right &&
            candidate_world_y >= tile_top &&
            candidate_world_y <= tile_bottom;

        if (!inside_tile)
            continue;

        if (!IsPositionSafe(
                candidate_world_x,
                candidate_world_y,
                radius))
        {
            continue;
        }

        float dx =
            goal_x -
            candidate_world_x;

        float dy =
            goal_y -
            candidate_world_y;

        float distance_squared =
            dx * dx +
            dy * dy;

        if (!found ||
            distance_squared <
                best_distance_squared)
        {
            found = true;

            best_distance_squared =
                distance_squared;

            *result_x =
                candidate_world_x;

            *result_y =
                candidate_world_y;
        }
    }

    /*
     * --------------------------------------------------------
     * Final fallback
     * --------------------------------------------------------
     *
     * If the candidate search failed, test the tile center
     * one final time.
     *
     * This keeps the function conservative while preventing
     * unnecessary path failures.
     */
    if (!found &&
        IsPositionSafe(
            center_x,
            center_y,
            radius))
    {
        *result_x =
            center_x;

        *result_y =
            center_y;

        return true;
    }

    return found;
}


/*
 * ------------------------------------------------------------
 * A* node selection
 * ------------------------------------------------------------
 */

static int FindLowestCostNode(
    PathNode nodes[MAP_HEIGHT][MAP_WIDTH]
)
{
    int best_x = -1;
    int best_y = -1;

    float best_f = INFINITY;

    for (int y = 0;
         y < MAP_HEIGHT;
         y++)
    {
        for (int x = 0;
         x < MAP_WIDTH;
         x++)
        {
            PathNode *node =
                &nodes[y][x];

            if (!node->opened ||
                node->closed)
            {
                continue;
            }

            if (node->f_cost < best_f)
            {
                best_f =
                    node->f_cost;

                best_x =
                    x;

                best_y =
                    y;
            }
        }
    }

    if (best_x < 0)
        return -1;

    return best_y * MAP_WIDTH +
           best_x;
}


/*
 * ------------------------------------------------------------
 * A* pathfinding
 * ------------------------------------------------------------
 */

bool Pathfinding_FindNextStep(
    float start_x,
    float start_y,
    float goal_x,
    float goal_y,
    float radius,
    float *next_x,
    float *next_y
)
{
    if (next_x == NULL ||
        next_y == NULL)
    {
        return false;
    }

    /*
     * Convert world coordinates to tile coordinates.
     */
    int start_tile_x =
        (int)floorf(
            start_x /
            TILE_SIZE
        );

    int start_tile_y =
        (int)floorf(
            start_y /
            TILE_SIZE
        );

    int goal_tile_x =
        (int)floorf(
            goal_x /
            TILE_SIZE
        );

    int goal_tile_y =
        (int)floorf(
            goal_y /
            TILE_SIZE
        );

    /*
     * Validate start.
     */
    if (!IsValidTile(
            start_tile_x,
            start_tile_y))
    {
        return false;
    }

    /*
     * Validate goal.
     */
    if (!IsValidTile(
            goal_tile_x,
            goal_tile_y))
    {
        return false;
    }

    /*
     * Enemy must be standing on floor.
     */
    if (!IsWalkable(
            start_tile_x,
            start_tile_y))
    {
        return false;
    }

    /*
     * If the player's/adaptive target's tile is a wall,
     * search for the nearest walkable tile.
     */
    int actual_goal_x =
        goal_tile_x;

    int actual_goal_y =
        goal_tile_y;

    if (!IsWalkable(
            actual_goal_x,
            actual_goal_y))
    {
        bool found_goal = false;

        float best_distance =
            INFINITY;

        for (int y = 0;
             y < MAP_HEIGHT;
             y++)
        {
            for (int x = 0;
                 x < MAP_WIDTH;
                 x++)
            {
                if (!IsWalkable(
                        x,
                        y))
                {
                    continue;
                }

                float dx =
                    (float)(
                        x -
                        goal_tile_x
                    );

                float dy =
                    (float)(
                        y -
                        goal_tile_y
                    );

                float distance =
                    dx * dx +
                    dy * dy;

                if (distance <
                    best_distance)
                {
                    best_distance =
                        distance;

                    actual_goal_x =
                        x;

                    actual_goal_y =
                        y;

                    found_goal = true;
                }
            }
        }

        if (!found_goal)
            return false;
    }

    /*
     * Enemy and goal are already in the same tile.
     */
    if (start_tile_x ==
            actual_goal_x &&
        start_tile_y ==
            actual_goal_y)
    {
        /*
         * Prefer the exact goal if it is physically safe.
         */
        if (IsPositionSafe(
                goal_x,
                goal_y,
                radius))
        {
            *next_x =
                goal_x;

            *next_y =
                goal_y;

            return true;
        }

        /*
         * Otherwise select a safe position inside the tile.
         */
        return FindSafeWaypoint(
            start_tile_x,
            start_tile_y,
            radius,
            goal_x,
            goal_y,
            next_x,
            next_y
        );
    }

    /*
     * Allocate the A* grid on the stack.
     */
    PathNode nodes[
        MAP_HEIGHT
    ][
        MAP_WIDTH
    ];

    /*
     * Initialize all nodes.
     */
    for (int y = 0;
         y < MAP_HEIGHT;
         y++)
    {
        for (int x = 0;
             x < MAP_WIDTH;
             x++)
        {
            nodes[y][x].x =
                x;

            nodes[y][x].y =
                y;

            nodes[y][x].g_cost =
                INFINITY;

            nodes[y][x].h_cost =
                0.0f;

            nodes[y][x].f_cost =
                INFINITY;

            nodes[y][x].parent_x =
                -1;

            nodes[y][x].parent_y =
                -1;

            nodes[y][x].opened =
                false;

            nodes[y][x].closed =
                false;
        }
    }

    /*
     * Initialize start node.
     */
    PathNode *start =
        &nodes[
            start_tile_y
        ][
            start_tile_x
        ];

    start->g_cost =
        0.0f;

    start->h_cost =
        Heuristic(
            start_tile_x,
            start_tile_y,
            actual_goal_x,
            actual_goal_y
        );

    start->f_cost =
        start->g_cost +
        start->h_cost;

    start->opened =
        true;

    /*
     * 8-direction navigation.
     */
    static const int directions[8][2] =
    {
        {  1,  0 },
        { -1,  0 },
        {  0,  1 },
        {  0, -1 },

        {  1,  1 },
        { -1,  1 },
        {  1, -1 },
        { -1, -1 }
    };

    bool found =
        false;

    /*
     * A* search.
     */
    for (int iteration = 0;
         iteration < MAX_NODES;
         iteration++)
    {
        int current_index =
            FindLowestCostNode(
                nodes
            );

        if (current_index < 0)
            break;

        int current_x =
            current_index %
            MAP_WIDTH;

        int current_y =
            current_index /
            MAP_WIDTH;

        PathNode *current =
            &nodes[
                current_y
            ][
                current_x
            ];

        /*
         * Goal reached.
         */
        if (current_x ==
                actual_goal_x &&
            current_y ==
                actual_goal_y)
        {
            found =
                true;

            break;
        }

        current->closed =
            true;

        /*
         * Examine neighbors.
         */
        for (int direction = 0;
             direction < 8;
             direction++)
        {
            int offset_x =
                directions[
                    direction
                ][0];

            int offset_y =
                directions[
                    direction
                ][1];

            int neighbor_x =
                current_x +
                offset_x;

            int neighbor_y =
                current_y +
                offset_y;

            if (!IsValidTile(
                    neighbor_x,
                    neighbor_y))
            {
                continue;
            }

            if (!IsWalkable(
                    neighbor_x,
                    neighbor_y))
            {
                continue;
            }

            /*
             * Prevent diagonal corner cutting.
             */
            if (offset_x != 0 &&
                offset_y != 0)
            {
                if (!IsWalkable(
                        current_x +
                            offset_x,
                        current_y))
                {
                    continue;
                }

                if (!IsWalkable(
                        current_x,
                        current_y +
                            offset_y))
                {
                    continue;
                }
            }

            PathNode *neighbor =
                &nodes[
                    neighbor_y
                ][
                    neighbor_x
                ];

            if (neighbor->closed)
                continue;

            float movement_cost =
                (offset_x != 0 &&
                 offset_y != 0)
                    ? 1.41421356f
                    : 1.0f;

            float tentative_g =
                current->g_cost +
                movement_cost;

            if (!neighbor->opened ||
                tentative_g <
                    neighbor->g_cost)
            {
                neighbor->parent_x =
                    current_x;

                neighbor->parent_y =
                    current_y;

                neighbor->g_cost =
                    tentative_g;

                neighbor->h_cost =
                    Heuristic(
                        neighbor_x,
                        neighbor_y,
                        actual_goal_x,
                        actual_goal_y
                    );

                neighbor->f_cost =
                    neighbor->g_cost +
                    neighbor->h_cost;

                neighbor->opened =
                    true;
            }
        }
    }

    if (!found)
        return false;

    /*
     * --------------------------------------------------------
     * Reconstruct first path step.
     * --------------------------------------------------------
     */

    int current_x =
        actual_goal_x;

    int current_y =
        actual_goal_y;

    int previous_x =
        -1;

    int previous_y =
        -1;

    while (current_x !=
               start_tile_x ||
           current_y !=
               start_tile_y)
    {
        PathNode *current =
            &nodes[
                current_y
            ][
                current_x
            ];

        previous_x =
            current_x;

        previous_y =
            current_y;

        current_x =
            current->parent_x;

        current_y =
            current->parent_y;

        if (!IsValidTile(
                current_x,
                current_y))
        {
            return false;
        }
    }

    /*
     * Convert the first A* tile into a physically safe
     * world-space waypoint.
     */
    return FindSafeWaypoint(
        previous_x,
        previous_y,
        radius,
        goal_x,
        goal_y,
        next_x,
        next_y
    );
}