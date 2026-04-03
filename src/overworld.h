#ifndef OVERWORLD_H
#define OVERWORLD_H

#include "common.h"

/* Overworld map dimensions -- 5x larger than original */
#define OW_WIDTH   400
#define OW_HEIGHT  200

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
    LOC_VOLCANO
} LocationType;

/* A named location on the overworld */
typedef struct {
    char          name[MAX_NAME];
    LocationType  type;
    Vec2          pos;
    char          glyph;
    short         color_pair;
    bool          discovered;
    int           id;
} Location;

#define MAX_LOCATIONS 128

/* Overworld state -- allocated on heap due to size */
typedef struct {
    Tile     map[OW_HEIGHT][OW_WIDTH];
    Location locations[MAX_LOCATIONS];
    int      num_locations;
} Overworld;

/* Initialize the overworld map and place all locations. */
void overworld_init(Overworld *ow);

/* Get the location at a given position, or NULL. */
Location *overworld_location_at(Overworld *ow, int x, int y);

/* Check if a tile is passable for the player. */
bool overworld_is_passable(Overworld *ow, int x, int y);

#endif /* OVERWORLD_H */
