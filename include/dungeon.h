#ifndef DUNGEON_H
#define DUNGEON_H

#include <stdbool.h>

#define MAX_DUNGEON_ROOMS 32
#define MAX_DUNGEON_CONNECTIONS 64

typedef struct
{
    int x;
    int y;

    int width;
    int height;

    int center_x;
    int center_y;

    bool connected;

} DungeonRoom;

typedef struct
{
    int room_a;
    int room_b;

} DungeonConnection;

typedef struct
{
    DungeonRoom rooms[MAX_DUNGEON_ROOMS];

    int room_count;

    DungeonConnection connections[MAX_DUNGEON_CONNECTIONS];

    int connection_count;

    unsigned int seed;

} Dungeon;

/*
 * Initialize and procedurally generate
 * a dungeon using the supplied seed.
 */
void Dungeon_Init(
    Dungeon *dungeon,
    unsigned int seed);

/*
 * Get the seed used to generate the dungeon.
 *
 * The seed is saved instead of the complete
 * dungeon because the dungeon can be regenerated
 * deterministically from the same seed.
 */
unsigned int Dungeon_GetSeed(
    const Dungeon *dungeon);

/*
 * Get the world-space position where
 * the player should spawn.
 */
bool Dungeon_GetPlayerSpawn(
    const Dungeon *dungeon,
    float *x,
    float *y);

/*
 * Check whether two rooms are directly
 * connected by a dungeon graph edge.
 */
bool Dungeon_AreRoomsConnected(
    const Dungeon *dungeon,
    int room_a,
    int room_b);

/*
 * Get the number of rooms directly
 * connected to a given room.
 */
int Dungeon_GetRoomConnectionCount(
    const Dungeon *dungeon,
    int room_index);

/*
 * Get the index of the connected room
 * at a specific connection position.
 *
 * Example:
 *
 * room_index = 2
 * connection_index = 0
 *
 * returns the first room connected to Room 2.
 *
 * Returns -1 if the requested connection
 * does not exist.
 */
int Dungeon_GetConnectedRoom(
    const Dungeon *dungeon,
    int room_index,
    int connection_index);

/*
 * Number of corridor hops between two rooms.
 *
 * Returns -1 if either room is invalid or the rooms
 * are not connected.
 */
int Dungeon_GetRoomDistance(
    const Dungeon *dungeon,
    int from_room,
    int to_room);

/*
 * The room that is the most hops away from from_room.
 *
 * Ties are broken by straight-line distance, then by
 * the lower room index, so the result is deterministic.
 *
 * Returns -1 if from_room is invalid.
 */
int Dungeon_GetFarthestRoom(
    const Dungeon *dungeon,
    int from_room);

#endif