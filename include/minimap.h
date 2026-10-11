#ifndef MINIMAP_H
#define MINIMAP_H

/*
 * Corner minimap with fog of war.
 *
 * Tiles are revealed as the player sees them and stay
 * revealed for the rest of the run.
 */

/* Clear the explored area and reveal around the player. */
void Minimap_Reset(void);

/* Reveal tiles the player can currently see. */
void Minimap_Update(void);

/* TAB: show or hide the minimap. */
void Minimap_Toggle(void);

void Minimap_Render(void);

#endif