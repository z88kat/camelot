#include "loot.h"
#include <stdio.h>
#include <string.h>

#define MAX_LOOT_ROWS 64

/* Builtin fallback mirrors the previously hardcoded drop ranges in game.c. */
static const LootRow builtin_rows[] = {
    { DROP_GOLD,        0, 99,  3, 15, 0, 0 },
    { DROP_GOLD_LARGE,  0, 99, 15, 50, 0, 0 },
    { DROP_ITEM_WEAPON, 0, 99,  0,  0, 100, 0 },
    { DROP_ITEM_ARMOR,  0, 99,  0,  0, 100, 0 },
    { DROP_ITEM_POTION, 0, 99,  0,  0, 100, 0 },
    { DROP_ITEM_FOOD,   0, 99,  0,  0, 100, 0 },
};
static const int num_builtin = sizeof(builtin_rows) / sizeof(builtin_rows[0]);

static LootRow loaded_rows[MAX_LOOT_ROWS];
static int num_loaded = 0;
static int csv_loaded = 0;

static DropType parse_drop(const char *s) {
    if (!strcmp(s, "none"))        return DROP_NONE;
    if (!strcmp(s, "gold"))        return DROP_GOLD;
    if (!strcmp(s, "gold_large"))  return DROP_GOLD_LARGE;
    if (!strcmp(s, "item_weapon")) return DROP_ITEM_WEAPON;
    if (!strcmp(s, "item_armor"))  return DROP_ITEM_ARMOR;
    if (!strcmp(s, "item_potion")) return DROP_ITEM_POTION;
    if (!strcmp(s, "item_food"))   return DROP_ITEM_FOOD;
    return DROP_NONE;
}

void loot_init(void) {
    FILE *f = fopen("data/loot_tables.csv", "r");
    if (!f) { csv_loaded = 0; return; }
    num_loaded = 0;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        line[strcspn(line, "\n\r")] = 0;
        if (num_loaded >= MAX_LOOT_ROWS) break;
        char type_str[24];
        int dmin, dmax, gmin, gmax, ic, rc;
        int n = sscanf(line, "%23[^,],%d,%d,%d,%d,%d,%d",
                       type_str, &dmin, &dmax, &gmin, &gmax, &ic, &rc);
        if (n < 7) continue;
        LootRow *r = &loaded_rows[num_loaded++];
        r->drop_type = parse_drop(type_str);
        r->depth_min = dmin;
        r->depth_max = dmax;
        r->gold_min  = gmin;
        r->gold_max  = gmax;
        r->item_chance = ic;
        r->rare_chance = rc;
    }
    fclose(f);
    csv_loaded = (num_loaded > 0);
}

const LootRow *loot_lookup(DropType drop, int depth) {
    const LootRow *rows = csv_loaded ? loaded_rows : builtin_rows;
    int n = csv_loaded ? num_loaded : num_builtin;
    for (int i = 0; i < n; i++) {
        if (rows[i].drop_type != drop) continue;
        int lo = rows[i].depth_min, hi = rows[i].depth_max;
        if (lo == 0 && hi == 0) return &rows[i];
        if (depth >= lo && depth <= hi) return &rows[i];
    }
    return NULL;
}
