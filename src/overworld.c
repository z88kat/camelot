#include "overworld.h"
#include "rng.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ------------------------------------------------------------------ */
/* Tile helpers                                                        */
/* ------------------------------------------------------------------ */

static void set_tile(Tile *t, TileType type, char glyph, short color, bool pass) {
    t->type = type;
    t->glyph = glyph;
    t->color_pair = color;
    t->passable = pass;
    t->blocks_sight = false;
    t->visible = true;
    t->revealed = true;
}

static void set_sea(Tile *t)    { set_tile(t, TILE_WATER, '~', CP_BLUE, false); }
static void set_grass(Tile *t)  { set_tile(t, TILE_GRASS, '"', CP_GREEN, true); }
static void set_hills(Tile *t)  { set_tile(t, TILE_HILLS, '^', CP_WHITE, true); }
static void set_mountain(Tile *t) { set_tile(t, TILE_MOUNTAIN, 'A', CP_WHITE_BOLD, false); }
static void set_forest(Tile *t) { set_tile(t, TILE_FOREST, 'T', CP_GREEN_BOLD, true); }
static void set_road(Tile *t)   { set_tile(t, TILE_ROAD, '.', CP_YELLOW, true); }
static void set_marsh(Tile *t)  { set_tile(t, TILE_MARSH, ',', CP_GREEN, true); }
static void set_swamp(Tile *t)  { set_tile(t, TILE_SWAMP, '%', CP_GREEN, true); }
static void set_river(Tile *t)  { set_tile(t, TILE_RIVER, '=', CP_BLUE, false); }
static void set_lake(Tile *t)   { set_tile(t, TILE_LAKE, 'o', CP_BLUE, false); }
static void set_bridge(Tile *t) { set_tile(t, TILE_BRIDGE, 'H', CP_BROWN, true); }

/* ------------------------------------------------------------------ */
/* Coastline of Great Britain                                          */
/* Defined as (left_x, right_x) land bounds per row.                   */
/* -1,-1 means all sea.                                                */
/* Coordinates designed for 400 wide x 200 tall map.                   */
/* Britain runs roughly from y=10 (north Scotland) to y=190 (Cornwall) */
/* Oriented: x increases east, y increases south.                      */
/* ------------------------------------------------------------------ */

typedef struct { int left, right; } Span;

/* Linearly interpolate between two values */
static int lerp(int a, int b, int t, int tmax) {
    if (tmax <= 0) return a;
    return a + (b - a) * t / tmax;
}

/* Build the coastline spans for Great Britain */
static void build_coastline(Span spans[OW_HEIGHT]) {
    for (int y = 0; y < OW_HEIGHT; y++)
        spans[y] = (Span){ -1, -1 };

    /* Define key coastline control points: (y, left_x, right_x) */
    /* Going from north (Scotland) to south (Cornwall/Kent) */
    typedef struct { int y, lx, rx; } CP;
    CP pts[] = {
        /* Northern Scotland - narrow, rugged */
        {  12,  175, 244 },
        {  19,  156, 262 },
        {  25,  138, 288 },
        {  31,  131, 300 },
        /* Scottish Highlands - wider */
        {  38,  125, 312 },
        {  44,  119, 319 },
        {  50,  125, 312 },
        /* Scottish Lowlands */
        {  56,  131, 325 },
        {  62,  138, 331 },
        /* Border region - narrower (Solway/Tyne) */
        {  69,  144, 338 },
        {  75,  138, 344 },
        /* Northern England - Lake District west, Yorkshire east */
        {  81,  131, 356 },
        {  88,  125, 362 },
        {  94,  125, 369 },
        /* Lancashire / Yorkshire */
        { 100,  119, 369 },
        { 106,  119, 372 },
        /* Wales begins - bulge west */
        { 112,  100, 375 },
        { 119,   88, 375 },
        { 125,   81, 381 },
        /* Mid Wales - widest western extent */
        { 131,   75, 381 },
        { 138,   75, 388 },
        { 144,   81, 388 },
        /* South Wales / Bristol Channel indentation */
        { 150,   88, 394 },
        { 156,  106, 394 },  /* Bristol Channel narrows */
        /* Southwest England */
        { 162,  100, 400 },
        { 169,   94, 406 },
        { 175,   88, 412 },
        { 181,   81, 419 },
        /* Devon / Dorset */
        { 188,   75, 419 },
        { 194,   69, 425 },
        { 200,   69, 431 },
        /* South coast / Hampshire / Sussex */
        { 206,   75, 438 },
        { 212,   88, 444 },
        /* Kent bulge southeast + Cornwall peninsula southwest */
        { 219,   62, 450 },
        { 225,   50, 456 },
        /* Cornwall narrows */
        { 231,   44, 438 },
        { 235,   38, 375 },
        { 240,   35, 250 },
        /* Cornwall tip */
        { 245,   38, 125 },
    };
    int npts = sizeof(pts) / sizeof(pts[0]);

    /* Interpolate between control points */
    for (int i = 0; i < npts - 1; i++) {
        int y0 = pts[i].y, y1 = pts[i+1].y;
        for (int y = y0; y <= y1; y++) {
            int t = y - y0, tmax = y1 - y0;
            spans[y].left  = lerp(pts[i].lx, pts[i+1].lx, t, tmax);
            spans[y].right = lerp(pts[i].rx, pts[i+1].rx, t, tmax);
        }
    }
}

/* ------------------------------------------------------------------ */
/* Terrain generation                                                  */
/* ------------------------------------------------------------------ */

/* Simple noise function for terrain variety */
static int hash2d(int x, int y) {
    int h = x * 374761393 + y * 668265263;
    h = (h ^ (h >> 13)) * 1274126177;
    return h ^ (h >> 16);
}

static double noise2d(int x, int y, int scale) {
    int gx = x / scale, gy = y / scale;
    double fx = (double)(x % scale) / scale;
    double fy = (double)(y % scale) / scale;

    int h00 = hash2d(gx, gy) & 0xFF;
    int h10 = hash2d(gx+1, gy) & 0xFF;
    int h01 = hash2d(gx, gy+1) & 0xFF;
    int h11 = hash2d(gx+1, gy+1) & 0xFF;

    double top = h00 + (h10 - h00) * fx;
    double bot = h01 + (h11 - h01) * fx;
    return (top + (bot - top) * fy) / 255.0;
}

/* Determine terrain type based on geographic region and noise */
static void assign_terrain(Tile *t, int x, int y) {
    double n1 = noise2d(x, y, 12);
    double n2 = noise2d(x + 1000, y + 1000, 20);

    /* Scottish Highlands (y < 62) -- mountainous with forest in glens */
    if (y < 62) {
        /* Central Highlands spine -- impassable mountains */
        int highland_cx = lerp(160, 200, y - 12, 50);
        int dist_from_spine = abs(x - highland_cx);
        if (dist_from_spine < 20 && n1 > 0.50) { set_mountain(t); return; }
        if (dist_from_spine < 30 && n1 > 0.60) { set_hills(t); return; }
        if (n1 > 0.70) { set_hills(t); return; }
        if (n1 > 0.45) { set_forest(t); return; }
        set_grass(t);
        return;
    }

    /* Northern England (y 62-106) -- Pennine hills, Lake District */
    if (y < 106) {
        /* Pennines: central spine of hills with occasional mountain peaks */
        int center_x = lerp(238, 250, y - 62, 44);
        int dist_from_pennines = abs(x - center_x);
        if (dist_from_pennines < 8 && n1 > 0.60) { set_mountain(t); return; }
        if (dist_from_pennines < 19 && n1 > 0.35) { set_hills(t); return; }

        /* Lake District (west side, y 75-97) */
        if (y >= 75 && y <= 97 && x >= 131 && x <= 175) {
            if (n1 > 0.7) { set_lake(t); return; }
            if (n1 > 0.5) { set_hills(t); return; }
        }

        if (n1 > 0.7) { set_forest(t); return; }
        set_grass(t);
        return;
    }

    /* Wales (y 110-156, x < 188) -- mountainous with Snowdonia */
    if (y >= 110 && y <= 156 && x < 188) {
        /* Snowdonia region (north Wales, y 112-130, x 85-120) -- mountain peaks */
        if (y >= 112 && y <= 132 && x >= 85 && x <= 125 && n1 > 0.50) { set_mountain(t); return; }
        if (n1 > 0.60) { set_hills(t); return; }
        if (n1 > 0.40 && n2 > 0.5) { set_forest(t); return; }
        set_grass(t);
        return;
    }

    /* Sherwood Forest area (y 106-125, x 250-325) */
    if (y >= 106 && y <= 125 && x >= 250 && x <= 325) {
        if (n1 > 0.35) { set_forest(t); return; }
    }

    /* The Fens -- marshland (y 125-147, x 325-388) */
    if (y >= 125 && y <= 147 && x >= 325 && x <= 388) {
        if (n1 > 0.55) { set_marsh(t); return; }
    }

    /* Somerset Levels -- marsh/swamp (y 162-185, x 119-181) */
    if (y >= 162 && y <= 185 && x >= 119 && x <= 181) {
        if (n1 > 0.6) { set_swamp(t); return; }
        if (n1 > 0.45) { set_marsh(t); return; }
    }

    /* Devon/Cornwall -- moorland (y 194+, x < 138) */
    if (y >= 194 && x < 138) {
        if (n1 > 0.6) { set_hills(t); return; }
        if (n1 > 0.45 && n2 > 0.5) { set_marsh(t); return; }
    }

    /* Southern Downs -- gentle hills (y 194-215, x 250-400) */
    if (y >= 194 && y <= 215 && x >= 250 && x <= 400) {
        if (n1 > 0.6) { set_hills(t); return; }
    }

    /* Dense woods patches -- impassable without Ranger/axe */
    if (n1 > 0.82 && n2 > 0.7) {
        set_tile(t, TILE_DENSE_WOODS, '&', CP_GREEN_BOLD, false);
        t->blocks_sight = true;
        return;
    }

    /* General scattered forest */
    if (n1 > 0.72 && n2 > 0.5) { set_forest(t); return; }

    set_grass(t);
}

/* Draw a road between two points using simple pathfinding */
static void draw_road(Overworld *ow, int x0, int y0, int x1, int y1) {
    int x = x0, y = y0;
    while (x != x1 || y != y1) {
        if (x >= 0 && x < OW_WIDTH && y >= 0 && y < OW_HEIGHT) {
            Tile *t = &ow->map[y][x];
            if (t->type != TILE_WATER && t->type != TILE_LAKE &&
                t->type != TILE_RIVER) {
                set_road(t);
            } else if (t->type == TILE_RIVER) {
                set_bridge(t);
            }
        }
        /* Move toward target, preferring horizontal then vertical */
        int dx = (x1 > x) ? 1 : (x1 < x) ? -1 : 0;
        int dy = (y1 > y) ? 1 : (y1 < y) ? -1 : 0;

        /* Alternate x/y movement for diagonal-ish paths */
        int adx = abs(x1 - x), ady = abs(y1 - y);
        if (adx >= ady) {
            x += dx;
            /* Occasionally step vertically too for natural look */
            if (ady > 0 && (hash2d(x, y) & 3) == 0) y += dy;
        } else {
            y += dy;
            if (adx > 0 && (hash2d(x, y) & 3) == 0) x += dx;
        }
    }
    /* Set endpoint */
    if (x1 >= 0 && x1 < OW_WIDTH && y1 >= 0 && y1 < OW_HEIGHT) {
        Tile *t = &ow->map[y1][x1];
        if (t->type != TILE_WATER && t->type != TILE_LAKE) {
            set_road(t);
        }
    }
}

/* Draw a river from source toward a direction. Width 2-3 tiles for a real barrier. */
static void draw_river(Overworld *ow, int x, int y, int dx, int dy, int len) {
    for (int i = 0; i < len; i++) {
        if (x < 0 || x >= OW_WIDTH || y < 0 || y >= OW_HEIGHT) break;
        Tile *t = &ow->map[y][x];
        if (t->type == TILE_WATER) break;  /* reached the sea */

        /* Draw river 2-3 tiles wide perpendicular to flow */
        set_river(t);
        if (dx != 0) {
            /* Flowing horizontally: widen vertically */
            if (y > 0) set_river(&ow->map[y-1][x]);
            if (y < OW_HEIGHT - 1 && (i % 3 != 0)) set_river(&ow->map[y+1][x]);
        } else {
            /* Flowing vertically: widen horizontally */
            if (x > 0) set_river(&ow->map[y][x-1]);
            if (x < OW_WIDTH - 1 && (i % 3 != 0)) set_river(&ow->map[y][x+1]);
        }

        x += dx;
        y += dy;
        /* Add some meandering */
        if ((hash2d(x * 3, y + i * 7) & 7) == 0) {
            if (dx == 0) x += ((hash2d(x, y) & 1) ? 1 : -1);
            else         y += ((hash2d(x, y) & 1) ? 1 : -1);
        }
    }
}

/* Draw a river between two points (for rivers that need specific routing) */
static void draw_river_to(Overworld *ow, int x0, int y0, int x1, int y1) {
    int x = x0, y = y0;
    int steps = 0;
    while ((x != x1 || y != y1) && steps < 500) {
        if (x < 0 || x >= OW_WIDTH || y < 0 || y >= OW_HEIGHT) break;
        Tile *t = &ow->map[y][x];
        if (t->type == TILE_WATER) break;

        set_river(t);
        /* Widen to 2 tiles */
        int pdx = (y1 != y0) ? 1 : 0;
        int pdy = (x1 != x0) ? 1 : 0;
        int wx = x + pdx, wy = y + pdy;
        if (wx >= 0 && wx < OW_WIDTH && wy >= 0 && wy < OW_HEIGHT)
            set_river(&ow->map[wy][wx]);

        int adx = abs(x1 - x), ady = abs(y1 - y);
        int sdx = (x1 > x) ? 1 : (x1 < x) ? -1 : 0;
        int sdy = (y1 > y) ? 1 : (y1 < y) ? -1 : 0;

        if (adx >= ady) x += sdx;
        else            y += sdy;

        /* Meander slightly */
        if ((hash2d(x * 5, y * 3 + steps) & 7) == 0) {
            if (adx >= ady && sdy == 0) y += ((hash2d(x, y) & 1) ? 1 : -1);
            else if (sdx == 0) x += ((hash2d(x, y) & 1) ? 1 : -1);
        }
        steps++;
    }
}

/* Place a small lake */
static void draw_lake(Overworld *ow, int cx, int cy, int radius) {
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            int x = cx + dx, y = cy + dy;
            if (x < 0 || x >= OW_WIDTH || y < 0 || y >= OW_HEIGHT) continue;
            if (dx*dx + dy*dy <= radius*radius) {
                set_lake(&ow->map[y][x]);
            }
        }
    }
}

/* Draw an island in the sea and place a boat on the nearest mainland shore */
static void draw_island(Overworld *ow, int cx, int cy, int rx, int ry) {
    /* Carve island shape (ellipse with noise for natural look) */
    for (int dy = -ry - 1; dy <= ry + 1; dy++) {
        for (int dx = -rx - 1; dx <= rx + 1; dx++) {
            int x = cx + dx, y = cy + dy;
            if (x < 0 || x >= OW_WIDTH || y < 0 || y >= OW_HEIGHT) continue;
            double dist = (double)(dx*dx) / (rx*rx) + (double)(dy*dy) / (ry*ry);
            /* Add noise to the edge for natural coastline */
            double noise = (hash2d(x * 13, y * 7) & 0xF) / 32.0 - 0.25;
            if (dist + noise < 1.0) {
                /* Island terrain: mix of grass, hills, forest */
                double n = noise2d(x, y, 8);
                if (n > 0.65) set_hills(&ow->map[y][x]);
                else if (n > 0.45) set_forest(&ow->map[y][x]);
                else set_grass(&ow->map[y][x]);
            }
        }
    }

    /* Find nearest mainland tile and place a boat there */
    /* Search outward from island center toward mainland */
    int best_dist = 999999;
    int boat_x = -1, boat_y = -1;
    int search_r = rx + ry + 60;  /* search radius */
    for (int dy = -search_r; dy <= search_r; dy++) {
        for (int dx = -search_r; dx <= search_r; dx++) {
            int x = cx + dx, y = cy + dy;
            if (x < 0 || x >= OW_WIDTH || y < 0 || y >= OW_HEIGHT) continue;
            Tile *t = &ow->map[y][x];
            /* Look for a land tile adjacent to water (shore) */
            if (!t->passable || t->type == TILE_BRIDGE) continue;
            /* Must be on the mainland, not on this island */
            int island_dist_sq = dx*dx + dy*dy;
            if (island_dist_sq < (rx+3)*(ry+3)) continue;  /* still on the island */

            /* Check if adjacent to water */
            bool near_water = false;
            for (int d = 0; d < 4; d++) {
                int nx = x + dir_dx[d*2], ny = y + dir_dy[d*2];
                if (nx >= 0 && nx < OW_WIDTH && ny >= 0 && ny < OW_HEIGHT) {
                    if (ow->map[ny][nx].type == TILE_WATER)
                        near_water = true;
                }
            }
            if (near_water && island_dist_sq < best_dist) {
                best_dist = island_dist_sq;
                boat_x = x;
                boat_y = y;
            }
        }
    }

    /* Place the boat on the mainland shore */
    if (boat_x >= 0 && boat_y >= 0) {
        set_tile(&ow->map[boat_y][boat_x], TILE_BRIDGE, 'B', CP_BROWN, true);
    }

    /* Also place a boat on the island shore facing the mainland */
    for (int dy = -ry - 2; dy <= ry + 2; dy++) {
        for (int dx = -rx - 2; dx <= rx + 2; dx++) {
            int x = cx + dx, y = cy + dy;
            if (x < 0 || x >= OW_WIDTH || y < 0 || y >= OW_HEIGHT) continue;
            Tile *t = &ow->map[y][x];
            if (!t->passable) continue;
            /* Check if this island tile is adjacent to sea */
            bool shore = false;
            for (int d = 0; d < 4; d++) {
                int nx = x + dir_dx[d*2], ny = y + dir_dy[d*2];
                if (nx >= 0 && nx < OW_WIDTH && ny >= 0 && ny < OW_HEIGHT) {
                    if (ow->map[ny][nx].type == TILE_WATER)
                        shore = true;
                }
            }
            if (shore) {
                /* Pick the shore tile closest to the mainland boat */
                if (boat_x >= 0) {
                    int dist_to_boat = (x - boat_x)*(x - boat_x) + (y - boat_y)*(y - boat_y);
                    if (dist_to_boat < best_dist) {
                        best_dist = dist_to_boat;
                        set_tile(&ow->map[y][x], TILE_BRIDGE, 'B', CP_BROWN, true);
                    }
                }
                break;  /* just place one island boat */
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* Location placement                                                  */
/* ------------------------------------------------------------------ */

static int ow_add_location(Overworld *ow, const char *name, LocationType type,
                            int x, int y, char glyph, short color) {
    if (ow->num_locations >= MAX_LOCATIONS) return -1;
    if (x < 0 || x >= OW_WIDTH || y < 0 || y >= OW_HEIGHT) return -1;

    Location *loc = &ow->locations[ow->num_locations];
    snprintf(loc->name, MAX_NAME, "%s", name);
    loc->type = type;
    loc->pos = (Vec2){ x, y };
    loc->glyph = glyph;
    loc->color_pair = color;
    loc->discovered = false;
    loc->id = ow->num_locations;

    /* Place glyph on map */
    ow->map[y][x].glyph = glyph;
    ow->map[y][x].color_pair = color;
    ow->map[y][x].passable = true;

    return ow->num_locations++;
}

/* ------------------------------------------------------------------ */
/* Main initialization                                                 */
/* ------------------------------------------------------------------ */

void overworld_init(Overworld *ow) {
    memset(ow, 0, sizeof(*ow));

    /* Step 1: Fill everything with sea */
    for (int y = 0; y < OW_HEIGHT; y++)
        for (int x = 0; x < OW_WIDTH; x++)
            set_sea(&ow->map[y][x]);

    /* Step 2: Build coastline and fill land */
    Span spans[OW_HEIGHT];
    build_coastline(spans);

    /* Add coastal roughness */
    for (int y = 0; y < OW_HEIGHT; y++) {
        if (spans[y].left < 0) continue;
        /* Perturb coastline for natural look */
        int jitter_l = (hash2d(y * 7, 42) % 5) - 2;
        int jitter_r = (hash2d(y * 13, 99) % 5) - 2;
        spans[y].left  += jitter_l;
        spans[y].right += jitter_r;
        if (spans[y].left < 2) spans[y].left = 2;
        if (spans[y].right >= OW_WIDTH - 2) spans[y].right = OW_WIDTH - 3;
    }

    /* Fill land with terrain */
    for (int y = 0; y < OW_HEIGHT; y++) {
        if (spans[y].left < 0) continue;
        for (int x = spans[y].left; x <= spans[y].right; x++) {
            assign_terrain(&ow->map[y][x], x, y);
        }
    }

    /* Step 3: Rivers -- wide enough to be barriers, need bridges to cross */

    /* River Thames -- major east-west barrier across southern England */
    draw_river_to(ow, 190, 190, 350, 185);

    /* River Severn -- flows south through Welsh border country to Bristol Channel */
    draw_river_to(ow, 140, 115, 130, 160);

    /* River Trent -- flows northeast through the Midlands */
    draw_river_to(ow, 240, 135, 310, 100);

    /* River Humber -- estuary flowing east into the North Sea */
    draw_river(ow, 290, 98, 3, 0, 30);

    /* River Tyne -- flows east across northern England */
    draw_river(ow, 220, 72, 3, 0, 35);

    /* River Ouse (Yorkshire) -- flows south through York area */
    draw_river_to(ow, 275, 80, 290, 98);

    /* River Dee -- flows north along Welsh border */
    draw_river_to(ow, 120, 140, 115, 112);

    /* River Exe -- flows south through Devon */
    draw_river_to(ow, 115, 195, 110, 215);

    /* River Avon (Bristol) -- flows west to Bristol Channel */
    draw_river_to(ow, 165, 172, 135, 168);

    /* River Medway -- flows through Kent */
    draw_river_to(ow, 350, 190, 365, 205);

    /* River Tweed -- Scottish border */
    draw_river(ow, 200, 62, 3, 0, 30);

    /* River Clyde -- Scottish lowlands, flows west */
    draw_river_to(ow, 180, 50, 140, 55);

    /* Step 4: Lakes (scaled 1.25x, radii +1) */
    draw_lake(ow, 148, 85, 4);    /* Lake District */
    draw_lake(ow, 156, 90, 3);
    draw_lake(ow, 144, 91, 3);
    draw_lake(ow, 181, 28, 5);    /* Loch Ness */
    draw_lake(ow, 162, 38, 4);    /* Loch Lomond */
    draw_lake(ow, 206, 22, 3);    /* Highland loch */
    draw_lake(ow, 112, 125, 3);   /* Lake Bala (Wales) */
    draw_lake(ow, 150, 81, 4);    /* Windermere */
    draw_lake(ow, 200, 169, 4);   /* Lady of the Lake */
    /* Norfolk Broads */
    draw_lake(ow, 355, 125, 3);
    draw_lake(ow, 360, 128, 2);
    /* Llyn Tegid (larger Lake Bala) */
    draw_lake(ow, 108, 128, 2);
    /* Ullswater (Lake District) */
    draw_lake(ow, 155, 82, 3);

    /* Step 4b: Islands -- accessible by boat (B tiles on shores) */

    /* Isle of Wight -- south coast, off Hampshire */
    draw_island(ow, 240, 215, 8, 3);

    /* Isle of Man -- Irish Sea, between England and Ireland */
    draw_island(ow, 100, 80, 6, 8);

    /* Lundy Island -- Bristol Channel, small rocky island */
    draw_island(ow, 85, 170, 3, 2);

    /* Anglesey -- off northwest Wales */
    draw_island(ow, 80, 112, 7, 5);

    /* Avalon -- mystical hidden isle, southwest in the sea */
    draw_island(ow, 40, 200, 5, 4);

    /* Holy Island (Lindisfarne) -- off Northumberland coast */
    draw_island(ow, 290, 55, 3, 2);

    /* Orkney -- far north Scotland */
    draw_island(ow, 190, 8, 6, 4);

    /* Ireland -- large island to the west */
    draw_island(ow, 30, 100, 15, 20);

    /* Gaul (France) -- across the channel, southeast */
    draw_island(ow, 400, 240, 12, 6);

    /* Brittany -- across the channel, south */
    draw_island(ow, 340, 245, 10, 4);

    /* Step 5: Roads (scaled 1.25x) */
    draw_road(ow, 212, 162, 312, 181);   /* Camelot -> London */
    draw_road(ow, 212, 162, 194, 188);   /* Camelot -> Glastonbury */
    draw_road(ow, 212, 162, 250, 125);   /* Camelot -> Sherwood */
    draw_road(ow, 212, 162, 138, 138);   /* Camelot -> Wales */
    draw_road(ow, 212, 162, 238, 194);   /* Camelot -> Winchester */
    draw_road(ow, 250, 125, 275, 94);    /* Sherwood -> York */
    draw_road(ow, 275, 94, 269, 72);     /* York -> north */
    draw_road(ow, 269, 72, 212, 56);     /* -> Scotland */
    draw_road(ow, 312, 181, 375, 194);   /* London -> Canterbury */
    draw_road(ow, 312, 181, 238, 194);   /* London -> Winchester */
    draw_road(ow, 194, 188, 162, 175);   /* Glastonbury -> Bath */
    draw_road(ow, 138, 138, 112, 131);   /* Wales -> inner Wales */
    draw_road(ow, 100, 212, 69, 231);    /* Devon -> Cornwall */
    draw_road(ow, 194, 188, 100, 212);   /* Glastonbury -> Devon/Cornwall */
    draw_road(ow, 312, 181, 325, 125);   /* London -> east midlands */
    draw_road(ow, 275, 94, 331, 88);     /* York -> Whitby */
    draw_road(ow, 375, 194, 431, 212);   /* Canterbury -> Dover */

    /* ---- Place Locations (scaled 1.25x) ---- */

    /* Towns */
    ow_add_location(ow, "Camelot",      LOC_TOWN, 212, 162, '*', CP_YELLOW_BOLD);
    ow_add_location(ow, "London",       LOC_TOWN, 312, 181, '*', CP_WHITE_BOLD);
    ow_add_location(ow, "Canterbury",   LOC_TOWN, 375, 194, '*', CP_WHITE_BOLD);
    ow_add_location(ow, "Winchester",   LOC_TOWN, 238, 194, '*', CP_WHITE);
    ow_add_location(ow, "Glastonbury",  LOC_TOWN, 194, 188, '*', CP_MAGENTA);
    ow_add_location(ow, "Bath",         LOC_TOWN, 162, 175, '*', CP_CYAN);
    ow_add_location(ow, "Tintagel",     LOC_TOWN,  94, 194, '*', CP_WHITE);
    ow_add_location(ow, "Cornwall",     LOC_TOWN,  60, 231, '*', CP_WHITE);
    ow_add_location(ow, "Sherwood",     LOC_TOWN, 262, 119, '*', CP_GREEN_BOLD);
    ow_add_location(ow, "York",         LOC_TOWN, 275, 94,  '*', CP_WHITE_BOLD);
    ow_add_location(ow, "Wales",        LOC_TOWN, 125, 135, '*', CP_RED);
    ow_add_location(ow, "Whitby",       LOC_TOWN, 331, 88,  '*', CP_GRAY);
    ow_add_location(ow, "Llanthony",    LOC_TOWN, 119, 148, '*', CP_WHITE);
    ow_add_location(ow, "Carbonek",     LOC_TOWN, 219, 138, '*', CP_YELLOW);

    /* Active Castles */
    ow_add_location(ow, "Camelot Castle",   LOC_CASTLE_ACTIVE, 215, 162, '#', CP_YELLOW_BOLD);
    ow_add_location(ow, "Castle Surluise",  LOC_CASTLE_ACTIVE, 175, 131, '#', CP_WHITE);
    ow_add_location(ow, "Castle Lothian",   LOC_CASTLE_ACTIVE, 212, 48,  '#', CP_WHITE);
    ow_add_location(ow, "Castle Northumberland", LOC_CASTLE_ACTIVE, 262, 69, '#', CP_WHITE);
    ow_add_location(ow, "Castle Gore",      LOC_CASTLE_ACTIVE, 150, 122, '#', CP_WHITE);
    ow_add_location(ow, "Castle Strangore", LOC_CASTLE_ACTIVE, 200, 148, '#', CP_WHITE);
    ow_add_location(ow, "Castle Cradelment",LOC_CASTLE_ACTIVE, 106, 125, '#', CP_WHITE);
    ow_add_location(ow, "Castle Cornwall",  LOC_CASTLE_ACTIVE,  69, 222, '#', CP_WHITE);
    ow_add_location(ow, "Castle Listenoise",LOC_CASTLE_ACTIVE, 294, 150, '#', CP_WHITE);
    ow_add_location(ow, "Castle Benwick",   LOC_CASTLE_ACTIVE, 350, 200, '#', CP_WHITE);
    ow_add_location(ow, "Castle Carados",   LOC_CASTLE_ACTIVE, 194, 38,  '#', CP_WHITE);

    /* Abandoned Castles */
    ow_add_location(ow, "Castle Dolorous Garde", LOC_CASTLE_ABANDONED, 250, 106, '#', CP_GRAY);
    ow_add_location(ow, "Castle Perilous",  LOC_CASTLE_ABANDONED, 181, 172, '#', CP_MAGENTA);
    ow_add_location(ow, "Bamburgh Castle",  LOC_CASTLE_ABANDONED, 281, 60,  '#', CP_GRAY);

    /* Landmarks */
    ow_add_location(ow, "Stonehenge",       LOC_LANDMARK, 231, 198, '+', CP_YELLOW);
    /* Hadrian's Wall -- draw a ruined wall east-west across northern England.
       Mostly impassable stone ruins with gaps (passable) every few tiles. */
    {
        int wall_y = 65;
        int wall_x_start = 140;
        int wall_x_end = 300;
        for (int x = wall_x_start; x <= wall_x_end; x++) {
            if (x < 0 || x >= OW_WIDTH) continue;
            Tile *t = &ow->map[wall_y][x];
            /* Skip if water/river/lake */
            if (t->type == TILE_WATER || t->type == TILE_RIVER || t->type == TILE_LAKE)
                continue;
            /* Gaps every 8-12 tiles for passage (ruined sections) */
            int gap = (hash2d(x, wall_y * 31) & 7);
            if (gap == 0) {
                /* Gap -- passable rubble */
                set_tile(t, TILE_ROAD, '.', CP_GRAY, true);
            } else {
                /* Ruined wall segment */
                set_tile(t, TILE_WALL, '#', CP_GRAY, false);
                t->blocks_sight = false;  /* low ruined wall, can see over */
            }
        }
    }
    ow_add_location(ow, "Hadrian's Wall",   LOC_LANDMARK, 238, 65,  '+', CP_WHITE);
    ow_add_location(ow, "White Cliffs",     LOC_LANDMARK, 440, 215, '+', CP_WHITE_BOLD);

    /* Dungeon Entrances */
    ow_add_location(ow, "Camelot Catacombs",LOC_DUNGEON_ENTRANCE, 216, 164, '>', CP_WHITE);
    ow_add_location(ow, "Tintagel Caves",   LOC_DUNGEON_ENTRANCE,  91, 195, '>', CP_WHITE);
    ow_add_location(ow, "Sherwood Depths",  LOC_DUNGEON_ENTRANCE, 265, 121, '>', CP_WHITE);
    ow_add_location(ow, "Glastonbury Tor",  LOC_DUNGEON_ENTRANCE, 196, 189, '>', CP_WHITE);
    ow_add_location(ow, "White Cliffs Cave",LOC_DUNGEON_ENTRANCE, 442, 217, '>', CP_CYAN);
    ow_add_location(ow, "Whitby Abbey",     LOC_DUNGEON_ENTRANCE, 334, 89,  '>', CP_RED);

    /* Volcano */
    ow_add_location(ow, "Mount Draig",      LOC_VOLCANO, 100, 122, 'V', CP_RED_BOLD);

    /* Faerie Rings */
    ow_add_location(ow, "Faerie Ring",      LOC_LANDMARK, 244, 115, 'o', CP_GREEN_BOLD);
    ow_add_location(ow, "Faerie Ring",      LOC_LANDMARK, 144, 181, 'o', CP_GREEN_BOLD);

    /* Island locations */
    ow_add_location(ow, "Isle of Wight",    LOC_TOWN, 240, 215, '*', CP_WHITE);
    ow_add_location(ow, "Isle of Man",      LOC_TOWN, 100, 80,  '*', CP_WHITE);
    ow_add_location(ow, "Lundy Island",     LOC_LANDMARK, 85, 170, '+', CP_GRAY);
    ow_add_location(ow, "Anglesey",         LOC_TOWN, 80, 112,  '*', CP_WHITE);
    ow_add_location(ow, "Avalon",           LOC_LANDMARK, 40, 200, '+', CP_YELLOW_BOLD);
    ow_add_location(ow, "Holy Island",      LOC_LANDMARK, 290, 55, '+', CP_WHITE_BOLD);
    ow_add_location(ow, "Orkney",           LOC_TOWN, 190, 8,   '*', CP_WHITE);

    /* Island dungeon entrances */
    ow_add_location(ow, "Avalon Shrine",    LOC_DUNGEON_ENTRANCE, 42, 200, '>', CP_YELLOW);
    ow_add_location(ow, "Orkney Barrows",   LOC_DUNGEON_ENTRANCE, 192, 9, '>', CP_GRAY);

    /* Overseas castles */
    ow_add_location(ow, "Castle Ireland",   LOC_CASTLE_ACTIVE, 30, 100, '#', CP_WHITE);
    ow_add_location(ow, "Castle Gaul",      LOC_CASTLE_ACTIVE, 400, 240, '#', CP_WHITE);
    ow_add_location(ow, "Castle Brittany",  LOC_CASTLE_ACTIVE, 340, 245, '#', CP_WHITE);

    /* ---- Random cottages (10) scattered on grassland/road near roads ---- */
    {
        int placed = 0;
        int attempts = 0;
        while (placed < 10 && attempts < 2000) {
            int rx = rng_range(40, OW_WIDTH - 40);
            int ry = rng_range(20, OW_HEIGHT - 20);
            Tile *t = &ow->map[ry][rx];
            if (t->passable && (t->type == TILE_GRASS || t->type == TILE_ROAD) &&
                !overworld_location_at(ow, rx, ry)) {
                char name[MAX_NAME];
                snprintf(name, MAX_NAME, "Cottage");
                ow_add_location(ow, name, LOC_COTTAGE, rx, ry, 'n', CP_BROWN);
                placed++;
            }
            attempts++;
        }
    }

    /* ---- Random caves (6) in hills/mountains ---- */
    {
        int placed = 0;
        int attempts = 0;
        while (placed < 6 && attempts < 2000) {
            int rx = rng_range(40, OW_WIDTH - 40);
            int ry = rng_range(20, OW_HEIGHT - 20);
            Tile *t = &ow->map[ry][rx];
            if (t->passable && (t->type == TILE_HILLS) &&
                !overworld_location_at(ow, rx, ry)) {
                char name[MAX_NAME];
                snprintf(name, MAX_NAME, "Cave");
                ow_add_location(ow, name, LOC_CAVE, rx, ry, '(', CP_GRAY);
                placed++;
            }
            attempts++;
        }
    }
}

Location *overworld_location_at(Overworld *ow, int x, int y) {
    for (int i = 0; i < ow->num_locations; i++) {
        if (ow->locations[i].pos.x == x && ow->locations[i].pos.y == y)
            return &ow->locations[i];
    }
    return NULL;
}

bool overworld_is_passable(Overworld *ow, int x, int y) {
    if (x < 0 || x >= OW_WIDTH || y < 0 || y >= OW_HEIGHT) return false;
    return ow->map[y][x].passable;
}
