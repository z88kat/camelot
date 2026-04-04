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

/* Carve a room in a leaf node */
static void bsp_carve_rooms(BSPNode *n, Tile tiles[MAP_HEIGHT][MAP_WIDTH]) {
    if (n->left || n->right) {
        if (n->left)  bsp_carve_rooms(n->left, tiles);
        if (n->right) bsp_carve_rooms(n->right, tiles);
        return;
    }

    /* Leaf node -- carve a room */
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

/* Carve a corridor between two points */
static void carve_corridor(Tile tiles[MAP_HEIGHT][MAP_WIDTH], int x1, int y1, int x2, int y2) {
    int x = x1, y = y1;

    /* L-shaped corridor: go horizontal first, then vertical (or vice versa) */
    bool horiz_first = rng_chance(50);

    if (horiz_first) {
        while (x != x2) {
            if (x > 0 && x < MAP_WIDTH - 1 && y > 0 && y < MAP_HEIGHT - 1) {
                Tile *t = &tiles[y][x];
                if (t->type == TILE_WALL) {
                    t->type = TILE_FLOOR; t->glyph = '.';
                    t->color_pair = CP_WHITE; t->passable = true;
                    t->blocks_sight = false;
                }
            }
            x += (x2 > x) ? 1 : -1;
        }
        while (y != y2) {
            if (x > 0 && x < MAP_WIDTH - 1 && y > 0 && y < MAP_HEIGHT - 1) {
                Tile *t = &tiles[y][x];
                if (t->type == TILE_WALL) {
                    t->type = TILE_FLOOR; t->glyph = '.';
                    t->color_pair = CP_WHITE; t->passable = true;
                    t->blocks_sight = false;
                }
            }
            y += (y2 > y) ? 1 : -1;
        }
    } else {
        while (y != y2) {
            if (x > 0 && x < MAP_WIDTH - 1 && y > 0 && y < MAP_HEIGHT - 1) {
                Tile *t = &tiles[y][x];
                if (t->type == TILE_WALL) {
                    t->type = TILE_FLOOR; t->glyph = '.';
                    t->color_pair = CP_WHITE; t->passable = true;
                    t->blocks_sight = false;
                }
            }
            y += (y2 > y) ? 1 : -1;
        }
        while (x != x2) {
            if (x > 0 && x < MAP_WIDTH - 1 && y > 0 && y < MAP_HEIGHT - 1) {
                Tile *t = &tiles[y][x];
                if (t->type == TILE_WALL) {
                    t->type = TILE_FLOOR; t->glyph = '.';
                    t->color_pair = CP_WHITE; t->passable = true;
                    t->blocks_sight = false;
                }
            }
            x += (x2 > x) ? 1 : -1;
        }
    }
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
            t->visible = true;
            t->revealed = true;
        }
    }

    /* BSP generate rooms */
    BSPNode *root = bsp_create(1, 1, MAP_WIDTH - 2, MAP_HEIGHT - 2);
    bsp_split(root, 6);  /* more splits for larger map = more rooms */
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

    /* Place stairs up -- first set in left subtree */
    {
        int cx, cy;
        if (root->left) bsp_get_room_center(root->left, &cx, &cy);
        else bsp_get_room_center(root, &cx, &cy);

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
        if (root->right) bsp_get_room_center(root->right, &cx, &cy);
        else {
            cx = MAP_WIDTH / 2; cy = MAP_HEIGHT / 2;
            bsp_get_room_center(root, &cx, &cy);
        }

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
