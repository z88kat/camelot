#include "ammo.h"
#include "common.h"
#include <stdio.h>
#include <string.h>

#define MAX_AMMO 16

static const AmmoDef builtin[] = {
    { "Arrows",          '/', CP_BROWN,  1, 5,  3, 50 },
    { "Bolts",           '/', CP_GRAY,   2, 5,  5, 50 },
    { "Stones",          '/', CP_GRAY,   0, 3,  1,  0 },
    { "Throwing Knives", '/', CP_WHITE,  2, 3,  8, 40 },
    { "Silver Arrows",   '/', CP_WHITE,  3, 5, 15, 60 },
    { "Fire Arrows",     '/', CP_RED,    2, 5, 12, 30 },
};
static const int num_builtin = sizeof(builtin) / sizeof(builtin[0]);

static AmmoDef loaded[MAX_AMMO];
static int num_loaded = 0;
static int csv_loaded = 0;

static short parse_color(const char *s) {
    if (!strcmp(s, "red"))     return CP_RED;
    if (!strcmp(s, "green"))   return CP_GREEN;
    if (!strcmp(s, "yellow"))  return CP_YELLOW;
    if (!strcmp(s, "blue"))    return CP_BLUE;
    if (!strcmp(s, "magenta")) return CP_MAGENTA;
    if (!strcmp(s, "cyan"))    return CP_CYAN;
    if (!strcmp(s, "brown"))   return CP_BROWN;
    if (!strcmp(s, "gray"))    return CP_GRAY;
    if (!strcmp(s, "white"))   return CP_WHITE;
    return CP_GRAY;
}

void ammo_init(void) {
    FILE *f = fopen("data/ammo.csv", "r");
    if (!f) { csv_loaded = 0; return; }
    num_loaded = 0;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        line[strcspn(line, "\n\r")] = 0;
        if (num_loaded >= MAX_AMMO) break;
        char name[32], color[16];
        char glyph;
        int dmg, wt, val, rec;
        int n = sscanf(line, "%31[^,],%c,%15[^,],%d,%d,%d,%d",
                       name, &glyph, color, &dmg, &wt, &val, &rec);
        if (n < 7) continue;
        AmmoDef *a = &loaded[num_loaded++];
        snprintf(a->name, sizeof(a->name), "%s", name);
        a->glyph = glyph;
        a->color_pair = parse_color(color);
        a->damage_bonus = dmg;
        a->weight = wt;
        a->value = val;
        a->recovery_chance = rec;
    }
    fclose(f);
    csv_loaded = (num_loaded > 0);
}

int ammo_def_count(void) {
    return csv_loaded ? num_loaded : num_builtin;
}

const AmmoDef *ammo_get(int i) {
    const AmmoDef *arr = csv_loaded ? loaded : builtin;
    int n = csv_loaded ? num_loaded : num_builtin;
    if (i < 0 || i >= n) return NULL;
    return &arr[i];
}

const AmmoDef *ammo_find_exact(const char *name) {
    int n = ammo_def_count();
    for (int i = 0; i < n; i++) {
        const AmmoDef *a = ammo_get(i);
        if (strcmp(a->name, name) == 0) return a;
    }
    return NULL;
}

const AmmoDef *ammo_find(const char *item_name) {
    /* Prefer the longest matching name (so "Silver Arrows" beats "Arrows"). */
    int n = ammo_def_count();
    const AmmoDef *best = NULL;
    size_t best_len = 0;
    for (int i = 0; i < n; i++) {
        const AmmoDef *a = ammo_get(i);
        if (strstr(item_name, a->name)) {
            size_t l = strlen(a->name);
            if (l > best_len) { best = a; best_len = l; }
        }
    }
    return best;
}
