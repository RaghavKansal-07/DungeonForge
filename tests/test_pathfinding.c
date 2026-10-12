#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

#include "pathfinding.h"
#include "tilemap.h"

static int tests_run = 0;
static int tests_passed = 0;

static void Test_Assert(
    bool condition,
    const char *message)
{
    tests_run++;

    if (condition)
    {
        tests_passed++;
        printf("[PASS] %s\n", message);
    }
    else
    {
        printf("[FAIL] %s\n", message);
    }
}

static bool IsFinitePosition(
    float x,
    float y)
{
    return isfinite(x) &&
           isfinite(y);
}

static bool IsWalkablePosition(
    float x,
    float y)
{
    int tile_x =
        (int)(x / TILE_SIZE);

    int tile_y =
        (int)(y / TILE_SIZE);

    if (tile_x < 0 ||
        tile_x >= MAP_WIDTH ||
        tile_y < 0 ||
        tile_y >= MAP_HEIGHT)
    {
        return false;
    }

    return TileMap_GetTile(
               tile_x,
               tile_y) == TILE_FLOOR;
}

int main(void)
{
    TileMap_Init();

    /*
     * Create a deterministic test map.
     *
     * Start with walls everywhere, then create
     * a small connected floor area.
     */
    for (int y = 0; y < MAP_HEIGHT; y++)
    {
        for (int x = 0; x < MAP_WIDTH; x++)
        {
            TileMap_SetTile(
                x,
                y,
                TILE_WALL);
        }
    }

    /*
     * Open a small 5x5 floor area.
     */
    for (int y = 8; y <= 12; y++)
    {
        for (int x = 8; x <= 12; x++)
        {
            TileMap_SetTile(
                x,
                y,
                TILE_FLOOR);
        }
    }

    float next_x = 0.0f;
    float next_y = 0.0f;

    /*
     * Invalid output pointers must fail safely.
     */
    Test_Assert(
        !Pathfinding_FindNextStep(
            100.0f,
            100.0f,
            200.0f,
            200.0f,
            10.0f,
            NULL,
            &next_y),
        "Rejects NULL X output");

    Test_Assert(
        !Pathfinding_FindNextStep(
            100.0f,
            100.0f,
            200.0f,
            200.0f,
            10.0f,
            &next_x,
            NULL),
        "Rejects NULL Y output");

    /*
     * Invalid start positions must fail.
     */
    Test_Assert(
        !Pathfinding_FindNextStep(
            -100.0f,
            -100.0f,
            300.0f,
            300.0f,
            10.0f,
            &next_x,
            &next_y),
        "Rejects invalid start position");

    /*
     * Invalid goal positions must fail.
     */
    Test_Assert(
        !Pathfinding_FindNextStep(
            9.0f * TILE_SIZE + TILE_SIZE * 0.5f,
            9.0f * TILE_SIZE + TILE_SIZE * 0.5f,
            -100.0f,
            -100.0f,
            10.0f,
            &next_x,
            &next_y),
        "Rejects invalid goal position");

    /*
     * Verify that our deterministic map contains
     * the expected floor tile.
     */
    Test_Assert(
        TileMap_GetTile(9, 9) == TILE_FLOOR,
        "Test map contains floor tile");

    /*
     * Same-tile navigation should succeed.
     */
    float start_x =
        9.0f * TILE_SIZE +
        TILE_SIZE * 0.5f;

    float start_y =
        9.0f * TILE_SIZE +
        TILE_SIZE * 0.5f;

    Test_Assert(
        Pathfinding_FindNextStep(
            start_x,
            start_y,
            start_x,
            start_y,
            10.0f,
            &next_x,
            &next_y),
        "Same-tile pathfinding succeeds");

    Test_Assert(
        IsFinitePosition(
            next_x,
            next_y),
        "Same-tile result is finite");

    /*
     * Pathfinding between two connected floor tiles.
     */
    float goal_x =
        12.0f * TILE_SIZE +
        TILE_SIZE * 0.5f;

    float goal_y =
        12.0f * TILE_SIZE +
        TILE_SIZE * 0.5f;

    Test_Assert(
        Pathfinding_FindNextStep(
            start_x,
            start_y,
            goal_x,
            goal_y,
            10.0f,
            &next_x,
            &next_y),
        "Finds path through connected floor tiles");

    Test_Assert(
        IsFinitePosition(
            next_x,
            next_y),
        "Pathfinding result is finite");

    Test_Assert(
        IsWalkablePosition(
            next_x,
            next_y),
        "Pathfinding result is walkable");

    /*
     * Start from a floor tile and request a goal
     * completely surrounded by walls.
     *
     * The implementation may relocate a wall goal
     * to the nearest walkable tile, so the important
     * property here is that the call remains safe.
     */
    TileMap_SetTile(
        20,
        10,
        TILE_WALL);

    Test_Assert(
        Pathfinding_FindNextStep(
            start_x,
            start_y,
            20.0f * TILE_SIZE + TILE_SIZE * 0.5f,
            10.0f * TILE_SIZE + TILE_SIZE * 0.5f,
            10.0f,
            &next_x,
            &next_y),
        "Handles wall goal by searching for walkable goal");

    Test_Assert(
        IsFinitePosition(
            next_x,
            next_y),
        "Wall-goal path result is finite");

    Test_Assert(
        IsWalkablePosition(
            next_x,
            next_y),
        "Wall-goal result is walkable");

    /*
     * Completely isolated goal should fail safely.
     */
    for (int y = 20; y <= 22; y++)
    {
        for (int x = 20; x <= 22; x++)
        {
            TileMap_SetTile(
                x,
                y,
                TILE_FLOOR);
        }
    }

    Test_Assert(
        !Pathfinding_FindNextStep(
            start_x,
            start_y,
            21.0f * TILE_SIZE + TILE_SIZE * 0.5f,
            21.0f * TILE_SIZE + TILE_SIZE * 0.5f,
            10.0f,
            &next_x,
            &next_y),
        "Rejects unreachable isolated goal");

    /*
     * ---------------------------------------------------------
     * Routing around an obstacle
     * ---------------------------------------------------------
     *
     * A floor area split by a wall row with a single gap at
     * the far right. The goal is directly below the start, so
     * the route must go east toward the gap first.
     */
    for (int y = 0; y < MAP_HEIGHT; y++)
    {
        for (int x = 0; x < MAP_WIDTH; x++)
            TileMap_SetTile(x, y, TILE_WALL);
    }

    for (int y = 2; y <= 20; y++)
    {
        for (int x = 2; x <= 30; x++)
            TileMap_SetTile(x, y, TILE_FLOOR);
    }

    for (int x = 2; x <= 29; x++)
        TileMap_SetTile(x, 10, TILE_WALL);

    float wall_start_x = 5.0f * TILE_SIZE + TILE_SIZE * 0.5f;
    float wall_start_y = 5.0f * TILE_SIZE + TILE_SIZE * 0.5f;
    float wall_goal_x = 5.0f * TILE_SIZE + TILE_SIZE * 0.5f;
    float wall_goal_y = 15.0f * TILE_SIZE + TILE_SIZE * 0.5f;

    bool routed =
        Pathfinding_FindNextStep(
            wall_start_x,
            wall_start_y,
            wall_goal_x,
            wall_goal_y,
            10.0f,
            &next_x,
            &next_y);

    Test_Assert(
        routed,
        "Finds a route around a wall that blocks the direct line");

    Test_Assert(
        routed &&
            next_x > wall_start_x &&
            next_y <= 9.0f * TILE_SIZE,
        "First step heads toward the gap and not into the wall");

    /*
     * ---------------------------------------------------------
     * Crossing a large open map
     * ---------------------------------------------------------
     */
    for (int y = 0; y < MAP_HEIGHT; y++)
    {
        for (int x = 0; x < MAP_WIDTH; x++)
        {
            bool border =
                x == 0 || y == 0 ||
                x == MAP_WIDTH - 1 ||
                y == MAP_HEIGHT - 1;

            TileMap_SetTile(
                x,
                y,
                border ? TILE_WALL : TILE_FLOOR);
        }
    }

    float far_start_x = 1.0f * TILE_SIZE + TILE_SIZE * 0.5f;
    float far_start_y = 1.0f * TILE_SIZE + TILE_SIZE * 0.5f;
    float far_goal_x = (MAP_WIDTH - 2) * TILE_SIZE + TILE_SIZE * 0.5f;
    float far_goal_y = (MAP_HEIGHT - 2) * TILE_SIZE + TILE_SIZE * 0.5f;

    bool crossed =
        Pathfinding_FindNextStep(
            far_start_x,
            far_start_y,
            far_goal_x,
            far_goal_y,
            10.0f,
            &next_x,
            &next_y);

    float before_dx = far_goal_x - far_start_x;
    float before_dy = far_goal_y - far_start_y;
    float after_dx = far_goal_x - next_x;
    float after_dy = far_goal_y - next_y;

    Test_Assert(
        crossed &&
            after_dx * after_dx + after_dy * after_dy <
                before_dx * before_dx + before_dy * before_dy,
        "Corner-to-corner search on the full map steps closer to the goal");

    /*
     * ---------------------------------------------------------
     * Long winding corridor (stress test for the open list)
     * ---------------------------------------------------------
     */
    for (int y = 0; y < MAP_HEIGHT; y++)
    {
        for (int x = 0; x < MAP_WIDTH; x++)
            TileMap_SetTile(x, y, TILE_WALL);
    }

    int last_row = 0;

    for (int row = 0; row < 12; row++)
    {
        int y = 2 + row * 4;

        last_row = y;

        for (int x = 2; x <= MAP_WIDTH - 3; x++)
            TileMap_SetTile(x, y, TILE_FLOOR);

        if (row < 11)
        {
            int connector_x =
                (row % 2 == 0)
                    ? MAP_WIDTH - 3
                    : 2;

            for (int y2 = y; y2 <= y + 4; y2++)
                TileMap_SetTile(connector_x, y2, TILE_FLOOR);
        }
    }

    float snake_start_x = 2.0f * TILE_SIZE + TILE_SIZE * 0.5f;
    float snake_start_y = 2.0f * TILE_SIZE + TILE_SIZE * 0.5f;
    float snake_goal_x = 2.0f * TILE_SIZE + TILE_SIZE * 0.5f;
    float snake_goal_y = (float)last_row * TILE_SIZE + TILE_SIZE * 0.5f;

    clock_t timer_start = clock();

    bool snaked =
        Pathfinding_FindNextStep(
            snake_start_x,
            snake_start_y,
            snake_goal_x,
            snake_goal_y,
            10.0f,
            &next_x,
            &next_y);

    double snake_ms =
        (double)(clock() - timer_start) * 1000.0 /
        (double)CLOCKS_PER_SEC;

    Test_Assert(
        snaked && next_x > snake_start_x,
        "Finds a path through a 12-row winding corridor");

    printf(
        "      (winding-corridor search took %.2f ms)\n",
        snake_ms);

    printf(
        "\nPathfinding: %d/%d passed\n",
        tests_passed,
        tests_run);

    return tests_passed == tests_run ? 0 : 1;
}