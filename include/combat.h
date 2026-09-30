#ifndef COMBAT_H
#define COMBAT_H

/*
 * Perform the player's melee attack.
 */
void Combat_PlayerAttack(void);


/*
 * Get the number of enemies hit by the
 * most recent player attack.
 *
 * Returns:
 *     0 -> no enemy hit
 *     1+ -> number of enemies hit
 */
int Combat_GetLastHitCount(void);

#endif