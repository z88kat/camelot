#ifdef HAS_SDL2

#include "render.h"
#include "tileset.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/* Constants                                                           */
/* ------------------------------------------------------------------ */
#define TILE_PX          16   /* DawnLike tile size in pixels */
#define FONT_GLYPH_W      8   /* SDS_8x8 native glyph width */
#define FONT_GLYPH_H      8   /* SDS_8x8 native glyph height */
#define FONT_SCALE         2   /* Scale to 16px to match tiles */
#define CELL_PX           16   /* Rendered cell size (FONT_GLYPH_W * FONT_SCALE) */
#define DEFAULT_COLS      80
#define DEFAULT_ROWS      36
#define MAX_SHEET_CACHE   64

/* ------------------------------------------------------------------ */
/* CP_* to RGB lookup                                                  */
/* ------------------------------------------------------------------ */
typedef struct { uint8_t r, g, b; } RGB;

static const RGB cp_to_rgb[] = {
    [CP_DEFAULT]      = { 192, 192, 192 },
    [CP_WHITE]        = { 192, 192, 192 },
    [CP_RED]          = { 255, 128,   0 },  /* orange, matching ncurses 208 */
    [CP_GREEN]        = {   0, 192,   0 },
    [CP_YELLOW]       = { 192, 192,   0 },
    [CP_BLUE]         = {   0, 128, 255 },
    [CP_MAGENTA]      = { 192,   0, 192 },
    [CP_CYAN]         = {   0, 192, 192 },
    [CP_BROWN]        = { 128,  96,   0 },
    [CP_GRAY]         = { 128, 128, 128 },
    [CP_WHITE_BOLD]   = { 255, 255, 255 },
    [CP_RED_BOLD]     = { 255, 160,   0 },
    [CP_GREEN_BOLD]   = {   0, 255,   0 },
    [CP_YELLOW_BOLD]  = { 255, 255,   0 },
    [CP_BLUE_BOLD]    = {   0, 160, 255 },
    [CP_MAGENTA_BOLD] = { 255,   0, 255 },
    [CP_CYAN_BOLD]    = {   0, 255, 255 },
    [CP_STATUS_BAR]   = { 255, 255, 255 },
    [CP_LOG_DAMAGE]   = { 255, 128,   0 },
    [CP_LOG_HEAL]     = {   0, 255,   0 },
    [CP_LOG_QUEST]    = { 255, 255,   0 },
    [CP_LOG_LOOT]     = {   0, 255, 255 },
};
#define NUM_CP (sizeof(cp_to_rgb) / sizeof(cp_to_rgb[0]))

static RGB color_for(short cp, int attrs) {
    RGB c = (cp >= 0 && (size_t)cp < NUM_CP) ? cp_to_rgb[cp] : cp_to_rgb[CP_DEFAULT];
    if (attrs & RATTR_BOLD) {
        /* Brighten */
        int r = c.r + 63; if (r > 255) r = 255; c.r = (uint8_t)r;
        int g = c.g + 63; if (g > 255) g = 255; c.g = (uint8_t)g;
        int b = c.b + 63; if (b > 255) b = 255; c.b = (uint8_t)b;
    }
    if (attrs & RATTR_DIM) {
        c.r /= 2; c.g /= 2; c.b /= 2;
    }
    return c;
}

/* ------------------------------------------------------------------ */
/* Backend-private data                                                */
/* ------------------------------------------------------------------ */
typedef struct RendererData {
    SDL_Window   *window;
    SDL_Renderer *sdl;
    Tileset       tileset;
    char          tiles_dir[256];

    /* Sprite sheet texture cache */
    struct {
        char  path[TILE_SHEET_MAX];
        SDL_Texture *tex;
    } sheet_cache[MAX_SHEET_CACHE];
    int num_sheets;

    /* Font */
    TTF_Font     *font;
    SDL_Texture  *font_atlas;  /* pre-rendered ASCII 32-126 glyphs */
    int           cols, rows;  /* window size in cells */
} RendererData;

/* ------------------------------------------------------------------ */
/* Sheet cache                                                         */
/* ------------------------------------------------------------------ */
static SDL_Texture *load_sheet(RendererData *d, const char *rel_path) {
    /* Check cache */
    for (int i = 0; i < d->num_sheets; i++) {
        if (strcmp(d->sheet_cache[i].path, rel_path) == 0)
            return d->sheet_cache[i].tex;
    }

    /* Load from disk */
    char full[512];
    snprintf(full, sizeof(full), "%s/%s", d->tiles_dir, rel_path);
    SDL_Surface *surf = IMG_Load(full);
    if (!surf) return NULL;

    SDL_Texture *tex = SDL_CreateTextureFromSurface(d->sdl, surf);
    SDL_FreeSurface(surf);
    if (!tex) return NULL;

    if (d->num_sheets < MAX_SHEET_CACHE) {
        snprintf(d->sheet_cache[d->num_sheets].path, TILE_SHEET_MAX, "%s", rel_path);
        d->sheet_cache[d->num_sheets].tex = tex;
        d->num_sheets++;
    }
    return tex;
}

/* ------------------------------------------------------------------ */
/* Font atlas: pre-render ASCII 32-126 into a texture strip            */
/* ------------------------------------------------------------------ */
static bool build_font_atlas(RendererData *d) {
    int num_glyphs = 95; /* 32..126 */
    int atlas_w = num_glyphs * FONT_GLYPH_W;
    int atlas_h = FONT_GLYPH_H;

    SDL_Surface *atlas_surf = SDL_CreateRGBSurfaceWithFormat(
        0, atlas_w, atlas_h, 32, SDL_PIXELFORMAT_RGBA32);
    if (!atlas_surf) return false;

    SDL_Color white = { 255, 255, 255, 255 };
    for (int i = 0; i < num_glyphs; i++) {
        char ch[2] = { (char)(32 + i), '\0' };
        SDL_Surface *glyph = TTF_RenderText_Solid(d->font, ch, white);
        if (glyph) {
            SDL_Rect dst = { i * FONT_GLYPH_W, 0, glyph->w, glyph->h };
            SDL_BlitSurface(glyph, NULL, atlas_surf, &dst);
            SDL_FreeSurface(glyph);
        }
    }

    d->font_atlas = SDL_CreateTextureFromSurface(d->sdl, atlas_surf);
    SDL_FreeSurface(atlas_surf);
    return d->font_atlas != NULL;
}

/* Draw a single character from the font atlas */
static void draw_font_char(RendererData *d, int px, int py, char ch, RGB color) {
    if (ch < 32 || ch > 126) ch = '?';
    int idx = ch - 32;
    SDL_Rect src = { idx * FONT_GLYPH_W, 0, FONT_GLYPH_W, FONT_GLYPH_H };
    SDL_Rect dst = { px, py, CELL_PX, CELL_PX };
    SDL_SetTextureColorMod(d->font_atlas, color.r, color.g, color.b);
    SDL_RenderCopy(d->sdl, d->font_atlas, &src, &dst);
}

/* ------------------------------------------------------------------ */
/* Renderer interface implementations                                  */
/* ------------------------------------------------------------------ */
static bool rs_init(Renderer *r) {
    RendererData *d = r->data;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) return false;
    if (IMG_Init(IMG_INIT_PNG) == 0) return false;
    if (TTF_Init() < 0) return false;

    d->cols = DEFAULT_COLS;
    d->rows = DEFAULT_ROWS;
    int win_w = d->cols * CELL_PX;
    int win_h = d->rows * CELL_PX;

    d->window = SDL_CreateWindow("Knights of Camelot",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        win_w, win_h, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!d->window) return false;

    d->sdl = SDL_CreateRenderer(d->window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!d->sdl) return false;

    /* Load tileset mapping */
    tileset_load(&d->tileset, "data/tileset.csv");

    /* Load bitmap font */
    char font_path[512];
    snprintf(font_path, sizeof(font_path), "%s/GUI/SDS_8x8.ttf", d->tiles_dir);
    d->font = TTF_OpenFont(font_path, FONT_GLYPH_H);
    if (!d->font) {
        fprintf(stderr, "Warning: could not load font %s: %s\n",
                font_path, TTF_GetError());
        return false;
    }

    if (!build_font_atlas(d)) return false;

    return true;
}

static void rs_shutdown(Renderer *r) {
    RendererData *d = r->data;
    if (d->font_atlas) SDL_DestroyTexture(d->font_atlas);
    if (d->font) TTF_CloseFont(d->font);
    for (int i = 0; i < d->num_sheets; i++)
        SDL_DestroyTexture(d->sheet_cache[i].tex);
    if (d->sdl) SDL_DestroyRenderer(d->sdl);
    if (d->window) SDL_DestroyWindow(d->window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

static void rs_get_size(Renderer *r, int *rows, int *cols) {
    RendererData *d = r->data;
    int w, h;
    SDL_GetWindowSize(d->window, &w, &h);
    d->cols = w / CELL_PX;
    d->rows = h / CELL_PX;
    if (d->cols < 1) d->cols = 1;
    if (d->rows < 1) d->rows = 1;
    *rows = d->rows;
    *cols = d->cols;
}

static void rs_clear(Renderer *r) {
    RendererData *d = r->data;
    SDL_SetRenderDrawColor(d->sdl, 0, 0, 0, 255);
    SDL_RenderClear(d->sdl);
}

static void rs_refresh(Renderer *r) {
    RendererData *d = r->data;
    SDL_RenderPresent(d->sdl);
}

static void rs_draw_char(Renderer *r, int x, int y, char ch,
                          short color_pair, int attrs) {
    RendererData *d = r->data;
    RGB c = color_for(color_pair, attrs);
    draw_font_char(d, x * CELL_PX, y * CELL_PX, ch, c);
}

static void rs_draw_text(Renderer *r, int row, int col,
                          short color_pair, int attrs,
                          const char *fmt, ...) {
    RendererData *d = r->data;
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    RGB c = color_for(color_pair, attrs);
    int px = col * CELL_PX;
    int py = row * CELL_PX;
    for (int i = 0; buf[i]; i++) {
        draw_font_char(d, px, py, buf[i], c);
        px += CELL_PX;
    }
}

static void rs_clear_line(Renderer *r, int row, int col) {
    RendererData *d = r->data;
    SDL_Rect rect = { col * CELL_PX, row * CELL_PX,
                      (d->cols - col) * CELL_PX, CELL_PX };
    SDL_SetRenderDrawColor(d->sdl, 0, 0, 0, 255);
    SDL_RenderFillRect(d->sdl, &rect);
}

static int rs_getkey(Renderer *r) {
    (void)r;
    SDL_Event e;
    /* Enable text input so we receive SDL_TEXTINPUT events */
    if (!SDL_IsTextInputActive()) SDL_StartTextInput();

    while (1) {
        if (!SDL_WaitEvent(&e)) return -1;

        if (e.type == SDL_QUIT) return 'q';

        if (e.type == SDL_KEYDOWN) {
            SDL_Keycode k = e.key.keysym.sym;

            /* Non-printable keys — handle before text input */
            if (k == SDLK_UP)    return 0x103; /* KEY_UP */
            if (k == SDLK_DOWN)  return 0x102; /* KEY_DOWN */
            if (k == SDLK_LEFT)  return 0x104; /* KEY_LEFT */
            if (k == SDLK_RIGHT) return 0x105; /* KEY_RIGHT */
            if (k == SDLK_RETURN || k == SDLK_KP_ENTER) return '\n';
            if (k == SDLK_ESCAPE) return 27;
            if (k == SDLK_BACKSPACE) return 0x107; /* KEY_BACKSPACE */
            if (k == SDLK_TAB) return '\t';

            /* For printable keys, prefer the SDL_TEXTINPUT event below
             * which handles shift/layout correctly. But if text input
             * is somehow not generating events, fall through. */
            continue;
        }

        /* SDL_TEXTINPUT gives us the actual character the user typed,
         * with correct shift, caps lock, and keyboard layout handling. */
        if (e.type == SDL_TEXTINPUT) {
            unsigned char ch = (unsigned char)e.text.text[0];
            if (ch >= 32 && ch < 127)
                return (int)ch;
        }

        if (e.type == SDL_WINDOWEVENT &&
            e.window.event == SDL_WINDOWEVENT_RESIZED) {
            /* Recalculate cell dimensions on next get_size */
        }
    }
}

static void rs_flush_input(Renderer *r) {
    (void)r;
    SDL_Event e;
    while (SDL_PollEvent(&e)) { /* discard */ }
}

static void rs_set_blocking(Renderer *r, bool blocking) {
    (void)r;
    (void)blocking;
    /* SDL getkey always blocks via SDL_WaitEvent;
     * non-blocking can be handled via SDL_PollEvent if needed later */
}

/* ------------------------------------------------------------------ */
/* Tile drawing                                                        */
/* ------------------------------------------------------------------ */
static void rs_draw_tile(Renderer *r, int x, int y,
                          const char *tile_id, short fallback_color,
                          char fallback_ch) {
    RendererData *d = r->data;
    const TileEntry *te = tileset_find(&d->tileset, tile_id);
    if (te) {
        SDL_Texture *sheet = load_sheet(d, te->sheet);
        if (sheet) {
            SDL_Rect src = { te->tx * TILE_PX, te->ty * TILE_PX,
                             TILE_PX, TILE_PX };
            SDL_Rect dst = { x * CELL_PX, y * CELL_PX, CELL_PX, CELL_PX };
            SDL_SetTextureColorMod(sheet, 255, 255, 255);
            SDL_RenderCopy(d->sdl, sheet, &src, &dst);
            return;
        }
    }
    /* Fallback to text glyph */
    rs_draw_char(r, x, y, fallback_ch, fallback_color, RATTR_NONE);
}

/* ------------------------------------------------------------------ */
/* High-level composites -- text-based for now, can be overridden      */
/* ------------------------------------------------------------------ */
static void rs_render_map(Renderer *r,
                           Tile *map, int map_w, int map_h,
                           Vec2 player_pos, int view_w, int view_h) {
    /* Simple text-based map rendering via draw_char */
    int cam_x = player_pos.x - view_w / 2;
    int cam_y = player_pos.y - view_h / 2;
    if (cam_x < 0) cam_x = 0;
    if (cam_y < 0) cam_y = 0;
    if (cam_x + view_w > map_w) cam_x = map_w - view_w;
    if (cam_y + view_h > map_h) cam_y = map_h - view_h;
    if (cam_x < 0) cam_x = 0;
    if (cam_y < 0) cam_y = 0;

    for (int vy = 0; vy < view_h; vy++) {
        for (int vx = 0; vx < view_w; vx++) {
            int mx = cam_x + vx;
            int my = cam_y + vy;
            if (mx < 0 || mx >= map_w || my < 0 || my >= map_h) continue;
            Tile *t = &map[my * map_w + mx];
            if (t->visible) {
                r->draw_char(r, vx, vy, t->glyph, t->color_pair, RATTR_NONE);
            } else if (t->revealed) {
                r->draw_char(r, vx, vy, t->glyph, t->color_pair, RATTR_DIM);
            }
        }
    }
}

static void rs_render_sidebar(Renderer *r, int col,
                               const char *name, int level,
                               int hp, int max_hp, int mp, int max_mp,
                               int str, int def, int intel, int spd,
                               int gold, int weight, int max_weight,
                               int encumbrance, int mounted, int chivalry,
                               int turn, int day, int hour, int minute) {
    /* Text-based sidebar */
    int row = 0;
    r->draw_text(r, row++, col, CP_WHITE_BOLD, RATTR_BOLD, "%s", name);
    r->draw_text(r, row++, col, CP_WHITE, RATTR_NONE, "Level %d", level);
    row++;
    r->draw_text(r, row++, col, CP_RED, RATTR_NONE, "HP:%d/%d", hp, max_hp);
    r->draw_text(r, row++, col, CP_CYAN, RATTR_NONE, "MP:%d/%d", mp, max_mp);
    row++;
    r->draw_text(r, row++, col, CP_WHITE, RATTR_NONE, "STR:%-3d DEF:%d", str, def);
    r->draw_text(r, row++, col, CP_WHITE, RATTR_NONE, "INT:%-3d SPD:%d", intel, spd);
    row++;
    r->draw_text(r, row++, col, CP_YELLOW, RATTR_NONE, "Gold: %d", gold);
    r->draw_text(r, row++, col, CP_WHITE, RATTR_NONE, "Wt: %d/%d", weight, max_weight);
    if (encumbrance) r->draw_text(r, row, col, CP_RED, RATTR_BOLD, "ENCUMBERED");
    row++;
    if (mounted) r->draw_text(r, row, col, CP_BROWN, RATTR_NONE, "Mounted");
    row++;
    r->draw_text(r, row++, col, CP_WHITE, RATTR_NONE, "Chiv: %d", chivalry);
    row++;
    r->draw_text(r, row++, col, CP_GRAY, RATTR_NONE, "T:%d D:%d", turn, day);
    r->draw_text(r, row++, col, CP_GRAY, RATTR_NONE, "%02d:%02d", hour, minute);
}

static void rs_render_log(Renderer *r, const MessageLog *log,
                           int start_row, int width, int lines) {
    (void)width;
    for (int i = 0; i < lines; i++) {
        const LogEntry *e = log_get(log, lines - 1 - i);
        if (e) {
            r->draw_text(r, start_row + i, 0, e->color_pair, RATTR_NONE,
                         "%s", e->text);
        }
    }
}

static void rs_render_status_bar(Renderer *r, int row, int width,
                                  const char *text) {
    r->draw_text(r, row, 0, CP_STATUS_BAR, RATTR_BOLD, "%-*s", width, text);
}

static void rs_render_minimap(Renderer *r,
                               Tile *map, int map_w, int map_h,
                               Vec2 player_pos, const char *locations_info) {
    /* Simplified minimap -- text-based overview */
    (void)locations_info;
    int rows, cols;
    r->get_size(r, &rows, &cols);
    r->clear(r);
    int scale = map_w / cols + 1;
    int row_scale = map_h / rows + 1;
    for (int sy = 0; sy < rows - 2; sy++) {
        for (int sx = 0; sx < cols; sx++) {
            int mx = sx * scale;
            int my = sy * row_scale;
            if (mx >= map_w || my >= map_h) continue;
            Tile *t = &map[my * map_w + mx];
            r->draw_char(r, sx, sy + 1, t->glyph, t->color_pair, RATTR_NONE);
        }
    }
    /* Player marker */
    int px = player_pos.x / scale;
    int py = player_pos.y / row_scale;
    r->draw_char(r, px, py + 1, '@', CP_WHITE_BOLD, RATTR_BOLD | RATTR_BLINK);
    r->refresh(r);
}

/* ------------------------------------------------------------------ */
/* Constructor                                                         */
/* ------------------------------------------------------------------ */
Renderer *render_sdl_create(const char *tiles_dir) {
    Renderer *r = calloc(1, sizeof(Renderer));
    if (!r) return NULL;

    RendererData *d = calloc(1, sizeof(RendererData));
    if (!d) { free(r); return NULL; }
    snprintf(d->tiles_dir, sizeof(d->tiles_dir), "%s", tiles_dir);
    r->data = d;

    r->init           = rs_init;
    r->shutdown       = rs_shutdown;
    r->get_size       = rs_get_size;
    r->clear          = rs_clear;
    r->refresh        = rs_refresh;
    r->draw_char      = rs_draw_char;
    r->draw_text      = rs_draw_text;
    r->clear_line     = rs_clear_line;
    r->getkey         = rs_getkey;
    r->flush_input    = rs_flush_input;
    r->set_blocking   = rs_set_blocking;
    r->render_map     = rs_render_map;
    r->render_sidebar = rs_render_sidebar;
    r->render_log     = rs_render_log;
    r->render_status_bar = rs_render_status_bar;
    r->render_minimap = rs_render_minimap;
    r->draw_tile      = rs_draw_tile;

    return r;
}

#endif /* HAS_SDL2 */
