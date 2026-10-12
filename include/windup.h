#ifndef WINDUP_H
#define WINDUP_H

#include <stdbool.h>

/*
 * A wind-up timer for telegraphed attacks.
 *
 * An enemy starts a wind-up, the game shows a warning while it
 * runs, and the attack lands on the single update where the
 * timer completes. Cancelling it (for example when the enemy
 * is hit) stops the attack from ever landing.
 *
 * No dependencies on the rest of the game.
 */
typedef struct
{
    float remaining;
    float duration;

} Windup;

/* Make the wind-up inactive. */
void Windup_Reset(Windup *windup);

/* Begin a wind-up. A duration of zero or less completes on the next update. */
void Windup_Start(Windup *windup, float duration);

/* Stop a running wind-up without completing it. */
void Windup_Cancel(Windup *windup);

bool Windup_IsActive(const Windup *windup);

/*
 * Advance the timer.
 *
 * Returns true on the one update where the wind-up completes,
 * and false on every other update.
 */
bool Windup_Update(Windup *windup, float dt);

/* 0.0 when inactive, otherwise how far along the wind-up is (0.0 to 1.0). */
float Windup_GetProgress(const Windup *windup);

#endif