#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "pathfinding.h"
#include "tilemap.h"

static int tests_run = 0;
static int tests_passed = 0;

static void Test_Assert(
    bool condition,
    const char *message
)
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
    float y
)
{
    return isfinite(x) &&
           isfinite(y);
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
                TILE_WALL
            );
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
                TILE_FLOOR
            );
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
        "Rejects NULL X output"
    );

    Test_Assert(
        !Pathfinding_FindNextStep(
            100.0f,
            100.0f,
            200.0f,
            200.0f,
            10.0f,
            &next_x,
            NULL),
        "Rejects NULL Y output"
    );

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
        "Rejects invalid start position"
    );

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
        "Rejects invalid goal position"
    );

    /*
     * Verify that our deterministic map contains
     * the expected floor tile.
     */
    Test_Assert(
        TileMap_GetTile(9, 9) == TILE_FLOOR,
        "Test map contains floor tile"
    );

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
        "Same-tile pathfinding succeeds"
    );

    Test_Assert(
        IsFinitePosition(
            next_x,
            next_y),
        "Same-tile result is finite"
    );

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
        "Finds path through connected floor tiles"
    );

    Test_Assert(
        IsFinitePosition(
            next_x,
            next_y),
        "Pathfinding result is finite"
    );

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
        TILE_WALL
    );

    Test_Assert(
        Pathfinding_FindNextStep(
            start_x,
            start_y,
            20.0f * TILE_SIZE + TILE_SIZE * 0.5f,
            10.0f * TILE_SIZE + TILE_SIZE * 0.5f,
            10.0f,
            &next_x,
            &next_y),
        "Handles wall goal by searching for walkable goal"
    );

    Test_Assert(
        IsFinitePosition(
            next_x,
            next_y),
        "Wall-goal path result is finite"
    );

    printf(
        "\nPathfinding: %d/%d passed\n",
        tests_passed,
        tests_run
    );

    return tests_passed == tests_run ? 0 : 1;
}