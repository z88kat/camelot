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
} Entity;

#define MAX_MONSTERS_PER_LEVEL 40

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
} MonsterTemplate;

/* Get the monster template table. */
const MonsterTemplate *entity_get_templates(int *count);

/* Spawn monsters on a dungeon level based on depth. */
void entity_spawn_monsters(Entity monsters[], int *num_monsters,
                           Tile tiles[MAP_HEIGHT][MAP_WIDTH],
                           int depth, Vec2 player_pos);

#endif /* ENTITY_H */
