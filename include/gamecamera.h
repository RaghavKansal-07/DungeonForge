#ifndef GAMECAMERA_H
#define GAMECAMERA_H

#include "raylib.h"

/*
 * Scrolling camera that follows the player.
 *
 * The camera never shows space outside the dungeon, and a
 * world smaller than the screen is simply centered.
 * Screen shake is added on top of the follow position.
 */

/* Snap the camera onto the player (new run, load). */
void GameCamera_Reset(void);

/* Move the camera toward the player. Call once per frame. */
void GameCamera_Update(float dt);

/* Camera for BeginMode2D, including screen shake. */
Camera2D GameCamera_Get(void);

/* Convert a world position to a screen position. */
Vector2 GameCamera_WorldToScreen(float x, float y);

/* Range of tiles currently on screen (with a small margin). */
void GameCamera_GetVisibleTiles(
    int *min_x,
    int *min_y,
    int *max_x,
    int *max_y);

/* Convert a screen position (for example the mouse) to a world position. */
Vector2 GameCamera_ScreenToWorld(Vector2 screen);

#endif