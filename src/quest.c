#include "quest.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static void add_quest(QuestLog *ql, const char *name, const char *desc,
                      QuestType type, const char *giver, const char *target_town,
                      const char *target_dungeon, int gold, int stat, int xp,
                      int chiv_min) {
    if (ql->num_quests >= MAX_QUESTS) return;
    Quest *q = &ql->quests[ql->num_quests];
    q->id = ql->num_quests;
    snprintf(q->name, sizeof(q->name), "%s", name);
    snprintf(q->description, sizeof(q->description), "%s", desc);
    q->type = type;
    q->state = QUEST_NOT_STARTED;
    snprintf(q->giver_town, MAX_NAME, "%s", giver);
    snprintf(q->target_town, MAX_NAME, "%s", target_town);
    snprintf(q->target_dungeon, MAX_NAME, "%s", target_dungeon);
    q->gold_reward = gold;
    q->stat_reward = stat;
    q->xp_reward = xp;
    q->chivalry_min = chiv_min;
    ql->num_quests++;
}

static QuestType parse_quest_type(const char *s) {
    if (strcmp(s, "delivery") == 0) return QTYPE_DELIVERY;
    if (strcmp(s, "fetch") == 0)    return QTYPE_FETCH;
    if (strcmp(s, "kill") == 0)     return QTYPE_KILL;
    return QTYPE_FETCH;
}

/* ------------------------------------------------------------------ */
/* Built-in quests (fallback if CSV not found)                         */
/* ------------------------------------------------------------------ */
static void quest_init_builtin(QuestLog *ql) {
    add_quest(ql, "Holy Water Delivery",
        "Deliver holy water to the monk in Glastonbury.",
        QTYPE_DELIVERY, "Winchester", "Glastonbury", "",
        50, -1, 30, 0);

    add_quest(ql, "Letter to London",
        "Bring this sealed letter to the blacksmith in London.",
        QTYPE_DELIVERY, "York", "London", "",
        60, -1, 30, 0);

    add_quest(ql, "Healing Herbs",
        "Carry these healing herbs to the mystic in Cornwall.",
        QTYPE_DELIVERY, "Sherwood", "Cornwall", "",
        40, 2, 25, 0);

    add_quest(ql, "King's Message",
        "Deliver a royal decree from Camelot to Castle Lothian.",
        QTYPE_DELIVERY, "Camelot", "Castle Lothian", "",
        80, 0, 40, 10);

    add_quest(ql, "Pilgrim's Offering",
        "Carry an offering from Bath to Canterbury cathedral.",
        QTYPE_DELIVERY, "Bath", "Canterbury", "",
        45, -1, 30, 0);

    add_quest(ql, "Wine for the King",
        "Deliver fine wine from Castle Gaul to Camelot Castle.",
        QTYPE_DELIVERY, "Castle Gaul", "Camelot Castle", "",
        100, -1, 50, 20);

    add_quest(ql, "Welsh Ore",
        "Bring Welsh mountain ore to the blacksmith in London.",
        QTYPE_DELIVERY, "Caernarfon", "London", "",
        70, 0, 35, 0);

    add_quest(ql, "Grandfather's Shield",
        "Find my grandfather's shield in the Camelot Catacombs.",
        QTYPE_FETCH, "Camelot", "", "Camelot Catacombs",
        80, 1, 50, 0);

    add_quest(ql, "Ancient Tome",
        "Retrieve the ancient tome from Tintagel Caves.",
        QTYPE_FETCH, "Tintagel", "", "Tintagel Caves",
        90, 2, 60, 0);

    add_quest(ql, "Dragon Scale",
        "Bring me a Dragon scale from Mount Draig.",
        QTYPE_FETCH, "Winchester", "", "Mount Draig",
        150, 0, 100, 20);

    add_quest(ql, "Ghost of Sherwood",
        "A ghost haunts Sherwood Depths. Put it to rest.",
        QTYPE_KILL, "Sherwood", "", "Sherwood Depths",
        100, 3, 80, 0);

    add_quest(ql, "Bandit Hideout",
        "Bandits have a hideout in the Catacombs. Clear them out.",
        QTYPE_KILL, "Camelot", "", "Camelot Catacombs",
        70, -1, 50, 0);

    add_quest(ql, "The Black Knight",
        "The Black Knight blocks the road to York. Defeat him.",
        QTYPE_KILL, "York", "", "",
        120, 0, 100, 15);

    add_quest(ql, "Whitby Vampire",
        "A vampire terrorizes Whitby at night. Destroy it.",
        QTYPE_KILL, "Whitby", "", "Whitby Abbey",
        150, 1, 120, 25);

    add_quest(ql, "Troll Bridge",
        "A troll lurks under a bridge near Caernarfon. Slay it.",
        QTYPE_KILL, "Caernarfon", "", "",
        80, 0, 60, 0);
}

void quest_init(QuestLog *ql) {
    memset(ql, 0, sizeof(*ql));

    FILE *f = fopen("data/quests.csv", "r");
    if (!f) { quest_init_builtin(ql); return; }

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        line[strcspn(line, "\n\r")] = 0;
        if (ql->num_quests >= MAX_QUESTS) break;

        /* name,description,type,giver_town,target_town,target_dungeon,gold_reward,stat_reward,xp_reward,chivalry_min */
        char name[64], desc[256], type_str[16], giver[48], target[48], dungeon[48];
        int gold, stat, xp, chiv;

        int n = sscanf(line, "%63[^,],%255[^,],%15[^,],%47[^,],%47[^,],%47[^,],%d,%d,%d,%d",
                       name, desc, type_str, giver, target, dungeon,
                       &gold, &stat, &xp, &chiv);
        if (n < 10) continue;

        /* Trim whitespace-only fields */
        if (strcmp(target, "  ") == 0 || strcmp(target, " ") == 0) target[0] = '\0';
        if (strcmp(dungeon, "  ") == 0 || strcmp(dungeon, " ") == 0) dungeon[0] = '\0';

        add_quest(ql, name, desc, parse_quest_type(type_str),
                  giver, target, dungeon, gold, stat, xp, chiv);
    }
    fclose(f);

    if (ql->num_quests == 0) quest_init_builtin(ql);
}

int quest_count_active(const QuestLog *ql) {
    int count = 0;
    for (int i = 0; i < ql->num_quests; i++)
        if (ql->quests[i].state == QUEST_ACTIVE) count++;
    return count;
}

int quest_count_completed(const QuestLog *ql) {
    int count = 0;
    for (int i = 0; i < ql->num_quests; i++)
        if (ql->quests[i].state == QUEST_TURNED_IN) count++;
    return count;
}

Quest *quest_find_available(QuestLog *ql, const char *town_name, int chivalry) {
    for (int i = 0; i < ql->num_quests; i++) {
        Quest *q = &ql->quests[i];
        if (q->state == QUEST_NOT_STARTED &&
            strcmp(q->giver_town, town_name) == 0 &&
            chivalry >= q->chivalry_min) {
            return q;
        }
    }
    return NULL;
}

Quest *quest_check_delivery(QuestLog *ql, const char *town_name) {
    for (int i = 0; i < ql->num_quests; i++) {
        Quest *q = &ql->quests[i];
        if (q->state == QUEST_ACTIVE && q->type == QTYPE_DELIVERY &&
            strcmp(q->target_town, town_name) == 0) {
            return q;
        }
    }
    return NULL;
}
