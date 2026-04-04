#ifndef ITEM_H
#define ITEM_H

#include "common.h"

/* Item types */
typedef enum {
    ITYPE_WEAPON,
    ITYPE_ARMOR,
    ITYPE_SHIELD,
    ITYPE_HELMET,
    ITYPE_BOOTS,
    ITYPE_GLOVES,
    ITYPE_RING,
    ITYPE_POTION,
    ITYPE_SCROLL,
    ITYPE_FOOD,
    ITYPE_QUEST,
    ITYPE_TOOL,
    ITYPE_GOLD
} ItemType;

/* Equipment slots */
typedef enum {
    SLOT_NONE = -1,
    SLOT_WEAPON,
    SLOT_ARMOR,
    SLOT_SHIELD,
    SLOT_HELMET,
    SLOT_BOOTS,
    SLOT_GLOVES,
    SLOT_RING1,
    SLOT_RING2,
    NUM_SLOTS
} EquipSlot;

/* An item definition (template) */
typedef struct {
    char     name[48];
    char     glyph;
    short    color_pair;
    ItemType type;
    int      power;       /* damage for weapons, defense for armor, heal for potions */
    int      weight;      /* weight in units (x10 for decimals: 10 = 1.0) */
    int      value;       /* gold value for buying/selling */
    int      min_depth;   /* min dungeon depth to find (0 = any) */
    int      max_depth;   /* max dungeon depth (0 = any) */
    int      rarity;      /* 1=very rare, 3=normal, 5=common */
} ItemTemplate;

/* A concrete item instance (on the ground or in inventory) */
typedef struct {
    int      template_id;  /* index into template table, -1 = empty */
    char     name[48];
    char     glyph;
    short    color_pair;
    ItemType type;
    int      power;
    int      weight;
    int      value;
    Vec2     pos;          /* position on map (-1,-1 if in inventory) */
    bool     on_ground;
} Item;

#define MAX_ITEM_TEMPLATES 256
#define MAX_GROUND_ITEMS   64   /* items on a dungeon level */

/* Initialize items (load from CSV or use defaults) */
void item_init(void);

/* Get templates */
const ItemTemplate *item_get_templates(int *count);

/* Create an item instance from a template */
Item item_create(int template_id, int x, int y);

/* Get equipment slot for an item type */
EquipSlot item_get_slot(ItemType type);

/* Get item type name string */
const char *item_type_name(ItemType type);

#endif /* ITEM_H */
