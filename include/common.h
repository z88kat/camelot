#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stdint.h>

/* Map dimensions */
#define MAP_WIDTH    80
#define MAP_HEIGHT   22

/* UI layout */
#define SIDEBAR_WIDTH  18
#define MIN_TERM_WIDTH  (MAP_WIDTH + SIDEBAR_WIDTH + 1)
#define MIN_TERM_HEIGHT 30
#define STATUS_ROW     (MAP_HEIGHT)
#define LOG_START_ROW  (MAP_HEIGHT + 1)
#define LOG_LINES      5
#define LOG_BUFFER     200

/* Game limits */
#define MAX_LEVELS     20
#define MAX_ENTITIES   256
#define MAX_ITEMS      512
#define MAX_INVENTORY  26   /* a-z */
#define MAX_NAME       32
#define MAX_SPELLS     50

/* Color pair IDs */
#define CP_DEFAULT     0
#define CP_WHITE       1
#define CP_RED         2
#define CP_GREEN       3
#define CP_YELLOW      4
#define CP_BLUE        5
#define CP_MAGENTA     6
#define CP_CYAN        7
#define CP_BROWN       8
#define CP_GRAY        9
#define CP_WHITE_BOLD  10
#define CP_RED_BOLD    11
#define CP_GREEN_BOLD  12
#define CP_YELLOW_BOLD 13
#define CP_BLUE_BOLD   14
#define CP_MAGENTA_BOLD 15
#define CP_CYAN_BOLD   16
#define CP_STATUS_BAR  17
#define CP_LOG_DAMAGE  18
#define CP_LOG_HEAL    19
#define CP_LOG_QUEST   20
#define CP_LOG_LOOT    21

/* 2D vector */
typedef struct {
    int x, y;
} Vec2;

/* Tile types */
typedef enum {
    TILE_WALL,
    TILE_FLOOR,
    TILE_DOOR_CLOSED,
    TILE_DOOR_OPEN,
    TILE_STAIRS_DOWN,
    TILE_STAIRS_UP,
    TILE_WATER,
    TILE_LAVA,
    TILE_ICE,
    TILE_GRASS,
    TILE_ROAD,
    TILE_FOREST,
    TILE_HILLS,
    TILE_MOUNTAIN,
    TILE_MARSH,
    TILE_SWAMP,
    TILE_RIVER,
    TILE_LAKE,
    TILE_BRIDGE,
    TILE_DENSE_WOODS,
    TILE_PORTAL,
    TILE_GRAIL_PEDESTAL,
    TILE_NONE
} TileType;

/* A single map tile */
typedef struct {
    TileType type;
    bool     visible;    /* currently in FOV */
    bool     revealed;   /* ever seen (fog of war) */
    char     glyph;
    short    color_pair;
    bool     passable;
    bool     blocks_sight;
} Tile;

/* Game mode */
typedef enum {
    MODE_TITLE,
    MODE_CHARACTER_CREATE,
    MODE_OVERWORLD,
    MODE_TOWN,
    MODE_DUNGEON,
    MODE_ENCOUNTER,
    MODE_INVENTORY,
    MODE_SPELL,
    MODE_JOURNAL,
    MODE_HELP,
    MODE_SETTINGS,
    MODE_LOOK,
    MODE_DEATH,
    MODE_VICTORY
} GameMode;

/* Direction for movement */
typedef enum {
    DIR_NONE = -1,
    DIR_N, DIR_NE, DIR_E, DIR_SE,
    DIR_S, DIR_SW, DIR_W, DIR_NW
} Direction;

/* Direction delta table (indexed by Direction) */
static const int dir_dx[] = { 0,  1,  1,  1,  0, -1, -1, -1 };
static const int dir_dy[] = { -1, -1,  0,  1,  1,  1,  0, -1 };

#endif /* COMMON_H */
