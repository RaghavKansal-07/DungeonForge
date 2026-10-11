#ifndef LIGHTING_H
#define LIGHTING_H

/*
 * Dynamic lighting for the dungeon.
 *
 * The world is drawn normally, then multiplied by a
 * light map: dim ambient light plus soft glows around
 * the player, enemies, items and tinted rooms.
 */

void Lighting_Init(void);
void Lighting_Shutdown(void);

/* F2: switch the whole effect on or off. */
void Lighting_Toggle(void);

/* Call after the world is drawn, before the HUD. */
void Lighting_Render(void);

#endif