#include "item.h"
#include "rng.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Built-in item templates (fallback if CSV not found)                  */
/* ------------------------------------------------------------------ */

/*                        name                glyph color       type         pow  wt  val min max rar */
static const ItemTemplate builtin_templates[] = {
    /* Weapons -- Swords */
    { "Rusty Sword",       '|', CP_GRAY,     ITYPE_WEAPON,     3,  50,  10, 0, 3, 5 },
    { "Dagger",            '|', CP_WHITE,    ITYPE_WEAPON,     2,  20,  15, 0, 4, 5 },
    { "Short Sword",       '|', CP_WHITE,    ITYPE_WEAPON,     4,  40,  30, 1, 5, 4 },
    { "Longsword",         '|', CP_CYAN,     ITYPE_WEAPON,     6,  50,  60, 2, 8, 3 },
    { "Broadsword",        '|', CP_CYAN,     ITYPE_WEAPON,     7,  60,  80, 3, 9, 3 },
    { "Claymore",          '|', CP_WHITE,    ITYPE_WEAPON,     9,  80, 120, 5, 12, 2 },
    { "Bastard Sword",     '|', CP_YELLOW,   ITYPE_WEAPON,     8,  70, 100, 4, 10, 2 },
    { "Silver Dagger",     '|', CP_WHITE,    ITYPE_WEAPON,     3,  20,  50, 2, 8, 2 },

    /* Weapons -- Axes & Maces */
    { "Hand Axe",          '|', CP_BROWN,    ITYPE_WEAPON,     4,  40,  25, 1, 5, 4 },
    { "Battle Axe",        '|', CP_BROWN,    ITYPE_WEAPON,     7,  70,  70, 3, 8, 3 },
    { "Mace",              '|', CP_GRAY,     ITYPE_WEAPON,     5,  60,  40, 2, 7, 3 },
    { "War Hammer",        '|', CP_GRAY,     ITYPE_WEAPON,     8,  80,  90, 4, 10, 2 },

    /* Weapons -- Staves */
    { "Wooden Staff",      '|', CP_BROWN,    ITYPE_WEAPON,     3,  30,  15, 0, 4, 4 },

    /* Armor -- Body */
    { "Tattered Robes",    '[', CP_BROWN,    ITYPE_ARMOR,      1,  20,   5, 0, 2, 5 },
    { "Leather Armor",     '[', CP_BROWN,    ITYPE_ARMOR,      3,  50,  30, 0, 5, 4 },
    { "Studded Leather",   '[', CP_BROWN,    ITYPE_ARMOR,      4,  60,  50, 2, 6, 3 },
    { "Chainmail",         '[', CP_GRAY,     ITYPE_ARMOR,      5, 100,  80, 3, 8, 3 },
    { "Scale Mail",        '[', CP_GRAY,     ITYPE_ARMOR,      6, 120, 100, 4, 9, 2 },
    { "Plate Armor",       '[', CP_WHITE,    ITYPE_ARMOR,      8, 150, 200, 6, 12, 1 },

    /* Armor -- Shields */
    { "Buckler",           ')', CP_BROWN,    ITYPE_SHIELD,     1,  30,  15, 0, 4, 4 },
    { "Round Shield",      ')', CP_BROWN,    ITYPE_SHIELD,     2,  50,  35, 1, 6, 3 },
    { "Kite Shield",       ')', CP_GRAY,     ITYPE_SHIELD,     3,  60,  60, 3, 8, 2 },
    { "Tower Shield",      ')', CP_WHITE,    ITYPE_SHIELD,     4,  80, 100, 5, 10, 1 },

    /* Armor -- Helmets */
    { "Leather Cap",       '^', CP_BROWN,    ITYPE_HELMET,     1,  15,  10, 0, 4, 4 },
    { "Iron Helm",         '^', CP_GRAY,     ITYPE_HELMET,     2,  30,  30, 2, 7, 3 },
    { "Great Helm",        '^', CP_WHITE,    ITYPE_HELMET,     3,  40,  60, 4, 10, 2 },

    /* Armor -- Boots */
    { "Leather Boots",     ']', CP_BROWN,    ITYPE_BOOTS,      1,  15,  10, 0, 4, 4 },
    { "Iron Boots",        ']', CP_GRAY,     ITYPE_BOOTS,      2,  40,  30, 2, 7, 3 },

    /* Armor -- Gloves */
    { "Leather Gloves",    ']', CP_BROWN,    ITYPE_GLOVES,     1,  10,   8, 0, 4, 4 },
    { "Gauntlets",         ']', CP_GRAY,     ITYPE_GLOVES,     2,  25,  25, 2, 7, 3 },

    /* Potions */
    { "Healing Potion",    '!', CP_RED,      ITYPE_POTION,    15,  10,  20, 0, 0, 4 },
    { "Greater Healing",   '!', CP_RED,      ITYPE_POTION,    40,  10,  50, 3, 0, 2 },
    { "Mana Potion",       '!', CP_BLUE,     ITYPE_POTION,    10,  10,  20, 0, 0, 4 },
    { "Greater Mana",      '!', CP_BLUE,     ITYPE_POTION,    30,  10,  50, 3, 0, 2 },
    { "Strength Potion",   '!', CP_YELLOW,   ITYPE_POTION,     3,  10,  40, 2, 0, 2 },
    { "Speed Potion",      '!', CP_CYAN,     ITYPE_POTION,     3,  10,  40, 2, 0, 2 },

    /* Food */
    { "Bread",             '%', CP_BROWN,    ITYPE_FOOD,       15, 10,   3, 0, 0, 5 },
    { "Cheese",            '%', CP_YELLOW,   ITYPE_FOOD,       12, 10,   5, 0, 0, 4 },
    { "Dried Meat",        '%', CP_BROWN,    ITYPE_FOOD,       18, 15,   8, 0, 0, 4 },
    { "Roast Chicken",     '%', CP_YELLOW,   ITYPE_FOOD,       25, 15,  12, 0, 0, 3 },
    { "Apple",             '%', CP_GREEN,    ITYPE_FOOD,        8,  5,   2, 0, 0, 5 },

    /* Scrolls */
    { "Scroll of Mapping", '?', CP_YELLOW,   ITYPE_SCROLL,     1,  5,  30, 2, 0, 2 },
    { "Scroll of Teleport",'?', CP_CYAN,     ITYPE_SCROLL,     1,  5,  40, 3, 0, 2 },

    /* Tools */
    { "Torch",             '(', CP_YELLOW,   ITYPE_TOOL,       1, 15,   5, 0, 0, 5 },
    { "Lockpick",          '(', CP_GRAY,     ITYPE_TOOL,       1,  5,  20, 1, 0, 3 },
};
static const int num_builtin = sizeof(builtin_templates) / sizeof(builtin_templates[0]);

/* Dynamic templates from CSV */
static ItemTemplate loaded_templates[MAX_ITEM_TEMPLATES];
static int num_loaded = 0;
static bool csv_loaded = false;

static short parse_item_color(const char *s) {
    if (strcmp(s, "brown") == 0) return CP_BROWN;
    if (strcmp(s, "gray") == 0) return CP_GRAY;
    if (strcmp(s, "green") == 0) return CP_GREEN;
    if (strcmp(s, "yellow") == 0) return CP_YELLOW;
    if (strcmp(s, "red") == 0) return CP_RED;
    if (strcmp(s, "white") == 0) return CP_WHITE;
    if (strcmp(s, "cyan") == 0) return CP_CYAN;
    if (strcmp(s, "magenta") == 0) return CP_MAGENTA;
    if (strcmp(s, "blue") == 0) return CP_BLUE;
    return CP_WHITE;
}

static ItemType parse_item_type(const char *s) {
    if (strcmp(s, "weapon") == 0) return ITYPE_WEAPON;
    if (strcmp(s, "armor") == 0) return ITYPE_ARMOR;
    if (strcmp(s, "shield") == 0) return ITYPE_SHIELD;
    if (strcmp(s, "helmet") == 0) return ITYPE_HELMET;
    if (strcmp(s, "boots") == 0) return ITYPE_BOOTS;
    if (strcmp(s, "gloves") == 0) return ITYPE_GLOVES;
    if (strcmp(s, "ring") == 0) return ITYPE_RING;
    if (strcmp(s, "amulet") == 0) return ITYPE_AMULET;
    if (strcmp(s, "gem") == 0) return ITYPE_GEM;
    if (strcmp(s, "treasure") == 0) return ITYPE_TREASURE;
    if (strcmp(s, "potion") == 0) return ITYPE_POTION;
    if (strcmp(s, "scroll") == 0) return ITYPE_SCROLL;
    if (strcmp(s, "food") == 0) return ITYPE_FOOD;
    if (strcmp(s, "quest") == 0) return ITYPE_QUEST;
    if (strcmp(s, "tool") == 0) return ITYPE_TOOL;
    return ITYPE_WEAPON;
}

void item_init(void) {
    FILE *f = fopen("data/items.csv", "r");
    if (!f) { csv_loaded = false; return; }

    num_loaded = 0;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        line[strcspn(line, "\n\r")] = 0;

        /* name,glyph,color,type,power,weight,value,min_depth,max_depth,rarity */
        char name[48], color[16], type_str[16];
        char glyph;
        int power, weight, value, min_d, max_d, rarity;

        int n = sscanf(line, "%47[^,],%c,%15[^,],%15[^,],%d,%d,%d,%d,%d,%d",
                       name, &glyph, color, type_str, &power, &weight, &value,
                       &min_d, &max_d, &rarity);
        if (n < 10) continue;
        if (num_loaded >= MAX_ITEM_TEMPLATES) break;

        ItemTemplate *it = &loaded_templates[num_loaded];
        snprintf(it->name, sizeof(it->name), "%s", name);
        it->glyph = glyph;
        it->color_pair = parse_item_color(color);
        it->type = parse_item_type(type_str);
        it->power = power;
        it->weight = weight;
        it->value = value;
        it->min_depth = min_d;
        it->max_depth = max_d;
        it->rarity = rarity;
        num_loaded++;
    }
    fclose(f);
    csv_loaded = (num_loaded > 0);
}

const ItemTemplate *item_get_templates(int *count) {
    if (csv_loaded) { *count = num_loaded; return loaded_templates; }
    *count = num_builtin; return builtin_templates;
}

Item item_create(int template_id, int x, int y) {
    Item it;
    memset(&it, 0, sizeof(it));
    int count;
    const ItemTemplate *tmps = item_get_templates(&count);
    if (template_id < 0 || template_id >= count) {
        it.template_id = -1;
        return it;
    }
    const ItemTemplate *t = &tmps[template_id];
    it.template_id = template_id;
    snprintf(it.name, sizeof(it.name), "%s", t->name);
    it.glyph = t->glyph;
    it.color_pair = t->color_pair;
    it.type = t->type;
    it.power = t->power;
    it.weight = t->weight;
    it.value = t->value;
    it.pos = (Vec2){ x, y };
    it.on_ground = true;
    return it;
}

EquipSlot item_get_slot(ItemType type) {
    switch (type) {
    case ITYPE_WEAPON:  return SLOT_WEAPON;
    case ITYPE_ARMOR:   return SLOT_ARMOR;
    case ITYPE_SHIELD:  return SLOT_SHIELD;
    case ITYPE_HELMET:  return SLOT_HELMET;
    case ITYPE_BOOTS:   return SLOT_BOOTS;
    case ITYPE_GLOVES:  return SLOT_GLOVES;
    case ITYPE_RING:    return SLOT_RING1;
    case ITYPE_AMULET:  return SLOT_AMULET;
    default:            return SLOT_NONE;
    }
}

const char *item_type_name(ItemType type) {
    switch (type) {
    case ITYPE_WEAPON:  return "Weapon";
    case ITYPE_ARMOR:   return "Armor";
    case ITYPE_SHIELD:  return "Shield";
    case ITYPE_HELMET:  return "Helmet";
    case ITYPE_BOOTS:   return "Boots";
    case ITYPE_GLOVES:  return "Gloves";
    case ITYPE_RING:    return "Ring";
    case ITYPE_POTION:  return "Potion";
    case ITYPE_SCROLL:  return "Scroll";
    case ITYPE_FOOD:    return "Food";
    case ITYPE_AMULET:  return "Amulet";
    case ITYPE_GEM:     return "Gem";
    case ITYPE_TREASURE:return "Treasure";
    case ITYPE_QUEST:   return "Quest";
    case ITYPE_TOOL:    return "Tool";
    case ITYPE_GOLD:    return "Gold";
    default:            return "Item";
    }
}
