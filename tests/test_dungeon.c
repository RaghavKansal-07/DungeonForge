#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "dungeon.h"
#include "tilemap.h"

/*
 * ---------------------------------------------------------
 * TEST HELPERS
 * ---------------------------------------------------------
 */

static int tests_run = 0;
static int tests_passed = 0;


static void Test_Assert(
    bool condition,
    const char *test_name
)
{
    tests_run++;

    if (condition)
    {
        tests_passed++;

        printf("[PASS] %s\n", test_name);
    }
    else
    {
        printf("[FAIL] %s\n", test_name);
    }
}


static Dungeon dungeon_a;
static Dungeon dungeon_b;

static TileType tiles_a[MAP_HEIGHT][MAP_WIDTH];
static TileType tiles_b[MAP_HEIGHT][MAP_WIDTH];


static void SnapshotTiles(
    TileType out[MAP_HEIGHT][MAP_WIDTH]
)
{
    for (int y = 0; y < MAP_HEIGHT; y++)
    {
        for (int x = 0; x < MAP_WIDTH; x++)
            out[y][x] = TileMap_GetTile(x, y);
    }
}


static bool SameLayout(
    const Dungeon *a,
    const Dungeon *b
)
{
    if (a->room_count != b->room_count ||
        a->connection_count != b->connection_count)
    {
        return false;
    }

    for (int i = 0; i < a->room_count; i++)
    {
        if (a->rooms[i].x != b->rooms[i].x ||
            a->rooms[i].y != b->rooms[i].y ||
            a->rooms[i].width != b->rooms[i].width ||
            a->rooms[i].height != b->rooms[i].height)
        {
            return false;
        }
    }

    for (int i = 0; i < a->connection_count; i++)
    {
        if (a->connections[i].room_a != b->connections[i].room_a ||
            a->connections[i].room_b != b->connections[i].room_b)
        {
            return false;
        }
    }

    return true;
}


static bool RoomsIntersect(
    const DungeonRoom *a,
    const DungeonRoom *b
)
{
    return a->x < b->x + b->width &&
           b->x < a->x + a->width &&
           a->y < b->y + b->height &&
           b->y < a->y + a->height;
}


/*
 * ---------------------------------------------------------
 * DETERMINISM
 * ---------------------------------------------------------
 */

static void Test_Determinism(void)
{
    Dungeon_Init(&dungeon_a, 12345u);
    SnapshotTiles(tiles_a);

    Dungeon_Init(&dungeon_b, 12345u);
    SnapshotTiles(tiles_b);

    Test_Assert(
        SameLayout(&dungeon_a, &dungeon_b),
        "Same seed produces the same rooms and connections"
    );

    Test_Assert(
        memcmp(tiles_a, tiles_b, sizeof(tiles_a)) == 0,
        "Same seed produces the same tile map"
    );

    Test_Assert(
        Dungeon_GetSeed(&dungeon_a) == 12345u,
        "Dungeon remembers the seed that generated it"
    );
}


static void Test_DifferentSeeds(void)
{
    int different = 0;

    Dungeon_Init(&dungeon_a, 1u);

    for (unsigned int seed = 2u; seed <= 21u; seed++)
    {
        Dungeon_Init(&dungeon_b, seed);

        if (!SameLayout(&dungeon_a, &dungeon_b))
            different++;

        dungeon_a = dungeon_b;
    }

    Test_Assert(
        different >= 19,
        "Consecutive seeds produce different dungeons"
    );
}


/*
 * ---------------------------------------------------------
 * STRUCTURAL PROPERTIES (many seeds)
 * ---------------------------------------------------------
 *
 * Failures are counted inside the loop and asserted once
 * afterwards, so each property is one honest check.
 */

static void Test_StructuralProperties(void)
{
    int bad_bounds = 0;
    int bad_overlap = 0;
    int bad_edges = 0;
    int unreachable = 0;
    int bad_spawn = 0;
    int bad_center = 0;
    int bad_farthest = 0;
    int large_enough = 0;

    for (unsigned int seed = 1u; seed <= 200u; seed++)
    {
        Dungeon_Init(&dungeon_a, seed);

        const Dungeon *d = &dungeon_a;

        if (d->room_count >= 10)
            large_enough++;

        for (int i = 0; i < d->room_count; i++)
        {
            const DungeonRoom *room = &d->rooms[i];

            if (room->x < 1 ||
                room->y < 1 ||
                room->x + room->width > MAP_WIDTH - 1 ||
                room->y + room->height > MAP_HEIGHT - 1)
            {
                bad_bounds++;
            }

            if (TileMap_GetTile(
                    room->center_x,
                    room->center_y) != TILE_FLOOR)
            {
                bad_center++;
            }

            for (int j = i + 1; j < d->room_count; j++)
            {
                if (RoomsIntersect(room, &d->rooms[j]))
                    bad_overlap++;
            }

            if (Dungeon_GetRoomDistance(d, 0, i) < 0)
                unreachable++;
        }

        if (d->room_count >= 1 &&
            d->connection_count != d->room_count - 1)
        {
            bad_edges++;
        }

        float spawn_x = 0.0f;
        float spawn_y = 0.0f;

        if (d->room_count >= 1)
        {
            if (!Dungeon_GetPlayerSpawn(d, &spawn_x, &spawn_y) ||
                TileMap_GetTile(
                    (int)(spawn_x / TILE_SIZE),
                    (int)(spawn_y / TILE_SIZE)) != TILE_FLOOR)
            {
                bad_spawn++;
            }
        }

        if (d->room_count >= 2)
        {
            int far_room = Dungeon_GetFarthestRoom(d, 0);

            if (far_room <= 0 ||
                far_room >= d->room_count)
            {
                bad_farthest++;
            }
            else
            {
                int far_distance =
                    Dungeon_GetRoomDistance(d, 0, far_room);

                for (int i = 0; i < d->room_count; i++)
                {
                    if (Dungeon_GetRoomDistance(d, 0, i) >
                        far_distance)
                    {
                        bad_farthest++;
                        break;
                    }
                }
            }
        }
    }

    Test_Assert(bad_bounds == 0,
        "Every room stays inside the map border (200 seeds)");

    Test_Assert(bad_overlap == 0,
        "No two rooms overlap (200 seeds)");

    Test_Assert(bad_center == 0,
        "Every room center is floor (200 seeds)");

    Test_Assert(bad_edges == 0,
        "The dungeon graph is a tree: rooms - 1 connections (200 seeds)");

    Test_Assert(unreachable == 0,
        "Every room is reachable from the first room (200 seeds)");

    Test_Assert(bad_spawn == 0,
        "The player spawn is always on a floor tile (200 seeds)");

    Test_Assert(bad_farthest == 0,
        "The farthest room is valid and no room is farther (200 seeds)");

    Test_Assert(large_enough > 0,
        "Some seeds produce at least 10 rooms, so the retry rule can succeed");
}


/*
 * ---------------------------------------------------------
 * DISTANCE HELPERS
 * ---------------------------------------------------------
 */

static void Test_DistanceHelpers(void)
{
    Dungeon_Init(&dungeon_a, 12345u);

    Test_Assert(
        Dungeon_GetRoomDistance(&dungeon_a, 0, 0) == 0,
        "A room is 0 hops from itself"
    );

    Test_Assert(
        Dungeon_GetRoomDistance(&dungeon_a, 0, -1) == -1,
        "Distance to a negative room index is invalid"
    );

    Test_Assert(
        Dungeon_GetRoomDistance(
            &dungeon_a, 0, dungeon_a.room_count) == -1,
        "Distance to a room index past the end is invalid"
    );

    Test_Assert(
        Dungeon_GetRoomDistance(NULL, 0, 0) == -1,
        "Distance on a NULL dungeon is invalid"
    );

    Test_Assert(
        Dungeon_GetFarthestRoom(NULL, 0) == -1,
        "Farthest room of a NULL dungeon is invalid"
    );

    Test_Assert(
        Dungeon_GetFarthestRoom(&dungeon_a, 99) == -1,
        "Farthest room from an invalid room is invalid"
    );

    int neighbour = Dungeon_GetConnectedRoom(&dungeon_a, 0, 0);

    Test_Assert(
        neighbour >= 0 &&
        Dungeon_GetRoomDistance(&dungeon_a, 0, neighbour) == 1,
        "A directly connected room is exactly 1 hop away"
    );
}


/*
 * ---------------------------------------------------------
 * MAIN TEST RUNNER
 * ---------------------------------------------------------
 */

int main(void)
{
    printf("\n");
    printf("========================================\n");
    printf(" DungeonForge - Dungeon Tests\n");
    printf("========================================\n\n");

    Test_Determinism();

    Test_DifferentSeeds();

    Test_StructuralProperties();

    Test_DistanceHelpers();

    printf("\n========================================\n");
    printf(
        "Tests: %d | Passed: %d | Failed: %d\n",
        tests_run,
        tests_passed,
        tests_run - tests_passed
    );
    printf("========================================\n\n");

    return tests_run == tests_passed ? 0 : 1;
}