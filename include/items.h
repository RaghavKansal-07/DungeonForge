#ifndef ITEMS_H
#define ITEMS_H

#include "inventory.h"


/*
 * Item IDs.
 *
 * These IDs uniquely identify actual items
 * that can exist in the game.
 */
typedef enum
{
    ITEM_ID_NONE = 0,

    ITEM_ID_HEALTH_POTION = 1,

    ITEM_ID_IRON_SWORD = 2,

    ITEM_ID_IRON_ARMOR = 3

} ItemId;


/*
 * Information describing an item.
 */
typedef struct
{
    ItemId id;

    ItemType type;

    const char *name;

    const char *description;

    int value;

} ItemDefinition;


/*
 * Get the definition of an item.
 *
 * Returns NULL if the item ID is invalid.
 */
const ItemDefinition *Items_GetDefinition(
    ItemId item_id
);


/*
 * Check whether an item ID represents
 * a valid game item.
 */
bool Items_IsValid(
    ItemId item_id
);

#endif