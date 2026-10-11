#ifndef EFFECTS_H
#define EFFECTS_H

#include "raylib.h"

/*
 * Short-lived visual feedback: floating numbers and
 * screen shake. Purely visual, no gameplay effect.
 */

void Effects_Clear(void);

void Effects_Update(float dt);

/*
 * Show a number that floats upward and fades out.
 *
 * prefix is '+' or '-' (or '\0' for none).
 * y is the top of the text where it starts.
 */
void Effects_SpawnNumber(
    float x,
    float y,
    int value,
    char prefix,
    Color color);

/* Add screen shake. The strongest recent shake wins. */
void Effects_AddShake(float amount);

/* Camera offset for this frame, in pixels. */
Vector2 Effects_GetShake(void);

/* Draw floating numbers (call inside the world camera). */
void Effects_RenderWorld(void);

#endif