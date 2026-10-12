#include "equipment.h"

#include "balance.h"
#include "items.h"

#include <stddef.h>

bool Equipment_HasSword(
    const Inventory *inventory)
{
    return inventory != NULL &&
           Inventory_GetQuantity(
               inventory,
               ITEM_ID_IRON_SWORD) > 0;
}

bool Equipment_HasArmor(
    const Inventory *inventory)
{
    return inventory != NULL &&
           Inventory_GetQuantity(
               inventory,
               ITEM_ID_IRON_ARMOR) > 0;
}

int Equipment_GetAttackDamage(
    const Inventory *inventory,
    int base_damage)
{
    if (Equipment_HasSword(inventory))
        return base_damage + BAL_SWORD_DAMAGE_BONUS;

    return base_damage;
}

int Equipment_ReduceDamage(
    const Inventory *inventory,
    int incoming_damage)
{
    if (incoming_damage <= 0 ||
        !Equipment_HasArmor(inventory))
    {
        return incoming_damage;
    }

    /*
     * Round up, so armor never turns a hit into 0 damage.
     */
    int reduced =
        (incoming_damage *
             (100 - BAL_ARMOR_REDUCTION_PERCENT) +
         99) /
        100;

    if (reduced < 1)
        reduced = 1;

    return reduced;
}