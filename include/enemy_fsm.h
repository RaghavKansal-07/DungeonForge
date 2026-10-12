#ifndef ENEMY_FSM_H
#define ENEMY_FSM_H

#include <stdbool.h>

#include "enemy.h"

void EnemyFSM_Update(int index);

void EnemyFSM_Reset(void);

/*
 * Where an enemy is currently steering because of its
 * adaptive decision (prediction point, flank point, ...).
 *
 * Returns false when the enemy has no adaptive target.
 * Used by the renderer so the on-screen cue always matches
 * the real behavior.
 */
bool EnemyFSM_GetAdaptiveCue(
    Enemy *enemy,
    float *target_x,
    float *target_y);

/*
 * Is this enemy winding up an attack?
 *
 * Returns true while a wind-up is running, and writes how far
 * along it is (0.0 to 1.0) into progress.
 */
bool EnemyFSM_GetWindup(
    int index,
    float *progress);

/*
 * Radius around the enemy that its current attack will hit.
 * Used to draw the warning, so the warning is the real zone.
 */
float EnemyFSM_GetWindupRadius(
    const Enemy *enemy);

#endif