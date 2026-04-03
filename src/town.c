#include "town.h"
#include "rng.h"
#include <string.h>
#include <stdio.h>

static TownDef towns[MAX_TOWNS];
static int num_towns = 0;

static void add_town(const char *name, int services, int inn_cost,
                     int beer_cost, const char *beer_name) {
    if (num_towns >= MAX_TOWNS) return;
    TownDef *t = &towns[num_towns++];
    snprintf(t->name, MAX_NAME, "%s", name);
    t->services = services;
    t->inn_cost = inn_cost;
    t->beer_cost = beer_cost;
    snprintf(t->beer_name, MAX_NAME, "%s", beer_name);
}

void town_init(void) {
    num_towns = 0;

    /* Major towns with full services */
    add_town("Camelot",
        SVC_INN | SVC_CHURCH | SVC_EQUIP_SHOP | SVC_POTION_SHOP | SVC_PAWN_SHOP |
        SVC_MYSTIC | SVC_BANK | SVC_WELL | SVC_STABLE,
        8, 2, "Camelot Ale");

    add_town("London",
        SVC_INN | SVC_CHURCH | SVC_EQUIP_SHOP | SVC_POTION_SHOP | SVC_PAWN_SHOP |
        SVC_MYSTIC | SVC_BANK | SVC_STABLE,
        10, 3, "London Porter");

    add_town("York",
        SVC_INN | SVC_CHURCH | SVC_EQUIP_SHOP | SVC_POTION_SHOP | SVC_PAWN_SHOP |
        SVC_BANK | SVC_STABLE,
        8, 2, "York Bitter");

    add_town("Winchester",
        SVC_INN | SVC_CHURCH | SVC_EQUIP_SHOP | SVC_PAWN_SHOP | SVC_BANK | SVC_STABLE,
        8, 2, "Winchester Mead");

    /* Medium towns */
    add_town("Canterbury",
        SVC_INN | SVC_CHURCH | SVC_POTION_SHOP | SVC_PAWN_SHOP | SVC_MYSTIC,
        7, 2, "Holy Water Ale");

    add_town("Glastonbury",
        SVC_INN | SVC_CHURCH | SVC_MYSTIC | SVC_POTION_SHOP | SVC_WELL,
        6, 2, "Glastonbury Cider");

    add_town("Bath",
        SVC_INN | SVC_CHURCH | SVC_POTION_SHOP | SVC_PAWN_SHOP,
        7, 2, "Roman Spring Ale");

    add_town("Sherwood",
        SVC_INN | SVC_EQUIP_SHOP | SVC_PAWN_SHOP | SVC_WELL,
        5, 2, "Sherwood Stout");

    add_town("Tintagel",
        SVC_INN | SVC_CHURCH | SVC_EQUIP_SHOP | SVC_WELL,
        6, 2, "Tintagel Dark");

    /* Smaller towns */
    add_town("Cornwall",
        SVC_INN | SVC_PAWN_SHOP | SVC_MYSTIC | SVC_EQUIP_SHOP,
        5, 2, "Cornish Scrumpy");

    add_town("Wales",
        SVC_INN | SVC_CHURCH | SVC_EQUIP_SHOP | SVC_WELL,
        5, 2, "Welsh Mead");

    add_town("Whitby",
        SVC_INN | SVC_CHURCH | SVC_PAWN_SHOP | SVC_POTION_SHOP,
        12, 3, "Whitby Stout");  /* double price per plan */

    add_town("Llanthony",
        SVC_INN | SVC_CHURCH | SVC_POTION_SHOP,
        4, 1, "Monk's Brew");

    add_town("Carbonek",
        SVC_INN | SVC_CHURCH | SVC_PAWN_SHOP,
        6, 2, "Grail Ale");

    /* Island towns */
    add_town("Isle of Wight",
        SVC_INN | SVC_PAWN_SHOP | SVC_EQUIP_SHOP,
        6, 2, "Island Bitter");

    add_town("Isle of Man",
        SVC_INN | SVC_EQUIP_SHOP | SVC_PAWN_SHOP,
        7, 2, "Manx Ale");

    add_town("Anglesey",
        SVC_INN | SVC_MYSTIC | SVC_WELL | SVC_PAWN_SHOP,
        5, 2, "Anglesey Mead");

    add_town("Orkney",
        SVC_INN | SVC_PAWN_SHOP | SVC_MYSTIC | SVC_EQUIP_SHOP,
        8, 3, "Orkney Whisky");

    /* Active castles -- each has an inn and various services */
    add_town("Camelot Castle",
        SVC_INN | SVC_EQUIP_SHOP | SVC_CHURCH,
        6, 2, "Royal Ale");

    add_town("Castle Surluise",
        SVC_INN | SVC_EQUIP_SHOP,
        6, 2, "Surluise Mead");

    add_town("Castle Lothian",
        SVC_INN | SVC_EQUIP_SHOP | SVC_PAWN_SHOP,
        7, 2, "Highland Ale");

    add_town("Castle Northumberland",
        SVC_INN | SVC_EQUIP_SHOP,
        7, 2, "Northern Bitter");

    add_town("Castle Gore",
        SVC_INN | SVC_EQUIP_SHOP | SVC_CHURCH,
        6, 2, "Gore Stout");

    add_town("Castle Strangore",
        SVC_INN | SVC_EQUIP_SHOP | SVC_PAWN_SHOP | SVC_POTION_SHOP,
        6, 2, "Strangore Draught");

    add_town("Castle Cradelment",
        SVC_INN | SVC_EQUIP_SHOP,
        6, 2, "Welsh Dragon Ale");

    add_town("Castle Cornwall",
        SVC_INN | SVC_PAWN_SHOP,
        7, 2, "Cornish Dark");

    add_town("Castle Listenoise",
        SVC_INN | SVC_CHURCH,
        5, 2, "Pilgrim's Rest");

    add_town("Castle Benwick",
        SVC_INN | SVC_EQUIP_SHOP | SVC_POTION_SHOP,
        8, 2, "French Wine");

    add_town("Castle Carados",
        SVC_INN | SVC_EQUIP_SHOP | SVC_PAWN_SHOP,
        7, 3, "Highland Whisky");

    add_town("Castle Ireland",
        SVC_INN | SVC_EQUIP_SHOP | SVC_MYSTIC,
        6, 2, "Irish Stout");

    add_town("Castle Gaul",
        SVC_INN | SVC_EQUIP_SHOP | SVC_POTION_SHOP | SVC_PAWN_SHOP,
        8, 3, "Gallic Wine");

    add_town("Castle Brittany",
        SVC_INN | SVC_EQUIP_SHOP | SVC_PAWN_SHOP,
        7, 2, "Breton Cider");
}

/* ------------------------------------------------------------------ */
/* Town interior map generation                                        */
/* ------------------------------------------------------------------ */

static void tm_set(Tile *t, TileType type, char glyph, short cp, bool pass) {
    t->type = type;
    t->glyph = glyph;
    t->color_pair = cp;
    t->passable = pass;
    t->blocks_sight = (type == TILE_WALL);
    t->visible = true;
    t->revealed = true;
}

/* Carve a building (room) with walls, floor, and a door */
static void carve_building(TownMap *tm, int x, int y, int w, int h, int door_side) {
    /* Walls */
    for (int by = y; by < y + h; by++) {
        for (int bx = x; bx < x + w; bx++) {
            if (bx < 0 || bx >= TOWN_MAP_W || by < 0 || by >= TOWN_MAP_H) continue;
            if (by == y || by == y + h - 1 || bx == x || bx == x + w - 1) {
                tm_set(&tm->map[by][bx], TILE_WALL, '#', CP_BROWN, false);
            } else {
                tm_set(&tm->map[by][bx], TILE_FLOOR, '.', CP_YELLOW, true);
            }
        }
    }
    /* Door -- placed on the side facing the courtyard */
    int dx, dy;
    switch (door_side) {
    case 0: dx = x + w / 2; dy = y + h - 1; break; /* south */
    case 1: dx = x + w / 2; dy = y;         break; /* north */
    case 2: dx = x;         dy = y + h / 2; break; /* west */
    case 3: dx = x + w - 1; dy = y + h / 2; break; /* east */
    default: dx = x + w / 2; dy = y + h - 1; break;
    }
    if (dx >= 0 && dx < TOWN_MAP_W && dy >= 0 && dy < TOWN_MAP_H) {
        tm_set(&tm->map[dy][dx], TILE_DOOR_OPEN, '/', CP_BROWN, true);
    }
}

/* Add an NPC inside a building */
static void add_npc(TownMap *tm, TownNPCType type, int x, int y,
                    char glyph, short cp, const char *label, bool wanders) {
    if (tm->num_npcs >= MAX_TOWN_NPCS) return;
    TownNPC *npc = &tm->npcs[tm->num_npcs++];
    npc->type = type;
    npc->pos = (Vec2){ x, y };
    npc->glyph = glyph;
    npc->color_pair = cp;
    npc->wanders = wanders;
    snprintf(npc->label, MAX_NAME, "%s", label);
}

void town_generate_map(TownMap *tm, const TownDef *td) {
    memset(tm, 0, sizeof(*tm));

    /* Fill with outdoor ground (courtyard) */
    for (int y = 0; y < TOWN_MAP_H; y++) {
        for (int x = 0; x < TOWN_MAP_W; x++) {
            if (y == 0 || y == TOWN_MAP_H - 1 || x == 0 || x == TOWN_MAP_W - 1) {
                /* Town border wall */
                tm_set(&tm->map[y][x], TILE_WALL, '#', CP_GRAY, false);
            } else {
                tm_set(&tm->map[y][x], TILE_FLOOR, '.', CP_GREEN, true);
            }
        }
    }

    /* Draw some paths through the courtyard */
    for (int x = 1; x < TOWN_MAP_W - 1; x++) {
        tm_set(&tm->map[TOWN_MAP_H / 2][x], TILE_ROAD, '.', CP_YELLOW, true);
    }
    for (int y = 1; y < TOWN_MAP_H - 1; y++) {
        tm_set(&tm->map[y][TOWN_MAP_W / 2], TILE_ROAD, '.', CP_YELLOW, true);
    }

    /* Entrance at bottom center */
    tm->entrance = (Vec2){ TOWN_MAP_W / 2, TOWN_MAP_H - 2 };
    tm_set(&tm->map[TOWN_MAP_H - 1][TOWN_MAP_W / 2], TILE_DOOR_OPEN, '/', CP_BROWN, true);

    /* Place buildings based on services -- arranged around the courtyard */
    /* Top row of buildings */
    int bw = 8, bh = 5;
    int slot = 0;

    typedef struct { int svc; char glyph; short cp; const char *label; TownNPCType ntype; } Bld;
    Bld buildings[] = {
        { SVC_INN,        'I', CP_BROWN,       "Innkeeper",    NPC_INNKEEPER },
        { SVC_CHURCH,     'P', CP_WHITE_BOLD,  "Priest",       NPC_PRIEST },
        { SVC_EQUIP_SHOP, '$', CP_YELLOW,      "Blacksmith",   NPC_EQUIP_SHOP },
        { SVC_POTION_SHOP,'!', CP_MAGENTA,     "Apothecary",   NPC_POTION_SHOP },
        { SVC_PAWN_SHOP,  'P', CP_GREEN,       "Pawnbroker",   NPC_PAWN_SHOP },
        { SVC_MYSTIC,     '?', CP_MAGENTA_BOLD,"Mystic",       NPC_MYSTIC },
        { SVC_BANK,       'B', CP_YELLOW_BOLD, "Banker",       NPC_BANKER },
        { SVC_STABLE,     'S', CP_BROWN,       "Stablemaster", NPC_STABLE },
    };
    int nbuildings = sizeof(buildings) / sizeof(buildings[0]);

    /* Layout positions: top-left, top-right, mid-left, mid-right, etc. */
    typedef struct { int x, y, door_side; } Slot;
    Slot slots[] = {
        {  3,  2, 0 },   /* top-left, door south */
        { 14,  2, 0 },   /* top-center-left */
        { 25,  2, 0 },   /* top-center */
        { 36,  2, 0 },   /* top-center-right */
        { 47,  2, 0 },   /* top-right */
        {  3, 16, 1 },   /* bottom-left, door north */
        { 14, 16, 1 },   /* bottom-center-left */
        { 25, 16, 1 },   /* bottom-center */
        { 36, 16, 1 },   /* bottom-center-right */
        { 47, 16, 1 },   /* bottom-right */
    };
    int nslots = sizeof(slots) / sizeof(slots[0]);

    slot = 0;
    for (int i = 0; i < nbuildings && slot < nslots; i++) {
        if (!(td->services & buildings[i].svc)) continue;

        Slot *s = &slots[slot++];
        carve_building(tm, s->x, s->y, bw, bh, s->door_side);

        /* Place NPC inside the building */
        int npc_x = s->x + bw / 2;
        int npc_y = (s->door_side == 0) ? s->y + 2 : s->y + bh - 3;
        add_npc(tm, buildings[i].ntype, npc_x, npc_y,
                buildings[i].glyph, buildings[i].cp, buildings[i].label, false);

        /* Label above the door */
        int label_y = (s->door_side == 0) ? s->y + bh : s->y - 1;
        if (label_y >= 0 && label_y < TOWN_MAP_H) {
            const char *lbl = buildings[i].label;
            int lx = s->x + 1;
            for (int c = 0; lbl[c] && lx + c < s->x + bw - 1 && lx + c < TOWN_MAP_W; c++) {
                Tile *lt = &tm->map[label_y][lx + c];
                if (lt->passable) {
                    lt->glyph = lbl[c];
                    lt->color_pair = buildings[i].cp;
                }
            }
        }
    }

    /* Well -- place in the courtyard if available */
    if (td->services & SVC_WELL) {
        int wx = TOWN_MAP_W / 2 + 5, wy = TOWN_MAP_H / 2 + 3;
        add_npc(tm, NPC_WELL, wx, wy, 'O', CP_BLUE, "Well", false);
    }

    /* Random townfolk (2-5) */
    int num_folk = rng_range(2, 5);
    for (int i = 0; i < num_folk; i++) {
        /* Find a random passable courtyard tile */
        for (int tries = 0; tries < 50; tries++) {
            int rx = rng_range(3, TOWN_MAP_W - 4);
            int ry = rng_range(3, TOWN_MAP_H - 4);
            if (tm->map[ry][rx].passable && !town_npc_at(tm, rx, ry)) {
                add_npc(tm, NPC_TOWNFOLK, rx, ry, '@', CP_WHITE, "Townfolk", true);
                break;
            }
        }
    }

    /* Random animals (0-4 total, random mix) */
    typedef struct { TownNPCType type; char glyph; short cp; const char *name; } AnimalDef;
    AnimalDef animals[] = {
        { NPC_DOG,     'd', CP_BROWN,  "Dog" },
        { NPC_CAT,     'c', CP_YELLOW, "Cat" },
        { NPC_CHICKEN, 'k', CP_WHITE,  "Chicken" },
    };
    int num_animals = rng_range(0, 4);
    for (int i = 0; i < num_animals; i++) {
        AnimalDef *a = &animals[rng_range(0, 2)];
        for (int tries = 0; tries < 50; tries++) {
            int rx = rng_range(3, TOWN_MAP_W - 4);
            int ry = rng_range(3, TOWN_MAP_H - 4);
            if (tm->map[ry][rx].passable && !town_npc_at(tm, rx, ry)) {
                add_npc(tm, a->type, rx, ry, a->glyph, a->cp, a->name, true);
                break;
            }
        }
    }
}

TownNPC *town_npc_at(TownMap *tm, int x, int y) {
    for (int i = 0; i < tm->num_npcs; i++) {
        if (tm->npcs[i].pos.x == x && tm->npcs[i].pos.y == y)
            return &tm->npcs[i];
    }
    return NULL;
}

void town_move_npcs(TownMap *tm, Vec2 player_pos) {
    for (int i = 0; i < tm->num_npcs; i++) {
        TownNPC *npc = &tm->npcs[i];
        if (!npc->wanders) continue;

        /* Only move ~40% of turns for a natural wandering pace */
        if (!rng_chance(40)) continue;

        /* Pick a random cardinal direction */
        int dirs[4][2] = { {0,-1}, {0,1}, {-1,0}, {1,0} };
        int d = rng_range(0, 3);
        /* Try a few directions if the first is blocked */
        for (int tries = 0; tries < 4; tries++) {
            int dd = (d + tries) & 3;
            int nx = npc->pos.x + dirs[dd][0];
            int ny = npc->pos.y + dirs[dd][1];

            if (nx <= 1 || nx >= TOWN_MAP_W - 2 || ny <= 1 || ny >= TOWN_MAP_H - 2)
                continue;
            if (!tm->map[ny][nx].passable) continue;
            if (town_npc_at(tm, nx, ny)) continue;  /* don't step on another NPC */
            if (nx == player_pos.x && ny == player_pos.y) continue;  /* don't step on player */

            npc->pos.x = nx;
            npc->pos.y = ny;
            break;
        }
    }
}

/* ------------------------------------------------------------------ */

const TownDef *town_get_def(const char *name) {
    for (int i = 0; i < num_towns; i++) {
        if (strcmp(towns[i].name, name) == 0)
            return &towns[i];
    }
    return NULL;
}

int town_count(void) { return num_towns; }

const TownDef *town_get_by_index(int idx) {
    if (idx < 0 || idx >= num_towns) return NULL;
    return &towns[idx];
}
