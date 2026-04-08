#include "spell.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static SpellDef spell_defs[MAX_SPELL_DEFS];
static int num_spells = 0;

static SpellAffiliation parse_affiliation(const char *s) {
    if (strcmp(s, "light") == 0)     return AFF_LIGHT;
    if (strcmp(s, "dark") == 0)      return AFF_DARK;
    if (strcmp(s, "nature") == 0)    return AFF_NATURE;
    return AFF_UNIVERSAL;
}

static SpellEffect parse_effect(const char *s) {
    if (strcmp(s, "damage") == 0)     return SEFF_DAMAGE;
    if (strcmp(s, "heal") == 0)       return SEFF_HEAL;
    if (strcmp(s, "buff_def") == 0)   return SEFF_BUFF_DEF;
    if (strcmp(s, "buff_str") == 0)   return SEFF_BUFF_STR;
    if (strcmp(s, "buff_spd") == 0)   return SEFF_BUFF_SPD;
    if (strcmp(s, "cure") == 0)       return SEFF_CURE;
    if (strcmp(s, "fear") == 0)       return SEFF_FEAR;
    if (strcmp(s, "teleport") == 0)   return SEFF_TELEPORT;
    if (strcmp(s, "detect") == 0)     return SEFF_DETECT;
    if (strcmp(s, "reveal") == 0)     return SEFF_REVEAL;
    if (strcmp(s, "dot") == 0)        return SEFF_DOT;
    if (strcmp(s, "root") == 0)       return SEFF_ROOT;
    if (strcmp(s, "shield") == 0)     return SEFF_SHIELD;
    if (strcmp(s, "summon") == 0)     return SEFF_SUMMON;
    if (strcmp(s, "drain") == 0)      return SEFF_DRAIN;
    if (strcmp(s, "debuff") == 0)     return SEFF_DEBUFF;
    if (strcmp(s, "push") == 0)       return SEFF_PUSH;
    if (strcmp(s, "invisible") == 0)  return SEFF_INVISIBLE;
    if (strcmp(s, "water_walk") == 0) return SEFF_WATER_WALK;
    return SEFF_DAMAGE;
}

void spell_init(void) {
    num_spells = 0;

    FILE *f = fopen("data/spells.csv", "r");
    if (!f) return;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        line[strcspn(line, "\n\r")] = 0;
        if (num_spells >= MAX_SPELL_DEFS) break;

        /* name,affiliation,threshold,mp_cost,damage,effect,duration,range,aoe,class_restrict,level_req */
        char name[48], aff_str[16], eff_str[24], class_str[16];
        int threshold, mp, dmg, dur, rng, aoe, lvl, min_lvl = 0;

        int n = sscanf(line, "%47[^,],%15[^,],%d,%d,%d,%23[^,],%d,%d,%d,%15[^,],%d,%d",
                       name, aff_str, &threshold, &mp, &dmg, eff_str,
                       &dur, &rng, &aoe, class_str, &lvl, &min_lvl);
        if (n < 11) continue;
        if (n < 12) min_lvl = 0;

        SpellDef *sp = &spell_defs[num_spells];
        snprintf(sp->name, MAX_NAME, "%s", name);
        sp->affiliation = parse_affiliation(aff_str);
        sp->threshold = threshold;
        sp->mp_cost = mp;
        sp->damage = dmg;
        sp->effect = parse_effect(eff_str);
        sp->duration = dur;
        sp->range = rng;
        sp->aoe = aoe;
        sp->level_required = lvl;
        sp->min_level = min_lvl;
        num_spells++;
    }
    fclose(f);
}

const SpellDef *spell_get_defs(int *count) {
    *count = num_spells;
    return spell_defs;
}

const SpellDef *spell_get(int index) {
    if (index < 0 || index >= num_spells) return NULL;
    return &spell_defs[index];
}
