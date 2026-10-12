#include "minimap.h"

#include "raylib.h"
#include "tilemap.h"
#include "collision.h"
#include "gamecamera.h"
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
#define MINIMAP_SCALE 3

#define MINIMAP_MAP_WIDTH (MAP_WIDTH * MINIMAP_SCALE)
#define MINIMAP_MAP_HEIGHT (MAP_HEIGHT * MINIMAP_SCALE)

#define MINIMAP_PANEL_WIDTH (MINIMAP_MAP_WIDTH + 16)
#define MINIMAP_PANEL_HEIGHT (MINIMAP_MAP_HEIGHT + 48)

/* Opacity while something is under the minimap. */
#define MINIMAP_FADED_ALPHA 0.28f

/* How close (pixels) something must be to count as "under". */
#define MINIMAP_FADE_MARGIN 24

static bool explored[MAP_HEIGHT][MAP_WIDTH];

static bool minimap_visible = true;

/* Top-right corner of the screen. Set every frame. */
static int panel_x = 0;
static int panel_y = 8;

/* Current opacity, eased toward its target. */
static float panel_alpha = 1.0f;

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

static Color Minimap_Tint(Color color, float alpha)
{
    color.a = (unsigned char)((float)color.a * alpha);

    return color;
}

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

/*
 * Is a WORLD position currently drawn inside a screen
 * rectangle? The camera scrolls, so world positions must be
 * converted to screen positions first.
 */
static bool Minimap_WorldPointUnder(
    float world_x,
    float world_y,
    int margin)
{
    Vector2 screen =
        GameCamera_WorldToScreen(world_x, world_y);

    return screen.x >= (float)(panel_x - margin) &&
           screen.x <= (float)(panel_x + MINIMAP_PANEL_WIDTH + margin) &&
           screen.y >= (float)(panel_y - margin) &&
           screen.y <= (float)(panel_y + MINIMAP_PANEL_HEIGHT + margin);
}

/*
 * Is the player, a living enemy or an item drop under
 * the minimap panel?
 */
static bool Minimap_SomethingUnder(int margin)
{
    Player *player =
        Player_Get();

    if (player != NULL &&
        Minimap_WorldPointUnder(player->x, player->y, margin))
    {
        return true;
    }

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

        if (Minimap_WorldPointUnder(enemy->x, enemy->y, margin))
            return true;
    }

    for (int i = 0; i < ITEM_DROP_MAX; i++)
    {
        const ItemDrop *drop =
            ItemDrop_Get(i);

        if (drop != NULL &&
            Minimap_WorldPointUnder(drop->x, drop->y, margin))
        {
            return true;
        }
    }

    return false;
}

void Minimap_Reset(void)
{
    for (int y = 0; y < MAP_HEIGHT; y++)
    {
        for (int x = 0; x < MAP_WIDTH; x++)
            explored[y][x] = false;
    }

    panel_alpha = 1.0f;

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

    /*
     * Fixed in the top-right corner, which is free of HUD.
     */
    panel_x =
        GetScreenWidth() - MINIMAP_PANEL_WIDTH - 10;

    panel_y = 8;

    /*
     * Fade out while anything is under the panel, so the
     * minimap never hides the action.
     */
    float target_alpha =
        Minimap_SomethingUnder(MINIMAP_FADE_MARGIN)
            ? MINIMAP_FADED_ALPHA
            : 1.0f;

    float blend =
        GetFrameTime() * 8.0f;

    if (blend > 1.0f)
        blend = 1.0f;

    panel_alpha +=
        (target_alpha - panel_alpha) * blend;

    float a = panel_alpha;

    const int map_x = panel_x + 8;
    const int map_y = panel_y + 22;

    Rectangle panel =
        {
            (float)panel_x,
            (float)panel_y,
            (float)MINIMAP_PANEL_WIDTH,
            (float)MINIMAP_PANEL_HEIGHT};

    Color gold = (Color){255, 210, 60, 255};
    Color dim = (Color){145, 153, 168, 255};

    DrawRectangleRounded(
        panel, 0.06f, 8,
        Minimap_Tint((Color){7, 9, 13, 225}, a));

    DrawRectangleRoundedLinesEx(
        panel, 0.06f, 8, 1.0f,
        Minimap_Tint(gold, a * 0.75f));

    UI_Text(
        "MAP",
        panel_x + 10,
        panel_y + 5,
        11,
        Minimap_Tint(gold, a));

    UI_Text(
        "TAB",
        panel_x + MINIMAP_PANEL_WIDTH - 34,
        panel_y + 6,
        9,
        Minimap_Tint(dim, a));

    DrawRectangle(
        map_x,
        map_y,
        MINIMAP_MAP_WIDTH,
        MINIMAP_MAP_HEIGHT,
        Minimap_Tint((Color){3, 4, 6, 255}, a));

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
                    Minimap_Tint(color, a));

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
                    Minimap_Tint((Color){34, 34, 42, 255}, a));
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
            Minimap_Tint((Color){255, 205, 80, 255}, a));
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
                3.5f, Minimap_Tint(color, a));

            DrawCircleLines(
                (int)dot_x, (int)dot_y,
                5.5f + sinf(time * 4.0f),
                Minimap_Tint(color, a * 0.9f));
        }
        else
        {
            DrawCircle(
                (int)dot_x, (int)dot_y,
                2.0f, Minimap_Tint(color, a));
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

        Color blue = (Color){90, 200, 255, 255};

        DrawCircleLines(
            (int)dot_x, (int)dot_y,
            3.5f + pulse * 2.0f,
            Minimap_Tint(blue, a * 0.6f));

        DrawCircle(
            (int)dot_x, (int)dot_y,
            2.5f,
            Minimap_Tint(blue, a));

        DrawCircle(
            (int)dot_x, (int)dot_y,
            1.0f,
            Minimap_Tint(WHITE, a));
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
        Minimap_Tint(dim, a));

    const char *left_text =
        TextFormat("%d", Enemy_GetCount());

    int left_width =
        UI_NumberWidth(left_text, 12);

    UI_Number(
        left_text,
        panel_x + MINIMAP_PANEL_WIDTH - 10 - left_width,
        footer_y,
        12,
        Minimap_Tint(WHITE, a));
}