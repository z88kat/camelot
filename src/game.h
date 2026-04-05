#ifndef GAME_H
#define GAME_H

#include "common.h"
#include "log.h"
#include "overworld.h"
#include "town.h"
#include "quest.h"
#include "map.h"
#include "fov.h"
#include "item.h"
#include "spell.h"

/* Player class */
typedef enum {
    CLASS_KNIGHT,
    CLASS_WIZARD,
    CLASS_RANGER
} PlayerClass;

/* Player gender */
typedef enum {
    GENDER_MALE,
    GENDER_FEMALE
} PlayerGender;

/* Weather types */
typedef enum {
    WEATHER_CLEAR,
    WEATHER_RAIN,
    WEATHER_STORM,
    WEATHER_FOG,
    WEATHER_SNOW,
    WEATHER_WIND
} WeatherType;

/* Time of day periods */
typedef enum {
    TOD_NIGHT,      /* 22:00 - 04:59 */
    TOD_DAWN,       /* 05:00 - 06:59 */
    TOD_MORNING,    /* 07:00 - 11:59 */
    TOD_MIDDAY,     /* 12:00 - 13:59 */
    TOD_AFTERNOON,  /* 14:00 - 16:59 */
    TOD_DUSK,       /* 17:00 - 18:59 */
    TOD_EVENING     /* 19:00 - 21:59 */
} TimeOfDay;

typedef struct {
    /* Current dungeon (heap allocated, NULL if not in dungeon) */
    Dungeon   *dungeon;

    /* Overworld (heap-allocated due to size) */
    Overworld  *overworld;

    /* Player */
    Vec2       player_pos;
    Vec2       ow_player_pos;
    char       player_name[MAX_NAME];
    PlayerClass player_class;
    PlayerGender player_gender;
    int        player_level;
    int        max_spells_capacity;  /* max spells this class can learn */
    int        appearance[4];        /* 0=hair, 1=eyes, 2=build, 3=feature */
    bool       pending_levelup;      /* waiting for player to choose stat */
    int        hp, max_hp;
    int        mp, max_mp;
    int        str, def, intel, spd;
    int        gold;
    bool       in_boat;     /* currently sailing across water */
    bool       has_torch;   /* carrying a lit torch */
    int        horse_type;  /* 0=none, 1=Pony, 2=Palfrey, 3=Destrier */
    bool       riding;      /* currently mounted */
    int        horse_wait_turns; /* turns horse has been waiting outside */
    int        beers_drunk; /* current drunkenness counter */
    int        drunk_turns; /* turns of drunkenness remaining */
    const TownDef *current_town; /* town we're currently in (NULL if not in town) */
    TownMap    town_map;         /* current town interior map */
    Vec2       town_player_pos;  /* player position inside town */
    bool       well_explored;    /* has the well been explored this visit? */
    bool       confessed;        /* has player confessed at church this visit? */
    /* Quests */
    QuestLog   quests;

    bool       stonehenge_used;  /* has player used Stonehenge this game? */
    bool       church_looted;    /* has player looted a church? */
    int        weight, max_weight;
    int        chivalry;
    int        xp;
    int        kills;

    /* Inventory & Equipment */
    Item       inventory[MAX_INVENTORY]; /* 26 slots a-z */
    int        num_items;               /* items in inventory */
    Item       equipment[NUM_SLOTS];    /* equipped items */

    /* Spells */
    int        spells_known[MAX_SPELLS]; /* spell def indices, -1 = empty */
    int        num_spells;
    int        light_affinity;           /* 0-100 */
    int        dark_affinity;
    int        nature_affinity;
    int        shield_absorb;            /* damage absorbed by Shield spell */
    int        buff_str_turns;           /* temporary STR buff remaining */
    int        buff_def_turns;
    int        buff_spd_turns;

    /* Potion identification system */
    bool       potions_identified[256];   /* potions_identified[template_id] = true */
    int        potion_colour_map[256];    /* maps template_id -> colour description index */

    /* Curses and special states */
    bool       has_lycanthropy;      /* werewolf curse */
    bool       nessie_defeated;      /* Loch Ness encounter done */
    bool       faerie_queen_met;     /* Faerie Queen pact done */

    /* Character creation step */
    int        create_step;  /* 0=class, 1=gender, 2=name, 3=appearance, 4=stats, 5=story */

    /* Time */
    int        turn;
    int        day;
    int        hour;
    int        minute;

    /* Weather */
    WeatherType weather;
    int         weather_turns_left;  /* turns until next weather change */

    /* Rainbow event */
    bool       rainbow_active;
    Vec2       rainbow_end;       /* tile where the pot of gold is */
    int        rainbow_turns;     /* turns remaining before rainbow fades */

    /* State */
    GameMode   mode;
    bool       running;

    /* Message log */
    MessageLog log;
} GameState;

/* Get current time of day. */
TimeOfDay game_get_tod(const GameState *gs);

/* Get moon phase day (1-30). */
int game_get_moon_day(const GameState *gs);

/* Get weather name string. */
const char *weather_name(WeatherType w);

/* Initialize a new game state. */
void game_init(GameState *gs);

/* Run the main game loop. */
void game_run(GameState *gs);

/* Process a single input key. */
void game_handle_input(GameState *gs, int key);

/* Update game state after input. */
void game_update(GameState *gs);

/* Initialize the dungeon map with a simple test layout. */
void game_init_test_map(GameState *gs);

#endif /* GAME_H */
