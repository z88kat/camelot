#ifndef TOWN_H
#define TOWN_H

#include "common.h"

/* Services available in a town */
#define SVC_INN        (1 << 0)
#define SVC_CHURCH     (1 << 1)
#define SVC_EQUIP_SHOP (1 << 2)
#define SVC_POTION_SHOP (1 << 3)
#define SVC_PAWN_SHOP  (1 << 4)
#define SVC_MYSTIC     (1 << 5)
#define SVC_BANK       (1 << 6)
#define SVC_WELL       (1 << 7)
#define SVC_STABLE     (1 << 8)

/* Town definition */
typedef struct {
    char     name[MAX_NAME];
    int      services;     /* bitmask of SVC_* flags */
    int      inn_cost;     /* gold to rest at inn */
    int      beer_cost;    /* gold per pint */
    char     beer_name[MAX_NAME]; /* local beer flavour */
} TownDef;

#define MAX_TOWNS 32

/* Get the town definition for a given location name. Returns NULL if not a town. */
const TownDef *town_get_def(const char *name);

/* Initialize town definitions. */
void town_init(void);

/* Get number of defined towns. */
int town_count(void);

/* Get town def by index. */
const TownDef *town_get_by_index(int idx);

#endif /* TOWN_H */
