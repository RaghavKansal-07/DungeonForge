#ifndef TILEMAP_H
#define TILEMAP_H

#include <stdbool.h>

#define TILE_SIZE 32

#define MAP_WIDTH 80
#define MAP_HEIGHT 50

typedef enum
{
    TILE_FLOOR,
    TILE_WALL

} TileType;

void TileMap_Init(void);

void TileMap_Render(void);

TileType TileMap_GetTile(
    int x,
    int y);

bool TileMap_IsWalkable(
    int x,
    int y);

void TileMap_SetTile(
    int x,
    int y,
    TileType type);

/* Draw only the tiles in this range (inclusive). */
void TileMap_RenderRegion(
    int min_x,
    int min_y,
    int max_x,
    int max_y);

#endif