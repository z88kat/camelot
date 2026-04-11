#include "game.h"
#include "ui.h"
#include "render.h"
#include "rng.h"
#include "save.h"
#include "loot.h"
#include "ammo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Title screen and menu defined in game.c */
extern int show_title_screen(void);
extern void show_high_scores_screen(void);
extern void show_fallen_heroes_screen(void);

int main(int argc, char *argv[]) {
    /* Parse command line flags */
    uint64_t seed = 0;
    bool debug_mode = false;
    const char *gfx_backend = "curses";
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 's' && i + 1 < argc) {
            seed = (uint64_t)atoll(argv[++i]);
        }
        if (argv[i][0] == '-' && argv[i][1] == 'd') {
            debug_mode = true;
        }
        if (strcmp(argv[i], "--gfx") == 0 && i + 1 < argc) {
            gfx_backend = argv[++i];
        }
    }

    /* Initialize RNG */
    rng_seed(seed);

    /* Initialize renderer */
    if (strcmp(gfx_backend, "sdl") == 0) {
#ifdef HAS_SDL2
        g_renderer = render_sdl_create("tiles");
#else
        fprintf(stderr, "SDL2 backend not available (compiled without SDL2).\n");
        return 1;
#endif
    } else {
        g_renderer = render_curses_create();
    }

    if (!g_renderer || !R_INIT()) {
        fprintf(stderr, "Error: failed to initialise %s renderer.\n", gfx_backend);
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

                /* Recreate overworld (heap allocated, can't be saved).
                 * Always rebuild it -- the player will need it when they
                 * eventually leave the dungeon. */
                gs.overworld = calloc(1, sizeof(Overworld));
                overworld_init(gs.overworld);
                overworld_spawn_creatures(gs.overworld);

                /* Town interior state is not preserved across saves. */
                gs.current_town = NULL;

                gs.running = true;

                /* Boats are regenerated on overworld init, so the saved
                 * sailing flag is stale -- drop it. */
                gs.in_boat = false;

                if (gs.dungeon != NULL && gs.mode == MODE_DUNGEON) {
                    /* Player saved inside a dungeon. Keep mode + position
                     * as-is; the restored dungeon holds tiles, monsters,
                     * items, and explored state. Skip the overworld
                     * fallback below. */
                } else {
                    /* No dungeon to restore (or saved in town/overworld).
                     * Force overworld mode and validate the position. */
                    if (gs.mode != MODE_OVERWORLD) {
                        gs.player_pos = gs.ow_player_pos;
                    }
                    gs.mode = MODE_OVERWORLD;

                    /* Defensive fallback: if ow_player_pos is invalid
                     * (sea, mountain, edge of map), find the nearest
                     * passable land tile, falling back to Camelot. */
                    int px = gs.player_pos.x, py = gs.player_pos.y;
                    bool bad = (px <= 0 || py <= 0 ||
                                px >= OW_WIDTH - 1 || py >= OW_HEIGHT - 1);
                    if (!bad) {
                        TileType tt = gs.overworld->map[py][px].type;
                        if (tt == TILE_WATER || tt == TILE_LAKE ||
                            tt == TILE_RIVER || tt == TILE_MOUNTAIN) bad = true;
                    }
                    if (bad) {
                        bool found = false;
                        for (int r = 1; r < 40 && !found; r++) {
                            for (int dy = -r; dy <= r && !found; dy++) {
                                for (int dx = -r; dx <= r && !found; dx++) {
                                    if (dx*dx + dy*dy > r*r) continue;
                                    int nx = px + dx, ny = py + dy;
                                    if (nx <= 0 || ny <= 0 ||
                                        nx >= OW_WIDTH-1 || ny >= OW_HEIGHT-1) continue;
                                    TileType nt = gs.overworld->map[ny][nx].type;
                                    if (nt != TILE_WATER && nt != TILE_LAKE &&
                                        nt != TILE_RIVER && nt != TILE_MOUNTAIN) {
                                        gs.player_pos = (Vec2){ nx, ny };
                                        gs.ow_player_pos = gs.player_pos;
                                        found = true;
                                    }
                                }
                            }
                        }
                        if (!found) {
                            gs.player_pos = (Vec2){ 212, 162 }; /* Camelot */
                            gs.ow_player_pos = gs.player_pos;
                        }
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
    R_SHUTDOWN();

    printf("Thanks for playing Knights of Camelot!\n");
    printf("Game seed: %llu\n", (unsigned long long)rng_get_seed());
    return 0;
}
