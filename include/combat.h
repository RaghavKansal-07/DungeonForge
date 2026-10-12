#ifndef COMBAT_H
#define COMBAT_H

#include "balance.h"

/*
 * Player melee attack settings.
 *
 * The same values drive hit detection (combat.c) and the
 * swing visual (player.c), so they cannot drift apart.
 */
#define PLAYER_ATTACK_RANGE 55.0f
#define PLAYER_ATTACK_DAMAGE BAL_PLAYER_ATTACK_DAMAGE
#define PLAYER_ATTACK_HALF_ANGLE_DEGREES 55.0f

/*
 * An attack counts as "against an enemy" for the behavior
 * profile only when an enemy is within this distance.
 */
#define COMBAT_MEASURE_RANGE 130.0f

void Combat_PlayerAttack(
    float direction_x,
    float direction_y);

int Combat_GetLastHitCount(void);

/*
 * Distance in pixels from the player to the nearest living
 * enemy, or -1.0f when no enemy is within max_distance.
 */
float Combat_GetNearestEnemyDistance(
    float max_distance);

#endif