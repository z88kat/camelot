#include "town.h"
#include <string.h>
#include <stdio.h>

static TownDef towns[MAX_TOWNS];
static int num_towns = 0;

static void add_town(const char *name, int services, int inn_cost,
                     int beer_cost, const char *beer_name) {
    if (num_towns >= MAX_TOWNS) return;
    TownDef *t = &towns[num_towns++];
    snprintf(t->name, MAX_NAME, "%s", name);
    t->services = services;
    t->inn_cost = inn_cost;
    t->beer_cost = beer_cost;
    snprintf(t->beer_name, MAX_NAME, "%s", beer_name);
}

void town_init(void) {
    num_towns = 0;

    /* Major towns with full services */
    add_town("Camelot",
        SVC_INN | SVC_CHURCH | SVC_EQUIP_SHOP | SVC_POTION_SHOP | SVC_PAWN_SHOP |
        SVC_MYSTIC | SVC_BANK | SVC_WELL | SVC_STABLE,
        8, 2, "Camelot Ale");

    add_town("London",
        SVC_INN | SVC_CHURCH | SVC_EQUIP_SHOP | SVC_POTION_SHOP | SVC_PAWN_SHOP |
        SVC_MYSTIC | SVC_BANK | SVC_STABLE,
        10, 3, "London Porter");

    add_town("York",
        SVC_INN | SVC_CHURCH | SVC_EQUIP_SHOP | SVC_POTION_SHOP | SVC_PAWN_SHOP |
        SVC_BANK | SVC_STABLE,
        8, 2, "York Bitter");

    add_town("Winchester",
        SVC_INN | SVC_CHURCH | SVC_EQUIP_SHOP | SVC_PAWN_SHOP | SVC_BANK | SVC_STABLE,
        8, 2, "Winchester Mead");

    /* Medium towns */
    add_town("Canterbury",
        SVC_INN | SVC_CHURCH | SVC_POTION_SHOP | SVC_PAWN_SHOP | SVC_MYSTIC,
        7, 2, "Holy Water Ale");

    add_town("Glastonbury",
        SVC_INN | SVC_CHURCH | SVC_MYSTIC | SVC_POTION_SHOP | SVC_WELL,
        6, 2, "Glastonbury Cider");

    add_town("Bath",
        SVC_INN | SVC_CHURCH | SVC_POTION_SHOP | SVC_PAWN_SHOP,
        7, 2, "Roman Spring Ale");

    add_town("Sherwood",
        SVC_INN | SVC_EQUIP_SHOP | SVC_PAWN_SHOP | SVC_WELL,
        5, 2, "Sherwood Stout");

    add_town("Tintagel",
        SVC_INN | SVC_CHURCH | SVC_EQUIP_SHOP | SVC_WELL,
        6, 2, "Tintagel Dark");

    /* Smaller towns */
    add_town("Cornwall",
        SVC_INN | SVC_PAWN_SHOP | SVC_MYSTIC,
        5, 2, "Cornish Scrumpy");

    add_town("Wales",
        SVC_INN | SVC_CHURCH | SVC_EQUIP_SHOP | SVC_WELL,
        5, 2, "Welsh Mead");

    add_town("Whitby",
        SVC_INN | SVC_CHURCH | SVC_PAWN_SHOP,
        12, 3, "Whitby Stout");  /* double price per plan */

    add_town("Llanthony",
        SVC_INN | SVC_CHURCH | SVC_POTION_SHOP,
        4, 1, "Monk's Brew");

    add_town("Carbonek",
        SVC_INN | SVC_CHURCH,
        6, 2, "Grail Ale");

    /* Island towns */
    add_town("Isle of Wight",
        SVC_INN | SVC_PAWN_SHOP,
        6, 2, "Island Bitter");

    add_town("Isle of Man",
        SVC_INN | SVC_EQUIP_SHOP | SVC_PAWN_SHOP,
        7, 2, "Manx Ale");

    add_town("Anglesey",
        SVC_INN | SVC_MYSTIC | SVC_WELL,
        5, 2, "Anglesey Mead");

    add_town("Orkney",
        SVC_INN | SVC_PAWN_SHOP | SVC_MYSTIC,
        8, 3, "Orkney Whisky");

    /* Active castles -- each has an inn and various services */
    add_town("Camelot Castle",
        SVC_INN | SVC_EQUIP_SHOP | SVC_CHURCH,
        6, 2, "Royal Ale");

    add_town("Castle Surluise",
        SVC_INN | SVC_EQUIP_SHOP,
        6, 2, "Surluise Mead");

    add_town("Castle Lothian",
        SVC_INN | SVC_EQUIP_SHOP | SVC_PAWN_SHOP,
        7, 2, "Highland Ale");

    add_town("Castle Northumberland",
        SVC_INN | SVC_EQUIP_SHOP,
        7, 2, "Northern Bitter");

    add_town("Castle Gore",
        SVC_INN | SVC_EQUIP_SHOP | SVC_CHURCH,
        6, 2, "Gore Stout");

    add_town("Castle Strangore",
        SVC_INN | SVC_EQUIP_SHOP | SVC_PAWN_SHOP | SVC_POTION_SHOP,
        6, 2, "Strangore Draught");

    add_town("Castle Cradelment",
        SVC_INN | SVC_EQUIP_SHOP,
        6, 2, "Welsh Dragon Ale");

    add_town("Castle Cornwall",
        SVC_INN | SVC_PAWN_SHOP,
        7, 2, "Cornish Dark");

    add_town("Castle Listenoise",
        SVC_INN | SVC_CHURCH,
        5, 2, "Pilgrim's Rest");

    add_town("Castle Benwick",
        SVC_INN | SVC_EQUIP_SHOP | SVC_POTION_SHOP,
        8, 2, "French Wine");

    add_town("Castle Carados",
        SVC_INN | SVC_EQUIP_SHOP | SVC_PAWN_SHOP,
        7, 3, "Highland Whisky");

    add_town("Castle Ireland",
        SVC_INN | SVC_EQUIP_SHOP | SVC_MYSTIC,
        6, 2, "Irish Stout");

    add_town("Castle Gaul",
        SVC_INN | SVC_EQUIP_SHOP | SVC_POTION_SHOP | SVC_PAWN_SHOP,
        8, 3, "Gallic Wine");

    add_town("Castle Brittany",
        SVC_INN | SVC_EQUIP_SHOP | SVC_PAWN_SHOP,
        7, 2, "Breton Cider");
}

const TownDef *town_get_def(const char *name) {
    for (int i = 0; i < num_towns; i++) {
        if (strcmp(towns[i].name, name) == 0)
            return &towns[i];
    }
    return NULL;
}

int town_count(void) { return num_towns; }

const TownDef *town_get_by_index(int idx) {
    if (idx < 0 || idx >= num_towns) return NULL;
    return &towns[idx];
}
