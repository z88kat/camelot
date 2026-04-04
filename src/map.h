#ifndef MAP_H
#define MAP_H

#include "common.h"

/* Trap types */
typedef enum {
    TRAP_NONE,
    TRAP_PIT,
    TRAP_POISON_DART,
    TRAP_TRIPWIRE,
    TRAP_ARROW,
    TRAP_ALARM,
    TRAP_BEAR,
    TRAP_FIRE,
    TRAP_TELEPORT,
    TRAP_GAS,
    TRAP_COLLAPSE,
    TRAP_SPIKE,
    TRAP_MANA_DRAIN
} TrapType;

/* A trap placed on the map */
typedef struct {
    Vec2     pos;
    TrapType type;
    bool     revealed;  /* has been detected */
    bool     triggered; /* already triggered */
} Trap;

#define MAX_TRAPS 16

/* A single dungeon level */
typedef struct {
    Tile     tiles[MAP_HEIGHT][MAP_WIDTH];
    Trap     traps[MAX_TRAPS];
    int      num_traps;
    Vec2     stairs_up[2];   /* up to 2 sets of stairs up */
    Vec2     stairs_down[2]; /* up to 2 sets of stairs down */
    int      num_stairs_up;
    int      num_stairs_down;
    bool     generated;  /* has this level been generated? */
    int      depth;      /* 0 = first level */
} DungeonLevel;

/* A complete dungeon (all levels) */
typedef struct {
    char          name[MAX_NAME];
    DungeonLevel *levels;    /* array of dungeon levels (heap allocated) */
    int           max_depth; /* total number of levels */
    int           current_level;
    bool          has_portal; /* exit portal on deepest level */
} Dungeon;

/* Generate a dungeon level using BSP. */
void map_generate(DungeonLevel *level, int depth, int max_depth);

/* Create a new dungeon with the given number of levels. */
Dungeon *dungeon_create(const char *name, int num_levels);

/* Free a dungeon. */
void dungeon_free(Dungeon *d);

#endif /* MAP_H */
