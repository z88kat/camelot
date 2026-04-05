#include "map.h"
#include "rng.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/* BSP tree for dungeon generation                                     */
/* ------------------------------------------------------------------ */

typedef struct BSPNode {
    int x, y, w, h;                  /* partition bounds */
    int room_x, room_y, room_w, room_h; /* carved room (0 if internal) */
    struct BSPNode *left, *right;
} BSPNode;

static BSPNode *bsp_create(int x, int y, int w, int h) {
    BSPNode *n = calloc(1, sizeof(BSPNode));
    n->x = x; n->y = y; n->w = w; n->h = h;
    return n;
}

static void bsp_free(BSPNode *n) {
    if (!n) return;
    bsp_free(n->left);
    bsp_free(n->right);
    free(n);
}

/* Recursively split the BSP tree */
static void bsp_split(BSPNode *n, int depth) {
    if (depth <= 0 || n->w < 12 || n->h < 10) return;

    /* Choose split direction: prefer splitting the longer axis */
    bool split_h;
    if (n->w > n->h * 1.5) split_h = false;      /* split vertically */
    else if (n->h > n->w * 1.5) split_h = true;   /* split horizontally */
    else split_h = rng_chance(50);

    if (split_h) {
        /* Horizontal split */
        int min_y = n->y + 5;
        int max_y = n->y + n->h - 5;
        if (min_y >= max_y) return;
        int split_y = rng_range(min_y, max_y);
        n->left  = bsp_create(n->x, n->y, n->w, split_y - n->y);
        n->right = bsp_create(n->x, split_y, n->w, n->y + n->h - split_y);
    } else {
        /* Vertical split */
        int min_x = n->x + 6;
        int max_x = n->x + n->w - 6;
        if (min_x >= max_x) return;
        int split_x = rng_range(min_x, max_x);
        n->left  = bsp_create(n->x, n->y, split_x - n->x, n->h);
        n->right = bsp_create(split_x, n->y, n->x + n->w - split_x, n->h);
    }

    bsp_split(n->left, depth - 1);
    bsp_split(n->right, depth - 1);
}

/* Carve a room in a leaf node -- clean rectangles with optional features */
static void bsp_carve_rooms(BSPNode *n, Tile tiles[MAP_HEIGHT][MAP_WIDTH]) {
    if (n->left || n->right) {
        if (n->left)  bsp_carve_rooms(n->left, tiles);
        if (n->right) bsp_carve_rooms(n->right, tiles);
        return;
    }

    /* Leaf node -- carve a rectangular room */
    int rw = rng_range(4, n->w - 2);
    int rh = rng_range(3, n->h - 2);
    if (rw > n->w - 2) rw = n->w - 2;
    if (rh > n->h - 2) rh = n->h - 2;
    if (rw < 3) rw = 3;
    if (rh < 3) rh = 3;

    int rx = n->x + rng_range(1, n->w - rw - 1);
    int ry = n->y + rng_range(1, n->h - rh - 1);

    n->room_x = rx; n->room_y = ry;
    n->room_w = rw; n->room_h = rh;

    /* Carve the rectangle */
    for (int y = ry; y < ry + rh; y++) {
        for (int x = rx; x < rx + rw; x++) {
            if (x > 0 && x < MAP_WIDTH - 1 && y > 0 && y < MAP_HEIGHT - 1) {
                tiles[y][x].type = TILE_FLOOR;
                tiles[y][x].glyph = '.';
                tiles[y][x].color_pair = CP_WHITE;
                tiles[y][x].passable = true;
                tiles[y][x].blocks_sight = false;
            }
        }
    }

    /* Optional pillars in larger rooms (20% chance, room must be 6x6+) */
    if (rw >= 6 && rh >= 6 && rng_chance(20)) {
        for (int y = ry + 2; y < ry + rh - 2; y += 3) {
            for (int x = rx + 2; x < rx + rw - 2; x += 3) {
                if (x > 0 && x < MAP_WIDTH - 1 && y > 0 && y < MAP_HEIGHT - 1) {
                    tiles[y][x].type = TILE_WALL;
                    tiles[y][x].glyph = 'O';
                    tiles[y][x].color_pair = CP_GRAY;
                    tiles[y][x].passable = false;
                    tiles[y][x].blocks_sight = true;
                }
            }
        }
    }
}

/* Get center of a subtree's room (for corridor connection) */
static void bsp_get_room_center(BSPNode *n, int *cx, int *cy) {
    if (!n->left && !n->right) {
        /* Leaf */
        *cx = n->room_x + n->room_w / 2;
        *cy = n->room_y + n->room_h / 2;
        return;
    }
    /* Internal: pick from left child */
    if (n->left) bsp_get_room_center(n->left, cx, cy);
    else if (n->right) bsp_get_room_center(n->right, cx, cy);
}

/* Carve a single tile as corridor floor */
static void carve_tile(Tile tiles[MAP_HEIGHT][MAP_WIDTH], int x, int y) {
    if (x > 0 && x < MAP_WIDTH - 1 && y > 0 && y < MAP_HEIGHT - 1) {
        Tile *t = &tiles[y][x];
        if (t->type == TILE_WALL) {
            t->type = TILE_FLOOR; t->glyph = '.';
            t->color_pair = CP_WHITE; t->passable = true;
            t->blocks_sight = false;
        }
    }
}

/* Carve a corridor between two points -- clean L-shaped */
static void carve_corridor(Tile tiles[MAP_HEIGHT][MAP_WIDTH], int x1, int y1, int x2, int y2) {
    int x = x1, y = y1;

    /* L-shaped: go horizontal first, then vertical (or vice versa) */
    if (rng_chance(50)) {
        while (x != x2) { carve_tile(tiles, x, y); x += (x2 > x) ? 1 : -1; }
        while (y != y2) { carve_tile(tiles, x, y); y += (y2 > y) ? 1 : -1; }
    } else {
        while (y != y2) { carve_tile(tiles, x, y); y += (y2 > y) ? 1 : -1; }
        while (x != x2) { carve_tile(tiles, x, y); x += (x2 > x) ? 1 : -1; }
    }
    carve_tile(tiles, x, y);
}

/* Connect sibling rooms with corridors */
static void bsp_connect(BSPNode *n, Tile tiles[MAP_HEIGHT][MAP_WIDTH]) {
    if (!n->left || !n->right) return;

    bsp_connect(n->left, tiles);
    bsp_connect(n->right, tiles);

    int lx, ly, rx, ry;
    bsp_get_room_center(n->left, &lx, &ly);
    bsp_get_room_center(n->right, &rx, &ry);

    carve_corridor(tiles, lx, ly, rx, ry);
}

/* Count floor tiles in a 3x3 area around a point (excluding center) */
static int count_nearby_floors(Tile tiles[MAP_HEIGHT][MAP_WIDTH], int cx, int cy) {
    int count = 0;
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int nx = cx + dx, ny = cy + dy;
            if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT &&
                tiles[ny][nx].type == TILE_FLOOR) count++;
        }
    return count;
}

/* Place doors at corridor-room junctions only.
   A valid doorway: narrow passage (walls on 2 opposite sides, floor on 2 opposite sides)
   AND one side leads to a room (wide open area) while the other is corridor-like. */
static void place_doors(Tile tiles[MAP_HEIGHT][MAP_WIDTH]) {
    for (int y = 3; y < MAP_HEIGHT - 3; y++) {
        for (int x = 3; x < MAP_WIDTH - 3; x++) {
            if (tiles[y][x].type != TILE_FLOOR) continue;

            bool wall_n = (tiles[y-1][x].type == TILE_WALL);
            bool wall_s = (tiles[y+1][x].type == TILE_WALL);
            bool wall_e = (tiles[y][x+1].type == TILE_WALL);
            bool wall_w = (tiles[y][x-1].type == TILE_WALL);
            bool floor_n = (tiles[y-1][x].type == TILE_FLOOR);
            bool floor_s = (tiles[y+1][x].type == TILE_FLOOR);
            bool floor_e = (tiles[y][x+1].type == TILE_FLOOR);
            bool floor_w = (tiles[y][x-1].type == TILE_FLOOR);

            bool is_doorway = false;

            /* Horizontal passage: walls N+S, floor E+W */
            if (wall_n && wall_s && floor_e && floor_w) {
                /* Check that one side is a room (open area) and other is corridor */
                int open_e = count_nearby_floors(tiles, x+1, y);
                int open_w = count_nearby_floors(tiles, x-1, y);
                /* A room side has many open neighbours (5+), corridor has few (2-3) */
                if ((open_e >= 5) != (open_w >= 5)) {
                    is_doorway = true;
                }
            }
            /* Vertical passage: walls E+W, floor N+S */
            else if (wall_e && wall_w && floor_n && floor_s) {
                int open_n = count_nearby_floors(tiles, x, y-1);
                int open_s = count_nearby_floors(tiles, x, y+1);
                if ((open_n >= 5) != (open_s >= 5)) {
                    is_doorway = true;
                }
            }

            if (is_doorway && rng_chance(60)) {
                /* Make sure no adjacent door already exists */
                bool adj_door = false;
                for (int d = 0; d < 4; d++) {
                    int nx = x + dir_dx[d*2], ny = y + dir_dy[d*2];
                    if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT &&
                        tiles[ny][nx].type == TILE_DOOR_CLOSED) {
                        adj_door = true;
                        break;
                    }
                }
                if (!adj_door) {
                    /* 20% chance the door is locked */
                    if (rng_chance(20)) {
                        tiles[y][x].type = TILE_DOOR_LOCKED;
                        tiles[y][x].glyph = '+';
                        tiles[y][x].color_pair = CP_RED;
                        tiles[y][x].passable = false;
                        tiles[y][x].blocks_sight = true;
                    } else {
                        tiles[y][x].type = TILE_DOOR_CLOSED;
                        tiles[y][x].glyph = '+';
                        tiles[y][x].color_pair = CP_BROWN;
                        tiles[y][x].passable = false;
                        tiles[y][x].blocks_sight = true;
                    }
                }
            }
        }
    }
}

/* Place traps on floor tiles */
static void place_traps(DungeonLevel *level) {
    int num = rng_range(3, 6 + level->depth);
    if (num > MAX_TRAPS) num = MAX_TRAPS;

    level->num_traps = 0;
    for (int i = 0; i < num; i++) {
        for (int tries = 0; tries < 100; tries++) {
            int x = rng_range(2, MAP_WIDTH - 3);
            int y = rng_range(2, MAP_HEIGHT - 3);
            if (level->tiles[y][x].type != TILE_FLOOR) continue;
            /* Don't place on stairs */
            bool on_stairs = false;
            for (int si = 0; si < level->num_stairs_up; si++)
                if (x == level->stairs_up[si].x && y == level->stairs_up[si].y) on_stairs = true;
            for (int si = 0; si < level->num_stairs_down; si++)
                if (x == level->stairs_down[si].x && y == level->stairs_down[si].y) on_stairs = true;
            if (on_stairs) continue;

            Trap *t = &level->traps[level->num_traps++];
            t->pos = (Vec2){ x, y };
            t->type = rng_range(TRAP_PIT, TRAP_MANA_DRAIN);
            t->revealed = false;
            t->triggered = false;
            break;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

void map_generate(DungeonLevel *level, int depth, int max_depth) {
    memset(level, 0, sizeof(*level));
    level->depth = depth;
    level->generated = true;

    /* Fill with walls */
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            Tile *t = &level->tiles[y][x];
            t->type = TILE_WALL;
            t->glyph = '#';
            t->color_pair = CP_GRAY;
            t->passable = false;
            t->blocks_sight = true;
            t->visible = false;
            t->revealed = false;
        }
    }

    /* BSP generate rooms */
    BSPNode *root = bsp_create(1, 1, MAP_WIDTH - 2, MAP_HEIGHT - 2);
    /* Scale BSP depth by dungeon depth -- deeper levels have more rooms */
    int bsp_depth = 5 + (depth >= 5 ? 1 : 0) + (depth >= 10 ? 1 : 0);
    bsp_split(root, bsp_depth);
    bsp_carve_rooms(root, level->tiles);
    bsp_connect(root, level->tiles);

    /* Place doors */
    place_doors(level->tiles);

    /* Find a random floor tile far from a given point */
    #define FIND_FLOOR(out_x, out_y, avoid_x, avoid_y, min_dist) do { \
        for (int _tries = 0; _tries < 500; _tries++) { \
            int _rx = rng_range(3, MAP_WIDTH - 4); \
            int _ry = rng_range(3, MAP_HEIGHT - 4); \
            if (level->tiles[_ry][_rx].type != TILE_FLOOR) continue; \
            int _dx = _rx - (avoid_x), _dy = _ry - (avoid_y); \
            if (_dx*_dx + _dy*_dy >= (min_dist)*(min_dist)) { \
                (out_x) = _rx; (out_y) = _ry; break; \
            } \
        } \
    } while(0)

    /* Helper to place a stair tile */
    #define PLACE_STAIR(x, y, type_val, glyph_val, cp_val) do { \
        Tile *_st = &level->tiles[(y)][(x)]; \
        _st->type = (type_val); _st->glyph = (glyph_val); \
        _st->color_pair = (cp_val); _st->passable = true; \
        _st->blocks_sight = false; \
    } while(0)

    /* Randomise which subtree gets stairs up vs down */
    BSPNode *up_tree = rng_chance(50) ? root->left : root->right;
    BSPNode *down_tree = (up_tree == root->left) ? root->right : root->left;
    if (!up_tree) up_tree = root;
    if (!down_tree) down_tree = root;

    /* Place stairs up */
    {
        int cx, cy;
        bsp_get_room_center(up_tree, &cx, &cy);

        level->stairs_up[0] = (Vec2){ cx, cy };
        PLACE_STAIR(cx, cy, TILE_STAIRS_UP, '<', CP_WHITE);
        level->num_stairs_up = 1;

        /* Second set of stairs up from level 1 onwards */
        if (depth > 0) {
            int sx = cx, sy = cy;
            FIND_FLOOR(sx, sy, cx, cy, 20);
            level->stairs_up[1] = (Vec2){ sx, sy };
            PLACE_STAIR(sx, sy, TILE_STAIRS_UP, '<', CP_WHITE);
            level->num_stairs_up = 2;
        }
    }

    /* Place stairs down (or portal on deepest level) */
    {
        int cx, cy;
        bsp_get_room_center(down_tree, &cx, &cy);

        level->stairs_down[0] = (Vec2){ cx, cy };
        level->num_stairs_down = 1;

        if (depth >= max_depth - 1) {
            /* Deepest level: exit portal */
            PLACE_STAIR(cx, cy, TILE_PORTAL, '0', CP_CYAN_BOLD);
        } else {
            PLACE_STAIR(cx, cy, TILE_STAIRS_DOWN, '>', CP_WHITE);

            /* Second set of stairs down from level 1 onwards */
            if (depth > 0) {
                int sx = cx, sy = cy;
                FIND_FLOOR(sx, sy, cx, cy, 20);
                level->stairs_down[1] = (Vec2){ sx, sy };
                PLACE_STAIR(sx, sy, TILE_STAIRS_DOWN, '>', CP_WHITE);
                level->num_stairs_down = 2;
            }
        }
    }

    #undef FIND_FLOOR
    #undef PLACE_STAIR

    /* Special room decorations -- find rooms and add features */
    /* Scan for room centers (floor tiles with many open neighbours) and decorate some */
    {
        int decorated = 0;
        for (int tries = 0; tries < 300 && decorated < 3; tries++) {
            int rx = rng_range(5, MAP_WIDTH - 6);
            int ry = rng_range(5, MAP_HEIGHT - 6);
            if (level->tiles[ry][rx].type != TILE_FLOOR) continue;
            if (level->tiles[ry][rx].glyph != '.') continue;

            /* Check it's in a room (many floor neighbours) */
            int open = 0;
            for (int dy = -2; dy <= 2; dy++)
                for (int dx = -2; dx <= 2; dx++) {
                    int cx = rx + dx, cy = ry + dy;
                    if (cx > 0 && cx < MAP_WIDTH - 1 && cy > 0 && cy < MAP_HEIGHT - 1 &&
                        level->tiles[cy][cx].type == TILE_FLOOR) open++;
                }
            if (open < 16) continue;  /* needs to be in a real room */

            int room_type = rng_range(0, 3);

            if (room_type == 0) {
                /* Temple -- altar in center, candle glyphs */
                level->tiles[ry][rx].glyph = '_';
                level->tiles[ry][rx].color_pair = CP_YELLOW_BOLD;
                /* Candles around altar */
                int candle_pos[][2] = {{-1,-1},{1,-1},{-1,1},{1,1}};
                for (int c = 0; c < 4; c++) {
                    int cx = rx + candle_pos[c][0];
                    int cy = ry + candle_pos[c][1];
                    if (cx > 0 && cx < MAP_WIDTH-1 && cy > 0 && cy < MAP_HEIGHT-1 &&
                        level->tiles[cy][cx].type == TILE_FLOOR &&
                        level->tiles[cy][cx].glyph == '.') {
                        level->tiles[cy][cx].glyph = '*';
                        level->tiles[cy][cx].color_pair = CP_YELLOW;
                    }
                }
                decorated++;
            } else if (room_type == 1) {
                /* Library -- bookshelves along walls */
                for (int dy = -3; dy <= 3; dy++)
                    for (int dx = -3; dx <= 3; dx++) {
                        int bx = rx + dx, by = ry + dy;
                        if (bx < 1 || bx >= MAP_WIDTH-1 || by < 1 || by >= MAP_HEIGHT-1) continue;
                        if (level->tiles[by][bx].type != TILE_FLOOR) continue;
                        /* Place bookshelves against walls */
                        bool adj_wall = false;
                        for (int d = 0; d < 4; d++) {
                            int wx = bx + dir_dx[d*2], wy = by + dir_dy[d*2];
                            if (wx >= 0 && wx < MAP_WIDTH && wy >= 0 && wy < MAP_HEIGHT &&
                                level->tiles[wy][wx].type == TILE_WALL) adj_wall = true;
                        }
                        if (adj_wall && rng_chance(60)) {
                            level->tiles[by][bx].glyph = '|';
                            level->tiles[by][bx].color_pair = CP_BROWN;
                        }
                    }
                /* Reading desk in center */
                level->tiles[ry][rx].glyph = '=';
                level->tiles[ry][rx].color_pair = CP_BROWN;
                decorated++;
            } else if (room_type == 2) {
                /* Crypt -- coffins in rows */
                for (int row = -2; row <= 2; row += 2) {
                    for (int col = -2; col <= 2; col += 2) {
                        int cx = rx + col, cy = ry + row;
                        if (cx > 0 && cx < MAP_WIDTH-1 && cy > 0 && cy < MAP_HEIGHT-1 &&
                            level->tiles[cy][cx].type == TILE_FLOOR &&
                            level->tiles[cy][cx].glyph == '.') {
                            level->tiles[cy][cx].glyph = '-';
                            level->tiles[cy][cx].color_pair = CP_GRAY;
                        }
                    }
                }
                decorated++;
            } else {
                /* Treasure room -- gold piles and a chest */
                level->tiles[ry][rx].glyph = '=';
                level->tiles[ry][rx].color_pair = CP_YELLOW_BOLD;
                /* Gold piles around it */
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        int gx = rx + dx, gy = ry + dy;
                        if (gx > 0 && gx < MAP_WIDTH-1 && gy > 0 && gy < MAP_HEIGHT-1 &&
                            level->tiles[gy][gx].type == TILE_FLOOR &&
                            level->tiles[gy][gx].glyph == '.' && rng_chance(50)) {
                            level->tiles[gy][gx].glyph = '$';
                            level->tiles[gy][gx].color_pair = CP_YELLOW;
                        }
                    }
                decorated++;
            }
        }
    }

    /* Flood some rooms with shallow water (~20% of floor tiles in affected rooms) */
    {
        /* Pick 1-3 random spots and flood a radius around them */
        int num_pools = rng_range(1, 3);
        for (int p = 0; p < num_pools; p++) {
            /* Find a random floor tile */
            for (int tries = 0; tries < 200; tries++) {
                int px = rng_range(5, MAP_WIDTH - 6);
                int py = rng_range(5, MAP_HEIGHT - 6);
                if (level->tiles[py][px].type != TILE_FLOOR) continue;

                /* Flood a patch around this point */
                int rad = rng_range(2, 5);
                for (int fy = py - rad; fy <= py + rad; fy++) {
                    for (int fx = px - rad; fx <= px + rad; fx++) {
                        if (fx < 1 || fx >= MAP_WIDTH - 1 || fy < 1 || fy >= MAP_HEIGHT - 1)
                            continue;
                        if (level->tiles[fy][fx].type != TILE_FLOOR) continue;

                        double dx = (double)(fx - px) / rad;
                        double dy = (double)(fy - py) / rad;
                        double dist = dx * dx + dy * dy;
                        /* Irregular edges */
                        double noise = (((fx * 13 + fy * 29) & 0xFF) - 128) / 300.0;
                        if (dist + noise < 0.8) {
                            /* Don't flood stairs */
                            bool is_stair = false;
                            for (int si = 0; si < level->num_stairs_up; si++)
                                if (fx == level->stairs_up[si].x && fy == level->stairs_up[si].y)
                                    is_stair = true;
                            for (int si = 0; si < level->num_stairs_down; si++)
                                if (fx == level->stairs_down[si].x && fy == level->stairs_down[si].y)
                                    is_stair = true;
                            if (!is_stair) {
                                level->tiles[fy][fx].glyph = '~';
                                level->tiles[fy][fx].color_pair = CP_BLUE;
                                /* Still passable -- it's shallow water */
                            }
                        }
                    }
                }
                break;
            }
        }
    }

    /* Occasionally place lava pools (deeper levels) or ice patches */
    if (depth >= 3 && rng_chance(40)) {
        /* Lava pool */
        for (int tries = 0; tries < 100; tries++) {
            int lx = rng_range(5, MAP_WIDTH - 6);
            int ly = rng_range(5, MAP_HEIGHT - 6);
            if (level->tiles[ly][lx].type != TILE_FLOOR) continue;

            int rad = rng_range(1, 3);
            for (int fy = ly - rad; fy <= ly + rad; fy++) {
                for (int fx = lx - rad; fx <= lx + rad; fx++) {
                    if (fx < 1 || fx >= MAP_WIDTH - 1 || fy < 1 || fy >= MAP_HEIGHT - 1) continue;
                    if (level->tiles[fy][fx].type != TILE_FLOOR) continue;
                    double dx = (double)(fx - lx) / rad;
                    double dy = (double)(fy - ly) / rad;
                    if (dx*dx + dy*dy < 0.9) {
                        level->tiles[fy][fx].glyph = '^';
                        level->tiles[fy][fx].color_pair = CP_RED_BOLD;
                        /* Still passable but will hurt */
                    }
                }
            }
            break;
        }
    }

    if (rng_chance(25)) {
        /* Ice patch */
        for (int tries = 0; tries < 100; tries++) {
            int ix = rng_range(5, MAP_WIDTH - 6);
            int iy = rng_range(5, MAP_HEIGHT - 6);
            if (level->tiles[iy][ix].type != TILE_FLOOR) continue;

            int rad = rng_range(2, 4);
            for (int fy = iy - rad; fy <= iy + rad; fy++) {
                for (int fx = ix - rad; fx <= ix + rad; fx++) {
                    if (fx < 1 || fx >= MAP_WIDTH - 1 || fy < 1 || fy >= MAP_HEIGHT - 1) continue;
                    if (level->tiles[fy][fx].type != TILE_FLOOR) continue;
                    if (level->tiles[fy][fx].glyph != '.') continue;
                    double dx = (double)(fx - ix) / rad;
                    double dy = (double)(fy - iy) / rad;
                    if (dx*dx + dy*dy < 0.8) {
                        level->tiles[fy][fx].glyph = '_';
                        level->tiles[fy][fx].color_pair = CP_CYAN;
                    }
                }
            }
            break;
        }
    }

    /* Scatter 1-3 standalone chests in rooms */
    {
        int num_chests = rng_range(1, 3);
        for (int c = 0; c < num_chests; c++) {
            for (int tries = 0; tries < 200; tries++) {
                int cx = rng_range(3, MAP_WIDTH - 4);
                int cy = rng_range(3, MAP_HEIGHT - 4);
                if (level->tiles[cy][cx].type != TILE_FLOOR) continue;
                if (level->tiles[cy][cx].glyph != '.') continue;
                /* Make sure it's in a room not a corridor */
                int open = 0;
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++) {
                        int nx = cx + dx, ny = cy + dy;
                        if (nx > 0 && nx < MAP_WIDTH-1 && ny > 0 && ny < MAP_HEIGHT-1 &&
                            level->tiles[ny][nx].type == TILE_FLOOR) open++;
                    }
                if (open < 5) continue;

                level->tiles[cy][cx].glyph = '=';
                level->tiles[cy][cx].color_pair = CP_BROWN;
                break;
            }
        }
    }

    /* Secret rooms -- hidden rooms with no corridor, accessed via secret doors or teleport */
    /* ~30% chance per level, more likely on deeper levels */
    if (rng_chance(30 + depth * 5)) {
        /* Find a spot in solid rock (all walls, no adjacent floor) */
        for (int tries = 0; tries < 500; tries++) {
            int sr_w = rng_range(4, 7);
            int sr_h = rng_range(3, 5);
            int sr_x = rng_range(3, MAP_WIDTH - sr_w - 3);
            int sr_y = rng_range(3, MAP_HEIGHT - sr_h - 3);

            /* Check the area is all solid wall */
            bool all_wall = true;
            for (int cy = sr_y - 1; cy <= sr_y + sr_h && all_wall; cy++)
                for (int cx = sr_x - 1; cx <= sr_x + sr_w && all_wall; cx++) {
                    if (cx < 0 || cx >= MAP_WIDTH || cy < 0 || cy >= MAP_HEIGHT) continue;
                    if (level->tiles[cy][cx].type != TILE_WALL) all_wall = false;
                }
            if (!all_wall) continue;

            /* Carve the secret room */
            for (int cy = sr_y; cy < sr_y + sr_h; cy++)
                for (int cx = sr_x; cx < sr_x + sr_w; cx++) {
                    Tile *t = &level->tiles[cy][cx];
                    t->type = TILE_FLOOR;
                    t->glyph = '.';
                    t->color_pair = CP_YELLOW;  /* golden floor to hint it's special */
                    t->passable = true;
                    t->blocks_sight = false;
                }

            /* Place treasure in the secret room */
            int tcx = sr_x + sr_w / 2;
            int tcy = sr_y + sr_h / 2;
            level->tiles[tcy][tcx].glyph = '=';
            level->tiles[tcy][tcx].color_pair = CP_YELLOW_BOLD;

            /* Place a secret door on one wall adjacent to existing corridors/rooms.
               Scan the room border for a wall tile that has floor on the outside. */
            bool door_placed = false;
            /* Try all four walls of the room */
            int walls[4][3] = {
                { sr_x - 1,         sr_y + sr_h / 2, 0 },  /* left wall */
                { sr_x + sr_w,      sr_y + sr_h / 2, 0 },  /* right wall */
                { sr_x + sr_w / 2,  sr_y - 1,        0 },  /* top wall */
                { sr_x + sr_w / 2,  sr_y + sr_h,     0 },  /* bottom wall */
            };
            for (int w = 0; w < 4 && !door_placed; w++) {
                int wx = walls[w][0], wy = walls[w][1];
                if (wx < 1 || wx >= MAP_WIDTH - 1 || wy < 1 || wy >= MAP_HEIGHT - 1) continue;

                /* Check if outside this wall point is near an existing corridor/room */
                int outside_x = wx + (w == 0 ? -1 : w == 1 ? 1 : 0);
                int outside_y = wy + (w == 2 ? -1 : w == 3 ? 1 : 0);
                if (outside_x < 1 || outside_x >= MAP_WIDTH - 1 ||
                    outside_y < 1 || outside_y >= MAP_HEIGHT - 1) continue;

                /* Search outward for a floor tile (up to 5 tiles away) */
                bool found_floor __attribute__((unused)) = false;
                int dx = (w == 0) ? -1 : (w == 1) ? 1 : 0;
                int dy = (w == 2) ? -1 : (w == 3) ? 1 : 0;
                for (int dist = 1; dist <= 5; dist++) {
                    int fx = wx + dx * dist;
                    int fy = wy + dy * dist;
                    if (fx < 1 || fx >= MAP_WIDTH - 1 || fy < 1 || fy >= MAP_HEIGHT - 1) break;
                    if (level->tiles[fy][fx].type == TILE_FLOOR &&
                        level->tiles[fy][fx].color_pair != CP_YELLOW) {
                        /* Carve a short passage from secret door to existing floor */
                        for (int p = 0; p < dist; p++) {
                            int px = wx + dx * p;
                            int py = wy + dy * p;
                            level->tiles[py][px].type = TILE_WALL;
                            level->tiles[py][px].glyph = '#';
                            level->tiles[py][px].color_pair = CP_GRAY;
                            level->tiles[py][px].passable = false;
                        }
                        /* The first tile (at the room border) is the secret door */
                        /* It looks like a wall but is actually a closed door */
                        level->tiles[wy][wx].type = TILE_WALL;  /* looks like wall */
                        level->tiles[wy][wx].glyph = '#';
                        level->tiles[wy][wx].color_pair = CP_GRAY;
                        level->tiles[wy][wx].passable = false;
                        /* The 's' search command can reveal it as a door */

                        found_floor = true;
                        door_placed = true;
                        break;
                    }
                }
            }

            if (door_placed) {
                /* Add a flavour hint -- golden floor visible only from inside */
                break;  /* one secret room per level */
            } else {
                /* Couldn't connect -- undo the room (teleport can still reach it) */
                /* Leave the room carved -- teleport traps and magic circles can reach it */
                break;
            }
        }
    }

    /* Fungal growth patches -- poison risk */
    if (rng_chance(35)) {
        for (int tries = 0; tries < 100; tries++) {
            int fx = rng_range(5, MAP_WIDTH - 6);
            int fy = rng_range(5, MAP_HEIGHT - 6);
            if (level->tiles[fy][fx].type != TILE_FLOOR) continue;
            if (level->tiles[fy][fx].glyph != '.') continue;

            int rad = rng_range(2, 4);
            for (int dy = -rad; dy <= rad; dy++)
                for (int dx = -rad; dx <= rad; dx++) {
                    int gx = fx + dx, gy = fy + dy;
                    if (gx < 1 || gx >= MAP_WIDTH-1 || gy < 1 || gy >= MAP_HEIGHT-1) continue;
                    if (level->tiles[gy][gx].type != TILE_FLOOR) continue;
                    if (level->tiles[gy][gx].glyph != '.') continue;
                    if (dx*dx + dy*dy > rad*rad) continue;
                    if (rng_chance(60)) {
                        level->tiles[gy][gx].glyph = '"';
                        level->tiles[gy][gx].color_pair = CP_GREEN;
                    }
                }
            break;
        }
    }

    /* Rubble patches -- impassable, blocks corridors */
    if (rng_chance(30)) {
        for (int tries = 0; tries < 100; tries++) {
            int rx = rng_range(5, MAP_WIDTH - 6);
            int ry = rng_range(5, MAP_HEIGHT - 6);
            if (level->tiles[ry][rx].type != TILE_FLOOR) continue;
            if (level->tiles[ry][rx].glyph != '.') continue;

            /* Place a small cluster of rubble */
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++) {
                    int bx = rx + dx, by = ry + dy;
                    if (bx < 1 || bx >= MAP_WIDTH-1 || by < 1 || by >= MAP_HEIGHT-1) continue;
                    if (level->tiles[by][bx].type != TILE_FLOOR) continue;
                    if (level->tiles[by][bx].glyph != '.') continue;
                    if (rng_chance(50)) {
                        level->tiles[by][bx].glyph = '%';
                        level->tiles[by][bx].color_pair = CP_BROWN;
                        level->tiles[by][bx].passable = false;
                    }
                }
            break;
        }
    }

    /* Crystal formations -- glowing, visual interest */
    if (rng_chance(20)) {
        for (int tries = 0; tries < 100; tries++) {
            int cx = rng_range(5, MAP_WIDTH - 6);
            int cy = rng_range(5, MAP_HEIGHT - 6);
            if (level->tiles[cy][cx].type != TILE_FLOOR) continue;
            if (level->tiles[cy][cx].glyph != '.') continue;

            /* Small cluster of crystals */
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++) {
                    int kx = cx + dx, ky = cy + dy;
                    if (kx < 1 || kx >= MAP_WIDTH-1 || ky < 1 || ky >= MAP_HEIGHT-1) continue;
                    if (level->tiles[ky][kx].type != TILE_FLOOR) continue;
                    if (level->tiles[ky][kx].glyph != '.') continue;
                    if (rng_chance(40)) {
                        level->tiles[ky][kx].glyph = '*';
                        level->tiles[ky][kx].color_pair = CP_CYAN_BOLD;
                    }
                }
            break;
        }
    }

    /* Deep water pool -- impassable */
    if (rng_chance(20)) {
        for (int tries = 0; tries < 100; tries++) {
            int wx = rng_range(5, MAP_WIDTH - 6);
            int wy = rng_range(5, MAP_HEIGHT - 6);
            if (level->tiles[wy][wx].type != TILE_FLOOR) continue;
            if (level->tiles[wy][wx].glyph != '.') continue;

            int rad = rng_range(1, 2);
            for (int dy = -rad; dy <= rad; dy++)
                for (int dx = -rad; dx <= rad; dx++) {
                    int px = wx + dx, py = wy + dy;
                    if (px < 1 || px >= MAP_WIDTH-1 || py < 1 || py >= MAP_HEIGHT-1) continue;
                    if (level->tiles[py][px].type != TILE_FLOOR) continue;
                    if (level->tiles[py][px].glyph != '.') continue;
                    if (dx*dx + dy*dy <= rad*rad) {
                        level->tiles[py][px].glyph = '~';
                        level->tiles[py][px].color_pair = CP_BLUE;
                        level->tiles[py][px].passable = false;  /* deep water -- can't walk */
                    }
                }
            break;
        }
    }

    /* Place 1-2 magic circles per level */
    {
        int num_circles = rng_range(1, 2);
        for (int c = 0; c < num_circles; c++) {
            for (int tries = 0; tries < 200; tries++) {
                int cx = rng_range(5, MAP_WIDTH - 6);
                int cy = rng_range(5, MAP_HEIGHT - 6);
                if (level->tiles[cy][cx].type != TILE_FLOOR) continue;
                if (level->tiles[cy][cx].glyph != '.') continue;  /* not water/rubble */

                /* Pick a color for the circle */
                short colors[] = { CP_WHITE, CP_BLUE, CP_YELLOW, CP_RED,
                                   CP_GREEN, CP_CYAN, CP_MAGENTA };
                short cp = colors[rng_range(0, 6)];

                level->tiles[cy][cx].glyph = '(';
                level->tiles[cy][cx].color_pair = cp;
                /* Still passable -- effect triggers on step */
                break;
            }
        }
    }

    /* Hide solid rock -- only show walls adjacent to open space */
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (level->tiles[y][x].type != TILE_WALL) continue;

            /* Check if any adjacent tile (8-dir) is non-wall */
            bool near_open = false;
            for (int dy = -1; dy <= 1 && !near_open; dy++) {
                for (int dx = -1; dx <= 1 && !near_open; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = x + dx, ny = y + dy;
                    if (nx < 0 || nx >= MAP_WIDTH || ny < 0 || ny >= MAP_HEIGHT) continue;
                    TileType tt = level->tiles[ny][nx].type;
                    if (tt != TILE_WALL && tt != TILE_NONE) {
                        near_open = true;
                    }
                }
            }

            if (!near_open) {
                /* Solid rock -- show as empty space */
                level->tiles[y][x].glyph = ' ';
                level->tiles[y][x].color_pair = CP_DEFAULT;
            }
        }
    }

    /* Maze region on deeper levels (depth >= 4) -- fill one area with a maze */
    if (depth >= 4 && rng_chance(40 + depth * 5)) {
        /* Find a rectangular area of solid wall to carve a maze into */
        int mw = rng_range(15, 25);
        int mh = rng_range(10, 18);
        if (mw % 2 == 0) mw++;  /* odd dimensions for maze */
        if (mh % 2 == 0) mh++;
        int mx = rng_range(5, MAP_WIDTH - mw - 5);
        int my = rng_range(5, MAP_HEIGHT - mh - 5);

        /* DFS maze carver */
        /* Check maze doesn't overlap stairs */
        bool overlaps_stairs = false;
        for (int si = 0; si < level->num_stairs_up; si++) {
            Vec2 su = level->stairs_up[si];
            if (su.x >= mx && su.x < mx + mw && su.y >= my && su.y < my + mh)
                overlaps_stairs = true;
        }
        for (int si = 0; si < level->num_stairs_down; si++) {
            Vec2 sd = level->stairs_down[si];
            if (sd.x >= mx && sd.x < mx + mw && sd.y >= my && sd.y < my + mh)
                overlaps_stairs = true;
        }
        if (overlaps_stairs) goto skip_maze;

        /* Initialize maze area as walls */
        for (int cy = my; cy < my + mh; cy++)
            for (int cx = mx; cx < mx + mw; cx++) {
                if (cx > 0 && cx < MAP_WIDTH - 1 && cy > 0 && cy < MAP_HEIGHT - 1) {
                    level->tiles[cy][cx].type = TILE_WALL;
                    level->tiles[cy][cx].glyph = '#';
                    level->tiles[cy][cx].color_pair = CP_GRAY;
                    level->tiles[cy][cx].passable = false;
                    level->tiles[cy][cx].blocks_sight = true;
                }
            }

        /* Carve maze passages using DFS */
        typedef struct { int x, y; } MPos;
        MPos stack[500];
        int sp = 0;
        int sx = mx + 1, sy = my + 1;
        level->tiles[sy][sx].type = TILE_FLOOR;
        level->tiles[sy][sx].glyph = '.';
        level->tiles[sy][sx].color_pair = CP_WHITE;
        level->tiles[sy][sx].passable = true;
        level->tiles[sy][sx].blocks_sight = false;
        stack[sp++] = (MPos){ sx, sy };

        while (sp > 0) {
            MPos cur = stack[sp - 1];
            /* Check 4 directions (step 2 tiles at a time for maze) */
            int dirs[4][2] = {{0,-2},{0,2},{-2,0},{2,0}};
            /* Shuffle directions */
            for (int i = 3; i > 0; i--) {
                int j = rng_range(0, i);
                int tx = dirs[i][0]; int ty = dirs[i][1];
                dirs[i][0] = dirs[j][0]; dirs[i][1] = dirs[j][1];
                dirs[j][0] = tx; dirs[j][1] = ty;
            }

            bool found = false;
            for (int d = 0; d < 4; d++) {
                int nx = cur.x + dirs[d][0];
                int ny = cur.y + dirs[d][1];
                int wx = cur.x + dirs[d][0] / 2;
                int wy = cur.y + dirs[d][1] / 2;
                if (nx <= mx || nx >= mx + mw - 1 || ny <= my || ny >= my + mh - 1) continue;
                if (level->tiles[ny][nx].passable) continue;

                /* Carve passage */
                level->tiles[wy][wx].type = TILE_FLOOR;
                level->tiles[wy][wx].glyph = '.';
                level->tiles[wy][wx].color_pair = CP_WHITE;
                level->tiles[wy][wx].passable = true;
                level->tiles[wy][wx].blocks_sight = false;
                level->tiles[ny][nx].type = TILE_FLOOR;
                level->tiles[ny][nx].glyph = '.';
                level->tiles[ny][nx].color_pair = CP_WHITE;
                level->tiles[ny][nx].passable = true;
                level->tiles[ny][nx].blocks_sight = false;

                if (sp < 500) stack[sp++] = (MPos){ nx, ny };
                found = true;
                break;
            }
            if (!found) sp--;
        }

        /* Connect maze to nearest room */
        for (int side = 0; side < 4; side++) {
            int bx, by, bdx, bdy;
            switch (side) {
            case 0: bx = mx + mw / 2; by = my;        bdx = 0; bdy = -1; break;
            case 1: bx = mx + mw / 2; by = my + mh -1; bdx = 0; bdy = 1; break;
            case 2: bx = mx;          by = my + mh / 2; bdx = -1; bdy = 0; break;
            default: bx = mx + mw - 1; by = my + mh / 2; bdx = 1; bdy = 0; break;
            }
            /* Walk outward to find existing floor */
            for (int step = 0; step < 10; step++) {
                int fx = bx + bdx * step, fy = by + bdy * step;
                if (fx < 1 || fx >= MAP_WIDTH - 1 || fy < 1 || fy >= MAP_HEIGHT - 1) break;
                if (level->tiles[fy][fx].type == TILE_FLOOR &&
                    level->tiles[fy][fx].color_pair != CP_WHITE) {
                    /* Found existing room -- carve connection */
                    for (int s = 0; s <= step; s++) {
                        int cx2 = bx + bdx * s, cy2 = by + bdy * s;
                        level->tiles[cy2][cx2].type = TILE_FLOOR;
                        level->tiles[cy2][cx2].glyph = '.';
                        level->tiles[cy2][cx2].color_pair = CP_WHITE;
                        level->tiles[cy2][cx2].passable = true;
                        level->tiles[cy2][cx2].blocks_sight = false;
                    }
                    break;
                }
            }
        }
    }
    skip_maze: ;

    /* Re-place stairs in case any decorations/hazards/maze overwrote them */
    for (int si = 0; si < level->num_stairs_up; si++) {
        Vec2 su = level->stairs_up[si];
        Tile *st = &level->tiles[su.y][su.x];
        st->type = TILE_STAIRS_UP; st->glyph = '<'; st->color_pair = CP_WHITE;
        st->passable = true; st->blocks_sight = false;
    }
    for (int si = 0; si < level->num_stairs_down; si++) {
        Vec2 sd = level->stairs_down[si];
        Tile *st = &level->tiles[sd.y][sd.x];
        if (depth >= max_depth - 1) {
            st->type = TILE_PORTAL; st->glyph = '0'; st->color_pair = CP_CYAN_BOLD;
        } else {
            st->type = TILE_STAIRS_DOWN; st->glyph = '>'; st->color_pair = CP_WHITE;
        }
        st->passable = true; st->blocks_sight = false;
    }

    /* Reachability validation: ensure stairs are connected */
    {
        /* Flood fill from stairs_up[0] */
        static bool visited[MAP_HEIGHT][MAP_WIDTH];
        memset(visited, 0, sizeof(visited));

        typedef struct { int x, y; } FPos;
        static FPos queue[MAP_WIDTH * MAP_HEIGHT / 4];
        int qhead = 0, qtail = 0;

        int sx = level->stairs_up[0].x, sy = level->stairs_up[0].y;
        visited[sy][sx] = true;
        queue[qtail++] = (FPos){ sx, sy };

        while (qhead < qtail) {
            FPos cur = queue[qhead++];
            for (int d = 0; d < 4; d++) {
                int nx = cur.x + dir_dx[d * 2];
                int ny = cur.y + dir_dy[d * 2];
                if (nx < 0 || nx >= MAP_WIDTH || ny < 0 || ny >= MAP_HEIGHT) continue;
                if (visited[ny][nx]) continue;
                if (!level->tiles[ny][nx].passable) continue;
                visited[ny][nx] = true;
                if (qtail < MAP_WIDTH * MAP_HEIGHT / 4)
                    queue[qtail++] = (FPos){ nx, ny };
            }
        }

        /* Check if stairs down and all stairs up are reachable */
        for (int si = 0; si < level->num_stairs_down; si++) {
            int dx = level->stairs_down[si].x, dy = level->stairs_down[si].y;
            if (!visited[dy][dx]) {
                /* Carve a proper L-shaped corridor from stairs_up[0] to
                   unreachable stairs_down (same style as BSP corridors) */
                carve_corridor(level->tiles, sx, sy, dx, dy);
            }
        }
    }

    /* Re-place stairs AGAIN after reachability carving (it can overwrite them) */
    for (int si = 0; si < level->num_stairs_up; si++) {
        Vec2 su = level->stairs_up[si];
        Tile *st = &level->tiles[su.y][su.x];
        st->type = TILE_STAIRS_UP; st->glyph = '<'; st->color_pair = CP_WHITE;
        st->passable = true; st->blocks_sight = false;
    }
    for (int si = 0; si < level->num_stairs_down; si++) {
        Vec2 sd = level->stairs_down[si];
        Tile *st = &level->tiles[sd.y][sd.x];
        if (depth >= max_depth - 1) {
            st->type = TILE_PORTAL; st->glyph = '0'; st->color_pair = CP_CYAN_BOLD;
        } else {
            st->type = TILE_STAIRS_DOWN; st->glyph = '>'; st->color_pair = CP_WHITE;
        }
        st->passable = true; st->blocks_sight = false;
    }

    /* Place traps */
    place_traps(level);

    bsp_free(root);
}

Dungeon *dungeon_create(const char *name, int num_levels) {
    Dungeon *d = calloc(1, sizeof(Dungeon));
    snprintf(d->name, MAX_NAME, "%s", name);
    d->max_depth = num_levels;
    d->current_level = 0;
    d->levels = calloc(num_levels, sizeof(DungeonLevel));
    d->has_portal = true;
    return d;
}

void dungeon_free(Dungeon *d) {
    if (!d) return;
    free(d->levels);
    free(d);
}
