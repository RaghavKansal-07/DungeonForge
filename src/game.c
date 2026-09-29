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
 * ------------------------------------------------------------
 * GAME ACCESS
 * ------------------------------------------------------------
 *
 * Provides controlled access to the currently active
 * dungeon for systems such as save/load.
 */
Dungeon *Game_GetDungeon(void)
{
    return &dungeon;
}


/*
 * ------------------------------------------------------------
 * START NEW RUN
 * ------------------------------------------------------------
 */
static void Game_StartRun(void)
{
    /*
     * Generate the procedural dungeon.
     *
     * The seed controls the dungeon layout.
     * Using the same seed produces the same
     * dungeon.
     */
    Dungeon_Init(
        &dungeon,
        12345
    );

    /*
     * Reset player state and inventory.
     */
    Player_Init();

    /*
     * Reset all world item drops.
     */
    ItemDrop_Init();

    /*
     * Reset the event queue.
     *
     * A new run must not contain events
     * left over from the previous run.
     */
    Events_Clear();

    /*
     * Reset the player's behavioral profile.
     *
     * Every dungeon run starts with no
     * previous behavioral knowledge.
     */
    Behavior_Init();

    /*
     * Place the player at the center of
     * the first generated dungeon room.
     */
    float spawn_x = 0.0f;
    float spawn_y = 0.0f;

    if (Dungeon_GetPlayerSpawn(
            &dungeon,
            &spawn_x,
            &spawn_y))
    {
        Player_SetPosition(
            spawn_x,
            spawn_y
        );
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
        "DungeonForge"
    );

    SetTargetFPS(60);

    /*
     * Initialize the event system before
     * gameplay starts.
     */
    Events_Init();

    /*
     * Initialize a fresh behavior profile.
     *
     * Game_StartRun() also resets it so that
     * restarting creates a completely fresh
     * behavioral profile.
     */
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
     * Save the current game.
     *
     * F6 writes the current player,
     * enemy, inventory and item-drop state.
     */
    if (IsKeyPressed(KEY_F6))
    {
        Save_Game();
    }

    /*
     * Load the previously saved game.
     *
     * F7 restores the saved game state.
     */
    if (IsKeyPressed(KEY_F7))
    {
        if (Load_Game())
        {
            /*
             * Events generated before loading
             * should not affect the newly restored
             * game state.
             */
            Events_Clear();

            /*
             * The current save format does not yet
             * serialize behavioral statistics.
             *
             * Therefore the behavior profile is
             * intentionally reset after loading.
             */
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
     * --------------------------------------------------------
     * NORMAL GAMEPLAY
     * --------------------------------------------------------
     */

    /*
     * Player actions generate gameplay events.
     */
    Player_Update();

    /*
     * Check whether the player died
     * during Player_Update().
     */
    if (Player_IsDead())
    {
        game_state =
            GAME_STATE_DEAD;

        /*
         * Process events generated before death.
         */
        Behavior_Update();

        return;
    }

    /*
     * Update enemies.
     *
     * Enemy actions can also generate events
     * such as PLAYER_DAMAGE.
     */
    Enemy_Update();

    /*
     * An enemy attack may have killed
     * the player during Enemy_Update().
     */
    if (Player_IsDead())
    {
        game_state =
            GAME_STATE_DEAD;

        /*
         * Process events generated during
         * this gameplay frame.
         */
        Behavior_Update();

        return;
    }

    /*
     * Update world item drops.
     *
     * This checks whether the player is
     * close enough to pick up an item.
     */
    ItemDrop_Update();

    /*
     * --------------------------------------------------------
     * BEHAVIOR ANALYSIS
     * --------------------------------------------------------
     *
     * All gameplay events generated during this
     * update are now consumed by the behavior analyzer.
     *
     * Example:
     *
     *     Player moves left
     *            ↓
     *     EVENT_PLAYER_MOVE
     *            ↓
     *     Events queue
     *            ↓
     *     Behavior_Update()
     *            ↓
     *     move_left_count++
     */
    Behavior_Update();

    /*
     * Check whether the dungeon has been
     * completely cleared.
     *
     * Victory requires:
     *
     *     1. No active enemies
     *     2. No active world item drops
     */
    if (Enemy_GetCount() == 0 &&
        ItemDrop_GetCount() == 0)
    {
        game_state =
            GAME_STATE_WON;
    }
}


/*
 * ------------------------------------------------------------
 * INVENTORY HUD
 * ------------------------------------------------------------
 *
 * Displays the player's current inventory
 * contents during gameplay.
 */
static void Game_RenderInventoryHUD(void)
{
    Player *player =
        Player_Get();

    if (player == NULL)
        return;

    /*
     * Inventory panel.
     */
    DrawRectangle(
        1030,
        10,
        235,
        100,
        Fade(
            BLACK,
            0.75f
        )
    );

    DrawText(
        "INVENTORY",
        1045,
        20,
        18,
        GOLD
    );

    /*
     * Display number of occupied inventory slots.
     */
    DrawText(
        TextFormat(
            "Slots: %d/%d",
            player->inventory.item_count,
            INVENTORY_MAX_ITEMS
        ),
        1045,
        43,
        14,
        WHITE
    );

    /*
     * Display health potion quantity.
     */
    int potion_quantity =
        Inventory_GetQuantity(
            &player->inventory,
            ITEM_ID_HEALTH_POTION
        );

    DrawText(
        TextFormat(
            "Health Potion: %d",
            potion_quantity
        ),
        1045,
        65,
        14,
        WHITE
    );

    /*
     * Display every currently stored item.
     */
    int y =
        87;

    for (int i = 0;
         i < INVENTORY_MAX_ITEMS;
         i++)
    {
        InventoryItem *item =
            &player->inventory.items[i];

        if (item->type == ITEM_NONE)
            continue;

        const ItemDefinition *definition =
            Items_GetDefinition(
                (ItemId)item->item_id
            );

        if (definition == NULL)
            continue;

        /*
         * Avoid drawing outside the panel.
         */
        if (y > 100)
            break;

        DrawText(
            TextFormat(
                "%s x%d",
                definition->name,
                item->quantity
            ),
            1045,
            y,
            12,
            LIGHTGRAY
        );

        y += 14;
    }
}


static void Game_RenderDungeonGraphDebug(void)
{
    if (!dungeon_debug_enabled)
        return;

    /*
     * Draw every graph connection first.
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
            &dungeon.rooms[
                connection->room_a
            ];

        DungeonRoom *room_b =
            &dungeon.rooms[
                connection->room_b
            ];

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
            YELLOW
        );
    }

    /*
     * Draw room centers and room IDs.
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
            ORANGE
        );

        DrawText(
            TextFormat(
                "R%d",
                i
            ),
            (int)center_x + 8,
            (int)center_y - 8,
            14,
            WHITE
        );
    }

    /*
     * Display graph statistics.
     */
    DrawRectangle(
        8,
        35,
        250,
        55,
        Fade(
            BLACK,
            0.75f
        )
    );

    DrawText(
        TextFormat(
            "Rooms: %d",
            dungeon.room_count
        ),
        18,
        45,
        16,
        WHITE
    );

    DrawText(
        TextFormat(
            "Connections: %d",
            dungeon.connection_count
        ),
        18,
        67,
        16,
        WHITE
    );
}


/*
 * ------------------------------------------------------------
 * A* PATHFINDING DEBUG
 * ------------------------------------------------------------
 */
static void Game_RenderPathfindingDebug(void)
{
    if (!pathfinding_debug_enabled)
        return;

    Player *player =
        Player_Get();

    /*
     * Display debug panel.
     */
    DrawRectangle(
        8,
        95,
        300,
        55,
        Fade(
            BLACK,
            0.75f
        )
    );

    DrawText(
        "A* PATHFINDING DEBUG",
        18,
        105,
        16,
        SKYBLUE
    );

    DrawText(
        "Next waypoint for each enemy",
        18,
        127,
        14,
        WHITE
    );

    /*
     * Test the real A* function for every
     * active enemy.
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
                &next_y
            );

        if (!found)
        {
            /*
             * No valid A* route exists.
             */
            DrawCircle(
                (int)enemy->x,
                (int)enemy->y,
                enemy->radius + 5.0f,
                MAROON
            );

            DrawText(
                TextFormat(
                    "NO PATH E%d",
                    i
                ),
                (int)enemy->x + 20,
                (int)enemy->y - 25,
                12,
                RED
            );

            continue;
        }

        /*
         * Draw the calculated next waypoint.
         */
        DrawLineEx(
            (Vector2){
                enemy->x,
                enemy->y
            },
            (Vector2){
                next_x,
                next_y
            },
            3.0f,
            SKYBLUE
        );

        DrawCircle(
            (int)next_x,
            (int)next_y,
            6.0f,
            SKYBLUE
        );

        DrawText(
            TextFormat(
                "E%d A*",
                i
            ),
            (int)enemy->x + 20,
            (int)enemy->y + 10,
            12,
            SKYBLUE
        );

        /*
         * Draw the direct enemy -> player line
         * as a reference.
         */
        DrawLineEx(
            (Vector2){
                enemy->x,
                enemy->y
            },
            (Vector2){
                player->x,
                player->y
            },
            1.0f,
            Fade(
                RED,
                0.35f
            )
        );
    }
}


void Game_Render(void)
{
    BeginDrawing();

    ClearBackground(BLACK);

    TileMap_Render();

    /*
     * Draw the dungeon graph before entities.
     */
    Game_RenderDungeonGraphDebug();

    /*
     * Draw A* debug information before entities.
     */
    Game_RenderPathfindingDebug();

    /*
     * Draw world item drops before entities.
     */
    ItemDrop_Render();

    Enemy_Render();
    Player_Render();

    /*
     * Draw inventory HUD.
     */
    Game_RenderInventoryHUD();

    /*
     * Death overlay.
     */
    if (game_state ==
        GAME_STATE_DEAD)
    {
        DrawRectangle(
            0,
            0,
            1280,
            720,
            Fade(
                BLACK,
                0.65f
            )
        );

        DrawText(
            "YOU DIED",
            1280 / 2 - 100,
            300,
            40,
            RED
        );

        DrawText(
            "Press R to restart",
            1280 / 2 - 105,
            355,
            20,
            WHITE
        );
    }

    /*
     * Victory overlay.
     */
    if (game_state ==
        GAME_STATE_WON)
    {
        DrawRectangle(
            0,
            0,
            1280,
            720,
            Fade(
                BLACK,
                0.65f
            )
        );

        DrawText(
            "YOU WON!",
            1280 / 2 - 100,
            300,
            40,
            GREEN
        );

        DrawText(
            "Press R to play again",
            1280 / 2 - 120,
            355,
            20,
            WHITE
        );
    }

    /*
     * Debug controls.
     */
    DrawText(
        "F3: Toggle Enemy Debug",
        10,
        10,
        14,
        WHITE
    );

    DrawText(
        "F4: Toggle Dungeon Graph",
        10,
        110,
        14,
        WHITE
    );

    DrawText(
        "F5: Toggle A* Debug",
        10,
        130,
        14,
        WHITE
    );

    /*
     * Save/load controls.
     */
    DrawText(
        "F6: Save Game",
        10,
        170,
        14,
        WHITE
    );

    DrawText(
        "F7: Load Game",
        10,
        190,
        14,
        WHITE
    );

    /*
     * Health potion control.
     */
    DrawText(
        "H: Use Health Potion",
        10,
        210,
        14,
        WHITE
    );

    EndDrawing();
}


void Game_Shutdown(void)
{
    CloseWindow();
}