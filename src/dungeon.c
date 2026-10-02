#include "dungeon.h"
#include "tilemap.h"

#include <stdlib.h>
#include <stdbool.h>

#define MIN_ROOM_WIDTH  5
#define MIN_ROOM_HEIGHT 4

#define MAX_ROOM_WIDTH  9
#define MAX_ROOM_HEIGHT 7

#define ROOM_PADDING 1

#define MAX_ROOM_ATTEMPTS 100

/*
 * Corridors are two tiles wide.
 *
 * TILE_SIZE = 32
 * Corridor width = 64 pixels
 *
 * This is wide enough for the player's
 * 40 pixel diameter.
 */
#define CORRIDOR_WIDTH 2


static int RandomRange(
    int min,
    int max
)
{
    return min +
           rand() % (max - min + 1);
}


static bool Dungeon_RoomsOverlap(
    const DungeonRoom *a,
    const DungeonRoom *b
)
{
    /*
     * Add a one-tile padding around each room.
     *
     * This prevents rooms from being placed
     * directly against each other.
     */
    int a_left =
        a->x - ROOM_PADDING;

    int a_right =
        a->x +
        a->width +
        ROOM_PADDING;

    int a_top =
        a->y - ROOM_PADDING;

    int a_bottom =
        a->y +
        a->height +
        ROOM_PADDING;

    int b_left =
        b->x - ROOM_PADDING;

    int b_right =
        b->x +
        b->width +
        ROOM_PADDING;

    int b_top =
        b->y - ROOM_PADDING;

    int b_bottom =
        b->y +
        b->height +
        ROOM_PADDING;

    if (a_right <= b_left ||
        a_left >= b_right ||
        a_bottom <= b_top ||
        a_top >= b_bottom)
    {
        return false;
    }

    return true;
}


static bool Dungeon_CanPlaceRoom(
    const Dungeon *dungeon,
    const DungeonRoom *room
)
{
    for (int i = 0;
         i < dungeon->room_count;
         i++)
    {
        if (Dungeon_RoomsOverlap(
                room,
                &dungeon->rooms[i]))
        {
            return false;
        }
    }

    return true;
}


static void Dungeon_CalculateRoomCenter(
    DungeonRoom *room
)
{
    room->center_x =
        room->x +
        room->width / 2;

    room->center_y =
        room->y +
        room->height / 2;
}


static void Dungeon_CarveRoom(
    const DungeonRoom *room
)
{
    for (int y = room->y;
         y < room->y + room->height;
         y++)
    {
        for (int x = room->x;
             x < room->x + room->width;
             x++)
        {
            TileMap_SetTile(
                x,
                y,
                TILE_FLOOR
            );
        }
    }
}


/*
 * Carve a horizontal corridor that is
 * CORRIDOR_WIDTH tiles wide.
 */
static void Dungeon_CarveHorizontalCorridor(
    int x1,
    int x2,
    int y
)
{
    int start_x = x1;
    int end_x = x2;

    if (start_x > end_x)
    {
        int temp = start_x;
        start_x = end_x;
        end_x = temp;
    }

    for (int x = start_x;
         x <= end_x;
         x++)
    {
        for (int offset = 0;
             offset < CORRIDOR_WIDTH;
             offset++)
        {
            TileMap_SetTile(
                x,
                y + offset,
                TILE_FLOOR
            );
        }
    }
}


/*
 * Carve a vertical corridor that is
 * CORRIDOR_WIDTH tiles wide.
 */
static void Dungeon_CarveVerticalCorridor(
    int y1,
    int y2,
    int x
)
{
    int start_y = y1;
    int end_y = y2;

    if (start_y > end_y)
    {
        int temp = start_y;
        start_y = end_y;
        end_y = temp;
    }

    for (int y = start_y;
         y <= end_y;
         y++)
    {
        for (int offset = 0;
             offset < CORRIDOR_WIDTH;
             offset++)
        {
            TileMap_SetTile(
                x + offset,
                y,
                TILE_FLOOR
            );
        }
    }
}


static void Dungeon_CarveCorridor(
    const DungeonRoom *a,
    const DungeonRoom *b
)
{
    int x1 = a->center_x;
    int y1 = a->center_y;

    int x2 = b->center_x;
    int y2 = b->center_y;

    /*
     * Create an L-shaped corridor.
     *
     * First move horizontally, then vertically.
     *
     * Both sections are two tiles wide.
     *
     * The overlap at the corner creates a
     * two-by-two walkable turning area.
     */
    Dungeon_CarveHorizontalCorridor(
        x1,
        x2,
        y1
    );

    Dungeon_CarveVerticalCorridor(
        y1,
        y2,
        x2
    );
}


/*
 * Calculate squared distance between
 * the centers of two rooms.
 *
 * Squared distance is sufficient because
 * we only need to compare distances.
 */
static int Dungeon_RoomDistanceSquared(
    const DungeonRoom *a,
    const DungeonRoom *b
)
{
    int dx =
        a->center_x -
        b->center_x;

    int dy =
        a->center_y -
        b->center_y;

    return dx * dx + dy * dy;
}


/*
 * Add a connection to the dungeon graph.
 *
 * room_a and room_b are room indices.
 */
static void Dungeon_AddConnection(
    Dungeon *dungeon,
    int room_a,
    int room_b
)
{
    if (dungeon == NULL)
        return;

    /*
     * Never create an invalid room reference.
     */
    if (room_a < 0 ||
        room_a >= dungeon->room_count ||
        room_b < 0 ||
        room_b >= dungeon->room_count)
    {
        return;
    }

    /*
     * A room cannot connect to itself.
     */
    if (room_a == room_b)
        return;

    /*
     * Prevent the connection array from
     * overflowing.
     */
    if (dungeon->connection_count >=
        MAX_DUNGEON_CONNECTIONS)
    {
        return;
    }

    /*
     * Avoid duplicate connections.
     *
     * Since the graph is undirected:
     *
     *     2 -> 5
     *
     * is the same as:
     *
     *     5 -> 2
     */
    for (int i = 0;
         i < dungeon->connection_count;
         i++)
    {
        DungeonConnection *connection =
            &dungeon->connections[i];

        if ((connection->room_a == room_a &&
             connection->room_b == room_b) ||
            (connection->room_a == room_b &&
             connection->room_b == room_a))
        {
            return;
        }
    }

    DungeonConnection *connection =
        &dungeon->connections[
            dungeon->connection_count
        ];

    connection->room_a = room_a;
    connection->room_b = room_b;

    dungeon->connection_count++;
}


/*
 * Connect all rooms using a spatially-aware
 * minimum-spanning-tree style algorithm.
 *
 * We begin with room 0.
 *
 * At every step, find the shortest connection
 * between:
 *
 *     an already-connected room
 *
 * and:
 *
 *     an unconnected room.
 *
 * This guarantees that:
 *
 *     1. Every room becomes reachable.
 *     2. The graph contains no unnecessary cycles.
 *     3. Connections are based on room position
 *        rather than generation order.
 */
static void Dungeon_ConnectRooms(
    Dungeon *dungeon
)
{
    if (dungeon == NULL)
        return;

    /*
     * If there is only one room, there is
     * nothing to connect.
     */
    if (dungeon->room_count <= 1)
        return;

    /*
     * Start the connected network with room 0.
     */
    dungeon->rooms[0].connected = true;

    int connected_count = 1;

    /*
     * A tree containing N rooms requires
     * exactly N - 1 connections.
     */
    while (connected_count <
           dungeon->room_count)
    {
        int best_room_a = -1;
        int best_room_b = -1;

        int best_distance = 0;

        /*
         * Search every pair where one room is
         * already connected and the other is not.
         */
        for (int room_a = 0;
             room_a < dungeon->room_count;
             room_a++)
        {
            if (!dungeon->rooms[
                    room_a
                ].connected)
            {
                continue;
            }

            for (int room_b = 0;
                 room_b < dungeon->room_count;
                 room_b++)
            {
                if (dungeon->rooms[
                        room_b
                    ].connected)
                {
                    continue;
                }

                int distance =
                    Dungeon_RoomDistanceSquared(
                        &dungeon->rooms[room_a],
                        &dungeon->rooms[room_b]
                    );

                if (best_room_a == -1 ||
                    distance < best_distance)
                {
                    best_distance =
                        distance;

                    best_room_a =
                        room_a;

                    best_room_b =
                        room_b;
                }
            }
        }

        /*
         * No valid connection could be found.
         *
         * This should never happen because every
         * generated room is a valid candidate.
         */
        if (best_room_a == -1 ||
            best_room_b == -1)
        {
            break;
        }

        /*
         * Carve the physical corridor.
         */
        Dungeon_CarveCorridor(
            &dungeon->rooms[best_room_a],
            &dungeon->rooms[best_room_b]
        );

        /*
         * Mark the new room as part of the
         * connected dungeon network.
         */
        dungeon->rooms[
            best_room_b
        ].connected = true;

        connected_count++;

        /*
         * Store the same relationship in the
         * abstract dungeon graph.
         */
        Dungeon_AddConnection(
            dungeon,
            best_room_a,
            best_room_b
        );
    }
}


static void Dungeon_GenerateRooms(
    Dungeon *dungeon
)
{
    int attempts = 0;

    while (dungeon->room_count <
               MAX_DUNGEON_ROOMS &&
           attempts <
               MAX_ROOM_ATTEMPTS)
    {
        attempts++;

        DungeonRoom room;

        room.width =
            RandomRange(
                MIN_ROOM_WIDTH,
                MAX_ROOM_WIDTH
            );

        room.height =
            RandomRange(
                MIN_ROOM_HEIGHT,
                MAX_ROOM_HEIGHT
            );

        /*
         * Leave at least one wall tile around
         * the dungeon boundary.
         */
        int max_x =
            MAP_WIDTH -
            room.width -
            2;

        int max_y =
            MAP_HEIGHT -
            room.height -
            2;

        if (max_x < 1 ||
            max_y < 1)
        {
            continue;
        }

        room.x =
            RandomRange(
                1,
                max_x
            );

        room.y =
            RandomRange(
                1,
                max_y
            );

        room.connected = false;

        Dungeon_CalculateRoomCenter(
            &room
        );

        /*
         * Only accept the room if it does not
         * overlap an existing room.
         */
        if (!Dungeon_CanPlaceRoom(
                dungeon,
                &room))
        {
            continue;
        }

        dungeon->rooms[
            dungeon->room_count
        ] = room;

        dungeon->room_count++;

        Dungeon_CarveRoom(
            &room
        );
    }
}


/*
 * Check whether two rooms share a direct
 * edge in the dungeon graph.
 */
bool Dungeon_AreRoomsConnected(
    const Dungeon *dungeon,
    int room_a,
    int room_b
)
{
    if (dungeon == NULL)
        return false;

    /*
     * Validate room indices.
     */
    if (room_a < 0 ||
        room_a >= dungeon->room_count ||
        room_b < 0 ||
        room_b >= dungeon->room_count)
    {
        return false;
    }

    /*
     * A room cannot be directly connected
     * to itself.
     */
    if (room_a == room_b)
        return false;

    /*
     * Search the undirected edge list.
     */
    for (int i = 0;
         i < dungeon->connection_count;
         i++)
    {
        const DungeonConnection *connection =
            &dungeon->connections[i];

        if ((connection->room_a == room_a &&
             connection->room_b == room_b) ||
            (connection->room_a == room_b &&
             connection->room_b == room_a))
        {
            return true;
        }
    }

    return false;
}


/*
 * Count how many rooms are directly
 * connected to the specified room.
 */
int Dungeon_GetRoomConnectionCount(
    const Dungeon *dungeon,
    int room_index
)
{
    if (dungeon == NULL)
        return 0;

    /*
     * Validate the room index.
     */
    if (room_index < 0 ||
        room_index >= dungeon->room_count)
    {
        return 0;
    }

    int count = 0;

    for (int i = 0;
         i < dungeon->connection_count;
         i++)
    {
        const DungeonConnection *connection =
            &dungeon->connections[i];

        if (connection->room_a == room_index ||
            connection->room_b == room_index)
        {
            count++;
        }
    }

    return count;
}


/*
 * Get a specific neighboring room.
 *
 * connection_index is the index among the
 * rooms directly connected to room_index.
 */
int Dungeon_GetConnectedRoom(
    const Dungeon *dungeon,
    int room_index,
    int connection_index
)
{
    if (dungeon == NULL)
        return -1;

    /*
     * Validate the room index.
     */
    if (room_index < 0 ||
        room_index >= dungeon->room_count)
    {
        return -1;
    }

    /*
     * Negative connection indices are invalid.
     */
    if (connection_index < 0)
        return -1;

    int current_connection = 0;

    for (int i = 0;
         i < dungeon->connection_count;
         i++)
    {
        const DungeonConnection *connection =
            &dungeon->connections[i];

        int connected_room = -1;

        if (connection->room_a == room_index)
        {
            connected_room =
                connection->room_b;
        }
        else if (connection->room_b == room_index)
        {
            connected_room =
                connection->room_a;
        }

        /*
         * This connection does not involve
         * the requested room.
         */
        if (connected_room == -1)
            continue;

        /*
         * Return the requested neighboring room.
         */
        if (current_connection ==
            connection_index)
        {
            return connected_room;
        }

        current_connection++;
    }

    /*
     * Requested connection does not exist.
     */
    return -1;
}


bool Dungeon_GetPlayerSpawn(
    const Dungeon *dungeon,
    float *x,
    float *y
)
{
    /*
     * Validate the output pointers.
     */
    if (dungeon == NULL ||
        x == NULL ||
        y == NULL)
    {
        return false;
    }

    /*
     * A dungeon without rooms has no valid
     * player spawn point.
     */
    if (dungeon->room_count <= 0)
    {
        return false;
    }

    /*
     * Use the center of the first generated
     * room as the starting position.
     */
    *x =
        dungeon->rooms[0].center_x *
            TILE_SIZE +
        TILE_SIZE * 0.5f;

    *y =
        dungeon->rooms[0].center_y *
            TILE_SIZE +
        TILE_SIZE * 0.5f;

    return true;
}


/*
 * Get the seed used to generate the dungeon.
 *
 * This is used by the save system so that the
 * dungeon can be deterministically regenerated
 * during loading.
 */
unsigned int Dungeon_GetSeed(
    const Dungeon *dungeon
)
{
    if (dungeon == NULL)
        return 0;

    return dungeon->seed;
}


void Dungeon_Init(
    Dungeon *dungeon,
    unsigned int seed
)
{
    if (dungeon == NULL)
        return;

    dungeon->room_count = 0;
    dungeon->connection_count = 0;
    dungeon->seed = seed;

    /*
     * Seed the pseudo-random number generator.
     *
     * The same seed produces the same dungeon.
     */
    srand(seed);

    /*
     * Reset every room slot.
     */
    for (int i = 0;
         i < MAX_DUNGEON_ROOMS;
         i++)
    {
        dungeon->rooms[i].x = 0;
        dungeon->rooms[i].y = 0;

        dungeon->rooms[i].width =
            MIN_ROOM_WIDTH;

        dungeon->rooms[i].height =
            MIN_ROOM_HEIGHT;

        dungeon->rooms[i].center_x = 0;
        dungeon->rooms[i].center_y = 0;

        dungeon->rooms[i].connected = false;
    }

    /*
     * Reset every connection slot.
     */
    for (int i = 0;
         i < MAX_DUNGEON_CONNECTIONS;
         i++)
    {
        dungeon->connections[i].room_a = -1;
        dungeon->connections[i].room_b = -1;
    }

    /*
     * Start with a completely solid map.
     */
    TileMap_Init();

    /*
     * Generate and carve rooms.
     */
    Dungeon_GenerateRooms(
        dungeon
    );

    /*
     * Connect the generated rooms using
     * a spatially-aware spanning tree.
     *
     * Each corridor is also recorded as an
     * edge in the dungeon graph.
     */
    Dungeon_ConnectRooms(
        dungeon
    );
}