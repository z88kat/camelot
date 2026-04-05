#ifndef SPELL_H
#define SPELL_H

#include "common.h"

/* Spell affiliation */
typedef enum {
    AFF_UNIVERSAL,
    AFF_LIGHT,
    AFF_DARK,
    AFF_NATURE
} SpellAffiliation;

/* Spell effect type */
typedef enum {
    SEFF_DAMAGE,
    SEFF_HEAL,
    SEFF_BUFF_DEF,
    SEFF_BUFF_STR,
    SEFF_BUFF_SPD,
    SEFF_CURE,
    SEFF_FEAR,
    SEFF_TELEPORT,
    SEFF_DETECT,
    SEFF_REVEAL,
    SEFF_DOT,
    SEFF_ROOT,
    SEFF_SHIELD,
    SEFF_SUMMON,
    SEFF_DRAIN,
    SEFF_DEBUFF,
    SEFF_PUSH,
    SEFF_INVISIBLE,
    SEFF_WATER_WALK
} SpellEffect;

/* Spell definition */
typedef struct {
    char            name[MAX_NAME];
    SpellAffiliation affiliation;
    int             threshold;      /* affinity required (0-100) */
    int             mp_cost;
    int             damage;
    SpellEffect     effect;
    int             duration;       /* turns, 0 = instant */
    int             range;          /* tiles, 0 = self */
    int             aoe;            /* radius, 0 = single target */
    int             level_required;
} SpellDef;

#define MAX_SPELL_DEFS 60

/* Initialize spell definitions (load from CSV). */
void spell_init(void);

/* Get the spell definition table. */
const SpellDef *spell_get_defs(int *count);

/* Get spell by index. Returns NULL if invalid. */
const SpellDef *spell_get(int index);

#endif /* SPELL_H */
