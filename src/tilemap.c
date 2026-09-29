#include "tilemap.h"
#include "raylib.h"

static TileType map[MAP_HEIGHT][MAP_WIDTH];

void TileMap_Init(void)
{
    /*
     * Start with the entire map as walls.
     *
     * The procedural dungeon generator will carve
     * rooms and corridors into this map.
     */
    for (int y = 0;
         y < MAP_HEIGHT;
         y++)
    {
        for (int x = 0;
             x < MAP_WIDTH;
             x++)
        {
            map[y][x] = TILE_WALL;
        }
    }
}

void TileMap_Render(void)
{
    for (int y = 0;
         y < MAP_HEIGHT;
         y++)
    {
        for (int x = 0;
             x < MAP_WIDTH;
             x++)
        {
            int screen_x =
                x * TILE_SIZE;

            int screen_y =
                y * TILE_SIZE;

            if (map[y][x] == TILE_WALL)
            {
                DrawRectangle(
                    screen_x,
                    screen_y,
                    TILE_SIZE,
                    TILE_SIZE,
                    DARKGRAY
                );
            }
            else
            {
                DrawRectangle(
                    screen_x,
                    screen_y,
                    TILE_SIZE,
                    TILE_SIZE,
                    LIGHTGRAY
                );
            }
        }
    }
}

TileType TileMap_GetTile(
    int x,
    int y
)
{
    /*
     * Treat positions outside the map as walls.
     *
     * This prevents systems such as collision and
     * pathfinding from accidentally accessing memory
     * outside the tile map.
     */
    if (x < 0 ||
        x >= MAP_WIDTH ||
        y < 0 ||
        y >= MAP_HEIGHT)
    {
        return TILE_WALL;
    }

    return map[y][x];
}

bool TileMap_IsWalkable(
    int x,
    int y
)
{
    return TileMap_GetTile(
               x,
               y
           ) == TILE_FLOOR;
}

void TileMap_SetTile(
    int x,
    int y,
    TileType type
)
{
    /*
     * Ignore coordinates outside the map.
     */
    if (x < 0 ||
        x >= MAP_WIDTH ||
        y < 0 ||
        y >= MAP_HEIGHT)
    {
        return;
    }

    map[y][x] = type;
}