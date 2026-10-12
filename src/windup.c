#include "windup.h"

#include <stddef.h>

#define WINDUP_MIN_DURATION 0.0001f

void Windup_Reset(Windup *windup)
{
    if (windup == NULL)
        return;

    windup->remaining = 0.0f;
    windup->duration = 0.0f;
}

void Windup_Start(
    Windup *windup,
    float duration)
{
    if (windup == NULL)
        return;

    if (!(duration > WINDUP_MIN_DURATION))
        duration = WINDUP_MIN_DURATION;

    windup->duration = duration;
    windup->remaining = duration;
}

void Windup_Cancel(Windup *windup)
{
    if (windup == NULL)
        return;

    windup->remaining = 0.0f;
}

bool Windup_IsActive(const Windup *windup)
{
    return windup != NULL &&
           windup->remaining > 0.0f;
}

bool Windup_Update(
    Windup *windup,
    float dt)
{
    if (!Windup_IsActive(windup))
        return false;

    if (!(dt > 0.0f))
        return false;

    windup->remaining -= dt;

    if (windup->remaining <= 0.0f)
    {
        windup->remaining = 0.0f;

        return true;
    }

    return false;
}

float Windup_GetProgress(const Windup *windup)
{
    if (!Windup_IsActive(windup) ||
        !(windup->duration > 0.0f))
    {
        return 0.0f;
    }

    float progress =
        1.0f -
        windup->remaining /
            windup->duration;

    if (progress < 0.0f)
        progress = 0.0f;

    if (progress > 1.0f)
        progress = 1.0f;

    return progress;
}