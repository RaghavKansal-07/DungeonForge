#include "inventory.h"


void Inventory_Init(
    Inventory *inventory
)
{
    if (inventory == NULL)
        return;

    inventory->item_count = 0;

    for (int i = 0;
         i < INVENTORY_MAX_ITEMS;
         i++)
    {
        inventory->items[i].type =
            ITEM_NONE;

        inventory->items[i].item_id =
            0;

        inventory->items[i].quantity =
            0;
    }
}


int Inventory_FindItem(
    const Inventory *inventory,
    int item_id
)
{
    if (inventory == NULL)
        return -1;

    for (int i = 0;
         i < INVENTORY_MAX_ITEMS;
         i++)
    {
        if (inventory->items[i].type ==
                ITEM_NONE)
        {
            continue;
        }

        if (inventory->items[i].item_id ==
            item_id)
        {
            return i;
        }
    }

    return -1;
}


bool Inventory_IsFull(
    const Inventory *inventory
)
{
    if (inventory == NULL)
        return true;

    return inventory->item_count >=
           INVENTORY_MAX_ITEMS;
}


bool Inventory_AddItem(
    Inventory *inventory,
    ItemType type,
    int item_id,
    int quantity
)
{
    if (inventory == NULL)
        return false;

    if (type == ITEM_NONE)
        return false;

    if (item_id <= 0)
        return false;

    if (quantity <= 0)
        return false;

    /*
     * If the item already exists,
     * increase its quantity.
     */
    int existing_index =
        Inventory_FindItem(
            inventory,
            item_id
        );

    if (existing_index >= 0)
    {
        inventory->items[
            existing_index
        ].quantity += quantity;

        return true;
    }

    /*
     * A new item requires a free slot.
     */
    if (Inventory_IsFull(inventory))
        return false;

    for (int i = 0;
         i < INVENTORY_MAX_ITEMS;
         i++)
    {
        if (inventory->items[i].type ==
            ITEM_NONE)
        {
            inventory->items[i].type =
                type;

            inventory->items[i].item_id =
                item_id;

            inventory->items[i].quantity =
                quantity;

            inventory->item_count++;

            return true;
        }
    }

    return false;
}


bool Inventory_RemoveItem(
    Inventory *inventory,
    int item_id,
    int quantity
)
{
    if (inventory == NULL)
        return false;

    if (item_id <= 0)
        return false;

    if (quantity <= 0)
        return false;

    int index =
        Inventory_FindItem(
            inventory,
            item_id
        );

    if (index < 0)
        return false;

    if (inventory->items[index].quantity <
        quantity)
    {
        return false;
    }

    inventory->items[index].quantity -=
        quantity;

    /*
     * Completely remove the slot when
     * its quantity reaches zero.
     */
    if (inventory->items[index].quantity ==
        0)
    {
        inventory->items[index].type =
            ITEM_NONE;

        inventory->items[index].item_id =
            0;

        inventory->items[index].quantity =
            0;

        inventory->item_count--;
    }

    return true;
}


int Inventory_GetQuantity(
    const Inventory *inventory,
    int item_id
)
{
    if (inventory == NULL)
        return 0;

    int index =
        Inventory_FindItem(
            inventory,
            item_id
        );

    if (index < 0)
        return 0;

    return inventory->items[index].quantity;
}