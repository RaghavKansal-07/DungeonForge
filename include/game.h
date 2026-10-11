#ifndef GAME_H
#define GAME_H

#include "dungeon.h"

#include <stdbool.h>

void Game_Init(void);

void Game_Update(void);

void Game_Render(void);

void Game_Shutdown(void);


/*
 * Get the currently active dungeon.
 *
 * Used by systems such as save/load that need
 * access to the current world state.
 */
Dungeon *Game_GetDungeon(void);

/* True once the player chose Quit in the menu. */
bool Game_ShouldQuit(void);

#endif