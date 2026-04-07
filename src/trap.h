#ifndef TRAP_H
#define TRAP_H

/* Data-driven trap definitions.
 * Loaded from data/traps.csv with builtin fallback. Each TrapDef pairs a
 * canonical name + glyph/colour with a string-keyed effect that
 * trap_apply_def() switches on. The legacy TrapType enum in map.h still
 * exists for save compatibility -- defs are looked up by enum index. */

#include "common.h"
#include "map.h"

#define MAX_TRAP_DEFS 32

typedef struct {
    char  name[32];
    char  glyph;
    short color_pair;
    int   trigger_chance;   /* % chance per step (currently unused -- step is auto) */
    int   damage;           /* base damage */
    char  effect[16];       /* none|teleport|alarm|gas_poison|gas_sleep|mana_drain|summon|pit|dart|spike|fire|ice|tripwire|bear|collapse */
    int   effect_param;     /* numeric extra (mp drain etc.) */
    char  message[96];      /* shown when triggered (use printf-safe format -- only %d allowed for damage) */
} TrapDef;

/* Forward decl -- GameState is a typedef in game.h */
typedef struct GameState GameState;

void trap_init(void);
const TrapDef *trap_get_def(int id);
int trap_def_count(void);

/* Apply a trap def's effect to the player (game state). Returns damage dealt. */
int trap_apply_def(GameState *gs, const TrapDef *def);

/* Lookup the def for a TrapType enum value */
const TrapDef *trap_def_for_type(TrapType t);

#endif /* TRAP_H */
