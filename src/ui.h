#ifndef UI_H
#define UI_H

#include "common.h"
#include "log.h"
#include <ncurses.h>

/* Initialize ncurses, colors, windows. Returns false on failure. */
bool ui_init(void);

/* Shut down ncurses cleanly. */
void ui_shutdown(void);

/* Get terminal dimensions. */
void ui_get_size(int *rows, int *cols);

/* Render a map viewport centered on the player (generic, any dimensions). */
void ui_render_map_generic(Tile *map, int map_w, int map_h,
                           Vec2 player_pos, int view_width, int view_height);

/* Render dungeon map (MAP_WIDTH x MAP_HEIGHT). */
void ui_render_map(Tile map[MAP_HEIGHT][MAP_WIDTH], Vec2 player_pos,
                   int view_width, int view_height);

/* Render the sidebar with character stats. */
void ui_render_sidebar(int col, const char *name, int level, int hp, int max_hp,
                       int mp, int max_mp, int str, int def, int intel, int spd,
                       int gold, int weight, int max_weight, int chivalry,
                       int turn, int day, int hour, int minute);

/* Render the message log at the bottom. */
void ui_render_log(const MessageLog *log, int start_row, int width, int lines);

/* Render a simple status effect bar. */
void ui_render_status_bar(int row, int width, const char *status_text);

/* Draw a single character at map position with color. */
void ui_draw_char(int x, int y, char ch, short color_pair, bool bold);

/* Clear the screen. */
void ui_clear(void);

/* Refresh the screen. */
void ui_refresh(void);

/* Get a keypress (blocking). */
int ui_getkey(void);

#endif /* UI_H */
