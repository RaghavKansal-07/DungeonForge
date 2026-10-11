#include "ui.h"

#include <stddef.h>
#include <stdbool.h>

/*
 * Pixel fonts are only sharp at their native sizes, so the
 * font is loaded at many sizes and text is always drawn at
 * exactly a loaded size (never resampled).
 */
static const int ui_font_sizes[] =
    {
        11, 12, 13, 14, 15, 16, 18, 20, 22, 24, 28, 32, 40, 48};

#define UI_FONT_COUNT \
    ((int)(sizeof(ui_font_sizes) / sizeof(ui_font_sizes[0])))

/*
 * Requested sizes are enlarged a little, then snapped to the
 * nearest loaded size. Never go below the minimum.
 */
#define UI_FONT_SCALE 1.2f
#define UI_MIN_TEXT_SIZE 12.0f

static const char *ui_font_candidates[] =
    {
        "assets/fonts/PixelifySans-Regular.ttf",
        "assets/fonts/PixelifySans-VariableFont_wght.ttf",
        "assets/fonts/PixelifySans[wght].ttf",
        "assets/fonts/PixelifySans.ttf"};

static Font ui_fonts[UI_FONT_COUNT];
static bool ui_font_ok[UI_FONT_COUNT];
static bool ui_ready = false;

void UI_Init(void)
{
    const char *path = NULL;

    for (size_t i = 0;
         i < sizeof(ui_font_candidates) /
                 sizeof(ui_font_candidates[0]);
         i++)
    {
        if (FileExists(ui_font_candidates[i]))
        {
            path = ui_font_candidates[i];
            break;
        }
    }

    if (path == NULL)
    {
        TraceLog(
            LOG_WARNING,
            "UI font not found in assets/fonts, using default font");

        return;
    }

    int loaded = 0;

    for (int i = 0; i < UI_FONT_COUNT; i++)
    {
        ui_fonts[i] =
            LoadFontEx(
                path,
                ui_font_sizes[i],
                NULL,
                0);

        ui_font_ok[i] =
            IsFontValid(ui_fonts[i]);

        if (ui_font_ok[i])
        {
            /* Drawn 1:1, so point sampling keeps edges crisp. */
            SetTextureFilter(
                ui_fonts[i].texture,
                TEXTURE_FILTER_POINT);

            loaded++;
        }
    }

    if (loaded == 0)
    {
        TraceLog(
            LOG_WARNING,
            "UI font failed to load, using default font");

        return;
    }

    ui_ready = true;

    TraceLog(
        LOG_INFO,
        "UI font loaded at %d sizes: %s",
        loaded,
        path);
}

void UI_Shutdown(void)
{
    if (!ui_ready)
        return;

    for (int i = 0; i < UI_FONT_COUNT; i++)
    {
        if (ui_font_ok[i])
        {
            UnloadFont(ui_fonts[i]);
            ui_font_ok[i] = false;
        }
    }

    ui_ready = false;
}

/*
 * Index of the loaded size closest to the wanted size.
 */
static int UI_PickIndex(int size)
{
    float wanted = (float)size * UI_FONT_SCALE;

    if (wanted < UI_MIN_TEXT_SIZE)
        wanted = UI_MIN_TEXT_SIZE;

    int best = -1;
    float best_difference = 0.0f;

    for (int i = 0; i < UI_FONT_COUNT; i++)
    {
        if (!ui_font_ok[i])
            continue;

        float difference =
            (float)ui_font_sizes[i] - wanted;

        if (difference < 0.0f)
            difference = -difference;

        if (best < 0 || difference < best_difference)
        {
            best = i;
            best_difference = difference;
        }
    }

    return best;
}

/*
 * Small pixel text needs a gap between letters, otherwise
 * words run together and digits blur into each other.
 */
static float UI_Spacing(int index)
{
    return ui_font_sizes[index] < 16
               ? 1.0f
               : 0.0f;
}

void UI_Text(
    const char *text,
    int x,
    int y,
    int size,
    Color color)
{
    if (!ui_ready)
    {
        DrawText(text, x, y, size, color);
        return;
    }

    int index = UI_PickIndex(size);

    DrawTextEx(
        ui_fonts[index],
        text,
        (Vector2){(float)x, (float)y},
        (float)ui_font_sizes[index],
        UI_Spacing(index),
        color);
}

int UI_TextWidth(
    const char *text,
    int size)
{
    if (!ui_ready)
        return MeasureText(text, size);

    int index = UI_PickIndex(size);

    return (int)MeasureTextEx(
               ui_fonts[index],
               text,
               (float)ui_font_sizes[index],
               UI_Spacing(index))
        .x;
}

void UI_TextShadow(
    const char *text,
    int x,
    int y,
    int size,
    Color color)
{
    UI_Text(text, x + 1, y + 1, size, Fade(BLACK, 0.85f));
    UI_Text(text, x, y, size, color);
}

void UI_Number(
    const char *text,
    int x,
    int y,
    int size,
    Color color)
{
    DrawText(text, x, y, size, color);
}


int UI_NumberWidth(
    const char *text,
    int size)
{
    return MeasureText(text, size);
}