#include "game.h"
#include "input.h"
#include "player.h"
#include "tilemap.h"
#include "enemy.h"
#include "dungeon.h"
#include "pathfinding.h"
#include "item_drop.h"
#include "items.h"
#include "save.h"
#include "events.h"
#include "behavior.h"
#include "audio.h"
#include "raylib.h"

#include <stddef.h>

typedef enum
{
    GAME_STATE_PLAYING,
    GAME_STATE_DEAD,
    GAME_STATE_WON

} GameState;

static GameState game_state;

static Dungeon dungeon;

static bool dungeon_debug_enabled = false;
static bool pathfinding_debug_enabled = false;

/*
 * ============================================================
 * UI CONSTANTS
 * ============================================================
 */

#define UI_FOOTER_HEIGHT       34

#define UI_PANEL_ROUNDNESS     0.16f
#define UI_PANEL_SEGMENTS      8

#define UI_GOLD                (Color){255, 210, 60, 255}
#define UI_GOLD_SOFT           (Color){255, 210, 60, 155}

#define UI_PANEL_BG            (Color){7, 9, 13, 238}
#define UI_PANEL_BG_DARK       (Color){4, 6, 9, 248}

#define UI_TEXT                (Color){240, 242, 247, 255}
#define UI_TEXT_DIM            (Color){145, 153, 168, 255}

#define UI_HP_GREEN            (Color){45, 220, 90, 255}
#define UI_HP_ORANGE           (Color){255, 165, 55, 255}
#define UI_HP_RED              (Color){235, 55, 65, 255}

#define UI_BOSS_RED            (Color){205, 35, 55, 255}
#define UI_BOSS_ORANGE         (Color){245, 135, 35, 255}
#define UI_BOSS_PURPLE         (Color){165, 80, 245, 255}

#define UI_CYAN                (Color){80, 200, 255, 255}

#define UI_GREEN               (Color){55, 220, 105, 255}

#define UI_SHADOW              (Color){0, 0, 0, 150}

/*
 * ============================================================
 * GAME ACCESS
 * ============================================================
 */

Dungeon *Game_GetDungeon(void)
{
    return &dungeon;
}

/*
 * ============================================================
 * START NEW RUN
 * ============================================================
 */

static void Game_StartRun(void)
{
    Dungeon_Init(
        &dungeon,
        12345);

    Player_Init();

    ItemDrop_Init();

    Events_Clear();

    Behavior_Init();

    float spawn_x = 0.0f;
    float spawn_y = 0.0f;

    if (Dungeon_GetPlayerSpawn(
            &dungeon,
            &spawn_x,
            &spawn_y))
    {
        Player_SetPosition(
            spawn_x,
            spawn_y);
    }

    Enemy_Init();

    game_state =
        GAME_STATE_PLAYING;
}

void Game_Init(void)
{
    InitWindow(
        1280,
        720,
        "DungeonForge");

    SetTargetFPS(60);

    /*
     * Initialize game audio after the
     * raylib window/context has been created.
     *
     * If audio initialization fails, the game
     * can still continue without sound.
     */
    Audio_Init();

    Events_Init();

    Behavior_Init();

    Game_StartRun();
}

void Game_Update(void)
{
    Input_Update();

    /*
     * Toggle dungeon graph debug view.
     */
    if (IsKeyPressed(KEY_F4))
    {
        dungeon_debug_enabled =
            !dungeon_debug_enabled;
    }

    /*
     * Toggle A* pathfinding debug view.
     */
    if (IsKeyPressed(KEY_F5))
    {
        pathfinding_debug_enabled =
            !pathfinding_debug_enabled;
    }

    /*
     * Save current game.
     */
    if (IsKeyPressed(KEY_F6))
    {
        Save_Game();
    }

    /*
     * Load previous game.
     */
    if (IsKeyPressed(KEY_F7))
    {
        if (Load_Game())
        {
            Events_Clear();

            Behavior_Init();

            game_state =
                GAME_STATE_PLAYING;
        }
    }

    /*
     * Restart after death or victory.
     */
    if (game_state ==
            GAME_STATE_DEAD ||
        game_state ==
            GAME_STATE_WON)
    {
        if (IsKeyPressed(KEY_R))
        {
            Game_StartRun();
        }

        return;
    }

    /*
     * ========================================================
     * NORMAL GAMEPLAY
     * ========================================================
     */

    Player_Update();

    if (Player_IsDead())
    {
        game_state =
            GAME_STATE_DEAD;

        Behavior_Update();

        return;
    }

    Enemy_Update();

    if (Player_IsDead())
    {
        game_state =
            GAME_STATE_DEAD;

        Behavior_Update();

        return;
    }

    ItemDrop_Update();

    Behavior_Update();

    /*
     * Victory requires all enemies and item drops
     * to be cleared.
     */
    if (Enemy_GetCount() == 0 &&
        ItemDrop_GetCount() == 0)
    {
        game_state =
            GAME_STATE_WON;
    }
}

/*
 * ============================================================
 * UI HELPERS
 * ============================================================
 */

static int Game_GetScreenWidth(void)
{
    return GetScreenWidth();
}

static int Game_GetScreenHeight(void)
{
    return GetScreenHeight();
}

/*
 * ------------------------------------------------------------
 * GENERIC PANEL
 * ------------------------------------------------------------
 */

static void Game_DrawPanel(
    int x,
    int y,
    int width,
    int height,
    Color border_color)
{
    Rectangle shadow =
    {
        (float)x + 3.0f,
        (float)y + 4.0f,
        (float)width,
        (float)height
    };

    Rectangle panel =
    {
        (float)x,
        (float)y,
        (float)width,
        (float)height
    };

    /*
     * Shadow.
     */
    DrawRectangleRounded(
        shadow,
        UI_PANEL_ROUNDNESS,
        UI_PANEL_SEGMENTS,
        Fade(BLACK, 0.55f));

    /*
     * Main body.
     */
    DrawRectangleRounded(
        panel,
        UI_PANEL_ROUNDNESS,
        UI_PANEL_SEGMENTS,
        UI_PANEL_BG);

    /*
     * Outer border.
     */
    DrawRectangleRoundedLinesEx(
        panel,
        UI_PANEL_ROUNDNESS,
        UI_PANEL_SEGMENTS,
        1.0f,
        Fade(border_color, 0.75f));

    /*
     * Top accent line.
     */
    if (width > 32)
    {
        DrawRectangleRounded(
            (Rectangle)
            {
                (float)x + 12.0f,
                (float)y + 2.0f,
                (float)width - 24.0f,
                2.0f
            },
            0.5f,
            4,
            Fade(border_color, 0.60f));
    }
}

/*
 * ------------------------------------------------------------
 * SMALL LABEL
 * ------------------------------------------------------------
 */

static void Game_DrawLabel(
    const char *text,
    int x,
    int y,
    int font_size,
    Color color)
{
    DrawText(
        text,
        x,
        y,
        font_size,
        color);
}

/*
 * ------------------------------------------------------------
 * SMALL BADGE
 * ------------------------------------------------------------
 */

static void Game_DrawBadge(
    const char *text,
    int x,
    int y,
    int width,
    int height,
    Color accent)
{
    DrawRectangleRounded(
        (Rectangle)
        {
            (float)x,
            (float)y,
            (float)width,
            (float)height
        },
        0.35f,
        6,
        Fade(accent, 0.12f));

    DrawRectangleRoundedLinesEx(
        (Rectangle)
        {
            (float)x,
            (float)y,
            (float)width,
            (float)height
        },
        0.35f,
        6,
        1.0f,
        Fade(accent, 0.40f));

    int text_width =
        MeasureText(
            text,
            9);

    DrawText(
        text,
        x +
            (width -
             text_width) / 2,
        y + 4,
        9,
        accent);
}

/*
 * ============================================================
 * PLAYER HUD
 * ============================================================
 *
 * Compact top-left player status.
 *
 * It is intentionally kept above the actual dungeon play
 * space so that enemies are not hidden by the HUD.
 */

static void Game_RenderPlayerHUD(void)
{
    Player *player =
        Player_Get();

    if (player == NULL)
        return;

    const int panel_x = 4;
    const int panel_y = 4;

    const int panel_width = 258;
    const int panel_height = 68;

    Game_DrawPanel(
        panel_x,
        panel_y,
        panel_width,
        panel_height,
        UI_GOLD);

    /*
     * --------------------------------------------------------
     * HEADER
     * --------------------------------------------------------
     */

    DrawCircle(
        panel_x + 16,
        panel_y + 16,
        4.0f,
        UI_GREEN);

    Game_DrawLabel(
        "PLAYER",
        panel_x + 27,
        panel_y + 9,
        12,
        UI_GOLD);

    Game_DrawLabel(
        "SURVIVOR",
        panel_x + 27,
        panel_y + 24,
        8,
        UI_TEXT_DIM);

    /*
     * Potion badge.
     */
    Game_DrawBadge(
        "H  POTION",
        panel_x + panel_width - 72,
        panel_y + 9,
        58,
        21,
        UI_GOLD);

    /*
     * --------------------------------------------------------
     * HP
     * --------------------------------------------------------
     */

    Game_DrawLabel(
        "HP",
        panel_x + 13,
        panel_y + 42,
        10,
        UI_TEXT);

    const int bar_x =
        panel_x + 39;

    const int bar_y =
        panel_y + 41;

    const int bar_width = 154;
    const int bar_height = 14;

    float health_ratio =
        (float)player->health / 100.0f;

    if (health_ratio < 0.0f)
        health_ratio = 0.0f;

    if (health_ratio > 1.0f)
        health_ratio = 1.0f;

    /*
     * Background.
     */
    DrawRectangleRounded(
        (Rectangle)
        {
            (float)bar_x,
            (float)bar_y,
            (float)bar_width,
            (float)bar_height
        },
        0.35f,
        6,
        Fade(DARKGRAY, 0.85f));

    /*
     * HP color.
     */
    Color health_color =
        UI_HP_GREEN;

    if (health_ratio <= 0.50f)
        health_color = UI_HP_ORANGE;

    if (health_ratio <= 0.25f)
        health_color = UI_HP_RED;

    int filled_width =
        (int)(
            (float)bar_width *
            health_ratio);

    if (filled_width > 0)
    {
        DrawRectangleRounded(
            (Rectangle)
            {
                (float)bar_x,
                (float)bar_y,
                (float)filled_width,
                (float)bar_height
            },
            0.35f,
            6,
            health_color);
    }

    /*
     * Subtle shine.
     */
    if (filled_width > 4)
    {
        DrawRectangle(
            bar_x + 3,
            bar_y + 2,
            filled_width - 6,
            2,
            Fade(WHITE, 0.18f));
    }

    /*
     * HP text.
     */
    const char *health_text =
        TextFormat(
            "%d / %d",
            player->health,
            100);

    int health_text_width =
        MeasureText(
            health_text,
            9);

    DrawText(
        health_text,
        bar_x +
            (bar_width -
             health_text_width) / 2,
        bar_y + 2,
        9,
        WHITE);

    /*
     * Potion label.
     */
    Game_DrawLabel(
        "HEALTH POTION",
        panel_x + 201,
        panel_y + 43,
        8,
        UI_TEXT_DIM);
}

/*
 * ============================================================
 * INVENTORY HUD
 * ============================================================
 *
 * The inventory is a dedicated bottom-right card.
 *
 * This is deliberately separated from the debug footer.
 */

static void Game_RenderInventoryHUD(void)
{
    Player *player =
        Player_Get();

    if (player == NULL)
        return;

    const int screen_width =
        Game_GetScreenWidth();

    const int screen_height =
        Game_GetScreenHeight();

    const int panel_width = 246;
    const int panel_height = 76;

    const int panel_x =
        screen_width -
        panel_width -
        8;

    const int panel_y =
        screen_height -
        UI_FOOTER_HEIGHT -
        panel_height -
        8;

    Game_DrawPanel(
        panel_x,
        panel_y,
        panel_width,
        panel_height,
        UI_GOLD);

    /*
     * --------------------------------------------------------
     * HEADER
     * --------------------------------------------------------
     */

    Game_DrawLabel(
        "INVENTORY",
        panel_x + 12,
        panel_y + 9,
        12,
        UI_GOLD);

    const char *capacity_text =
        TextFormat(
            "%d / %d",
            player->inventory.item_count,
            INVENTORY_MAX_ITEMS);

    int capacity_width =
        MeasureText(
            capacity_text,
            9);

    DrawText(
        capacity_text,
        panel_x +
            panel_width -
            capacity_width -
            12,
        panel_y + 11,
        9,
        UI_TEXT_DIM);

    /*
     * Divider.
     */
    DrawLine(
        panel_x + 12,
        panel_y + 27,
        panel_x + panel_width - 12,
        panel_y + 27,
        Fade(WHITE, 0.12f));

    /*
     * --------------------------------------------------------
     * HEALTH POTION ITEM
     * --------------------------------------------------------
     */

    int potion_quantity =
        Inventory_GetQuantity(
            &player->inventory,
            ITEM_ID_HEALTH_POTION);

    /*
     * Item slot.
     */
    DrawRectangleRounded(
        (Rectangle)
        {
            (float)panel_x + 12,
            (float)panel_y + 35,
            32.0f,
            28.0f
        },
        0.20f,
        6,
        Fade(UI_GOLD, 0.10f));

    DrawRectangleRoundedLinesEx(
        (Rectangle)
        {
            (float)panel_x + 12,
            (float)panel_y + 35,
            32.0f,
            28.0f
        },
        0.20f,
        6,
        1.0f,
        Fade(UI_GOLD, 0.35f));

    /*
     * Potion symbol.
     */
    DrawCircle(
        panel_x + 28,
        panel_y + 49,
        7.0f,
        UI_HP_RED);

    DrawRectangle(
        panel_x + 25,
        panel_y + 39,
        6,
        5,
        UI_TEXT);

    /*
     * Item text.
     */
    Game_DrawLabel(
        "HEALTH POTION",
        panel_x + 55,
        panel_y + 37,
        10,
        UI_TEXT);

    Game_DrawLabel(
        "Restore health",
        panel_x + 55,
        panel_y + 51,
        8,
        UI_TEXT_DIM);

    /*
     * Quantity badge.
     */
    Game_DrawBadge(
        TextFormat(
            "x%d",
            potion_quantity),
        panel_x + panel_width - 45,
        panel_y + 42,
        31,
        20,
        UI_GOLD);
}

/*
 * ============================================================
 * BOSS HUD
 * ============================================================
 */

static void Game_RenderBossHUD(void)
{
    Enemy *boss = NULL;

    /*
     * Find active boss.
     */
    for (int i = 0;
         i < MAX_ENEMIES;
         i++)
    {
        Enemy *enemy =
            Enemy_Get(i);

        if (enemy == NULL)
            continue;

        if (!enemy->active)
            continue;

        if (!enemy->is_boss)
            continue;

        if (enemy->state ==
            ENEMY_STATE_DEAD)
        {
            continue;
        }

        boss = enemy;
        break;
    }

    if (boss == NULL)
        return;

    const int screen_width =
        Game_GetScreenWidth();

    const int screen_height =
        Game_GetScreenHeight();

    const int panel_width = 520;
    const int panel_height = 78;

    const int panel_x =
        (screen_width -
         panel_width) /
        2;

    const int panel_y =
        screen_height -
        UI_FOOTER_HEIGHT -
        panel_height -
        8;

    Color boss_accent =
        UI_BOSS_RED;

    if (boss->boss_phase ==
        BOSS_PHASE_TWO)
    {
        boss_accent =
            UI_BOSS_ORANGE;
    }
    else if (boss->boss_phase ==
             BOSS_PHASE_THREE)
    {
        boss_accent =
            UI_BOSS_PURPLE;
    }

    Game_DrawPanel(
        panel_x,
        panel_y,
        panel_width,
        panel_height,
        boss_accent);

    /*
     * --------------------------------------------------------
     * HEADER
     * --------------------------------------------------------
     */

    Game_DrawLabel(
        "DUNGEON BOSS",
        panel_x + 16,
        panel_y + 8,
        13,
        UI_GOLD);

    /*
     * Phase badge.
     */

    const char *phase_text =
        "PHASE I";

    if (boss->boss_phase ==
        BOSS_PHASE_TWO)
    {
        phase_text =
            "PHASE II";
    }
    else if (boss->boss_phase ==
             BOSS_PHASE_THREE)
    {
        phase_text =
            "PHASE III";
    }

    Game_DrawBadge(
        phase_text,
        panel_x + panel_width - 78,
        panel_y + 6,
        62,
        20,
        boss_accent);

    /*
     * --------------------------------------------------------
     * HEALTH
     * --------------------------------------------------------
     */

    float health_ratio = 0.0f;

    if (boss->max_health > 0)
    {
        health_ratio =
            (float)boss->health /
            (float)boss->max_health;
    }

    if (health_ratio < 0.0f)
        health_ratio = 0.0f;

    if (health_ratio > 1.0f)
        health_ratio = 1.0f;

    const int bar_x =
        panel_x + 16;

    const int bar_y =
        panel_y + 35;

    const int bar_width =
        panel_width - 32;

    const int bar_height = 18;

    /*
     * Background.
     */
    DrawRectangleRounded(
        (Rectangle)
        {
            (float)bar_x,
            (float)bar_y,
            (float)bar_width,
            (float)bar_height
        },
        0.28f,
        8,
        Fade(DARKGRAY, 0.90f));

    /*
     * Fill.
     */
    int filled_width =
        (int)(
            (float)bar_width *
            health_ratio);

    if (filled_width > 0)
    {
        DrawRectangleRounded(
            (Rectangle)
            {
                (float)bar_x,
                (float)bar_y,
                (float)filled_width,
                (float)bar_height
            },
            0.28f,
            8,
            boss_accent);
    }

    /*
     * Phase markers.
     */
    int marker_one =
        bar_x +
        (int)(
            (float)bar_width *
            0.33f);

    int marker_two =
        bar_x +
        (int)(
            (float)bar_width *
            0.66f);

    DrawLine(
        marker_one,
        bar_y + 2,
        marker_one,
        bar_y + bar_height - 2,
        Fade(WHITE, 0.40f));

    DrawLine(
        marker_two,
        bar_y + 2,
        marker_two,
        bar_y + bar_height - 2,
        Fade(WHITE, 0.40f));

    /*
     * Border.
     */
    DrawRectangleRoundedLinesEx(
        (Rectangle)
        {
            (float)bar_x,
            (float)bar_y,
            (float)bar_width,
            (float)bar_height
        },
        0.28f,
        8,
        1.0f,
        Fade(boss_accent, 0.80f));

    /*
     * HP text.
     */

    const char *health_text =
        TextFormat(
            "%d / %d",
            boss->health,
            boss->max_health);

    int health_text_width =
        MeasureText(
            health_text,
            10);

    DrawText(
        health_text,
        bar_x +
            (bar_width -
             health_text_width) /
                2,
        bar_y + 4,
        10,
        WHITE);

    /*
     * Small status line.
     */
    Game_DrawLabel(
        "ADAPTIVE BOSS",
        panel_x + 16,
        panel_y + 59,
        8,
        UI_TEXT_DIM);

    Game_DrawLabel(
        "ENCOUNTER",
        panel_x + panel_width - 71,
        panel_y + 59,
        8,
        UI_TEXT_DIM);
}

/*
 * ============================================================
 * END GAME SCREEN
 * ============================================================
 */

static void Game_RenderEndScreen(void)
{
    bool victory =
        (game_state == GAME_STATE_WON);

    const int screen_width =
        Game_GetScreenWidth();

    const int screen_height =
        Game_GetScreenHeight();

    /*
     * Dark cinematic overlay.
     */
    DrawRectangle(
        0,
        0,
        screen_width,
        screen_height,
        Fade(BLACK, 0.82f));

    /*
     * Main card.
     */
    const int panel_width = 560;
    const int panel_height = 250;

    int panel_x =
        (screen_width -
         panel_width) /
        2;

    int panel_y =
        (screen_height -
         panel_height) /
        2;

    Color accent_color =
        victory
            ? UI_GREEN
            : UI_HP_RED;

    Rectangle panel =
    {
        (float)panel_x,
        (float)panel_y,
        (float)panel_width,
        (float)panel_height
    };

    /*
     * Shadow.
     */
    DrawRectangleRounded(
        (Rectangle)
        {
            (float)panel_x + 5.0f,
            (float)panel_y + 7.0f,
            (float)panel_width,
            (float)panel_height
        },
        0.08f,
        10,
        Fade(BLACK, 0.60f));

    /*
     * Body.
     */
    DrawRectangleRounded(
        panel,
        0.08f,
        10,
        Fade(BLACK, 0.96f));

    /*
     * Border.
     */
    DrawRectangleRoundedLinesEx(
        panel,
        0.08f,
        10,
        2.0f,
        Fade(accent_color, 0.85f));

    /*
     * Title.
     */
    const char *title =
        victory
            ? "DUNGEON CLEARED"
            : "YOU DIED";

    int title_size =
        victory
            ? 34
            : 40;

    int title_width =
        MeasureText(
            title,
            title_size);

    DrawText(
        title,
        panel_x +
            (panel_width -
             title_width) / 2,
        panel_y + 36,
        title_size,
        accent_color);

    /*
     * Subtitle.
     */
    const char *subtitle =
        victory
            ? "All enemies and world drops have been cleared."
            : "Your run has come to an end.";

    int subtitle_width =
        MeasureText(
            subtitle,
            16);

    DrawText(
        subtitle,
        panel_x +
            (panel_width -
             subtitle_width) / 2,
        panel_y + 98,
        16,
        UI_TEXT_DIM);

    /*
     * Separator.
     */
    DrawLine(
        panel_x + 55,
        panel_y + 136,
        panel_x + panel_width - 55,
        panel_y + 136,
        Fade(WHITE, 0.16f));

    /*
     * Restart button.
     */
    const int button_width = 270;
    const int button_height = 42;

    const int button_x =
        panel_x +
        (panel_width -
         button_width) / 2;

    const int button_y =
        panel_y + 156;

    DrawRectangleRounded(
        (Rectangle)
        {
            (float)button_x,
            (float)button_y,
            (float)button_width,
            (float)button_height
        },
        0.22f,
        8,
        Fade(accent_color, 0.12f));

    DrawRectangleRoundedLinesEx(
        (Rectangle)
        {
            (float)button_x,
            (float)button_y,
            (float)button_width,
            (float)button_height
        },
        0.22f,
        8,
        1.0f,
        Fade(accent_color, 0.65f));

    const char *restart_text =
        "PRESS R  •  NEW RUN";

    int restart_width =
        MeasureText(
            restart_text,
            14);

    DrawText(
        restart_text,
        button_x +
            (button_width -
             restart_width) / 2,
        button_y + 13,
        14,
        WHITE);

    /*
     * Branding.
     */
    const char *footer =
        "DUNGEONFORGE";

    int footer_width =
        MeasureText(
            footer,
            11);

    DrawText(
        footer,
        panel_x +
            (panel_width -
             footer_width) / 2,
        panel_y + 216,
        11,
        Fade(UI_GOLD, 0.65f));
}

/*
 * ============================================================
 * DUNGEON GRAPH DEBUG
 * ============================================================
 */

static void Game_RenderDungeonGraphDebug(void)
{
    if (!dungeon_debug_enabled)
        return;

    /*
     * Draw graph connections.
     */
    for (int i = 0;
         i < dungeon.connection_count;
         i++)
    {
        DungeonConnection *connection =
            &dungeon.connections[i];

        if (connection->room_a < 0 ||
            connection->room_a >=
                dungeon.room_count)
        {
            continue;
        }

        if (connection->room_b < 0 ||
            connection->room_b >=
                dungeon.room_count)
        {
            continue;
        }

        DungeonRoom *room_a =
            &dungeon.rooms[connection->room_a];

        DungeonRoom *room_b =
            &dungeon.rooms[connection->room_b];

        float x1 =
            room_a->center_x *
                TILE_SIZE +
            TILE_SIZE * 0.5f;

        float y1 =
            room_a->center_y *
                TILE_SIZE +
            TILE_SIZE * 0.5f;

        float x2 =
            room_b->center_x *
                TILE_SIZE +
            TILE_SIZE * 0.5f;

        float y2 =
            room_b->center_y *
                TILE_SIZE +
            TILE_SIZE * 0.5f;

        DrawLineEx(
            (Vector2){x1, y1},
            (Vector2){x2, y2},
            3.0f,
            YELLOW);
    }

    /*
     * Draw room centers.
     */
    for (int i = 0;
         i < dungeon.room_count;
         i++)
    {
        DungeonRoom *room =
            &dungeon.rooms[i];

        float center_x =
            room->center_x *
                TILE_SIZE +
            TILE_SIZE * 0.5f;

        float center_y =
            room->center_y *
                TILE_SIZE +
            TILE_SIZE * 0.5f;

        DrawCircle(
            (int)center_x,
            (int)center_y,
            7.0f,
            ORANGE);

        DrawText(
            TextFormat(
                "R%d",
                i),
            (int)center_x + 8,
            (int)center_y - 8,
            14,
            WHITE);
    }

    /*
     * Debug statistics panel.
     */
    Game_DrawPanel(
        8,
        88,
        250,
        58,
        UI_CYAN);

    DrawText(
        TextFormat(
            "ROOMS  %d",
            dungeon.room_count),
        18,
        98,
        14,
        WHITE);

    DrawText(
        TextFormat(
            "CONNECTIONS  %d",
            dungeon.connection_count),
        18,
        120,
        14,
        WHITE);
}

/*
 * ============================================================
 * A* PATHFINDING DEBUG
 * ============================================================
 */

static void Game_RenderPathfindingDebug(void)
{
    if (!pathfinding_debug_enabled)
        return;

    Player *player =
        Player_Get();

    if (player == NULL)
        return;

    /*
     * Debug panel.
     */
    Game_DrawPanel(
        8,
        154,
        300,
        58,
        UI_CYAN);

    DrawText(
        "A* PATHFINDING DEBUG",
        18,
        164,
        15,
        UI_CYAN);

    DrawText(
        "Next waypoint for each enemy",
        18,
        186,
        13,
        WHITE);

    /*
     * Test A* for every active enemy.
     */
    for (int i = 0;
         i < Enemy_GetCount();
         i++)
    {
        Enemy *enemy =
            Enemy_Get(i);

        if (enemy == NULL ||
            !enemy->active)
        {
            continue;
        }

        float next_x = 0.0f;
        float next_y = 0.0f;

        bool found =
            Pathfinding_FindNextStep(
                enemy->x,
                enemy->y,
                player->x,
                player->y,
                enemy->radius,
                &next_x,
                &next_y);

        if (!found)
        {
            DrawCircle(
                (int)enemy->x,
                (int)enemy->y,
                enemy->radius + 5.0f,
                MAROON);

            DrawText(
                TextFormat(
                    "NO PATH E%d",
                    i),
                (int)enemy->x + 20,
                (int)enemy->y - 25,
                12,
                RED);

            continue;
        }

        DrawLineEx(
            (Vector2)
            {
                enemy->x,
                enemy->y
            },
            (Vector2)
            {
                next_x,
                next_y
            },
            3.0f,
            SKYBLUE);

        DrawCircle(
            (int)next_x,
            (int)next_y,
            6.0f,
            SKYBLUE);

        DrawText(
            TextFormat(
                "E%d A*",
                i),
            (int)enemy->x + 20,
            (int)enemy->y + 10,
            12,
            SKYBLUE);

        DrawLineEx(
            (Vector2)
            {
                enemy->x,
                enemy->y
            },
            (Vector2)
            {
                player->x,
                player->y
            },
            1.0f,
            Fade(RED, 0.35f));
    }
}

/*
 * ============================================================
 * DEBUG FOOTER
 * ============================================================
 *
 * This footer contains developer controls only.
 *
 * Gameplay inventory is no longer mixed into this area.
 */

static void Game_RenderDebugFooter(void)
{
    const int screen_width =
        Game_GetScreenWidth();

    const int screen_height =
        Game_GetScreenHeight();

    const int footer_y =
        screen_height -
        UI_FOOTER_HEIGHT;

    /*
     * Footer background.
     */
    DrawRectangle(
        0,
        footer_y,
        screen_width,
        UI_FOOTER_HEIGHT,
        UI_PANEL_BG_DARK);

    /*
     * Upper edge.
     */
    DrawLine(
        0,
        footer_y,
        screen_width,
        footer_y,
        Fade(UI_GOLD, 0.38f));

    /*
     * Lower edge.
     */
    DrawLine(
        0,
        screen_height - 1,
        screen_width,
        screen_height - 1,
        Fade(WHITE, 0.06f));

    /*
     * Debug badge.
     */
    Game_DrawBadge(
        "DEBUG",
        8,
        footer_y + 7,
        48,
        20,
        UI_GOLD);

    /*
     * Controls.
     */
    DrawText(
        "F3 Enemy",
        66,
        footer_y + 11,
        9,
        UI_TEXT_DIM);

    DrawText(
        "F4 Graph",
        138,
        footer_y + 11,
        9,
        UI_TEXT_DIM);

    DrawText(
        "F5 A*",
        213,
        footer_y + 11,
        9,
        UI_TEXT_DIM);

    DrawText(
        "F6 Save",
        264,
        footer_y + 11,
        9,
        UI_TEXT_DIM);

    DrawText(
        "F7 Load",
        334,
        footer_y + 11,
        9,
        UI_TEXT_DIM);

    /*
     * Center identity.
     */
    const char *identity =
        "DUNGEONFORGE";

    int identity_width =
        MeasureText(
            identity,
            9);

    DrawText(
        identity,
        (screen_width -
         identity_width) / 2,
        footer_y + 11,
        9,
        Fade(UI_GOLD, 0.55f));
}

/*
 * ============================================================
 * GAME RENDER
 * ============================================================
 */

void Game_Render(void)
{
    BeginDrawing();

    ClearBackground(BLACK);

    /*
     * --------------------------------------------------------
     * WORLD
     * --------------------------------------------------------
     */

    TileMap_Render();

    /*
     * --------------------------------------------------------
     * DEVELOPER OVERLAYS
     * --------------------------------------------------------
     */

    Game_RenderDungeonGraphDebug();

    Game_RenderPathfindingDebug();

    /*
     * --------------------------------------------------------
     * WORLD ITEMS
     * --------------------------------------------------------
     */

    ItemDrop_Render();

    /*
     * --------------------------------------------------------
     * ENTITIES
     * --------------------------------------------------------
     */

    Enemy_Render();

    Player_Render();

    /*
     * --------------------------------------------------------
     * PLAYING STATE UI
     * --------------------------------------------------------
     */

    if (game_state ==
        GAME_STATE_PLAYING)
    {
        /*
         * Player status.
         */
        Game_RenderPlayerHUD();

        /*
         * Boss status.
         */
        Game_RenderBossHUD();

        /*
         * Inventory card.
         */
        Game_RenderInventoryHUD();

        /*
         * Developer footer.
         */
        Game_RenderDebugFooter();
    }

    /*
     * --------------------------------------------------------
     * END GAME
     * --------------------------------------------------------
     */

    if (game_state ==
            GAME_STATE_DEAD ||
        game_state ==
            GAME_STATE_WON)
    {
        Game_RenderEndScreen();
    }

    EndDrawing();
}

void Game_Shutdown(void)
{
    Audio_Shutdown();

    CloseWindow();
}