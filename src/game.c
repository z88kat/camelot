#include "game.h"
#include "ui.h"
#include "rng.h"
#include <string.h>
#include <stdio.h>

/* Tile definitions for glyph and color */
static void tile_set(Tile *t, TileType type) {
    t->type = type;
    t->visible = true;   /* Phase 1: everything visible */
    t->revealed = true;

    switch (type) {
    case TILE_WALL:
        t->glyph = '#';
        t->color_pair = CP_GRAY;
        t->passable = false;
        t->blocks_sight = true;
        break;
    case TILE_FLOOR:
        t->glyph = '.';
        t->color_pair = CP_WHITE;
        t->passable = true;
        t->blocks_sight = false;
        break;
    case TILE_DOOR_CLOSED:
        t->glyph = '+';
        t->color_pair = CP_BROWN;
        t->passable = true;
        t->blocks_sight = true;
        break;
    case TILE_DOOR_OPEN:
        t->glyph = '/';
        t->color_pair = CP_BROWN;
        t->passable = true;
        t->blocks_sight = false;
        break;
    case TILE_STAIRS_DOWN:
        t->glyph = '>';
        t->color_pair = CP_WHITE;
        t->passable = true;
        t->blocks_sight = false;
        break;
    case TILE_STAIRS_UP:
        t->glyph = '<';
        t->color_pair = CP_WHITE;
        t->passable = true;
        t->blocks_sight = false;
        break;
    default:
        t->glyph = ' ';
        t->color_pair = CP_DEFAULT;
        t->passable = false;
        t->blocks_sight = true;
        break;
    }
}

/* Create a simple test dungeon: a room with corridors */
void game_init_test_map(GameState *gs) {
    /* Fill with walls */
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            tile_set(&gs->map[y][x], TILE_WALL);
        }
    }

    /* Carve a central room (30x12) */
    for (int y = 5; y < 17; y++) {
        for (int x = 20; x < 50; x++) {
            tile_set(&gs->map[y][x], TILE_FLOOR);
        }
    }

    /* Carve a smaller room to the left (10x6) */
    for (int y = 8; y < 14; y++) {
        for (int x = 5; x < 15; x++) {
            tile_set(&gs->map[y][x], TILE_FLOOR);
        }
    }

    /* Corridor connecting left room to central room */
    for (int x = 15; x < 20; x++) {
        tile_set(&gs->map[11][x], TILE_FLOOR);
    }

    /* Carve a room to the right (12x8) */
    for (int y = 4; y < 12; y++) {
        for (int x = 55; x < 67; x++) {
            tile_set(&gs->map[y][x], TILE_FLOOR);
        }
    }

    /* Corridor connecting central room to right room */
    for (int x = 50; x < 55; x++) {
        tile_set(&gs->map[8][x], TILE_FLOOR);
    }

    /* Carve a bottom room (15x5) */
    for (int y = 17; y < 21; y++) {
        for (int x = 30; x < 45; x++) {
            tile_set(&gs->map[y][x], TILE_FLOOR);
        }
    }

    /* Corridor connecting central room to bottom room */
    for (int y = 17; y < 18; y++) {
        tile_set(&gs->map[y][35], TILE_FLOOR);
    }

    /* Add a door between corridor and left room */
    tile_set(&gs->map[11][15], TILE_DOOR_CLOSED);

    /* Add stairs */
    tile_set(&gs->map[6][22], TILE_STAIRS_UP);
    tile_set(&gs->map[19][42], TILE_STAIRS_DOWN);
}

void game_init(GameState *gs) {
    memset(gs, 0, sizeof(*gs));

    /* Initialize player */
    snprintf(gs->player_name, MAX_NAME, "Sir Nobody");
    gs->player_level = 1;
    gs->hp = 25; gs->max_hp = 25;
    gs->mp = 10; gs->max_mp = 10;
    gs->str = 6; gs->def = 5; gs->intel = 4; gs->spd = 5;
    gs->gold = 50;
    gs->weight = 12; gs->max_weight = 48;
    gs->chivalry = 25;
    gs->turn = 0;
    gs->day = 1;
    gs->hour = 8;
    gs->minute = 0;

    gs->mode = MODE_DUNGEON;
    gs->running = true;

    /* Initialize map and place player */
    game_init_test_map(gs);
    gs->player_pos = (Vec2){ 35, 10 }; /* center of main room */

    /* Initialize log */
    log_init(&gs->log);
    log_add(&gs->log, gs->turn, CP_WHITE,
            "Welcome to Knights of Camelot! Press ? for help.");
    log_add(&gs->log, gs->turn, CP_YELLOW,
            "You stand in the dungeons beneath Camelot Castle.");
}

/* Convert a keypress to a direction, or DIR_NONE */
static Direction key_to_direction(int key) {
    switch (key) {
    /* Vi keys */
    case 'k': return DIR_N;
    case 'l': return DIR_E;
    case 'j': return DIR_S;
    case 'h': return DIR_W;
    case 'y': return DIR_NW;
    case 'u': return DIR_NE;
    case 'b': return DIR_SW;
    case 'n': return DIR_SE;

    /* Arrow keys */
    case KEY_UP:    return DIR_N;
    case KEY_DOWN:  return DIR_S;
    case KEY_LEFT:  return DIR_W;
    case KEY_RIGHT: return DIR_E;

    /* Numpad */
    case '8': return DIR_N;
    case '6': return DIR_E;
    case '2': return DIR_S;
    case '4': return DIR_W;
    case '7': return DIR_NW;
    case '9': return DIR_NE;
    case '1': return DIR_SW;
    case '3': return DIR_SE;

    default: return DIR_NONE;
    }
}

void game_handle_input(GameState *gs, int key) {
    /* Quit */
    if (key == 'q' || key == 'Q') {
        gs->running = false;
        return;
    }

    /* Wait */
    if (key == '.' || key == '5') {
        gs->turn++;
        log_add(&gs->log, gs->turn, CP_WHITE, "You wait...");
        return;
    }

    /* Movement */
    Direction dir = key_to_direction(key);
    if (dir != DIR_NONE) {
        int nx = gs->player_pos.x + dir_dx[dir];
        int ny = gs->player_pos.y + dir_dy[dir];

        if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT) {
            Tile *target = &gs->map[ny][nx];
            if (target->passable) {
                gs->player_pos.x = nx;
                gs->player_pos.y = ny;
                gs->turn++;

                /* Advance time: 1 turn = 1 minute */
                gs->minute++;
                if (gs->minute >= 60) {
                    gs->minute = 0;
                    gs->hour++;
                    if (gs->hour >= 24) {
                        gs->hour = 0;
                        gs->day++;
                    }
                }
            } else {
                log_add(&gs->log, gs->turn, CP_GRAY, "You bump into a wall.");
            }
        }
        return;
    }
}

void game_update(GameState *gs) {
    (void)gs; /* placeholder for Phase 1 */
}

/* Render the full game screen */
static void game_render(GameState *gs) {
    ui_clear();

    int term_rows, term_cols;
    ui_get_size(&term_rows, &term_cols);

    bool show_sidebar = (term_cols >= MIN_TERM_WIDTH);
    int map_view_width = show_sidebar ? (term_cols - SIDEBAR_WIDTH - 1) : term_cols;
    if (map_view_width > MAP_WIDTH) map_view_width = MAP_WIDTH;
    int map_view_height = MAP_HEIGHT;
    if (map_view_height > term_rows - LOG_LINES - 2) {
        map_view_height = term_rows - LOG_LINES - 2;
    }

    /* Map */
    ui_render_map(gs->map, gs->player_pos, map_view_width, map_view_height);

    /* Sidebar */
    if (show_sidebar) {
        int sidebar_col = map_view_width + 1;

        /* Draw vertical separator */
        attron(COLOR_PAIR(CP_GRAY));
        for (int y = 0; y < map_view_height; y++) {
            mvaddch(y, map_view_width, ACS_VLINE);
        }
        attroff(COLOR_PAIR(CP_GRAY));

        ui_render_sidebar(sidebar_col, gs->player_name, gs->player_level,
                          gs->hp, gs->max_hp, gs->mp, gs->max_mp,
                          gs->str, gs->def, gs->intel, gs->spd,
                          gs->gold, gs->weight, gs->max_weight, gs->chivalry,
                          gs->turn, gs->day, gs->hour, gs->minute);
    }

    /* Status bar */
    int status_row = map_view_height;
    char status[256] = "";
    snprintf(status, sizeof(status), "Dungeon | Turn %d | Day %d %02d:%02d",
             gs->turn, gs->day, gs->hour, gs->minute);
    ui_render_status_bar(status_row, term_cols, status);

    /* Message log */
    int log_row = status_row + 1;
    int log_lines = term_rows - log_row;
    if (log_lines > LOG_LINES) log_lines = LOG_LINES;
    if (log_lines > 0) {
        ui_render_log(&gs->log, log_row, term_cols, log_lines);
    }

    ui_refresh();
}

void game_run(GameState *gs) {
    while (gs->running) {
        game_render(gs);
        int key = ui_getkey();
        game_handle_input(gs, key);
        game_update(gs);
    }
}
