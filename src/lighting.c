#include "lighting.h"

#include "raylib.h"
#include "effects.h"
#include "player.h"
#include "enemy.h"
#include "item_drop.h"
#include "dungeon.h"
#include "game.h"
#include "tilemap.h"

#include <math.h>
#include <stddef.h>
#include <stdbool.h>

/*
 * Number of stacked circles per glow. More rings give a
 * smoother falloff.
 */
#define LIGHT_RINGS 24

/*
 * Light level where nothing shines. 255 would be full
 * brightness, lower is darker. Raise it if the game
 * feels too dark.
 */
#define LIGHT_AMBIENT ((Color){125, 125, 145, 255})

static RenderTexture2D light_mask;
static bool light_ready = false;
static bool light_enabled = true;


void Lighting_Init(void)
{
    light_mask =
        LoadRenderTexture(
            GetScreenWidth(),
            GetScreenHeight());

    light_ready =
        IsRenderTextureValid(light_mask);

    if (!light_ready)
    {
        TraceLog(
            LOG_WARNING,
            "Light map could not be created, lighting disabled");

        return;
    }

    SetTextureFilter(
        light_mask.texture,
        TEXTURE_FILTER_BILINEAR);
}


void Lighting_Shutdown(void)
{
    if (light_ready)
    {
        UnloadRenderTexture(light_mask);
        light_ready = false;
    }
}


void Lighting_Toggle(void)
{
    light_enabled = !light_enabled;
}


/*
 * Soft glow: stacked circles added together, so the light
 * is strongest in the middle and fades toward the edge.
 */
static void Lighting_Glow(
    float x,
    float y,
    float radius,
    Color color,
    float intensity)
{
    int alpha =
        (int)(255.0f * intensity /
              (float)LIGHT_RINGS);

    if (alpha < 1)
        alpha = 1;

    color.a = (unsigned char)alpha;

    for (int i = 0; i < LIGHT_RINGS; i++)
    {
        float r =
            radius *
            (float)(LIGHT_RINGS - i) /
            (float)LIGHT_RINGS;

        DrawCircle(
            (int)x,
            (int)y,
            r,
            color);
    }
}


static Color Lighting_EnemyColor(
    const Enemy *enemy)
{
    if (enemy->is_boss)
    {
        switch (enemy->boss_phase)
        {
        case BOSS_PHASE_TWO:
            return (Color){255, 150, 60, 255};

        case BOSS_PHASE_THREE:
            return (Color){255, 70, 80, 255};

        case BOSS_PHASE_ONE:
        default:
            return (Color){175, 95, 255, 255};
        }
    }

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


/*
 * Dark edges that draw the eye toward the center.
 */
static void Lighting_Vignette(void)
{
    int w = GetScreenWidth();
    int h = GetScreenHeight();

    int band = 150;

    Color edge  = (Color){0, 0, 0, 120};
    Color clear = (Color){0, 0, 0, 0};

    DrawRectangleGradientV(0, 0, w, band, edge, clear);
    DrawRectangleGradientV(0, h - band, w, band, clear, edge);
    DrawRectangleGradientH(0, 0, band, h, edge, clear);
    DrawRectangleGradientH(w - band, 0, band, h, clear, edge);
}


void Lighting_Render(void)
{
    if (!light_enabled ||
        !light_ready)
    {
        return;
    }

    Vector2 shake =
        Effects_GetShake();

    const Dungeon *dungeon =
        Game_GetDungeon();

    Player *player =
        Player_Get();

    float time =
        (float)GetTime();

    /*
     * ---------------------------------------------------
     * Build the light map
     * ---------------------------------------------------
     */
    BeginTextureMode(light_mask);

    ClearBackground(LIGHT_AMBIENT);

    BeginBlendMode(BLEND_ADDITIVE);

    /*
     * Faint color for each room.
     */
    static const Color room_tints[] =
    {
        {  70, 110, 255, 255 },
        { 255, 160,  60, 255 },
        {  70, 220, 140, 255 },
        { 200,  90, 255, 255 },
        {  60, 210, 230, 255 },
        { 255,  90,  80, 255 }
    };

    if (dungeon != NULL)
    {
        for (int i = 0;
             i < dungeon->room_count;
             i++)
        {
            const DungeonRoom *room =
                &dungeon->rooms[i];

            Color tint =
                room_tints[i % 6];

            tint.a = 22;

            DrawRectangle(
                (int)((float)(room->x * TILE_SIZE) + shake.x),
                (int)((float)(room->y * TILE_SIZE) + shake.y),
                room->width * TILE_SIZE,
                room->height * TILE_SIZE,
                tint);
        }
    }

    /*
     * Dropped items.
     */
    for (int i = 0;
         i < ITEM_DROP_MAX;
         i++)
    {
        const ItemDrop *drop =
            ItemDrop_Get(i);

        if (drop == NULL)
            continue;

        Lighting_Glow(
            drop->x + shake.x,
            drop->y + shake.y,
            64.0f,
            (Color){255, 205, 80, 255},
            0.55f);
    }

    /*
     * Enemies.
     */
    for (int i = 0;
         i < MAX_ENEMIES;
         i++)
    {
        const Enemy *enemy =
            Enemy_Get(i);

        if (enemy == NULL ||
            !enemy->active ||
            enemy->state == ENEMY_STATE_DEAD)
        {
            continue;
        }

        Lighting_Glow(
            enemy->x + shake.x,
            enemy->y + shake.y,
            enemy->is_boss ? 190.0f : 115.0f,
            Lighting_EnemyColor(enemy),
            enemy->is_boss ? 0.50f : 0.42f);
    }

    /*
     * The player's torch, with a slight flicker.
     */
    if (player != NULL)
    {
        float flicker =
            sinf(time * 9.0f) * 6.0f +
            sinf(time * 23.0f) * 4.0f;

        Lighting_Glow(
            player->x + shake.x,
            player->y + shake.y,
            270.0f + flicker,
            (Color){255, 225, 170, 255},
            0.62f);
    }

    EndBlendMode();

    EndTextureMode();

    /*
     * ---------------------------------------------------
     * Multiply the finished scene by the light map
     * ---------------------------------------------------
     */
    BeginBlendMode(BLEND_MULTIPLIED);

    DrawTextureRec(
        light_mask.texture,
        (Rectangle){
            0.0f,
            0.0f,
            (float)light_mask.texture.width,
            -(float)light_mask.texture.height},
        (Vector2){ 0.0f, 0.0f },
        WHITE);

    EndBlendMode();

    Lighting_Vignette();
}