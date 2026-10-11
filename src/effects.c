#include "effects.h"
#include "ui.h"

#include <math.h>
#include <stdbool.h>

#define MAX_FLOATING_NUMBERS 32

#define NUMBER_LIFETIME 0.9f
#define NUMBER_RISE_SPEED 38.0f
#define NUMBER_TEXT_SIZE 20

#define SHAKE_MAX 10.0f
#define SHAKE_DECAY 30.0f

typedef struct
{
    bool active;

    float x;
    float y;
    float age;

    int value;
    char prefix;

    Color color;

} FloatingNumber;

static FloatingNumber numbers[MAX_FLOATING_NUMBERS];

static unsigned int spawn_counter = 0;

static float shake_amount = 0.0f;
static float shake_time = 0.0f;

void Effects_Clear(void)
{
    for (int i = 0; i < MAX_FLOATING_NUMBERS; i++)
        numbers[i].active = false;

    shake_amount = 0.0f;
    shake_time = 0.0f;
}

void Effects_Update(float dt)
{
    if (dt <= 0.0f)
        return;

    if (dt > 0.05f)
        dt = 0.05f;

    for (int i = 0; i < MAX_FLOATING_NUMBERS; i++)
    {
        if (!numbers[i].active)
            continue;

        numbers[i].age += dt;

        if (numbers[i].age >= NUMBER_LIFETIME)
            numbers[i].active = false;
    }

    shake_time += dt;

    if (shake_amount > 0.0f)
    {
        shake_amount -= SHAKE_DECAY * dt;

        if (shake_amount < 0.0f)
            shake_amount = 0.0f;
    }
}

void Effects_SpawnNumber(
    float x,
    float y,
    int value,
    char prefix,
    Color color)
{
    /*
     * Use a free slot, or replace the oldest number.
     */
    int slot = -1;
    float oldest_age = -1.0f;

    for (int i = 0; i < MAX_FLOATING_NUMBERS; i++)
    {
        if (!numbers[i].active)
        {
            slot = i;
            break;
        }

        if (numbers[i].age > oldest_age)
        {
            oldest_age = numbers[i].age;
            slot = i;
        }
    }

    if (slot < 0)
        return;

    /*
     * Small sideways offset so numbers from rapid hits
     * do not land exactly on top of each other.
     */
    spawn_counter++;

    float drift =
        (float)((int)((spawn_counter * 37u) % 21u) - 10);

    numbers[slot].active = true;
    numbers[slot].x = x + drift;
    numbers[slot].y = y;
    numbers[slot].age = 0.0f;
    numbers[slot].value = value;
    numbers[slot].prefix = prefix;
    numbers[slot].color = color;
}

void Effects_AddShake(float amount)
{
    if (amount > SHAKE_MAX)
        amount = SHAKE_MAX;

    if (amount > shake_amount)
        shake_amount = amount;
}

Vector2 Effects_GetShake(void)
{
    if (shake_amount <= 0.0f)
        return (Vector2){0.0f, 0.0f};

    /*
     * Whole pixels only, so tile edges do not shimmer.
     */
    return (Vector2){
        roundf(sinf(shake_time * 93.0f) * shake_amount),
        roundf(cosf(shake_time * 71.0f) * shake_amount)};
}

void Effects_RenderWorld(void)
{
    for (int i = 0; i < MAX_FLOATING_NUMBERS; i++)
    {
        const FloatingNumber *n = &numbers[i];

        if (!n->active)
            continue;

        float t = n->age / NUMBER_LIFETIME;

        float alpha = 1.0f;

        if (t > 0.6f)
            alpha = 1.0f - (t - 0.6f) / 0.4f;

        if (alpha < 0.0f)
            alpha = 0.0f;

        const char *text =
            n->prefix != '\0'
                ? TextFormat("%c%d", n->prefix, n->value)
                : TextFormat("%d", n->value);

        int width =
            UI_NumberWidth(text, NUMBER_TEXT_SIZE);

        int x = (int)n->x - width / 2;
        int y = (int)(n->y - n->age * NUMBER_RISE_SPEED);

        UI_Number(
            text,
            x + 1,
            y + 1,
            NUMBER_TEXT_SIZE,
            Fade(BLACK, alpha * 0.9f));

        UI_Number(
            text,
            x,
            y,
            NUMBER_TEXT_SIZE,
            Fade(n->color, alpha));
    }
}