#include "render.h"
#include "ui.h"
#include <ncurses.h>
#include <stdlib.h>
#include <stdarg.h>

/* ------------------------------------------------------------------ */
/* Backend-private data                                                */
/* ------------------------------------------------------------------ */
typedef struct RendererData {
    bool initialised;
} RendererData;

/* ------------------------------------------------------------------ */
/* Translate RATTR_* flags to ncurses attributes                       */
/* ------------------------------------------------------------------ */
static int rattr_to_ncurses(int attrs) {
    int a = 0;
    if (attrs & RATTR_BOLD)  a |= A_BOLD;
    if (attrs & RATTR_DIM)   a |= A_DIM;
    if (attrs & RATTR_BLINK) a |= A_BLINK;
    return a;
}

/* ------------------------------------------------------------------ */
/* Renderer interface implementations                                  */
/* ------------------------------------------------------------------ */
static bool rc_init(Renderer *r) {
    (void)r;
    return ui_init();
}

static void rc_shutdown(Renderer *r) {
    (void)r;
    ui_shutdown();
}

static void rc_get_size(Renderer *r, int *rows, int *cols) {
    (void)r;
    getmaxyx(stdscr, *rows, *cols);
}

static void rc_clear(Renderer *r) {
    (void)r;
    clear();
}

static void rc_refresh(Renderer *r) {
    (void)r;
    refresh();
}

static void rc_draw_char(Renderer *r, int x, int y, char ch,
                          short color_pair, int attrs) {
    (void)r;
    int na = rattr_to_ncurses(attrs);
    attron(COLOR_PAIR(color_pair) | na);
    mvaddch(y, x, ch);
    attroff(COLOR_PAIR(color_pair) | na);
}

static void rc_draw_text(Renderer *r, int row, int col,
                          short color_pair, int attrs,
                          const char *fmt, ...) {
    (void)r;
    int na = rattr_to_ncurses(attrs);
    attron(COLOR_PAIR(color_pair) | na);
    va_list ap;
    va_start(ap, fmt);
    move(row, col);
    vw_printw(stdscr, fmt, ap);
    va_end(ap);
    attroff(COLOR_PAIR(color_pair) | na);
}

static void rc_clear_line(Renderer *r, int row, int col) {
    (void)r;
    move(row, col);
    clrtoeol();
}

static int rc_getkey(Renderer *r) {
    (void)r;
    return getch();
}

static void rc_flush_input(Renderer *r) {
    (void)r;
    flushinp();
}

static void rc_set_blocking(Renderer *r, bool blocking) {
    (void)r;
    nodelay(stdscr, blocking ? FALSE : TRUE);
}

/* ------------------------------------------------------------------ */
/* High-level composites -- delegate to existing ui.c functions        */
/* ------------------------------------------------------------------ */
static void rc_render_map(Renderer *r,
                           Tile *map, int map_w, int map_h,
                           Vec2 player_pos, int view_w, int view_h) {
    (void)r;
    ui_render_map_generic(map, map_w, map_h, player_pos, view_w, view_h);
}

static void rc_render_sidebar(Renderer *r, int col,
                               const char *name, int level,
                               int hp, int max_hp, int mp, int max_mp,
                               int str, int def, int intel, int spd,
                               int gold, int weight, int max_weight,
                               int encumbrance, int mounted, int chivalry,
                               int turn, int day, int hour, int minute) {
    (void)r;
    ui_render_sidebar(col, name, level, hp, max_hp, mp, max_mp,
                      str, def, intel, spd, gold, weight, max_weight,
                      encumbrance, mounted, chivalry, turn, day, hour, minute);
}

static void rc_render_log(Renderer *r, const MessageLog *log,
                           int start_row, int width, int lines) {
    (void)r;
    ui_render_log(log, start_row, width, lines);
}

static void rc_render_status_bar(Renderer *r,
                                  int row, int width, const char *text) {
    (void)r;
    ui_render_status_bar(row, width, text);
}

static void rc_render_minimap(Renderer *r,
                               Tile *map, int map_w, int map_h,
                               Vec2 player_pos, const char *locations_info) {
    (void)r;
    ui_render_minimap(map, map_w, map_h, player_pos, locations_info);
}

static void rc_draw_tile(Renderer *r, int x, int y,
                          const char *tile_id, short fallback_color,
                          char fallback_ch) {
    /* ncurses backend ignores tile_id, uses ASCII fallback */
    (void)tile_id;
    rc_draw_char(r, x, y, fallback_ch, fallback_color, RATTR_NONE);
}

/* ------------------------------------------------------------------ */
/* Constructor                                                         */
/* ------------------------------------------------------------------ */

/* Global renderer pointer */
Renderer *g_renderer = NULL;

Renderer *render_curses_create(void) {
    Renderer *r = calloc(1, sizeof(Renderer));
    if (!r) return NULL;

    RendererData *d = calloc(1, sizeof(RendererData));
    if (!d) { free(r); return NULL; }
    r->data = d;

    r->init           = rc_init;
    r->shutdown       = rc_shutdown;
    r->get_size       = rc_get_size;
    r->clear          = rc_clear;
    r->refresh        = rc_refresh;
    r->draw_char      = rc_draw_char;
    r->draw_text      = rc_draw_text;
    r->clear_line     = rc_clear_line;
    r->getkey         = rc_getkey;
    r->flush_input    = rc_flush_input;
    r->set_blocking   = rc_set_blocking;
    r->render_map     = rc_render_map;
    r->render_sidebar = rc_render_sidebar;
    r->render_log     = rc_render_log;
    r->render_status_bar = rc_render_status_bar;
    r->render_minimap = rc_render_minimap;
    r->draw_tile      = rc_draw_tile;

    return r;
}
