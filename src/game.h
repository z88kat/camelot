#ifndef GAME_H
#define GAME_H

#include "common.h"
#include "log.h"

/* Minimal game state for Phase 1 */
typedef struct {
    /* Map */
    Tile       map[MAP_HEIGHT][MAP_WIDTH];

    /* Player */
    Vec2       player_pos;
    char       player_name[MAX_NAME];
    int        player_level;
    int        hp, max_hp;
    int        mp, max_mp;
    int        str, def, intel, spd;
    int        gold;
    int        weight, max_weight;
    int        chivalry;

    /* Time */
    int        turn;
    int        day;
    int        hour;
    int        minute;

    /* State */
    GameMode   mode;
    bool       running;

    /* Message log */
    MessageLog log;
} GameState;

/* Initialize a new game state. */
void game_init(GameState *gs);

/* Run the main game loop. */
void game_run(GameState *gs);

/* Process a single input key. */
void game_handle_input(GameState *gs, int key);

/* Update game state after input. */
void game_update(GameState *gs);

/* Initialize the map with a simple test layout. */
void game_init_test_map(GameState *gs);

#endif /* GAME_H */
