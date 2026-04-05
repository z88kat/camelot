#include "game.h"
#include "ui.h"
#include "rng.h"
#include "save.h"
#include <stdio.h>
#include <stdlib.h>

/* Title screen and menu defined in game.c */
extern int show_title_screen(void);
extern void show_high_scores_screen(void);
extern void show_fallen_heroes_screen(void);

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

    /* Ensure save directory exists */
    save_ensure_dir();

    /* Title screen loop */
    bool play = false;
    bool load = false;
    while (1) {
        int choice = show_title_screen();
        if (choice == 0) break;          /* Quit */
        if (choice == 3) { show_high_scores_screen(); continue; }
        if (choice == 4) { show_fallen_heroes_screen(); continue; }
        if (choice == 2) { load = true; play = true; break; }  /* Continue */
        if (choice == 1) { play = true; break; }                /* New Game */
    }

    if (play) {
        GameState gs;
        if (load && save_exists()) {
            /* Load saved game -- need to reinitialize heap data */
            load_game(&gs);
            entity_init();
            item_init();
            spell_init();
            gs.overworld = calloc(1, sizeof(Overworld));
            overworld_init(gs.overworld);
            overworld_spawn_creatures(gs.overworld);
            town_init();
            /* Restore player position on overworld */
            gs.running = true;
            if (gs.mode != MODE_OVERWORLD) gs.mode = MODE_OVERWORLD;
            gs.dungeon = NULL;
            log_init(&gs.log);
            log_add(&gs.log, gs.turn, CP_WHITE, "Welcome back, %s!", gs.player_name);
        } else {
            game_init(&gs);
        }
        game_run(&gs);
    }

    /* Shutdown */
    ui_shutdown();

    printf("Thanks for playing Knights of Camelot!\n");
    printf("Game seed: %llu\n", (unsigned long long)rng_get_seed());
    return 0;
}
