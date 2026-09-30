#include "tilemap.h"
#include "raylib.h"

static TileType map[MAP_HEIGHT][MAP_WIDTH];


/*
 * ---------------------------------------------------------
 * TILE VISUAL HELPERS
 * ---------------------------------------------------------
 */

/*
 * Return a subtle variation for floor tiles.
 *
 * The variation is deterministic and depends only
 * on the tile coordinates, so the dungeon appearance
 * remains stable during the run.
 */
static Color TileMap_GetFloorColor(
    int x,
    int y
)
{
    /*
     * Small deterministic pattern.
     *
     * This is intentionally subtle so that it
     * does not distract from gameplay.
     */
    int pattern =
        (x * 17 +
         y * 31 +
         x * y) % 5;

    switch (pattern)
    {
        case 0:
            return (Color){48, 48, 52, 255};

        case 1:
            return (Color){51, 51, 55, 255};

        case 2:
            return (Color){54, 54, 58, 255};

        case 3:
            return (Color){49, 49, 53, 255};

        default:
            return (Color){52, 52, 56, 255};
    }
}


/*
 * Return the base wall color.
 */
static Color TileMap_GetWallColor(
    int x,
    int y
)
{
    /*
     * Slight deterministic variation keeps large
     * wall regions from looking completely flat.
     */
    int pattern =
        (x * 13 +
         y * 7 +
         x * y) % 4;

    switch (pattern)
    {
        case 0:
            return (Color){68, 68, 72, 255};

        case 1:
            return (Color){72, 72, 76, 255};

        case 2:
            return (Color){65, 65, 69, 255};

        default:
            return (Color){70, 70, 74, 255};
    }
}


/*
 * ---------------------------------------------------------
 * TILE MAP INITIALIZATION
 * ---------------------------------------------------------
 */

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


/*
 * ---------------------------------------------------------
 * TILE MAP RENDERING
 * ---------------------------------------------------------
 */

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

            /*
             * -------------------------------------------------
             * WALL
             * -------------------------------------------------
             */
            if (map[y][x] == TILE_WALL)
            {
                Color wall_color =
                    TileMap_GetWallColor(
                        x,
                        y
                    );

                /*
                 * Main wall body.
                 */
                DrawRectangle(
                    screen_x,
                    screen_y,
                    TILE_SIZE,
                    TILE_SIZE,
                    wall_color
                );

                /*
                 * Subtle top highlight.
                 *
                 * This gives walls a little depth without
                 * introducing textures or external assets.
                 */
                DrawRectangle(
                    screen_x,
                    screen_y,
                    TILE_SIZE,
                    2,
                    Fade(
                        WHITE,
                        0.08f
                    )
                );

                /*
                 * Subtle left highlight.
                 */
                DrawRectangle(
                    screen_x,
                    screen_y,
                    2,
                    TILE_SIZE,
                    Fade(
                        WHITE,
                        0.05f
                    )
                );

                /*
                 * Dark bottom edge.
                 */
                DrawRectangle(
                    screen_x,
                    screen_y +
                        TILE_SIZE -
                        2,
                    TILE_SIZE,
                    2,
                    Fade(
                        BLACK,
                        0.20f
                    )
                );

                /*
                 * Dark right edge.
                 */
                DrawRectangle(
                    screen_x +
                        TILE_SIZE -
                        2,
                    screen_y,
                    2,
                    TILE_SIZE,
                    Fade(
                        BLACK,
                        0.16f
                    )
                );

                /*
                 * Very subtle tile boundary.
                 */
                DrawRectangleLines(
                    screen_x,
                    screen_y,
                    TILE_SIZE,
                    TILE_SIZE,
                    Fade(
                        BLACK,
                        0.12f
                    )
                );

                continue;
            }

            /*
             * -------------------------------------------------
             * FLOOR
             * -------------------------------------------------
             */

            Color floor_color =
                TileMap_GetFloorColor(
                    x,
                    y
                );

            /*
             * Main floor tile.
             */
            DrawRectangle(
                screen_x,
                screen_y,
                TILE_SIZE,
                TILE_SIZE,
                floor_color
            );

            /*
             * Very subtle floor border.
             *
             * This helps visually separate adjacent tiles
             * while remaining unobtrusive.
             */
            DrawRectangleLines(
                screen_x,
                screen_y,
                TILE_SIZE,
                TILE_SIZE,
                Fade(
                    BLACK,
                    0.10f
                )
            );

            /*
             * Small center detail.
             *
             * This is intentionally extremely subtle.
             * It gives large rooms some visual texture
             * without looking like a grid.
             */
            DrawRectangle(
                screen_x +
                    TILE_SIZE / 2 - 1,
                screen_y +
                    TILE_SIZE / 2 - 1,
                2,
                2,
                Fade(
                    WHITE,
                    0.035f
                )
            );
        }
    }
}


/*
 * ---------------------------------------------------------
 * TILE ACCESS
 * ---------------------------------------------------------
 */

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


/*
 * ---------------------------------------------------------
 * WALKABILITY
 * ---------------------------------------------------------
 */

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


/*
 * ---------------------------------------------------------
 * TILE MODIFICATION
 * ---------------------------------------------------------
 */

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