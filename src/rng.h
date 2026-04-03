#ifndef RNG_H
#define RNG_H

#include <stdbool.h>
#include <stdint.h>

/* Seed the RNG. If seed is 0, uses current time. */
void     rng_seed(uint64_t seed);

/* Get the current seed (for saving / morgue file). */
uint64_t rng_get_seed(void);

/* Return a random uint64. */
uint64_t rng_next(void);

/* Return a random int in [min, max] inclusive. */
int      rng_range(int min, int max);

/* Return a random float in [0.0, 1.0). */
double   rng_float(void);

/* Return true with probability pct/100. */
bool     rng_chance(int pct);

#endif /* RNG_H */
