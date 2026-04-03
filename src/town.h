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

#define MAX_TOWNS 40

/* Town interior map dimensions */
#define TOWN_MAP_W  60
#define TOWN_MAP_H  24

/* NPC types inside a town */
typedef enum {
    NPC_NONE,
    NPC_INNKEEPER,
    NPC_PRIEST,
    NPC_EQUIP_SHOP,
    NPC_POTION_SHOP,
    NPC_PAWN_SHOP,
    NPC_MYSTIC,
    NPC_BANKER,
    NPC_STABLE,
    NPC_WELL,       /* not really an NPC, but interactive */
    NPC_TOWNFOLK,    /* flavour NPC */
    NPC_DOG,         /* wanders, harmless */
    NPC_CAT,         /* wanders, harmless */
    NPC_CHICKEN      /* wanders, harmless */
} TownNPCType;

/* An NPC placed in the town interior */
typedef struct {
    TownNPCType type;
    Vec2        pos;
    char        glyph;
    short       color_pair;
    char        label[MAX_NAME];  /* "Innkeeper", "Priest", etc. */
    bool        wanders;         /* moves around randomly each turn */
} TownNPC;

#define MAX_TOWN_NPCS 16

/* A generated town interior map */
typedef struct {
    Tile     map[TOWN_MAP_H][TOWN_MAP_W];
    TownNPC  npcs[MAX_TOWN_NPCS];
    int      num_npcs;
    Vec2     entrance;   /* where the player spawns */
} TownMap;

/* Get the town definition for a given location name. Returns NULL if not a town. */
const TownDef *town_get_def(const char *name);

/* Initialize town definitions. */
void town_init(void);

/* Get number of defined towns. */
int town_count(void);

/* Get town def by index. */
const TownDef *town_get_by_index(int idx);

/* Generate a town interior map based on its services. */
void town_generate_map(TownMap *tm, const TownDef *td);

/* Get the NPC at a position, or NULL. */
TownNPC *town_npc_at(TownMap *tm, int x, int y);

/* Move wandering NPCs randomly. Call once per town turn. player_pos to avoid. */
void town_move_npcs(TownMap *tm, Vec2 player_pos);

#endif /* TOWN_H */
