#ifndef UI_H
#define UI_H

#include "raylib.h"

/*
 * Shared text rendering for the whole game.
 *
 * Loads one custom font and falls back to raylib's
 * default font if the file is missing.
 */
void UI_Init(void);
void UI_Shutdown(void);

/* Same arguments as raylib's DrawText / MeasureText. */
void UI_Text(const char *text, int x, int y, int size, Color color);
int UI_TextWidth(const char *text, int size);

/* Text with a dark drop shadow, for labels over the map. */
void UI_TextShadow(const char *text, int x, int y, int size, Color color);

/*
 * Numbers use raylib's default font. Pixelify Sans draws
 * a 5 like an S at small sizes, so HP and counters stay
 * readable this way.
 */
void UI_Number(const char *text, int x, int y, int size, Color color);
int UI_NumberWidth(const char *text, int size);

#endif