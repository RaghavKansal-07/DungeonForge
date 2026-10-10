#include "save.h"
#include "behavior.h"
#include "serialization.h"
#include "player.h"
#include "enemy.h"
#include "item_drop.h"
#include "dungeon.h"
#include "game.h"

#include <stdint.h>
#include <string.h>

/*
 * Save file location.
 */
#define SAVE_FILE "saves/save.dat"

/*
 * Save file identification.
 *
 * The magic number lets us detect whether
 * the file is actually a DungeonForge save.
 */
#define SAVE_MAGIC 0x44464F52u

/*
 * Increase this whenever the save-file
 * format changes incompatibly.
 */
#define SAVE_VERSION 3u

/*
 * Maximum number of item drops that can
 * exist in the game.
 *
 * This must match ITEM_DROP_MAX.
 */
#define SAVE_MAX_DROPS ITEM_DROP_MAX

/*
 * ---------------------------------------------------------
 * Save Header
 * ---------------------------------------------------------
 */

typedef struct
{
    uint32_t magic;
    uint32_t version;

} SaveHeader;

/*
 * ---------------------------------------------------------
 * Player Save Data
 * ---------------------------------------------------------
 *
 * Only persistent gameplay state is stored.
 *
 * Runtime-specific systems such as timers
 * are intentionally not part of the save format.
 */

typedef struct
{
    float x;
    float y;

    float speed;

    int health;

    bool dead;

    Inventory inventory;

} PlayerSaveData;

/*
 * ---------------------------------------------------------
 * Enemy Save Data
 * ---------------------------------------------------------
 *
 * Enemy itself is already a plain data structure,
 * but we keep the save format explicitly defined
 * instead of directly serializing internal runtime
 * arrays.
 */

typedef struct
{
    /* Basic enemy state */
    bool active;

    float x;
    float y;

    float speed;

    int health;
    int max_health;

    float radius;

    EnemyState state;

    /* Combat / navigation state */
    float attack_cooldown;
    float attack_timer;

    float hurt_timer;

    float target_x;
    float target_y;

    float awareness_timer;

    float path_timer;

    float waypoint_x;
    float waypoint_y;

    bool path_valid;

    /* Adaptive Enemy Intelligence state */
    AdaptiveAIType adaptive_type;
    AdaptiveDecision adaptive_decision;

    float adaptation_level;
    float adaptive_timer;

    /* Boss identity and state */
    bool is_boss;

    BossState boss_state;
    BossPhase boss_phase;

    float boss_state_timer;
    float boss_special_timer;
    float boss_recovery_timer;

} EnemySaveData;

/*
 * ---------------------------------------------------------
 * Item Drop Save Data
 * ---------------------------------------------------------
 */

typedef struct
{
    bool active;

    float x;
    float y;

    ItemId item_id;

    int quantity;

} ItemDropSaveData;

/*
 * ---------------------------------------------------------
 * Complete Save Data
 * ---------------------------------------------------------
 */

typedef struct
{
    SaveHeader header;

    /*
     * The dungeon itself is NOT serialized.
     *
     * The same seed regenerates the same dungeon.
     */
    uint32_t dungeon_seed;

    PlayerSaveData player;

    EnemySaveData enemies[MAX_ENEMIES];

    ItemDropSaveData drops[SAVE_MAX_DROPS];

    /*
     * Learned player behavior, so enemies keep
     * adapting after a load.
     */
    PlayerBehavior behavior;

} SaveGameData;

/*
 * ---------------------------------------------------------
 * Save_Game
 * ---------------------------------------------------------
 */

bool Save_Game(void)
{
    SaveGameData save_data;

    memset(
        &save_data,
        0,
        sizeof(save_data));

    /*
     * Save header.
     */
    save_data.header.magic =
        SAVE_MAGIC;

    save_data.header.version =
        SAVE_VERSION;

    /*
     * -----------------------------------------------------
     * Dungeon
     * -----------------------------------------------------
     *
     * Store the seed of the ACTUAL dungeon currently
     * being played.
     *
     * We do not serialize the complete dungeon because
     * Dungeon_Init() can deterministically reconstruct
     * the same dungeon from this seed.
     */
    Dungeon *dungeon =
        Game_GetDungeon();

    if (dungeon == NULL)
        return false;

    save_data.dungeon_seed =
        dungeon->seed;

    /*
     * -----------------------------------------------------
     * Player
     * -----------------------------------------------------
     */

    Player *player =
        Player_Get();

    if (player == NULL)
        return false;

    save_data.player.x =
        player->x;

    save_data.player.y =
        player->y;

    save_data.player.speed =
        player->speed;

    save_data.player.health =
        player->health;

    save_data.player.dead =
        player->dead;

    memcpy(
        &save_data.player.inventory,
        &player->inventory,
        sizeof(Inventory));

    /*
     * -----------------------------------------------------
     * Enemies
     * -----------------------------------------------------
     */

    for (int i = 0;
         i < MAX_ENEMIES;
         i++)
    {
        Enemy *enemy =
            Enemy_Get(i);

        if (enemy == NULL)
            continue;

        /* Basic enemy state */
        save_data.enemies[i].active =
            enemy->active;

        save_data.enemies[i].x =
            enemy->x;

        save_data.enemies[i].y =
            enemy->y;

        save_data.enemies[i].speed =
            enemy->speed;

        save_data.enemies[i].health =
            enemy->health;

        save_data.enemies[i].max_health =
            enemy->max_health;

        save_data.enemies[i].radius =
            enemy->radius;

        save_data.enemies[i].state =
            enemy->state;

        /* Combat / navigation state */
        save_data.enemies[i].attack_cooldown =
            enemy->attack_cooldown;

        save_data.enemies[i].attack_timer =
            enemy->attack_timer;

        save_data.enemies[i].hurt_timer =
            enemy->hurt_timer;

        save_data.enemies[i].target_x =
            enemy->target_x;

        save_data.enemies[i].target_y =
            enemy->target_y;

        save_data.enemies[i].awareness_timer =
            enemy->awareness_timer;

        save_data.enemies[i].path_timer =
            enemy->path_timer;

        save_data.enemies[i].waypoint_x =
            enemy->waypoint_x;

        save_data.enemies[i].waypoint_y =
            enemy->waypoint_y;

        save_data.enemies[i].path_valid =
            enemy->path_valid;

        /* Adaptive Enemy Intelligence state */
        save_data.enemies[i].adaptive_type =
            enemy->adaptive_type;

        save_data.enemies[i].adaptive_decision =
            enemy->adaptive_decision;

        save_data.enemies[i].adaptation_level =
            enemy->adaptation_level;

        save_data.enemies[i].adaptive_timer =
            enemy->adaptive_timer;

        /* Boss identity and state */
        save_data.enemies[i].is_boss =
            enemy->is_boss;

        save_data.enemies[i].boss_state =
            enemy->boss_state;

        save_data.enemies[i].boss_phase =
            enemy->boss_phase;

        save_data.enemies[i].boss_state_timer =
            enemy->boss_state_timer;

        save_data.enemies[i].boss_special_timer =
            enemy->boss_special_timer;

        save_data.enemies[i].boss_recovery_timer =
            enemy->boss_recovery_timer;
    }

    /*
     * -----------------------------------------------------
     * Item Drops
     * -----------------------------------------------------
     *
     * ItemDrop_Get() gives us access to active drops.
     */
    for (int i = 0;
         i < SAVE_MAX_DROPS;
         i++)
    {
        ItemDrop *drop =
            ItemDrop_Get(i);

        if (drop == NULL)
            continue;

        save_data.drops[i].active =
            drop->active;

        save_data.drops[i].x =
            drop->x;

        save_data.drops[i].y =
            drop->y;

        save_data.drops[i].item_id =
            drop->item_id;

        save_data.drops[i].quantity =
            drop->quantity;
    }

    /*
     * -----------------------------------------------------
     * Write save file
     * -----------------------------------------------------
     */

    save_data.behavior =
        *Behavior_GetProfile();

    return Serialization_Write(
        SAVE_FILE,
        &save_data,
        sizeof(save_data));
}

/*
 * ---------------------------------------------------------
 * Load_Game
 * ---------------------------------------------------------
 */

bool Load_Game(void)
{
    SaveGameData save_data;

    memset(
        &save_data,
        0,
        sizeof(save_data));

    /*
     * Read the complete save file.
     */
    if (!Serialization_Read(
            SAVE_FILE,
            &save_data,
            sizeof(save_data)))
    {
        return false;
    }

    /*
     * -----------------------------------------------------
     * Validate header
     * -----------------------------------------------------
     */

    if (save_data.header.magic !=
        SAVE_MAGIC)
    {
        return false;
    }

    if (save_data.header.version !=
        SAVE_VERSION)
    {
        return false;
    }

    /*
     * -----------------------------------------------------
     * Regenerate the actual game dungeon
     * -----------------------------------------------------
     *
     * Dungeon generation is deterministic.
     *
     * Therefore:
     *
     *     saved seed
     *          ↓
     *     Dungeon_Init()
     *          ↓
     *     same rooms
     *     same corridors
     *     same tile map
     *     same dungeon graph
     *
     * Game_GetDungeon() gives us the global dungeon
     * that the renderer and gameplay systems already use.
     */
    Dungeon *dungeon =
        Game_GetDungeon();

    if (dungeon == NULL)
        return false;

    Dungeon_Init(
        dungeon,
        save_data.dungeon_seed);

    /*
     * -----------------------------------------------------
     * Restore player
     * -----------------------------------------------------
     */

    Player_Init();

    Player *player =
        Player_Get();

    if (player == NULL)
        return false;

    player->x =
        save_data.player.x;

    player->y =
        save_data.player.y;

    player->speed =
        save_data.player.speed;

    player->health =
        save_data.player.health;

    player->dead =
        save_data.player.dead;

    memcpy(
        &player->inventory,
        &save_data.player.inventory,
        sizeof(Inventory));

    /*
     * -----------------------------------------------------
     * Restore enemies
     * -----------------------------------------------------
     *
     * Enemy_Init() resets the internal enemy
     * system first.
     *
     * Enemy_Restore() is then called for EVERY
     * enemy slot, including inactive/dead slots.
     *
     * This is important because Enemy_Init()
     * creates the default enemies. If a saved
     * slot is inactive, Enemy_Restore() must
     * explicitly clear that slot; otherwise
     * the freshly-created enemy would remain
     * after loading.
     */
    Enemy_Init();

    for (int i = 0;
         i < MAX_ENEMIES;
         i++)
    {
        Enemy enemy;

        memset(
            &enemy,
            0,
            sizeof(enemy));

        enemy.active =
            save_data.enemies[i].active;

        enemy.x =
            save_data.enemies[i].x;

        enemy.y =
            save_data.enemies[i].y;

        enemy.speed =
            save_data.enemies[i].speed;

        enemy.health =
            save_data.enemies[i].health;

        enemy.max_health =
            save_data.enemies[i].max_health;

        enemy.radius =
            save_data.enemies[i].radius;

        enemy.state =
            save_data.enemies[i].state;

        enemy.attack_cooldown =
            save_data.enemies[i].attack_cooldown;

        enemy.attack_timer =
            save_data.enemies[i].attack_timer;

        enemy.hurt_timer =
            save_data.enemies[i].hurt_timer;

        enemy.target_x =
            save_data.enemies[i].target_x;

        enemy.target_y =
            save_data.enemies[i].target_y;

        enemy.awareness_timer =
            save_data.enemies[i].awareness_timer;

        enemy.path_timer =
            save_data.enemies[i].path_timer;

        enemy.waypoint_x =
            save_data.enemies[i].waypoint_x;

        enemy.waypoint_y =
            save_data.enemies[i].waypoint_y;

        enemy.path_valid =
            save_data.enemies[i].path_valid;

        /* Adaptive Enemy Intelligence state */
        enemy.adaptive_type =
            save_data.enemies[i].adaptive_type;

        enemy.adaptive_decision =
            save_data.enemies[i].adaptive_decision;

        enemy.adaptation_level =
            save_data.enemies[i].adaptation_level;

        enemy.adaptive_timer =
            save_data.enemies[i].adaptive_timer;

        /* Boss identity and state */
        enemy.is_boss =
            save_data.enemies[i].is_boss;

        enemy.boss_state =
            save_data.enemies[i].boss_state;

        enemy.boss_phase =
            save_data.enemies[i].boss_phase;

        enemy.boss_state_timer =
            save_data.enemies[i].boss_state_timer;

        enemy.boss_special_timer =
            save_data.enemies[i].boss_special_timer;

        enemy.boss_recovery_timer =
            save_data.enemies[i].boss_recovery_timer;

        /*
         * Always restore the slot.
         *
         * Enemy_Restore() handles:
         *
         *     active saved enemy
         *     inactive saved slot
         *     dead saved enemy
         *
         * This prevents Enemy_Init() enemies
         * from surviving a load when they did
         * not exist in the saved state.
         */
        if (!Enemy_Restore(
                i,
                &enemy))
        {
            return false;
        }
    }

    /*
     * -----------------------------------------------------
     * Restore item drops
     * -----------------------------------------------------
     */

    ItemDrop_Init();

    for (int i = 0;
         i < SAVE_MAX_DROPS;
         i++)
    {
        if (!save_data.drops[i].active)
            continue;

        if (!ItemDrop_Spawn(
                save_data.drops[i].x,
                save_data.drops[i].y,
                save_data.drops[i].item_id,
                save_data.drops[i].quantity))
        {
            return false;
        }
    }

    if (!Behavior_Restore(
            &save_data.behavior))
    {
        return false;
    }

    return true;
}