#include "game.h"
#include "ui.h"
#include "rng.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    /* Parse optional seed from command line */
    uint64_t seed = 0;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 's' && i + 1 < argc) {
            seed = (uint64_t)atoll(argv[++i]);
        }
    }

    /* Initialize RNG */
    rng_seed(seed);

    /* Initialize ncurses UI */
    if (!ui_init()) {
        fprintf(stderr, "Error: terminal does not support colors.\n");
        return 1;
    }

    /* Initialize and run game */
    GameState gs;
    game_init(&gs);
    game_run(&gs);

    /* Shutdown */
    ui_shutdown();

    printf("Thanks for playing Knights of Camelot!\n");
    printf("Game seed: %llu\n", (unsigned long long)rng_get_seed());
    return 0;
}
