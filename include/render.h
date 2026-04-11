#ifndef RENDER_H
#define RENDER_H

#include "common.h"
#include "log.h"

/* Backend-neutral text attribute flags */
#define RATTR_NONE   0x00
#define RATTR_BOLD   0x01
#define RATTR_DIM    0x02
#define RATTR_BLINK  0x04

/* Opaque backend-specific data */
typedef struct RendererData RendererData;

typedef struct Renderer {
    /* Lifecycle */
    bool (*init)(struct Renderer *r);
    void (*shutdown)(struct Renderer *r);

    /* Screen geometry in character/tile cells */
    void (*get_size)(struct Renderer *r, int *rows, int *cols);

    /* Primitive drawing */
    void (*clear)(struct Renderer *r);
    void (*refresh)(struct Renderer *r);
    void (*draw_char)(struct Renderer *r, int x, int y, char ch,
                      short color_pair, int attrs);

    /* Formatted text at (row, col) -- replaces attron/mvprintw/attroff */
    void (*draw_text)(struct Renderer *r, int row, int col,
                      short color_pair, int attrs,
                      const char *fmt, ...);

    /* Clear to end of line from (row, col) */
    void (*clear_line)(struct Renderer *r, int row, int col);

    /* Input -- blocking keypress */
    int  (*getkey)(struct Renderer *r);

    /* Flush pending input */
    void (*flush_input)(struct Renderer *r);

    /* Set blocking / non-blocking input mode */
    void (*set_blocking)(struct Renderer *r, bool blocking);

    /* High-level composites (backend may override for tile rendering) */
    void (*render_map)(struct Renderer *r,
                       Tile *map, int map_w, int map_h,
                       Vec2 player_pos, int view_w, int view_h);
    void (*render_sidebar)(struct Renderer *r, int col,
                           const char *name, int level,
                           int hp, int max_hp, int mp, int max_mp,
                           int str, int def, int intel, int spd,
                           int gold, int weight, int max_weight,
                           int encumbrance, int mounted, int chivalry,
                           int turn, int day, int hour, int minute);
    void (*render_log)(struct Renderer *r, const MessageLog *log,
                       int start_row, int width, int lines);
    void (*render_status_bar)(struct Renderer *r,
                              int row, int width, const char *text);
    void (*render_minimap)(struct Renderer *r,
                           Tile *map, int map_w, int map_h,
                           Vec2 player_pos, const char *locations_info);

    /* Draw a tile from the tileset at screen position.
     * ncurses backend uses fallback_ch/fallback_color. */
    void (*draw_tile)(struct Renderer *r, int x, int y,
                      const char *tile_id, short fallback_color, char fallback_ch);

    /* Backend-private data */
    RendererData *data;
} Renderer;

/* Global renderer instance */
extern Renderer *g_renderer;

/* Backend constructors */
Renderer *render_curses_create(void);

#ifdef HAS_SDL2
Renderer *render_sdl_create(const char *tiles_dir);
#endif

/* ------------------------------------------------------------------ */
/* Convenience macros                                                  */
/* ------------------------------------------------------------------ */
#define R_INIT()              g_renderer->init(g_renderer)
#define R_SHUTDOWN()          g_renderer->shutdown(g_renderer)
#define R_CLEAR()             g_renderer->clear(g_renderer)
#define R_REFRESH()           g_renderer->refresh(g_renderer)
#define R_GETKEY()            g_renderer->getkey(g_renderer)
#define R_GET_SIZE(r, c)      g_renderer->get_size(g_renderer, r, c)
#define R_DRAW_CHAR(x, y, ch, cp, a) \
    g_renderer->draw_char(g_renderer, x, y, ch, cp, a)
#define R_DRAW_TEXT(row, col, cp, a, ...) \
    g_renderer->draw_text(g_renderer, row, col, cp, a, __VA_ARGS__)
#define R_CLEAR_LINE(row, col) \
    g_renderer->clear_line(g_renderer, row, col)
#define R_FLUSH_INPUT()       g_renderer->flush_input(g_renderer)
#define R_SET_BLOCKING(b)     g_renderer->set_blocking(g_renderer, b)

#endif /* RENDER_H */
