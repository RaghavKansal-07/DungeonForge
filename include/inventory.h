#ifndef INVENTORY_H
#define INVENTORY_H

#include <stdbool.h>
#include <stddef.h>

#include "balance.h"

#define INVENTORY_MAX_ITEMS 12

/*
 * The most potions a single slot can hold. Weapons and armor
 * are limited to one each.
 */
#define INVENTORY_MAX_STACK BAL_INVENTORY_MAX_STACK

typedef enum
{
    ITEM_NONE,
    ITEM_HEALTH_POTION,
    ITEM_WEAPON,
    ITEM_ARMOR

} ItemType;

typedef struct
{
    ItemType type;

    int item_id;

    int quantity;

} InventoryItem;

typedef struct
{
    InventoryItem items[INVENTORY_MAX_ITEMS];

    /* Number of slots in use (not the number of items). */
    int item_count;

} Inventory;

/*
 * Initialize an inventory.
 */
void Inventory_Init(
    Inventory *inventory);

/*
 * Add an item to the inventory.
 *
 * Items with the same ID stack in one slot, up to
 * INVENTORY_MAX_STACK. The add is all-or-nothing: if the whole
 * quantity does not fit, nothing is added.
 *
 * Returns true if the item was added successfully.
 */
bool Inventory_AddItem(
    Inventory *inventory,
    ItemType type,
    int item_id,
    int quantity);

/*
 * Remove a quantity of an item.
 *
 * Returns true if the requested quantity
 * was successfully removed.
 */
bool Inventory_RemoveItem(
    Inventory *inventory,
    int item_id,
    int quantity);

/*
 * Find an item by its ID.
 *
 * Returns the item index, or -1 if the
 * item does not exist.
 */
int Inventory_FindItem(
    const Inventory *inventory,
    int item_id);

/*
 * Get the current quantity of an item.
 */
int Inventory_GetQuantity(
    const Inventory *inventory,
    int item_id);

/*
 * Check whether every slot is in use.
 */
bool Inventory_IsFull(
    const Inventory *inventory);

#endif