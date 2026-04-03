#include "game.h"
#include "ui.h"
#include "rng.h"
#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/* Tile helpers                                                        */
/* ------------------------------------------------------------------ */

static void tile_set(Tile *t, TileType type) {
    t->type = type;
    t->visible = true;
    t->revealed = true;

    switch (type) {
    case TILE_WALL:
        t->glyph = '#'; t->color_pair = CP_GRAY;
        t->passable = false; t->blocks_sight = true;
        break;
    case TILE_FLOOR:
        t->glyph = '.'; t->color_pair = CP_WHITE;
        t->passable = true; t->blocks_sight = false;
        break;
    case TILE_DOOR_CLOSED:
        t->glyph = '+'; t->color_pair = CP_BROWN;
        t->passable = true; t->blocks_sight = true;
        break;
    case TILE_DOOR_OPEN:
        t->glyph = '/'; t->color_pair = CP_BROWN;
        t->passable = true; t->blocks_sight = false;
        break;
    case TILE_STAIRS_DOWN:
        t->glyph = '>'; t->color_pair = CP_WHITE;
        t->passable = true; t->blocks_sight = false;
        break;
    case TILE_STAIRS_UP:
        t->glyph = '<'; t->color_pair = CP_WHITE;
        t->passable = true; t->blocks_sight = false;
        break;
    default:
        t->glyph = ' '; t->color_pair = CP_DEFAULT;
        t->passable = false; t->blocks_sight = true;
        break;
    }
}

/* ------------------------------------------------------------------ */
/* Test dungeon (Phase 1 holdover, will be replaced by BSP gen)        */
/* ------------------------------------------------------------------ */

void game_init_test_map(GameState *gs) {
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            tile_set(&gs->dungeon_map[y][x], TILE_WALL);

    /* Central room */
    for (int y = 5; y < 17; y++)
        for (int x = 20; x < 50; x++)
            tile_set(&gs->dungeon_map[y][x], TILE_FLOOR);

    /* Left room */
    for (int y = 8; y < 14; y++)
        for (int x = 5; x < 15; x++)
            tile_set(&gs->dungeon_map[y][x], TILE_FLOOR);

    /* Corridor left -> center */
    for (int x = 15; x < 20; x++)
        tile_set(&gs->dungeon_map[11][x], TILE_FLOOR);

    /* Right room */
    for (int y = 4; y < 12; y++)
        for (int x = 55; x < 67; x++)
            tile_set(&gs->dungeon_map[y][x], TILE_FLOOR);

    /* Corridor center -> right */
    for (int x = 50; x < 55; x++)
        tile_set(&gs->dungeon_map[8][x], TILE_FLOOR);

    /* Bottom room */
    for (int y = 17; y < 21; y++)
        for (int x = 30; x < 45; x++)
            tile_set(&gs->dungeon_map[y][x], TILE_FLOOR);

    /* Corridor center -> bottom */
    tile_set(&gs->dungeon_map[17][35], TILE_FLOOR);

    /* Door and stairs */
    tile_set(&gs->dungeon_map[11][15], TILE_DOOR_CLOSED);
    tile_set(&gs->dungeon_map[6][22], TILE_STAIRS_UP);
    tile_set(&gs->dungeon_map[19][42], TILE_STAIRS_DOWN);
}

/* ------------------------------------------------------------------ */
/* Direction helpers                                                    */
/* ------------------------------------------------------------------ */

static Direction key_to_direction(int key) {
    switch (key) {
    case 'k': case KEY_UP:    case '8': return DIR_N;
    case 'l': case KEY_RIGHT: case '6': return DIR_E;
    case 'j': case KEY_DOWN:  case '2': return DIR_S;
    case 'h': case KEY_LEFT:  case '4': return DIR_W;
    case 'y': case '7': return DIR_NW;
    case 'u': case '9': return DIR_NE;
    case 'b': case '1': return DIR_SW;
    case 'n': case '3': return DIR_SE;
    default: return DIR_NONE;
    }
}

/* Advance game clock by 1 turn (1 minute) */
static void advance_time(GameState *gs) {
    gs->turn++;
    gs->minute++;
    if (gs->minute >= 60) {
        gs->minute = 0;
        gs->hour++;
        if (gs->hour >= 24) {
            gs->hour = 0;
            gs->day++;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Terrain name for status display                                     */
/* ------------------------------------------------------------------ */

static const char *terrain_name(TileType type) {
    switch (type) {
    case TILE_ROAD:   return "Road";
    case TILE_GRASS:  return "Grassland";
    case TILE_FOREST: return "Forest";
    case TILE_HILLS:  return "Hills";
    case TILE_MARSH:  return "Marsh";
    case TILE_SWAMP:  return "Swamp";
    case TILE_BRIDGE: return "Bridge";
    case TILE_WATER:  return "Sea";
    case TILE_RIVER:  return "River";
    case TILE_LAKE:   return "Lake";
    default:          return "";
    }
}

/* ------------------------------------------------------------------ */
/* Overworld input                                                     */
/* ------------------------------------------------------------------ */

static void handle_overworld_input(GameState *gs, int key) {
    Direction dir = key_to_direction(key);
    if (dir != DIR_NONE) {
        int nx = gs->player_pos.x + dir_dx[dir];
        int ny = gs->player_pos.y + dir_dy[dir];

        if (!overworld_is_passable(&gs->overworld, nx, ny)) {
            TileType tt = gs->overworld.map[ny][nx].type;
            if (tt == TILE_WATER || tt == TILE_LAKE)
                log_add(&gs->log, gs->turn, CP_BLUE, "The water blocks your path.");
            else if (tt == TILE_RIVER)
                log_add(&gs->log, gs->turn, CP_BLUE, "The river is too deep to cross. Find a bridge.");
            else
                log_add(&gs->log, gs->turn, CP_GRAY, "You cannot go that way.");
            return;
        }

        gs->player_pos.x = nx;
        gs->player_pos.y = ny;
        advance_time(gs);

        /* Check for a location at the new position */
        Location *loc = overworld_location_at(&gs->overworld, nx, ny);
        if (loc) {
            if (!loc->discovered) {
                loc->discovered = true;
                log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                         "You discover %s!", loc->name);
            }

            switch (loc->type) {
            case LOC_TOWN:
            case LOC_CASTLE_ACTIVE:
                log_add(&gs->log, gs->turn, CP_WHITE,
                         "You arrive at %s. (Towns not yet implemented)", loc->name);
                break;
            case LOC_CASTLE_ABANDONED:
                log_add(&gs->log, gs->turn, CP_GRAY,
                         "The ruins of %s loom before you.", loc->name);
                break;
            case LOC_DUNGEON_ENTRANCE:
                log_add(&gs->log, gs->turn, CP_WHITE,
                         "A dungeon entrance: %s. Press > to enter.", loc->name);
                break;
            case LOC_LANDMARK:
                log_add(&gs->log, gs->turn, CP_YELLOW,
                         "You stand at %s.", loc->name);
                break;
            case LOC_VOLCANO:
                log_add(&gs->log, gs->turn, CP_RED,
                         "Mount Draig! Smoke rises from the volcano.");
                break;
            default:
                break;
            }
        }
        return;
    }

    /* Enter dungeon */
    if (key == '>') {
        Location *loc = overworld_location_at(&gs->overworld,
                                               gs->player_pos.x, gs->player_pos.y);
        if (loc && (loc->type == LOC_DUNGEON_ENTRANCE || loc->type == LOC_VOLCANO)) {
            gs->ow_player_pos = gs->player_pos;
            gs->player_pos = (Vec2){ 35, 10 };
            gs->mode = MODE_DUNGEON;
            log_add(&gs->log, gs->turn, CP_WHITE,
                     "You descend into %s...", loc->name);
        } else {
            log_add(&gs->log, gs->turn, CP_GRAY, "There is no entrance here.");
        }
        return;
    }
}

/* ------------------------------------------------------------------ */
/* Dungeon input                                                       */
/* ------------------------------------------------------------------ */

static void handle_dungeon_input(GameState *gs, int key) {
    /* Ascend stairs to overworld */
    if (key == '<') {
        Tile *t = &gs->dungeon_map[gs->player_pos.y][gs->player_pos.x];
        if (t->type == TILE_STAIRS_UP) {
            gs->player_pos = gs->ow_player_pos;
            gs->mode = MODE_OVERWORLD;
            log_add(&gs->log, gs->turn, CP_WHITE,
                     "You climb back to the surface.");
            return;
        }
        log_add(&gs->log, gs->turn, CP_GRAY, "There are no stairs up here.");
        return;
    }

    Direction dir = key_to_direction(key);
    if (dir != DIR_NONE) {
        int nx = gs->player_pos.x + dir_dx[dir];
        int ny = gs->player_pos.y + dir_dy[dir];

        if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT) {
            Tile *target = &gs->dungeon_map[ny][nx];
            if (target->passable) {
                gs->player_pos.x = nx;
                gs->player_pos.y = ny;
                advance_time(gs);
            } else {
                log_add(&gs->log, gs->turn, CP_GRAY, "You bump into a wall.");
            }
        }
        return;
    }
}

/* ------------------------------------------------------------------ */
/* Main input dispatch                                                 */
/* ------------------------------------------------------------------ */

void game_handle_input(GameState *gs, int key) {
    if (key == 'q' || key == 'Q') {
        gs->running = false;
        return;
    }

    /* Wait */
    if (key == '.' || key == '5') {
        advance_time(gs);
        log_add(&gs->log, gs->turn, CP_WHITE, "You wait...");
        return;
    }

    switch (gs->mode) {
    case MODE_OVERWORLD:
        handle_overworld_input(gs, key);
        break;
    case MODE_DUNGEON:
        handle_dungeon_input(gs, key);
        break;
    default:
        break;
    }
}

/* ------------------------------------------------------------------ */
/* Game init                                                           */
/* ------------------------------------------------------------------ */

void game_init(GameState *gs) {
    memset(gs, 0, sizeof(*gs));

    /* Player defaults */
    snprintf(gs->player_name, MAX_NAME, "Sir Nobody");
    gs->player_level = 1;
    gs->hp = 25;  gs->max_hp = 25;
    gs->mp = 10;  gs->max_mp = 10;
    gs->str = 6;  gs->def = 5;  gs->intel = 4;  gs->spd = 5;
    gs->gold = 50;
    gs->weight = 12; gs->max_weight = 48;
    gs->chivalry = 25;
    gs->turn = 0;
    gs->day = 1;
    gs->hour = 8;
    gs->minute = 0;

    /* Start on overworld at Camelot */
    gs->mode = MODE_OVERWORLD;
    gs->running = true;

    /* Initialize overworld */
    overworld_init(&gs->overworld);

    /* Place player at Camelot (45, 22) */
    gs->player_pos = (Vec2){ 45, 22 };
    gs->ow_player_pos = gs->player_pos;

    /* Initialize dungeon (test map for now) */
    game_init_test_map(gs);

    /* Log */
    log_init(&gs->log);
    log_add(&gs->log, gs->turn, CP_WHITE,
            "Welcome to Knights of Camelot! Press ? for help.");
    log_add(&gs->log, gs->turn, CP_YELLOW,
            "You stand at the gates of Camelot. Seek King Arthur.");
}

void game_update(GameState *gs) {
    (void)gs;
}

/* ------------------------------------------------------------------ */
/* Rendering                                                           */
/* ------------------------------------------------------------------ */

static void game_render(GameState *gs) {
    ui_clear();

    int term_rows, term_cols;
    ui_get_size(&term_rows, &term_cols);

    bool show_sidebar = (term_cols >= MIN_TERM_WIDTH);
    int map_view_width = show_sidebar ? (term_cols - SIDEBAR_WIDTH - 1) : term_cols;
    int map_view_height = MAP_HEIGHT;
    if (map_view_height > term_rows - LOG_LINES - 2)
        map_view_height = term_rows - LOG_LINES - 2;
    if (map_view_height < 5) map_view_height = 5;

    /* Render the appropriate map */
    if (gs->mode == MODE_OVERWORLD) {
        /* Clamp view to overworld dimensions */
        if (map_view_width > OW_WIDTH) map_view_width = OW_WIDTH;
        if (map_view_height > OW_HEIGHT) map_view_height = OW_HEIGHT;

        ui_render_map_generic((Tile *)gs->overworld.map, OW_WIDTH, OW_HEIGHT,
                              gs->player_pos, map_view_width, map_view_height);
    } else if (gs->mode == MODE_DUNGEON) {
        if (map_view_width > MAP_WIDTH) map_view_width = MAP_WIDTH;

        ui_render_map(gs->dungeon_map, gs->player_pos,
                      map_view_width, map_view_height);
    }

    /* Sidebar */
    if (show_sidebar) {
        int sidebar_col = map_view_width + 1;

        attron(COLOR_PAIR(CP_GRAY));
        for (int y = 0; y < map_view_height; y++)
            mvaddch(y, map_view_width, ACS_VLINE);
        attroff(COLOR_PAIR(CP_GRAY));

        ui_render_sidebar(sidebar_col, gs->player_name, gs->player_level,
                          gs->hp, gs->max_hp, gs->mp, gs->max_mp,
                          gs->str, gs->def, gs->intel, gs->spd,
                          gs->gold, gs->weight, gs->max_weight, gs->chivalry,
                          gs->turn, gs->day, gs->hour, gs->minute);
    }

    /* Status bar */
    int status_row = map_view_height;
    char status[256];

    if (gs->mode == MODE_OVERWORLD) {
        /* Show terrain and location info */
        Tile *t = &gs->overworld.map[gs->player_pos.y][gs->player_pos.x];
        Location *loc = overworld_location_at(&gs->overworld,
                                               gs->player_pos.x, gs->player_pos.y);
        if (loc) {
            snprintf(status, sizeof(status), "Overworld: %s | %s | Turn %d | Day %d %02d:%02d",
                     loc->name, terrain_name(t->type),
                     gs->turn, gs->day, gs->hour, gs->minute);
        } else {
            snprintf(status, sizeof(status), "Overworld: %s | Turn %d | Day %d %02d:%02d",
                     terrain_name(t->type),
                     gs->turn, gs->day, gs->hour, gs->minute);
        }
    } else {
        snprintf(status, sizeof(status), "Dungeon | Turn %d | Day %d %02d:%02d",
                 gs->turn, gs->day, gs->hour, gs->minute);
    }

    ui_render_status_bar(status_row, term_cols, status);

    /* Message log */
    int log_row = status_row + 1;
    int log_lines = term_rows - log_row;
    if (log_lines > LOG_LINES) log_lines = LOG_LINES;
    if (log_lines > 0)
        ui_render_log(&gs->log, log_row, term_cols, log_lines);

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
