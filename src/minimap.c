#include "minimap.h"

#include "raylib.h"
#include "tilemap.h"
#include "collision.h"
#include "player.h"
#include "enemy.h"
#include "item_drop.h"
#include "dungeon.h"
#include "game.h"
#include "ui.h"

#include <math.h>
#include <stddef.h>
#include <stdbool.h>

/* How far the player can see, in tiles. */
#define MINIMAP_REVEAL_RADIUS 8

/* Pixels per tile on the minimap. */
#define MINIMAP_SCALE 4

#define MINIMAP_MAP_WIDTH (MAP_WIDTH * MINIMAP_SCALE)
#define MINIMAP_MAP_HEIGHT (MAP_HEIGHT * MINIMAP_SCALE)

static bool explored[MAP_HEIGHT][MAP_WIDTH];

static bool minimap_visible = true;

/*
 * Same colors as the room lighting.
 */
static const Color room_tints[] =
    {
        {70, 110, 255, 255},
        {255, 160, 60, 255},
        {70, 220, 140, 255},
        {200, 90, 255, 255},
        {60, 210, 230, 255},
        {255, 90, 80, 255}};

static bool Minimap_IsExplored(int x, int y)
{
    if (x < 0 || x >= MAP_WIDTH ||
        y < 0 || y >= MAP_HEIGHT)
    {
        return false;
    }

    return explored[y][x];
}

static int Minimap_RoomAt(
    const Dungeon *dungeon,
    int x,
    int y)
{
    if (dungeon == NULL)
        return -1;

    for (int i = 0; i < dungeon->room_count; i++)
    {
        const DungeonRoom *room =
            &dungeon->rooms[i];

        if (x >= room->x &&
            x < room->x + room->width &&
            y >= room->y &&
            y < room->y + room->height)
        {
            return i;
        }
    }

    return -1;
}

void Minimap_Reset(void)
{
    for (int y = 0; y < MAP_HEIGHT; y++)
    {
        for (int x = 0; x < MAP_WIDTH; x++)
            explored[y][x] = false;
    }

    Minimap_Update();
}

void Minimap_Toggle(void)
{
    minimap_visible = !minimap_visible;
}

void Minimap_Update(void)
{
    Player *player =
        Player_Get();

    if (player == NULL)
        return;

    int player_tile_x =
        (int)floorf(player->x / TILE_SIZE);

    int player_tile_y =
        (int)floorf(player->y / TILE_SIZE);

    for (int dy = -MINIMAP_REVEAL_RADIUS;
         dy <= MINIMAP_REVEAL_RADIUS;
         dy++)
    {
        for (int dx = -MINIMAP_REVEAL_RADIUS;
             dx <= MINIMAP_REVEAL_RADIUS;
             dx++)
        {
            if (dx * dx + dy * dy >
                MINIMAP_REVEAL_RADIUS *
                    MINIMAP_REVEAL_RADIUS)
            {
                continue;
            }

            int x = player_tile_x + dx;
            int y = player_tile_y + dy;

            if (x < 0 || x >= MAP_WIDTH ||
                y < 0 || y >= MAP_HEIGHT)
            {
                continue;
            }

            if (explored[y][x])
                continue;

            if (TileMap_GetTile(x, y) != TILE_FLOOR)
                continue;

            float tile_center_x =
                (float)x * TILE_SIZE + TILE_SIZE * 0.5f;

            float tile_center_y =
                (float)y * TILE_SIZE + TILE_SIZE * 0.5f;

            if (Collision_HasLineOfSight(
                    player->x,
                    player->y,
                    tile_center_x,
                    tile_center_y))
            {
                explored[y][x] = true;
            }
        }
    }
}

static Color Minimap_EnemyColor(const Enemy *enemy)
{
    if (enemy->is_boss)
        return (Color){255, 215, 80, 255};

    switch (enemy->adaptive_type)
    {
    case ADAPTIVE_AI_HUNTER:
        return (Color){255, 85, 95, 255};

    case ADAPTIVE_AI_GUARDIAN:
        return (Color){255, 175, 65, 255};

    case ADAPTIVE_AI_ASSASSIN:
        return (Color){225, 90, 255, 255};

    case ADAPTIVE_AI_BOSS:
    default:
        return (Color){175, 95, 255, 255};
    }
}

void Minimap_Render(void)
{
    if (!minimap_visible)
        return;

    const Dungeon *dungeon =
        Game_GetDungeon();

    const int panel_width =
        MINIMAP_MAP_WIDTH + 16;

    const int panel_height =
        MINIMAP_MAP_HEIGHT + 48;

    /* Directly under the player HUD panel (which ends at y = 72). */
    const int panel_x = 4;
    const int panel_y = 80;

    const int map_x = panel_x + 8;
    const int map_y = panel_y + 22;

    Rectangle panel =
        {
            (float)panel_x,
            (float)panel_y,
            (float)panel_width,
            (float)panel_height};

    DrawRectangleRounded(
        panel, 0.06f, 8,
        (Color){7, 9, 13, 225});

    DrawRectangleRoundedLinesEx(
        panel, 0.06f, 8, 1.0f,
        Fade((Color){255, 210, 60, 255}, 0.75f));

    UI_Text(
        "MAP",
        panel_x + 10,
        panel_y + 5,
        11,
        (Color){255, 210, 60, 255});

    UI_Text(
        "TAB",
        panel_x + panel_width - 34,
        panel_y + 6,
        9,
        (Color){145, 153, 168, 255});

    DrawRectangle(
        map_x,
        map_y,
        MINIMAP_MAP_WIDTH,
        MINIMAP_MAP_HEIGHT,
        (Color){3, 4, 6, 255});

    /*
     * Terrain: explored floor, and walls next to it.
     */
    for (int y = 0; y < MAP_HEIGHT; y++)
    {
        for (int x = 0; x < MAP_WIDTH; x++)
        {
            int draw_x = map_x + x * MINIMAP_SCALE;
            int draw_y = map_y + y * MINIMAP_SCALE;

            if (TileMap_GetTile(x, y) == TILE_FLOOR)
            {
                if (!explored[y][x])
                    continue;

                Color color =
                    (Color){92, 92, 104, 255};

                int room =
                    Minimap_RoomAt(dungeon, x, y);

                if (room >= 0)
                {
                    Color tint = room_tints[room % 6];

                    color.r = (unsigned char)(color.r * 0.65f + tint.r * 0.35f);
                    color.g = (unsigned char)(color.g * 0.65f + tint.g * 0.35f);
                    color.b = (unsigned char)(color.b * 0.65f + tint.b * 0.35f);
                }

                DrawRectangle(
                    draw_x, draw_y,
                    MINIMAP_SCALE, MINIMAP_SCALE,
                    color);

                continue;
            }

            bool near_explored = false;

            for (int oy = -1; oy <= 1 && !near_explored; oy++)
            {
                for (int ox = -1; ox <= 1; ox++)
                {
                    if (Minimap_IsExplored(x + ox, y + oy))
                    {
                        near_explored = true;
                        break;
                    }
                }
            }

            if (near_explored)
            {
                DrawRectangle(
                    draw_x, draw_y,
                    MINIMAP_SCALE, MINIMAP_SCALE,
                    (Color){34, 34, 42, 255});
            }
        }
    }

    float time = (float)GetTime();

    /*
     * Dropped items.
     */
    for (int i = 0; i < ITEM_DROP_MAX; i++)
    {
        const ItemDrop *drop =
            ItemDrop_Get(i);

        if (drop == NULL)
            continue;

        int tile_x = (int)(drop->x / TILE_SIZE);
        int tile_y = (int)(drop->y / TILE_SIZE);

        if (!Minimap_IsExplored(tile_x, tile_y))
            continue;

        DrawCircle(
            map_x + (int)(drop->x / TILE_SIZE * MINIMAP_SCALE),
            map_y + (int)(drop->y / TILE_SIZE * MINIMAP_SCALE),
            2.0f,
            (Color){255, 205, 80, 255});
    }

    /*
     * Enemies in revealed areas.
     */
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        const Enemy *enemy =
            Enemy_Get(i);

        if (enemy == NULL ||
            !enemy->active ||
            enemy->state == ENEMY_STATE_DEAD)
        {
            continue;
        }

        int tile_x = (int)(enemy->x / TILE_SIZE);
        int tile_y = (int)(enemy->y / TILE_SIZE);

        if (!Minimap_IsExplored(tile_x, tile_y))
            continue;

        float dot_x =
            (float)map_x + enemy->x / TILE_SIZE * MINIMAP_SCALE;

        float dot_y =
            (float)map_y + enemy->y / TILE_SIZE * MINIMAP_SCALE;

        Color color =
            Minimap_EnemyColor(enemy);

        if (enemy->is_boss)
        {
            DrawCircle(
                (int)dot_x, (int)dot_y,
                4.0f, color);

            DrawCircleLines(
                (int)dot_x, (int)dot_y,
                6.0f + sinf(time * 4.0f),
                Fade(color, 0.9f));
        }
        else
        {
            DrawCircle(
                (int)dot_x, (int)dot_y,
                2.5f, color);
        }
    }

    /*
     * The player.
     */
    Player *player =
        Player_Get();

    if (player != NULL)
    {
        float dot_x =
            (float)map_x + player->x / TILE_SIZE * MINIMAP_SCALE;

        float dot_y =
            (float)map_y + player->y / TILE_SIZE * MINIMAP_SCALE;

        float pulse =
            (sinf(time * 6.0f) + 1.0f) * 0.5f;

        DrawCircleLines(
            (int)dot_x, (int)dot_y,
            4.0f + pulse * 2.0f,
            Fade((Color){90, 200, 255, 255}, 0.6f));

        DrawCircle(
            (int)dot_x, (int)dot_y,
            3.0f,
            (Color){90, 200, 255, 255});

        DrawCircle(
            (int)dot_x, (int)dot_y,
            1.2f,
            WHITE);
    }

    /*
     * Remaining enemies.
     */
    int footer_y =
        map_y + MINIMAP_MAP_HEIGHT + 6;

    UI_Text(
        "ENEMIES LEFT",
        map_x,
        footer_y,
        10,
        (Color){145, 153, 168, 255});

    const char *left_text =
        TextFormat("%d", Enemy_GetCount());

    int left_width =
        UI_NumberWidth(left_text, 12);

    UI_Number(
        left_text,
        panel_x + panel_width - 10 - left_width,
        footer_y,
        12,
        WHITE);
}