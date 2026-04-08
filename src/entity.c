#include "entity.h"
#include "rng.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Monster template table                                              */
/* name, glyph, color, hp, str, def, spd, xp, min_depth, max_depth    */
/* ------------------------------------------------------------------ */

/*              name             glyph color      hp str def spd  xp min max category      freq group               ai_flags                  drop */
static const MonsterTemplate templates[] = {
    /* Beasts */
    { "Giant Rat",     'r', CP_BROWN,    6,  3, 1, 4,  5, 0, 3, MCAT_BEAST,   5, SPAWN_SMALL_GROUP, AI_FLEES_LOW_HP|AI_ERRATIC,         DROP_NONE },
    { "Bat Swarm",     'b', CP_GRAY,     4,  2, 0, 6,  3, 0, 4, MCAT_BEAST,   5, SPAWN_LARGE_GROUP, AI_ERRATIC,                          DROP_NONE },
    { "Adder",         's', CP_GREEN,    5,  4, 1, 5,  6, 0, 3, MCAT_BEAST,   4, SPAWN_SINGLE,      AI_FLEES_LOW_HP,                     DROP_NONE },
    { "Wild Boar",     'B', CP_BROWN,   10,  5, 2, 3,  8, 1, 5, MCAT_BEAST,   3, SPAWN_SINGLE,      0,                                   DROP_NONE },
    { "Wolf",          'w', CP_GRAY,    10,  5, 2, 5, 10, 1, 5, MCAT_BEAST,   4, SPAWN_SMALL_GROUP, AI_FLEES_LOW_HP,                     DROP_NONE },
    { "Giant Spider",  'S', CP_GREEN,   12,  6, 2, 4, 12, 2, 6, MCAT_BEAST,   3, SPAWN_SINGLE,      0,                                   DROP_NONE },
    { "Bear",          'U', CP_BROWN,   18,  8, 3, 3, 18, 3, 7, MCAT_BEAST,   2, SPAWN_SINGLE,      0,                                   DROP_NONE },
    { "Giant Beetle",  'a', CP_GREEN,    8,  4, 4, 2,  7, 1, 4, MCAT_BEAST,   4, SPAWN_SMALL_GROUP, AI_ERRATIC,                          DROP_NONE },

    /* Bandits */
    { "Bandit",        'p', CP_YELLOW,  12,  5, 3, 4, 10, 1, 5, MCAT_BANDIT,  4, SPAWN_SMALL_GROUP, AI_FLEES_LOW_HP|AI_OPENS_DOORS,     DROP_GOLD },
    { "Cutthroat",     'p', CP_RED,     14,  7, 2, 5, 14, 2, 6, MCAT_BANDIT,  3, SPAWN_SINGLE,      AI_FLEES_LOW_HP,                     DROP_GOLD },
    { "Highwayman",    'p', CP_YELLOW,  16,  6, 4, 4, 16, 3, 7, MCAT_BANDIT,  2, SPAWN_SINGLE,      AI_OPENS_DOORS,                      DROP_GOLD },
    { "Bandit Chief",  'P', CP_YELLOW,  22,  8, 5, 4, 25, 4, 8, MCAT_BANDIT,  1, SPAWN_SINGLE,      AI_OPENS_DOORS,                      DROP_GOLD_LARGE },

    /* Undead */
    { "Skeleton",      'z', CP_WHITE,   10,  5, 3, 3, 10, 2, 6, MCAT_UNDEAD,  4, SPAWN_SMALL_GROUP, 0,                                   DROP_GOLD },
    { "Zombie",        'Z', CP_GREEN,   14,  6, 2, 2, 12, 2, 6, MCAT_UNDEAD,  4, SPAWN_SMALL_GROUP, 0,                                   DROP_NONE },
    { "Ghoul",         'g', CP_CYAN,    16,  7, 3, 4, 16, 3, 7, MCAT_UNDEAD,  3, SPAWN_SINGLE,      0,                                   DROP_GOLD },
    { "Wight",         'W', CP_WHITE,   20,  8, 4, 3, 22, 4, 8, MCAT_UNDEAD,  2, SPAWN_SINGLE,      0,                                   DROP_GOLD },
    { "Wraith",        'W', CP_CYAN,    18,  9, 2, 5, 25, 5, 9, MCAT_UNDEAD,  2, SPAWN_SINGLE,      AI_ERRATIC,                          DROP_GOLD },
    { "Spectre",       'G', CP_CYAN,    15,  7, 1, 6, 20, 5, 9, MCAT_UNDEAD,  2, SPAWN_SINGLE,      AI_ERRATIC,                          DROP_GOLD },
    { "Skeleton Knight",'Z',CP_WHITE,   25, 10, 6, 3, 30, 6, 10, MCAT_UNDEAD, 1, SPAWN_SINGLE,      AI_OPENS_DOORS,                      DROP_GOLD_LARGE },
    { "Death Knight",  'D', CP_RED,     35, 12, 8, 4, 50, 8, 15, MCAT_UNDEAD, 1, SPAWN_SINGLE,      AI_OPENS_DOORS,                      DROP_GOLD_LARGE },

    /* Dark Knights */
    { "Dark Squire",   'q', CP_RED,     15,  6, 5, 3, 14, 3, 7, MCAT_DARK_KNIGHT, 3, SPAWN_SINGLE,  AI_OPENS_DOORS,                      DROP_GOLD },
    { "Mercenary",     'q', CP_YELLOW,  20,  8, 5, 4, 20, 4, 8, MCAT_DARK_KNIGHT, 2, SPAWN_SINGLE,  AI_OPENS_DOORS,                      DROP_GOLD },
    { "Rogue Knight",  'K', CP_RED,     25,  9, 7, 3, 28, 5, 9, MCAT_DARK_KNIGHT, 2, SPAWN_SINGLE,  AI_OPENS_DOORS,                      DROP_GOLD_LARGE },
    { "Black Knight",  'K', CP_RED,     30, 10, 8, 3, 35, 6, 10, MCAT_DARK_KNIGHT, 1, SPAWN_SINGLE, AI_OPENS_DOORS,                      DROP_GOLD_LARGE },
    { "Fallen Paladin",'K', CP_MAGENTA, 35, 11, 9, 3, 45, 7, 12, MCAT_DARK_KNIGHT, 1, SPAWN_SINGLE, AI_OPENS_DOORS,                     DROP_GOLD_LARGE },

    /* Magical */
    { "Imp",           'i', CP_RED,      8,  4, 1, 7,  8, 2, 6, MCAT_MAGICAL, 3, SPAWN_SMALL_GROUP, AI_ERRATIC|AI_RANGED_ATTACK,         DROP_NONE },
    { "Dark Monk",     'm', CP_MAGENTA, 18,  7, 5, 3, 20, 4, 8, MCAT_MAGICAL, 2, SPAWN_SINGLE,      AI_RANGED_ATTACK,                    DROP_GOLD },
    { "Hellhound",     'h', CP_RED,     20,  9, 3, 6, 24, 5, 9, MCAT_MAGICAL, 2, SPAWN_SINGLE,      AI_RANGED_ATTACK,                    DROP_NONE },
    { "Warlock",       'W', CP_MAGENTA, 22,  8, 4, 4, 28, 6, 10, MCAT_MAGICAL, 1, SPAWN_SINGLE,     AI_RANGED_ATTACK|AI_FLEES_LOW_HP,    DROP_GOLD_LARGE },
    { "Shadow Fiend",  'F', CP_MAGENTA, 25, 10, 3, 5, 32, 7, 12, MCAT_MAGICAL, 1, SPAWN_SINGLE,     AI_ERRATIC,                          DROP_GOLD_LARGE },

    /* Dragons */
    { "Dragon Whelp",  'd', CP_RED,     15,  7, 4, 4, 18, 4, 8, MCAT_DRAGON,  2, SPAWN_SINGLE,      AI_RANGED_ATTACK,                    DROP_GOLD_LARGE },
    { "Young Drake",   'd', CP_RED,     22,  9, 5, 4, 28, 5, 9, MCAT_DRAGON,  1, SPAWN_SINGLE,      AI_RANGED_ATTACK,                    DROP_GOLD_LARGE },
    { "Fire Drake",    'D', CP_RED,     30, 12, 6, 3, 40, 7, 12, MCAT_DRAGON, 1, SPAWN_SINGLE,      AI_RANGED_ATTACK,                    DROP_GOLD_LARGE },
    { "Wyvern",        'W', CP_RED,     28, 10, 5, 5, 35, 6, 11, MCAT_DRAGON, 1, SPAWN_SINGLE,      AI_RANGED_ATTACK|AI_FLEES_LOW_HP,    DROP_GOLD_LARGE },

    /* Trolls & Giants */
    { "Hill Troll",    'T', CP_GREEN,   25,  9, 5, 2, 25, 4, 8, MCAT_TROLL,   2, SPAWN_SINGLE,      AI_OPENS_DOORS,                      DROP_GOLD },
    { "Cave Troll",    'T', CP_BROWN,   30, 11, 6, 2, 32, 5, 10, MCAT_TROLL,  2, SPAWN_SINGLE,      AI_OPENS_DOORS,                      DROP_GOLD },
    { "Ogre",          'O', CP_BROWN,   28, 10, 4, 2, 28, 5, 9, MCAT_TROLL,   2, SPAWN_SINGLE,      0,                                   DROP_GOLD },
    { "Stone Giant",   'H', CP_GRAY,    40, 14, 8, 1, 50, 8, 15, MCAT_TROLL,  1, SPAWN_SINGLE,      0,                                   DROP_GOLD_LARGE },
};

static const int num_builtin_templates = sizeof(templates) / sizeof(templates[0]);

/* Dynamic templates loaded from CSV */
static MonsterTemplate loaded_templates[160];
static int num_loaded = 0;
static bool csv_loaded = false;

static short parse_color(const char *s) {
    if (strcmp(s, "brown") == 0) return CP_BROWN;
    if (strcmp(s, "gray") == 0) return CP_GRAY;
    if (strcmp(s, "green") == 0) return CP_GREEN;
    if (strcmp(s, "yellow") == 0) return CP_YELLOW;
    if (strcmp(s, "red") == 0) return CP_RED;
    if (strcmp(s, "white") == 0) return CP_WHITE;
    if (strcmp(s, "cyan") == 0) return CP_CYAN;
    if (strcmp(s, "magenta") == 0) return CP_MAGENTA;
    return CP_WHITE;
}

static MonsterCategory parse_category(const char *s) {
    if (strcmp(s, "beast") == 0) return MCAT_BEAST;
    if (strcmp(s, "bandit") == 0) return MCAT_BANDIT;
    if (strcmp(s, "undead") == 0) return MCAT_UNDEAD;
    if (strcmp(s, "dark_knight") == 0) return MCAT_DARK_KNIGHT;
    if (strcmp(s, "magical") == 0) return MCAT_MAGICAL;
    if (strcmp(s, "dragon") == 0) return MCAT_DRAGON;
    if (strcmp(s, "troll") == 0) return MCAT_TROLL;
    if (strcmp(s, "boss") == 0) return MCAT_BOSS;
    return MCAT_BEAST;
}

static SpawnGroup parse_group(const char *s) {
    if (strcmp(s, "small") == 0) return SPAWN_SMALL_GROUP;
    if (strcmp(s, "large") == 0) return SPAWN_LARGE_GROUP;
    return SPAWN_SINGLE;
}

static uint32_t parse_ai_flags(const char *s) {
    uint32_t flags = 0;
    if (strstr(s, "flee"))      flags |= AI_FLEES_LOW_HP;
    if (strstr(s, "erratic"))   flags |= AI_ERRATIC;
    if (strstr(s, "ranged"))    flags |= AI_RANGED_ATTACK;
    if (strstr(s, "doors"))     flags |= AI_OPENS_DOORS;
    if (strstr(s, "ghost"))     flags |= AI_GHOST;
    if (strstr(s, "breathe"))   flags |= AI_BREATHE_FIRE;
    if (strstr(s, "summon"))    flags |= AI_SUMMON;
    if (strstr(s, "heal"))      flags |= AI_HEAL_ALLIES;
    if (strstr(s, "debuff"))    flags |= AI_DEBUFF;
    if (strstr(s, "explode"))   flags |= AI_EXPLODE_DEATH;
    if (strstr(s, "fear_aura")) flags |= AI_FEAR_AURA;
    if (strstr(s, "chase"))     flags |= AI_ALWAYS_CHASE;
    return flags;
}

static DropType parse_drop(const char *s) {
    if (strcmp(s, "gold") == 0) return DROP_GOLD;
    if (strcmp(s, "gold_large") == 0) return DROP_GOLD_LARGE;
    if (strcmp(s, "weapon") == 0) return DROP_ITEM_WEAPON;
    if (strcmp(s, "armor") == 0) return DROP_ITEM_ARMOR;
    if (strcmp(s, "potion") == 0) return DROP_ITEM_POTION;
    if (strcmp(s, "food") == 0) return DROP_ITEM_FOOD;
    return DROP_NONE;
}

void entity_init(void) {
    FILE *f = fopen("data/monsters.csv", "r");
    if (!f) {
        csv_loaded = false;
        return;
    }

    num_loaded = 0;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        /* Skip comments and empty lines */
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;

        /* Remove trailing newline */
        line[strcspn(line, "\n\r")] = 0;

        /* Parse CSV: name,glyph,color,hp,str,def,spd,xp,min,max,cat,freq,group,ai,drop */
        char name[64], color[16], cat[16], grp[16], ai[64], drop[16];
        char glyph;
        int hp, str, def, spd, xp, min_d, max_d, freq;

        int n = sscanf(line, "%63[^,],%c,%15[^,],%d,%d,%d,%d,%d,%d,%d,%15[^,],%d,%15[^,],%63[^,],%15s",
                       name, &glyph, color, &hp, &str, &def, &spd, &xp,
                       &min_d, &max_d, cat, &freq, grp, ai, drop);
        if (n < 15) continue;

        if (num_loaded >= 128) break;
        MonsterTemplate *mt = &loaded_templates[num_loaded];
        snprintf(mt->name, MAX_NAME, "%s", name);
        mt->glyph = glyph;
        mt->color_pair = parse_color(color);
        mt->hp = hp;
        mt->str = str;
        mt->def = def;
        mt->spd = spd;
        mt->xp_reward = xp;
        mt->min_depth = min_d;
        mt->max_depth = max_d;
        mt->category = parse_category(cat);
        mt->frequency = freq;
        mt->group = parse_group(grp);
        mt->ai_flags = parse_ai_flags(ai);
        mt->drop = parse_drop(drop);
        num_loaded++;
    }
    fclose(f);
    csv_loaded = (num_loaded > 0);
}

const MonsterTemplate *entity_get_templates(int *count) {
    if (csv_loaded) {
        *count = num_loaded;
        return loaded_templates;
    }
    *count = num_builtin_templates;
    return templates;
}

/* Helper: try to place one monster at/near (x,y). Returns true if placed. */
static bool place_one(Entity monsters[], int *num_monsters,
                      Tile tiles[MAP_HEIGHT][MAP_WIDTH],
                      const MonsterTemplate *mt, int x, int y) {
    if (*num_monsters >= MAX_MONSTERS_PER_LEVEL) return false;
    if (x < 1 || x >= MAP_WIDTH - 1 || y < 1 || y >= MAP_HEIGHT - 1) return false;
    if (!tiles[y][x].passable) return false;
    char g = tiles[y][x].glyph;
    if (g == '<' || g == '>' || g == '=' || g == '_' || g == '0' || g == '(') return false;

    /* Don't overlap another monster */
    for (int e = 0; e < *num_monsters; e++)
        if (monsters[e].pos.x == x && monsters[e].pos.y == y) return false;

    Entity *ent = &monsters[*num_monsters];
    snprintf(ent->name, MAX_NAME, "%s", mt->name);
    ent->glyph = mt->glyph;
    ent->color_pair = mt->color_pair;
    ent->hp = mt->hp + rng_range(-2, 2);
    if (ent->hp < 1) ent->hp = 1;
    ent->max_hp = ent->hp;
    ent->str = mt->str;
    ent->def = mt->def;
    ent->spd = mt->spd;
    ent->xp_reward = mt->xp_reward;
    ent->min_depth = mt->min_depth;
    ent->max_depth = mt->max_depth;
    ent->category = mt->category;
    ent->pos = (Vec2){ x, y };
    ent->alive = true;
    ent->energy = 0;
    ent->ai_flags = mt->ai_flags;
    (*num_monsters)++;
    return true;
}

/* Weighted template selection based on frequency and depth proximity */
static const MonsterTemplate *pick_weighted(const MonsterTemplate *valid[], int *weights, int num_valid) {
    int total = 0;
    for (int i = 0; i < num_valid; i++) total += weights[i];
    if (total <= 0) return valid[0];

    int roll = rng_range(1, total);
    int cumulative = 0;
    for (int i = 0; i < num_valid; i++) {
        cumulative += weights[i];
        if (roll <= cumulative) return valid[i];
    }
    return valid[num_valid - 1];
}

void entity_spawn_monsters(Entity monsters[], int *num_monsters,
                           Tile tiles[MAP_HEIGHT][MAP_WIDTH],
                           int depth, Vec2 player_pos) {
    *num_monsters = 0;

    /* Build weighted list of valid templates */
    int total_templates;
    const MonsterTemplate *all_templates = entity_get_templates(&total_templates);
    const MonsterTemplate *valid[64];
    int weights[64];
    int num_valid = 0;
    for (int i = 0; i < total_templates; i++) {
        const MonsterTemplate *mt = &all_templates[i];

        /* Depth check with soft edges: 5% out-of-depth chance */
        bool in_range = (depth >= mt->min_depth &&
                         (mt->max_depth == 0 || depth <= mt->max_depth));
        bool out_of_depth = (!in_range && depth >= mt->min_depth - 1 &&
                             depth <= mt->max_depth + 2 && rng_chance(5));
        if (!in_range && !out_of_depth) continue;

        /* Weight = frequency * proximity to ideal depth */
        int ideal = (mt->min_depth + mt->max_depth) / 2;
        int dist = abs(depth - ideal);
        int w = mt->frequency * (10 - (dist > 5 ? 5 : dist));
        if (w < 1) w = 1;
        if (out_of_depth) w = 1;  /* very low weight for out-of-depth */

        valid[num_valid] = mt;
        weights[num_valid] = w;
        num_valid++;
        if (num_valid >= 64) break;
    }
    if (num_valid == 0) return;

    /* Pre-build list of all valid floor tiles for fast placement */
    typedef struct { int x, y; } Pos;
    static Pos floor_tiles[8000];
    int num_floors = 0;
    for (int y = 3; y < MAP_HEIGHT - 3; y++) {
        for (int x = 3; x < MAP_WIDTH - 3; x++) {
            if (!tiles[y][x].passable) continue;
            char g = tiles[y][x].glyph;
            if (g == '<' || g == '>' || g == '=' || g == '_' || g == '0' || g == '(') continue;
            /* Don't spawn right on top of player (3 tile buffer) */
            int dx = x - player_pos.x, dy = y - player_pos.y;
            if (dx * dx + dy * dy < 9) continue;
            if (num_floors < 8000) {
                floor_tiles[num_floors++] = (Pos){ x, y };
            }
        }
    }

    /* Debug: this value will be visible in the spawn count message */
    if (num_floors == 0) {
        /* No valid floor tiles found -- spawning impossible */
        return;
    }

    /* Shuffle the floor tiles for random placement order */
    for (int i = num_floors - 1; i > 0; i--) {
        int j = rng_range(0, i);
        Pos tmp = floor_tiles[i];
        floor_tiles[i] = floor_tiles[j];
        floor_tiles[j] = tmp;
    }

    /* Target monster count */
    int target = rng_range(25, 40 + depth * 3);
    if (target > MAX_MONSTERS_PER_LEVEL) target = MAX_MONSTERS_PER_LEVEL;

    int floor_idx = 0;
    int spawned = 0;
    while (spawned < target && *num_monsters < MAX_MONSTERS_PER_LEVEL && floor_idx < num_floors) {
        const MonsterTemplate *mt = pick_weighted(valid, weights, num_valid);

        /* Pick next available floor tile */
        int x = floor_tiles[floor_idx].x;
        int y = floor_tiles[floor_idx].y;
        floor_idx++;

        /* Minimum spacing: don't overlap other monsters */
        bool too_close = false;
        for (int e = 0; e < *num_monsters; e++) {
            if (monsters[e].pos.x == x && monsters[e].pos.y == y) { too_close = true; break; }
        }
        if (too_close) continue;

        if (!place_one(monsters, num_monsters, tiles, mt, x, y)) continue;
        spawned++;

        /* Group spawning: place same-type monsters nearby */
        if (mt->group == SPAWN_SMALL_GROUP || mt->group == SPAWN_LARGE_GROUP) {
            int group_size = (mt->group == SPAWN_SMALL_GROUP) ?
                             rng_range(1, 3) : rng_range(3, 7);
            for (int gi = 0; gi < group_size; gi++) {
                int gx = x + rng_range(-2, 2);
                int gy = y + rng_range(-2, 2);
                if (place_one(monsters, num_monsters, tiles, mt, gx, gy))
                    spawned++;
            }
        }
    }

    /* Second pass: find large open areas and ensure they have monsters.
       Scan a grid of sample points; if a point has many open tiles around it
       but no monsters nearby, force-spawn some. */
    for (int gy = 10; gy < MAP_HEIGHT - 10; gy += 8) {
        for (int gx = 10; gx < MAP_WIDTH - 10; gx += 10) {
            if (*num_monsters >= MAX_MONSTERS_PER_LEVEL) break;
            if (!tiles[gy][gx].passable) continue;

            /* Count floor tiles in a 6-tile radius */
            int floor_count = 0;
            for (int dy = -6; dy <= 6; dy++)
                for (int dx = -6; dx <= 6; dx++) {
                    int cx = gx + dx, cy = gy + dy;
                    if (cx > 0 && cx < MAP_WIDTH - 1 && cy > 0 && cy < MAP_HEIGHT - 1 &&
                        tiles[cy][cx].passable) floor_count++;
                }
            if (floor_count < 40) continue;  /* not a big room */

            /* Count monsters in same radius */
            int mon_count = 0;
            for (int e = 0; e < *num_monsters; e++) {
                int dx = monsters[e].pos.x - gx, dy2 = monsters[e].pos.y - gy;
                if (dx * dx + dy2 * dy2 < 49) mon_count++;
            }
            if (mon_count >= 2) continue;  /* already populated */

            /* Force-spawn 2-4 monsters in this room */
            int to_add = rng_range(2, 4);
            const MonsterTemplate *mt = pick_weighted(valid, weights, num_valid);
            for (int a = 0; a < to_add; a++) {
                int rx = gx + rng_range(-4, 4);
                int ry = gy + rng_range(-4, 4);
                place_one(monsters, num_monsters, tiles, mt, rx, ry);
            }
        }
    }
}

/* Spawn a single monster out of FOV (for timed respawning) */
bool entity_spawn_one(Entity monsters[], int *num_monsters,
                      Tile tiles[MAP_HEIGHT][MAP_WIDTH],
                      int depth, Vec2 player_pos, int fov_radius) {
    if (*num_monsters >= MAX_MONSTERS_PER_LEVEL) return false;

    /* Build valid list */
    int total_t;
    const MonsterTemplate *all_t = entity_get_templates(&total_t);
    const MonsterTemplate *valid[64];
    int weights[64];
    int num_valid = 0;
    for (int i = 0; i < total_t; i++) {
        if (depth >= all_t[i].min_depth &&
            (all_t[i].max_depth == 0 || depth <= all_t[i].max_depth)) {
            valid[num_valid] = &all_t[i];
            weights[num_valid] = all_t[i].frequency;
            num_valid++;
            if (num_valid >= 64) break;
        }
    }
    if (num_valid == 0) return false;

    const MonsterTemplate *mt = pick_weighted(valid, weights, num_valid);

    /* Place outside FOV */
    for (int tries = 0; tries < 500; tries++) {
        int x = rng_range(3, MAP_WIDTH - 4);
        int y = rng_range(3, MAP_HEIGHT - 4);
        if (!tiles[y][x].passable) continue;
        if (tiles[y][x].visible) continue;  /* must be outside FOV */

        int dx = x - player_pos.x, dy = y - player_pos.y;
        if (dx * dx + dy * dy < (fov_radius + 3) * (fov_radius + 3)) continue;

        return place_one(monsters, num_monsters, tiles, mt, x, y);
    }
    return false;
}
