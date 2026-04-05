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

/* AI state machine */
typedef enum {
    AI_STATE_IDLE,    /* hasn't noticed player */
    AI_STATE_CHASE,   /* pursuing player */
    AI_STATE_FLEE     /* running away */
} AIState;

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
    AIState ai_state;     /* current FSM state */
    int     ability_cd;   /* turns until special ability ready */
    int     summon_cd;    /* turns until can summon again */
    int     last_seen_x, last_seen_y;  /* last known player position */
} Entity;

#define MAX_MONSTERS_PER_LEVEL 60

/* AI behaviour flags */
#define AI_FLEES_LOW_HP   (1 << 0)  /* flee when HP < 25% */
#define AI_ERRATIC        (1 << 1)  /* 25% random movement */
#define AI_RANGED_ATTACK  (1 << 2)  /* attack at range 2-5 */
#define AI_OPENS_DOORS    (1 << 3)  /* can open doors */
#define AI_GHOST          (1 << 4)  /* walks through walls */
#define AI_BREATHE_FIRE   (1 << 5)  /* breath weapon (3-tile line) */
#define AI_SUMMON         (1 << 6)  /* summons allies */
#define AI_HEAL_ALLIES    (1 << 7)  /* heals nearby allies */
#define AI_DEBUFF         (1 << 8)  /* curses/debuffs player */
#define AI_EXPLODE_DEATH  (1 << 9)  /* explodes on death */
#define AI_FEAR_AURA      (1 << 10) /* nearby monsters flee */
#define AI_ALWAYS_CHASE   (1 << 11) /* never idles, always chases */

/* Drop type */
typedef enum {
    DROP_NONE,
    DROP_GOLD,
    DROP_GOLD_LARGE,
    DROP_ITEM_WEAPON,   /* drops a depth-appropriate weapon */
    DROP_ITEM_ARMOR,    /* drops armor */
    DROP_ITEM_POTION,   /* drops a potion */
    DROP_ITEM_FOOD      /* drops food */
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

/* Initialize monster templates (load from CSV or use defaults). */
void entity_init(void);

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
