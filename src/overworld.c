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
        {  10,  140, 195 },
        {  15,  125, 210 },
        {  20,  110, 230 },
        {  25,  105, 240 },
        /* Scottish Highlands - wider */
        {  30,  100, 250 },
        {  35,   95, 255 },
        {  40,  100, 250 },
        /* Scottish Lowlands */
        {  45,  105, 260 },
        {  50,  110, 265 },
        /* Border region - narrower (Solway/Tyne) */
        {  55,  115, 270 },
        {  60,  110, 275 },
        /* Northern England - Lake District west, Yorkshire east */
        {  65,  105, 285 },
        {  70,  100, 290 },
        {  75,  100, 295 },
        /* Lancashire / Yorkshire */
        {  80,   95, 295 },
        {  85,   95, 298 },
        /* Wales begins - bulge west */
        {  90,   80, 300 },
        {  95,   70, 300 },
        { 100,   65, 305 },
        /* Mid Wales - widest western extent */
        { 105,   60, 305 },
        { 110,   60, 310 },
        { 115,   65, 310 },
        /* South Wales / Bristol Channel indentation */
        { 120,   70, 315 },
        { 125,   85, 315 },  /* Bristol Channel narrows */
        /* Southwest England */
        { 130,   80, 320 },
        { 135,   75, 325 },
        { 140,   70, 330 },
        { 145,   65, 335 },
        /* Devon / Dorset */
        { 150,   60, 335 },
        { 155,   55, 340 },
        { 160,   55, 345 },
        /* South coast / Hampshire / Sussex */
        { 165,   60, 350 },
        { 170,   70, 355 },
        /* Kent bulge southeast + Cornwall peninsula southwest */
        { 175,   50, 360 },
        { 180,   40, 365 },
        /* Cornwall narrows */
        { 185,   35, 350 },
        { 188,   30, 300 },
        { 192,   28, 200 },
        /* Cornwall tip */
        { 196,   30, 100 },
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

    /* Scottish Highlands (y < 50) -- hilly and forested */
    if (y < 50) {
        if (n1 > 0.65) { set_hills(t); return; }
        if (n1 > 0.45) { set_forest(t); return; }
        set_grass(t);
        return;
    }

    /* Northern England (y 50-85) -- Pennine hills, Lake District */
    if (y < 85) {
        /* Pennines: central spine of hills */
        int center_x = lerp(190, 200, y - 50, 35);
        int dist_from_pennines = abs(x - center_x);
        if (dist_from_pennines < 15 && n1 > 0.35) { set_hills(t); return; }

        /* Lake District (west side, y 60-75) */
        if (y >= 60 && y <= 78 && x >= 105 && x <= 140) {
            if (n1 > 0.7) { set_lake(t); return; }
            if (n1 > 0.5) { set_hills(t); return; }
        }

        if (n1 > 0.7) { set_forest(t); return; }
        set_grass(t);
        return;
    }

    /* Wales (y 90-125, x < 140) -- mountainous */
    if (y >= 88 && y <= 125 && x < 150) {
        if (n1 > 0.55) { set_hills(t); return; }
        if (n1 > 0.40 && n2 > 0.5) { set_forest(t); return; }
        set_grass(t);
        return;
    }

    /* Sherwood Forest area (y 85-100, x 200-260) */
    if (y >= 85 && y <= 100 && x >= 200 && x <= 260) {
        if (n1 > 0.35) { set_forest(t); return; }
    }

    /* The Fens -- marshland (y 100-115, x 260-310) */
    if (y >= 100 && y <= 118 && x >= 260 && x <= 310) {
        if (n1 > 0.55) { set_marsh(t); return; }
    }

    /* Somerset Levels -- marsh/swamp (y 130-145, x 100-140) */
    if (y >= 130 && y <= 148 && x >= 95 && x <= 145) {
        if (n1 > 0.6) { set_swamp(t); return; }
        if (n1 > 0.45) { set_marsh(t); return; }
    }

    /* Devon/Cornwall -- moorland (y 155-190, x < 100) */
    if (y >= 155 && x < 110) {
        if (n1 > 0.6) { set_hills(t); return; }
        if (n1 > 0.45 && n2 > 0.5) { set_marsh(t); return; }
    }

    /* Southern Downs -- gentle hills (y 155-170, x 200-320) */
    if (y >= 155 && y <= 172 && x >= 200 && x <= 320) {
        if (n1 > 0.6) { set_hills(t); return; }
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

/* Draw a river from source to coast/edge */
static void draw_river(Overworld *ow, int x, int y, int dx, int dy, int len) {
    for (int i = 0; i < len; i++) {
        if (x < 0 || x >= OW_WIDTH || y < 0 || y >= OW_HEIGHT) break;
        Tile *t = &ow->map[y][x];
        if (t->type == TILE_WATER || t->type == TILE_LAKE) break;
        set_river(t);
        x += dx;
        y += dy;
        /* Add some wandering */
        if ((hash2d(x, y + i) & 7) == 0) {
            if (dx == 0) x += ((hash2d(x, y) & 1) ? 1 : -1);
            else         y += ((hash2d(x, y) & 1) ? 1 : -1);
        }
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

    /* Step 3: Rivers */
    /* River Thames -- flows east across southern England */
    draw_river(ow, 180, 155, 3, 0, 60);
    /* River Severn -- flows south through Wales/west England */
    draw_river(ow, 130, 95, 0, 2, 25);
    /* River Trent -- flows northeast through Midlands */
    draw_river(ow, 210, 100, 2, -1, 30);
    /* River Humber -- east coast */
    draw_river(ow, 250, 82, 3, 0, 20);
    /* River Tyne -- northern England */
    draw_river(ow, 200, 58, 3, 0, 25);

    /* Step 4: Lakes */
    /* Lake District */
    draw_lake(ow, 118, 68, 3);
    draw_lake(ow, 125, 72, 2);
    draw_lake(ow, 115, 73, 2);
    /* Scottish Lochs */
    draw_lake(ow, 145, 22, 4);   /* Loch Ness */
    draw_lake(ow, 130, 30, 3);   /* Loch Lomond */
    draw_lake(ow, 165, 18, 2);   /* small highland loch */
    /* Lake Bala (Wales) */
    draw_lake(ow, 90, 100, 2);
    /* Windermere */
    draw_lake(ow, 120, 65, 3);
    /* Lady of the Lake */
    draw_lake(ow, 160, 135, 3);

    /* Step 5: Roads connecting major locations */
    /* Main road network */
    /* Camelot (170, 130) hub */
    draw_road(ow, 170, 130, 250, 145);   /* Camelot -> London */
    draw_road(ow, 170, 130, 155, 150);   /* Camelot -> Glastonbury */
    draw_road(ow, 170, 130, 200, 100);   /* Camelot -> Sherwood */
    draw_road(ow, 170, 130, 110, 110);   /* Camelot -> Wales */
    draw_road(ow, 170, 130, 190, 155);   /* Camelot -> Winchester */
    draw_road(ow, 200, 100, 220, 75);    /* Sherwood -> York */
    draw_road(ow, 220, 75, 215, 58);     /* York -> north */
    draw_road(ow, 215, 58, 170, 45);     /* -> Scotland */
    draw_road(ow, 250, 145, 300, 155);   /* London -> Canterbury */
    draw_road(ow, 250, 145, 190, 155);   /* London -> Winchester */
    draw_road(ow, 155, 150, 130, 140);   /* Glastonbury -> Bath */
    draw_road(ow, 110, 110, 90, 105);    /* Wales -> inner Wales */
    draw_road(ow, 80, 170, 55, 185);     /* Devon -> Cornwall */
    draw_road(ow, 155, 150, 80, 170);    /* Glastonbury -> Devon/Cornwall */
    draw_road(ow, 250, 145, 260, 100);   /* London -> east midlands */
    draw_road(ow, 220, 75, 260, 72);     /* York -> Whitby */
    draw_road(ow, 300, 155, 340, 170);   /* Canterbury -> Dover */

    /* ---- Place Locations ---- */

    /* Towns */
    ow_add_location(ow, "Camelot",      LOC_TOWN, 170, 130, '*', CP_YELLOW_BOLD);
    ow_add_location(ow, "London",       LOC_TOWN, 250, 145, '*', CP_WHITE_BOLD);
    ow_add_location(ow, "Canterbury",   LOC_TOWN, 300, 155, '*', CP_WHITE_BOLD);
    ow_add_location(ow, "Winchester",   LOC_TOWN, 190, 155, '*', CP_WHITE);
    ow_add_location(ow, "Glastonbury",  LOC_TOWN, 155, 150, '*', CP_MAGENTA);
    ow_add_location(ow, "Bath",         LOC_TOWN, 130, 140, '*', CP_CYAN);
    ow_add_location(ow, "Tintagel",     LOC_TOWN,  75, 155, '*', CP_WHITE);
    ow_add_location(ow, "Cornwall",     LOC_TOWN,  48, 185, '*', CP_WHITE);
    ow_add_location(ow, "Sherwood",     LOC_TOWN, 210, 95,  '*', CP_GREEN_BOLD);
    ow_add_location(ow, "York",         LOC_TOWN, 220, 75,  '*', CP_WHITE_BOLD);
    ow_add_location(ow, "Wales",        LOC_TOWN, 100, 108, '*', CP_RED);
    ow_add_location(ow, "Whitby",       LOC_TOWN, 265, 70,  '*', CP_GRAY);
    ow_add_location(ow, "Llanthony",    LOC_TOWN,  95, 118, '*', CP_WHITE);
    ow_add_location(ow, "Carbonek",     LOC_TOWN, 175, 110, '*', CP_YELLOW);

    /* Active Castles */
    ow_add_location(ow, "Camelot Castle",   LOC_CASTLE_ACTIVE, 172, 130, '#', CP_YELLOW_BOLD);
    ow_add_location(ow, "Castle Surluise",  LOC_CASTLE_ACTIVE, 140, 105, '#', CP_WHITE);
    ow_add_location(ow, "Castle Lothian",   LOC_CASTLE_ACTIVE, 170, 38,  '#', CP_WHITE);
    ow_add_location(ow, "Castle Northumberland", LOC_CASTLE_ACTIVE, 210, 55, '#', CP_WHITE);
    ow_add_location(ow, "Castle Gore",      LOC_CASTLE_ACTIVE, 120, 98,  '#', CP_WHITE);
    ow_add_location(ow, "Castle Strangore", LOC_CASTLE_ACTIVE, 160, 118, '#', CP_WHITE);
    ow_add_location(ow, "Castle Cradelment",LOC_CASTLE_ACTIVE,  85, 100, '#', CP_WHITE);
    ow_add_location(ow, "Castle Cornwall",  LOC_CASTLE_ACTIVE,  55, 178, '#', CP_WHITE);
    ow_add_location(ow, "Castle Listenoise",LOC_CASTLE_ACTIVE, 235, 120, '#', CP_WHITE);
    ow_add_location(ow, "Castle Benwick",   LOC_CASTLE_ACTIVE, 280, 160, '#', CP_WHITE);
    ow_add_location(ow, "Castle Carados",   LOC_CASTLE_ACTIVE, 155, 30,  '#', CP_WHITE);

    /* Abandoned Castles */
    ow_add_location(ow, "Castle Dolorous Garde", LOC_CASTLE_ABANDONED, 200, 85, '#', CP_GRAY);
    ow_add_location(ow, "Castle Perilous",  LOC_CASTLE_ABANDONED, 145, 138, '#', CP_MAGENTA);
    ow_add_location(ow, "Bamburgh Castle",  LOC_CASTLE_ABANDONED, 225, 48, '#', CP_GRAY);

    /* Landmarks */
    ow_add_location(ow, "Stonehenge",       LOC_LANDMARK, 185, 158, '+', CP_YELLOW);
    ow_add_location(ow, "Hadrian's Wall",   LOC_LANDMARK, 190, 52,  '+', CP_WHITE);
    ow_add_location(ow, "White Cliffs",     LOC_LANDMARK, 345, 170, '+', CP_WHITE_BOLD);

    /* Dungeon Entrances */
    ow_add_location(ow, "Camelot Catacombs",LOC_DUNGEON_ENTRANCE, 173, 131, '>', CP_WHITE);
    ow_add_location(ow, "Tintagel Caves",   LOC_DUNGEON_ENTRANCE,  73, 156, '>', CP_WHITE);
    ow_add_location(ow, "Sherwood Depths",  LOC_DUNGEON_ENTRANCE, 212, 97,  '>', CP_WHITE);
    ow_add_location(ow, "Glastonbury Tor",  LOC_DUNGEON_ENTRANCE, 157, 151, '>', CP_WHITE);
    ow_add_location(ow, "White Cliffs Cave",LOC_DUNGEON_ENTRANCE, 347, 172, '>', CP_CYAN);
    ow_add_location(ow, "Whitby Abbey",     LOC_DUNGEON_ENTRANCE, 267, 71,  '>', CP_RED);

    /* Volcano */
    ow_add_location(ow, "Mount Draig",      LOC_VOLCANO,  80, 98, 'V', CP_RED_BOLD);

    /* Faerie Rings */
    ow_add_location(ow, "Faerie Ring",      LOC_LANDMARK, 195, 92,  'o', CP_GREEN_BOLD);
    ow_add_location(ow, "Faerie Ring",      LOC_LANDMARK, 115, 145, 'o', CP_GREEN_BOLD);
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
