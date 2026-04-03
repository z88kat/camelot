#include "rng.h"
#include <stdbool.h>
#include <time.h>

/* xoshiro256** PRNG -- fast, high quality, public domain */
static uint64_t s[4];
static uint64_t initial_seed;

static uint64_t rotl(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

static uint64_t splitmix64(uint64_t *state) {
    uint64_t z = (*state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

void rng_seed(uint64_t seed) {
    if (seed == 0) {
        seed = (uint64_t)time(NULL);
    }
    initial_seed = seed;
    uint64_t sm = seed;
    s[0] = splitmix64(&sm);
    s[1] = splitmix64(&sm);
    s[2] = splitmix64(&sm);
    s[3] = splitmix64(&sm);
}

uint64_t rng_get_seed(void) {
    return initial_seed;
}

uint64_t rng_next(void) {
    const uint64_t result = rotl(s[1] * 5, 7) * 9;
    const uint64_t t = s[1] << 17;

    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];

    s[2] ^= t;
    s[3] = rotl(s[3], 45);

    return result;
}

int rng_range(int min, int max) {
    if (min >= max) return min;
    uint64_t range = (uint64_t)(max - min + 1);
    return min + (int)(rng_next() % range);
}

double rng_float(void) {
    return (double)(rng_next() >> 11) * (1.0 / 9007199254740992.0);
}

bool rng_chance(int pct) {
    return rng_range(1, 100) <= pct;
}
