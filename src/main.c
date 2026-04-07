#include "game.h"
#include "ui.h"
#include "rng.h"
#include "save.h"
#include "loot.h"
#include "ammo.h"
#include <stdio.h>
#include <stdlib.h>

/* Title screen and menu defined in game.c */
extern int show_title_screen(void);
extern void show_high_scores_screen(void);
extern void show_fallen_heroes_screen(void);

int main(int argc, char *argv[]) {
    /* Parse command line flags */
    uint64_t seed = 0;
    bool debug_mode = false;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 's' && i + 1 < argc) {
            seed = (uint64_t)atoll(argv[++i]);
        }
        if (argv[i][0] == '-' && argv[i][1] == 'd') {
            debug_mode = true;
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
            bool load_ok = load_game(&gs);
            if (!load_ok || gs.player_name[0] == '\0') {
                /* Load failed or corrupt save, start new game */
                delete_save();
                load = false;
                game_init(&gs);
            } else {
                entity_init();
                item_init();
                trap_init();
                loot_init();
                ammo_init();
                weather_init();
                spell_init();
                town_init();

                /* Recreate overworld (heap allocated, can't be saved) */
                gs.overworld = calloc(1, sizeof(Overworld));
                overworld_init(gs.overworld);
                overworld_spawn_creatures(gs.overworld);

                /* Clear invalid pointers from save */
                gs.dungeon = NULL;
                gs.current_town = NULL;

                /* Force overworld mode (dungeon/town state not preserved).
                 * If the player saved inside a dungeon or town, player_pos
                 * holds those local coordinates -- restore overworld pos. */
                if (gs.mode != MODE_OVERWORLD) {
                    gs.player_pos = gs.ow_player_pos;
                }
                gs.mode = MODE_OVERWORLD;
                gs.running = true;

                /* Defensive fallback: if ow_player_pos is invalid (e.g.
                 * a pre-fix save left it at 0,0 in the sea), drop player
                 * at Camelot. */
                {
                    int px = gs.player_pos.x, py = gs.player_pos.y;
                    bool bad = (px <= 0 || py <= 0 ||
                                px >= OW_WIDTH - 1 || py >= OW_HEIGHT - 1);
                    if (!bad) {
                        TileType tt = gs.overworld->map[py][px].type;
                        if (tt == TILE_WATER || tt == TILE_MOUNTAIN) bad = true;
                    }
                    if (bad) {
                        gs.player_pos = (Vec2){ 212, 162 }; /* Camelot */
                        gs.ow_player_pos = gs.player_pos;
                    }
                }

                /* Reinitialize message log */
                log_init(&gs.log);
                log_add(&gs.log, gs.turn, CP_WHITE,
                        "Welcome back, %s!", gs.player_name);
                log_add(&gs.log, gs.turn, CP_GRAY,
                        "Day %d, %02d:%02d. You resume your quest.",
                        gs.day, gs.hour, gs.minute);
            }
        } else {
            game_init(&gs);
        }
        /* Safety: ensure game is running before entering loop */
        gs.running = true;
        if (debug_mode) {
            gs.debug_mode = true;
            gs.cheat_mode = true; /* debug implies cheat (score=0) */
        }
        /* Flush any leftover input from title screen */
        flushinp();
        game_run(&gs);
    }

    /* Shutdown */
    ui_shutdown();

    if (play && load) {
        printf("DEBUG: Game exited after continue. running=%d mode=%d\n", 0, 0);
        printf("If this happened immediately, the save may be corrupt.\n");
        printf("Delete ~/.camelot/save.dat and try again.\n");
    }
    printf("Thanks for playing Knights of Camelot!\n");
    printf("Game seed: %llu\n", (unsigned long long)rng_get_seed());
    return 0;
}
