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

    /* Helper to set a floor tile */
    #define SET_FLOOR(x, y) do { \
        if ((x) > 0 && (x) < MAP_WIDTH - 1 && (y) > 0 && (y) < MAP_HEIGHT - 1) { \
            tiles[(y)][(x)].type = TILE_FLOOR; \
            tiles[(y)][(x)].glyph = '.'; \
            tiles[(y)][(x)].color_pair = CP_WHITE; \
            tiles[(y)][(x)].passable = true; \
            tiles[(y)][(x)].blocks_sight = false; \
        } \
    } while(0)

    #define SET_PILLAR(x, y) do { \
        if ((x) > 0 && (x) < MAP_WIDTH - 1 && (y) > 0 && (y) < MAP_HEIGHT - 1) { \
            tiles[(y)][(x)].type = TILE_WALL; \
            tiles[(y)][(x)].glyph = 'O'; \
            tiles[(y)][(x)].color_pair = CP_GRAY; \
            tiles[(y)][(x)].passable = false; \
            tiles[(y)][(x)].blocks_sight = true; \
        } \
    } while(0)

    /* Choose room shape */
    int shape = rng_range(0, 9);

    switch (shape) {
    case 0:
    case 1:
        /* Standard rectangle (most common) */
        for (int y = ry; y < ry + rh; y++)
            for (int x = rx; x < rx + rw; x++)
                SET_FLOOR(x, y);
        break;

    case 2: {
        /* Rectangle with pillars */
        for (int y = ry; y < ry + rh; y++)
            for (int x = rx; x < rx + rw; x++)
                SET_FLOOR(x, y);
        /* Place pillars in a grid pattern */
        for (int y = ry + 2; y < ry + rh - 2; y += 3)
            for (int x = rx + 2; x < rx + rw - 2; x += 3)
                SET_PILLAR(x, y);
        break;
    }

    case 3: {
        /* Circular / cavern room */
        int cx_r = rx + rw / 2;
        int cy_r = ry + rh / 2;
        int rad_x = rw / 2;
        int rad_y = rh / 2;
        for (int y = ry; y < ry + rh; y++) {
            for (int x = rx; x < rx + rw; x++) {
                double dx = (double)(x - cx_r) / rad_x;
                double dy = (double)(y - cy_r) / rad_y;
                /* Add noise for natural cave edges */
                double noise = ((rng_range(0, 100) - 50) / 500.0);
                if (dx * dx + dy * dy + noise < 1.0) {
                    SET_FLOOR(x, y);
                }
            }
        }
        break;
    }

    case 4: {
        /* Cross-shaped room */
        int mid_x = rx + rw / 2;
        int mid_y = ry + rh / 2;
        int arm_w = rw / 3;
        int arm_h = rh / 3;
        if (arm_w < 2) arm_w = 2;
        if (arm_h < 2) arm_h = 2;
        /* Horizontal bar */
        for (int y = mid_y - arm_h; y <= mid_y + arm_h; y++)
            for (int x = rx; x < rx + rw; x++)
                SET_FLOOR(x, y);
        /* Vertical bar */
        for (int y = ry; y < ry + rh; y++)
            for (int x = mid_x - arm_w; x <= mid_x + arm_w; x++)
                SET_FLOOR(x, y);
        break;
    }

    case 5: {
        /* L-shaped room */
        /* Horizontal part */
        for (int y = ry; y < ry + rh / 2; y++)
            for (int x = rx; x < rx + rw; x++)
                SET_FLOOR(x, y);
        /* Vertical part (left or right side) */
        if (rng_chance(50)) {
            for (int y = ry + rh / 2; y < ry + rh; y++)
                for (int x = rx; x < rx + rw / 2; x++)
                    SET_FLOOR(x, y);
        } else {
            for (int y = ry + rh / 2; y < ry + rh; y++)
                for (int x = rx + rw / 2; x < rx + rw; x++)
                    SET_FLOOR(x, y);
        }
        break;
    }

    case 6: {
        /* Large cavern with rough edges and rubble */
        int cx_r = rx + rw / 2;
        int cy_r = ry + rh / 2;
        for (int y = ry; y < ry + rh; y++) {
            for (int x = rx; x < rx + rw; x++) {
                double dx = (double)(x - cx_r) / (rw / 2);
                double dy = (double)(y - cy_r) / (rh / 2);
                double dist = dx * dx + dy * dy;
                /* Rough noise at edges */
                int h = ((x * 31 + y * 17) ^ (x * 7 + y * 53)) & 0xFF;
                double noise = (h - 128) / 400.0;
                if (dist + noise < 0.9) {
                    SET_FLOOR(x, y);
                } else if (dist + noise < 1.05 && rng_chance(30)) {
                    /* Scattered rocks at cave edge */
                    if (x > 0 && x < MAP_WIDTH - 1 && y > 0 && y < MAP_HEIGHT - 1) {
                        tiles[y][x].type = TILE_FLOOR;
                        tiles[y][x].glyph = ',';
                        tiles[y][x].color_pair = CP_GRAY;
                        tiles[y][x].passable = true;
                        tiles[y][x].blocks_sight = false;
                    }
                }
            }
        }
        /* A few stalagmites */
        for (int i = 0; i < 3; i++) {
            int px = rx + rng_range(2, rw - 3);
            int py = ry + rng_range(2, rh - 3);
            if (tiles[py][px].type == TILE_FLOOR) {
                SET_PILLAR(px, py);
                tiles[py][px].glyph = '^';
                tiles[py][px].color_pair = CP_BROWN;
            }
        }
        break;
    }

    case 7: {
        /* Jagged cavern -- very irregular shape with bites taken out */
        for (int y = ry; y < ry + rh; y++) {
            /* Each row has a random left/right indent for jagged walls */
            int indent_l = rng_range(0, 3);
            int indent_r = rng_range(0, 3);
            for (int x = rx + indent_l; x < rx + rw - indent_r; x++) {
                /* Additional random holes in the edges */
                if ((y == ry || y == ry + rh - 1) && rng_chance(40)) continue;
                SET_FLOOR(x, y);
            }
        }
        /* Scatter rubble and rocks */
        for (int i = 0; i < rw * rh / 8; i++) {
            int px = rx + rng_range(1, rw - 2);
            int py = ry + rng_range(1, rh - 2);
            if (tiles[py][px].type == TILE_FLOOR && rng_chance(30)) {
                tiles[py][px].glyph = ',';
                tiles[py][px].color_pair = CP_GRAY;
            }
        }
        break;
    }

    case 8: {
        /* Multi-chamber -- 2-3 overlapping circles */
        int num_blobs = rng_range(2, 3);
        for (int b = 0; b < num_blobs; b++) {
            int bcx = rx + rng_range(rw / 4, 3 * rw / 4);
            int bcy = ry + rng_range(rh / 4, 3 * rh / 4);
            int brad_x = rng_range(rw / 5, rw / 3);
            int brad_y = rng_range(rh / 5, rh / 3);
            if (brad_x < 2) brad_x = 2;
            if (brad_y < 2) brad_y = 2;
            for (int y = bcy - brad_y; y <= bcy + brad_y; y++) {
                for (int x = bcx - brad_x; x <= bcx + brad_x; x++) {
                    double ddx = (double)(x - bcx) / brad_x;
                    double ddy = (double)(y - bcy) / brad_y;
                    double noise = ((((x * 17 + y * 31) ^ 0x5A) & 0xFF) - 128) / 500.0;
                    if (ddx * ddx + ddy * ddy + noise < 1.0) {
                        SET_FLOOR(x, y);
                    }
                }
            }
        }
        break;
    }

    case 9: {
        /* Pillared temple -- rectangular with many pillars and a clear center */
        for (int y = ry; y < ry + rh; y++)
            for (int x = rx; x < rx + rw; x++)
                SET_FLOOR(x, y);
        /* Ring of pillars around the edge */
        for (int y = ry + 1; y < ry + rh - 1; y += 2)
            SET_PILLAR(rx + 1, y);
        for (int y = ry + 1; y < ry + rh - 1; y += 2)
            SET_PILLAR(rx + rw - 2, y);
        for (int x = rx + 1; x < rx + rw - 1; x += 2)
            SET_PILLAR(x, ry + 1);
        for (int x = rx + 1; x < rx + rw - 1; x += 2)
            SET_PILLAR(x, ry + rh - 2);
        /* Altar or feature in center */
        {
            int acx = rx + rw / 2, acy = ry + rh / 2;
            if (acx > 0 && acx < MAP_WIDTH - 1 && acy > 0 && acy < MAP_HEIGHT - 1) {
                tiles[acy][acx].glyph = '_';
                tiles[acy][acx].color_pair = CP_YELLOW;
            }
        }
        break;
    }
    }

    #undef SET_FLOOR
    #undef SET_PILLAR
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

/* Carve a corridor between two points -- multiple styles */
static void carve_corridor(Tile tiles[MAP_HEIGHT][MAP_WIDTH], int x1, int y1, int x2, int y2) {
    int style = rng_range(0, 2);

    int x = x1, y = y1;
    int dx = (x2 > x1) ? 1 : (x2 < x1) ? -1 : 0;
    int dy = (y2 > y1) ? 1 : (y2 < y1) ? -1 : 0;

    switch (style) {
    case 0: {
        /* L-shaped: horizontal then vertical (or vice versa) */
        if (rng_chance(50)) {
            while (x != x2) { carve_tile(tiles, x, y); x += dx; }
            while (y != y2) { carve_tile(tiles, x, y); y += dy; }
        } else {
            while (y != y2) { carve_tile(tiles, x, y); y += dy; }
            while (x != x2) { carve_tile(tiles, x, y); x += dx; }
        }
        carve_tile(tiles, x, y);
        break;
    }

    case 1: {
        /* Winding/twisting -- moves toward target but wanders randomly */
        int steps = 0;
        int max_steps = abs(x2 - x1) + abs(y2 - y1) + 50;
        while ((x != x2 || y != y2) && steps < max_steps) {
            carve_tile(tiles, x, y);

            /* 70% move toward target, 30% wander perpendicular */
            if (rng_chance(70)) {
                /* Move toward target on the longer axis */
                if (abs(x2 - x) >= abs(y2 - y)) {
                    x += (x2 > x) ? 1 : -1;
                } else {
                    y += (y2 > y) ? 1 : -1;
                }
            } else {
                /* Wander perpendicular */
                if (abs(x2 - x) >= abs(y2 - y)) {
                    y += rng_chance(50) ? 1 : -1;
                } else {
                    x += rng_chance(50) ? 1 : -1;
                }
            }
            /* Clamp to bounds */
            if (x < 1) x = 1;
            if (y < 1) y = 1;
            if (x >= MAP_WIDTH - 1) x = MAP_WIDTH - 2;
            if (y >= MAP_HEIGHT - 1) y = MAP_HEIGHT - 2;
            steps++;
        }
        carve_tile(tiles, x, y);
        break;
    }

    case 2: {
        /* S-curve / Z-shaped -- horizontal, then vertical, then horizontal again */
        int mid_y = (y1 + y2) / 2 + rng_range(-3, 3);
        /* Go horizontal to midpoint x */
        int mid_x = (x1 + x2) / 2;
        while (x != mid_x) { carve_tile(tiles, x, y); x += (mid_x > x) ? 1 : -1; }
        /* Go vertical to mid_y */
        while (y != mid_y) { carve_tile(tiles, x, y); y += (mid_y > y) ? 1 : -1; }
        /* Go horizontal some more */
        while (x != x2) { carve_tile(tiles, x, y); x += (x2 > x) ? 1 : -1; }
        /* Go vertical to target */
        while (y != y2) { carve_tile(tiles, x, y); y += (y2 > y) ? 1 : -1; }
        carve_tile(tiles, x, y);
        break;
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
