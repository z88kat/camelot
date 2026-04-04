#include "ui.h"
#include <string.h>

static bool use_256_colors = false;

bool ui_init(void) {
    initscr();
    if (!has_colors()) {
        endwin();
        return false;
    }

    raw();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    start_color();

    /* Detect 256-color support */
    use_256_colors = (COLORS >= 256);

    /* Basic 8-color pairs */
    init_pair(CP_WHITE,       COLOR_WHITE,   COLOR_BLACK);
    init_pair(CP_RED,         COLOR_RED,     COLOR_BLACK);
    init_pair(CP_GREEN,       COLOR_GREEN,   COLOR_BLACK);
    init_pair(CP_YELLOW,      COLOR_YELLOW,  COLOR_BLACK);
    init_pair(CP_BLUE,        COLOR_CYAN,    COLOR_BLACK);
    init_pair(CP_MAGENTA,     COLOR_MAGENTA, COLOR_BLACK);
    init_pair(CP_CYAN,        COLOR_CYAN,    COLOR_BLACK);

    /* Brown approximation (yellow without bold) */
    init_pair(CP_BROWN,       COLOR_YELLOW,  COLOR_BLACK);

    /* Gray approximation */
    init_pair(CP_GRAY,        COLOR_WHITE,   COLOR_BLACK);

    /* Bold variants (same pairs, applied with A_BOLD at render time) */
    init_pair(CP_WHITE_BOLD,  COLOR_WHITE,   COLOR_BLACK);
    init_pair(CP_RED_BOLD,    COLOR_RED,     COLOR_BLACK);
    init_pair(CP_GREEN_BOLD,  COLOR_GREEN,   COLOR_BLACK);
    init_pair(CP_YELLOW_BOLD, COLOR_YELLOW,  COLOR_BLACK);
    init_pair(CP_BLUE_BOLD,   COLOR_BLUE,    COLOR_BLACK);
    init_pair(CP_MAGENTA_BOLD,COLOR_MAGENTA, COLOR_BLACK);
    init_pair(CP_CYAN_BOLD,   COLOR_CYAN,    COLOR_BLACK);

    /* UI color pairs */
    init_pair(CP_STATUS_BAR,  COLOR_WHITE,   COLOR_BLUE);
    init_pair(CP_LOG_DAMAGE,  COLOR_RED,     COLOR_BLACK);
    init_pair(CP_LOG_HEAL,    COLOR_GREEN,   COLOR_BLACK);
    init_pair(CP_LOG_QUEST,   COLOR_YELLOW,  COLOR_BLACK);
    init_pair(CP_LOG_LOOT,    COLOR_CYAN,    COLOR_BLACK);

    return true;
}

void ui_shutdown(void) {
    endwin();
}

void ui_get_size(int *rows, int *cols) {
    getmaxyx(stdscr, *rows, *cols);
}

void ui_render_map_generic(Tile *map, int map_w, int map_h,
                           Vec2 player_pos, int view_width, int view_height) {
    /* Calculate camera offset to keep player centered */
    int cam_x = player_pos.x - view_width / 2;
    int cam_y = player_pos.y - view_height / 2;

    /* Clamp camera to map bounds */
    if (cam_x < 0) cam_x = 0;
    if (cam_y < 0) cam_y = 0;
    if (cam_x + view_width > map_w) cam_x = map_w - view_width;
    if (cam_y + view_height > map_h) cam_y = map_h - view_height;
    if (cam_x < 0) cam_x = 0;
    if (cam_y < 0) cam_y = 0;

    for (int vy = 0; vy < view_height; vy++) {
        int my = cam_y + vy;
        for (int vx = 0; vx < view_width; vx++) {
            int mx = cam_x + vx;
            if (mx < 0 || mx >= map_w || my < 0 || my >= map_h) {
                mvaddch(vy, vx, ' ');
                continue;
            }

            Tile *t = &map[my * map_w + mx];
            if (t->visible) {
                attron(COLOR_PAIR(t->color_pair));
                mvaddch(vy, vx, t->glyph);
                attroff(COLOR_PAIR(t->color_pair));
            } else if (t->revealed) {
                attron(COLOR_PAIR(t->color_pair) | A_DIM);
                mvaddch(vy, vx, t->glyph);
                attroff(COLOR_PAIR(t->color_pair) | A_DIM);
            } else {
                mvaddch(vy, vx, ' ');
            }
        }
    }

    /* Draw player */
    int px = player_pos.x - cam_x;
    int py = player_pos.y - cam_y;
    if (px >= 0 && px < view_width && py >= 0 && py < view_height) {
        attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        mvaddch(py, px, '@');
        attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
    }
}

void ui_render_map(Tile map[MAP_HEIGHT][MAP_WIDTH], Vec2 player_pos,
                   int view_width, int view_height) {
    ui_render_map_generic((Tile *)map, MAP_WIDTH, MAP_HEIGHT,
                          player_pos, view_width, view_height);
}

void ui_render_sidebar(int col, const char *name, int level, int hp, int max_hp,
                       int mp, int max_mp, int str, int def, int intel, int spd,
                       int gold, int weight, int max_weight, int chivalry,
                       int turn, int day, int hour, int minute) {
    int row = 0;
    attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
    mvprintw(row++, col, " %-16s", name);
    attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);

    attron(COLOR_PAIR(CP_WHITE));
    mvprintw(row++, col, " Level %-10d", level);
    row++; /* blank line */

    /* HP */
    mvprintw(row++, col, " HP: %3d/%-3d", hp, max_hp);
    {
        int bar_len = 16;
        int hp_bars = (max_hp > 0) ? (hp * bar_len / max_hp) : 0;
        mvaddch(row, col, ' ');
        for (int i = 0; i < bar_len; i++) {
            if (i < hp_bars) {
                attron(COLOR_PAIR(CP_GREEN) | A_BOLD);
                mvaddch(row, col + 1 + i, '=');
                attroff(COLOR_PAIR(CP_GREEN) | A_BOLD);
            } else {
                attron(COLOR_PAIR(CP_RED));
                mvaddch(row, col + 1 + i, '-');
                attroff(COLOR_PAIR(CP_RED));
            }
        }
    }
    row++;

    /* MP */
    mvprintw(row++, col, " MP: %3d/%-3d", mp, max_mp);
    {
        int bar_len = 16;
        int mp_bars = (max_mp > 0) ? (mp * bar_len / max_mp) : 0;
        mvaddch(row, col, ' ');
        for (int i = 0; i < bar_len; i++) {
            if (i < mp_bars) {
                attron(COLOR_PAIR(CP_CYAN) | A_BOLD);
                mvaddch(row, col + 1 + i, '=');
                attroff(COLOR_PAIR(CP_CYAN) | A_BOLD);
            } else {
                attron(COLOR_PAIR(CP_BLUE));
                mvaddch(row, col + 1 + i, '-');
                attroff(COLOR_PAIR(CP_BLUE));
            }
        }
    }
    row++;

    row++; /* blank line */
    mvprintw(row++, col, " STR: %-3d", str);
    mvprintw(row++, col, " DEF: %-3d", def);
    mvprintw(row++, col, " INT: %-3d", intel);
    mvprintw(row++, col, " SPD: %-3d", spd);
    row++;
    mvprintw(row++, col, " Wt:  %d/%d", weight, max_weight);

    attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
    mvprintw(row++, col, " Gold: %-6d", gold);
    attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);

    mvprintw(row++, col, " Chiv: %-3d", chivalry);
    row++;
    mvprintw(row++, col, " Turn: %-6d", turn);
    mvprintw(row++, col, " Day %-2d %02d:%02d", day, hour, minute);
    attroff(COLOR_PAIR(CP_WHITE));
}

void ui_render_log(const MessageLog *log, int start_row, int width, int lines) {
    for (int i = 0; i < lines; i++) {
        int row = start_row + (lines - 1 - i);
        move(row, 0);
        clrtoeol();

        const LogEntry *entry = log_get(log, i);
        if (entry) {
            attron(COLOR_PAIR(entry->color_pair));
            mvprintw(row, 0, " %.*s", width - 2, entry->text);
            attroff(COLOR_PAIR(entry->color_pair));
        }
    }
}

void ui_render_status_bar(int row, int width, const char *status_text) {
    attron(COLOR_PAIR(CP_STATUS_BAR));
    move(row, 0);
    for (int i = 0; i < width; i++) addch(' ');
    mvprintw(row, 1, "%.*s", width - 2, status_text);
    attroff(COLOR_PAIR(CP_STATUS_BAR));
}

void ui_draw_char(int x, int y, char ch, short color_pair, bool bold) {
    if (bold) {
        attron(COLOR_PAIR(color_pair) | A_BOLD);
    } else {
        attron(COLOR_PAIR(color_pair));
    }
    mvaddch(y, x, ch);
    if (bold) {
        attroff(COLOR_PAIR(color_pair) | A_BOLD);
    } else {
        attroff(COLOR_PAIR(color_pair));
    }
}

void ui_clear(void) {
    clear();
}

void ui_refresh(void) {
    refresh();
}

int ui_getkey(void) {
    return getch();
}

void ui_render_minimap(Tile *map, int map_w, int map_h, Vec2 player_pos,
                       const char *locations_info) {
    int term_rows, term_cols;
    getmaxyx(stdscr, term_rows, term_cols);

    clear();

    /* Reserve rows: 1 for title, 1 for footer, rest for map */
    int avail_rows = term_rows - 3;
    int avail_cols = term_cols - 2;
    if (avail_rows < 5 || avail_cols < 10) {
        mvprintw(0, 0, "Terminal too small for minimap");
        refresh();
        return;
    }

    /* Calculate scale: how many map tiles per screen cell */
    int scale_x = (map_w + avail_cols - 1) / avail_cols;
    int scale_y = (map_h + avail_rows - 1) / avail_rows;
    if (scale_x < 1) scale_x = 1;
    if (scale_y < 1) scale_y = 1;

    int mini_w = map_w / scale_x;
    int mini_h = map_h / scale_y;
    if (mini_w > avail_cols) mini_w = avail_cols;
    if (mini_h > avail_rows) mini_h = avail_rows;

    /* Center the minimap */
    int off_x = (term_cols - mini_w) / 2;
    int off_y = 1 + (avail_rows - mini_h) / 2;

    /* Title */
    attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
    const char *title = (map_w > 200) ?
        "-- Map of England -- (press any key to close)" :
        "-- Dungeon Map -- (press any key to close)";
    mvprintw(0, (term_cols - (int)strlen(title)) / 2, "%s", title);
    attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);

    /* Render scaled map */
    for (int sy = 0; sy < mini_h; sy++) {
        for (int sx = 0; sx < mini_w; sx++) {
            /* Sample the center tile of this scaled block */
            int mx = sx * scale_x + scale_x / 2;
            int my = sy * scale_y + scale_y / 2;
            if (mx >= map_w) mx = map_w - 1;
            if (my >= map_h) my = map_h - 1;

            Tile *t = &map[my * map_w + mx];

            /* Determine what to draw: prioritise locations */
            char ch = t->glyph;
            short cp = t->color_pair;

            /* Check if any special glyph is in this block */
            for (int by = sy * scale_y; by < (sy + 1) * scale_y && by < map_h; by++) {
                for (int bx = sx * scale_x; bx < (sx + 1) * scale_x && bx < map_w; bx++) {
                    Tile *bt = &map[by * map_w + bx];
                    /* Prioritise location markers over terrain */
                    if (bt->glyph == '*' || bt->glyph == '#' || bt->glyph == '+'
                        || bt->glyph == '>' || bt->glyph == 'V' || bt->glyph == 'B') {
                        ch = bt->glyph;
                        cp = bt->color_pair;
                    }
                }
            }

            attron(COLOR_PAIR(cp));
            mvaddch(off_y + sy, off_x + sx, ch);
            attroff(COLOR_PAIR(cp));
        }
    }

    /* Draw player position */
    int px = player_pos.x / scale_x;
    int py = player_pos.y / scale_y;
    if (px >= 0 && px < mini_w && py >= 0 && py < mini_h) {
        attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD | A_BLINK);
        mvaddch(off_y + py, off_x + px, '@');
        attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD | A_BLINK);
    }

    /* Footer with location info */
    if (locations_info) {
        attron(COLOR_PAIR(CP_WHITE));
        mvprintw(term_rows - 1, 1, "%.*s", term_cols - 2, locations_info);
        attroff(COLOR_PAIR(CP_WHITE));
    }

    refresh();
}
