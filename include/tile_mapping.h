#ifndef TILE_MAPPING_H
#define TILE_MAPPING_H

#include "common.h"
#include "item.h"

/* Map a TileType to a tileset CSV id string. Returns NULL if unmapped.
 *
 * NOTE: DawnLike terrain sheets (Floor.png, Wall.png, Tree0.png, Hill0.png)
 * use RPG Maker-style autotile format -- individual tiles are quarter-pieces
 * designed to be assembled based on neighbors. Until proper autotile rendering
 * is implemented, terrain falls back to colored ASCII glyphs.
 * Only standalone tile objects (doors, stairs, portals) are mapped here. */
static inline const char *tiletype_to_tile_id(TileType type) {
    switch (type) {
    case TILE_STAIRS_DOWN:    return "tile_stairs_down";
    case TILE_STAIRS_UP:      return "tile_stairs_up";
    case TILE_PORTAL:         return "tile_portal";
    default:                  return NULL;
    }
}

/* Map an ItemType to a tileset CSV id string. */
static inline const char *itemtype_to_tile_id(ItemType type) {
    switch (type) {
    case ITYPE_WEAPON:       return "item_weapon_sword";
    case ITYPE_ARMOR:        return "item_armor";
    case ITYPE_SHIELD:       return "item_shield";
    case ITYPE_HELMET:       return "item_helmet";
    case ITYPE_BOOTS:        return "item_boots";
    case ITYPE_GLOVES:       return "item_gloves";
    case ITYPE_RING:         return "item_ring";
    case ITYPE_POTION:       return "item_potion";
    case ITYPE_SCROLL:       return "item_scroll";
    case ITYPE_SPELL_SCROLL: return "item_scroll";
    case ITYPE_FOOD:         return "item_food";
    case ITYPE_AMULET:       return "item_amulet";
    case ITYPE_GEM:          return "item_gem";
    case ITYPE_TREASURE:     return "item_treasure";
    case ITYPE_QUEST:        return "item_key";
    case ITYPE_TOOL:         return "item_tool";
    case ITYPE_GOLD:         return "item_gold";
    default:                 return NULL;
    }
}

/* Build a monster tile_id from the monster name.
 * Converts name to lowercase, replaces spaces with _, prepends "mon_".
 * Caller provides buffer of at least 64 bytes. */
static inline void monster_name_to_tile_id(const char *name, char *buf, int bufsize) {
    int pos = 0;
    const char *prefix = "mon_";
    while (*prefix && pos < bufsize - 1) buf[pos++] = *prefix++;
    for (int i = 0; name[i] && pos < bufsize - 1; i++) {
        char c = name[i];
        if (c == ' ' || c == '-') buf[pos++] = '_';
        else if (c >= 'A' && c <= 'Z') buf[pos++] = c + 32;
        else buf[pos++] = c;
    }
    buf[pos] = '\0';
}

/* Build an overworld creature tile_id from its name.
 * Converts to "npc_<lowercase name>". */
static inline void creature_name_to_tile_id(const char *name, char *buf, int bufsize) {
    int pos = 0;
    const char *prefix = "npc_";
    while (*prefix && pos < bufsize - 1) buf[pos++] = *prefix++;
    for (int i = 0; name[i] && pos < bufsize - 1; i++) {
        char c = name[i];
        if (c == ' ' || c == '-') buf[pos++] = '_';
        else if (c >= 'A' && c <= 'Z') buf[pos++] = c + 32;
        else buf[pos++] = c;
    }
    buf[pos] = '\0';
}

#endif /* TILE_MAPPING_H */
