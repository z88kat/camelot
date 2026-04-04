#ifndef ENTITY_H
#define ENTITY_H

#include "common.h"

/* Monster category for spawning by depth */
typedef enum {
    MCAT_BEAST,
    MCAT_BANDIT,
    MCAT_UNDEAD,
    MCAT_DARK_KNIGHT,
    MCAT_MAGICAL,
    MCAT_DRAGON,
    MCAT_TROLL,
    MCAT_BOSS
} MonsterCategory;

/* A monster/entity on the map */
typedef struct {
    char    name[MAX_NAME];
    char    glyph;
    short   color_pair;
    int     hp, max_hp;
    int     str, def, spd;
    int     xp_reward;
    int     min_depth;   /* earliest dungeon depth to appear */
    int     max_depth;   /* latest dungeon depth (0 = any) */
    MonsterCategory category;
    Vec2    pos;
    bool    alive;
    int     energy;       /* speed/energy system */
    uint32_t ai_flags;
} Entity;

#define MAX_MONSTERS_PER_LEVEL 60

/* AI behaviour flags */
#define AI_FLEES_LOW_HP   (1 << 0)  /* flee when HP < 25% */
#define AI_ERRATIC        (1 << 1)  /* 25% random movement */
#define AI_RANGED_ATTACK  (1 << 2)  /* attack at range 2-5 */
#define AI_OPENS_DOORS    (1 << 3)  /* can open doors */

/* Drop type */
typedef enum {
    DROP_NONE,
    DROP_GOLD,
    DROP_GOLD_LARGE
} DropType;

/* Spawn group behaviour */
typedef enum {
    SPAWN_SINGLE,       /* always alone */
    SPAWN_SMALL_GROUP,  /* 2-4 of the same type together */
    SPAWN_LARGE_GROUP   /* 4-8 of the same type together */
} SpawnGroup;

/* Monster template for spawning */
typedef struct {
    char    name[MAX_NAME];
    char    glyph;
    short   color_pair;
    int     hp;
    int     str, def, spd;
    int     xp_reward;
    int     min_depth, max_depth;
    MonsterCategory category;
    int     frequency;     /* 1=very rare, 3=normal, 5=common */
    SpawnGroup group;
    uint32_t ai_flags;
    DropType drop;
} MonsterTemplate;

/* Get the monster template table. */
const MonsterTemplate *entity_get_templates(int *count);

/* Spawn monsters on a dungeon level based on depth. */
void entity_spawn_monsters(Entity monsters[], int *num_monsters,
                           Tile tiles[MAP_HEIGHT][MAP_WIDTH],
                           int depth, Vec2 player_pos);

/* Spawn a single monster on the level (for timed spawning). Returns true if spawned. */
bool entity_spawn_one(Entity monsters[], int *num_monsters,
                      Tile tiles[MAP_HEIGHT][MAP_WIDTH],
                      int depth, Vec2 player_pos, int fov_radius);

#endif /* ENTITY_H */
