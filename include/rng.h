#ifndef RNG_H
#define RNG_H

#include <stdint.h>

/*
 * Small, self-contained pseudo-random generator (xorshift32).
 *
 * Each module owns its own Rng, so one system reseeding
 * its generator can never disturb another.
 */
typedef struct
{
    uint32_t state;

} Rng;

/* Seed the generator. Any seed (including 0) is valid. */
void Rng_Seed(Rng *rng, uint32_t seed);

/* Next raw 32-bit value. */
uint32_t Rng_Next(Rng *rng);

/* Integer in [min, max] (inclusive). */
int Rng_Range(Rng *rng, int min, int max);

/* Float in [0.0, 1.0]. */
float Rng_Float(Rng *rng);

#endif