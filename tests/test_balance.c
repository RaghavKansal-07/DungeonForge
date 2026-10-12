#include <stdio.h>
#include <stdbool.h>

#include "balance.h"

/*
 * ---------------------------------------------------------
 * These tests check relationships between the balance numbers,
 * not the numbers themselves. They fail if a change makes the
 * game unfair (no time to react), trivial, or instantly lethal.
 * ---------------------------------------------------------
 */

static int tests_run = 0;
static int tests_passed = 0;

static void Test_Assert(
    bool condition,
    const char *test_name)
{
    tests_run++;

    if (condition)
    {
        tests_passed++;

        printf("[PASS] %s\n", test_name);
    }
    else
    {
        printf("[FAIL] %s\n", test_name);
    }
}

typedef struct
{
    const char *name;

    int health;
    int damage;

    float windup;
    float cooldown;

} NormalEnemy;

static const NormalEnemy normal_enemies[] =
    {
        {"Hunter",
         BAL_HUNTER_HEALTH, BAL_HUNTER_DAMAGE,
         BAL_HUNTER_WINDUP, BAL_HUNTER_COOLDOWN},

        {"Guardian",
         BAL_GUARDIAN_HEALTH, BAL_GUARDIAN_DAMAGE,
         BAL_GUARDIAN_WINDUP, BAL_GUARDIAN_COOLDOWN},

        {"Assassin",
         BAL_ASSASSIN_HEALTH, BAL_ASSASSIN_DAMAGE,
         BAL_ASSASSIN_WINDUP, BAL_ASSASSIN_COOLDOWN}};

#define ENEMY_COUNT \
    ((int)(sizeof(normal_enemies) / sizeof(normal_enemies[0])))

static int HitsToKill(
    int health,
    int damage)
{
    return (health + damage - 1) / damage;
}

static void Test_Fairness(void)
{
    int too_fast = 0;
    int immunity_blocks_swings = 0;

    for (int i = 0; i < ENEMY_COUNT; i++)
    {
        const NormalEnemy *e = &normal_enemies[i];

        /* A person needs about 0.3 s to react to a warning. */
        if (e->windup < 0.30f)
            too_fast++;

        /*
         * Damage immunity must be shorter than one swing cycle,
         * otherwise a single enemy could never hit twice in a row
         * and the immunity would mask the enemy's damage.
         */
        if (BAL_PLAYER_DAMAGE_IMMUNITY >=
            e->windup + e->cooldown)
        {
            immunity_blocks_swings++;
        }
    }

    Test_Assert(too_fast == 0,
                "Every normal enemy gives at least 0.30 s of warning");

    Test_Assert(immunity_blocks_swings == 0,
                "Damage immunity is shorter than every enemy's swing cycle");

    Test_Assert(BAL_BOSS_MELEE_WINDUP >= 0.40f,
                "The boss melee warning is at least 0.40 s");

    Test_Assert(BAL_BOSS_SPECIAL_WINDUP >= 0.60f,
                "The boss special warning is at least 0.60 s");

    Test_Assert(
        BAL_WINDUP_INTERRUPT_LIMIT > 0.2f &&
            BAL_WINDUP_INTERRUPT_LIMIT < 0.8f,
        "Only part of a wind-up can be interrupted");
}

static void Test_Geometry(void)
{
    /* Enemy radius 20 + player radius 20: bodies touch at 40 px. */
    Test_Assert(BAL_ENEMY_ATTACK_RANGE >= 40.0f + 25.0f,
                "The enemy swing reaches well beyond body contact");

    Test_Assert(BAL_BOSS_CONTACT_DISTANCE < BAL_BOSS_ATTACK_RANGE,
                "The boss attacks from farther than it backs away");

    Test_Assert(BAL_BOSS_SPECIAL_RANGE > BAL_BOSS_ATTACK_RANGE,
                "The boss special reaches farther than its melee");
}

static void Test_HitsToKill(void)
{
    int player_dies_too_fast = 0;
    int player_dies_too_slow = 0;
    int enemy_dies_too_fast = 0;
    int enemy_dies_too_slow = 0;

    for (int i = 0; i < ENEMY_COUNT; i++)
    {
        const NormalEnemy *e = &normal_enemies[i];

        int hits_on_player =
            HitsToKill(BAL_PLAYER_MAX_HEALTH, e->damage);

        int hits_on_enemy =
            HitsToKill(e->health, BAL_PLAYER_ATTACK_DAMAGE);

        if (hits_on_player < 5)
            player_dies_too_fast++;

        if (hits_on_player > 10)
            player_dies_too_slow++;

        if (hits_on_enemy < 3)
            enemy_dies_too_fast++;

        if (hits_on_enemy > 10)
            enemy_dies_too_slow++;
    }

    Test_Assert(player_dies_too_fast == 0,
                "No normal enemy kills the player in fewer than 5 hits");

    Test_Assert(player_dies_too_slow == 0,
                "Every normal enemy kills the player within 10 hits");

    Test_Assert(enemy_dies_too_fast == 0,
                "Every normal enemy takes at least 3 player hits");

    Test_Assert(enemy_dies_too_slow == 0,
                "Every normal enemy dies within 10 player hits");

    Test_Assert(
        HitsToKill(BAL_PLAYER_MAX_HEALTH, BAL_BOSS_ATTACK_DAMAGE) >= 4,
        "The boss needs at least 4 melee hits to kill the player");

    Test_Assert(BAL_BOSS_SPECIAL_DAMAGE >= BAL_BOSS_ATTACK_DAMAGE,
                "The boss special hurts at least as much as its melee");
}

static void Test_OneOnOne(void)
{
    /*
     * Simple duel: the player attacks nonstop and never dodges,
     * and the enemy swings nonstop. How much health does the
     * player lose before the enemy dies?
     *
     * Skilled play (dodging, interrupting) costs far less.
     * This is the cost of standing still and trading blows.
     */
    int too_cheap = 0;
    int too_costly = 0;

    for (int i = 0; i < ENEMY_COUNT; i++)
    {
        const NormalEnemy *e = &normal_enemies[i];

        float time_to_kill =
            (float)HitsToKill(e->health, BAL_PLAYER_ATTACK_DAMAGE) *
            BAL_PLAYER_ATTACK_COOLDOWN;

        float enemy_dps =
            (float)e->damage /
            (e->windup + e->cooldown);

        float damage_taken =
            enemy_dps * time_to_kill;

        float share =
            damage_taken /
            (float)BAL_PLAYER_MAX_HEALTH;

        if (share < 0.15f)
            too_cheap++;

        if (share > 0.70f)
            too_costly++;
    }

    Test_Assert(too_cheap == 0,
                "Trading blows with any normal enemy costs at least 15% health");

    Test_Assert(too_costly == 0,
                "Trading blows with any normal enemy costs at most 70% health");
}

static void Test_PlayerAttack(void)
{
    Test_Assert(
        BAL_PLAYER_SWING_MOVE_SCALE > 0.0f &&
            BAL_PLAYER_SWING_MOVE_SCALE < 1.0f,
        "Swinging slows the player down, but does not stop them");

    Test_Assert(
        BAL_PLAYER_SWING_TIME > 0.0f &&
            BAL_PLAYER_SWING_TIME < BAL_PLAYER_ATTACK_COOLDOWN,
        "The swing slowdown ends before the next attack is ready");
}

static void Test_Gear(void)
{
    Test_Assert(
        BAL_SWORD_DAMAGE_BONUS > 0 &&
            BAL_ARMOR_REDUCTION_PERCENT > 0 &&
            BAL_ARMOR_REDUCTION_PERCENT <= 60,
        "Gear helps the player but armor never blocks most damage");

    Test_Assert(
        BAL_HUNTER_SWORD_DROP_CHANCE > 0.0f &&
            BAL_HUNTER_SWORD_DROP_CHANCE < 1.0f &&
            BAL_ASSASSIN_SWORD_DROP_CHANCE > 0.0f &&
            BAL_ASSASSIN_SWORD_DROP_CHANCE < 1.0f &&
            BAL_GUARDIAN_ARMOR_DROP_CHANCE > 0.0f &&
            BAL_GUARDIAN_ARMOR_DROP_CHANCE < 1.0f,
        "Gear drops are possible but never guaranteed");

    /*
     * With a sword and armor, a duel should still cost something.
     */
    int too_cheap = 0;

    for (int i = 0; i < ENEMY_COUNT; i++)
    {
        const NormalEnemy *e = &normal_enemies[i];

        int damage =
            BAL_PLAYER_ATTACK_DAMAGE +
            BAL_SWORD_DAMAGE_BONUS;

        float time_to_kill =
            (float)HitsToKill(e->health, damage) *
            BAL_PLAYER_ATTACK_COOLDOWN;

        float enemy_dps =
            (float)e->damage *
            (100.0f - (float)BAL_ARMOR_REDUCTION_PERCENT) /
            100.0f /
            (e->windup + e->cooldown);

        float share =
            enemy_dps * time_to_kill /
            (float)BAL_PLAYER_MAX_HEALTH;

        if (share < 0.10f)
            too_cheap++;
    }

    Test_Assert(too_cheap == 0,
                "Even fully geared, trading blows costs at least 10% health");
}

static void Test_BossKeepDistance(void)
{
    Test_Assert(
        BAL_BOSS_KEEP_DISTANCE > BAL_BOSS_CONTACT_DISTANCE &&
            BAL_BOSS_KEEP_DISTANCE < BAL_BOSS_ATTACK_RANGE,
        "The boss keep-distance is between its contact and attack range");

    Test_Assert(
        BAL_BOSS_KEEP_SPECIAL_SCALE > 0.3f &&
            BAL_BOSS_KEEP_SPECIAL_SCALE < 1.0f,
        "Keeping distance speeds up the boss special, but not absurdly");
}

static void Test_Healing(void)
{
    Test_Assert(
        BAL_PLAYER_POTION_HEAL * 100 <= BAL_PLAYER_MAX_HEALTH * 40,
        "One potion heals at most 40% of the player's health");

    /*
     * Healing per second must stay well below the weakest
     * enemy's damage per second, so potions cannot out-heal
     * a fight.
     */
    float weakest_dps = 1000000.0f;

    for (int i = 0; i < ENEMY_COUNT; i++)
    {
        const NormalEnemy *e = &normal_enemies[i];

        float dps =
            (float)e->damage /
            (e->windup + e->cooldown);

        if (dps < weakest_dps)
            weakest_dps = dps;
    }

    float potion_hps =
        (float)BAL_PLAYER_POTION_HEAL /
        BAL_PLAYER_POTION_COOLDOWN;

    Test_Assert(
        potion_hps < weakest_dps * 0.40f,
        "Potion healing per second stays below 40% of the weakest enemy's damage");

    Test_Assert(BAL_PLAYER_POTION_COOLDOWN >= 1.0f,
                "Potions cannot be chugged back to back");

    Test_Assert(BAL_INVENTORY_MAX_STACK >= 1,
                "At least one potion can be carried");

    Test_Assert(
        BAL_ENEMY_POTION_DROP_CHANCE > 0.0f &&
            BAL_ENEMY_POTION_DROP_CHANCE < 1.0f,
        "Normal enemies drop potions only sometimes");

    Test_Assert(BAL_BOSS_POTION_DROPS >= 1,
                "The boss drops at least one potion");

    Test_Assert(
        BAL_POTION_DROP_BONUS_NO_POTIONS > 0.0f &&
            BAL_POTION_DROP_BONUS_LOW_HEALTH > 0.0f,
        "The potion mercy bonuses help the player");

    Test_Assert(
        BAL_ENEMY_POTION_DROP_CHANCE +
                BAL_POTION_DROP_BONUS_NO_POTIONS +
                BAL_POTION_DROP_BONUS_LOW_HEALTH <
            1.0f,
        "Even with every bonus, a potion drop is never guaranteed");
}

int main(void)
{
    printf("\n");
    printf("========================================\n");
    printf(" DungeonForge - Balance Tests\n");
    printf("========================================\n\n");

    Test_Fairness();

    Test_Geometry();

    Test_HitsToKill();

    Test_OneOnOne();

    Test_PlayerAttack();

    Test_Gear();

    Test_BossKeepDistance();

    Test_Healing();

    printf("\n========================================\n");
    printf(
        "Tests: %d | Passed: %d | Failed: %d\n",
        tests_run,
        tests_passed,
        tests_run - tests_passed);
    printf("========================================\n\n");

    return tests_run == tests_passed ? 0 : 1;
}