#ifndef INVENTORY_H
#define INVENTORY_H

#include <stdbool.h>
#include <stddef.h>

#define INVENTORY_MAX_ITEMS 12


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

    int item_count;

} Inventory;


/*
 * Initialize an inventory.
 */
void Inventory_Init(
    Inventory *inventory
);


/*
 * Add an item to the inventory.
 *
 * Returns true if the item was added
 * successfully.
 */
bool Inventory_AddItem(
    Inventory *inventory,
    ItemType type,
    int item_id,
    int quantity
);


/*
 * Remove a quantity of an item.
 *
 * Returns true if the requested quantity
 * was successfully removed.
 */
bool Inventory_RemoveItem(
    Inventory *inventory,
    int item_id,
    int quantity
);


/*
 * Find an item by its ID.
 *
 * Returns the item index, or -1 if the
 * item does not exist.
 */
int Inventory_FindItem(
    const Inventory *inventory,
    int item_id
);


/*
 * Get the current quantity of an item.
 */
int Inventory_GetQuantity(
    const Inventory *inventory,
    int item_id
);


/*
 * Check whether the inventory is full.
 */
bool Inventory_IsFull(
    const Inventory *inventory
);

#endif