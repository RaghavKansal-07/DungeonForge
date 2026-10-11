#include "rng.h"

#include <stddef.h>

#define RNG_FALLBACK_STATE 0x6D2B79F5u

void Rng_Seed(Rng *rng, uint32_t seed)
{
    if (rng == NULL)
        return;

    /*
     * Mix the seed so that small, similar seeds
     * (1, 2, 3...) give very different sequences.
     */
    uint32_t state =
        seed * 2654435761u + 0x9E3779B9u;

    state ^= state >> 15;

    /* xorshift must never start at zero. */
    if (state == 0)
        state = RNG_FALLBACK_STATE;

    rng->state = state;
}


uint32_t Rng_Next(Rng *rng)
{
    if (rng == NULL)
        return 0;

    uint32_t x = rng->state;

    if (x == 0)
        x = RNG_FALLBACK_STATE;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;

    rng->state = x;

    return x;
}


int Rng_Range(Rng *rng, int min, int max)
{
    if (max <= min)
        return min;

    uint32_t span =
        (uint32_t)(max - min) + 1u;

    return min +
           (int)(Rng_Next(rng) % span);
}


float Rng_Float(Rng *rng)
{
    /* 24 random bits give an exact float in [0, 1]. */
    return (float)(Rng_Next(rng) >> 8) /
           16777215.0f;
}