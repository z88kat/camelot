#ifndef OVERWORLD_H
#define OVERWORLD_H

#include "common.h"

/* Overworld map dimensions -- ~6.25x original (25% larger than previous) */
#define OW_WIDTH   500
#define OW_HEIGHT  250

/* Location types on the overworld */
typedef enum {
    LOC_NONE,
    LOC_TOWN,
    LOC_CASTLE_ACTIVE,
    LOC_CASTLE_ABANDONED,
    LOC_LANDMARK,
    LOC_DUNGEON_ENTRANCE,
    LOC_COTTAGE,
    LOC_CAVE,
    LOC_VOLCANO,
    LOC_MAGIC_CIRCLE,
    LOC_ABBEY
} LocationType;

/* A named location on the overworld */
typedef struct {
    char          name[MAX_NAME];
    LocationType  type;
    Vec2          pos;
    char          glyph;
    short         color_pair;
    bool          discovered;
    bool          visited;      /* for cottages/caves: has been explored */
    int           visited_day;  /* day when visited (for reset timer) */
    int           id;
} Location;

#define MAX_LOCATIONS 128

/* Wandering creature on the overworld */
typedef enum {
    OW_NPC_TRAVELLER,
    OW_NPC_PILGRIM,
    OW_NPC_MERCHANT,
    OW_NPC_PEASANT,
    OW_NPC_DEER,
    OW_NPC_SHEEP,
    OW_NPC_RABBIT,
    OW_NPC_CROW,
    OW_NPC_DRUID
} OWCreatureType;

typedef struct {
    OWCreatureType type;
    Vec2           pos;
    char           glyph;
    short          color_pair;
    char           name[MAX_NAME];
} OWCreature;

#define MAX_OW_CREATURES 40

/* Overworld state -- allocated on heap due to size */
typedef struct {
    Tile        map[OW_HEIGHT][OW_WIDTH];
    Location    locations[MAX_LOCATIONS];
    int         num_locations;
    OWCreature  creatures[MAX_OW_CREATURES];
    int         num_creatures;
} Overworld;

/* Initialize the overworld map and place all locations. */
void overworld_init(Overworld *ow);

/* Get the location at a given position, or NULL. */
Location *overworld_location_at(Overworld *ow, int x, int y);

/* Check if a tile is passable for the player. */
bool overworld_is_passable(Overworld *ow, int x, int y);

/* Spawn wandering creatures on the overworld. Call after overworld_init. */
void overworld_spawn_creatures(Overworld *ow);

/* Move all wandering creatures. Call once per overworld turn. */
void overworld_move_creatures(Overworld *ow, Vec2 player_pos);

/* Get the creature at a position, or NULL. */
OWCreature *overworld_creature_at(Overworld *ow, int x, int y);

#endif /* OVERWORLD_H */
