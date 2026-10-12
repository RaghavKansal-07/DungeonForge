#include <stdio.h>
#include <stdbool.h>

#include "inventory.h"

#define POTION 1
#define SWORD 2

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

static void Test_Basics(void)
{
    Inventory inventory;

    Inventory_Init(&inventory);

    Test_Assert(
        inventory.item_count == 0 &&
            !Inventory_IsFull(&inventory),
        "A new inventory is empty and not full");

    Test_Assert(
        Inventory_AddItem(&inventory, ITEM_HEALTH_POTION, POTION, 1) &&
            Inventory_GetQuantity(&inventory, POTION) == 1 &&
            inventory.item_count == 1,
        "Adding an item uses one slot");

    Test_Assert(
        Inventory_FindItem(&inventory, POTION) >= 0 &&
            Inventory_FindItem(&inventory, 99) == -1,
        "Finding works for present and missing items");

    Test_Assert(
        Inventory_GetQuantity(&inventory, 99) == 0,
        "A missing item has quantity 0");
}

static void Test_Stacking(void)
{
    Inventory inventory;

    Inventory_Init(&inventory);

    Inventory_AddItem(&inventory, ITEM_HEALTH_POTION, POTION, 1);
    Inventory_AddItem(&inventory, ITEM_HEALTH_POTION, POTION, 1);

    Test_Assert(
        Inventory_GetQuantity(&inventory, POTION) == 2 &&
            inventory.item_count == 1,
        "The same item stacks into one slot");

    while (Inventory_AddItem(
        &inventory, ITEM_HEALTH_POTION, POTION, 1))
    {
    }

    Test_Assert(
        Inventory_GetQuantity(&inventory, POTION) ==
            INVENTORY_MAX_STACK,
        "A stack stops growing at the stack limit");

    Test_Assert(
        !Inventory_AddItem(&inventory, ITEM_HEALTH_POTION, POTION, 1),
        "Adding to a full stack is refused");

    Inventory_RemoveItem(&inventory, POTION, 2);

    Test_Assert(
        !Inventory_AddItem(
            &inventory, ITEM_HEALTH_POTION, POTION, 3) &&
            Inventory_GetQuantity(&inventory, POTION) ==
                INVENTORY_MAX_STACK - 2,
        "An add that does not fully fit adds nothing");
}

static void Test_Removal(void)
{
    Inventory inventory;

    Inventory_Init(&inventory);

    Inventory_AddItem(&inventory, ITEM_HEALTH_POTION, POTION, 3);

    Test_Assert(
        Inventory_RemoveItem(&inventory, POTION, 1) &&
            Inventory_GetQuantity(&inventory, POTION) == 2,
        "Removing some items lowers the quantity");

    Test_Assert(
        !Inventory_RemoveItem(&inventory, POTION, 5) &&
            Inventory_GetQuantity(&inventory, POTION) == 2,
        "Removing more than owned is refused and changes nothing");

    Test_Assert(
        Inventory_RemoveItem(&inventory, POTION, 2) &&
            inventory.item_count == 0 &&
            Inventory_FindItem(&inventory, POTION) == -1,
        "Removing everything frees the slot");

    Test_Assert(
        !Inventory_RemoveItem(&inventory, POTION, 1),
        "Removing an item that is not there is refused");
}

static void Test_Slots(void)
{
    Inventory inventory;

    Inventory_Init(&inventory);

    Inventory_AddItem(&inventory, ITEM_HEALTH_POTION, POTION, 2);
    Inventory_AddItem(&inventory, ITEM_WEAPON, SWORD, 1);

    Test_Assert(
        inventory.item_count == 2,
        "Different items use different slots");

    Inventory_Init(&inventory);

    for (int id = 1; id <= INVENTORY_MAX_ITEMS; id++)
        Inventory_AddItem(&inventory, ITEM_WEAPON, id, 1);

    Test_Assert(
        Inventory_IsFull(&inventory) &&
            inventory.item_count == INVENTORY_MAX_ITEMS,
        "Filling every slot makes the inventory full");

    Test_Assert(
        !Inventory_AddItem(
            &inventory, ITEM_WEAPON, INVENTORY_MAX_ITEMS + 1, 1),
        "A new item cannot be added to a full inventory");

    /* Weapons and armor never stack. */
    Test_Assert(
        !Inventory_AddItem(&inventory, ITEM_WEAPON, 1, 1) &&
            Inventory_GetQuantity(&inventory, 1) == 1,
        "A second copy of a weapon is refused");

    Inventory_Init(&inventory);

    for (int id = 1; id < INVENTORY_MAX_ITEMS; id++)
        Inventory_AddItem(&inventory, ITEM_WEAPON, id + 10, 1);

    Inventory_AddItem(&inventory, ITEM_HEALTH_POTION, POTION, 1);

    Test_Assert(
        Inventory_IsFull(&inventory) &&
            Inventory_AddItem(&inventory, ITEM_HEALTH_POTION, POTION, 1) &&
            Inventory_GetQuantity(&inventory, POTION) == 2,
        "Potions can still stack when the inventory is full");
}

static void Test_GearDoesNotStack(void)
{
    Inventory inventory;

    Inventory_Init(&inventory);

    Test_Assert(
        Inventory_AddItem(&inventory, ITEM_WEAPON, SWORD, 1) &&
            !Inventory_AddItem(&inventory, ITEM_WEAPON, SWORD, 1) &&
            Inventory_GetQuantity(&inventory, SWORD) == 1,
        "Only one sword can be carried");

    Test_Assert(
        !Inventory_AddItem(&inventory, ITEM_ARMOR, 3, 2) &&
            Inventory_GetQuantity(&inventory, 3) == 0,
        "Two pieces of armor cannot be added at once");

    Test_Assert(
        Inventory_AddItem(&inventory, ITEM_ARMOR, 3, 1) &&
            Inventory_GetQuantity(&inventory, 3) == 1,
        "One piece of armor can be added");
}

static void Test_InvalidInput(void)
{
    Inventory inventory;

    Inventory_Init(&inventory);

    Test_Assert(
        !Inventory_AddItem(NULL, ITEM_WEAPON, 1, 1) &&
            !Inventory_RemoveItem(NULL, 1, 1) &&
            Inventory_FindItem(NULL, 1) == -1 &&
            Inventory_GetQuantity(NULL, 1) == 0 &&
            Inventory_IsFull(NULL),
        "Every function is safe with a NULL inventory");

    Test_Assert(
        !Inventory_AddItem(&inventory, ITEM_NONE, 1, 1),
        "An item of type NONE is refused");

    Test_Assert(
        !Inventory_AddItem(&inventory, ITEM_WEAPON, 0, 1) &&
            !Inventory_AddItem(&inventory, ITEM_WEAPON, -3, 1),
        "An item ID of zero or less is refused");

    Test_Assert(
        !Inventory_AddItem(&inventory, ITEM_WEAPON, 1, 0) &&
            !Inventory_AddItem(&inventory, ITEM_WEAPON, 1, -2),
        "A quantity of zero or less is refused");

    Test_Assert(
        inventory.item_count == 0,
        "Refused adds leave the inventory empty");
}

int main(void)
{
    printf("\n");
    printf("========================================\n");
    printf(" DungeonForge - Inventory Tests\n");
    printf("========================================\n\n");

    Test_Basics();

    Test_Stacking();

    Test_Removal();

    Test_Slots();

    Test_GearDoesNotStack();

    Test_InvalidInput();

    printf("\n========================================\n");
    printf(
        "Tests: %d | Passed: %d | Failed: %d\n",
        tests_run,
        tests_passed,
        tests_run - tests_passed);
    printf("========================================\n\n");

    return tests_run == tests_passed ? 0 : 1;
}