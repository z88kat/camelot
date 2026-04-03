#include "overworld.h"
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Hand-crafted ASCII map of England, Wales, and southern Scotland     */
/* 120 x 40 tiles.  Legend:                                            */
/*   ~ = sea/ocean     " = grassland     ^ = hills/mountains          */
/*   T = forest        . = road          , = marsh                    */
/*   % = swamp         & = dense woods   = = river                    */
/*   o = lake          H = bridge                                     */
/* Locations are placed separately as markers on top of terrain.       */
/* ------------------------------------------------------------------ */

static const char *ow_raw_map[OW_HEIGHT] = {
/*          1111111111222222222233333333334444444444555555555566666666667777777777888888888899999999990000000000111111111112 */
/* 0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789 */
  "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",  /* 0  */
  "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",  /* 1  */
  "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^^^^^^^^^^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ooo",  /* 2  */
  "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^^^\"\"\"\"\"\"\"\"^^^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ooo~",  /* 3  */
  "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^^\"\"\"\"T\"\"\"\"\"\"\"^^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~o~~",  /* 4  */
  "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^^\"\"\"T\"\"\"\"\"\"..\"\"^^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~o~~",  /* 5  */
  "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^^\"\"\"\"\"\"\"\"\"\"\"..\"\"\"^^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~o~~",  /* 6  */
  "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^^\"\"TT\"\"\"\"\"\"\"\"..\"\"\"\"^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~o~~",  /* 7  */
  "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^^\"\"\"\"\"\"\"\"\"\"\"\"\"..\"\"\"\"\"^^~~~~~~~~~~~~~~~oooo~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",  /* 8  */
  "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^^\"\"\"\"\"\"\"\"o\"\"\"\"\"..\"\"\"\"\"\"^^~~~~~~~~~~~~~oo\"\"oo~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",  /* 9  */
  "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^\"\"\"\"\"\"\"ooo\"\"\"\"\"..\"\"\"\"\"\"\"^^~~~~~~~~~~~oo\"\"\"\"oo~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",  /* 10 */
  "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^^\"\"\"\"\"\"\"\"o\"\"\"\"\"\"..\"\"\"\"\"\"\"\"^~~~~~~~~~~o\"\"\"\"\"\"o~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",  /* 11 */
  "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"..\"\"\"\"\"\"\"\"\"^^~~~~~~~~oo\"\"\"\"\"oo~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",  /* 12 */
  "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^^\"\"\",,\"\"\"\"\"\"\"\"\"\"..\"\"\"\"\"\"\"\"\"\"^~~~~~~~~~oooooo~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",  /* 13 */
  "~~~~~~~~~~~~~~^^^^^~~~~~~~~~~~~~~~^^\"\"\",,\"\"\"\"\"\"\"\"\"\"..\"\"\"\"\"\"\"\"\"\"^^~~~~~~~~~~~oo~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",  /* 14 */
  "~~~~~~~~~~~~^^^\"\"\"^^^~~~~~~~~~~~~^^\"\"\"\"\"\"\"\"\"\"\"=====H==\"\"\"\"\"\"\"\"\"\"\"^^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ooo",  /* 15 */
  "~~~~~~~~~~~^^\"\"\"\"\"\"\"^^~~~~~~~~~~^^\"\"\"\"TT\"\"\"\"\"=\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"^^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~o~",  /* 16 */
  "~~~~~~~~~~^^\"\"\"\"T\"\"\"\"^^~~~~~~~~^^\"\"\"TT\"\"\"\"\"\"\"=\"\"\"\"\"\"\"\"\"\"TT\"\"\"\"\"\"\"\"\"^^^^^^^^^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~o~",  /* 17 */
  "~~~~~~~~~~^\"\"\"\"TTT\"\"\"\"^~~~~~~~^^\"\"\"\"\"\"\"\"..\"\"=\"\"\"\"\"\"\"\"\"TTT\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"^^^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~oo~",  /* 18 */
  "~~~~~~~~~^^\"\"\"\"T\"\"\"\"\"\"^^~~~~~^^\"\"\"\"\"\"\"\"..\"\"=\"\"\"\"\"\"\"\"\"\"T\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"^^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~oo~~",  /* 19 */
  "~~~~~~~~~^\"\"\"\"\"\"\"\"\"\"\"\"\"^~~~~^^\"\"\"\"\"\"\"\"..\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"^^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~oo~~",  /* 20 */
  "~~~~~~~~^^\"\"\"\"\"\"\"\",,\"\"\"^^~~^^\"\"\"\"\"\"\"\"..\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"TT\"\"\"\"\"\"\"\"\"\"\"\"^^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~o~~~",  /* 21 */
  "~~~~~~~~^\"\"\"\"\"\"\"\",%,\"\"\"\"^^^^\"\"\"\"\"\"\"\"..\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"TTT\"\"\"\"\"\"\"\"\"\"\"\"\"\"^^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~o~~~~",  /* 22 */
  "~~~~~~~~^^\"\"\"\"\"\"\",,\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"..\"\"\"\"\"\"\"\"\"\"\"\"\"TT\"\"\"\"\"\"\"TTT\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~o~~~~",  /* 23 */
  "~~~~~~~~~^^\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"..\"\"\"\"\"\"\"\"\"\"\"TTT\"\"\"\"\"\"\"\"T\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"^^~~~~~~~~~~~~~~~~~~~~~~~~~~~~o~~~~",  /* 24 */
  "~~~~~~~~~~^^\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"..\"\"\"\"\"\"\"\"\"\"\"T\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\",,\"\"\"\"\"^^~~~~~~~~~~~~~~~~~~~~~~~~~~oo~~~~",  /* 25 */
  "~~~~~~~~~~~^^\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"..\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\",,%%\"\"\"\"\"^^~~~~~~~~~~~~~~~~~~~~~~~~oo~~~~~",  /* 26 */
  "~~~~~~~~~~~~^^^\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"..\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\",,\"\"\"\"\"\"\"\"^^~~~~~~~~~~~~~~~~~~~~~~o~~~~~~",  /* 27 */
  "~~~~~~~~~~~~~~^^^\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"..\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"^^~~~~~~~~~~~~~~~~~~~oo~~~~~~~",  /* 28 */
  "~~~~~~~~~~~~~~~~^^^\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"..\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"^^^~~~~~~~~~~~~~~~~o~~~~~~~~",  /* 29 */
  "~~~~~~~~~~~~~~~~~~^^^^\"\"\"\"\"\"\"\"\"\"\"\"..\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"^^^^~~~~~~~~~~~oo~~~~~~~~~",  /* 30 */
  "~~~~~~~~~~~~~~~~~~~~~^^^\"\"\"\"\"\"\"\"\"\"..\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"^^^^~~~~~oo~~~~~~~~~~",  /* 31 */
  "~~~~~~~~~~~~~~~~~~~~~~~^^^^\"\"\"\"\"\"\"..\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"^^oo~~~~~~~~~~~",  /* 32 */
  "~~~~~~~~~~~~~~~~~~~~~~~~~~^^^^\"\"\"\"..\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"^^^^~~~~~~~~",  /* 33 */
  "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^^^^^..\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"^^^^^^~~~~~~~~~~",  /* 34 */
  "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^^^^\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"^^^^^~~~~~~~~~~~~~",  /* 35 */
  "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^^^^^\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"^^^^^~~~~~~~~~~~~~~~~",  /* 36 */
  "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^^^^^^^\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"^^^^^^^~~~~~~~~~~~~~~~~~~",  /* 37 */
  "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^~~~~~~~~~~~~~~~~~~~~~~~",  /* 38 */
  "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",  /* 39 */
};

/* Set a tile from raw map character */
static void ow_tile_from_char(Tile *t, char ch) {
    t->visible = true;
    t->revealed = true;

    switch (ch) {
    case '~':
        t->type = TILE_WATER;
        t->glyph = '~';
        t->color_pair = CP_BLUE;
        t->passable = false;
        t->blocks_sight = false;
        break;
    case '"':
        t->type = TILE_GRASS;
        t->glyph = '"';
        t->color_pair = CP_GREEN;
        t->passable = true;
        t->blocks_sight = false;
        break;
    case '^':
        t->type = TILE_HILLS;
        t->glyph = '^';
        t->color_pair = CP_WHITE;
        t->passable = true;
        t->blocks_sight = false;
        break;
    case 'T':
        t->type = TILE_FOREST;
        t->glyph = 'T';
        t->color_pair = CP_GREEN_BOLD;
        t->passable = true;
        t->blocks_sight = false;
        break;
    case '.':
        t->type = TILE_ROAD;
        t->glyph = '.';
        t->color_pair = CP_YELLOW;
        t->passable = true;
        t->blocks_sight = false;
        break;
    case ',':
        t->type = TILE_MARSH;
        t->glyph = ',';
        t->color_pair = CP_GREEN;
        t->passable = true;
        t->blocks_sight = false;
        break;
    case '%':
        t->type = TILE_SWAMP;
        t->glyph = '%';
        t->color_pair = CP_GREEN;
        t->passable = true;
        t->blocks_sight = false;
        break;
    case '&':
        t->type = TILE_DENSE_WOODS;
        t->glyph = '&';
        t->color_pair = CP_GREEN_BOLD;
        t->passable = false;
        t->blocks_sight = true;
        break;
    case '=':
        t->type = TILE_RIVER;
        t->glyph = '=';
        t->color_pair = CP_BLUE;
        t->passable = false;
        t->blocks_sight = false;
        break;
    case 'o':
        t->type = TILE_LAKE;
        t->glyph = 'o';
        t->color_pair = CP_BLUE;
        t->passable = false;
        t->blocks_sight = false;
        break;
    case 'H':
        t->type = TILE_BRIDGE;
        t->glyph = 'H';
        t->color_pair = CP_BROWN;
        t->passable = true;
        t->blocks_sight = false;
        break;
    default:
        t->type = TILE_NONE;
        t->glyph = ' ';
        t->color_pair = CP_DEFAULT;
        t->passable = false;
        t->blocks_sight = false;
        break;
    }
}

/* Add a location to the overworld */
static int ow_add_location(Overworld *ow, const char *name, LocationType type,
                            int x, int y, char glyph, short color) {
    if (ow->num_locations >= MAX_LOCATIONS) return -1;
    Location *loc = &ow->locations[ow->num_locations];
    snprintf(loc->name, MAX_NAME, "%s", name);
    loc->type = type;
    loc->pos = (Vec2){ x, y };
    loc->glyph = glyph;
    loc->color_pair = color;
    loc->discovered = false;
    loc->id = ow->num_locations;

    /* Place glyph on map (overwrite terrain) */
    ow->map[y][x].glyph = glyph;
    ow->map[y][x].color_pair = color;
    ow->map[y][x].passable = true;

    return ow->num_locations++;
}

void overworld_init(Overworld *ow) {
    memset(ow, 0, sizeof(*ow));

    /* Build terrain from raw map */
    for (int y = 0; y < OW_HEIGHT; y++) {
        const char *row = ow_raw_map[y];
        int len = (int)strlen(row);
        for (int x = 0; x < OW_WIDTH; x++) {
            char ch = (x < len) ? row[x] : '~';
            ow_tile_from_char(&ow->map[y][x], ch);
        }
    }

    /* ---- Towns ---- */
    ow_add_location(ow, "Camelot",      LOC_TOWN, 45, 22, '*', CP_YELLOW_BOLD);
    ow_add_location(ow, "London",       LOC_TOWN, 65, 28, '*', CP_WHITE_BOLD);
    ow_add_location(ow, "Canterbury",   LOC_TOWN, 75, 30, '*', CP_WHITE_BOLD);
    ow_add_location(ow, "Winchester",   LOC_TOWN, 52, 31, '*', CP_WHITE);
    ow_add_location(ow, "Glastonbury",  LOC_TOWN, 40, 29, '*', CP_MAGENTA);
    ow_add_location(ow, "Bath",         LOC_TOWN, 42, 27, '*', CP_CYAN);
    ow_add_location(ow, "Tintagel",     LOC_TOWN, 25, 27, '*', CP_WHITE);
    ow_add_location(ow, "Cornwall",     LOC_TOWN, 23, 33, '*', CP_WHITE);
    ow_add_location(ow, "Sherwood",     LOC_TOWN, 58, 18, '*', CP_GREEN_BOLD);
    ow_add_location(ow, "York",         LOC_TOWN, 55, 12, '*', CP_WHITE_BOLD);
    ow_add_location(ow, "Wales",        LOC_TOWN, 30, 20, '*', CP_RED);
    ow_add_location(ow, "Whitby",       LOC_TOWN, 62, 10, '*', CP_GRAY);
    ow_add_location(ow, "Llanthony",    LOC_TOWN, 33, 24, '*', CP_WHITE);
    ow_add_location(ow, "Carbonek",     LOC_TOWN, 48, 16, '*', CP_YELLOW);

    /* ---- Active Castles ---- */
    ow_add_location(ow, "Camelot Castle",  LOC_CASTLE_ACTIVE, 46, 22, '#', CP_YELLOW_BOLD);
    ow_add_location(ow, "Castle Surluise", LOC_CASTLE_ACTIVE, 38, 17, '#', CP_WHITE);
    ow_add_location(ow, "Castle Lothian",  LOC_CASTLE_ACTIVE, 50,  6, '#', CP_WHITE);
    ow_add_location(ow, "Castle Northumberland", LOC_CASTLE_ACTIVE, 55, 8, '#', CP_WHITE);
    ow_add_location(ow, "Castle Gore",     LOC_CASTLE_ACTIVE, 35, 15, '#', CP_WHITE);
    ow_add_location(ow, "Castle Strangore",LOC_CASTLE_ACTIVE, 42, 19, '#', CP_WHITE);
    ow_add_location(ow, "Castle Cradelment", LOC_CASTLE_ACTIVE, 28, 18, '#', CP_WHITE);
    ow_add_location(ow, "Castle Cornwall", LOC_CASTLE_ACTIVE, 22, 31, '#', CP_WHITE);
    ow_add_location(ow, "Castle Listenoise", LOC_CASTLE_ACTIVE, 60, 22, '#', CP_WHITE);
    ow_add_location(ow, "Castle Benwick",  LOC_CASTLE_ACTIVE, 70, 34, '#', CP_WHITE);
    ow_add_location(ow, "Castle Carados",  LOC_CASTLE_ACTIVE, 45,  5, '#', CP_WHITE);

    /* ---- Abandoned Castles ---- */
    ow_add_location(ow, "Castle Dolorous Garde", LOC_CASTLE_ABANDONED, 52, 14, '#', CP_GRAY);
    ow_add_location(ow, "Castle Perilous",       LOC_CASTLE_ABANDONED, 38, 25, '#', CP_MAGENTA);
    ow_add_location(ow, "Bamburgh Castle",        LOC_CASTLE_ABANDONED, 57,  5, '#', CP_GRAY);

    /* ---- Landmarks ---- */
    ow_add_location(ow, "Stonehenge",      LOC_LANDMARK, 48, 30, '+', CP_YELLOW);
    ow_add_location(ow, "Hadrian's Wall",  LOC_LANDMARK, 48,  4, '+', CP_WHITE);
    ow_add_location(ow, "White Cliffs",    LOC_LANDMARK, 78, 33, '+', CP_WHITE_BOLD);

    /* ---- Dungeon Entrances ---- */
    ow_add_location(ow, "Camelot Catacombs", LOC_DUNGEON_ENTRANCE, 47, 23, '>', CP_WHITE);
    ow_add_location(ow, "Tintagel Caves",    LOC_DUNGEON_ENTRANCE, 24, 28, '>', CP_WHITE);
    ow_add_location(ow, "Sherwood Depths",   LOC_DUNGEON_ENTRANCE, 59, 19, '>', CP_WHITE);
    ow_add_location(ow, "Glastonbury Tor",   LOC_DUNGEON_ENTRANCE, 41, 30, '>', CP_WHITE);
    ow_add_location(ow, "White Cliffs Cave", LOC_DUNGEON_ENTRANCE, 79, 34, '>', CP_CYAN);
    ow_add_location(ow, "Whitby Abbey",      LOC_DUNGEON_ENTRANCE, 63, 11, '>', CP_RED);

    /* ---- Volcano ---- */
    ow_add_location(ow, "Mount Draig",  LOC_VOLCANO, 27, 19, 'V', CP_RED_BOLD);
}

Location *overworld_location_at(Overworld *ow, int x, int y) {
    for (int i = 0; i < ow->num_locations; i++) {
        if (ow->locations[i].pos.x == x && ow->locations[i].pos.y == y) {
            return &ow->locations[i];
        }
    }
    return NULL;
}

bool overworld_is_passable(Overworld *ow, int x, int y) {
    if (x < 0 || x >= OW_WIDTH || y < 0 || y >= OW_HEIGHT) return false;
    return ow->map[y][x].passable;
}
