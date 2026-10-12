#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>

#include "equipment.h"
#include "items.h"
#include "balance.h"

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

static void Test_NoGear(void)
{
    Inventory inventory;

    Inventory_Init(&inventory);

    Test_Assert(
        !Equipment_HasSword(&inventory) &&
            !Equipment_HasArmor(&inventory),
        "A new inventory owns no gear");

    Test_Assert(
        Equipment_GetAttackDamage(&inventory, 20) == 20,
        "Without a sword the attack damage is unchanged");

    Test_Assert(
        Equipment_ReduceDamage(&inventory, 16) == 16,
        "Without armor the incoming damage is unchanged");
}

static void Test_Sword(void)
{
    Inventory inventory;

    Inventory_Init(&inventory);

    Inventory_AddItem(
        &inventory, ITEM_WEAPON, ITEM_ID_IRON_SWORD, 1);

    Test_Assert(
        Equipment_HasSword(&inventory) &&
            !Equipment_HasArmor(&inventory),
        "Picking up a sword is detected");

    Test_Assert(
        Equipment_GetAttackDamage(&inventory, 20) ==
            20 + BAL_SWORD_DAMAGE_BONUS,
        "The sword adds its damage bonus");

    Test_Assert(
        Equipment_ReduceDamage(&inventory, 16) == 16,
        "A sword does not reduce incoming damage");
}

static void Test_Armor(void)
{
    Inventory inventory;

    Inventory_Init(&inventory);

    Inventory_AddItem(
        &inventory, ITEM_ARMOR, ITEM_ID_IRON_ARMOR, 1);

    Test_Assert(
        Equipment_HasArmor(&inventory) &&
            !Equipment_HasSword(&inventory),
        "Picking up armor is detected");

    Test_Assert(
        Equipment_ReduceDamage(&inventory, 20) ==
            20 * (100 - BAL_ARMOR_REDUCTION_PERCENT) / 100,
        "Armor blocks the configured share of damage");

    Test_Assert(
        Equipment_ReduceDamage(&inventory, 18) == 14,
        "Reduced damage rounds up (18 becomes 14)");

    Test_Assert(
        Equipment_ReduceDamage(&inventory, 1) == 1,
        "Armor never turns a hit into 0 damage");

    Test_Assert(
        Equipment_ReduceDamage(&inventory, 0) == 0 &&
            Equipment_ReduceDamage(&inventory, -5) == -5,
        "Zero and negative damage pass through unchanged");

    Test_Assert(
        Equipment_GetAttackDamage(&inventory, 20) == 20,
        "Armor does not change the damage the player deals");
}

static void Test_BothAndRemoval(void)
{
    Inventory inventory;

    Inventory_Init(&inventory);

    Inventory_AddItem(
        &inventory, ITEM_WEAPON, ITEM_ID_IRON_SWORD, 1);

    Inventory_AddItem(
        &inventory, ITEM_ARMOR, ITEM_ID_IRON_ARMOR, 1);

    Test_Assert(
        Equipment_HasSword(&inventory) &&
            Equipment_HasArmor(&inventory),
        "Sword and armor can be owned together");

    Inventory_RemoveItem(&inventory, ITEM_ID_IRON_SWORD, 1);

    Test_Assert(
        !Equipment_HasSword(&inventory) &&
            Equipment_GetAttackDamage(&inventory, 20) == 20,
        "Losing the sword removes its bonus");

    Test_Assert(
        !Inventory_AddItem(
            &inventory, ITEM_ARMOR, ITEM_ID_IRON_ARMOR, 1),
        "A second set of armor is not accepted");
}

static void Test_NullSafety(void)
{
    Test_Assert(
        !Equipment_HasSword(NULL) &&
            !Equipment_HasArmor(NULL) &&
            Equipment_GetAttackDamage(NULL, 20) == 20 &&
            Equipment_ReduceDamage(NULL, 16) == 16,
        "Every function is safe with a NULL inventory");
}

int main(void)
{
    printf("\n");
    printf("========================================\n");
    printf(" DungeonForge - Equipment Tests\n");
    printf("========================================\n\n");

    Test_NoGear();

    Test_Sword();

    Test_Armor();

    Test_BothAndRemoval();

    Test_NullSafety();

    printf("\n========================================\n");
    printf(
        "Tests: %d | Passed: %d | Failed: %d\n",
        tests_run,
        tests_passed,
        tests_run - tests_passed);
    printf("========================================\n\n");

    return tests_run == tests_passed ? 0 : 1;
}