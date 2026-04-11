#ifndef TILESET_H
#define TILESET_H

#include <stdbool.h>

#define TILE_ID_MAX    64
#define TILE_SHEET_MAX 64
#define MAX_TILE_ENTRIES 512

typedef struct {
    char id[TILE_ID_MAX];       /* e.g. "tile_wall" */
    char name[64];              /* e.g. "Wall" */
    char sheet[TILE_SHEET_MAX]; /* e.g. "Objects/Wall.png" */
    int  tx, ty;                /* tile column,row within sheet (0-indexed) */
} TileEntry;

typedef struct {
    TileEntry entries[MAX_TILE_ENTRIES];
    int count;
} Tileset;

/* Load tileset mapping from CSV file. Returns true on success. */
bool tileset_load(Tileset *ts, const char *path);

/* Look up a tile entry by id. Returns NULL if not found. */
const TileEntry *tileset_find(const Tileset *ts, const char *id);

/* Look up a tile entry by display name. Returns NULL if not found. */
const TileEntry *tileset_find_by_name(const Tileset *ts, const char *name);

#endif /* TILESET_H */
