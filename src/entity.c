#include "entity.h"
#include "rng.h"
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Monster template table                                              */
/* name, glyph, color, hp, str, def, spd, xp, min_depth, max_depth    */
/* ------------------------------------------------------------------ */

static const MonsterTemplate templates[] = {
    /* Beasts -- early game */
    { "Giant Rat",     'r', CP_BROWN,    6,  3, 1, 4,  5, 0, 3, MCAT_BEAST },
    { "Bat Swarm",     'b', CP_GRAY,     4,  2, 0, 6,  3, 0, 4, MCAT_BEAST },
    { "Adder",         's', CP_GREEN,    5,  4, 1, 5,  6, 0, 3, MCAT_BEAST },
    { "Wild Boar",     'B', CP_BROWN,   10,  5, 2, 3,  8, 1, 5, MCAT_BEAST },
    { "Wolf",          'w', CP_GRAY,    10,  5, 2, 5, 10, 1, 5, MCAT_BEAST },
    { "Giant Spider",  'S', CP_GREEN,   12,  6, 2, 4, 12, 2, 6, MCAT_BEAST },
    { "Bear",          'U', CP_BROWN,   18,  8, 3, 3, 18, 3, 7, MCAT_BEAST },
    { "Giant Beetle",  'a', CP_GREEN,    8,  4, 4, 2,  7, 1, 4, MCAT_BEAST },

    /* Bandits -- roads and early dungeons */
    { "Bandit",        'p', CP_YELLOW,  12,  5, 3, 4, 10, 1, 5, MCAT_BANDIT },
    { "Cutthroat",     'p', CP_RED,     14,  7, 2, 5, 14, 2, 6, MCAT_BANDIT },
    { "Highwayman",    'p', CP_YELLOW,  16,  6, 4, 4, 16, 3, 7, MCAT_BANDIT },
    { "Bandit Chief",  'P', CP_YELLOW,  22,  8, 5, 4, 25, 4, 8, MCAT_BANDIT },

    /* Undead -- dungeons and abandoned castles */
    { "Skeleton",      'z', CP_WHITE,   10,  5, 3, 3, 10, 2, 6, MCAT_UNDEAD },
    { "Zombie",        'Z', CP_GREEN,   14,  6, 2, 2, 12, 2, 6, MCAT_UNDEAD },
    { "Ghoul",         'g', CP_CYAN,    16,  7, 3, 4, 16, 3, 7, MCAT_UNDEAD },
    { "Wight",         'W', CP_WHITE,   20,  8, 4, 3, 22, 4, 8, MCAT_UNDEAD },
    { "Wraith",        'W', CP_CYAN,    18,  9, 2, 5, 25, 5, 9, MCAT_UNDEAD },
    { "Spectre",       'G', CP_CYAN,    15,  7, 1, 6, 20, 5, 9, MCAT_UNDEAD },
    { "Skeleton Knight",'Z',CP_WHITE,   25, 10, 6, 3, 30, 6, 10, MCAT_UNDEAD },
    { "Death Knight",  'D', CP_RED,     35, 12, 8, 4, 50, 8, 15, MCAT_UNDEAD },

    /* Dark Knights -- mid/late game */
    { "Dark Squire",   'q', CP_RED,     15,  6, 5, 3, 14, 3, 7, MCAT_DARK_KNIGHT },
    { "Mercenary",     'q', CP_YELLOW,  20,  8, 5, 4, 20, 4, 8, MCAT_DARK_KNIGHT },
    { "Rogue Knight",  'K', CP_RED,     25,  9, 7, 3, 28, 5, 9, MCAT_DARK_KNIGHT },
    { "Black Knight",  'K', CP_RED,     30, 10, 8, 3, 35, 6, 10, MCAT_DARK_KNIGHT },
    { "Fallen Paladin",'K', CP_MAGENTA, 35, 11, 9, 3, 45, 7, 12, MCAT_DARK_KNIGHT },

    /* Magical creatures */
    { "Imp",           'i', CP_RED,      8,  4, 1, 7,  8, 2, 6, MCAT_MAGICAL },
    { "Dark Monk",     'm', CP_MAGENTA, 18,  7, 5, 3, 20, 4, 8, MCAT_MAGICAL },
    { "Hellhound",     'h', CP_RED,     20,  9, 3, 6, 24, 5, 9, MCAT_MAGICAL },
    { "Warlock",       'W', CP_MAGENTA, 22,  8, 4, 4, 28, 6, 10, MCAT_MAGICAL },
    { "Shadow Fiend",  'F', CP_MAGENTA, 25, 10, 3, 5, 32, 7, 12, MCAT_MAGICAL },

    /* Dragons */
    { "Dragon Whelp",  'd', CP_RED,     15,  7, 4, 4, 18, 4, 8, MCAT_DRAGON },
    { "Young Drake",   'd', CP_RED,     22,  9, 5, 4, 28, 5, 9, MCAT_DRAGON },
    { "Fire Drake",    'D', CP_RED,     30, 12, 6, 3, 40, 7, 12, MCAT_DRAGON },
    { "Wyvern",        'W', CP_RED,     28, 10, 5, 5, 35, 6, 11, MCAT_DRAGON },

    /* Trolls & Giants */
    { "Hill Troll",    'T', CP_GREEN,   25,  9, 5, 2, 25, 4, 8, MCAT_TROLL },
    { "Cave Troll",    'T', CP_BROWN,   30, 11, 6, 2, 32, 5, 10, MCAT_TROLL },
    { "Ogre",          'O', CP_BROWN,   28, 10, 4, 2, 28, 5, 9, MCAT_TROLL },
    { "Stone Giant",   'H', CP_GRAY,    40, 14, 8, 1, 50, 8, 15, MCAT_TROLL },
};

static const int num_templates = sizeof(templates) / sizeof(templates[0]);

const MonsterTemplate *entity_get_templates(int *count) {
    *count = num_templates;
    return templates;
}

void entity_spawn_monsters(Entity monsters[], int *num_monsters,
                           Tile tiles[MAP_HEIGHT][MAP_WIDTH],
                           int depth, Vec2 player_pos) {
    *num_monsters = 0;

    /* How many monsters for this level */
    int target = rng_range(12, 20 + depth * 2);
    if (target > MAX_MONSTERS_PER_LEVEL) target = MAX_MONSTERS_PER_LEVEL;

    /* Build list of valid templates for this depth */
    const MonsterTemplate *valid[64];
    int num_valid = 0;
    for (int i = 0; i < num_templates; i++) {
        if (depth >= templates[i].min_depth &&
            (templates[i].max_depth == 0 || depth <= templates[i].max_depth)) {
            valid[num_valid++] = &templates[i];
            if (num_valid >= 64) break;
        }
    }
    if (num_valid == 0) return;

    for (int m = 0; m < target; m++) {
        /* Pick a random valid template */
        const MonsterTemplate *mt = valid[rng_range(0, num_valid - 1)];

        /* Find a floor tile away from the player */
        for (int tries = 0; tries < 200; tries++) {
            int x = rng_range(3, MAP_WIDTH - 4);
            int y = rng_range(3, MAP_HEIGHT - 4);

            if (tiles[y][x].type != TILE_FLOOR) continue;
            if (tiles[y][x].glyph != '.') continue;

            /* Not too close to player */
            int dx = x - player_pos.x;
            int dy = y - player_pos.y;
            if (dx * dx + dy * dy < 25) continue;  /* at least 5 tiles away */

            /* Not on another monster */
            bool occupied = false;
            for (int e = 0; e < *num_monsters; e++) {
                if (monsters[e].pos.x == x && monsters[e].pos.y == y) {
                    occupied = true;
                    break;
                }
            }
            if (occupied) continue;

            /* Spawn the monster */
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
            (*num_monsters)++;
            break;
        }
    }
}
