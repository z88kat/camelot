#ifndef LOOT_H
#define LOOT_H

#include "entity.h"

/* Loot table row (see data/loot_tables.csv) */
typedef struct {
    DropType drop_type;
    int      depth_min;
    int      depth_max;
    int      gold_min;
    int      gold_max;
    int      item_chance;
    int      rare_chance;
} LootRow;

void loot_init(void);

/* Look up a loot row for a given drop type and dungeon depth.
 * Returns NULL if no match. */
const LootRow *loot_lookup(DropType drop, int depth);

#endif /* LOOT_H */
