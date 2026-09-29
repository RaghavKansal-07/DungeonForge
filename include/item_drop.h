#ifndef ITEM_DROP_H
#define ITEM_DROP_H

#include <stdbool.h>

#include "items.h"


#define ITEM_DROP_MAX 32


typedef struct
{
    bool active;

    float x;
    float y;

    ItemId item_id;

    int quantity;

} ItemDrop;


/*
 * Initialize the item drop system.
 */
void ItemDrop_Init(void);


/*
 * Create an item drop at a world position.
 *
 * Returns true if the item was spawned.
 */
bool ItemDrop_Spawn(
    float x,
    float y,
    ItemId item_id,
    int quantity
);


/*
 * Update item pickup logic.
 */
void ItemDrop_Update(void);


/*
 * Render all active item drops.
 */
void ItemDrop_Render(void);


/*
 * Get the number of active item drops.
 */
int ItemDrop_GetCount(void);


/*
 * Get an item drop by index.
 */
ItemDrop *ItemDrop_Get(
    int index
);

#endif