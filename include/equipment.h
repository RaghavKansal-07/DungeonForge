#ifndef EQUIPMENT_H
#define EQUIPMENT_H

#include <stdbool.h>

#include "inventory.h"

/*
 * Gear carried in the inventory takes effect automatically:
 *
 *   Iron Sword - extra damage on every hit the player deals
 *   Iron Armor - blocks part of the damage the player takes
 *
 * No dependencies on the rest of the game.
 */

bool Equipment_HasSword(const Inventory *inventory);
bool Equipment_HasArmor(const Inventory *inventory);

/* Damage the player deals, including the sword bonus. */
int Equipment_GetAttackDamage(
    const Inventory *inventory,
    int base_damage);

/*
 * Damage the player takes after armor. Never drops below 1 for
 * a real hit, and 0 or negative damage is returned unchanged.
 */
int Equipment_ReduceDamage(
    const Inventory *inventory,
    int incoming_damage);

#endif