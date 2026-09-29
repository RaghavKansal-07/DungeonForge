#include "items.h"

#include <stddef.h>
#include <stdbool.h>


static const ItemDefinition item_definitions[] =
{
    {
        ITEM_ID_HEALTH_POTION,
        ITEM_HEALTH_POTION,
        "Health Potion",
        "Restores player health.",
        25
    },

    {
        ITEM_ID_IRON_SWORD,
        ITEM_WEAPON,
        "Iron Sword",
        "A basic melee weapon.",
        50
    },

    {
        ITEM_ID_IRON_ARMOR,
        ITEM_ARMOR,
        "Iron Armor",
        "Basic protective armor.",
        75
    }
};


#define ITEM_DEFINITION_COUNT \
    (sizeof(item_definitions) / sizeof(item_definitions[0]))


const ItemDefinition *Items_GetDefinition(
    ItemId item_id
)
{
    for (size_t i = 0;
         i < ITEM_DEFINITION_COUNT;
         i++)
    {
        if (item_definitions[i].id ==
            item_id)
        {
            return &item_definitions[i];
        }
    }

    return NULL;
}


bool Items_IsValid(
    ItemId item_id
)
{
    return Items_GetDefinition(item_id) != NULL;
}