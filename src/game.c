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
#include "ui.h"
#include "effects.h"
#include "lighting.h"
#include "minimap.h"
#include <time.h>
#include "gamecamera.h"
#include "balance.h"
#include "equipment.h"

#include <stddef.h>
#include <math.h>

typedef enum
{
    GAME_STATE_PLAYING,
    GAME_STATE_DEAD,
    GAME_STATE_WON,
    GAME_STATE_MENU,
    GAME_STATE_PAUSED

} GameState;

static GameState game_state;

/* Run statistics for the end screen. */
static float run_time = 0.0f;

/*
 * How much the player is "in a fight": 0 when calm, 1 in combat.
 * Drives the low-health warning.
 */
static float combat_intensity = 0.0f;

static int run_start_enemies = 0;

static bool quit_requested = false;

static Dungeon dungeon;

static bool dungeon_debug_enabled = false;
static bool pathfinding_debug_enabled = false;

static bool ui_debug_visible = false;

typedef enum
{
    SAVE_STATUS_NONE,
    SAVE_STATUS_SAVED,
    SAVE_STATUS_SAVE_FAILED,
    SAVE_STATUS_LOADED,
    SAVE_STATUS_LOAD_FAILED

} SaveStatus;

static SaveStatus save_status = SAVE_STATUS_NONE;
static float save_status_timer = 0.0f;

/*
 * ============================================================
 * UI CONSTANTS
 * ============================================================
 */

#define UI_FOOTER_HEIGHT 34

#define UI_PANEL_ROUNDNESS 0.16f
#define UI_PANEL_SEGMENTS 8

#define UI_GOLD (Color){255, 210, 60, 255}
#define UI_GOLD_SOFT (Color){255, 210, 60, 155}

#define UI_PANEL_BG (Color){7, 9, 13, 238}
#define UI_PANEL_BG_DARK (Color){4, 6, 9, 248}

#define UI_TEXT (Color){240, 242, 247, 255}
#define UI_TEXT_DIM (Color){145, 153, 168, 255}

#define UI_HP_GREEN (Color){45, 220, 90, 255}
#define UI_HP_ORANGE (Color){255, 165, 55, 255}
#define UI_HP_RED (Color){235, 55, 65, 255}

#define UI_BOSS_RED (Color){205, 35, 55, 255}
#define UI_BOSS_ORANGE (Color){245, 135, 35, 255}
#define UI_BOSS_PURPLE (Color){165, 80, 245, 255}

#define UI_CYAN (Color){80, 200, 255, 255}

#define UI_GREEN (Color){55, 220, 105, 255}

#define UI_SHADOW (Color){0, 0, 0, 150}

/*
 * The footer only takes space while debug UI is visible.
 */
static int Game_FooterHeight(void)
{
    return ui_debug_visible
               ? UI_FOOTER_HEIGHT
               : 0;
}

/*
 * ============================================================
 * GAME ACCESS
 * ============================================================
 */

Dungeon *Game_GetDungeon(void)
{
    return &dungeon;
}

bool Game_ShouldQuit(void)
{
    return quit_requested;
}

/*
 * ============================================================
 * DUNGEON GENERATION
 * ============================================================
 */

#define MIN_DUNGEON_ROOMS 12
#define MAX_SEED_ATTEMPTS 200

/*
 * A different seed for every run, even when two runs start
 * within the same second.
 */
static unsigned int Game_NewSeed(void)
{
    static unsigned int counter = 0;

    counter++;

    unsigned int seed =
        (unsigned int)time(NULL);

    return seed * 2654435761u +
           counter * 40503u +
           0x9E3779B9u;
}

/*
 * Generate a dungeon, trying following seeds until it has
 * enough rooms. The seed that was finally used is stored in
 * the dungeon, so a save can recreate the same map.
 */
static void Game_GenerateDungeon(unsigned int seed)
{
    for (int attempt = 0;
         attempt < MAX_SEED_ATTEMPTS;
         attempt++)
    {
        Dungeon_Init(
            &dungeon,
            seed + (unsigned int)attempt);

        if (dungeon.room_count >= MIN_DUNGEON_ROOMS)
            return;
    }
}

/*
 * ============================================================
 * START NEW RUN
 * ============================================================
 */

static void Game_StartRun(void)
{
    Game_GenerateDungeon(
        Game_NewSeed());

    Player_Init();

    ItemDrop_Init();

    Events_Clear();

    Effects_Clear();

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

    run_time = 0.0f;

    run_start_enemies =
        Enemy_GetCount();

    GameCamera_Reset();

    Minimap_Reset();

    combat_intensity = 0.0f;

    game_state =
        GAME_STATE_PLAYING;
}

void Game_Init(void)
{
    InitWindow(
        1280,
        720,
        "DungeonForge");

    SetExitKey(KEY_NULL);

    SetTargetFPS(60);
    UI_Init();
    Lighting_Init();

    /*
     * Initialize game audio after the
     * raylib window/context has been created.
     *
     * If audio initialization fails, the game
     * can still continue without sound.
     */
    if (!Audio_Init())
    {
        TraceLog(
            LOG_WARNING,
            "Some sounds failed to load. Run the game from the project root.");
    }

    Events_Init();

    Behavior_Init();

    Game_StartRun();

    game_state =
        GAME_STATE_MENU;
}

/*
 * ============================================================
 * COMBAT LEVEL
 * ============================================================
 *
 * Rises when the player was just hit or an awake enemy is near,
 * and fades slowly once the fight is over.
 */

#define COMBAT_ENGAGE_RANGE 420.0f

static void Game_UpdateCombatIntensity(float dt)
{
    Player *player =
        Player_Get();

    bool in_combat = false;

    if (player != NULL)
    {
        if (player->damage_timer > 0.0f)
            in_combat = true;

        for (int i = 0;
             i < MAX_ENEMIES && !in_combat;
             i++)
        {
            const Enemy *enemy =
                Enemy_Get(i);

            if (enemy == NULL ||
                !enemy->active ||
                enemy->state == ENEMY_STATE_DEAD ||
                enemy->state == ENEMY_STATE_IDLE)
            {
                continue;
            }

            float dx = enemy->x - player->x;
            float dy = enemy->y - player->y;

            if (dx * dx + dy * dy <=
                COMBAT_ENGAGE_RANGE * COMBAT_ENGAGE_RANGE)
            {
                in_combat = true;
            }
        }
    }

    float target =
        in_combat ? 1.0f : 0.0f;

    float rate =
        (in_combat ? 6.0f : 0.7f) * dt;

    if (rate > 1.0f)
        rate = 1.0f;

    combat_intensity +=
        (target - combat_intensity) * rate;
}

static void Game_AfterLoad(void)
{
    Events_Clear();

    Effects_Clear();

    GameCamera_Reset();

    Minimap_Reset();

    combat_intensity = 0.0f;

    run_time = 0.0f;

    run_start_enemies =
        Enemy_GetCount();

    game_state =
        GAME_STATE_PLAYING;
}

void Game_Update(void)
{
    Input_Update();

    float frame_time =
        GetFrameTime();

    Effects_Update(frame_time);

    if (save_status_timer > 0.0f)
    {
        save_status_timer -= frame_time;

        if (save_status_timer <= 0.0f)
        {
            save_status_timer = 0.0f;
            save_status = SAVE_STATUS_NONE;
        }
    }

    if (IsKeyPressed(KEY_F1))
    {
        ui_debug_visible =
            !ui_debug_visible;
    }

    if (IsKeyPressed(KEY_F2))
    {
        Lighting_Toggle();
    }

    if (IsKeyPressed(KEY_TAB))
    {
        Minimap_Toggle();
    }

    /*
     * ========================================================
     * MAIN MENU
     * ========================================================
     */
    if (game_state ==
        GAME_STATE_MENU)
    {
        if (IsKeyPressed(KEY_ENTER) ||
            IsKeyPressed(KEY_KP_ENTER))
        {
            Game_StartRun();
        }
        else if (IsKeyPressed(KEY_L))
        {
            if (Load_Game())
            {
                Game_AfterLoad();

                save_status =
                    SAVE_STATUS_LOADED;
            }
            else
            {
                save_status =
                    SAVE_STATUS_LOAD_FAILED;
            }

            save_status_timer = 2.0f;
        }
        else if (IsKeyPressed(KEY_ESCAPE) ||
                 IsKeyPressed(KEY_Q))
        {
            quit_requested = true;
        }

        return;
    }

    /*
     * Debug toggles.
     */
    if (IsKeyPressed(KEY_F4))
    {
        dungeon_debug_enabled =
            !dungeon_debug_enabled;
    }

    if (IsKeyPressed(KEY_F5))
    {
        pathfinding_debug_enabled =
            !pathfinding_debug_enabled;
    }

    /*
     * Save and load (also available while paused).
     */
    if (IsKeyPressed(KEY_F6))
    {
        if (Save_Game())
            save_status = SAVE_STATUS_SAVED;
        else
            save_status = SAVE_STATUS_SAVE_FAILED;

        save_status_timer = 2.0f;
    }

    if (IsKeyPressed(KEY_F7))
    {
        if (Load_Game())
        {
            Game_AfterLoad();

            save_status =
                SAVE_STATUS_LOADED;
        }
        else
        {
            save_status =
                SAVE_STATUS_LOAD_FAILED;
        }

        save_status_timer = 2.0f;
    }

    /*
     * ========================================================
     * PAUSED
     * ========================================================
     */
    if (game_state ==
        GAME_STATE_PAUSED)
    {
        if (IsKeyPressed(KEY_ESCAPE) ||
            IsKeyPressed(KEY_P))
        {
            game_state =
                GAME_STATE_PLAYING;
        }
        else if (IsKeyPressed(KEY_M))
        {
            game_state =
                GAME_STATE_MENU;
        }

        return;
    }

    /*
     * ========================================================
     * DEAD OR WON
     * ========================================================
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
        else if (IsKeyPressed(KEY_M))
        {
            game_state =
                GAME_STATE_MENU;
        }

        return;
    }

    /*
     * Pause.
     */
    if (IsKeyPressed(KEY_ESCAPE) ||
        IsKeyPressed(KEY_P))
    {
        game_state =
            GAME_STATE_PAUSED;

        return;
    }

    /*
     * ========================================================
     * NORMAL GAMEPLAY
     * ========================================================
     */

    if (frame_time > 0.1f)
        frame_time = 0.1f;

    run_time += frame_time;

    Behavior_Tick(
        GetFrameTime());

    Player_Update();

    GameCamera_Update(frame_time);

    if (Player_IsDead())
    {
        game_state =
            GAME_STATE_DEAD;

        Behavior_Update();

        return;
    }

    Enemy_Update();

    Game_UpdateCombatIntensity(frame_time);

    if (Player_IsDead())
    {
        game_state =
            GAME_STATE_DEAD;

        Behavior_Update();

        return;
    }

    ItemDrop_Update();

    Minimap_Update();

    Behavior_Update();

    /*
     * Victory requires all enemies and item drops
     * to be cleared.
     */
    if (Enemy_GetCount() == 0)
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
            (float)height};

    Rectangle panel =
        {
            (float)x,
            (float)y,
            (float)width,
            (float)height};

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
            (Rectangle){
                (float)x + 12.0f,
                (float)y + 2.0f,
                (float)width - 24.0f,
                2.0f},
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
    UI_Text(
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
        (Rectangle){
            (float)x,
            (float)y,
            (float)width,
            (float)height},
        0.35f,
        6,
        Fade(accent, 0.12f));

    DrawRectangleRoundedLinesEx(
        (Rectangle){
            (float)x,
            (float)y,
            (float)width,
            (float)height},
        0.35f,
        6,
        1.0f,
        Fade(accent, 0.40f));

    int text_width =
        UI_TextWidth(
            text,
            9);

    UI_Text(
        text,
        x +
            (width -
             text_width) /
                2,
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

    const int bar_width = 200;
    const int bar_height = 14;

    float health_ratio =
        (float)player->health / (float)PLAYER_MAX_HEALTH;

    if (health_ratio < 0.0f)
        health_ratio = 0.0f;

    if (health_ratio > 1.0f)
        health_ratio = 1.0f;

    /*
     * Background.
     */
    DrawRectangleRounded(
        (Rectangle){
            (float)bar_x,
            (float)bar_y,
            (float)bar_width,
            (float)bar_height},
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
        (int)((float)bar_width *
              health_ratio);

    if (filled_width > 0)
    {
        DrawRectangleRounded(
            (Rectangle){
                (float)bar_x,
                (float)bar_y,
                (float)filled_width,
                (float)bar_height},
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
            PLAYER_MAX_HEALTH);

    int health_text_width =
        UI_NumberWidth(
            health_text,
            10);

    UI_Number(
        health_text,
        bar_x +
            (bar_width -
             health_text_width) /
                2,
        bar_y + 2,
        10,
        WHITE);
}

/*
 * One equipment slot: bright when owned, dim when not.
 */
static void Game_DrawEquipSlot(
    const char *name,
    const char *effect,
    bool owned,
    int x,
    int y,
    int width,
    Color accent)
{
    Rectangle rect =
        {
            (float)x,
            (float)y,
            (float)width,
            28.0f};

    float strength =
        owned
            ? 1.0f
            : 0.35f;

    DrawRectangleRounded(
        rect,
        0.25f,
        6,
        Fade(accent, owned ? 0.16f : 0.05f));

    DrawRectangleRoundedLinesEx(
        rect,
        0.25f,
        6,
        1.0f,
        Fade(accent, 0.55f * strength));

    UI_Text(
        name,
        x + 8,
        y + 4,
        9,
        Fade(UI_TEXT, strength));

    UI_Number(
        owned
            ? effect
            : "---",
        x + 8,
        y + 15,
        9,
        Fade(accent, strength));
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
    const int panel_height = 112;

    const int panel_x =
        screen_width -
        panel_width -
        8;

    const int panel_y =
        screen_height -
        Game_FooterHeight() -
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
            "SLOTS %d / %d",
            player->inventory.item_count,
            INVENTORY_MAX_ITEMS);

    int capacity_width =
        UI_NumberWidth(
            capacity_text,
            10);

    UI_Number(
        capacity_text,
        panel_x +
            panel_width -
            capacity_width -
            12,
        panel_y + 11,
        10,
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
        (Rectangle){
            (float)panel_x + 12,
            (float)panel_y + 35,
            32.0f,
            28.0f},
        0.20f,
        6,
        Fade(UI_GOLD, 0.10f));

    DrawRectangleRoundedLinesEx(
        (Rectangle){
            (float)panel_x + 12,
            (float)panel_y + 35,
            32.0f,
            28.0f},
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

    /*
     * --------------------------------------------------------
     * EQUIPMENT
     * --------------------------------------------------------
     */
    Game_DrawEquipSlot(
        "IRON SWORD",
        TextFormat(
            "+%d DAMAGE",
            BAL_SWORD_DAMAGE_BONUS),
        Equipment_HasSword(&player->inventory),
        panel_x + 12,
        panel_y + 74,
        106,
        (Color){255, 150, 60, 255});

    Game_DrawEquipSlot(
        "IRON ARMOR",
        TextFormat(
            "-%d%% DAMAGE",
            BAL_ARMOR_REDUCTION_PERCENT),
        Equipment_HasArmor(&player->inventory),
        panel_x + 128,
        panel_y + 74,
        106,
        (Color){90, 200, 255, 255});
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
        Game_FooterHeight() -
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
        (Rectangle){
            (float)bar_x,
            (float)bar_y,
            (float)bar_width,
            (float)bar_height},
        0.28f,
        8,
        Fade(DARKGRAY, 0.90f));

    /*
     * Fill.
     */
    int filled_width =
        (int)((float)bar_width *
              health_ratio);

    if (filled_width > 0)
    {
        DrawRectangleRounded(
            (Rectangle){
                (float)bar_x,
                (float)bar_y,
                (float)filled_width,
                (float)bar_height},
            0.28f,
            8,
            boss_accent);
    }

    /*
     * Phase markers.
     */
    int marker_one =
        bar_x +
        (int)((float)bar_width *
              0.40f);

    int marker_two =
        bar_x +
        (int)((float)bar_width *
              0.70f);

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
        (Rectangle){
            (float)bar_x,
            (float)bar_y,
            (float)bar_width,
            (float)bar_height},
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
        UI_NumberWidth(
            health_text,
            10);

    UI_Number(
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

/*
 * ============================================================
 * SHARED SCREEN HELPERS
 * ============================================================
 */

static void Game_DrawCenteredText(
    const char *text,
    int center_x,
    int y,
    int size,
    Color color)
{
    int width =
        UI_TextWidth(
            text,
            size);

    UI_Text(
        text,
        center_x - width / 2,
        y,
        size,
        color);
}

static void Game_DrawMenuButton(
    const char *text,
    int x,
    int y,
    int width,
    int height,
    Color accent)
{
    Rectangle rect =
        {
            (float)x,
            (float)y,
            (float)width,
            (float)height};

    DrawRectangleRounded(
        rect,
        0.22f,
        8,
        Fade(accent, 0.12f));

    DrawRectangleRoundedLinesEx(
        rect,
        0.22f,
        8,
        1.0f,
        Fade(accent, 0.65f));

    Game_DrawCenteredText(
        text,
        x + width / 2,
        y + (height - 16) / 2,
        14,
        WHITE);
}

static void Game_DrawStatTile(
    const char *label,
    const char *value,
    int x,
    int y,
    int width,
    Color accent)
{
    Rectangle rect =
        {
            (float)x,
            (float)y,
            (float)width,
            64.0f};

    DrawRectangleRounded(
        rect,
        0.16f,
        6,
        Fade(WHITE, 0.05f));

    DrawRectangleRoundedLinesEx(
        rect,
        0.16f,
        6,
        1.0f,
        Fade(accent, 0.35f));

    Game_DrawCenteredText(
        label,
        x + width / 2,
        y + 8,
        9,
        UI_TEXT_DIM);

    int value_width =
        UI_NumberWidth(
            value,
            24);

    UI_Number(
        value,
        x + (width - value_width) / 2,
        y + 30,
        24,
        WHITE);
}

/*
 * Index of the largest value: 0 left, 1 right, 2 up, 3 down.
 */
static int Game_DominantDirection(
    float left,
    float right,
    float up,
    float down,
    float *share)
{
    int best = 0;
    float value = left;

    if (right > value)
    {
        value = right;
        best = 1;
    }

    if (up > value)
    {
        value = up;
        best = 2;
    }

    if (down > value)
    {
        value = down;
        best = 3;
    }

    *share = value;

    return best;
}

static const char *game_direction_names[] =
    {
        "left",
        "right",
        "up",
        "down"};

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

    DrawRectangle(
        0,
        0,
        screen_width,
        screen_height,
        Fade(BLACK, 0.82f));

    const int panel_width = 640;
    const int panel_height = 450;

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
            (float)panel_height};

    DrawRectangleRounded(
        (Rectangle){
            (float)panel_x + 5.0f,
            (float)panel_y + 7.0f,
            (float)panel_width,
            (float)panel_height},
        0.05f,
        10,
        Fade(BLACK, 0.60f));

    DrawRectangleRounded(
        panel,
        0.05f,
        10,
        Fade(BLACK, 0.96f));

    DrawRectangleRoundedLinesEx(
        panel,
        0.05f,
        10,
        2.0f,
        Fade(accent_color, 0.85f));

    const int center_x =
        screen_width / 2;

    /*
     * Title.
     */
    Game_DrawCenteredText(
        victory
            ? "DUNGEON CLEARED"
            : "YOU DIED",
        center_x,
        panel_y + 20,
        victory
            ? 34
            : 40,
        accent_color);

    Game_DrawCenteredText(
        victory
            ? "Every enemy in the dungeon has been defeated."
            : "Your run has come to an end.",
        center_x,
        panel_y + 76,
        14,
        UI_TEXT_DIM);

    DrawLine(
        panel_x + 40,
        panel_y + 108,
        panel_x + panel_width - 40,
        panel_y + 108,
        Fade(WHITE, 0.16f));

    /*
     * Run statistics.
     */
    const PlayerBehavior *profile =
        Behavior_GetProfile();

    int seconds =
        (int)run_time;

    int defeated =
        run_start_enemies -
        Enemy_GetCount();

    if (defeated < 0)
        defeated = 0;

    const int tile_width = 134;
    const int tile_gap = 12;

    int tile_x =
        panel_x +
        (panel_width -
         (4 * tile_width +
          3 * tile_gap)) /
            2;

    int tile_y =
        panel_y + 124;

    Game_DrawStatTile(
        "TIME",
        TextFormat(
            "%d:%02d",
            seconds / 60,
            seconds % 60),
        tile_x,
        tile_y,
        tile_width,
        accent_color);

    Game_DrawStatTile(
        "DEFEATED",
        TextFormat(
            "%d / %d",
            defeated,
            run_start_enemies),
        tile_x + (tile_width + tile_gap),
        tile_y,
        tile_width,
        accent_color);

    Game_DrawStatTile(
        "DAMAGE TAKEN",
        TextFormat(
            "%d",
            profile->total_damage_taken),
        tile_x + 2 * (tile_width + tile_gap),
        tile_y,
        tile_width,
        accent_color);

    Game_DrawStatTile(
        "DODGES",
        TextFormat(
            "%d",
            profile->total_dodges),
        tile_x + 3 * (tile_width + tile_gap),
        tile_y,
        tile_width,
        accent_color);

    /*
     * What the enemies learned about the player.
     */
    Game_DrawCenteredText(
        "WHAT THE ENEMIES LEARNED",
        center_x,
        panel_y + 208,
        12,
        UI_GOLD);

    float dodge_share = 0.0f;

    int dodge_direction =
        Game_DominantDirection(
            profile->dodge_left_probability,
            profile->dodge_right_probability,
            profile->dodge_up_probability,
            profile->dodge_down_probability,
            &dodge_share);

    float move_share = 0.0f;

    int move_direction =
        Game_DominantDirection(
            profile->move_left_probability,
            profile->move_right_probability,
            profile->move_up_probability,
            profile->move_down_probability,
            &move_share);

    bool has_dodges =
        profile->total_dodges >= 3;

    bool has_movement =
        profile->total_movements >= 10;

    if (has_dodges)
    {
        Game_DrawCenteredText(
            TextFormat(
                "You dodge %s %.0f%% of the time",
                game_direction_names[dodge_direction],
                dodge_share * 100.0f),
            center_x,
            panel_y + 236,
            14,
            UI_TEXT);
    }
    else
    {
        Game_DrawCenteredText(
            "Dodge at least 3 times to teach the enemies",
            center_x,
            panel_y + 236,
            14,
            UI_TEXT_DIM);
    }

    if (has_movement)
    {
        Game_DrawCenteredText(
            TextFormat(
                "You mostly move %s (%.0f%%)",
                game_direction_names[move_direction],
                move_share * 100.0f),
            center_x,
            panel_y + 262,
            14,
            UI_TEXT);
    }
    else
    {
        Game_DrawCenteredText(
            "Too little movement to learn from",
            center_x,
            panel_y + 262,
            14,
            UI_TEXT_DIM);
    }

    if (has_dodges)
    {
        const char *counter;

        if (dodge_direction == 0)
            counter = "Assassin and Boss countered by flanking right";
        else if (dodge_direction == 1)
            counter = "Assassin and Boss countered by flanking left";
        else
            counter = "Assassin and Boss countered by rushing you head-on";

        Game_DrawCenteredText(
            counter,
            center_x,
            panel_y + 288,
            14,
            UI_GOLD);
    }

    FightStyle style =
        Behavior_GetFightStyle();

    if (style != FIGHT_STYLE_UNKNOWN)
    {
        const char *style_text;

        if (style == FIGHT_STYLE_CLOSE)
            style_text = "You fight up close";
        else if (style == FIGHT_STYLE_MID)
            style_text = "You fight at mid range";
        else
            style_text = "You fight at the edge of your reach";

        Game_DrawCenteredText(
            TextFormat(
                "%s (%.0f%% of your attacks)",
                style_text,
                Behavior_GetFightStyleShare() * 100.0f),
            center_x,
            panel_y + 310,
            14,
            UI_TEXT);
    }

    DrawLine(
        panel_x + 40,
        panel_y + 332,
        panel_x + panel_width - 40,
        panel_y + 332,
        Fade(WHITE, 0.16f));

    /*
     * Buttons.
     */
    const int button_width = 200;
    const int button_gap = 20;

    int button_x =
        center_x -
        (2 * button_width + button_gap) / 2;

    Game_DrawMenuButton(
        "R   NEW RUN",
        button_x,
        panel_y + 352,
        button_width,
        42,
        accent_color);

    Game_DrawMenuButton(
        "M   MAIN MENU",
        button_x + button_width + button_gap,
        panel_y + 352,
        button_width,
        42,
        UI_GOLD);

    const char *seed_text =
        TextFormat(
            "SEED %08X",
            dungeon.seed);

    int seed_width =
        UI_NumberWidth(
            seed_text,
            10);

    UI_Number(
        seed_text,
        center_x - seed_width / 2,
        panel_y + 402,
        10,
        UI_TEXT_DIM);

    Game_DrawCenteredText(
        "DUNGEONFORGE",
        center_x,
        panel_y + panel_height - 28,
        10,
        Fade(UI_GOLD, 0.65f));
}

/*
 * ============================================================
 * MAIN MENU
 * ============================================================
 */

static void Game_RenderMenu(void)
{
    const int screen_width =
        Game_GetScreenWidth();

    const int screen_height =
        Game_GetScreenHeight();

    const int center_x =
        screen_width / 2;

    DrawRectangle(
        0,
        0,
        screen_width,
        screen_height,
        Fade(BLACK, 0.80f));

    float pulse =
        (sinf((float)GetTime() * 2.0f) + 1.0f) *
        0.5f;

    const char *title =
        "DUNGEONFORGE";

    int title_width =
        UI_TextWidth(
            title,
            40);

    int title_y =
        screen_height / 2 - 190;

    UI_Text(
        title,
        center_x - title_width / 2 + 3,
        title_y + 3,
        40,
        Fade(UI_BOSS_RED, 0.8f));

    UI_Text(
        title,
        center_x - title_width / 2,
        title_y,
        40,
        UI_GOLD);

    DrawRectangle(
        center_x - 150,
        title_y + 62,
        300,
        2,
        Fade(UI_GOLD, 0.35f + 0.35f * pulse));

    Game_DrawCenteredText(
        "A ROGUELIKE WHERE THE ENEMIES LEARN HOW YOU FIGHT",
        center_x,
        title_y + 80,
        14,
        UI_TEXT_DIM);

    const int button_width = 300;
    const int button_height = 44;

    int button_x =
        center_x -
        button_width / 2;

    int button_y =
        title_y + 130;

    Game_DrawMenuButton(
        "ENTER   NEW RUN",
        button_x,
        button_y,
        button_width,
        button_height,
        UI_GREEN);

    Game_DrawMenuButton(
        "L   LOAD SAVED GAME",
        button_x,
        button_y + 56,
        button_width,
        button_height,
        UI_CYAN);

    Game_DrawMenuButton(
        "ESC   QUIT",
        button_x,
        button_y + 112,
        button_width,
        button_height,
        UI_HP_RED);

    if (save_status ==
        SAVE_STATUS_LOAD_FAILED)
    {
        Game_DrawCenteredText(
            "NO VALID SAVE FOUND",
            center_x,
            button_y + 170,
            14,
            UI_HP_RED);
    }

    Game_DrawCenteredText(
        "WASD MOVE     MOUSE AIM     CLICK OR SPACE ATTACK     SHIFT DODGE     H POTION     ESC PAUSE",
        center_x,
        screen_height - 60,
        12,
        UI_TEXT_DIM);
}

/*
 * ============================================================
 * PAUSE SCREEN
 * ============================================================
 */

static void Game_RenderPauseScreen(void)
{
    const int screen_width =
        Game_GetScreenWidth();

    const int screen_height =
        Game_GetScreenHeight();

    const int center_x =
        screen_width / 2;

    DrawRectangle(
        0,
        0,
        screen_width,
        screen_height,
        Fade(BLACK, 0.62f));

    const int panel_width = 380;
    const int panel_height = 300;

    int panel_x =
        center_x -
        panel_width / 2;

    int panel_y =
        screen_height / 2 -
        panel_height / 2;

    Game_DrawPanel(
        panel_x,
        panel_y,
        panel_width,
        panel_height,
        UI_GOLD);

    Game_DrawCenteredText(
        "PAUSED",
        center_x,
        panel_y + 22,
        30,
        UI_GOLD);

    const int button_width = 260;
    const int button_height = 40;

    int button_x =
        center_x -
        button_width / 2;

    int button_y =
        panel_y + 80;

    Game_DrawMenuButton(
        "ESC   RESUME",
        button_x,
        button_y,
        button_width,
        button_height,
        UI_GREEN);

    Game_DrawMenuButton(
        "F6   SAVE GAME",
        button_x,
        button_y + 52,
        button_width,
        button_height,
        UI_GOLD);

    Game_DrawMenuButton(
        "F7   LOAD GAME",
        button_x,
        button_y + 104,
        button_width,
        button_height,
        UI_CYAN);

    Game_DrawMenuButton(
        "M   MAIN MENU",
        button_x,
        button_y + 156,
        button_width,
        button_height,
        UI_HP_RED);
}
/*
 * ============================================================
 * DUNGEON GRAPH DEBUG
 * ============================================================
 *
 * The world part is drawn through the camera. The panel is
 * drawn in screen space so it does not scroll.
 */

static void Game_RenderDungeonGraphWorld(void)
{
    if (!dungeon_debug_enabled)
        return;

    for (int i = 0;
         i < dungeon.connection_count;
         i++)
    {
        DungeonConnection *connection =
            &dungeon.connections[i];

        if (connection->room_a < 0 ||
            connection->room_a >= dungeon.room_count ||
            connection->room_b < 0 ||
            connection->room_b >= dungeon.room_count)
        {
            continue;
        }

        DungeonRoom *room_a =
            &dungeon.rooms[connection->room_a];

        DungeonRoom *room_b =
            &dungeon.rooms[connection->room_b];

        DrawLineEx(
            (Vector2){
                room_a->center_x * TILE_SIZE + TILE_SIZE * 0.5f,
                room_a->center_y * TILE_SIZE + TILE_SIZE * 0.5f},
            (Vector2){
                room_b->center_x * TILE_SIZE + TILE_SIZE * 0.5f,
                room_b->center_y * TILE_SIZE + TILE_SIZE * 0.5f},
            3.0f,
            YELLOW);
    }

    for (int i = 0;
         i < dungeon.room_count;
         i++)
    {
        DungeonRoom *room =
            &dungeon.rooms[i];

        float center_x =
            room->center_x * TILE_SIZE + TILE_SIZE * 0.5f;

        float center_y =
            room->center_y * TILE_SIZE + TILE_SIZE * 0.5f;

        DrawCircle(
            (int)center_x,
            (int)center_y,
            7.0f,
            ORANGE);

        UI_Text(
            TextFormat("R%d", i),
            (int)center_x + 8,
            (int)center_y - 8,
            14,
            WHITE);
    }
}

static void Game_RenderDungeonGraphPanel(void)
{
    if (!dungeon_debug_enabled)
        return;

    Game_DrawPanel(
        190,
        88,
        250,
        58,
        UI_CYAN);

    UI_Text(
        TextFormat(
            "ROOMS  %d",
            dungeon.room_count),
        200,
        98,
        14,
        WHITE);

    UI_Text(
        TextFormat(
            "CONNECTIONS  %d",
            dungeon.connection_count),
        200,
        120,
        14,
        WHITE);
}

/*
 * ============================================================
 * A* PATHFINDING DEBUG
 * ============================================================
 *
 * Only enemies near the player are tested, to keep this
 * overlay light on the large map.
 */

#define PATH_DEBUG_RANGE 700.0f

static void Game_RenderPathfindingWorld(void)
{
    if (!pathfinding_debug_enabled)
        return;

    Player *player =
        Player_Get();

    if (player == NULL)
        return;

    for (int i = 0;
         i < MAX_ENEMIES;
         i++)
    {
        Enemy *enemy =
            Enemy_Get(i);

        if (enemy == NULL ||
            !enemy->active)
        {
            continue;
        }

        float dx = enemy->x - player->x;
        float dy = enemy->y - player->y;

        if (dx * dx + dy * dy >
            PATH_DEBUG_RANGE * PATH_DEBUG_RANGE)
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

            UI_Text(
                TextFormat("NO PATH E%d", i),
                (int)enemy->x + 20,
                (int)enemy->y - 25,
                12,
                RED);

            continue;
        }

        DrawLineEx(
            (Vector2){enemy->x, enemy->y},
            (Vector2){next_x, next_y},
            3.0f,
            SKYBLUE);

        DrawCircle(
            (int)next_x,
            (int)next_y,
            6.0f,
            SKYBLUE);

        UI_Text(
            TextFormat("E%d A*", i),
            (int)enemy->x + 20,
            (int)enemy->y + 10,
            12,
            SKYBLUE);

        DrawLineEx(
            (Vector2){enemy->x, enemy->y},
            (Vector2){player->x, player->y},
            1.0f,
            Fade(RED, 0.35f));
    }
}

static void Game_RenderPathfindingPanel(void)
{
    if (!pathfinding_debug_enabled)
        return;

    Game_DrawPanel(
        190,
        154,
        300,
        58,
        UI_CYAN);

    UI_Text(
        "A* PATHFINDING DEBUG",
        200,
        164,
        15,
        UI_CYAN);

    UI_Text(
        "Next waypoint for enemies near you",
        200,
        186,
        13,
        WHITE);
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
    UI_Text(
        "F3 Enemy",
        66,
        footer_y + 11,
        9,
        UI_TEXT_DIM);

    UI_Text(
        "F4 Graph",
        138,
        footer_y + 11,
        9,
        UI_TEXT_DIM);

    UI_Text(
        "F5 A*",
        213,
        footer_y + 11,
        9,
        UI_TEXT_DIM);

    UI_Text(
        "F6 Save",
        264,
        footer_y + 11,
        9,
        UI_TEXT_DIM);

    UI_Text(
        "F7 Load",
        334,
        footer_y + 11,
        9,
        UI_TEXT_DIM);

    UI_Number(
        TextFormat(
            "SEED %08X",
            dungeon.seed),
        420,
        footer_y + 11,
        10,
        UI_TEXT_DIM);

    /*
     * Save/load status.
     */
    if (save_status != SAVE_STATUS_NONE)
    {
        const char *status_text = "";
        Color status_color = UI_TEXT_DIM;
        int status_width = 70;

        switch (save_status)
        {
        case SAVE_STATUS_SAVED:
            status_text = "SAVED";
            status_color = UI_GREEN;
            status_width = 58;
            break;

        case SAVE_STATUS_SAVE_FAILED:
            status_text = "SAVE FAILED";
            status_color = UI_HP_RED;
            status_width = 92;
            break;

        case SAVE_STATUS_LOADED:
            status_text = "LOADED";
            status_color = UI_GREEN;
            status_width = 64;
            break;

        case SAVE_STATUS_LOAD_FAILED:
            status_text = "LOAD FAILED";
            status_color = UI_HP_RED;
            status_width = 92;
            break;

        case SAVE_STATUS_NONE:
            break;
        }

        Game_DrawBadge(
            status_text,
            screen_width - status_width - 8,
            footer_y + 7,
            status_width,
            20,
            status_color);
    }
    /*
     * Center identity.
     */
    const char *identity =
        "DUNGEONFORGE";

    int identity_width =
        UI_TextWidth(
            identity,
            9);

    UI_Text(
        identity,
        (screen_width -
         identity_width) /
            2,
        footer_y + 11,
        9,
        Fade(UI_GOLD, 0.55f));
}

/*
 * Save/load message when the debug footer is hidden.
 */
static void Game_RenderSaveToast(void)
{
    if (ui_debug_visible)
        return;

    const char *text = NULL;
    Color color = UI_TEXT_DIM;

    switch (save_status)
    {
    case SAVE_STATUS_SAVED:
        text = "GAME SAVED";
        color = UI_GREEN;
        break;

    case SAVE_STATUS_SAVE_FAILED:
        text = "SAVE FAILED";
        color = UI_HP_RED;
        break;

    case SAVE_STATUS_LOADED:
        text = "GAME LOADED";
        color = UI_GREEN;
        break;

    case SAVE_STATUS_LOAD_FAILED:
        text = "LOAD FAILED";
        color = UI_HP_RED;
        break;

    case SAVE_STATUS_NONE:
        break;
    }

    if (text != NULL)
    {
        int width = UI_TextWidth(text, 10) + 28;

        Game_DrawBadge(
            text,
            (Game_GetScreenWidth() - width) / 2,
            10,
            width,
            22,
            color);
    }

    UI_Text(
        "F1  DEBUG",
        8,
        Game_GetScreenHeight() - 18,
        9,
        Fade(UI_TEXT_DIM, 0.6f));
}

/*
 * ============================================================
 * LOW HEALTH WARNING
 * ============================================================
 *
 * Red pulsing edges, faster and stronger the lower the
 * player's health.
 */

#define LOW_HEALTH_THRESHOLD 30

static void Game_RenderLowHealthWarning(void)
{
    Player *player =
        Player_Get();

    if (player == NULL ||
        player->dead ||
        player->health > LOW_HEALTH_THRESHOLD ||
        combat_intensity < 0.01f)
    {
        return;
    }

    float severity =
        1.0f -
        (float)player->health /
            (float)LOW_HEALTH_THRESHOLD;

    float pulse =
        (sinf((float)GetTime() *
              (4.0f + 6.0f * severity)) +
         1.0f) *
        0.5f;

    /*
     * Strongest when health is low AND the player is
     * in a fight.
     */
    float alpha =
        (0.18f + 0.30f * severity) *
        (0.5f + 0.5f * pulse) *
        combat_intensity;

    Color edge = (Color){220, 30, 40, 0};
    Color clear = edge;

    edge.a = (unsigned char)(alpha * 255.0f);

    int w = GetScreenWidth();
    int h = GetScreenHeight();
    int band = 110;

    DrawRectangleGradientV(0, 0, w, band, edge, clear);
    DrawRectangleGradientV(0, h - band, w, band, clear, edge);
    DrawRectangleGradientH(0, 0, band, h, edge, clear);
    DrawRectangleGradientH(w - band, 0, band, h, clear, edge);
}

/*
 * Crosshair drawn where the player is aiming.
 */
static void Game_RenderCrosshair(void)
{
    Vector2 mouse =
        GetMousePosition();

    int x = (int)mouse.x;
    int y = (int)mouse.y;

    Color color = Fade(WHITE, 0.92f);
    Color shadow = Fade(BLACK, 0.70f);

    DrawCircleLines(x + 1, y + 1, 9.0f, shadow);
    DrawCircleLines(x, y, 9.0f, color);

    DrawLine(x - 15, y, x - 6, y, color);
    DrawLine(x + 6, y, x + 15, y, color);
    DrawLine(x, y - 15, x, y - 6, color);
    DrawLine(x, y + 6, x, y + 15, color);

    DrawCircle(x, y, 2.0f, color);
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
     * World, drawn through a camera so the screen can shake.
     */
    Camera2D camera =
        GameCamera_Get();

    BeginMode2D(camera);

    {
        int min_x;
        int min_y;
        int max_x;
        int max_y;

        GameCamera_GetVisibleTiles(
            &min_x,
            &min_y,
            &max_x,
            &max_y);

        TileMap_RenderRegion(
            min_x,
            min_y,
            max_x,
            max_y);
    }

    Game_RenderDungeonGraphWorld();

    Game_RenderPathfindingWorld();

    ItemDrop_Render();

    Enemy_Render();

    Player_Render();

    Effects_RenderWorld();

    EndMode2D();

    Lighting_Render();

    /*
     * Screen-space UI (does not shake).
     */
    if (game_state ==
            GAME_STATE_PLAYING ||
        game_state ==
            GAME_STATE_PAUSED)
    {
        Game_RenderLowHealthWarning();

        Game_RenderDungeonGraphPanel();

        Game_RenderPathfindingPanel();

        Game_RenderPlayerHUD();

        Game_RenderBossHUD();

        Game_RenderInventoryHUD();

        Minimap_Render();

        if (ui_debug_visible)
            Game_RenderDebugFooter();

        Game_RenderSaveToast();
    }

    if (game_state ==
            GAME_STATE_DEAD ||
        game_state ==
            GAME_STATE_WON)
    {
        Game_RenderEndScreen();
    }

    if (game_state ==
        GAME_STATE_PAUSED)
    {
        Game_RenderPauseScreen();
    }

    if (game_state ==
        GAME_STATE_MENU)
    {
        Game_RenderMenu();
    }

    {
        static bool cursor_hidden = false;

        bool aiming =
            (game_state == GAME_STATE_PLAYING);

        if (aiming && !cursor_hidden)
        {
            HideCursor();
            cursor_hidden = true;
        }
        else if (!aiming && cursor_hidden)
        {
            ShowCursor();
            cursor_hidden = false;
        }

        if (aiming)
            Game_RenderCrosshair();
    }

    EndDrawing();
}

void Game_Shutdown(void)
{
    Audio_Shutdown();

    Lighting_Shutdown();

    UI_Shutdown();

    CloseWindow();
}