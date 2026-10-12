#include "save.h"
#include "behavior.h"
#include "serialization.h"
#include "player.h"
#include "enemy.h"
#include "item_drop.h"
#include "dungeon.h"
#include "game.h"
#include <math.h>
#include "items.h"
#include "tilemap.h"

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
#define SAVE_VERSION 7u

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
 * Save Validation
 * ---------------------------------------------------------
 *
 * The whole file is checked BEFORE any game state is
 * touched, so a bad save can never leave a half-loaded
 * world.
 */

#define SAVE_PLAYER_MAX_HEALTH PLAYER_MAX_HEALTH
#define SAVE_MAX_QUANTITY 9999
#define SAVE_MAX_ENEMY_HEALTH 10000
#define SAVE_MAX_TIMER 1000.0f

static bool Save_BoolValid(const bool *value)
{
    unsigned char raw;

    memcpy(&raw, value, 1);

    return raw <= 1;
}

static bool Save_TimerValid(float value)
{
    return isfinite(value) &&
           value >= 0.0f &&
           value <= SAVE_MAX_TIMER;
}

static bool Save_PositionValid(float x, float y)
{
    return isfinite(x) &&
           isfinite(y) &&
           x >= 0.0f &&
           x <= (float)(MAP_WIDTH * TILE_SIZE) &&
           y >= 0.0f &&
           y <= (float)(MAP_HEIGHT * TILE_SIZE);
}

static bool Save_InventoryValid(const Inventory *inventory)
{
    if (inventory->item_count < 0 ||
        inventory->item_count > INVENTORY_MAX_ITEMS)
    {
        return false;
    }

    int used = 0;

    for (int i = 0; i < INVENTORY_MAX_ITEMS; i++)
    {
        const InventoryItem *item =
            &inventory->items[i];

        if ((int)item->type < (int)ITEM_NONE ||
            (int)item->type > (int)ITEM_ARMOR)
        {
            return false;
        }

        if (item->type == ITEM_NONE)
        {
            if (item->item_id != 0 ||
                item->quantity != 0)
            {
                return false;
            }

            continue;
        }

        int max_quantity =
            item->type == ITEM_HEALTH_POTION
                ? INVENTORY_MAX_STACK
                : 1;

        if (!Items_IsValid((ItemId)item->item_id) ||
            item->quantity < 1 ||
            item->quantity > max_quantity)
        {
            return false;
        }

        used++;
    }

    return used == inventory->item_count;
}

static bool Save_PlayerValid(const PlayerSaveData *player)
{
    return Save_PositionValid(player->x, player->y) &&
           isfinite(player->speed) &&
           player->speed > 0.0f &&
           player->speed <= 2000.0f &&
           player->health >= 0 &&
           player->health <= SAVE_PLAYER_MAX_HEALTH &&
           Save_BoolValid(&player->dead) &&
           Save_InventoryValid(&player->inventory);
}

static bool Save_EnemyValid(const EnemySaveData *e)
{
    if (!Save_BoolValid(&e->active))
        return false;

    /*
     * Inactive slots are simply cleared on load.
     */
    if (!e->active)
        return true;

    if ((int)e->state < (int)ENEMY_STATE_IDLE ||
        (int)e->state > (int)ENEMY_STATE_DEAD)
        return false;

    if ((int)e->adaptive_type < (int)ADAPTIVE_AI_HUNTER ||
        (int)e->adaptive_type > (int)ADAPTIVE_AI_BOSS)
        return false;

    if ((int)e->adaptive_decision < (int)ADAPTIVE_DECISION_NONE ||
        (int)e->adaptive_decision > (int)ADAPTIVE_DECISION_KEEP_DISTANCE)
        return false;

    if ((int)e->boss_state < (int)BOSS_STATE_IDLE ||
        (int)e->boss_state > (int)BOSS_STATE_ENRAGED)
        return false;

    if ((int)e->boss_phase < (int)BOSS_PHASE_ONE ||
        (int)e->boss_phase > (int)BOSS_PHASE_THREE)
        return false;

    if (!Save_BoolValid(&e->path_valid) ||
        !Save_BoolValid(&e->is_boss))
        return false;

    if (e->max_health <= 0 ||
        e->max_health > SAVE_MAX_ENEMY_HEALTH ||
        e->health < 0 ||
        e->health > e->max_health)
        return false;

    if (!isfinite(e->radius) ||
        e->radius < 5.0f ||
        e->radius > 60.0f)
        return false;

    if (!isfinite(e->speed) ||
        e->speed < 0.0f ||
        e->speed > 1000.0f)
        return false;

    if (!isfinite(e->adaptation_level) ||
        e->adaptation_level < 0.0f ||
        e->adaptation_level > 1.0f)
        return false;

    if (!Save_PositionValid(e->x, e->y) ||
        !Save_PositionValid(e->target_x, e->target_y) ||
        !Save_PositionValid(e->waypoint_x, e->waypoint_y))
        return false;

    return Save_TimerValid(e->attack_cooldown) &&
           Save_TimerValid(e->attack_timer) &&
           Save_TimerValid(e->hurt_timer) &&
           Save_TimerValid(e->awareness_timer) &&
           Save_TimerValid(e->path_timer) &&
           Save_TimerValid(e->adaptive_timer) &&
           Save_TimerValid(e->boss_state_timer) &&
           Save_TimerValid(e->boss_special_timer) &&
           Save_TimerValid(e->boss_recovery_timer);
}

static bool Save_DropValid(const ItemDropSaveData *drop)
{
    if (!Save_BoolValid(&drop->active))
        return false;

    if (!drop->active)
        return true;

    return Save_PositionValid(drop->x, drop->y) &&
           Items_IsValid(drop->item_id) &&
           drop->quantity >= 1 &&
           drop->quantity <= SAVE_MAX_QUANTITY;
}

static bool Save_ValidateData(const SaveGameData *data)
{
    if (!Save_PlayerValid(&data->player))
        return false;

    int boss_count = 0;

    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (!Save_EnemyValid(&data->enemies[i]))
            return false;

        if (data->enemies[i].active &&
            data->enemies[i].is_boss)
        {
            boss_count++;
        }
    }

    if (boss_count > 1)
        return false;

    for (int i = 0; i < SAVE_MAX_DROPS; i++)
    {
        if (!Save_DropValid(&data->drops[i]))
            return false;
    }

    return Behavior_IsValid(&data->behavior);
}

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
     * Validate the ENTIRE file before touching any
     * game state.
     */
    if (!Save_ValidateData(&save_data))
        return false;

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