#ifndef QUEST_H
#define QUEST_H

#include "common.h"

/* Quest state */
typedef enum {
    QUEST_NOT_STARTED,
    QUEST_ACTIVE,
    QUEST_COMPLETE,    /* objective done, needs turn-in */
    QUEST_TURNED_IN    /* fully done, reward collected */
} QuestState;

/* Quest type */
typedef enum {
    QTYPE_FETCH,       /* retrieve item from dungeon */
    QTYPE_KILL,        /* slay specific enemy */
    QTYPE_DELIVERY     /* carry item between towns */
} QuestType;

/* A quest definition */
typedef struct {
    int         id;
    char        name[64];
    char        description[256];
    QuestType   type;
    QuestState  state;
    char        giver_town[MAX_NAME];     /* town where quest NPC is */
    char        target_town[MAX_NAME];    /* for delivery: destination town */
    char        target_dungeon[MAX_NAME]; /* for fetch/kill: dungeon name */
    int         gold_reward;
    int         stat_reward;              /* index: 0=STR 1=DEF 2=INT 3=SPD, -1=none */
    int         xp_reward;
    int         chivalry_min;             /* minimum chivalry to be offered */
} Quest;

#define MAX_QUESTS 20

/* Quest system state */
typedef struct {
    Quest   quests[MAX_QUESTS];
    int     num_quests;
    bool    grail_quest_active;   /* has Arthur given the main quest? */
    bool    grail_quest_complete; /* has the Grail been returned? */
} QuestLog;

/* Initialize quests for a new game. */
void quest_init(QuestLog *ql);

/* Get number of active quests. */
int quest_count_active(const QuestLog *ql);

/* Get number of completed (turned in) quests. */
int quest_count_completed(const QuestLog *ql);

/* Find quest by giver town that hasn't been started yet. Returns NULL if none. */
Quest *quest_find_available(QuestLog *ql, const char *town_name, int chivalry);

/* Check if a delivery quest can be completed at this town. */
Quest *quest_check_delivery(QuestLog *ql, const char *town_name);

#endif /* QUEST_H */
