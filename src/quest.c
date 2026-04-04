#include "quest.h"
#include <string.h>
#include <stdio.h>

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

void quest_init(QuestLog *ql) {
    memset(ql, 0, sizeof(*ql));

    /* Delivery quests -- can be completed now (just visit target town) */
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
        40, 2, 25, 0);  /* +1 INT */

    add_quest(ql, "King's Message",
        "Deliver a royal decree from Camelot to Castle Lothian.",
        QTYPE_DELIVERY, "Camelot", "Castle Lothian", "",
        80, 0, 40, 10);  /* +1 STR, needs chiv 10 */

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
        QTYPE_DELIVERY, "Wales", "London", "",
        70, 0, 35, 0);  /* +1 STR */

    /* Fetch quests -- need dungeons/items to complete */
    add_quest(ql, "Grandfather's Shield",
        "Find my grandfather's shield in the Camelot Catacombs.",
        QTYPE_FETCH, "Camelot", "", "Camelot Catacombs",
        80, 1, 50, 0);  /* +1 DEF */

    add_quest(ql, "Ancient Tome",
        "Retrieve the ancient tome from Tintagel Caves.",
        QTYPE_FETCH, "Tintagel", "", "Tintagel Caves",
        90, 2, 60, 0);  /* +1 INT */

    add_quest(ql, "Dragon Scale",
        "Bring me a Dragon scale from Mount Draig.",
        QTYPE_FETCH, "Winchester", "", "Mount Draig",
        150, 0, 100, 20);  /* +1 STR */

    /* Kill quests -- need combat to complete */
    add_quest(ql, "Ghost of Sherwood",
        "A ghost haunts Sherwood Depths. Put it to rest.",
        QTYPE_KILL, "Sherwood", "", "Sherwood Depths",
        100, 3, 80, 0);  /* +1 SPD */

    add_quest(ql, "Bandit Hideout",
        "Bandits have a hideout in the Catacombs. Clear them out.",
        QTYPE_KILL, "Camelot", "", "Camelot Catacombs",
        70, -1, 50, 0);

    add_quest(ql, "The Black Knight",
        "The Black Knight blocks the road to York. Defeat him.",
        QTYPE_KILL, "York", "", "",
        120, 0, 100, 15);  /* +1 STR */

    add_quest(ql, "Whitby Vampire",
        "A vampire terrorizes Whitby at night. Destroy it.",
        QTYPE_KILL, "Whitby", "", "Whitby Abbey",
        150, 1, 120, 25);  /* +1 DEF */

    add_quest(ql, "Troll Bridge",
        "A troll lurks under a bridge near Wales. Slay it.",
        QTYPE_KILL, "Wales", "", "",
        80, 0, 60, 0);  /* +1 STR */
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
