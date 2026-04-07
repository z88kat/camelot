#include "trap.h"
#include "game.h"
#include "log.h"
#include "rng.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Builtin trap definitions -- mirrors the legacy TRAP_* enum order so that
 * trap_def_for_type() can index directly. Index 0 (TRAP_NONE) is a stub. */
static const TrapDef builtin_defs[] = {
    /* TRAP_NONE */
    { "None",           '.', CP_GRAY,    0,  0, "none",       0, "" },
    /* TRAP_PIT */
    { "Pit Trap",       '^', CP_BROWN,   100, 8, "pit",        0, "You fall into a pit trap! -%d HP" },
    /* TRAP_POISON_DART */
    { "Poison Dart",    '^', CP_GREEN,   100, 3, "dart",       0, "A poison dart shoots from the wall! -%d HP (poisoned!)" },
    /* TRAP_TRIPWIRE */
    { "Tripwire",       '^', CP_GRAY,    100, 0, "tripwire",   0, "You trip over a wire and stumble!" },
    /* TRAP_ARROW */
    { "Arrow Trap",     '^', CP_YELLOW,  100, 8, "dart",       0, "Arrows fly from the walls! -%d HP" },
    /* TRAP_ALARM */
    { "Alarm Trap",     '^', CP_RED,     100, 0, "alarm",      0, "ALARM! A loud bell rings! All enemies alerted!" },
    /* TRAP_BEAR */
    { "Bear Trap",      '^', CP_BROWN,   100, 4, "bear",       0, "A bear trap snaps shut on your leg! -%d HP" },
    /* TRAP_FIRE */
    { "Fire Trap",      '^', CP_RED,     100, 10, "fire",      0, "Flames erupt beneath you! -%d HP" },
    /* TRAP_TELEPORT */
    { "Teleport Trap",  '^', CP_MAGENTA, 100, 0, "teleport",   0, "A teleport trap! The world spins... you're somewhere else!" },
    /* TRAP_GAS */
    { "Gas Trap",       '^', CP_GREEN,   100, 0, "gas_poison", 0, "Poison gas fills the air! You feel confused." },
    /* TRAP_COLLAPSE */
    { "Collapsing Floor",'^',CP_GRAY,    100, 15, "collapse",  0, "The floor collapses beneath you! -%d HP" },
    /* TRAP_SPIKE */
    { "Spike Trap",     '^', CP_GRAY,    100, 6, "spike",      0, "Spikes shoot up from the floor! -%d HP" },
    /* TRAP_MANA_DRAIN */
    { "Mana Drain",     '^', CP_MAGENTA, 100, 0, "mana_drain", 15, "A magical trap drains your mana! -%d MP" },
};
static const int num_builtin = sizeof(builtin_defs)/sizeof(builtin_defs[0]);

static TrapDef loaded_defs[MAX_TRAP_DEFS];
static int num_loaded = 0;
static bool csv_loaded = false;

static short parse_color(const char *s) {
    if (!strcmp(s,"red")) return CP_RED;
    if (!strcmp(s,"green")) return CP_GREEN;
    if (!strcmp(s,"yellow")) return CP_YELLOW;
    if (!strcmp(s,"blue")) return CP_BLUE;
    if (!strcmp(s,"magenta")) return CP_MAGENTA;
    if (!strcmp(s,"cyan")) return CP_CYAN;
    if (!strcmp(s,"brown")) return CP_BROWN;
    if (!strcmp(s,"gray")) return CP_GRAY;
    if (!strcmp(s,"white")) return CP_WHITE;
    return CP_GRAY;
}

void trap_init(void) {
    FILE *f = fopen("data/traps.csv", "r");
    if (!f) { csv_loaded = false; return; }
    num_loaded = 0;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (line[0]=='#' || line[0]=='\n' || line[0]=='\r') continue;
        line[strcspn(line, "\n\r")] = 0;
        if (num_loaded >= MAX_TRAP_DEFS) break;
        /* name,glyph,color,trigger_chance,damage,effect,effect_param,message */
        char name[32], color[16], effect[16], message[96];
        char glyph;
        int chance, damage, param;
        int n = sscanf(line, "%31[^,],%c,%15[^,],%d,%d,%15[^,],%d,%95[^\n]",
                       name, &glyph, color, &chance, &damage, effect, &param, message);
        if (n < 8) continue;
        TrapDef *d = &loaded_defs[num_loaded++];
        snprintf(d->name, sizeof(d->name), "%s", name);
        d->glyph = glyph;
        d->color_pair = parse_color(color);
        d->trigger_chance = chance;
        d->damage = damage;
        snprintf(d->effect, sizeof(d->effect), "%s", effect);
        d->effect_param = param;
        snprintf(d->message, sizeof(d->message), "%s", message);
    }
    fclose(f);
    csv_loaded = (num_loaded > 0);
}

const TrapDef *trap_get_def(int id) {
    if (csv_loaded) {
        if (id < 0 || id >= num_loaded) return &builtin_defs[0];
        return &loaded_defs[id];
    }
    if (id < 0 || id >= num_builtin) return &builtin_defs[0];
    return &builtin_defs[id];
}

int trap_def_count(void) {
    return csv_loaded ? num_loaded : num_builtin;
}

const TrapDef *trap_def_for_type(TrapType t) {
    /* TrapType is an enum starting at TRAP_NONE=0; defs are aligned. */
    return trap_get_def((int)t);
}

int trap_apply_def(GameState *gs, const TrapDef *d) {
    if (!d) return 0;
    int dmg = d->damage;
    if (strcmp(d->effect, "pit") == 0) {
        dmg = rng_range(5, 10);
        gs->hp -= dmg;
        log_add(&gs->log, gs->turn, CP_RED, d->message, dmg);
    } else if (strcmp(d->effect, "dart") == 0) {
        gs->hp -= dmg;
        log_add(&gs->log, gs->turn, CP_RED, d->message, dmg);
    } else if (strcmp(d->effect, "tripwire") == 0) {
        log_add(&gs->log, gs->turn, CP_RED, "%s", d->message);
        dmg = 0;
    } else if (strcmp(d->effect, "alarm") == 0) {
        log_add(&gs->log, gs->turn, CP_RED, "%s", d->message);
        dmg = 0;
    } else if (strcmp(d->effect, "bear") == 0) {
        gs->hp -= dmg;
        log_add(&gs->log, gs->turn, CP_RED, d->message, dmg);
    } else if (strcmp(d->effect, "fire") == 0) {
        gs->hp -= dmg;
        log_add(&gs->log, gs->turn, CP_RED, d->message, dmg);
    } else if (strcmp(d->effect, "ice") == 0) {
        gs->hp -= dmg;
        log_add(&gs->log, gs->turn, CP_CYAN, d->message, dmg);
    } else if (strcmp(d->effect, "spike") == 0) {
        gs->hp -= dmg;
        log_add(&gs->log, gs->turn, CP_RED, d->message, dmg);
    } else if (strcmp(d->effect, "collapse") == 0) {
        gs->hp -= dmg;
        log_add(&gs->log, gs->turn, CP_RED, d->message, dmg);
    } else if (strcmp(d->effect, "teleport") == 0) {
        if (gs->dungeon) {
            DungeonLevel *dl = &gs->dungeon->levels[gs->dungeon->current_level];
            for (int tries = 0; tries < 200; tries++) {
                int rx = rng_range(2, MAP_WIDTH - 3);
                int ry = rng_range(2, MAP_HEIGHT - 3);
                if (dl->tiles[ry][rx].type == TILE_FLOOR) {
                    gs->player_pos = (Vec2){ rx, ry };
                    break;
                }
            }
        }
        log_add(&gs->log, gs->turn, CP_MAGENTA, "%s", d->message);
        dmg = 0;
    } else if (strcmp(d->effect, "gas_poison") == 0 ||
               strcmp(d->effect, "gas_sleep") == 0) {
        log_add(&gs->log, gs->turn, CP_GREEN, "%s", d->message);
        dmg = 0;
    } else if (strcmp(d->effect, "mana_drain") == 0) {
        int drain = d->effect_param > 0 ? rng_range(d->effect_param/2, d->effect_param) : rng_range(10, 20);
        gs->mp -= drain;
        if (gs->mp < 0) gs->mp = 0;
        log_add(&gs->log, gs->turn, CP_MAGENTA, d->message, drain);
        dmg = 0;
    } else if (strcmp(d->effect, "summon") == 0) {
        log_add(&gs->log, gs->turn, CP_RED, "%s", d->message);
        dmg = 0;
    } else {
        if (d->message[0]) log_add(&gs->log, gs->turn, CP_RED, "%s", d->message);
    }
    if (gs->hp < 1) gs->hp = 1;
    return dmg;
}
