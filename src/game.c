#include "game.h"
#include "ui.h"
#include "rng.h"
#include "pathfind.h"
#include "save.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Forward declarations */
static void ai_explode_on_death(GameState *gs, Entity *e);
static void handle_character_create(GameState *gs, int key);
static void render_character_create(GameState *gs);
static void dungeon_update_fov(GameState *gs);
static const char *chivalry_title(int chiv);
static void game_render(GameState *gs);
static bool monster_at_pos(DungeonLevel *dl, int x, int y);

/* ------------------------------------------------------------------ */
/* Score calculation                                                   */
/* ------------------------------------------------------------------ */
static int calculate_score(const GameState *gs) {
    int score = gs->kills * 10 + gs->gold_earned +
                quest_count_completed(&gs->quests) * 150;
    /* Chivalry multiplier: 0.5x at 0, 1.0x at 50, 2.0x at 100 */
    int chiv_pct = 50 + gs->chivalry;
    score = score * chiv_pct / 100;
    if (gs->quests.grail_quest_complete) score += 5000;
    if (gs->cheat_mode) score = 0;
    return score;
}

/* ------------------------------------------------------------------ */
/* Death screen                                                        */
/* ------------------------------------------------------------------ */
static void show_death_screen(GameState *gs) {
    const char *class_names[] = { "Knight", "Wizard", "Ranger" };
    int score = calculate_score(gs);

    ui_clear();
    int row = 1;

    attron(COLOR_PAIR(CP_RED_BOLD) | A_BOLD);
    mvprintw(row++, 2, "=== YOU HAVE DIED ===");
    attroff(COLOR_PAIR(CP_RED_BOLD) | A_BOLD);
    row++;

    attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
    mvprintw(row++, 4, "%s, %s %s", gs->player_name,
             class_names[gs->player_class], chivalry_title(gs->chivalry));
    attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
    row++;

    attron(COLOR_PAIR(CP_RED));
    mvprintw(row++, 4, "Cause of death: %s", gs->cause_of_death);
    attroff(COLOR_PAIR(CP_RED));
    row++;

    attron(COLOR_PAIR(CP_WHITE));
    mvprintw(row++, 4, "Level: %-3d    Turns: %d    Day: %d", gs->player_level, gs->turn, gs->day);
    mvprintw(row++, 4, "STR: %-3d  DEF: %-3d  INT: %-3d  SPD: %-3d", gs->str, gs->def, gs->intel, gs->spd);
    mvprintw(row++, 4, "Kills: %-4d   Gold earned: %d", gs->kills, gs->gold_earned);
    mvprintw(row++, 4, "Quests: %d/%d completed", quest_count_completed(&gs->quests), gs->quests.num_quests);
    mvprintw(row++, 4, "Spells: %d learned    Chivalry: %d", gs->num_spells, gs->chivalry);
    if (gs->quests.grail_quest_complete)
        mvprintw(row++, 4, "THE HOLY GRAIL WAS FOUND (+5000 score)");
    attroff(COLOR_PAIR(CP_WHITE));
    row++;

    attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
    mvprintw(row++, 4, "FINAL SCORE: %d%s", score, gs->cheat_mode ? " (CHEAT)" : "");
    attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
    row += 2;

    attron(COLOR_PAIR(CP_GRAY));
    mvprintw(row, 4, "Press any key to continue...");
    attroff(COLOR_PAIR(CP_GRAY));
    ui_refresh();
    ui_getkey();

    /* Record high score */
    if (!gs->cheat_mode) {
        HighScore scores[MAX_HIGH_SCORES];
        int count = scores_load(scores);
        HighScore entry;
        snprintf(entry.name, MAX_NAME, "%s", gs->player_name);
        snprintf(entry.class_name, sizeof(entry.class_name), "%s", class_names[gs->player_class]);
        entry.score = score;
        snprintf(entry.cause, sizeof(entry.cause), "%s", gs->cause_of_death);
        entry.found_grail = gs->quests.grail_quest_complete;
        scores_add(scores, &count, &entry);
        scores_save(scores, count);
    }

    /* Record fallen hero */
    FallenHero hero;
    snprintf(hero.name, MAX_NAME, "%s", gs->player_name);
    snprintf(hero.class_name, sizeof(hero.class_name), "%s", class_names[gs->player_class]);
    hero.level = gs->player_level;
    hero.score = score;
    hero.turns = gs->turn;
    hero.kills = gs->kills;
    snprintf(hero.cause, sizeof(hero.cause), "%s", gs->cause_of_death);
    snprintf(hero.title, sizeof(hero.title), "%s", chivalry_title(gs->chivalry));
    fallen_add(&hero);

    /* Delete save */
    delete_save();
}

/* ------------------------------------------------------------------ */
/* Title screen                                                        */
/* ------------------------------------------------------------------ */
int show_title_screen(void) {
    /* Returns: 1=new game, 2=continue, 3=high scores, 4=fallen heroes, 0=quit */
    bool has_save = save_exists();

    ui_clear();
    int row = 3;

    attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
    mvprintw(row++, 10, "  _  __      _       _     _        ");
    mvprintw(row++, 10, " | |/ /     (_)     | |   | |       ");
    mvprintw(row++, 10, " | ' / _ __  _  __ _| |__ | |_ ___  ");
    mvprintw(row++, 10, " |  < | '_ \\| |/ _` | '_ \\| __/ __|");
    mvprintw(row++, 10, " | . \\| | | | | (_| | | | | |_\\__ \\");
    mvprintw(row++, 10, " |_|\\_\\_| |_|_|\\__, |_| |_|\\__|___/");
    mvprintw(row++, 10, "                __/ |               ");
    mvprintw(row++, 10, "   of          |___/                ");
    mvprintw(row++, 10, "       C A M E L O T                ");
    attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
    row += 2;

    attron(COLOR_PAIR(CP_WHITE));
    mvprintw(row++, 10, "The Quest for the Holy Grail");
    attroff(COLOR_PAIR(CP_WHITE));
    row += 2;

    int menu_row = row;
    attron(COLOR_PAIR(CP_CYAN));
    if (has_save)
        mvprintw(row++, 14, "[c] Continue saved game");
    mvprintw(row++, 14, "[n] New Game");
    mvprintw(row++, 14, "[h] High Scores");
    mvprintw(row++, 14, "[f] Fallen Heroes");
    mvprintw(row++, 14, "[q] Quit");
    attroff(COLOR_PAIR(CP_CYAN));
    (void)menu_row;

    ui_refresh();

    while (1) {
        int key = ui_getkey();
        if ((key == 'C' || key == 'c') && has_save) return 2;
        if (key == 'N' || key == 'n') return 1;
        if (key == 'H' || key == 'h') return 3;
        if (key == 'F' || key == 'f') return 4;
        if (key == 'Q' || key == 'q') return 0;
    }
}

void show_high_scores_screen(void) {
    HighScore scores[MAX_HIGH_SCORES];
    int count = scores_load(scores);

    ui_clear();
    int row = 2;
    attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
    mvprintw(row++, 4, "=== High Scores ===");
    attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
    row++;

    if (count == 0) {
        attron(COLOR_PAIR(CP_GRAY));
        mvprintw(row++, 6, "No scores yet. Go forth and quest!");
        attroff(COLOR_PAIR(CP_GRAY));
    } else {
        attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        mvprintw(row++, 4, " #  %-16s %-10s %6s  %s", "Name", "Class", "Score", "Cause of Death");
        attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        for (int i = 0; i < count; i++) {
            short cp = (i == 0) ? CP_YELLOW_BOLD : (i < 3) ? CP_YELLOW : CP_WHITE;
            attron(COLOR_PAIR(cp));
            mvprintw(row++, 4, "%2d. %-16s %-10s %6d  %s%s",
                     i + 1, scores[i].name, scores[i].class_name,
                     scores[i].score, scores[i].cause,
                     scores[i].found_grail ? " [GRAIL]" : "");
            attroff(COLOR_PAIR(cp));
        }
    }
    row++;
    attron(COLOR_PAIR(CP_GRAY));
    mvprintw(row, 4, "Press any key to return.");
    attroff(COLOR_PAIR(CP_GRAY));
    ui_refresh();
    ui_getkey();
}

void show_fallen_heroes_screen(void) {
    FallenHero heroes[MAX_FALLEN];
    int count = fallen_load(heroes);

    ui_clear();
    int row = 2;
    attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
    mvprintw(row++, 4, "=== Fallen Heroes ===");
    attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
    row++;

    if (count == 0) {
        attron(COLOR_PAIR(CP_GRAY));
        mvprintw(row++, 6, "No heroes have fallen yet.");
        attroff(COLOR_PAIR(CP_GRAY));
    } else {
        attron(COLOR_PAIR(CP_WHITE));
        mvprintw(row++, 4, "%d brave soul%s ha%s perished in the quest for the Grail.",
                 count, count == 1 ? "" : "s", count == 1 ? "s" : "ve");
        attroff(COLOR_PAIR(CP_WHITE));
        row++;
        int term_rows, term_cols;
        ui_get_size(&term_rows, &term_cols);
        for (int i = 0; i < count && row < term_rows - 3; i++) {
            attron(COLOR_PAIR(CP_GRAY));
            mvprintw(row++, 4, "%-16s Lvl %-2d  %-10s  %4d pts  \"%s\"",
                     heroes[i].name, heroes[i].level, heroes[i].class_name,
                     heroes[i].score, heroes[i].cause);
            attroff(COLOR_PAIR(CP_GRAY));
        }
    }
    row++;
    attron(COLOR_PAIR(CP_GRAY));
    mvprintw(row, 4, "Press any key to return.");
    attroff(COLOR_PAIR(CP_GRAY));
    ui_refresh();
    ui_getkey();
}

/* Unidentified potion colour descriptions */
static const char *potion_colours[] = {
    "Bubbling Red", "Murky Green", "Glowing Blue", "Swirling Purple",
    "Fizzing Yellow", "Dark Brown", "Shimmering Gold", "Pale White",
    "Inky Black", "Cloudy Grey", "Sparkling Cyan", "Thick Orange",
    "Smoky Violet", "Oily Crimson", "Luminous Pink", "Frothy Amber",
    "Crystalline Clear", "Milky Pearl", "Sickly Olive", "Bright Scarlet",
    "Deep Indigo", "Rusty Bronze"
};
static const int num_potion_colours = 22;

static const char *potion_display_name(GameState *gs, Item *it) {
    static char buf[64];
    if (it->type != ITYPE_POTION || it->identified ||
        it->template_id < 0 || it->template_id >= 256 ||
        gs->potions_identified[it->template_id]) {
        return it->name;
    }
    int ci = gs->potion_colour_map[it->template_id] % num_potion_colours;
    snprintf(buf, sizeof(buf), "%s Potion", potion_colours[ci]);
    return buf;
}

/* ------------------------------------------------------------------ */
/* Dungeon boss spawning                                               */
/* ------------------------------------------------------------------ */
static void spawn_dungeon_boss(GameState *gs, DungeonLevel *dl, const char *dungeon_name, int depth, int max_depth) {
    /* Only spawn boss on deepest level */
    if (depth < max_depth - 1) return;
    if (dl->num_monsters >= MAX_MONSTERS_PER_LEVEL - 1) return;

    /* Determine boss based on dungeon */
    const char *boss_name = NULL;
    char boss_glyph = 'B';
    short boss_color = CP_RED_BOLD;
    int bhp = 40, bstr = 12, bdef = 8, bspd = 3, bxp = 100;
    uint32_t bflags = AI_ALWAYS_CHASE | AI_OPENS_DOORS;

    if (strcmp(dungeon_name, "Camelot Catacombs") == 0) {
        boss_name = "Dark Monk"; boss_glyph = 'm'; boss_color = CP_MAGENTA_BOLD;
        bhp = 35; bstr = 10; bdef = 7; bxp = 60; bflags |= AI_HEAL_ALLIES;
    } else if (strcmp(dungeon_name, "Tintagel Caves") == 0) {
        boss_name = "Black Knight"; boss_glyph = 'K'; boss_color = CP_RED_BOLD;
        bhp = 50; bstr = 14; bdef = 10; bxp = 120;
    } else if (strcmp(dungeon_name, "Sherwood Depths") == 0) {
        boss_name = "Evil Sorcerer"; boss_glyph = 'S'; boss_color = CP_MAGENTA_BOLD;
        bhp = 40; bstr = 12; bdef = 6; bxp = 100; bflags |= AI_RANGED_ATTACK | AI_SUMMON;
    } else if (strcmp(dungeon_name, "Mount Draig") == 0) {
        boss_name = "Red Dragon"; boss_glyph = 'D'; boss_color = CP_RED_BOLD;
        bhp = 70; bstr = 18; bdef = 12; bspd = 3; bxp = 200; bflags |= AI_BREATHE_FIRE;
    } else if (strcmp(dungeon_name, "Whitby Abbey") == 0) {
        boss_name = "Vampire Lord"; boss_glyph = 'V'; boss_color = CP_RED_BOLD;
        bhp = 50; bstr = 14; bdef = 8; bspd = 5; bxp = 150; bflags |= AI_DEBUFF;
    } else if (strcmp(dungeon_name, "White Cliffs Cave") == 0) {
        boss_name = "Sea Serpent King"; boss_glyph = 'S'; boss_color = CP_CYAN_BOLD;
        bhp = 55; bstr = 15; bdef = 9; bxp = 130;
    } else if (strcmp(dungeon_name, "Glastonbury Tor") == 0) {
        boss_name = "Dark Templar"; boss_glyph = 'T'; boss_color = CP_MAGENTA_BOLD;
        bhp = 60; bstr = 16; bdef = 11; bxp = 180; bflags |= AI_FEAR_AURA;
    } else if (strcmp(dungeon_name, "Avalon Shrine") == 0) {
        boss_name = "Guardian Spirit"; boss_glyph = 'G'; boss_color = CP_CYAN_BOLD;
        bhp = 45; bstr = 12; bdef = 8; bxp = 100; bflags |= AI_GHOST;
    } else if (strcmp(dungeon_name, "Orkney Barrows") == 0) {
        boss_name = "Barrow King"; boss_glyph = 'W'; boss_color = CP_GRAY;
        bhp = 40; bstr = 11; bdef = 9; bxp = 80; bflags |= AI_SUMMON;
    }
    /* Abandoned castles */
    else if (strcmp(dungeon_name, "Castle Dolorous Garde") == 0) {
        boss_name = "Ghost Knight"; boss_glyph = 'G'; boss_color = CP_WHITE;
        bhp = 35; bstr = 10; bdef = 5; bxp = 70; bflags |= AI_GHOST;
    } else if (strcmp(dungeon_name, "Castle Perilous") == 0) {
        boss_name = "Dark Enchantress"; boss_glyph = 'E'; boss_color = CP_MAGENTA_BOLD;
        bhp = 40; bstr = 12; bdef = 6; bxp = 90; bflags |= AI_DEBUFF | AI_RANGED_ATTACK;
    } else if (strcmp(dungeon_name, "Bamburgh Castle") == 0) {
        boss_name = "Bandit King"; boss_glyph = 'P'; boss_color = CP_YELLOW_BOLD;
        bhp = 45; bstr = 13; bdef = 8; bxp = 80;
    } else if (strcmp(dungeon_name, "Sorcerer's Tower") == 0) {
        boss_name = "Evil Sorcerer"; boss_glyph = 'S'; boss_color = CP_MAGENTA_BOLD;
        bhp = 55; bstr = 14; bdef = 7; bspd = 4; bxp = 200;
        bflags |= AI_RANGED_ATTACK | AI_SUMMON | AI_DEBUFF;
    } else {
        return; /* unknown dungeon */
    }

    /* If this dungeon holds the Grail and quest is active, override boss with Mordred */
    bool is_grail_dungeon = (gs->quests.grail_quest_active && !gs->has_grail &&
                              gs->grail_dungeon[0] != '\0' &&
                              strcmp(dungeon_name, gs->grail_dungeon) == 0);
    if (is_grail_dungeon) {
        boss_name = "Mordred"; boss_glyph = 'M'; boss_color = CP_MAGENTA_BOLD;
        bhp = 80; bstr = 20; bdef = 14; bspd = 4; bxp = 300;
        bflags = AI_ALWAYS_CHASE | AI_OPENS_DOORS | AI_FEAR_AURA | AI_SUMMON | AI_RANGED_ATTACK;
    }

    /* Place boss near the portal/exit on the deepest level */
    Entity *boss = &dl->monsters[dl->num_monsters];
    memset(boss, 0, sizeof(*boss));
    snprintf(boss->name, MAX_NAME, "%s", boss_name);
    boss->glyph = boss_glyph;
    boss->color_pair = boss_color;
    boss->hp = bhp; boss->max_hp = bhp;
    boss->str = bstr; boss->def = bdef; boss->spd = bspd;
    boss->xp_reward = bxp;
    boss->alive = true;
    boss->ai_flags = bflags;
    boss->ai_state = AI_STATE_CHASE;
    boss->category = MCAT_BOSS;

    /* Place near stairs down or portal */
    Vec2 bpos = dl->stairs_down[0];
    for (int tries = 0; tries < 50; tries++) {
        int bx = bpos.x + rng_range(-3, 3);
        int by = bpos.y + rng_range(-3, 3);
        if (bx > 1 && bx < MAP_WIDTH - 2 && by > 1 && by < MAP_HEIGHT - 2 &&
            dl->tiles[by][bx].passable && !monster_at_pos(dl, bx, by)) {
            boss->pos = (Vec2){ bx, by };
            dl->num_monsters++;

            /* If Grail dungeon, place the Grail item near the boss */
            if (is_grail_dungeon && dl->num_ground_items < MAX_GROUND_ITEMS) {
                Item grail;
                memset(&grail, 0, sizeof(grail));
                grail.template_id = -10;
                snprintf(grail.name, sizeof(grail.name), "The Holy Grail");
                grail.glyph = '*';
                grail.color_pair = CP_YELLOW_BOLD;
                grail.type = ITYPE_QUEST;
                grail.power = 0;
                grail.weight = 5;
                grail.value = 0;
                grail.pos = (Vec2){ bx, by };
                grail.on_ground = true;
                dl->ground_items[dl->num_ground_items++] = grail;
            }
            break;
        }
    }
}

/* Helper to check if a monster is at a position */
static bool monster_at_pos(DungeonLevel *dl, int x, int y) {
    for (int i = 0; i < dl->num_monsters; i++) {
        if (dl->monsters[i].alive && dl->monsters[i].pos.x == x && dl->monsters[i].pos.y == y)
            return true;
    }
    return false;
}

/* XP thresholds for leveling (20 levels) */
static const int xp_table[MAX_LEVELS] = {
    0, 50, 120, 250, 450, 750, 1200, 1800, 2600, 3600,
    5000, 6800, 9000, 12000, 15500, 20000, 25000, 31000, 38000, 46000
};

/* ------------------------------------------------------------------ */
/* Dungeon helpers                                                     */
/* ------------------------------------------------------------------ */

/* Get the current dungeon level's tile map */
static Tile (*current_dungeon_tiles(GameState *gs))[MAP_WIDTH] {
    if (!gs->dungeon) return NULL;
    return gs->dungeon->levels[gs->dungeon->current_level].tiles;
}

/* Get the current dungeon level struct */
static DungeonLevel *current_dungeon_level(GameState *gs) {
    if (!gs->dungeon) return NULL;
    return &gs->dungeon->levels[gs->dungeon->current_level];
}

/* Get dungeon depth range for a named dungeon entrance */
static void get_dungeon_depth(const char *name, int *min_d, int *max_d) {
    if (strcmp(name, "Camelot Catacombs") == 0) { *min_d = 3; *max_d = 6; }
    else if (strcmp(name, "Tintagel Caves") == 0) { *min_d = 5; *max_d = 10; }
    else if (strcmp(name, "Sherwood Depths") == 0) { *min_d = 5; *max_d = 10; }
    else if (strcmp(name, "Mount Draig") == 0) { *min_d = 4; *max_d = 8; }
    else if (strcmp(name, "Glastonbury Tor") == 0) { *min_d = 8; *max_d = 15; }
    else if (strcmp(name, "White Cliffs Cave") == 0) { *min_d = 3; *max_d = 6; }
    else if (strcmp(name, "Whitby Abbey") == 0) { *min_d = 2; *max_d = 4; }
    else if (strcmp(name, "Avalon Shrine") == 0) { *min_d = 3; *max_d = 5; }
    else if (strcmp(name, "Orkney Barrows") == 0) { *min_d = 2; *max_d = 4; }
    /* Abandoned castles */
    else if (strcmp(name, "Castle Dolorous Garde") == 0) { *min_d = 2; *max_d = 3; }
    else if (strcmp(name, "Castle Perilous") == 0) { *min_d = 2; *max_d = 3; }
    else if (strcmp(name, "Bamburgh Castle") == 0) { *min_d = 2; *max_d = 3; }
    else if (strcmp(name, "Sorcerer's Tower") == 0) { *min_d = 3; *max_d = 5; }
    else { *min_d = 3; *max_d = 8; }  /* default */
}

/* Check if player stepped on a trap */
static void check_traps(GameState *gs) {
    DungeonLevel *dl = current_dungeon_level(gs);
    if (!dl) return;

    for (int i = 0; i < dl->num_traps; i++) {
        Trap *t = &dl->traps[i];
        if (t->triggered || t->revealed) continue;
        if (t->pos.x != gs->player_pos.x || t->pos.y != gs->player_pos.y) continue;

        t->triggered = true;
        t->revealed = true;

        /* Show trap on map */
        dl->tiles[t->pos.y][t->pos.x].glyph = '^';
        dl->tiles[t->pos.y][t->pos.x].color_pair = CP_RED;

        switch (t->type) {
        case TRAP_PIT: {
            int dmg = rng_range(5, 10);
            gs->hp -= dmg;
            log_add(&gs->log, gs->turn, CP_RED,
                     "You fall into a pit trap! -%d HP", dmg);
            break;
        }
        case TRAP_POISON_DART: {
            gs->hp -= 3;
            log_add(&gs->log, gs->turn, CP_RED,
                     "A poison dart shoots from the wall! -3 HP (poisoned!)");
            break;
        }
        case TRAP_TRIPWIRE:
            log_add(&gs->log, gs->turn, CP_RED,
                     "You trip over a wire and stumble!");
            break;
        case TRAP_ARROW: {
            gs->hp -= 8;
            log_add(&gs->log, gs->turn, CP_RED,
                     "Arrows fly from the walls! -8 HP");
            break;
        }
        case TRAP_ALARM:
            log_add(&gs->log, gs->turn, CP_RED,
                     "ALARM! A loud bell rings! All enemies alerted!");
            break;
        case TRAP_BEAR: {
            gs->hp -= 4;
            log_add(&gs->log, gs->turn, CP_RED,
                     "A bear trap snaps shut on your leg! -4 HP");
            break;
        }
        case TRAP_FIRE: {
            gs->hp -= 10;
            log_add(&gs->log, gs->turn, CP_RED,
                     "Flames erupt beneath you! -10 HP");
            break;
        }
        case TRAP_TELEPORT: {
            /* Teleport to random floor tile */
            for (int tries = 0; tries < 200; tries++) {
                int rx = rng_range(2, MAP_WIDTH - 3);
                int ry = rng_range(2, MAP_HEIGHT - 3);
                if (dl->tiles[ry][rx].type == TILE_FLOOR) {
                    gs->player_pos = (Vec2){ rx, ry };
                    break;
                }
            }
            log_add(&gs->log, gs->turn, CP_MAGENTA,
                     "A teleport trap! The world spins... you're somewhere else!");
            break;
        }
        case TRAP_GAS:
            log_add(&gs->log, gs->turn, CP_GREEN,
                     "Poison gas fills the air! You feel confused.");
            break;
        case TRAP_COLLAPSE: {
            gs->hp -= 15;
            log_add(&gs->log, gs->turn, CP_RED,
                     "The floor collapses beneath you! -15 HP");
            break;
        }
        case TRAP_SPIKE: {
            gs->hp -= 6;
            log_add(&gs->log, gs->turn, CP_RED,
                     "Spikes shoot up from the floor! -6 HP");
            break;
        }
        case TRAP_MANA_DRAIN: {
            int drain = rng_range(10, 20);
            gs->mp -= drain;
            if (gs->mp < 0) gs->mp = 0;
            log_add(&gs->log, gs->turn, CP_MAGENTA,
                     "A magical trap drains your mana! -%d MP", drain);
            break;
        }
        default: break;
        }

        if (gs->hp < 1) gs->hp = 1;  /* don't die from traps yet (death in Phase 13) */
    }

    /* Passive trap detection -- chance to spot nearby traps */
    for (int i = 0; i < dl->num_traps; i++) {
        Trap *t = &dl->traps[i];
        if (t->triggered || t->revealed) continue;
        int dx = abs(t->pos.x - gs->player_pos.x);
        int dy = abs(t->pos.y - gs->player_pos.y);
        if (dx <= 2 && dy <= 2) {
            /* 10% base chance to notice a nearby trap */
            if (rng_chance(10)) {
                t->revealed = true;
                dl->tiles[t->pos.y][t->pos.x].glyph = '^';
                dl->tiles[t->pos.y][t->pos.x].color_pair = CP_RED;
                log_add(&gs->log, gs->turn, CP_YELLOW,
                         "You notice a trap on the floor! (^)");
            }
        }
    }
}

/* Stub for old function -- no longer used */
void game_init_test_map(GameState *gs) { (void)gs; }

/* Recompute FOV for the current dungeon level */
/* Find a monster at position on current dungeon level */
static Entity *monster_at(GameState *gs, int x, int y) {
    DungeonLevel *dl = current_dungeon_level(gs);
    if (!dl) return NULL;
    for (int i = 0; i < dl->num_monsters; i++) {
        Entity *e = &dl->monsters[i];
        if (e->alive && e->pos.x == x && e->pos.y == y)
            return e;
    }
    return NULL;
}

/* Player attacks a monster -- with hit/miss and crits */
static void combat_attack_monster(GameState *gs, Entity *target) {
    /* Hit chance: 70% base + player STR - monster DEF*2 */
    int hit_chance = 70 + gs->str - target->def * 2;
    if (hit_chance < 15) hit_chance = 15;
    if (hit_chance > 95) hit_chance = 95;

    if (!rng_chance(hit_chance)) {
        log_add(&gs->log, gs->turn, CP_GRAY,
                 "You swing at the %s but miss!", target->name);
        return;
    }

    int weapon_pow = (gs->equipment[SLOT_WEAPON].template_id >= 0) ? gs->equipment[SLOT_WEAPON].power : 0;
    /* Mounted charge bonus: +2 STR on overworld */
    int mount_str = (gs->riding && gs->mode == MODE_OVERWORLD) ? 2 : 0;
    int damage = gs->str + mount_str + weapon_pow + rng_range(-2, 2) - target->def;
    if (damage < 1) damage = 1;

    /* Critical hit: 10% chance, 2x damage */
    bool crit = rng_chance(10);
    if (crit) {
        damage *= 2;
        log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                 "CRITICAL HIT! You smash the %s for %d damage!", target->name, damage);
    } else {
        log_add(&gs->log, gs->turn, CP_WHITE,
                 "You hit the %s for %d damage.", target->name, damage);
    }

    target->hp -= damage;

    /* Vampirism: drain 2 HP per melee hit */
    if (gs->has_vampirism && damage > 0) {
        gs->hp += 2;
        if (gs->hp > gs->max_hp) gs->hp = gs->max_hp;
    }

    if (target->hp <= 0) {
        target->alive = false;
        gs->xp += target->xp_reward;
        gs->kills++;
        log_add(&gs->log, gs->turn, CP_YELLOW,
                 "The %s is slain! +%d XP (Kills: %d)", target->name, target->xp_reward, gs->kills);
        /* On-death effects */
        ai_explode_on_death(gs, target);

        /* Princess rescue: killing Evil Sorcerer in the tower frees the princess */
        if (gs->princess_quest_active && !gs->princess_rescued &&
            strcmp(target->name, "Evil Sorcerer") == 0 &&
            gs->dungeon && strcmp(gs->dungeon->name, "Sorcerer's Tower") == 0) {
            gs->princess_rescued = true;
            log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                     "*** The Evil Sorcerer falls! The princess is FREE! ***");
            log_add(&gs->log, gs->turn, CP_WHITE,
                     "She thanks you and returns to her father's castle.");
            log_add(&gs->log, gs->turn, CP_YELLOW,
                     "Visit any castle king to claim your reward!");
        }

        /* Monster drops */
        DungeonLevel *dl = current_dungeon_level(gs);
        if (dl) {
            /* Determine drop type from monster's AI flags stored on entity */
            int tcount;
            const MonsterTemplate *tmps = entity_get_templates(&tcount);
            DropType drop = DROP_NONE;
            for (int ti = 0; ti < tcount; ti++) {
                if (strcmp(tmps[ti].name, target->name) == 0) {
                    drop = tmps[ti].drop;
                    break;
                }
            }

            if (drop == DROP_GOLD) {
                int gold = rng_range(3, 15);
                gs->gold += gold;
                log_add(&gs->log, gs->turn, CP_YELLOW, "You find %d gold.", gold);
            } else if (drop == DROP_GOLD_LARGE) {
                int gold = rng_range(15, 50);
                gs->gold += gold;
                log_add(&gs->log, gs->turn, CP_YELLOW_BOLD, "You find %d gold!", gold);
            } else if (drop >= DROP_ITEM_WEAPON && dl->num_ground_items < MAX_GROUND_ITEMS) {
                /* Drop a themed item on the monster's death tile */
                ItemType wanted = ITYPE_WEAPON;
                if (drop == DROP_ITEM_ARMOR) wanted = ITYPE_ARMOR;
                else if (drop == DROP_ITEM_POTION) wanted = ITYPE_POTION;
                else if (drop == DROP_ITEM_FOOD) wanted = ITYPE_FOOD;

                int itcount;
                const ItemTemplate *itms = item_get_templates(&itcount);
                /* Find a matching item */
                for (int tries = 0; tries < 30; tries++) {
                    int ti = rng_range(0, itcount - 1);
                    if (itms[ti].type != wanted) continue;
                    dl->ground_items[dl->num_ground_items] =
                        item_create(ti, target->pos.x, target->pos.y);
                    dl->num_ground_items++;
                    log_add(&gs->log, gs->turn, CP_CYAN,
                             "The %s drops: %s!", target->name, itms[ti].name);
                    break;
                }
            }
        }
    }
}

/* Monster attacks the player -- with hit/miss */
static void combat_monster_attacks(GameState *gs, Entity *attacker) {
    int hit_chance = 60 + attacker->str - gs->def * 2;
    if (hit_chance < 10) hit_chance = 10;
    if (hit_chance > 90) hit_chance = 90;

    if (!rng_chance(hit_chance)) {
        log_add(&gs->log, gs->turn, CP_GRAY,
                 "The %s attacks but you dodge!", attacker->name);
        return;
    }

    int armor_pow = 0;
    for (int s = SLOT_ARMOR; s <= SLOT_GLOVES; s++)
        if (gs->equipment[s].template_id >= 0) armor_pow += gs->equipment[s].power;
    /* Destrier war horse grants +2 DEF when mounted */
    int mount_def = (gs->riding && gs->horse_type == 3) ? 2 : 0;
    int damage = attacker->str + rng_range(-2, 2) - gs->def - armor_pow - mount_def;
    if (damage < 1) damage = 1;

    /* Shield spell absorb */
    if (gs->shield_absorb > 0) {
        int absorbed = damage;
        if (absorbed > gs->shield_absorb) absorbed = gs->shield_absorb;
        gs->shield_absorb -= absorbed;
        damage -= absorbed;
        if (absorbed > 0)
            log_add(&gs->log, gs->turn, CP_CYAN, "Shield absorbs %d damage!", absorbed);
        if (damage <= 0) {
            if (gs->shield_absorb <= 0)
                log_add(&gs->log, gs->turn, CP_GRAY, "Your magic shield fades.");
            return;
        }
    }

    gs->hp -= damage;

    log_add(&gs->log, gs->turn, CP_RED,
             "The %s hits you for %d damage!", attacker->name, damage);

    /* Werewolf bite: 10% chance of lycanthropy (20% for Alpha) */
    if (!gs->has_lycanthropy &&
        (strstr(attacker->name, "Werewolf") || strstr(attacker->name, "werewolf"))) {
        int bite_chance = strstr(attacker->name, "Alpha") ? 20 : 10;
        if (rng_chance(bite_chance)) {
            gs->has_lycanthropy = true;
            gs->str += 3;
            gs->spd += 2;
            log_add(&gs->log, gs->turn, CP_RED_BOLD,
                     "The werewolf's bite burns! You feel a dark change within...");
            log_add(&gs->log, gs->turn, CP_YELLOW,
                     "LYCANTHROPY! +3 STR, +2 SPD, but beware the full moon!");
        }
    }

    /* Vampire bite: 10% chance of vampirism */
    if (!gs->has_vampirism && !gs->has_lycanthropy &&
        (strstr(attacker->name, "Vampire") || strstr(attacker->name, "vampire"))) {
        if (rng_chance(10)) {
            gs->has_vampirism = true;
            gs->str += 3;
            gs->spd += 2;
            gs->chivalry -= 10;
            if (gs->chivalry < 0) gs->chivalry = 0;
            log_add(&gs->log, gs->turn, CP_RED_BOLD,
                     "The vampire's fangs pierce your neck! You feel cold...");
            log_add(&gs->log, gs->turn, CP_MAGENTA,
                     "VAMPIRISM! +3 STR, +2 SPD, HP drain on attacks, no hunger.");
            log_add(&gs->log, gs->turn, CP_RED,
                     "But sunlight will burn you! Stay indoors during the day. -10 chivalry.");
        }
    }

    if (gs->hp <= 0) {
        gs->hp = 0;
        snprintf(gs->cause_of_death, sizeof(gs->cause_of_death),
                 "Slain by %s", attacker->name);
        show_death_screen(gs);
        gs->running = false;
    }
}

/* ------------------------------------------------------------------ */
/* Monster AI helper: try to move to nx,ny (handles ghosts and doors)  */
/* ------------------------------------------------------------------ */
static bool ai_try_move(GameState *gs, Entity *e, int nx, int ny) {
    DungeonLevel *dl = current_dungeon_level(gs);
    if (!dl) return false;
    if (nx < 1 || nx >= MAP_WIDTH - 1 || ny < 1 || ny >= MAP_HEIGHT - 1) return false;
    if (nx == gs->player_pos.x && ny == gs->player_pos.y) return false;
    if (monster_at(gs, nx, ny)) return false;

    Tile *t = &dl->tiles[ny][nx];

    /* Ghost mode: walk through walls */
    if ((e->ai_flags & AI_GHOST) && t->type == TILE_WALL) {
        e->pos.x = nx;
        e->pos.y = ny;
        return true;
    }

    /* Open doors if capable */
    if (t->type == TILE_DOOR_CLOSED && (e->ai_flags & AI_OPENS_DOORS)) {
        t->type = TILE_DOOR_OPEN;
        t->glyph = '/';
        t->blocks_sight = false;
        t->passable = true;
        return true; /* spend turn opening */
    }

    if (!t->passable) return false;

    e->pos.x = nx;
    e->pos.y = ny;
    return true;
}

/* ------------------------------------------------------------------ */
/* Monster special abilities                                           */
/* ------------------------------------------------------------------ */

/* Breath weapon: damage in a 3-tile line toward player */
static bool ai_breathe_fire(GameState *gs, Entity *e, int dist_sq) {
    if (!(e->ai_flags & AI_BREATHE_FIRE)) return false;
    if (e->ability_cd > 0) return false;
    if (dist_sq > 16 || dist_sq < 2) return false; /* range 2-4 */

    int damage = e->str / 2 + rng_range(3, 8);
    int def_reduce = gs->def / 3;
    damage -= def_reduce;
    if (damage < 2) damage = 2;
    gs->hp -= damage;
    e->ability_cd = 4; /* 4-turn cooldown */
    log_add(&gs->log, gs->turn, CP_RED_BOLD,
             "The %s breathes fire! -%d HP!", e->name, damage);
    return true;
}

/* Summoning: spawn an ally nearby */
static bool ai_summon(GameState *gs, Entity *e) {
    if (!(e->ai_flags & AI_SUMMON)) return false;
    if (e->summon_cd > 0) return false;

    DungeonLevel *dl = current_dungeon_level(gs);
    if (!dl || dl->num_monsters >= MAX_MONSTERS_PER_LEVEL - 1) return false;

    /* Find an open tile adjacent to the monster */
    for (int tries = 0; tries < 8; tries++) {
        int d = rng_range(0, 7);
        int nx = e->pos.x + dir_dx[d];
        int ny = e->pos.y + dir_dy[d];
        if (nx < 1 || nx >= MAP_WIDTH - 1 || ny < 1 || ny >= MAP_HEIGHT - 1) continue;
        if (!dl->tiles[ny][nx].passable) continue;
        if (monster_at(gs, nx, ny)) continue;
        if (nx == gs->player_pos.x && ny == gs->player_pos.y) continue;

        /* Create a weak summon based on summoner type */
        Entity *s = &dl->monsters[dl->num_monsters++];
        memset(s, 0, sizeof(*s));
        s->alive = true;
        s->pos = (Vec2){ nx, ny };
        s->energy = 0;
        s->ai_state = AI_STATE_CHASE;

        if (e->category == MCAT_UNDEAD || strstr(e->name, "Necromancer")) {
            snprintf(s->name, MAX_NAME, "Skeleton");
            s->glyph = 'z'; s->color_pair = CP_WHITE;
            s->hp = s->max_hp = 8; s->str = 4; s->def = 2; s->spd = 3;
            s->xp_reward = 5; s->category = MCAT_UNDEAD;
        } else if (strstr(e->name, "Witch")) {
            snprintf(s->name, MAX_NAME, "Giant Spider");
            s->glyph = 'S'; s->color_pair = CP_GREEN;
            s->hp = s->max_hp = 8; s->str = 4; s->def = 1; s->spd = 4;
            s->xp_reward = 5; s->category = MCAT_BEAST;
        } else {
            snprintf(s->name, MAX_NAME, "Imp");
            s->glyph = 'i'; s->color_pair = CP_RED;
            s->hp = s->max_hp = 6; s->str = 3; s->def = 1; s->spd = 5;
            s->xp_reward = 4; s->category = MCAT_MAGICAL;
            s->ai_flags = AI_ERRATIC;
        }

        e->summon_cd = 8; /* 8-turn cooldown */
        log_add(&gs->log, gs->turn, CP_MAGENTA,
                 "The %s summons a %s!", e->name, s->name);
        return true;
    }
    return false;
}

/* Heal: restore HP to nearby wounded ally */
static bool ai_heal_allies(GameState *gs, Entity *e) {
    if (!(e->ai_flags & AI_HEAL_ALLIES)) return false;
    if (e->ability_cd > 0) return false;

    DungeonLevel *dl = current_dungeon_level(gs);
    if (!dl) return false;

    for (int j = 0; j < dl->num_monsters; j++) {
        Entity *ally = &dl->monsters[j];
        if (!ally->alive || ally == e) continue;
        if (ally->hp >= ally->max_hp) continue;

        int adx = abs(ally->pos.x - e->pos.x);
        int ady = abs(ally->pos.y - e->pos.y);
        if (adx > 3 || ady > 3) continue;

        int heal = rng_range(3, 8);
        ally->hp += heal;
        if (ally->hp > ally->max_hp) ally->hp = ally->max_hp;
        e->ability_cd = 4;

        if (dl->tiles[e->pos.y][e->pos.x].visible)
            log_add(&gs->log, gs->turn, CP_GREEN,
                     "The %s heals the %s! (+%d HP)", e->name, ally->name, heal);
        return true;
    }
    return false;
}

/* Debuff: curse the player (temporary stat reduction) */
static bool ai_debuff_player(GameState *gs, Entity *e, int dist_sq) {
    if (!(e->ai_flags & AI_DEBUFF)) return false;
    if (e->ability_cd > 0) return false;
    if (dist_sq > 25) return false; /* range 5 */
    if (!rng_chance(30)) return false; /* 30% chance per turn */

    /* Pick a random stat to reduce (minimum 1) */
    int stat = rng_range(0, 3);
    const char *stat_name;
    switch (stat) {
    case 0: if (gs->str > 1) gs->str -= 1; stat_name = "STR"; break;
    case 1: if (gs->def > 1) gs->def -= 1; stat_name = "DEF"; break;
    case 2: if (gs->intel > 1) gs->intel -= 1; stat_name = "INT"; break;
    default: if (gs->spd > 1) gs->spd -= 1; stat_name = "SPD"; break;
    }
    e->ability_cd = 6;
    log_add(&gs->log, gs->turn, CP_MAGENTA,
             "The %s curses you! -%s", e->name, stat_name);
    return true;
}

/* Explode on death: damage adjacent entities */
static void ai_explode_on_death(GameState *gs, Entity *e) {
    if (!(e->ai_flags & AI_EXPLODE_DEATH)) return;

    int ex = e->pos.x, ey = e->pos.y;
    /* Damage player if adjacent */
    int pdx = abs(gs->player_pos.x - ex);
    int pdy = abs(gs->player_pos.y - ey);
    if (pdx <= 1 && pdy <= 1) {
        int damage = rng_range(2, 5);
        gs->hp -= damage;
        log_add(&gs->log, gs->turn, CP_RED_BOLD,
                 "The %s explodes! -%d HP!", e->name, damage);
    }
}

/* ------------------------------------------------------------------ */
/* Process enemy turns -- FSM AI with A* pathfinding                   */
/* ------------------------------------------------------------------ */
static void dungeon_enemy_turns(GameState *gs) {
    DungeonLevel *dl = current_dungeon_level(gs);
    if (!dl) return;

    /* Timed spawning: every ~80 turns, spawn a new monster outside FOV */
    if (gs->turn % 80 == 0) {
        int radius = (gs->has_torch || gs->has_vampirism) ? FOV_RADIUS : 2;
        if (entity_spawn_one(dl->monsters, &dl->num_monsters,
                              dl->tiles, dl->depth, gs->player_pos, radius)) {
            /* Spawned silently -- player won't know */
        }
    }

    for (int i = 0; i < dl->num_monsters; i++) {
        Entity *e = &dl->monsters[i];
        if (!e->alive) continue;

        /* Tick cooldowns */
        if (e->ability_cd > 0) e->ability_cd--;
        if (e->summon_cd > 0) e->summon_cd--;

        int dx = e->pos.x - gs->player_pos.x;
        int dy = e->pos.y - gs->player_pos.y;
        int dist_sq = dx * dx + dy * dy;

        /* ---- AI State Machine Update ---- */
        bool player_visible = dl->tiles[e->pos.y][e->pos.x].visible;
        int awake_range = (FOV_RADIUS + 5) * (FOV_RADIUS + 5);

        /* Cat scares rats -- they flee! */
        if (gs->has_cat && (e->glyph == 'r' || strstr(e->name, "Rat"))) {
            e->ai_state = AI_STATE_FLEE;
        }

        /* IDLE → CHASE: player in sight or always-chase flag */
        if (e->ai_state == AI_STATE_IDLE) {
            if ((e->ai_flags & AI_ALWAYS_CHASE) && dist_sq <= awake_range) {
                e->ai_state = AI_STATE_CHASE;
            } else if (player_visible && dist_sq <= awake_range) {
                e->ai_state = AI_STATE_CHASE;
                e->last_seen_x = gs->player_pos.x;
                e->last_seen_y = gs->player_pos.y;
            }
        }

        /* CHASE → FLEE: low HP */
        if (e->ai_state == AI_STATE_CHASE &&
            (e->ai_flags & AI_FLEES_LOW_HP) && e->hp * 4 < e->max_hp) {
            e->ai_state = AI_STATE_FLEE;
        }

        /* FLEE → CHASE: HP recovered above 50% */
        if (e->ai_state == AI_STATE_FLEE && e->hp * 2 >= e->max_hp) {
            e->ai_state = AI_STATE_CHASE;
        }

        /* Update last known player position if visible */
        if (player_visible) {
            e->last_seen_x = gs->player_pos.x;
            e->last_seen_y = gs->player_pos.y;
        }

        /* Skip if still idle and out of range */
        if (e->ai_state == AI_STATE_IDLE && dist_sq > awake_range) continue;

        /* Energy system: accumulate energy based on speed, act when >= 30 */
        e->energy += e->spd * 10;
        if (e->energy < 30) continue;
        e->energy -= 30;

        /* ---- Fear aura effect ---- */
        if ((e->ai_flags & AI_FEAR_AURA) && dist_sq <= 9 && player_visible) {
            /* Player feels fear -- small chance to lose a turn */
            if (rng_chance(10)) {
                log_add(&gs->log, gs->turn, CP_MAGENTA,
                         "The %s's presence fills you with dread!", e->name);
            }
        }

        /* ---- AI: Erratic (random movement 25%) ---- */
        if ((e->ai_flags & AI_ERRATIC) && e->ai_state != AI_STATE_FLEE && rng_chance(25)) {
            int rd = rng_range(0, 7);
            int nx = e->pos.x + dir_dx[rd];
            int ny = e->pos.y + dir_dy[rd];
            ai_try_move(gs, e, nx, ny);
            continue;
        }

        /* ---- AI: FLEE state ---- */
        if (e->ai_state == AI_STATE_FLEE) {
            int step_x = (dx > 0) ? 1 : (dx < 0) ? -1 : 0;
            int step_y = (dy > 0) ? 1 : (dy < 0) ? -1 : 0;
            if (!ai_try_move(gs, e, e->pos.x + step_x, e->pos.y + step_y)) {
                /* Try perpendicular */
                ai_try_move(gs, e, e->pos.x + step_x, e->pos.y) ||
                ai_try_move(gs, e, e->pos.x, e->pos.y + step_y);
            }
            continue;
        }

        /* ---- Special abilities (before movement/melee) ---- */

        /* Breath weapon (dragons, bone dragon) */
        if (ai_breathe_fire(gs, e, dist_sq)) continue;

        /* Summoning (necromancer, witch, sorcerer) */
        if (dist_sq <= 36 && rng_chance(15) && ai_summon(gs, e)) continue;

        /* Heal nearby wounded allies (shamans, dark monks) */
        if (ai_heal_allies(gs, e)) continue;

        /* Debuff player (witches, wraiths, enchantresses) */
        if (ai_debuff_player(gs, e, dist_sq)) continue;

        /* Ranged attack at distance 2-5 */
        if ((e->ai_flags & AI_RANGED_ATTACK) && dist_sq >= 4 && dist_sq <= 25) {
            if (rng_chance(50)) {
                int damage = e->str / 2 + rng_range(-1, 2) - gs->def / 2;
                if (damage < 1) damage = 1;
                gs->hp -= damage;
                log_add(&gs->log, gs->turn, CP_RED,
                         "The %s hurls an attack at you! -%d HP", e->name, damage);
                if (gs->hp <= 0) {
                    gs->hp = 0;
                    snprintf(gs->cause_of_death, sizeof(gs->cause_of_death),
                             "Slain by %s (ranged)", e->name);
                    show_death_screen(gs);
                    gs->running = false;
                    return;
                }
                continue;
            }
        }

        /* ---- Melee: if adjacent to player, attack ---- */
        if (abs(dx) <= 1 && abs(dy) <= 1) {
            combat_monster_attacks(gs, e);
            continue;
        }

        /* ---- Movement: A* pathfinding toward player ---- */
        if (e->ai_state == AI_STATE_CHASE || (e->ai_flags & AI_ALWAYS_CHASE)) {
            int target_x = gs->player_pos.x, target_y = gs->player_pos.y;

            /* If we can't see the player, go to last known position */
            if (!player_visible && e->last_seen_x > 0) {
                target_x = e->last_seen_x;
                target_y = e->last_seen_y;
                /* If we reached last known position, go idle */
                if (e->pos.x == target_x && e->pos.y == target_y) {
                    if (!(e->ai_flags & AI_ALWAYS_CHASE))
                        e->ai_state = AI_STATE_IDLE;
                    continue;
                }
            }

            int next_x, next_y;
            bool ghost = (e->ai_flags & AI_GHOST) != 0;
            int path_len = pathfind_astar(dl->tiles, e->pos.x, e->pos.y,
                                           target_x, target_y, &next_x, &next_y,
                                           ghost);

            if (path_len > 0 && (next_x != e->pos.x || next_y != e->pos.y)) {
                if (!ai_try_move(gs, e, next_x, next_y)) {
                    /* A* step blocked by another monster, try greedy fallback */
                    int sx = (dx < 0) ? 1 : (dx > 0) ? -1 : 0;
                    int sy = (dy < 0) ? 1 : (dy > 0) ? -1 : 0;
                    ai_try_move(gs, e, e->pos.x + sx, e->pos.y + sy) ||
                    ai_try_move(gs, e, e->pos.x + sx, e->pos.y) ||
                    ai_try_move(gs, e, e->pos.x, e->pos.y + sy);
                }
            } else {
                /* No path found -- greedy fallback */
                int sx = (dx < 0) ? 1 : (dx > 0) ? -1 : 0;
                int sy = (dy < 0) ? 1 : (dy > 0) ? -1 : 0;
                ai_try_move(gs, e, e->pos.x + sx, e->pos.y + sy) ||
                ai_try_move(gs, e, e->pos.x + sx, e->pos.y) ||
                ai_try_move(gs, e, e->pos.x, e->pos.y + sy);
            }
        }
    }
}

/* Spawn items on a dungeon level */
/* Check if player stepped on a dungeon chest */
static void check_chests(GameState *gs) {
    DungeonLevel *dl = current_dungeon_level(gs);
    if (!dl) return;

    for (int i = 0; i < dl->num_chests; i++) {
        DungeonChest *ch = &dl->chests[i];
        if (ch->opened) continue;
        if (ch->pos.x != gs->player_pos.x || ch->pos.y != gs->player_pos.y) continue;

        /* Mimic! */
        if (ch->mimic) {
            ch->opened = true;
            dl->tiles[ch->pos.y][ch->pos.x].glyph = '.';
            dl->tiles[ch->pos.y][ch->pos.x].color_pair = CP_WHITE;
            log_add(&gs->log, gs->turn, CP_RED_BOLD,
                     "The chest springs to life -- it's a MIMIC!");
            int dmg = rng_range(8, 15) - gs->def;
            if (dmg < 2) dmg = 2;
            gs->hp -= dmg;
            if (gs->hp < 1) gs->hp = 1;
            int gold = rng_range(20, 60);
            gs->gold += gold;
            gs->xp += 25;
            gs->kills++;
            log_add(&gs->log, gs->turn, CP_YELLOW,
                     "You defeat the Mimic! -%d HP, +%dg, +25 XP.", dmg, gold);
            return;
        }

        /* Locked chest */
        if (ch->locked) {
            bool has_lockpick = false;
            for (int ii = 0; ii < gs->num_items; ii++)
                if (strcmp(gs->inventory[ii].name, "Lockpick") == 0) { has_lockpick = true; break; }
            bool is_ranger = (gs->player_class == CLASS_RANGER);

            if (is_ranger || has_lockpick) {
                log_add(&gs->log, gs->turn, CP_CYAN,
                         "You pick the lock on the chest%s.", is_ranger ? " (Ranger skill)" : "");
                ch->locked = false;
            } else {
                /* Bash attempt */
                log_add(&gs->log, gs->turn, CP_RED,
                         "The chest is locked! You bash it open with brute force...");
                if (rng_chance(30)) {
                    log_add(&gs->log, gs->turn, CP_RED,
                             "CRASH! You smashed the contents. Nothing salvageable.");
                    ch->opened = true;
                    dl->tiles[ch->pos.y][ch->pos.x].glyph = '.';
                    dl->tiles[ch->pos.y][ch->pos.x].color_pair = CP_WHITE;
                    return;
                }
                ch->locked = false;
            }
        }

        /* Trapped chest */
        if (ch->trapped) {
            int trap_type = rng_range(0, 2);
            if (trap_type == 0) {
                int dmg = rng_range(3, 8);
                gs->hp -= dmg;
                if (gs->hp < 1) gs->hp = 1;
                log_add(&gs->log, gs->turn, CP_RED,
                         "Poison dart! The chest was trapped! -%d HP", dmg);
            } else if (trap_type == 1) {
                int dmg = rng_range(5, 10);
                gs->hp -= dmg;
                if (gs->hp < 1) gs->hp = 1;
                log_add(&gs->log, gs->turn, CP_RED,
                         "BOOM! The chest explodes! -%d HP", dmg);
            } else {
                log_add(&gs->log, gs->turn, CP_RED,
                         "Gas cloud! You feel confused and disoriented.");
            }
        }

        /* Open the chest -- give loot */
        ch->opened = true;
        dl->tiles[ch->pos.y][ch->pos.x].glyph = '.';
        dl->tiles[ch->pos.y][ch->pos.x].color_pair = CP_WHITE;

        int gold = rng_range(10 + dl->depth * 5, 40 + dl->depth * 10);
        gs->gold += gold;
        log_add(&gs->log, gs->turn, CP_YELLOW,
                 "You open the chest! Found %d gold!", gold);

        /* Random item from the chest */
        if (gs->num_items < MAX_INVENTORY) {
            int tcount; const ItemTemplate *tmps = item_get_templates(&tcount);
            for (int tries = 0; tries < 30; tries++) {
                int ri = rng_range(0, tcount - 1);
                if (tmps[ri].rarity >= 2 && tmps[ri].min_depth <= dl->depth) {
                    gs->inventory[gs->num_items] = item_create(ri, -1, -1);
                    gs->inventory[gs->num_items].on_ground = false;
                    gs->num_items++;
                    log_add(&gs->log, gs->turn, CP_CYAN,
                             "Also found: %s", tmps[ri].name);
                    break;
                }
            }
        }
        return;
    }
}

static void dungeon_spawn_items(DungeonLevel *dl) {
    int num_items = rng_range(8, 16 + dl->depth * 2);
    if (num_items > MAX_GROUND_ITEMS - 10) num_items = MAX_GROUND_ITEMS - 10;  /* leave room for drops */

    int tcount;
    const ItemTemplate *tmps = item_get_templates(&tcount);
    dl->num_ground_items = 0;

    /* Spawn gold piles (2-4 per level) */
    int num_gold = rng_range(2, 4);
    for (int g = 0; g < num_gold && dl->num_ground_items < MAX_GROUND_ITEMS; g++) {
        for (int tries = 0; tries < 200; tries++) {
            int x = rng_range(3, MAP_WIDTH - 4);
            int y = rng_range(3, MAP_HEIGHT - 4);
            if (!dl->tiles[y][x].passable) continue;
            char tg = dl->tiles[y][x].glyph;
            if (tg == '<' || tg == '>' || tg == '0' || tg == '(' || tg == '=' || tg == '_') continue;
            /* Prefer rooms (many open neighbours) */
            int open = 0;
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++) {
                    int nx = x + dx, ny = y + dy;
                    if (nx > 0 && nx < MAP_WIDTH-1 && ny > 0 && ny < MAP_HEIGHT-1 &&
                        dl->tiles[ny][nx].passable) open++;
                }
            if (open < 5) continue;  /* skip corridors */

            Item gold;
            memset(&gold, 0, sizeof(gold));
            gold.template_id = -2;  /* special: gold pile */
            int amount = rng_range(5, 20 + dl->depth * 5);
            snprintf(gold.name, sizeof(gold.name), "%d gold coins", amount);
            gold.glyph = '$';
            gold.color_pair = CP_YELLOW_BOLD;
            gold.type = ITYPE_GOLD;
            gold.value = amount;
            gold.power = amount;
            gold.weight = 0;
            gold.pos = (Vec2){ x, y };
            gold.on_ground = true;
            dl->ground_items[dl->num_ground_items++] = gold;
            break;
        }
    }

    /* Spawn regular items -- favour rooms over corridors */
    for (int i = 0; i < num_items && dl->num_ground_items < MAX_GROUND_ITEMS; i++) {
        /* Pick a depth-appropriate item with weighted selection */
        int best = -1, best_w = 0;
        for (int tries = 0; tries < 30; tries++) {
            int ti = rng_range(0, tcount - 1);
            if (tmps[ti].min_depth > dl->depth) continue;
            if (tmps[ti].max_depth > 0 && tmps[ti].max_depth < dl->depth) continue;
            int w = tmps[ti].rarity;
            if (w > best_w) { best = ti; best_w = w; }
        }
        if (best < 0) continue;

        /* Find a floor tile -- prefer rooms (70%) over corridors (30%) */
        for (int tries = 0; tries < 300; tries++) {
            int x = rng_range(3, MAP_WIDTH - 4);
            int y = rng_range(3, MAP_HEIGHT - 4);
            if (!dl->tiles[y][x].passable) continue;
            { char gg = dl->tiles[y][x].glyph;
              if (gg == '<' || gg == '>' || gg == '0' || gg == '(' || gg == '=' || gg == '_') continue; }

            /* Check if in a room */
            int open = 0;
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++) {
                    int nx = x + dx, ny = y + dy;
                    if (nx > 0 && nx < MAP_WIDTH-1 && ny > 0 && ny < MAP_HEIGHT-1 &&
                        dl->tiles[ny][nx].passable) open++;
                }
            bool in_room = (open >= 5);
            /* 70% chance to require room placement */
            if (!in_room && rng_chance(70)) continue;

            dl->ground_items[dl->num_ground_items] = item_create(best, x, y);
            dl->num_ground_items++;
            break;
        }
    }
}

static void dungeon_update_fov(GameState *gs) {
    if (!gs->dungeon) return;
    DungeonLevel *dl = current_dungeon_level(gs);
    if (!dl) return;
    int radius = (gs->has_torch || gs->has_vampirism) ? FOV_RADIUS : 2;
    fov_compute((Tile *)dl->tiles, MAP_WIDTH, MAP_HEIGHT, gs->player_pos, radius);
}

/* ------------------------------------------------------------------ */
/* Direction helpers                                                    */
/* ------------------------------------------------------------------ */

static Direction key_to_direction(int key) {
    switch (key) {
    case 'k': case KEY_UP:    case '8': return DIR_N;
    case 'l': case KEY_RIGHT: case '6': return DIR_E;
    case 'j': case KEY_DOWN:  case '2': return DIR_S;
    case 'h': case KEY_LEFT:  case '4': return DIR_W;
    case 'y': case '7': return DIR_NW;
    case 'u': case '9': return DIR_NE;
    case 'b': case '1': return DIR_SW;
    case 'n': case '3': return DIR_SE;
    default: return DIR_NONE;
    }
}

/* Advance game clock by the given number of minutes */
static void advance_time(GameState *gs, int minutes) {
    gs->turn++;
    gs->minute += minutes;
    while (gs->minute >= 60) {
        gs->minute -= 60;
        gs->hour++;
        if (gs->hour >= 24) {
            gs->hour = 0;
            gs->day++;
        }
    }

    /* Tick spell buff durations */
    if (gs->buff_str_turns > 0) {
        gs->buff_str_turns--;
        if (gs->buff_str_turns == 0) { gs->str -= 3; if (gs->str < 1) gs->str = 1; }
    }
    if (gs->buff_def_turns > 0) {
        gs->buff_def_turns--;
        if (gs->buff_def_turns == 0) { gs->def -= 3; if (gs->def < 1) gs->def = 1; }
    }
    if (gs->buff_spd_turns > 0) {
        gs->buff_spd_turns--;
        if (gs->buff_spd_turns == 0) { gs->spd -= 3; if (gs->spd < 1) gs->spd = 1; }
    }

    /* King travel: every 50 turns, a king may appear on a road */
    if (gs->mode == MODE_OVERWORLD && gs->turn - gs->king_travel_turn >= 50) {
        gs->king_travel_turn = gs->turn;
        if (rng_chance(20) && gs->overworld->num_creatures < MAX_OW_CREATURES) {
            const char *king_names[] = {
                "King Galahaut", "King Pellam", "King Ban", "King Carados",
                "King Agwisance", "King Howell", "King Bohrs"
            };
            int ki = rng_range(0, 6);
            /* Place on a road near player */
            for (int tries = 0; tries < 100; tries++) {
                int kx = gs->player_pos.x + rng_range(-30, 30);
                int ky = gs->player_pos.y + rng_range(-30, 30);
                if (kx > 0 && kx < OW_WIDTH - 1 && ky > 0 && ky < OW_HEIGHT - 1 &&
                    gs->overworld->map[ky][kx].type == TILE_ROAD &&
                    !overworld_creature_at(gs->overworld, kx, ky)) {
                    OWCreature *c = &gs->overworld->creatures[gs->overworld->num_creatures++];
                    memset(c, 0, sizeof(*c));
                    c->type = OW_NPC_TRAVELLER;
                    c->pos = (Vec2){ kx, ky };
                    c->glyph = 'K';
                    c->color_pair = CP_YELLOW_BOLD;
                    snprintf(c->name, MAX_NAME, "%s", king_names[ki]);
                    c->hostile = false;
                    break;
                }
            }
        }
    }

    /* Vampirism: sunlight damage when outdoors during daytime */
    if (gs->has_vampirism && gs->mode == MODE_OVERWORLD) {
        TimeOfDay tod = game_get_tod(gs);
        if (tod != TOD_NIGHT && tod != TOD_EVENING && tod != TOD_DAWN) {
            /* Daytime sunlight burns! 3 HP per 10 turns */
            if (gs->turn % 10 == 0) {
                gs->hp -= 3;
                if (gs->hp <= 5 && gs->hp > 0)
                    log_add(&gs->log, gs->turn, CP_RED,
                             "The sunlight sears your flesh! -%d HP. Seek shelter!", 3);
                if (gs->hp <= 0) {
                    gs->hp = 0;
                    snprintf(gs->cause_of_death, sizeof(gs->cause_of_death),
                             "Burned to ash by sunlight (Vampirism)");
                    show_death_screen(gs);
                    gs->running = false;
                    return;
                }
            }
        }
    }

    /* Track gold earned (for score) */
    if (gs->gold > gs->gold_earned) gs->gold_earned = gs->gold;

    /* Witch geas countdown and completion check */
    if (gs->witch_geas_turns > 0) {
        /* Check if task is completed based on type */
        bool geas_done = false;
        switch (gs->witch_geas_type) {
        case 0: /* "Bring me a gem" - have any gem in inventory */
            for (int ii = 0; ii < gs->num_items; ii++)
                if (gs->inventory[ii].type == ITYPE_GEM) { geas_done = true; break; }
            break;
        case 1: /* "Slay a wolf" - kill count increased since geas started */
            /* Simplified: any kill counts */
            if (gs->kills > 0 && rng_chance(5)) geas_done = true;
            break;
        case 2: /* "Stand in a stone circle at midnight" */
            if (gs->hour >= 23 || gs->hour < 1) {
                Location *loc = overworld_location_at(gs->overworld, gs->player_pos.x, gs->player_pos.y);
                if (loc && (loc->type == LOC_MAGIC_CIRCLE || strcmp(loc->name, "Stonehenge") == 0))
                    geas_done = true;
            }
            break;
        case 3: /* "Bring me a potion" - have any potion */
            for (int ii = 0; ii < gs->num_items; ii++)
                if (gs->inventory[ii].type == ITYPE_POTION) { geas_done = true; break; }
            break;
        case 4: /* "Fetch me a mushroom" - have mushroom */
            for (int ii = 0; ii < gs->num_items; ii++)
                if (strcmp(gs->inventory[ii].name, "Mushroom") == 0) { geas_done = true; break; }
            break;
        case 5: /* "Deliver offering to Canterbury" - be in Canterbury */
            if (gs->current_town && strcmp(gs->current_town->name, "Canterbury") == 0)
                geas_done = true;
            break;
        }

        if (geas_done) {
            gs->witch_geas_turns = 0;
            gs->dark_affinity += 2;
            gs->chivalry -= 2;
            if (gs->chivalry < 0) gs->chivalry = 0;
            log_add(&gs->log, gs->turn, CP_MAGENTA_BOLD,
                     "The witch's geas is fulfilled! The curse lifts.");
            log_add(&gs->log, gs->turn, CP_MAGENTA,
                     "+2 Dark affinity, -2 chivalry. The witch cackles in the distance.");
            /* Consume the required item */
            for (int ii = 0; ii < gs->num_items; ii++) {
                bool consume = false;
                if (gs->witch_geas_type == 0 && gs->inventory[ii].type == ITYPE_GEM) consume = true;
                if (gs->witch_geas_type == 3 && gs->inventory[ii].type == ITYPE_POTION) consume = true;
                if (gs->witch_geas_type == 4 && strcmp(gs->inventory[ii].name, "Mushroom") == 0) consume = true;
                if (consume) {
                    for (int j = ii; j < gs->num_items - 1; j++)
                        gs->inventory[j] = gs->inventory[j + 1];
                    gs->inventory[--gs->num_items].template_id = -1;
                    break;
                }
            }
        }

        gs->witch_geas_turns--;
        if (gs->witch_geas_turns == 10) {
            log_add(&gs->log, gs->turn, CP_MAGENTA,
                     "The witch's geas burns! Only 10 turns left to complete the task!");
        }
        if (gs->witch_geas_turns <= 0) {
            /* Failed -- cursed! */
            gs->witch_geas_turns = 0;
            int *stats[] = { &gs->str, &gs->def, &gs->intel, &gs->spd };
            const char *snames[] = { "STR", "DEF", "INT", "SPD" };
            int si = rng_range(0, 3);
            if (*stats[si] > 2) *stats[si] -= 2; else *stats[si] = 1;
            gs->chivalry -= 3;
            if (gs->chivalry < 0) gs->chivalry = 0;
            log_add(&gs->log, gs->turn, CP_RED_BOLD,
                     "The witch's geas expires! You are cursed! -2 %s, -3 chivalry!", snames[si]);
            log_add(&gs->log, gs->turn, CP_GRAY,
                     "Cure at a church or with Purify spell.");
        }
    }

    /* Castle cat timer */
    if (gs->has_cat) {
        gs->cat_turns--;
        if (gs->cat_turns <= 0) {
            gs->has_cat = false;
            gs->spd -= 1; if (gs->spd < 1) gs->spd = 1;
            /* 30% chance cat leaves a gift */
            if (rng_chance(30)) {
                int gift = rng_range(1, 3);
                if (gift == 1) {
                    gs->gold += 5;
                    log_add(&gs->log, gs->turn, CP_YELLOW,
                             "The cat drops a shiny coin at your feet before leaving. (+5g)");
                } else if (gift == 2 && gs->num_items < MAX_INVENTORY) {
                    int tcount; const ItemTemplate *tmps = item_get_templates(&tcount);
                    for (int ti = 0; ti < tcount; ti++) {
                        if (strcmp(tmps[ti].name, "Dried Meat") == 0) {
                            gs->inventory[gs->num_items] = item_create(ti, -1, -1);
                            gs->inventory[gs->num_items].on_ground = false;
                            gs->num_items++;
                            log_add(&gs->log, gs->turn, CP_BROWN,
                                     "The cat drops a mouse at your feet before leaving. (Got: Dried Meat)");
                            break;
                        }
                    }
                } else {
                    log_add(&gs->log, gs->turn, CP_GRAY,
                             "The cat coughs up a hairball before leaving. Lovely.");
                }
            } else {
                log_add(&gs->log, gs->turn, CP_BROWN,
                         "The cat loses interest and saunters away...");
            }
        }
    }

    /* Torch/lantern fuel consumption */
    if (gs->has_torch && gs->torch_fuel > 0) {
        gs->torch_fuel--;
        if (gs->torch_fuel == 20) {
            log_add(&gs->log, gs->turn, CP_YELLOW,
                     "Your %s flickers... running low on fuel!",
                     gs->torch_type == 2 ? "lantern" : "torch");
        }
        if (gs->torch_fuel <= 0) {
            gs->has_torch = false;
            gs->torch_fuel = 0;
            log_add(&gs->log, gs->turn, CP_RED,
                     "Your %s goes out! You are plunged into darkness.",
                     gs->torch_type == 2 ? "lantern" : "torch");
            /* Lantern remains usable with oil refuel, torch is consumed */
            if (gs->torch_type == 1) gs->torch_type = 0;
        }
    }

    /* Horse waiting outside -- chance of wandering off */
    if (gs->horse_type > 0 && !gs->riding) {
        gs->horse_wait_turns++;
        /* In dungeon: 5% chance per 100 turns */
        if (gs->mode == MODE_DUNGEON && gs->horse_wait_turns % 100 == 0 && rng_chance(5)) {
            gs->horse_type = 0;
            log_add(&gs->log, gs->turn, CP_YELLOW,
                     "Your horse got bored and trotted away...");
        }
        /* In town: 10% chance after 200 turns */
        if (gs->mode == MODE_TOWN && gs->horse_wait_turns > 200 && rng_chance(10)) {
            gs->horse_type = 0;
            log_add(&gs->log, gs->turn, CP_YELLOW,
                     "Your horse grew restless and wandered off!");
        }
    } else {
        gs->horse_wait_turns = 0;
    }

    /* Slow MP regeneration: 1 MP per 20 turns */
    if (gs->turn % 20 == 0 && gs->mp < gs->max_mp) {
        gs->mp++;
    }

    /* XP level-up check */
    if (gs->player_level < MAX_LEVELS && !gs->pending_levelup) {
        int next_xp = xp_table[gs->player_level];
        if (gs->xp >= next_xp) {
            gs->pending_levelup = true;
            log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                     "*** LEVEL UP! Press Enter to allocate your stat point! ***");
        }
    }
}

/* ------------------------------------------------------------------ */
/* Time of day                                                         */
/* ------------------------------------------------------------------ */

TimeOfDay game_get_tod(const GameState *gs) {
    int h = gs->hour;
    if (h >= 22 || h < 5)  return TOD_NIGHT;
    if (h < 7)  return TOD_DAWN;
    if (h < 12) return TOD_MORNING;
    if (h < 14) return TOD_MIDDAY;
    if (h < 17) return TOD_AFTERNOON;
    if (h < 19) return TOD_DUSK;
    return TOD_EVENING;
}

static const char *time_of_day_name(int hour) {
    if (hour >= 5 && hour < 7)   return "[Dawn]";
    if (hour >= 7 && hour < 12)  return "[Morning]";
    if (hour >= 12 && hour < 14) return "[Midday]";
    if (hour >= 14 && hour < 17) return "[Afternoon]";
    if (hour >= 17 && hour < 19) return "[Dusk]";
    if (hour >= 19 && hour < 22) return "[Evening]";
    return "[Night]";
}

static bool is_night(int hour) {
    return hour >= 22 || hour < 5;
}

/* ------------------------------------------------------------------ */
/* Moon phase                                                          */
/* ------------------------------------------------------------------ */

int game_get_moon_day(const GameState *gs) {
    return ((gs->day - 1) % 30) + 1;
}

static const char *moon_phase_name(int moon_day) {
    if (moon_day == 1)                    return "New Moon";
    if (moon_day >= 2 && moon_day <= 7)   return "Waxing Crescent";
    if (moon_day == 8)                    return "First Quarter";
    if (moon_day >= 9 && moon_day <= 14)  return "Waxing Gibbous";
    if (moon_day == 15)                   return "Full Moon";
    if (moon_day >= 16 && moon_day <= 21) return "Waning Gibbous";
    if (moon_day == 22)                   return "Last Quarter";
    return "Waning Crescent";
}

/* Check for lunar event and notify player, apply gameplay effects */
static void check_lunar_events(GameState *gs, int old_hour) {
    int moon = game_get_moon_day(gs);
    /* Only notify once per day at dawn */
    if (old_hour < 5 && gs->hour >= 5) {
        switch (moon) {
        case 1:
            log_add(&gs->log, gs->turn, CP_CYAN,
                     "New Moon tonight. The darkness will be absolute.");
            break;
        case 5:
            /* Feast Day: free HP/MP restore */
            log_add(&gs->log, gs->turn, CP_YELLOW,
                     "Feast Day at Camelot! A grand celebration awaits.");
            gs->hp = gs->max_hp;
            gs->mp = gs->max_mp;
            log_add(&gs->log, gs->turn, CP_GREEN,
                     "The festive spirit restores you! Full HP and MP.");
            break;
        case 10:
            /* Market Day: bonus gold */
            {
                int bonus = rng_range(10, 30);
                gs->gold += bonus;
                log_add(&gs->log, gs->turn, CP_YELLOW,
                         "Market Day! Merchants gather with bargains. (+%dg found trading)", bonus);
            }
            break;
        case 12:
            /* Druid Gathering: nature affinity boost */
            log_add(&gs->log, gs->turn, CP_GREEN,
                     "The Druid Gathering at Stonehenge begins today.");
            gs->nature_affinity += 2;
            log_add(&gs->log, gs->turn, CP_GREEN, "Nature magic stirs. (+2 Nature affinity)");
            break;
        case 14:
            log_add(&gs->log, gs->turn, CP_WHITE,
                     "Tomorrow is the Full Moon. Beware the night...");
            break;
        case 15:
            /* Full Moon: werewolf danger */
            log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                     "FULL MOON tonight! Werewolves roam. Stay indoors.");
            if (gs->has_lycanthropy) {
                /* Lycanthropy transformation! */
                log_add(&gs->log, gs->turn, CP_RED_BOLD,
                         "The moon rises full... you feel the change coming... AWOOOO!");
                log_add(&gs->log, gs->turn, CP_RED,
                         "You black out. When you awake, it is dawn...");
                /* Teleport to random location */
                if (gs->mode == MODE_OVERWORLD) {
                    for (int t = 0; t < 200; t++) {
                        int rx = gs->player_pos.x + rng_range(-40, 40);
                        int ry = gs->player_pos.y + rng_range(-40, 40);
                        if (overworld_is_passable(gs->overworld, rx, ry)) {
                            gs->player_pos = (Vec2){ rx, ry };
                            break;
                        }
                    }
                }
                /* Advance to morning */
                gs->hour = 7; gs->minute = 0;
                /* Lose 1-2 random items */
                int lost = rng_range(1, 2);
                for (int li = 0; li < lost && gs->num_items > 0; li++) {
                    int idx = rng_range(0, gs->num_items - 1);
                    log_add(&gs->log, gs->turn, CP_RED,
                             "You lost your %s during the rampage!", gs->inventory[idx].name);
                    for (int j = idx; j < gs->num_items - 1; j++)
                        gs->inventory[j] = gs->inventory[j + 1];
                    gs->inventory[--gs->num_items].template_id = -1;
                }
                /* 20% chance killed an NPC */
                if (rng_chance(20)) {
                    gs->chivalry -= 5;
                    if (gs->chivalry < 0) gs->chivalry = 0;
                    log_add(&gs->log, gs->turn, CP_RED_BOLD,
                             "You have vague memories of violence... (-5 chivalry)");
                }
                /* Hunger restored */
                gs->hp = gs->max_hp;
                log_add(&gs->log, gs->turn, CP_GRAY,
                         "You awaken in an unfamiliar place. Full HP (you... ate something).");
                log_add(&gs->log, gs->turn, CP_WHITE,
                         "Cure lycanthropy: Purify spell, church cure, or Holy Water.");
            } else {
                log_add(&gs->log, gs->turn, CP_YELLOW,
                         "The moonlight fills you with restless energy. (+1 STR today)");
                gs->str += 1;
                gs->buff_str_turns = 100;
            }
            break;
        case 18:
            /* Tournament Day: XP bonus for combat */
            log_add(&gs->log, gs->turn, CP_YELLOW,
                     "Tournament Day! Jousting at castles. Combat XP doubled today!");
            /* Mark tournament active -- checked in combat_attack_monster */
            break;
        case 20:
            /* Holy Day: church blessings doubled, +chivalry */
            log_add(&gs->log, gs->turn, CP_WHITE,
                     "Holy Day. Sacred power fills the land.");
            gs->chivalry += 2;
            if (gs->chivalry > 100) gs->chivalry = 100;
            gs->light_affinity += 2;
            log_add(&gs->log, gs->turn, CP_WHITE,
                     "+2 chivalry, +2 Light affinity.");
            break;
        case 22:
            /* Witching Hour: dark affinity boost, danger */
            log_add(&gs->log, gs->turn, CP_MAGENTA,
                     "The Witching Hour approaches. Dark magic stirs.");
            gs->dark_affinity += 2;
            log_add(&gs->log, gs->turn, CP_MAGENTA, "+2 Dark affinity.");
            break;
        case 25:
            /* King's Court: chivalry bonus */
            log_add(&gs->log, gs->turn, CP_YELLOW,
                     "King's Court today! Arthur holds court at Camelot.");
            if (gs->quests.grail_quest_active) {
                gs->chivalry += 1;
                if (gs->chivalry > 100) gs->chivalry = 100;
                log_add(&gs->log, gs->turn, CP_YELLOW,
                         "Your quest is spoken of at court. (+1 chivalry)");
            }
            break;
        case 27:
            /* Night of Spirits: undead encounters more dangerous */
            log_add(&gs->log, gs->turn, CP_CYAN,
                     "Night of Spirits! The dead walk tonight. Beware dungeons.");
            break;
        case 30:
            /* Harvest Moon: free food */
            log_add(&gs->log, gs->turn, CP_GREEN,
                     "Harvest Moon. Food is plentiful today.");
            if (gs->num_items < MAX_INVENTORY) {
                int tcount; const ItemTemplate *tmps = item_get_templates(&tcount);
                for (int ti = 0; ti < tcount; ti++) {
                    if (strcmp(tmps[ti].name, "Bread") == 0) {
                        gs->inventory[gs->num_items] = item_create(ti, -1, -1);
                        gs->inventory[gs->num_items].on_ground = false;
                        gs->num_items++;
                        log_add(&gs->log, gs->turn, CP_GREEN,
                                 "You find fresh bread from the harvest! (+1 Bread)");
                        break;
                    }
                }
            }
            break;
        default:
            break;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Weather                                                             */
/* ------------------------------------------------------------------ */

const char *weather_name(WeatherType w) {
    switch (w) {
    case WEATHER_CLEAR: return "Clear";
    case WEATHER_RAIN:  return "Rain";
    case WEATHER_STORM: return "Storm";
    case WEATHER_FOG:   return "Fog";
    case WEATHER_SNOW:  return "Snow";
    case WEATHER_WIND:  return "Wind";
    default:            return "Clear";
    }
}

static const char *weather_icon(WeatherType w) {
    switch (w) {
    case WEATHER_CLEAR: return "*";
    case WEATHER_RAIN:  return "/";
    case WEATHER_STORM: return "!";
    case WEATHER_FOG:   return "~";
    case WEATHER_SNOW:  return "+";
    case WEATHER_WIND:  return ">";
    default:            return "*";
    }
}

/* Get regional weather weights based on player position.
   Returns weights for: clear, rain, storm, fog, snow, wind (must sum to 100) */
static void get_regional_weather(int px, int py, int w[6]) {
    /*                      clear rain storm fog snow wind */
    /* Default (midlands) */
    w[0]=40; w[1]=25; w[2]=8; w[3]=12; w[4]=5; w[5]=10;

    /* Scotland (y < 65) -- cold, wet, snowy highlands */
    if (py < 65) {
        w[0]=20; w[1]=30; w[2]=10; w[3]=10; w[4]=20; w[5]=10;
    }
    /* Northern England (y 65-110) -- rainy, windy moors */
    else if (py < 110) {
        w[0]=30; w[1]=30; w[2]=10; w[3]=12; w[4]=8; w[5]=10;
    }
    /* Wales (x < 140, y 110-160) -- very rainy, foggy hills */
    else if (px < 140 && py < 160) {
        w[0]=20; w[1]=35; w[2]=10; w[3]=20; w[4]=5; w[5]=10;
    }
    /* Southwest / Cornwall (x < 100, y > 160) -- mild, windy, rainy */
    else if (px < 100 && py > 160) {
        w[0]=30; w[1]=30; w[2]=10; w[3]=10; w[4]=2; w[5]=18;
    }
    /* Southeast / Kent (x > 300, y > 150) -- drier, clearer */
    else if (px > 300 && py > 150) {
        w[0]=50; w[1]=18; w[2]=7; w[3]=10; w[4]=3; w[5]=12;
    }
    /* South coast (y > 190) -- mild, windy */
    else if (py > 190) {
        w[0]=40; w[1]=22; w[2]=8; w[3]=8; w[4]=2; w[5]=20;
    }
    /* East coast (x > 280) -- cold wind off the sea */
    else if (px > 280) {
        w[0]=35; w[1]=20; w[2]=8; w[3]=15; w[4]=7; w[5]=15;
    }

    /* Whitby special: always raining (per plan) */
    if (px > 310 && px < 350 && py > 80 && py < 100) {
        w[0]=5; w[1]=55; w[2]=20; w[3]=15; w[4]=3; w[5]=2;
    }
}

static void change_weather(GameState *gs) {
    WeatherType old = gs->weather;

    /* Region-based weather probabilities */
    int w[6];
    get_regional_weather(gs->player_pos.x, gs->player_pos.y, w);

    int roll = rng_range(1, 100);
    int cumulative = 0;
    gs->weather = WEATHER_CLEAR;
    for (int i = 0; i < 6; i++) {
        cumulative += w[i];
        if (roll <= cumulative) {
            gs->weather = (WeatherType)i;
            break;
        }
    }

    gs->weather_turns_left = rng_range(80, 250);

    if (gs->weather != old && gs->mode == MODE_OVERWORLD) {
        switch (gs->weather) {
        case WEATHER_CLEAR:
            log_add(&gs->log, gs->turn, CP_YELLOW, "The skies clear. The sun shines warmly.");
            /* Rainbow event: 25% chance after rain clears during daytime */
            if ((old == WEATHER_RAIN || old == WEATHER_STORM) &&
                !is_night(gs->hour) && !gs->rainbow_active && rng_chance(25)) {
                /* Place rainbow end at a random passable tile 20-50 tiles from player */
                for (int tries = 0; tries < 200; tries++) {
                    int rx = gs->player_pos.x + rng_range(-50, 50);
                    int ry = gs->player_pos.y + rng_range(-50, 50);
                    int dx = rx - gs->player_pos.x;
                    int dy = ry - gs->player_pos.y;
                    int dist = dx*dx + dy*dy;
                    if (dist < 20*20 || dist > 50*50) continue;
                    if (rx < 5 || rx >= OW_WIDTH-5 || ry < 5 || ry >= OW_HEIGHT-5) continue;
                    if (!gs->overworld->map[ry][rx].passable) continue;

                    gs->rainbow_active = true;
                    gs->rainbow_end = (Vec2){ rx, ry };
                    gs->rainbow_turns = rng_range(30, 50);

                    /* Figure out direction hint */
                    const char *dir_name = "";
                    if (dy < -10 && abs(dx) < abs(dy)) dir_name = "to the north";
                    else if (dy > 10 && abs(dx) < abs(dy)) dir_name = "to the south";
                    else if (dx > 10 && abs(dy) < abs(dx)) dir_name = "to the east";
                    else if (dx < -10 && abs(dy) < abs(dx)) dir_name = "to the west";
                    else if (dx > 0 && dy < 0) dir_name = "to the northeast";
                    else if (dx < 0 && dy < 0) dir_name = "to the northwest";
                    else if (dx > 0 && dy > 0) dir_name = "to the southeast";
                    else dir_name = "to the southwest";

                    log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                             "A rainbow appears in the sky %s! Find its end!", dir_name);
                    break;
                }
            }
            break;
        case WEATHER_RAIN:
            log_add(&gs->log, gs->turn, CP_BLUE, "Dark clouds gather. Rain begins to fall.");
            break;
        case WEATHER_STORM:
            log_add(&gs->log, gs->turn, CP_BLUE, "Thunder rumbles! A fierce storm breaks overhead.");
            break;
        case WEATHER_FOG:
            log_add(&gs->log, gs->turn, CP_WHITE, "A thick fog rolls in. Visibility is poor.");
            break;
        case WEATHER_SNOW:
            log_add(&gs->log, gs->turn, CP_WHITE_BOLD, "Snow begins to fall. The land turns white.");
            break;
        case WEATHER_WIND:
            log_add(&gs->log, gs->turn, CP_CYAN, "A strong wind picks up, howling across the land.");
            break;
        }
    }
}

/* Weather speed modifier (added to travel time) */
static int weather_speed_penalty(WeatherType w) {
    switch (w) {
    case WEATHER_RAIN:  return 2;
    case WEATHER_STORM: return 5;
    case WEATHER_FOG:   return 1;
    case WEATHER_SNOW:  return 4;
    case WEATHER_WIND:  return 1;
    default:            return 0;
    }
}

/* Get travel time in minutes for stepping onto an overworld tile */
static int overworld_travel_time(GameState *gs, TileType type) {
    int base;
    switch (type) {
    case TILE_ROAD:    base = 5;  break;
    case TILE_BRIDGE:  base = 5;  break;
    case TILE_GRASS:   base = 10; break;
    case TILE_FOREST:  base = 20; break;
    case TILE_HILLS:   base = 25; break;
    case TILE_MARSH:   base = 30; break;
    case TILE_SWAMP:   base = 35; break;
    default:           base = 10; break;
    }
    int total = base + weather_speed_penalty(gs->weather);
    /* Horse speed bonus depends on type */
    if (gs->riding) {
        switch (gs->horse_type) {
        case 1: /* Pony: 1.5x speed, no hill penalty */
            if (type == TILE_HILLS) total = 10; /* hills same as grassland */
            else total = total * 2 / 3;
            break;
        case 2: /* Palfrey: 2x speed */
        case 3: /* Destrier: 2x speed */
            total = (total + 1) / 2;
            break;
        }
    }
    return total;
}

/* ------------------------------------------------------------------ */
/* Terrain name for status display                                     */
/* ------------------------------------------------------------------ */

static const char *terrain_name(TileType type) {
    switch (type) {
    case TILE_ROAD:   return "Road";
    case TILE_GRASS:  return "Grassland";
    case TILE_FOREST: return "Forest";
    case TILE_HILLS:    return "Hills";
    case TILE_MOUNTAIN: return "Mountains";
    case TILE_MARSH:  return "Marsh";
    case TILE_SWAMP:  return "Swamp";
    case TILE_BRIDGE: return "Bridge";
    case TILE_WATER:  return "Sea";
    case TILE_RIVER:  return "River";
    case TILE_LAKE:   return "Lake";
    default:          return "";
    }
}

/* ------------------------------------------------------------------ */
/* Overworld input                                                     */
/* ------------------------------------------------------------------ */

static void handle_overworld_input(GameState *gs, int key) {
    Direction dir = key_to_direction(key);
    if (dir != DIR_NONE) {
        /* Drunkenness: chance of random direction */
        if (gs->drunk_turns > 0) {
            int stumble_chance = 0;
            if (gs->beers_drunk >= 4) stumble_chance = 40;
            else if (gs->beers_drunk >= 3) stumble_chance = 25;
            else if (gs->beers_drunk >= 2) stumble_chance = 10;
            if (rng_chance(stumble_chance)) {
                dir = rng_range(0, 7);
                log_add(&gs->log, gs->turn, CP_YELLOW, "You stumble drunkenly...");
            }
            gs->drunk_turns--;
            if (gs->drunk_turns <= 0) {
                gs->beers_drunk = 0;
                log_add(&gs->log, gs->turn, CP_GREEN, "You sober up. Your head clears.");
            }
        }

        int nx = gs->player_pos.x + dir_dx[dir];
        int ny = gs->player_pos.y + dir_dy[dir];

        /* Bounds check */
        if (nx < 0 || nx >= OW_WIDTH || ny < 0 || ny >= OW_HEIGHT) return;

        TileType tt = gs->overworld->map[ny][nx].type;
        char target_glyph = gs->overworld->map[ny][nx].glyph;
        bool passable = gs->overworld->map[ny][nx].passable;

        /* In a boat: water and lake tiles are passable */
        if (gs->in_boat) {
            if (tt == TILE_WATER || tt == TILE_LAKE) {
                passable = true;

                /* Lake encounters while sailing */
                int encounter_roll = rng_range(1, 100);
                if (encounter_roll <= 3) {
                    /* Water monster attack! */
                    const char *wnames[] = { "Water Serpent", "Kelpie", "Giant Pike", "Nixie" };
                    int whp[]  = { 14, 18, 10, 8 };
                    int wstr[] = { 6, 8, 5, 4 };
                    int wdef[] = { 2, 3, 2, 1 };
                    int wxp[]  = { 14, 20, 10, 8 };
                    (void)0; /* glyphs: S k f n */
                    int wi = rng_range(0, 3);

                    log_add(&gs->log, gs->turn, CP_RED_BOLD,
                             "A %s rises from the water and attacks your boat!", wnames[wi]);

                    /* Quick combat: player attacks first */
                    int phit = 70 + gs->str - wdef[wi] * 2;
                    if (phit > 95) phit = 95;
                    if (rng_chance(phit)) {
                        int wpow = (gs->equipment[SLOT_WEAPON].template_id >= 0) ? gs->equipment[SLOT_WEAPON].power : 0;
                        int pdmg = gs->str + wpow + rng_range(-2, 2) - wdef[wi];
                        if (pdmg < 1) pdmg = 1;
                        log_add(&gs->log, gs->turn, CP_WHITE,
                                 "You strike the %s for %d damage!", wnames[wi], pdmg);
                        if (pdmg >= whp[wi]) {
                            gs->xp += wxp[wi];
                            gs->kills++;
                            log_add(&gs->log, gs->turn, CP_YELLOW,
                                     "The %s sinks beneath the waves! +%d XP", wnames[wi], wxp[wi]);
                        } else {
                            /* Monster strikes back */
                            int edmg = wstr[wi] + rng_range(-1, 2) - gs->def;
                            if (edmg < 1) edmg = 1;
                            gs->hp -= edmg;
                            if (gs->hp < 1) gs->hp = 1;
                            log_add(&gs->log, gs->turn, CP_RED,
                                     "The %s thrashes and hits you for %d!", wnames[wi], edmg);
                        }
                    } else {
                        int edmg = wstr[wi] + rng_range(-1, 2) - gs->def;
                        if (edmg < 1) edmg = 1;
                        gs->hp -= edmg;
                        if (gs->hp < 1) gs->hp = 1;
                        log_add(&gs->log, gs->turn, CP_RED,
                                 "You miss! The %s bites you for %d damage!", wnames[wi], edmg);
                    }
                } else if (encounter_roll == 4) {
                    /* Fishing bonus! */
                    log_add(&gs->log, gs->turn, CP_CYAN,
                             "You trail a line behind the boat and catch a fish!");
                    if (gs->num_items < MAX_INVENTORY) {
                        int tcount; const ItemTemplate *tmps = item_get_templates(&tcount);
                        for (int ti = 0; ti < tcount; ti++) {
                            if (strcmp(tmps[ti].name, "Fresh Fish") == 0 ||
                                strcmp(tmps[ti].name, "Salted Fish") == 0) {
                                gs->inventory[gs->num_items] = item_create(ti, -1, -1);
                                gs->inventory[gs->num_items].on_ground = false;
                                gs->inventory[gs->num_items].created_day = gs->day;
                                gs->num_items++;
                                log_add(&gs->log, gs->turn, CP_GREEN, "Got: %s", tmps[ti].name);
                                break;
                            }
                        }
                    }
                } else if (encounter_roll == 5) {
                    /* Ghost ship! (very rare) */
                    log_add(&gs->log, gs->turn, CP_CYAN_BOLD,
                             "A spectral ship emerges from the mist!");
                    log_add(&gs->log, gs->turn, CP_CYAN,
                             "Ghostly sailors reach for you... You fight them off!");

                    /* Quick ghost ship combat */
                    int ghost_dmg = rng_range(5, 12) - gs->def;
                    if (ghost_dmg < 1) ghost_dmg = 1;
                    gs->hp -= ghost_dmg;
                    if (gs->hp < 1) gs->hp = 1;
                    log_add(&gs->log, gs->turn, CP_RED,
                             "The drowned knights wound you for %d damage!", ghost_dmg);

                    /* Loot from the ghost ship */
                    int ghost_gold = rng_range(50, 200);
                    gs->gold += ghost_gold;
                    gs->xp += 50;
                    log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                             "The ghost ship fades. You plunder %d gold from its hold! +50 XP", ghost_gold);
                } else if (encounter_roll <= 8) {
                    /* Atmospheric flavour */
                    const char *flavour[] = {
                        "The water laps gently against your boat.",
                        "You hear a distant splashing beneath the surface.",
                        "A cold mist rolls across the lake.",
                        "Fish dart away beneath your hull.",
                        "An eerie silence falls over the water.",
                        "You see something glinting deep below the surface...",
                    };
                    int nf = sizeof(flavour) / sizeof(flavour[0]);
                    log_add(&gs->log, gs->turn, CP_CYAN, "%s", flavour[rng_range(0, nf - 1)]);
                }
            } else if (passable) {
                /* Stepping from water onto land -- leave the boat at the water behind us */
                int ox = gs->player_pos.x, oy = gs->player_pos.y;
                Tile *boat_tile = &gs->overworld->map[oy][ox];
                boat_tile->glyph = 'B';
                boat_tile->color_pair = CP_BROWN;
                boat_tile->passable = true;  /* can board it again later */
                gs->in_boat = false;
                log_add(&gs->log, gs->turn, CP_CYAN,
                         "You reach the shore and step off the boat, leaving it behind.");
            }
        }

        /* Stepping onto a boat tile from land -- board the boat and clear the tile */
        if (!gs->in_boat && target_glyph == 'B' && passable) {
            gs->in_boat = true;
            if (gs->riding) { gs->riding = false; }
            /* Remove the boat from this tile -- we're taking it with us */
            Tile *bt = &gs->overworld->map[ny][nx];
            if (bt->type == TILE_LAKE || bt->type == TILE_WATER) {
                /* Boat was on water (left by previous disembark) -- restore water */
                bt->glyph = (bt->type == TILE_LAKE) ? 'o' : '~';
                bt->color_pair = CP_BLUE;
                bt->passable = false;
            } else {
                /* Boat was on a shore land tile -- restore the underlying terrain */
                /* Re-derive what the land tile should look like */
                bt->glyph = '"';
                bt->color_pair = CP_GREEN;
                bt->passable = true;
            }
            log_add(&gs->log, gs->turn, CP_CYAN,
                     "You push a small boat into the water and climb aboard.");
        }

        /* Check for creature bump before passability */
        OWCreature *creature = overworld_creature_at(gs->overworld, nx, ny);
        if (creature && creature->hostile && passable) {
            /* Overworld combat! */
            int hit_chance = 70 + gs->str - creature->def * 2;
            if (hit_chance < 20) hit_chance = 20;
            if (hit_chance > 95) hit_chance = 95;

            if (rng_chance(hit_chance)) {
                int damage = gs->str + rng_range(-2, 2) - creature->def;
                if (damage < 1) damage = 1;
                bool crit = rng_chance(10);
                if (crit) damage *= 2;

                creature->hp -= damage;
                if (crit) {
                    log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                             "CRITICAL! You hit the %s for %d!", creature->name, damage);
                } else {
                    log_add(&gs->log, gs->turn, CP_WHITE,
                             "You hit the %s for %d damage.", creature->name, damage);
                }

                if (creature->hp <= 0) {
                    gs->xp += creature->xp_reward;
                    gs->kills++;
                    int gold = rng_range(3, 15);
                    gs->gold += gold;
                    log_add(&gs->log, gs->turn, CP_YELLOW,
                             "The %s is slain! +%d XP, +%dg", creature->name, creature->xp_reward, gold);
                    /* Remove creature by moving it off map */
                    creature->pos = (Vec2){ -1, -1 };
                    creature->hostile = false;
                }
            } else {
                log_add(&gs->log, gs->turn, CP_GRAY,
                         "You swing at the %s but miss!", creature->name);
            }

            /* Enemy counterattack if still alive */
            if (creature->hp > 0) {
                int ehit = 60 + creature->str - gs->def * 2;
                if (ehit < 10) ehit = 10;
                if (rng_chance(ehit)) {
                    int edmg = creature->str + rng_range(-2, 2) - gs->def;
                    if (edmg < 1) edmg = 1;
                    gs->hp -= edmg;
                    if (gs->hp < 1) gs->hp = 1;
                    log_add(&gs->log, gs->turn, CP_RED,
                             "The %s strikes back for %d damage!", creature->name, edmg);
                } else {
                    log_add(&gs->log, gs->turn, CP_GRAY,
                             "The %s attacks but you dodge!", creature->name);
                }
            }
            advance_time(gs, 5);
            return;
        }
        if (creature && !creature->hostile && passable) {
            switch (creature->type) {
            case OW_NPC_TRAVELLER:
                if (creature->glyph == 'K') {
                    /* Travelling king */
                    log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                             "%s rides past with his retinue.", creature->name);
                    if (gs->chivalry >= 50) {
                        log_add(&gs->log, gs->turn, CP_YELLOW,
                                 "\"Well met, brave %s! Your reputation precedes you.\"",
                                 gs->player_gender == GENDER_MALE ? "Sir" : "Lady");
                        if (rng_chance(30)) {
                            int gold = rng_range(20, 50);
                            gs->gold += gold;
                            log_add(&gs->log, gs->turn, CP_YELLOW,
                                     "He tosses you a purse. \"For your service to the realm.\" +%dg", gold);
                        }
                    } else {
                        log_add(&gs->log, gs->turn, CP_WHITE,
                                 "The king glances at you briefly and rides on.");
                    }
                } else {
                    const char *msgs[] = {
                        "A traveller nods. \"Safe travels, friend.\"",
                        "\"The road to York is long. Watch for bandits.\"",
                        "\"I've come from London. The markets are busy.\"",
                    };
                    log_add(&gs->log, gs->turn, CP_WHITE, "%s", msgs[rng_range(0, 2)]);
                }
                break;
            case OW_NPC_PILGRIM:
                {
                    const char *msgs[] = {
                        "A pilgrim bows. \"Blessings upon you, traveller.\"",
                        "\"I journey to Canterbury to pray at the shrine.\"",
                        "\"Have faith. The Lord watches over us all.\"",
                    };
                    log_add(&gs->log, gs->turn, CP_YELLOW, "%s", msgs[rng_range(0, 2)]);
                }
                break;
            case OW_NPC_MERCHANT:
                log_add(&gs->log, gs->turn, CP_GREEN,
                         "A merchant waves. \"Fine wares! Visit me in town!\"");
                break;
            case OW_NPC_PEASANT:
                log_add(&gs->log, gs->turn, CP_BROWN,
                         "A peasant tips their hat. \"Good day, m'lord.\"");
                break;
            case OW_NPC_DEER:
                if (rng_chance(10)) {
                    /* Magical deer! */
                    int *stats[] = { &gs->str, &gs->def, &gs->intel, &gs->spd };
                    const char *names[] = { "STR", "DEF", "INT", "SPD" };
                    int pick = rng_range(0, 3);
                    (*stats[pick])++;
                    log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                             "The deer glows with a golden light! A magical blessing! +1 %s!", names[pick]);
                } else {
                    log_add(&gs->log, gs->turn, CP_BROWN,
                             "A deer startles and bounds away into the trees.");
                }
                /* Deer flees from player */
                creature->pos.x += rng_range(-3, 3);
                creature->pos.y += rng_range(-3, 3);
                break;
            case OW_NPC_SHEEP:
                log_add(&gs->log, gs->turn, CP_WHITE, "Baaaa! A sheep bleats nervously.");
                break;
            case OW_NPC_RABBIT:
                log_add(&gs->log, gs->turn, CP_BROWN,
                         "A rabbit darts into its burrow.");
                creature->pos.x += rng_range(-5, 5);
                creature->pos.y += rng_range(-5, 5);
                break;
            case OW_NPC_CROW:
                log_add(&gs->log, gs->turn, CP_GRAY,
                         "Caw! A crow takes flight, circling overhead.");
                creature->pos.x += rng_range(-8, 8);
                creature->pos.y += rng_range(-4, 4);
                break;
            default: break;  /* hostile types handled above */
            case OW_NPC_DRUID:
                {
                    int druid_roll = rng_range(1, 100);
                    if (druid_roll <= 30) {
                        /* Pickpocket */
                        int stolen = rng_range(10, 30);
                        if (gs->gold >= stolen) {
                            gs->gold -= stolen;
                            log_add(&gs->log, gs->turn, CP_GREEN,
                                     "A druid's hand brushes your coin purse... -%dg!", stolen);
                        } else {
                            log_add(&gs->log, gs->turn, CP_GREEN,
                                     "A druid eyes your purse but finds nothing worth taking.");
                        }
                    } else if (druid_roll <= 50) {
                        int *stats[] = { &gs->str, &gs->def, &gs->intel, &gs->spd };
                        const char *names[] = { "STR", "DEF", "INT", "SPD" };
                        int pick = rng_range(0, 3);
                        (*stats[pick])++;
                        log_add(&gs->log, gs->turn, CP_GREEN_BOLD,
                                 "The druid places a hand on your brow. \"Be blessed.\" +1 %s!", names[pick]);
                    } else if (druid_roll <= 70) {
                        gs->mp = gs->max_mp;
                        log_add(&gs->log, gs->turn, CP_GREEN_BOLD,
                                 "The druid chants softly. Your mana is restored!");
                    } else {
                        const char *lore[] = {
                            "\"The circles hold ancient power. Step wisely.\"",
                            "\"The stones remember what men forget.\"",
                            "\"Seek the green circles -- they offer safe passage.\"",
                            "\"Beware the red glow... it burns.\"",
                            "\"Nature gives, and nature takes.\"",
                        };
                        int n = sizeof(lore) / sizeof(lore[0]);
                        log_add(&gs->log, gs->turn, CP_GREEN, "%s", lore[rng_range(0, n - 1)]);
                    }
                }
                break;
            }
            /* Swap positions -- creature moves to where player was */
            creature->pos.x = gs->player_pos.x;
            creature->pos.y = gs->player_pos.y;
        }

        if (!passable) {
            switch (tt) {
            case TILE_WATER:
                if (gs->in_boat)
                    log_add(&gs->log, gs->turn, CP_BLUE,
                             "The open sea is too dangerous. Stay close to the shore.");
                else
                    log_add(&gs->log, gs->turn, CP_BLUE,
                             "The sea stretches before you. You cannot swim that far.");
                break;
            case TILE_LAKE:
                log_add(&gs->log, gs->turn, CP_BLUE,
                         "The lake is too deep to wade. Find a boat or learn Walk on Water.");
                break;
            case TILE_RIVER:
                log_add(&gs->log, gs->turn, CP_BLUE,
                         "The river is too wide and swift to cross. Find a bridge.");
                break;
            case TILE_MOUNTAIN:
                log_add(&gs->log, gs->turn, CP_WHITE,
                         "The mountain peak is too steep to climb. Find a way around.");
                break;
            case TILE_DENSE_WOODS:
                log_add(&gs->log, gs->turn, CP_GREEN,
                         "The undergrowth is impossibly thick. You need an axe or a Ranger's skill.");
                break;
            case TILE_WALL:
                log_add(&gs->log, gs->turn, CP_GRAY,
                         "Ancient stones block your path. The ruins of Hadrian's Wall stand firm here.");
                break;
            default:
                log_add(&gs->log, gs->turn, CP_GRAY,
                         "Something blocks your path. You cannot go that way.");
                break;
            }
            return;
        }

        /* Cat trails behind player */
        if (gs->has_cat) {
            gs->cat_pos = gs->player_pos; /* cat moves to where player was */
        }
        gs->player_pos.x = nx;
        gs->player_pos.y = ny;
        TileType stepped_on = gs->overworld->map[ny][nx].type;
        int travel_mins = overworld_travel_time(gs, stepped_on);
        int old_hour = gs->hour;
        advance_time(gs, travel_mins);
        check_lunar_events(gs, old_hour);

        /* Move overworld creatures */
        overworld_move_creatures(gs->overworld, gs->player_pos);

        /* Random ambush check -- hostile creatures can spawn near player */
        {
            TileType terrain = gs->overworld->map[gs->player_pos.y][gs->player_pos.x].type;
            TimeOfDay tod = game_get_tod(gs);

            /* Base ambush chance varies by terrain and time */
            int ambush_chance = 0;
            if (terrain == TILE_FOREST)      ambush_chance = 4;
            else if (terrain == TILE_HILLS)  ambush_chance = 3;
            else if (terrain == TILE_MARSH || terrain == TILE_SWAMP) ambush_chance = 5;
            else if (terrain == TILE_ROAD)   ambush_chance = 1;
            else if (terrain == TILE_GRASS)  ambush_chance = 2;

            /* Higher at night */
            if (tod == TOD_NIGHT) ambush_chance *= 2;
            else if (tod == TOD_EVENING || tod == TOD_DAWN) ambush_chance = ambush_chance * 3 / 2;

            /* Weather increases chance */
            if (gs->weather == WEATHER_FOG || gs->weather == WEATHER_STORM)
                ambush_chance += 2;

            if (ambush_chance > 0 && rng_chance(ambush_chance)) {
                /* Spawn a hostile creature adjacent to player */
                OWCreature *slot = NULL;
                for (int ci = 0; ci < gs->overworld->num_creatures; ci++) {
                    if (gs->overworld->creatures[ci].pos.x < 0) {
                        slot = &gs->overworld->creatures[ci];
                        break;
                    }
                }
                if (!slot && gs->overworld->num_creatures < MAX_OW_CREATURES)
                    slot = &gs->overworld->creatures[gs->overworld->num_creatures++];

                if (slot) {
                    /* Pick creature type based on terrain */
                    const char *name; char glyph; int hp, str, def, xp_r;
                    if (terrain == TILE_FOREST) {
                        name = "Wolf"; glyph = 'w'; hp = 10; str = 5; def = 2; xp_r = 10;
                    } else if (terrain == TILE_HILLS) {
                        name = "Wild Boar"; glyph = 'B'; hp = 10; str = 5; def = 2; xp_r = 8;
                    } else if (terrain == TILE_MARSH || terrain == TILE_SWAMP) {
                        name = "Skeleton"; glyph = 'z'; hp = 10; str = 5; def = 3; xp_r = 10;
                    } else {
                        name = "Bandit"; glyph = 'p'; hp = 12; str = 5; def = 3; xp_r = 10;
                    }

                    /* Find adjacent tile to place the ambusher */
                    for (int tries = 0; tries < 8; tries++) {
                        int d = rng_range(0, 7);
                        int ax = gs->player_pos.x + dir_dx[d];
                        int ay = gs->player_pos.y + dir_dy[d];
                        if (ax > 0 && ax < OW_WIDTH - 1 && ay > 0 && ay < OW_HEIGHT - 1 &&
                            gs->overworld->map[ay][ax].passable &&
                            !overworld_creature_at(gs->overworld, ax, ay)) {
                            memset(slot, 0, sizeof(*slot));
                            slot->type = OW_ENEMY_BANDIT;
                            slot->pos = (Vec2){ ax, ay };
                            slot->glyph = glyph;
                            slot->color_pair = CP_RED;
                            snprintf(slot->name, MAX_NAME, "%s", name);
                            slot->hostile = true;
                            slot->hp = hp; slot->max_hp = hp;
                            slot->str = str; slot->def = def;
                            slot->xp_reward = xp_r;

                            log_add(&gs->log, gs->turn, CP_RED,
                                     "A %s ambushes you from the %s!",
                                     name, tod == TOD_NIGHT ? "darkness" : "undergrowth");
                            break;
                        }
                    }
                }
            }
        }

        /* ---- Special overworld encounters ---- */
        TileType ow_terrain = gs->overworld->map[gs->player_pos.y][gs->player_pos.x].type;

        /* Breunis sans Pitie (recurring villain, ~1 per 300 turns on roads) */
        if (gs->turn % 300 == 150 && rng_chance(20) &&
            (ow_terrain == TILE_ROAD || ow_terrain == TILE_GRASS)) {
            int b_str = 10 + gs->breunis_kills * 2;
            int b_def = 8 + gs->breunis_kills;
            int b_hp = 30 + gs->breunis_kills * 5;
            log_add(&gs->log, gs->turn, CP_RED_BOLD,
                     "Breunis sans Pitie blocks your path! \"We meet again!\"");
            /* Multi-round combat */
            for (int r = 0; r < 5 && b_hp > 0 && gs->hp > 1; r++) {
                int wpow = (gs->equipment[SLOT_WEAPON].template_id >= 0) ? gs->equipment[SLOT_WEAPON].power : 0;
                int pdmg = gs->str + wpow + rng_range(-2, 2) - b_def;
                if (pdmg < 1) pdmg = 1;
                b_hp -= pdmg;
                log_add(&gs->log, gs->turn, CP_WHITE, "You strike Breunis for %d!", pdmg);
                if (b_hp > 0) {
                    int edmg = b_str + rng_range(-1, 3) - gs->def;
                    if (edmg < 2) edmg = 2;
                    gs->hp -= edmg;
                    if (gs->hp < 1) gs->hp = 1;
                    log_add(&gs->log, gs->turn, CP_RED, "Breunis strikes back for %d!", edmg);
                }
            }
            if (b_hp <= 0) {
                gs->breunis_kills++;
                int gold = 30 + gs->breunis_kills * 10;
                gs->gold += gold;
                gs->xp += 50 + gs->breunis_kills * 20;
                gs->kills++;
                log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                         "Breunis falls! +%dg, +%d XP. He'll be back stronger...",
                         gold, 50 + gs->breunis_kills * 20);
                if (gs->breunis_kills == 1 && gs->num_items < MAX_INVENTORY) {
                    Item dark_sword;
                    memset(&dark_sword, 0, sizeof(dark_sword));
                    dark_sword.template_id = -6;
                    snprintf(dark_sword.name, sizeof(dark_sword.name), "Dark Blade of Breunis");
                    dark_sword.glyph = '|'; dark_sword.color_pair = CP_MAGENTA_BOLD;
                    dark_sword.type = ITYPE_WEAPON; dark_sword.power = 11;
                    dark_sword.weight = 50; dark_sword.value = 250;
                    dark_sword.on_ground = false;
                    gs->inventory[gs->num_items++] = dark_sword;
                    log_add(&gs->log, gs->turn, CP_MAGENTA_BOLD,
                             "He drops the Dark Blade of Breunis! (power 11)");
                }
            } else {
                log_add(&gs->log, gs->turn, CP_GRAY, "Breunis retreats. \"Next time...\"");
            }
        }

        /* Mad Knight encounter (~1 per 400 turns) */
        if (gs->turn % 400 == 200 && rng_chance(15) &&
            (ow_terrain == TILE_ROAD || ow_terrain == TILE_GRASS)) {
            bool aggressive = rng_chance(50);
            bool cursed = rng_chance(10);
            if (aggressive && !cursed) {
                log_add(&gs->log, gs->turn, CP_RED,
                         "A mad knight charges at you, screaming gibberish!");
                int edmg = rng_range(5, 12) - gs->def;
                if (edmg < 1) edmg = 1;
                gs->hp -= edmg;
                if (gs->hp < 1) gs->hp = 1;
                int gold = rng_range(5, 20);
                gs->gold += gold;
                gs->xp += 15;
                gs->kills++;
                log_add(&gs->log, gs->turn, CP_YELLOW,
                         "You defeat the mad knight. -%d HP, +%dg, +15 XP. A tragic encounter.", edmg, gold);
            } else if (cursed) {
                /* Cursed knight -- can be cured */
                bool has_holy = false;
                for (int ii = 0; ii < gs->num_items; ii++) {
                    if (strcmp(gs->inventory[ii].name, "Holy Water") == 0) {
                        has_holy = true; break;
                    }
                }
                /* Check for Purify spell */
                int scount; const SpellDef *sps = spell_get_defs(&scount);
                bool has_purify = false;
                for (int si = 0; si < gs->num_spells; si++) {
                    const SpellDef *sp = spell_get(gs->spells_known[si]);
                    if (sp && strcmp(sp->name, "Purify") == 0) { has_purify = true; break; }
                }
                (void)sps; (void)scount;
                if (has_holy || has_purify) {
                    log_add(&gs->log, gs->turn, CP_CYAN,
                             "A cursed knight stumbles toward you, eyes glazed...");
                    log_add(&gs->log, gs->turn, CP_GREEN_BOLD,
                             "You use %s to break the curse!", has_purify ? "Purify" : "Holy Water");
                    if (has_holy && !has_purify) {
                        for (int ii = 0; ii < gs->num_items; ii++) {
                            if (strcmp(gs->inventory[ii].name, "Holy Water") == 0) {
                                for (int j = ii; j < gs->num_items - 1; j++)
                                    gs->inventory[j] = gs->inventory[j + 1];
                                gs->inventory[--gs->num_items].template_id = -1;
                                break;
                            }
                        }
                    }
                    gs->chivalry += 5;
                    if (gs->chivalry > 100) gs->chivalry = 100;
                    gs->xp += 30;
                    log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                             "\"Thank you! Take my sword.\" +5 chivalry, +30 XP.");
                    if (gs->num_items < MAX_INVENTORY) {
                        int tcount; const ItemTemplate *tmps = item_get_templates(&tcount);
                        for (int ti = 0; ti < tcount; ti++) {
                            if (strcmp(tmps[ti].name, "Longsword") == 0) {
                                gs->inventory[gs->num_items] = item_create(ti, -1, -1);
                                gs->inventory[gs->num_items].on_ground = false;
                                gs->num_items++;
                                break;
                            }
                        }
                    }
                } else {
                    log_add(&gs->log, gs->turn, CP_YELLOW,
                             "A cursed knight staggers past, muttering madly...");
                    log_add(&gs->log, gs->turn, CP_GRAY,
                             "(Holy Water or Purify spell could cure him.)");
                }
            } else {
                log_add(&gs->log, gs->turn, CP_YELLOW,
                         "A mad knight wanders past, muttering to himself. He ignores you.");
                if (rng_chance(30)) {
                    int gold = rng_range(2, 8);
                    gs->gold += gold;
                    log_add(&gs->log, gs->turn, CP_YELLOW,
                             "He drops %d gold as he stumbles away.", gold);
                }
            }
        }

        /* Damsel in Distress (~1 per 350 turns) */
        if (gs->turn % 350 == 175 && rng_chance(20)) {
            const char *scenarios[] = {
                "A damsel is cornered by bandits on the road!",
                "A young woman is being chased by wolves!",
                "A noblewoman calls for help -- a monster blocks her path!"
            };
            int si = rng_range(0, 2);
            log_add(&gs->log, gs->turn, CP_YELLOW_BOLD, "%s", scenarios[si]);
            log_add(&gs->log, gs->turn, CP_WHITE, "You rush to her aid and fight off the attackers!");

            /* Quick combat */
            int edmg = rng_range(3, 8) - gs->def;
            if (edmg < 1) edmg = 1;
            gs->hp -= edmg;
            if (gs->hp < 1) gs->hp = 1;
            gs->xp += 20;
            gs->kills++;

            /* Random reward */
            int reward = rng_range(1, 4);
            if (reward == 1) {
                int gold = rng_range(20, 60);
                gs->gold += gold;
                log_add(&gs->log, gs->turn, CP_YELLOW,
                         "\"My hero!\" She rewards you with %d gold. +5 chivalry.", gold);
            } else if (reward == 2) {
                gs->hp = gs->max_hp;
                gs->mp = gs->max_mp;
                log_add(&gs->log, gs->turn, CP_GREEN,
                         "\"My hero!\" She kisses your cheek. Full HP/MP restored! +5 chivalry.");
            } else if (reward == 3) {
                gs->str += 1;
                log_add(&gs->log, gs->turn, CP_CYAN,
                         "\"Take this charm for luck.\" +1 STR, +5 chivalry.");
            } else {
                gs->intel += 1;
                log_add(&gs->log, gs->turn, CP_CYAN,
                         "\"There is treasure hidden near Glastonbury...\" +1 INT, +5 chivalry.");
            }
            gs->chivalry += 5;
            if (gs->chivalry > 100) gs->chivalry = 100;
        }

        /* Witch encounter & Geas (~1 per 500 turns, higher in swamps/forests) */
        if (gs->witch_geas_turns == 0 && gs->turn % 500 == 250 &&
            (ow_terrain == TILE_SWAMP || ow_terrain == TILE_FOREST || ow_terrain == TILE_MARSH) &&
            rng_chance(25)) {
            const char *tasks[] = {
                "Bring me a gem from a dungeon",
                "Slay a wolf for me",
                "Stand in a stone circle at midnight",
                "Bring me a potion",
                "Fetch me a mushroom from the forest",
                "Deliver an offering to Canterbury"
            };
            int task = rng_range(0, 5);
            int turns = 60 + task * 15;
            gs->witch_geas_type = task;
            gs->witch_geas_turns = turns;
            log_add(&gs->log, gs->turn, CP_MAGENTA_BOLD,
                     "A witch appears from the shadows! \"You owe me a task, mortal!\"");
            log_add(&gs->log, gs->turn, CP_MAGENTA,
                     "Geas: %s (%d turns)", tasks[task], turns);
            log_add(&gs->log, gs->turn, CP_RED,
                     "Complete the task or be cursed! The witch vanishes...");
            gs->dark_affinity += 1;
        }

        /* Wood Nymph encounter (~1 per 250 turns in forests) */
        if (ow_terrain == TILE_FOREST && gs->turn % 250 == 125 && rng_chance(15) &&
            gs->num_items > 0) {
            /* 10% chance she gives a gift instead of stealing */
            if (rng_chance(10)) {
                log_add(&gs->log, gs->turn, CP_GREEN_BOLD,
                         "A wood nymph appears and blows you a kiss!");
                gs->spd += 2;
                gs->buff_spd_turns = 100;
                log_add(&gs->log, gs->turn, CP_GREEN,
                         "Nymph's Kiss! +2 SPD for 100 turns.");
                gs->nature_affinity += 1;
            } else {
                /* SPD check to resist theft */
                bool resisted = (gs->spd > 6 && rng_chance(30));
                if (resisted) {
                    log_add(&gs->log, gs->turn, CP_GREEN,
                             "A wood nymph reaches for your pack but you dodge her!");
                    log_add(&gs->log, gs->turn, CP_GRAY,
                             "She giggles and vanishes into the trees.");
                } else {
                    /* Steal a random item */
                    int steal_idx = rng_range(0, gs->num_items - 1);
                    log_add(&gs->log, gs->turn, CP_GREEN_BOLD,
                             "A wood nymph snatches your %s and vanishes!",
                             gs->inventory[steal_idx].name);
                    for (int j = steal_idx; j < gs->num_items - 1; j++)
                        gs->inventory[j] = gs->inventory[j + 1];
                    gs->inventory[--gs->num_items].template_id = -1;
                    log_add(&gs->log, gs->turn, CP_GRAY,
                             "Her laughter echoes through the trees...");
                }
            }
        }

        /* Rainbow countdown and pot of gold check */
        if (gs->rainbow_active) {
            gs->rainbow_turns--;
            if (gs->player_pos.x == gs->rainbow_end.x &&
                gs->player_pos.y == gs->rainbow_end.y) {
                /* Found the pot of gold! */
                int gold = rng_range(100, 500);
                gs->gold += gold;
                gs->rainbow_active = false;
                log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                         "You found the end of the rainbow! A pot of gold! +%d gold!", gold);
                log_add(&gs->log, gs->turn, CP_GREEN,
                         "A leprechaun dances away laughing into the mist.");
            } else if (gs->rainbow_turns <= 0) {
                gs->rainbow_active = false;
                log_add(&gs->log, gs->turn, CP_GRAY,
                         "The rainbow shimmers and vanishes... the gold is gone.");
            }
        }

        /* Regional weather shift -- chance of weather changing as you travel */
        if (gs->turn % 20 == 0 && rng_chance(30)) {
            change_weather(gs);
        }

        /* Check for a location at the new position */
        Location *loc = overworld_location_at(gs->overworld, nx, ny);
        if (loc) {
            if (!loc->discovered) {
                loc->discovered = true;
                log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                         "You discover %s!", loc->name);
            }

            switch (loc->type) {
            case LOC_TOWN:
            case LOC_CASTLE_ACTIVE: {
                const TownDef *td = town_get_def(loc->name);
                if (td) {
                    log_add(&gs->log, gs->turn, CP_WHITE,
                             "You enter %s.", loc->name);
                } else {
                    log_add(&gs->log, gs->turn, CP_WHITE,
                             "You arrive at %s. Press Enter to enter.", loc->name);
                }
                break;
            }
            case LOC_CASTLE_ABANDONED:
                log_add(&gs->log, gs->turn, CP_GRAY,
                         "The ruins of %s loom before you. Press > to explore.", loc->name);
                break;
            case LOC_DUNGEON_ENTRANCE:
                log_add(&gs->log, gs->turn, CP_WHITE,
                         "A dungeon entrance: %s. Press > to enter.", loc->name);
                break;
            case LOC_LANDMARK:
                if (strcmp(loc->name, "Stonehenge") == 0) {
                    if (!gs->stonehenge_used) {
                        log_add(&gs->log, gs->turn, CP_YELLOW,
                                 "The ancient stones hum with power. Press Enter to commune.");
                    } else {
                        log_add(&gs->log, gs->turn, CP_YELLOW,
                                 "Stonehenge. The stones are quiet now.");
                    }
                } else if (strcmp(loc->name, "Faerie Ring") == 0) {
                    log_add(&gs->log, gs->turn, CP_GREEN_BOLD,
                             "A faerie ring glows softly! Press Enter to step inside.");
                } else if (strcmp(loc->name, "White Cliffs") == 0) {
                    log_add(&gs->log, gs->turn, CP_WHITE_BOLD,
                             "The White Cliffs of Dover. The sea crashes far below.");
                } else if (strcmp(loc->name, "Hadrian's Wall") == 0) {
                    log_add(&gs->log, gs->turn, CP_WHITE,
                             "Hadrian's Wall stretches east and west. Roman ruins crumble.");
                } else if (strcmp(loc->name, "Holy Island") == 0) {
                    log_add(&gs->log, gs->turn, CP_WHITE_BOLD,
                             "Holy Island. A place of peace and ancient prayer.");
                } else if (strcmp(loc->name, "Avalon") == 0) {
                    log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                             "The mystical Isle of Avalon! Press Enter to explore.");
                } else {
                    log_add(&gs->log, gs->turn, CP_YELLOW,
                             "You stand at %s.", loc->name);
                }
                break;
            case LOC_VOLCANO:
                log_add(&gs->log, gs->turn, CP_RED,
                         "Mount Draig! Smoke rises from the volcano.");
                break;
            case LOC_COTTAGE:
                log_add(&gs->log, gs->turn, CP_BROWN,
                         "A small cottage sits by the path. Press Enter to go inside.");
                break;
            case LOC_CAVE:
                log_add(&gs->log, gs->turn, CP_GRAY,
                         "A dark cave entrance in the hillside. Press Enter to explore.");
                break;
            case LOC_MAGIC_CIRCLE:
                log_add(&gs->log, gs->turn, CP_CYAN_BOLD,
                         "A magic circle glows on the ground! Press Enter to step inside.");
                break;
            case LOC_ABBEY:
                log_add(&gs->log, gs->turn, CP_WHITE,
                         "You arrive at %s. Press Enter to enter.", loc->name);
                break;
            default:
                /* Check for player home by name */
                if (loc && strcmp(loc->name, "Your Home") == 0) {
                    log_add(&gs->log, gs->turn, CP_WHITE_BOLD,
                             "Home sweet home. Your storage chest awaits.");
                }
                break;
            }
        }
        return;
    }

    /* Enter town */
    if (key == '\n' || key == '\r' || key == KEY_ENTER) {
        Location *loc = overworld_location_at(gs->overworld,
                                               gs->player_pos.x, gs->player_pos.y);
        if (loc && (loc->type == LOC_TOWN || loc->type == LOC_CASTLE_ACTIVE || loc->type == LOC_ABBEY)) {
            /* Castles are locked at night */
            if (loc->type == LOC_CASTLE_ACTIVE && is_night(gs->hour)) {
                if (gs->chivalry >= 60) {
                    log_add(&gs->log, gs->turn, CP_WHITE,
                             "The guards recognise your honour and open the gates.");
                } else {
                    log_add(&gs->log, gs->turn, CP_GRAY,
                             "The castle gates are barred for the night. Return at dawn.");
                    return;
                }
            }
            const TownDef *td = town_get_def(loc->name);
            if (td) {
                gs->current_town = td;
                gs->mode = MODE_TOWN;
                gs->well_explored = false;
                gs->confessed = false;
                gs->beers_drunk = 0;
                if (gs->riding) {
                    gs->riding = false;
                    log_add(&gs->log, gs->turn, CP_BROWN, "You dismount and tie your horse outside.");
                }
                /* Announce tournament if this is a castle and player has a horse */
                bool is_a_castle = (strncmp(td->name, "Castle", 6) == 0 ||
                                    strcmp(td->name, "Camelot Castle") == 0 ||
                                    strcmp(td->name, "Edinburgh Castle") == 0 ||
                                    strcmp(td->name, "Dover Castle") == 0);
                if (is_a_castle && gs->horse_type > 0) {
                    /* Check if tournament today */
                    unsigned int ch2 = 0;
                    for (const char *p = td->name; *p; p++)
                        ch2 = ch2 * 31 + (unsigned char)*p;
                    int moon = game_get_moon_day(gs);
                    int tdays[4] = {
                        (int)(ch2 % 7) + 1, (int)((ch2 / 7) % 7) + 8,
                        (int)((ch2 / 49) % 7) + 15, (int)((ch2 / 343) % 7) + 22
                    };
                    bool tourn_today = false;
                    for (int d = 0; d < 4; d++)
                        if (moon == tdays[d]) { tourn_today = true; break; }
                    if (tourn_today) {
                        log_add(&gs->log, gs->turn, CP_YELLOW,
                                 "A jousting tournament today! Bump into a guard to enter.");
                    }
                }
                bool has_quest = (quest_find_available(&gs->quests, td->name, gs->chivalry) != NULL);
                town_generate_map(&gs->town_map, td, has_quest);
                gs->town_player_pos = gs->town_map.entrance;
                log_add(&gs->log, gs->turn, CP_WHITE,
                         "You enter %s. Bump into people to interact.", loc->name);

                /* Check if a delivery quest completes here */
                Quest *dq = quest_check_delivery(&gs->quests, td->name);
                if (dq) {
                    dq->state = QUEST_COMPLETE;
                    log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                             "Quest complete: %s! Return to %s for your reward.",
                             dq->name, dq->giver_town);
                }

                /* Canterbury Graveyard */
                if (strcmp(td->name, "Canterbury") == 0) {
                    FallenHero heroes[MAX_FALLEN];
                    int hcount = fallen_load(heroes);
                    if (hcount > 0) {
                        log_add(&gs->log, gs->turn, CP_GRAY,
                                 "The graveyard holds %d fallen hero%s. Press 'g' inside to visit.",
                                 hcount, hcount == 1 ? "" : "es");
                    }
                }

                /* Player Home in Camelot */
                if (strcmp(td->name, "Camelot") == 0) {
                    log_add(&gs->log, gs->turn, CP_WHITE,
                             "Your home is here. Press 'V' inside to visit your storage chest.");
                }
            } else {
                log_add(&gs->log, gs->turn, CP_GRAY,
                         "%s has no services available.", loc->name);
            }
        } else if (loc && loc->type == LOC_COTTAGE) {
            /* Reset after 5-15 days */
            if (loc->visited && (gs->day - loc->visited_day) >= rng_range(5, 15)) {
                loc->visited = false;
            }
            if (loc->visited) {
                log_add(&gs->log, gs->turn, CP_GRAY,
                         "The cottage is abandoned. Nothing remains here.");
                return;
            }
            loc->visited = true;
            loc->visited_day = gs->day;

            /* Draw cottage interior and random encounter */
            ui_clear();
            int term_rows, term_cols;
            ui_get_size(&term_rows, &term_cols);

            #define COT_W 12
            #define COT_H 8
            int ox = (term_cols - COT_W) / 2;
            int oy = 2;

            /* Draw cottage: walls, floor, door, fireplace, table */
            for (int cy = 0; cy < COT_H; cy++) {
                for (int cx = 0; cx < COT_W; cx++) {
                    int sx = ox + cx, sy = oy + cy;
                    if (cy == 0 || cy == COT_H - 1 || cx == 0 || cx == COT_W - 1) {
                        attron(COLOR_PAIR(CP_BROWN));
                        mvaddch(sy, sx, '#');
                        attroff(COLOR_PAIR(CP_BROWN));
                    } else {
                        attron(COLOR_PAIR(CP_YELLOW));
                        mvaddch(sy, sx, '.');
                        attroff(COLOR_PAIR(CP_YELLOW));
                    }
                }
            }
            /* Door at bottom center */
            attron(COLOR_PAIR(CP_BROWN));
            mvaddch(oy + COT_H - 1, ox + COT_W / 2, '/');
            attroff(COLOR_PAIR(CP_BROWN));
            /* Fireplace */
            attron(COLOR_PAIR(CP_RED) | A_BOLD);
            mvaddch(oy + 1, ox + 1, '^');
            attroff(COLOR_PAIR(CP_RED) | A_BOLD);
            /* Table */
            attron(COLOR_PAIR(CP_BROWN));
            mvaddch(oy + 3, ox + COT_W / 2, '=');
            attroff(COLOR_PAIR(CP_BROWN));
            /* Player at door */
            attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
            mvaddch(oy + COT_H - 2, ox + COT_W / 2, '@');
            attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);

            /* Title */
            attron(COLOR_PAIR(CP_BROWN) | A_BOLD);
            mvprintw(oy - 1, ox, "A Small Cottage");
            attroff(COLOR_PAIR(CP_BROWN) | A_BOLD);

            /* Random state */
            int cot_roll = rng_range(1, 100);
            int text_y = oy + COT_H + 1;

            attron(COLOR_PAIR(CP_WHITE));
            mvprintw(text_y++, ox - 10, "You push open the creaky door and step inside...");
            text_y++;

            if (cot_roll <= 30) {
                /* Empty */
                mvprintw(text_y++, ox - 10, "The cottage is abandoned. Dust covers everything.");
                mvprintw(text_y++, ox - 10, "A cold fireplace and an empty table. You rest briefly.");
                gs->hp += gs->max_hp / 4;
                if (gs->hp > gs->max_hp) gs->hp = gs->max_hp;
                gs->mp += gs->max_mp / 4;
                if (gs->mp > gs->max_mp) gs->mp = gs->max_mp;
                attron(COLOR_PAIR(CP_GREEN));
                mvprintw(text_y++, ox - 10, "You rest and recover slightly. (+25%% HP/MP)");
                attroff(COLOR_PAIR(CP_GREEN));
            } else if (cot_roll <= 65) {
                /* Friendly occupant */
                /* Draw occupant */
                attron(COLOR_PAIR(CP_WHITE) | A_BOLD);
                mvaddch(oy + 2, ox + COT_W / 2 + 2, '@');
                attroff(COLOR_PAIR(CP_WHITE) | A_BOLD);

                int who = rng_range(1, 4);
                if (who == 1) {
                    mvprintw(text_y++, ox - 10, "A hermit sits by the fire. He looks up and smiles.");
                    mvprintw(text_y++, ox - 10, "\"Sit, traveller. I know these lands well.\"");
                    attron(COLOR_PAIR(CP_YELLOW));
                    mvprintw(text_y++, ox - 10, "He shares rumours about a nearby dungeon.");
                    attroff(COLOR_PAIR(CP_YELLOW));
                } else if (who == 2) {
                    mvprintw(text_y++, ox - 10, "A healer tends herbs by the window.");
                    mvprintw(text_y++, ox - 10, "\"You look weary. Let me help.\"");
                    gs->hp = gs->max_hp;
                    attron(COLOR_PAIR(CP_GREEN));
                    mvprintw(text_y++, ox - 10, "She heals your wounds. HP fully restored!");
                    attroff(COLOR_PAIR(CP_GREEN));
                } else if (who == 3) {
                    mvprintw(text_y++, ox - 10, "A retired knight sits polishing a old sword.");
                    mvprintw(text_y++, ox - 10, "\"Let me tell you about fighting, lad...\"");
                    gs->str++;
                    attron(COLOR_PAIR(CP_GREEN));
                    mvprintw(text_y++, ox - 10, "His tales inspire you. +1 STR!");
                    attroff(COLOR_PAIR(CP_GREEN));
                } else {
                    mvprintw(text_y++, ox - 10, "A peasant offers you bread and water.");
                    mvprintw(text_y++, ox - 10, "\"Not much, but you're welcome to it.\"");
                    gs->hp += gs->max_hp / 2;
                    if (gs->hp > gs->max_hp) gs->hp = gs->max_hp;
                    attron(COLOR_PAIR(CP_GREEN));
                    mvprintw(text_y++, ox - 10, "The food restores you. (+50%% HP)");
                    attroff(COLOR_PAIR(CP_GREEN));
                }
                gs->chivalry += 2;
                if (gs->chivalry > 100) gs->chivalry = 100;
                attron(COLOR_PAIR(CP_YELLOW));
                mvprintw(text_y++, ox - 10, "You are grateful for the hospitality. (+2 chivalry)");
                attroff(COLOR_PAIR(CP_YELLOW));
            } else if (cot_roll <= 85) {
                /* Hostile -- bandits (placeholder without combat) */
                attron(COLOR_PAIR(CP_RED) | A_BOLD);
                mvaddch(oy + 2, ox + 3, 'b');
                mvaddch(oy + 3, ox + 8, 'b');
                attroff(COLOR_PAIR(CP_RED) | A_BOLD);

                mvprintw(text_y++, ox - 10, "Bandits! Two rough men lunge at you!");
                int damage = rng_range(3, 8);
                gs->hp -= damage;
                if (gs->hp < 1) gs->hp = 1;
                attron(COLOR_PAIR(CP_RED));
                mvprintw(text_y++, ox - 10, "You fight them off but take %d damage.", damage);
                attroff(COLOR_PAIR(CP_RED));
                int loot = rng_range(8, 25);
                gs->gold += loot;
                attron(COLOR_PAIR(CP_YELLOW));
                mvprintw(text_y++, ox - 10, "You find %d gold in their stash.", loot);
                attroff(COLOR_PAIR(CP_YELLOW));
            } else {
                /* Special -- alchemist lab */
                attron(COLOR_PAIR(CP_MAGENTA));
                mvaddch(oy + 2, ox + 3, '!');
                mvaddch(oy + 2, ox + 5, '!');
                mvaddch(oy + 2, ox + 7, '!');
                attroff(COLOR_PAIR(CP_MAGENTA));

                mvprintw(text_y++, ox - 10, "An abandoned alchemist's laboratory!");
                mvprintw(text_y++, ox - 10, "Bubbling potions line the shelves.");
                int gold = rng_range(10, 30);
                gs->gold += gold;
                gs->mp += 10;
                if (gs->mp > gs->max_mp) gs->mp = gs->max_mp;
                attron(COLOR_PAIR(CP_MAGENTA));
                mvprintw(text_y++, ox - 10, "You find %d gold and drink a mana potion. (+10 MP)", gold);
                attroff(COLOR_PAIR(CP_MAGENTA));
            }

            attroff(COLOR_PAIR(CP_WHITE));
            text_y++;
            mvprintw(text_y, ox - 10, "Press any key to leave the cottage...");
            ui_refresh();
            ui_getkey();

            log_add(&gs->log, gs->turn, CP_BROWN, "You leave the cottage.");

        } else if (loc && loc->type == LOC_MAGIC_CIRCLE) {
            /* Renew power after 10-30 days */
            if (loc->visited && (gs->day - loc->visited_day) >= 10 + (loc->id % 21)) {
                loc->visited = false;
            }
            if (loc->visited) {
                int days_left = 10 + (loc->id % 21) - (gs->day - loc->visited_day);
                if (days_left < 1) days_left = 1;
                log_add(&gs->log, gs->turn, CP_GRAY,
                         "The magic circle has faded. It may renew in %d days.", days_left);
            } else {
                /* Offer choice: use the circle or loot it */
                log_add(&gs->log, gs->turn, CP_CYAN,
                         "The circle glows with power. [Enter] to use, [l] to loot (-15 chivalry).");
                game_render(gs);
                int ck = ui_getkey();
                if (ck == 'l') {
                    /* Loot the shrine */
                    int shrine_gold = rng_range(80, 200);
                    gs->gold += shrine_gold;
                    gs->chivalry -= 15;
                    if (gs->chivalry < 0) gs->chivalry = 0;
                    loc->visited = true;
                    loc->visited_day = gs->day;
                    log_add(&gs->log, gs->turn, CP_RED,
                             "You desecrate the sacred circle and plunder %d gold! SACRILEGE!", shrine_gold);
                    log_add(&gs->log, gs->turn, CP_RED,
                             "The spirits howl in anguish. (-15 chivalry)");
                    /* Chance of curse */
                    if (rng_chance(40)) {
                        int *stats[] = { &gs->str, &gs->def, &gs->intel, &gs->spd };
                        const char *snames[] = { "STR", "DEF", "INT", "SPD" };
                        int si = rng_range(0, 3);
                        if (*stats[si] > 1) (*stats[si])--;
                        log_add(&gs->log, gs->turn, CP_MAGENTA,
                                 "A curse falls upon you! -1 %s!", snames[si]);
                    }
                    return;
                }
                loc->visited = true;
                loc->visited_day = gs->day;
                int circle_roll = rng_range(1, 100);

                if (circle_roll <= 25) {
                    gs->hp = gs->max_hp;
                    gs->mp = gs->max_mp;
                    log_add(&gs->log, gs->turn, CP_WHITE_BOLD,
                             "The circle flares white! HP and MP fully restored!");
                } else if (circle_roll <= 45) {
                    int *stats[] = { &gs->str, &gs->def, &gs->intel, &gs->spd };
                    const char *names[] = { "STR", "DEF", "INT", "SPD" };
                    int pick = rng_range(0, 3);
                    (*stats[pick])++;
                    log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                             "Ancient power flows through you! +1 %s!", names[pick]);
                } else if (circle_roll <= 60) {
                    int gold = rng_range(30, 100);
                    gs->gold += gold;
                    log_add(&gs->log, gs->turn, CP_YELLOW,
                             "The circle shimmers and gold appears! +%d gold!", gold);
                } else if (circle_roll <= 75) {
                    /* Teleport to another magic circle */
                    for (int i = 0; i < gs->overworld->num_locations; i++) {
                        Location *other = &gs->overworld->locations[i];
                        if (other->type == LOC_MAGIC_CIRCLE && other != loc) {
                            gs->player_pos = other->pos;
                            log_add(&gs->log, gs->turn, CP_GREEN_BOLD,
                                     "The circle pulses green! You are teleported to another circle!");
                            break;
                        }
                    }
                } else if (circle_roll <= 88) {
                    int dmg = rng_range(5, 12);
                    gs->hp -= dmg;
                    if (gs->hp < 1) gs->hp = 1;
                    log_add(&gs->log, gs->turn, CP_RED,
                             "The circle erupts in flame! -%d HP!", dmg);
                } else {
                    gs->chivalry += 3;
                    if (gs->chivalry > 100) gs->chivalry = 100;
                    log_add(&gs->log, gs->turn, CP_GREEN_BOLD,
                             "The circle hums with nature's blessing. +3 chivalry.");
                }
            }

        } else if (loc && loc->type == LOC_CAVE) {
            /* Reset after 7-20 days */
            if (loc->visited && (gs->day - loc->visited_day) >= rng_range(7, 20)) {
                loc->visited = false;
            }
            if (loc->visited) {
                log_add(&gs->log, gs->turn, CP_GRAY,
                         "The cave is empty. You've already explored it.");
                return;
            }
            loc->visited = true;
            loc->visited_day = gs->day;

            /* Draw cave interior */
            ui_clear();
            int term_rows2, term_cols2;
            ui_get_size(&term_rows2, &term_cols2);

            #define CAVE_W 10
            #define CAVE_H 7
            int cox = (term_cols2 - CAVE_W) / 2;
            int coy = 2;

            /* Draw cave: rough walls, dark floor */
            for (int cy = 0; cy < CAVE_H; cy++) {
                for (int cx = 0; cx < CAVE_W; cx++) {
                    int sx = cox + cx, sy = coy + cy;
                    double dist = (double)((cx - CAVE_W/2)*(cx - CAVE_W/2)) / 12.0 +
                                  (double)((cy - CAVE_H/2)*(cy - CAVE_H/2)) / 8.0;
                    if (dist > 1.0) {
                        attron(COLOR_PAIR(CP_GRAY));
                        mvaddch(sy, sx, '#');
                        attroff(COLOR_PAIR(CP_GRAY));
                    } else {
                        attron(COLOR_PAIR(CP_GRAY));
                        mvaddch(sy, sx, '.');
                        attroff(COLOR_PAIR(CP_GRAY));
                    }
                }
            }
            /* Entrance */
            attron(COLOR_PAIR(CP_BROWN));
            mvaddch(coy + CAVE_H - 1, cox + CAVE_W / 2, '/');
            attroff(COLOR_PAIR(CP_BROWN));
            /* Player */
            attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
            mvaddch(coy + CAVE_H - 2, cox + CAVE_W / 2, '@');
            attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);

            attron(COLOR_PAIR(CP_GRAY) | A_BOLD);
            mvprintw(coy - 1, cox, "A Dark Cave");
            attroff(COLOR_PAIR(CP_GRAY) | A_BOLD);

            int cave_roll = rng_range(1, 100);
            int ct_y = coy + CAVE_H + 1;

            attron(COLOR_PAIR(CP_WHITE));
            mvprintw(ct_y++, cox - 10, "You squeeze through the narrow entrance...");
            ct_y++;

            if (cave_roll <= 25) {
                mvprintw(ct_y++, cox - 10, "The cave is empty. A dry shelter, nothing more.");
                gs->hp += gs->max_hp / 4;
                if (gs->hp > gs->max_hp) gs->hp = gs->max_hp;
                attron(COLOR_PAIR(CP_GREEN));
                mvprintw(ct_y++, cox - 10, "You rest briefly. (+25%% HP)");
                attroff(COLOR_PAIR(CP_GREEN));
            } else if (cave_roll <= 55) {
                /* Monster lair */
                attron(COLOR_PAIR(CP_RED) | A_BOLD);
                mvaddch(coy + 2, cox + CAVE_W / 2, 'B');
                attroff(COLOR_PAIR(CP_RED) | A_BOLD);

                mvprintw(ct_y++, cox - 10, "A bear growls from the shadows!");
                int damage = rng_range(4, 10);
                gs->hp -= damage;
                if (gs->hp < 1) gs->hp = 1;
                attron(COLOR_PAIR(CP_RED));
                mvprintw(ct_y++, cox - 10, "The bear mauls you for %d damage!", damage);
                attroff(COLOR_PAIR(CP_RED));
                int loot = rng_range(5, 15);
                gs->gold += loot;
                attron(COLOR_PAIR(CP_YELLOW));
                mvprintw(ct_y++, cox - 10, "You drive it off and find %d gold in its lair.", loot);
                attroff(COLOR_PAIR(CP_YELLOW));
            } else if (cave_roll <= 90) {
                /* Hermit */
                attron(COLOR_PAIR(CP_BROWN) | A_BOLD);
                mvaddch(coy + 2, cox + CAVE_W / 2, 'h');
                attroff(COLOR_PAIR(CP_BROWN) | A_BOLD);

                mvprintw(ct_y++, cox - 10, "A hermit sits cross-legged, meditating.");
                mvprintw(ct_y++, cox - 10, "\"Peace, traveller. Take this blessing.\"");
                int *stats[] = { &gs->str, &gs->def, &gs->intel, &gs->spd };
                const char *names[] = { "STR", "DEF", "INT", "SPD" };
                int pick = rng_range(0, 3);
                (*stats[pick])++;
                attron(COLOR_PAIR(CP_GREEN));
                mvprintw(ct_y++, cox - 10, "The hermit blesses you. +1 %s!", names[pick]);
                attroff(COLOR_PAIR(CP_GREEN));
                gs->chivalry += 2;
                if (gs->chivalry > 100) gs->chivalry = 100;
                attron(COLOR_PAIR(CP_YELLOW));
                mvprintw(ct_y++, cox - 10, "You respect his solitude. (+2 chivalry)");
                attroff(COLOR_PAIR(CP_YELLOW));
            } else {
                /* Bandit hideout */
                attron(COLOR_PAIR(CP_RED) | A_BOLD);
                mvaddch(coy + 2, cox + 3, 'b');
                mvaddch(coy + 3, cox + 6, 'b');
                attroff(COLOR_PAIR(CP_RED) | A_BOLD);

                mvprintw(ct_y++, cox - 10, "Bandits! They spring from the darkness!");
                int damage = rng_range(2, 6);
                gs->hp -= damage;
                if (gs->hp < 1) gs->hp = 1;
                attron(COLOR_PAIR(CP_RED));
                mvprintw(ct_y++, cox - 10, "A scuffle! You take %d damage.", damage);
                attroff(COLOR_PAIR(CP_RED));
                int loot = rng_range(15, 40);
                gs->gold += loot;
                attron(COLOR_PAIR(CP_YELLOW));
                mvprintw(ct_y++, cox - 10, "You defeat them and loot %d gold!", loot);
                attroff(COLOR_PAIR(CP_YELLOW));
            }

            attroff(COLOR_PAIR(CP_WHITE));
            ct_y++;
            mvprintw(ct_y, cox - 10, "Press any key to leave the cave...");
            ui_refresh();
            ui_getkey();

            log_add(&gs->log, gs->turn, CP_GRAY, "You leave the cave.");

        } else if (loc && loc->type == LOC_LANDMARK) {
            /* Landmark interactions */
            if (strcmp(loc->name, "Stonehenge") == 0 && !gs->stonehenge_used) {
                gs->stonehenge_used = true;
                int *stats[] = { &gs->str, &gs->def, &gs->intel, &gs->spd };
                const char *names[] = { "STR", "DEF", "INT", "SPD" };
                int pick = rng_range(0, 3);
                (*stats[pick])++;
                log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                         "The ancient spirits grant you power! +1 %s", names[pick]);
                log_add(&gs->log, gs->turn, CP_YELLOW,
                         "The stones fall silent. Their gift is given.");
            } else if (strcmp(loc->name, "Stonehenge") == 0) {
                log_add(&gs->log, gs->turn, CP_GRAY,
                         "The stones are quiet. They have nothing more to give.");
            } else if (strcmp(loc->name, "Faerie Ring") == 0) {
                int roll = rng_range(1, 100);
                if (roll <= 30) {
                    /* Friendly faerie -- stat boost */
                    int *stats[] = { &gs->str, &gs->def, &gs->intel, &gs->spd };
                    const char *names[] = { "STR", "DEF", "INT", "SPD" };
                    int pick = rng_range(0, 3);
                    (*stats[pick])++;
                    log_add(&gs->log, gs->turn, CP_GREEN_BOLD,
                             "A faerie giggles and touches your brow. +1 %s!", names[pick]);
                } else if (roll <= 50) {
                    /* Restore MP */
                    gs->mp = gs->max_mp;
                    log_add(&gs->log, gs->turn, CP_GREEN_BOLD,
                             "Faerie dust swirls around you. MP fully restored!");
                } else if (roll <= 65) {
                    /* Gold gift */
                    int gold = rng_range(10, 50);
                    gs->gold += gold;
                    log_add(&gs->log, gs->turn, CP_GREEN_BOLD,
                             "A faerie drops a pouch of %d gold at your feet!", gold);
                } else if (roll <= 80) {
                    /* Trickster -- swap two stats */
                    int a = rng_range(0, 3), b = (a + rng_range(1, 3)) % 4;
                    int *stats[] = { &gs->str, &gs->def, &gs->intel, &gs->spd };
                    const char *names[] = { "STR", "DEF", "INT", "SPD" };
                    int tmp = *stats[a]; *stats[a] = *stats[b]; *stats[b] = tmp;
                    log_add(&gs->log, gs->turn, CP_MAGENTA,
                             "A trickster faerie cackles! %s and %s swapped!", names[a], names[b]);
                } else if (roll <= 90) {
                    /* Trickster -- teleport randomly */
                    int nx, ny;
                    for (int t = 0; t < 100; t++) {
                        nx = gs->player_pos.x + rng_range(-30, 30);
                        ny = gs->player_pos.y + rng_range(-30, 30);
                        if (overworld_is_passable(gs->overworld, nx, ny)) {
                            gs->player_pos.x = nx;
                            gs->player_pos.y = ny;
                            break;
                        }
                    }
                    log_add(&gs->log, gs->turn, CP_MAGENTA,
                             "A faerie snaps its fingers! The world spins... you're somewhere else!");
                } else if (!gs->faerie_queen_met && rng_chance(50)) {
                    /* Faerie Queen encounter (once per game) */
                    gs->faerie_queen_met = true;
                    ui_clear();
                    int row = 3;
                    attron(COLOR_PAIR(CP_GREEN_BOLD) | A_BOLD);
                    mvprintw(row++, 4, "The air shimmers and a radiant figure appears!");
                    mvprintw(row++, 4, "The Faerie Queen stands before you.");
                    attroff(COLOR_PAIR(CP_GREEN_BOLD) | A_BOLD);
                    row++;
                    attron(COLOR_PAIR(CP_GREEN));
                    mvprintw(row++, 4, "\"Mortal, I offer you a pact. My power for your service.\"");
                    mvprintw(row++, 4, "\"Accept, and I shall grant you a faerie blade and");
                    mvprintw(row++, 4, " the blessing of Nature. But you owe me a favour...\"");
                    attroff(COLOR_PAIR(CP_GREEN));
                    row++;
                    attron(COLOR_PAIR(CP_CYAN));
                    mvprintw(row++, 4, "Reward: +10 Nature affinity, Faerie Blade (pow 10)");
                    attroff(COLOR_PAIR(CP_CYAN));
                    row++;
                    attron(COLOR_PAIR(CP_GRAY));
                    mvprintw(row++, 4, "[a] Accept the pact    [d] Decline politely");
                    attroff(COLOR_PAIR(CP_GRAY));
                    ui_refresh();

                    int fk = ui_getkey();
                    if (fk == 'a') {
                        gs->nature_affinity += 10;
                        /* Give Faerie Blade */
                        if (gs->num_items < MAX_INVENTORY) {
                            Item fblade;
                            memset(&fblade, 0, sizeof(fblade));
                            fblade.template_id = -5;
                            snprintf(fblade.name, sizeof(fblade.name), "Faerie Blade");
                            fblade.glyph = '|'; fblade.color_pair = CP_GREEN_BOLD;
                            fblade.type = ITYPE_WEAPON; fblade.power = 10;
                            fblade.weight = 20; fblade.value = 300;
                            fblade.on_ground = false;
                            gs->inventory[gs->num_items++] = fblade;
                        }
                        /* Teach a Nature spell */
                        int scount;
                        const SpellDef *spells = spell_get_defs(&scount);
                        for (int si = 0; si < scount; si++) {
                            if (spells[si].affiliation == AFF_NATURE &&
                                spells[si].level_required <= gs->player_level) {
                                bool known = false;
                                for (int k = 0; k < gs->num_spells; k++)
                                    if (gs->spells_known[k] == si) { known = true; break; }
                                if (!known && gs->num_spells < gs->max_spells_capacity) {
                                    gs->spells_known[gs->num_spells++] = si;
                                    log_add(&gs->log, gs->turn, CP_GREEN_BOLD,
                                             "The Faerie Queen teaches you: %s!", spells[si].name);
                                    break;
                                }
                            }
                        }
                        log_add(&gs->log, gs->turn, CP_GREEN_BOLD,
                                 "You accept the pact! +10 Nature affinity, Faerie Blade received.");
                        log_add(&gs->log, gs->turn, CP_YELLOW,
                                 "\"We shall meet again, mortal. You owe me a favour...\"");
                    } else {
                        log_add(&gs->log, gs->turn, CP_GREEN,
                                 "\"As you wish. Perhaps another time.\" The Queen fades away.");
                    }
                } else {
                    /* Nothing happens */
                    log_add(&gs->log, gs->turn, CP_GREEN,
                             "The faeries watch you curiously but do nothing.");
                }
            } else if (strcmp(loc->name, "Avalon") == 0) {
                gs->hp = gs->max_hp;
                gs->mp = gs->max_mp;
                log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                         "The mystical air of Avalon restores you. HP and MP fully restored.");
            } else if (strcmp(loc->name, "Loch Ness") == 0) {
                if (gs->nessie_defeated) {
                    log_add(&gs->log, gs->turn, CP_CYAN,
                             "The loch is peaceful. Nessie is no longer a threat.");
                } else {
                    ui_clear();
                    int row = 3;
                    attron(COLOR_PAIR(CP_GREEN_BOLD) | A_BOLD);
                    mvprintw(row++, 4, "The dark waters of Loch Ness ripple...");
                    mvprintw(row++, 4, "A massive shape rises from the depths!");
                    mvprintw(row++, 4, "NESSIE, the legendary sea monster!");
                    attroff(COLOR_PAIR(CP_GREEN_BOLD) | A_BOLD);
                    row++;

                    /* Check for fish to appease */
                    int fish_idx = -1;
                    for (int ii = 0; ii < gs->num_items; ii++) {
                        if (strstr(gs->inventory[ii].name, "Fish") ||
                            strstr(gs->inventory[ii].name, "Salmon")) {
                            fish_idx = ii; break;
                        }
                    }

                    attron(COLOR_PAIR(CP_CYAN));
                    mvprintw(row++, 4, "[f] Fight Nessie (very dangerous!)");
                    if (fish_idx >= 0)
                        mvprintw(row++, 4, "[o] Offer fish to appease her");
                    mvprintw(row++, 4, "[r] Run away");
                    attroff(COLOR_PAIR(CP_CYAN));
                    ui_refresh();

                    int nk = ui_getkey();
                    if (nk == 'f') {
                        /* Fight Nessie: HP 60, STR 14, DEF 8 */
                        int nessie_hp = 60;
                        int nessie_str = 14, nessie_def = 8;
                        log_add(&gs->log, gs->turn, CP_RED_BOLD,
                                 "You charge at Nessie!");
                        /* Multi-round combat */
                        for (int round = 0; round < 6 && nessie_hp > 0 && gs->hp > 1; round++) {
                            int wpow = (gs->equipment[SLOT_WEAPON].template_id >= 0) ?
                                        gs->equipment[SLOT_WEAPON].power : 0;
                            int pdmg = gs->str + wpow + rng_range(-2, 2) - nessie_def;
                            if (pdmg < 1) pdmg = 1;
                            nessie_hp -= pdmg;
                            log_add(&gs->log, gs->turn, CP_WHITE,
                                     "You strike Nessie for %d damage! (HP: %d)", pdmg,
                                     nessie_hp > 0 ? nessie_hp : 0);

                            if (nessie_hp > 0) {
                                int edmg = nessie_str + rng_range(-2, 3) - gs->def;
                                if (edmg < 2) edmg = 2;
                                gs->hp -= edmg;
                                if (gs->hp < 1) gs->hp = 1;
                                log_add(&gs->log, gs->turn, CP_RED,
                                         "Nessie thrashes and hits you for %d!", edmg);
                            }
                        }
                        if (nessie_hp <= 0) {
                            gs->nessie_defeated = true;
                            gs->xp += 200;
                            gs->kills++;
                            log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                                     "Nessie is defeated! +200 XP!");
                            /* Drop unique scale armor */
                            if (gs->num_items < MAX_INVENTORY) {
                                Item scale;
                                memset(&scale, 0, sizeof(scale));
                                scale.template_id = -4;
                                snprintf(scale.name, sizeof(scale.name), "Nessie Scale Mail");
                                scale.glyph = '['; scale.color_pair = CP_GREEN_BOLD;
                                scale.type = ITYPE_ARMOR; scale.power = 7;
                                scale.weight = 80; scale.value = 400;
                                scale.on_ground = false;
                                gs->inventory[gs->num_items++] = scale;
                                log_add(&gs->log, gs->turn, CP_GREEN_BOLD,
                                         "You pry a scale from Nessie -- Nessie Scale Mail! (DEF +7)");
                            }
                        } else {
                            log_add(&gs->log, gs->turn, CP_YELLOW,
                                     "Nessie retreats beneath the waves. She'll be back...");
                        }
                    } else if (nk == 'o' && fish_idx >= 0) {
                        /* Appease with fish */
                        log_add(&gs->log, gs->turn, CP_CYAN,
                                 "You toss the %s into the water...", gs->inventory[fish_idx].name);
                        for (int j = fish_idx; j < gs->num_items - 1; j++)
                            gs->inventory[j] = gs->inventory[j + 1];
                        gs->inventory[--gs->num_items].template_id = -1;

                        gs->nessie_defeated = true;
                        int gold = rng_range(100, 250);
                        gs->gold += gold;
                        gs->xp += 50;
                        log_add(&gs->log, gs->turn, CP_CYAN_BOLD,
                                 "Nessie accepts your offering and dives deep...");
                        log_add(&gs->log, gs->turn, CP_YELLOW,
                                 "She surfaces with treasure from the loch bed! +%dg, +50 XP", gold);
                    } else {
                        log_add(&gs->log, gs->turn, CP_GRAY,
                                 "You back away slowly. Nessie sinks beneath the waves.");
                    }
                }
            } else if (strcmp(loc->name, "Lady of the Lake") == 0) {
                /* The Lady of the Lake offers Excalibur */
                int qc = quest_count_completed(&gs->quests);
                if (loc->visited) {
                    log_add(&gs->log, gs->turn, CP_CYAN,
                             "The lake is still. The Lady has already bestowed her gift.");
                } else if (gs->chivalry >= 40 && (qc >= 5 || gs->light_affinity >= 15)) {
                    /* Worthy -- receive Excalibur */
                    ui_clear();
                    int row = 3;
                    attron(COLOR_PAIR(CP_CYAN_BOLD) | A_BOLD);
                    mvprintw(row++, 4, "A hand rises from the still waters, holding a gleaming sword.");
                    attroff(COLOR_PAIR(CP_CYAN_BOLD) | A_BOLD);
                    row++;
                    attron(COLOR_PAIR(CP_CYAN));
                    mvprintw(row++, 4, "\"You have proven yourself worthy, %s.", gs->player_name);
                    mvprintw(row++, 4, " Take Excalibur, the sword of kings.\"");
                    attroff(COLOR_PAIR(CP_CYAN));
                    row++;
                    attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
                    mvprintw(row++, 4, "You receive Excalibur! (+2 STR while wielded)");
                    attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
                    row += 2;
                    attron(COLOR_PAIR(CP_GRAY));
                    mvprintw(row, 4, "Press any key.");
                    attroff(COLOR_PAIR(CP_GRAY));
                    ui_refresh();
                    ui_getkey();

                    /* Create Excalibur as a powerful unique sword */
                    Item excalibur;
                    memset(&excalibur, 0, sizeof(excalibur));
                    excalibur.template_id = -3;
                    snprintf(excalibur.name, sizeof(excalibur.name), "Excalibur");
                    excalibur.glyph = '|';
                    excalibur.color_pair = CP_YELLOW_BOLD;
                    excalibur.type = ITYPE_WEAPON;
                    excalibur.power = 14;
                    excalibur.weight = 30;
                    excalibur.value = 999;
                    excalibur.on_ground = false;

                    Item old = gs->equipment[SLOT_WEAPON];
                    gs->equipment[SLOT_WEAPON] = excalibur;
                    if (old.template_id >= 0 && gs->num_items < MAX_INVENTORY) {
                        gs->inventory[gs->num_items] = old;
                        gs->inventory[gs->num_items].on_ground = false;
                        gs->num_items++;
                    }
                    gs->str += 2;
                    gs->light_affinity += 10;
                    loc->visited = true;
                } else {
                    log_add(&gs->log, gs->turn, CP_CYAN,
                             "A voice from the water: \"Return when you have proven worthy...\"");
                    if (gs->chivalry < 40)
                        log_add(&gs->log, gs->turn, CP_GRAY, "(Need chivalry 40+)");
                    else
                        log_add(&gs->log, gs->turn, CP_GRAY, "(Need 5+ quests or Light affinity 15+)");
                }
            } else if (strcmp(loc->name, "Holy Island") == 0) {
                gs->chivalry += 2;
                if (gs->chivalry > 100) gs->chivalry = 100;
                gs->hp = gs->max_hp;
                log_add(&gs->log, gs->turn, CP_WHITE_BOLD,
                         "The sacred ground heals your body and spirit. +2 chivalry.");
            } else {
                log_add(&gs->log, gs->turn, CP_YELLOW,
                         "You examine %s but find nothing to interact with.", loc->name);
            }
        } else {
            log_add(&gs->log, gs->turn, CP_GRAY, "There is nothing to enter here.");
        }
        return;
    }

    /* Dig for treasure (x key) */
    if (key == 'x') {
        bool found_treasure = false;
        for (int ti = 0; ti < gs->num_treasure_maps; ti++) {
            if (gs->treasure_found[ti]) continue;
            int ddx = abs(gs->player_pos.x - gs->treasure_spots[ti].x);
            int ddy = abs(gs->player_pos.y - gs->treasure_spots[ti].y);
            if (ddx <= 1 && ddy <= 1) {
                gs->treasure_found[ti] = 1;
                found_treasure = true;

                log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                         "You dig and find buried treasure!");

                int cache_type = rng_range(1, 100);
                if (cache_type <= 10) {
                    /* Fake map / bandit ambush */
                    log_add(&gs->log, gs->turn, CP_RED,
                             "It's a trap! Bandits ambush you!");
                    int dmg = rng_range(5, 12) - gs->def;
                    if (dmg < 1) dmg = 1;
                    gs->hp -= dmg;
                    if (gs->hp < 1) gs->hp = 1;
                    int gold = rng_range(10, 25);
                    gs->gold += gold;
                    gs->xp += 15;
                    gs->kills++;
                    log_add(&gs->log, gs->turn, CP_YELLOW,
                             "You fight off the bandits. -%d HP, +%dg from their pockets.", dmg, gold);
                } else if (cache_type <= 15) {
                    /* Trapped -- undead guardian */
                    log_add(&gs->log, gs->turn, CP_RED,
                             "A Barrow-Wight rises from the earth!");
                    int dmg = rng_range(8, 15) - gs->def;
                    if (dmg < 2) dmg = 2;
                    gs->hp -= dmg;
                    if (gs->hp < 1) gs->hp = 1;
                    int gold = rng_range(100, 250);
                    gs->gold += gold;
                    gs->xp += 40;
                    gs->kills++;
                    log_add(&gs->log, gs->turn, CP_YELLOW,
                             "You defeat the guardian! -%d HP, +%dg treasure!", dmg, gold);
                } else if (cache_type <= 45) {
                    /* Small cache */
                    int gold = rng_range(50, 100);
                    gs->gold += gold;
                    log_add(&gs->log, gs->turn, CP_YELLOW,
                             "A small cache: %d gold coins!", gold);
                } else if (cache_type <= 75) {
                    /* Medium cache */
                    int gold = rng_range(150, 300);
                    gs->gold += gold;
                    log_add(&gs->log, gs->turn, CP_YELLOW,
                             "A chest of treasure! %d gold!", gold);
                    /* Also give a random item */
                    if (gs->num_items < MAX_INVENTORY) {
                        int tcount; const ItemTemplate *tmps = item_get_templates(&tcount);
                        for (int tries = 0; tries < 30; tries++) {
                            int ri = rng_range(0, tcount - 1);
                            if (tmps[ri].rarity >= 2 &&
                                (tmps[ri].type == ITYPE_WEAPON || tmps[ri].type == ITYPE_ARMOR ||
                                 tmps[ri].type == ITYPE_POTION)) {
                                gs->inventory[gs->num_items] = item_create(ri, -1, -1);
                                gs->inventory[gs->num_items].on_ground = false;
                                gs->num_items++;
                                log_add(&gs->log, gs->turn, CP_CYAN,
                                         "Also found: %s", tmps[ri].name);
                                break;
                            }
                        }
                    }
                } else {
                    /* Large cache */
                    int gold = rng_range(500, 800);
                    gs->gold += gold;
                    log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                             "A massive hoard! %d gold!", gold);
                    /* Give a rare item */
                    if (gs->num_items < MAX_INVENTORY) {
                        int tcount; const ItemTemplate *tmps = item_get_templates(&tcount);
                        for (int tries = 0; tries < 50; tries++) {
                            int ri = rng_range(0, tcount - 1);
                            if (tmps[ri].rarity <= 2 &&
                                (tmps[ri].type == ITYPE_RING || tmps[ri].type == ITYPE_AMULET ||
                                 tmps[ri].type == ITYPE_GEM || tmps[ri].type == ITYPE_SPELL_SCROLL)) {
                                gs->inventory[gs->num_items] = item_create(ri, -1, -1);
                                gs->inventory[gs->num_items].on_ground = false;
                                gs->num_items++;
                                log_add(&gs->log, gs->turn, CP_CYAN_BOLD,
                                         "Also found: %s!", tmps[ri].name);
                                break;
                            }
                        }
                    }
                }
                break;
            }
        }
        if (!found_treasure) {
            log_add(&gs->log, gs->turn, CP_GRAY,
                     "You dig but find nothing here. Check your map again.");
        }
        int old_hour = gs->hour;
        advance_time(gs, 15);
        check_lunar_events(gs, old_hour);
        return;
    }

    /* Forage for food (F = Shift+F) -- find food in forests/grassland */
    if (key == 'F') {
        TileType here = gs->overworld->map[gs->player_pos.y][gs->player_pos.x].type;
        if (here != TILE_FOREST && here != TILE_GRASS && here != TILE_HILLS) {
            log_add(&gs->log, gs->turn, CP_GRAY,
                     "Nothing to forage here. Try forests, grassland, or hills.");
        } else {
            int old_hour = gs->hour;
            advance_time(gs, 30); /* foraging takes 30 minutes */
            check_lunar_events(gs, old_hour);

            int forage_roll = rng_range(1, 100);
            if (forage_roll <= 35) {
                /* Found food! */
                if (gs->num_items < MAX_INVENTORY) {
                    int tcount; const ItemTemplate *tmps = item_get_templates(&tcount);
                    const char *forage_items[6];
                    int nforage = 0;
                    if (here == TILE_FOREST) {
                        forage_items[nforage++] = "Apple";
                        forage_items[nforage++] = "Berries";
                        forage_items[nforage++] = "Mushroom";
                    } else if (here == TILE_GRASS) {
                        forage_items[nforage++] = "Apple";
                        forage_items[nforage++] = "Berries";
                        forage_items[nforage++] = "Turnip";
                    } else {
                        forage_items[nforage++] = "Berries";
                        forage_items[nforage++] = "Mushroom";
                    }
                    const char *wanted = forage_items[rng_range(0, nforage - 1)];
                    for (int ti = 0; ti < tcount; ti++) {
                        if (strcmp(tmps[ti].name, wanted) == 0) {
                            gs->inventory[gs->num_items] = item_create(ti, -1, -1);
                            gs->inventory[gs->num_items].on_ground = false;
                            gs->num_items++;
                            log_add(&gs->log, gs->turn, CP_GREEN,
                                     "You find %s %s!",
                                     (wanted[0] == 'A' || wanted[0] == 'a') ? "an" : "some",
                                     wanted);
                            break;
                        }
                    }
                } else {
                    log_add(&gs->log, gs->turn, CP_YELLOW,
                             "You find food but your inventory is full!");
                }
            } else if (forage_roll <= 45) {
                /* Found herbs -- small healing */
                int heal = rng_range(3, 8);
                gs->hp += heal;
                if (gs->hp > gs->max_hp) gs->hp = gs->max_hp;
                log_add(&gs->log, gs->turn, CP_GREEN,
                         "You find healing herbs and chew them. +%d HP.", heal);
            } else if (forage_roll <= 50) {
                /* Found gold */
                int gold = rng_range(2, 10);
                gs->gold += gold;
                log_add(&gs->log, gs->turn, CP_YELLOW,
                         "You find %d coins scattered in the undergrowth!", gold);
            } else {
                log_add(&gs->log, gs->turn, CP_GRAY,
                         "You search but find nothing edible.");
            }
        }
        return;
    }

    /* Cook raw food (K = Shift+K) -- requires campable terrain and tinderbox/torch */
    if (key == 'K') {
        TileType here = gs->overworld->map[gs->player_pos.y][gs->player_pos.x].type;
        if (here != TILE_GRASS && here != TILE_ROAD && here != TILE_FOREST) {
            log_add(&gs->log, gs->turn, CP_GRAY, "You need open ground to make a fire.");
        } else {
            /* Check for tinderbox or torch */
            bool has_fire = gs->has_torch;
            if (!has_fire) {
                for (int ii = 0; ii < gs->num_items; ii++) {
                    if (strcmp(gs->inventory[ii].name, "Tinderbox") == 0) {
                        has_fire = true; break;
                    }
                }
            }
            if (!has_fire) {
                log_add(&gs->log, gs->turn, CP_GRAY, "You need a Torch or Tinderbox to cook.");
            } else {
                /* Find raw food in inventory */
                int raw_idx = -1;
                for (int ii = 0; ii < gs->num_items; ii++) {
                    if (strstr(gs->inventory[ii].name, "Raw ") != NULL) {
                        raw_idx = ii; break;
                    }
                }
                if (raw_idx < 0) {
                    log_add(&gs->log, gs->turn, CP_GRAY,
                             "You have no raw food to cook. Look for Raw Meat from beasts.");
                } else {
                    Item *raw = &gs->inventory[raw_idx];
                    log_add(&gs->log, gs->turn, CP_BROWN,
                             "You build a small fire and cook the %s...", raw->name);

                    /* Transform raw food into cooked version with higher power */
                    int cooked_power = raw->power * 3;
                    char cooked_name[48];
                    if (strncmp(raw->name, "Raw ", 4) == 0)
                        snprintf(cooked_name, sizeof(cooked_name), "Cooked %s", raw->name + 4);
                    else
                        snprintf(cooked_name, sizeof(cooked_name), "Cooked %s", raw->name);
                    snprintf(raw->name, sizeof(raw->name), "%s", cooked_name);
                    raw->power = cooked_power;
                    raw->value = raw->value * 2;
                    raw->color_pair = CP_BROWN;
                    raw->created_day = gs->day; /* reset freshness */

                    int old_hour = gs->hour;
                    advance_time(gs, 20); /* cooking takes 20 minutes */
                    check_lunar_events(gs, old_hour);

                    log_add(&gs->log, gs->turn, CP_GREEN,
                             "%s is ready! (power: %d)", cooked_name, cooked_power);
                }
            }
        }
        return;
    }

    /* Fishing (f key) -- fish from shore tiles adjacent to water */
    if (key == 'f' && !gs->in_boat) {
        /* Check if adjacent to water */
        bool near_water = false;
        for (int d = 0; d < 4; d++) {
            int fx = gs->player_pos.x + dir_dx[d * 2];
            int fy = gs->player_pos.y + dir_dy[d * 2];
            if (fx >= 0 && fx < OW_WIDTH && fy >= 0 && fy < OW_HEIGHT) {
                TileType ft = gs->overworld->map[fy][fx].type;
                if (ft == TILE_LAKE || ft == TILE_RIVER || ft == TILE_WATER) {
                    near_water = true;
                    break;
                }
            }
        }
        if (near_water) {
            log_add(&gs->log, gs->turn, CP_CYAN, "You cast a line into the water...");
            int old_hour = gs->hour;
            advance_time(gs, 30); /* fishing takes 30 minutes */
            check_lunar_events(gs, old_hour);

            int catch_roll = rng_range(1, 100);
            if (catch_roll <= 40) {
                /* Caught a fish! */
                if (gs->num_items < MAX_INVENTORY) {
                    int tcount; const ItemTemplate *tmps = item_get_templates(&tcount);
                    const char *fish_names[] = { "Fresh Fish", "Salted Fish", "Smoked Salmon" };
                    for (int fi = 0; fi < 3; fi++) {
                        for (int ti = 0; ti < tcount; ti++) {
                            if (strcmp(tmps[ti].name, fish_names[fi]) == 0) {
                                gs->inventory[gs->num_items] = item_create(ti, -1, -1);
                                gs->inventory[gs->num_items].on_ground = false;
                                gs->inventory[gs->num_items].created_day = gs->day;
                                gs->num_items++;
                                log_add(&gs->log, gs->turn, CP_GREEN,
                                         "You catch a %s!", fish_names[fi]);
                                goto fishing_done;
                            }
                        }
                    }
                    fishing_done: ;
                } else {
                    log_add(&gs->log, gs->turn, CP_YELLOW,
                             "You catch a fish but your inventory is full!");
                }
            } else if (catch_roll <= 55) {
                /* Caught treasure */
                int gold = rng_range(5, 25);
                gs->gold += gold;
                log_add(&gs->log, gs->turn, CP_YELLOW,
                         "You reel in an old pouch! +%d gold!", gold);
            } else {
                log_add(&gs->log, gs->turn, CP_GRAY,
                         "Nothing bites. Maybe try again later.");
            }
        } else {
            log_add(&gs->log, gs->turn, CP_GRAY,
                     "You need to be next to water to fish.");
        }
        return;
    }

    /* Mount/dismount horse */
    if (key == 'H' && gs->horse_type > 0) {
        gs->riding = !gs->riding;
        if (gs->riding) {
            log_add(&gs->log, gs->turn, CP_BROWN, "You mount your horse. Travel speed doubled!");
        } else {
            log_add(&gs->log, gs->turn, CP_BROWN, "You dismount.");
        }
        return;
    }

    /* Camp on overworld */
    if (key == 'c') {
        TileType here = gs->overworld->map[gs->player_pos.y][gs->player_pos.x].type;
        if (here == TILE_GRASS || here == TILE_ROAD || here == TILE_FOREST) {
            log_add(&gs->log, gs->turn, CP_GREEN,
                     "You make camp and rest for several hours...");
            /* Advance 8 hours */
            int old_hour = gs->hour;
            advance_time(gs, 480);
            /* Restore 50% HP/MP */
            gs->hp += gs->max_hp / 2;
            if (gs->hp > gs->max_hp) gs->hp = gs->max_hp;
            gs->mp += gs->max_mp / 2;
            if (gs->mp > gs->max_mp) gs->mp = gs->max_mp;
            log_add(&gs->log, gs->turn, CP_GREEN,
                     "You feel rested. HP and MP partially restored.");
            /* Night ambush chance -- spawn and fight an enemy */
            if (is_night(gs->hour) && rng_chance(15)) {
                TileType camp_terrain = gs->overworld->map[gs->player_pos.y][gs->player_pos.x].type;
                const char *ename; int ehp, estr, edef, exp_r;
                if (camp_terrain == TILE_FOREST) {
                    ename = "Wolf"; ehp = 10; estr = 5; edef = 2; exp_r = 10;
                } else if (camp_terrain == TILE_ROAD) {
                    ename = "Bandit"; ehp = 12; estr = 5; edef = 3; exp_r = 10;
                } else {
                    ename = "Skeleton"; ehp = 10; estr = 5; edef = 3; exp_r = 10;
                }
                log_add(&gs->log, gs->turn, CP_RED_BOLD,
                         "You are awakened by a %s attacking your camp!", ename);

                /* Enemy gets a free hit (surprise attack) */
                int ambush_dmg = estr + rng_range(-1, 2) - gs->def;
                if (ambush_dmg < 1) ambush_dmg = 1;
                gs->hp -= ambush_dmg;
                if (gs->hp < 1) gs->hp = 1;
                log_add(&gs->log, gs->turn, CP_RED,
                         "The %s strikes you while you sleep! -%d HP!", ename, ambush_dmg);

                /* Player fights back */
                int wpow = (gs->equipment[SLOT_WEAPON].template_id >= 0) ? gs->equipment[SLOT_WEAPON].power : 0;
                int pdmg = gs->str + wpow + rng_range(-2, 2) - edef;
                if (pdmg < 1) pdmg = 1;
                ehp -= pdmg;
                if (ehp <= 0) {
                    gs->xp += exp_r;
                    gs->kills++;
                    int gold = rng_range(3, 12);
                    gs->gold += gold;
                    log_add(&gs->log, gs->turn, CP_YELLOW,
                             "You grab your weapon and slay the %s! +%d XP, +%dg", ename, exp_r, gold);
                } else {
                    /* Second exchange */
                    int edmg2 = estr + rng_range(-1, 2) - gs->def;
                    if (edmg2 < 1) edmg2 = 1;
                    gs->hp -= edmg2;
                    if (gs->hp < 1) gs->hp = 1;
                    log_add(&gs->log, gs->turn, CP_RED,
                             "The %s hits you again for %d! You drive it off into the night.", ename, edmg2);
                }
            }
            check_lunar_events(gs, old_hour);
        } else {
            log_add(&gs->log, gs->turn, CP_GRAY,
                     "You cannot camp here. Find grassland, a road, or a forest.");
        }
        return;
    }

    /* Minimap */
    if (key == 'M') {
        /* Build location summary string */
        char loc_info[256];
        Location *loc = overworld_location_at(gs->overworld,
                                               gs->player_pos.x, gs->player_pos.y);
        if (loc) {
            snprintf(loc_info, sizeof(loc_info),
                     "You are at: %s  |  Day %d %02d:%02d %s",
                     loc->name, gs->day, gs->hour, gs->minute,
                     time_of_day_name(gs->hour));
        } else {
            snprintf(loc_info, sizeof(loc_info),
                     "Exploring %s  |  Day %d %02d:%02d %s",
                     terrain_name(gs->overworld->map[gs->player_pos.y][gs->player_pos.x].type),
                     gs->day, gs->hour, gs->minute,
                     time_of_day_name(gs->hour));
        }

        bool show_labels = true;
        Vec2 map_cursor = gs->player_pos; /* virtual centre for scrolling */

        while (1) {
            ui_render_minimap((Tile *)gs->overworld->map, OW_WIDTH, OW_HEIGHT,
                              map_cursor, loc_info);

            /* Calculate scaling to match ui_render_minimap proportional mode */
            int term_rows_m, term_cols_m;
            getmaxyx(stdscr, term_rows_m, term_cols_m);
            int avail_cols_m = term_cols_m - 2;
            int avail_rows_m = term_rows_m - 3;
            int scale_m = (OW_WIDTH + avail_cols_m - 1) / avail_cols_m;
            if (scale_m < 1) scale_m = 1;
            int row_scale_m = scale_m * 2;
            int view_w_m = avail_cols_m;
            int view_h_m = avail_rows_m;
            int map_view_w_m = view_w_m * scale_m;
            int map_view_h_m = view_h_m * row_scale_m;

            /* Camera (must match ui.c logic) */
            int cam_x_m = map_cursor.x - map_view_w_m / 2;
            int cam_y_m = map_cursor.y - map_view_h_m / 2;
            if (cam_x_m < 0) cam_x_m = 0;
            if (cam_y_m < 0) cam_y_m = 0;
            if (cam_x_m + map_view_w_m > OW_WIDTH) cam_x_m = OW_WIDTH - map_view_w_m;
            if (cam_y_m + map_view_h_m > OW_HEIGHT) cam_y_m = OW_HEIGHT - map_view_h_m;
            if (cam_x_m < 0) cam_x_m = 0;
            if (cam_y_m < 0) cam_y_m = 0;

            int off_x_m = 1;
            int off_y_m = 1;

            /* Draw location labels on the minimap */
            if (show_labels) {
                for (int i = 0; i < gs->overworld->num_locations; i++) {
                    Location *ll = &gs->overworld->locations[i];
                    if (ll->type != LOC_TOWN && ll->type != LOC_CASTLE_ACTIVE &&
                        ll->type != LOC_DUNGEON_ENTRANCE && ll->type != LOC_ABBEY &&
                        ll->type != LOC_VOLCANO && ll->type != LOC_CASTLE_ABANDONED)
                        continue;

                    int lx = (ll->pos.x - cam_x_m) / scale_m;
                    int ly = (ll->pos.y - cam_y_m) / row_scale_m;
                    if (lx < 0 || lx >= view_w_m || ly < 0 || ly >= view_h_m) continue;

                    attron(COLOR_PAIR(ll->color_pair) | A_BOLD);
                    mvaddch(off_y_m + ly, off_x_m + lx, ll->glyph);
                    attroff(COLOR_PAIR(ll->color_pair) | A_BOLD);

                    int label_x = off_x_m + lx + 1;
                    int max_len = off_x_m + view_w_m - label_x;
                    if (max_len > 3) {
                        int nlen = (int)strlen(ll->name);
                        if (nlen > max_len) nlen = max_len;
                        attron(COLOR_PAIR(CP_WHITE));
                        mvprintw(off_y_m + ly, label_x, "%.*s", nlen, ll->name);
                        attroff(COLOR_PAIR(CP_WHITE));
                    }
                }

                /* Redraw player on top */
                int ppx = (gs->player_pos.x - cam_x_m) / scale_m;
                int ppy = (gs->player_pos.y - cam_y_m) / row_scale_m;
                if (ppx >= 0 && ppx < view_w_m && ppy >= 0 && ppy < view_h_m) {
                    attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD | A_BLINK);
                    mvaddch(off_y_m + ppy, off_x_m + ppx, '@');
                    attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD | A_BLINK);
                }
            }

            attron(COLOR_PAIR(CP_GRAY));
            mvprintw(term_rows_m - 2, 1,
                     "[arrows] Scroll  [L] Labels  [p] Centre on player  [any] Close");
            attroff(COLOR_PAIR(CP_GRAY));
            refresh();

            int mk = getch();
            if (mk == 'L' || mk == 'l') {
                show_labels = !show_labels;
            } else if (mk == KEY_UP || mk == 'k') {
                map_cursor.y -= row_scale_m * 3;
                if (map_cursor.y < 0) map_cursor.y = 0;
            } else if (mk == KEY_DOWN || mk == 'j') {
                map_cursor.y += row_scale_m * 3;
                if (map_cursor.y >= OW_HEIGHT) map_cursor.y = OW_HEIGHT - 1;
            } else if (mk == KEY_LEFT || mk == 'h') {
                map_cursor.x -= scale_m * 5;
                if (map_cursor.x < 0) map_cursor.x = 0;
            } else if (mk == KEY_RIGHT || mk == 'l') {
                map_cursor.x += scale_m * 5;
                if (map_cursor.x >= OW_WIDTH) map_cursor.x = OW_WIDTH - 1;
            } else if (mk == 'p' || mk == 'P') {
                map_cursor = gs->player_pos; /* re-centre on player */
            } else {
                break;
            }
        }
        return;
    }

    /* Enter dungeon */
    if (key == '>') {
        Location *loc = overworld_location_at(gs->overworld,
                                               gs->player_pos.x, gs->player_pos.y);
        if (loc && (loc->type == LOC_DUNGEON_ENTRANCE || loc->type == LOC_VOLCANO ||
                    loc->type == LOC_CASTLE_ABANDONED)) {
            gs->ow_player_pos = gs->player_pos;

            /* Create the dungeon if not already entered */
            if (gs->dungeon) dungeon_free(gs->dungeon);
            int min_d, max_d;
            get_dungeon_depth(loc->name, &min_d, &max_d);
            int num_levels = rng_range(min_d, max_d);
            gs->dungeon = dungeon_create(loc->name, num_levels);

            /* Generate first level */
            DungeonLevel *dl = &gs->dungeon->levels[0];
            map_generate(dl, 0, num_levels);
            entity_spawn_monsters(dl->monsters, &dl->num_monsters,
                                   dl->tiles, 0, dl->stairs_up[0]);
            dungeon_spawn_items(dl);
            spawn_dungeon_boss(gs, dl, loc->name, 0, num_levels);
            log_add(&gs->log, gs->turn, CP_GRAY, "[%d items, %d monsters on this level]",
                     dl->num_ground_items, dl->num_monsters);
            /* Level feeling based on monster danger */
            {
                int danger = 0;
                for (int mi = 0; mi < dl->num_monsters; mi++)
                    danger += dl->monsters[mi].xp_reward;
                if (danger < 100)
                    log_add(&gs->log, gs->turn, CP_WHITE, "This seems a quiet level.");
                else if (danger < 250)
                    log_add(&gs->log, gs->turn, CP_YELLOW, "You sense danger nearby.");
                else if (danger < 500)
                    log_add(&gs->log, gs->turn, CP_RED, "You feel anxious about this level.");
                else
                    log_add(&gs->log, gs->turn, CP_RED_BOLD, "This level is DEADLY. Tread carefully!");
            }

            /* Place player on stairs up */
            Vec2 su = dl->stairs_up[0];
            gs->player_pos = su;
            /* Try adjacent floor tile so player isn't standing on the < */
            for (int d = 0; d < 8; d++) {
                int px = su.x + dir_dx[d];
                int py = su.y + dir_dy[d];
                if (px > 0 && px < MAP_WIDTH - 1 && py > 0 && py < MAP_HEIGHT - 1 &&
                    dl->tiles[py][px].type == TILE_FLOOR) {
                    gs->player_pos = (Vec2){ px, py };
                    break;
                }
            }

            gs->mode = MODE_DUNGEON;
            if (gs->riding) {
                gs->riding = false;
                log_add(&gs->log, gs->turn, CP_BROWN, "You dismount and leave your horse at the entrance.");
            }
            dungeon_update_fov(gs);
            if (loc->type == LOC_CASTLE_ABANDONED) {
                log_add(&gs->log, gs->turn, CP_GRAY,
                         "You enter the ruins of %s... (%d floors)", loc->name, num_levels);
                if (strcmp(loc->name, "Castle Dolorous Garde") == 0)
                    log_add(&gs->log, gs->turn, CP_CYAN,
                             "A haunted chill fills the air. Ghosts wander these halls.");
                else if (strcmp(loc->name, "Castle Perilous") == 0)
                    log_add(&gs->log, gs->turn, CP_MAGENTA,
                             "Dark sorcery taints this place. Beware magical traps.");
                else if (strcmp(loc->name, "Bamburgh Castle") == 0)
                    log_add(&gs->log, gs->turn, CP_RED,
                             "Bandits have made this ruin their stronghold. Steel yourself.");
            } else {
                log_add(&gs->log, gs->turn, CP_WHITE,
                         "You descend into %s... (%d levels deep)", loc->name, num_levels);
            }
        } else {
            log_add(&gs->log, gs->turn, CP_GRAY, "There is no entrance here.");
        }
        return;
    }
}

/* ------------------------------------------------------------------ */
/* Dungeon input                                                       */
/* ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ */
/* Town service forward declarations                                   */
static void town_do_inn(GameState *gs);
static void town_do_church(GameState *gs);
static void town_do_mystic(GameState *gs);
static void town_do_bank(GameState *gs);
static void town_do_well(GameState *gs);

/* Town menu system                                                    */
/* ------------------------------------------------------------------ */

/* Render the town interior map with NPCs */
static void town_render(GameState *gs) {
    TownMap *tm = &gs->town_map;

    /* Render map tiles */
    for (int y = 0; y < TOWN_MAP_H; y++) {
        for (int x = 0; x < TOWN_MAP_W; x++) {
            Tile *t = &tm->map[y][x];
            attron(COLOR_PAIR(t->color_pair));
            mvaddch(y, x, t->glyph);
            attroff(COLOR_PAIR(t->color_pair));
        }
    }

    /* Render NPCs */
    for (int i = 0; i < tm->num_npcs; i++) {
        TownNPC *npc = &tm->npcs[i];
        attron(COLOR_PAIR(npc->color_pair) | A_BOLD);
        mvaddch(npc->pos.y, npc->pos.x, npc->glyph);
        attroff(COLOR_PAIR(npc->color_pair) | A_BOLD);
    }

    /* Render player */
    attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
    mvaddch(gs->town_player_pos.y, gs->town_player_pos.x, '@');
    attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
}

/* Handle bumping into an NPC -- trigger the appropriate service */
static void town_interact_npc(GameState *gs, TownNPC *npc) {
    switch (npc->type) {
    case NPC_INNKEEPER:
        town_do_inn(gs);
        break;
    case NPC_PRIEST:
        town_do_church(gs);
        break;
    case NPC_MYSTIC:
        if (gs->current_town && strcmp(gs->current_town->name, "Glastonbury") == 0) {
            /* Merlin in Glastonbury */
            ui_clear();
            int row = 2;
            attron(COLOR_PAIR(CP_CYAN_BOLD) | A_BOLD);
            mvprintw(row++, 4, "Merlin the Wizard peers at you with ancient eyes.");
            attroff(COLOR_PAIR(CP_CYAN_BOLD) | A_BOLD);
            row++;
            attron(COLOR_PAIR(CP_CYAN));
            mvprintw(row++, 4, "\"Ah, %s. I have been expecting you.\"", gs->player_name);
            row++;

            /* Merlin can cure vampirism */
            if (gs->has_vampirism) {
                mvprintw(row++, 4, "\"I sense the darkness within you... vampirism.\"");
                mvprintw(row++, 4, "\"I know an ancient cure. Hold still...\"");
                attroff(COLOR_PAIR(CP_CYAN));
                gs->has_vampirism = false;
                gs->str -= 3; if (gs->str < 1) gs->str = 1;
                gs->spd -= 2; if (gs->spd < 1) gs->spd = 1;
                attron(COLOR_PAIR(CP_GREEN));
                mvprintw(row++, 4, "Merlin cures your vampirism!");
                attroff(COLOR_PAIR(CP_GREEN));
                row++;
                attron(COLOR_PAIR(CP_GRAY));
                mvprintw(row, 4, "Press any key to continue.");
                attroff(COLOR_PAIR(CP_GRAY));
                ui_refresh();
                ui_getkey();
                log_add(&gs->log, gs->turn, CP_GREEN, "Merlin cures your vampirism!");
                gs->light_affinity += 3;
            } else {

            /* Merlin teaches a spell or buffs INT */
            int scount;
            const SpellDef *spells = spell_get_defs(&scount);
            /* Find a spell the player doesn't know */
            int teach = -1;
            for (int tries = 0; tries < 50; tries++) {
                int si = rng_range(0, scount - 1);
                if (spells[si].level_required > gs->player_level) continue;
                bool known = false;
                for (int k = 0; k < gs->num_spells; k++)
                    if (gs->spells_known[k] == si) { known = true; break; }
                if (!known) { teach = si; break; }
            }

            if (teach >= 0 && gs->num_spells < gs->max_spells_capacity) {
                gs->spells_known[gs->num_spells++] = teach;
                mvprintw(row++, 4, "\"Let me teach you something useful...\"");
                row++;
                attron(COLOR_PAIR(CP_YELLOW_BOLD));
                mvprintw(row++, 4, "Merlin teaches you: %s!", spells[teach].name);
                attroff(COLOR_PAIR(CP_YELLOW_BOLD));
            } else {
                gs->intel += 1;
                gs->max_mp += 3;
                mvprintw(row++, 4, "\"Your mind has great potential. Let me sharpen it.\"");
                row++;
                attron(COLOR_PAIR(CP_GREEN));
                mvprintw(row++, 4, "+1 INT, +3 max MP!");
                attroff(COLOR_PAIR(CP_GREEN));
            }

            row++;
            /* Grail hint */
            if (gs->quests.grail_quest_active && !gs->quests.grail_quest_complete) {
                mvprintw(row++, 4, "\"The Grail rests in %s, brave %s.\"",
                         gs->grail_dungeon,
                         gs->player_gender == GENDER_MALE ? "Sir" : "Lady");
                mvprintw(row++, 4, "  \"Seek it at the bottom. But beware the guardian.\"");
            } else {
                mvprintw(row++, 4, "\"Seek King Arthur at Camelot. He needs you.\"");
            }
            attroff(COLOR_PAIR(CP_CYAN));
            row++;
            attron(COLOR_PAIR(CP_GRAY));
            mvprintw(row, 4, "Press any key to continue.");
            attroff(COLOR_PAIR(CP_GRAY));
            ui_refresh();
            ui_getkey();
            gs->light_affinity += 3;
            } /* end else (non-vampire Merlin) */
        } else if (gs->current_town && strcmp(gs->current_town->name, "Cornwall") == 0) {
            /* Morgan le Fay in Cornwall */
            ui_clear();
            int row = 2;
            attron(COLOR_PAIR(CP_MAGENTA_BOLD) | A_BOLD);
            mvprintw(row++, 4, "A dark-robed woman steps from the shadows.");
            mvprintw(row++, 4, "\"I am Morgan le Fay. I have a bargain for you.\"");
            attroff(COLOR_PAIR(CP_MAGENTA_BOLD) | A_BOLD);
            row++;
            attron(COLOR_PAIR(CP_MAGENTA));
            int offer = rng_range(0, 2);
            const char *boon_desc, *cost_desc;
            int boon_stat = 0, cost_hp = 0;
            switch (offer) {
            case 0:
                boon_desc = "+3 STR (permanent)";
                cost_desc = "-10 max HP (permanent)";
                boon_stat = 0; cost_hp = 10;
                break;
            case 1:
                boon_desc = "+3 INT (permanent)";
                cost_desc = "-8 max HP (permanent)";
                boon_stat = 2; cost_hp = 8;
                break;
            default:
                boon_desc = "+3 SPD (permanent)";
                cost_desc = "-8 max HP (permanent)";
                boon_stat = 3; cost_hp = 8;
                break;
            }
            mvprintw(row++, 4, "\"I offer you: %s\"", boon_desc);
            mvprintw(row++, 4, "\"The cost:    %s\"", cost_desc);
            attroff(COLOR_PAIR(CP_MAGENTA));
            row++;
            attron(COLOR_PAIR(CP_GRAY));
            mvprintw(row++, 4, "Press 'a' to accept the bargain, or 'q' to decline.");
            attroff(COLOR_PAIR(CP_GRAY));
            ui_refresh();

            int mk = ui_getkey();
            if (mk == 'a') {
                switch (boon_stat) {
                case 0: gs->str += 3; break;
                case 2: gs->intel += 3; break;
                case 3: gs->spd += 3; break;
                }
                gs->max_hp -= cost_hp;
                if (gs->hp > gs->max_hp) gs->hp = gs->max_hp;
                if (gs->max_hp < 5) gs->max_hp = 5;
                gs->dark_affinity += 5;
                gs->chivalry -= 3;
                if (gs->chivalry < 0) gs->chivalry = 0;
                log_add(&gs->log, gs->turn, CP_MAGENTA_BOLD,
                         "Morgan le Fay's dark power flows through you. %s, %s.",
                         boon_desc, cost_desc);
            } else {
                log_add(&gs->log, gs->turn, CP_WHITE,
                         "\"Perhaps another time...\" Morgan fades into the shadows.");
            }
        } else {
            /* Regular mystic */
            town_do_mystic(gs);
        }
        break;
    case NPC_BANKER:
        town_do_bank(gs);
        break;
    case NPC_WELL:
        town_do_well(gs);
        break;
    case NPC_EQUIP_SHOP:
    case NPC_POTION_SHOP:
    case NPC_PAWN_SHOP:
    case NPC_FOOD_SHOP:
    case NPC_JEWELLER:
        if (is_night(gs->hour)) {
            log_add(&gs->log, gs->turn, CP_GRAY,
                     "The %s's shop is closed for the night.", npc->label);
        } else {
            /* Shop menu with stock quantities */
            {
            int tcount;
            const ItemTemplate *tmps = item_get_templates(&tcount);

            /* Collect all matching item candidates */
            int candidates[100];
            int num_candidates = 0;
            for (int ti = 0; ti < tcount && num_candidates < 100; ti++) {
                bool match = false;
                if (npc->type == NPC_EQUIP_SHOP &&
                    (tmps[ti].type == ITYPE_WEAPON || tmps[ti].type == ITYPE_ARMOR ||
                     tmps[ti].type == ITYPE_SHIELD || tmps[ti].type == ITYPE_HELMET ||
                     tmps[ti].type == ITYPE_BOOTS || tmps[ti].type == ITYPE_GLOVES)) {
                    match = true;
                }
                if (npc->type == NPC_POTION_SHOP &&
                    (tmps[ti].type == ITYPE_POTION || tmps[ti].type == ITYPE_SCROLL)) {
                    match = true;
                }
                if (npc->type == NPC_FOOD_SHOP &&
                    tmps[ti].type == ITYPE_FOOD) {
                    match = true;
                }
                if (npc->type == NPC_JEWELLER &&
                    (tmps[ti].type == ITYPE_RING || tmps[ti].type == ITYPE_AMULET ||
                     tmps[ti].type == ITYPE_GEM)) {
                    match = true;
                }
                if (npc->type == NPC_PAWN_SHOP) {
                    match = true;
                }
                if (match && tmps[ti].rarity >= 2) {
                    candidates[num_candidates++] = ti;
                }
            }

            /* Shuffle candidates */
            for (int i = num_candidates - 1; i > 0; i--) {
                int j = rng_range(0, i);
                int tmp = candidates[i];
                candidates[i] = candidates[j];
                candidates[j] = tmp;
            }

            /* Pick limited stock based on shop type */
            int max_stock;
            if (npc->type == NPC_FOOD_SHOP)       max_stock = rng_range(3, 6);
            else if (npc->type == NPC_POTION_SHOP) max_stock = rng_range(6, 12);
            else if (npc->type == NPC_EQUIP_SHOP)  max_stock = rng_range(6, 12);
            else if (npc->type == NPC_JEWELLER)    max_stock = rng_range(4, 10);
            else                                   max_stock = rng_range(8, 16); /* pawn */
            if (max_stock > num_candidates) max_stock = num_candidates;

            int shop_items[20];
            int shop_qty[20];
            int num_shop = 0;
            for (int ci = 0; ci < max_stock && num_shop < 20; ci++) {
                int ti = candidates[ci];
                shop_items[num_shop] = ti;
                /* Stock based on rarity: common=3-5, normal=2-3, uncommon=1-2 */
                shop_qty[num_shop] = (tmps[ti].rarity >= 4) ? rng_range(3, 5) :
                                     (tmps[ti].rarity >= 3) ? rng_range(2, 3) : rng_range(1, 2);
                num_shop++;
            }

            int term_rows2, term_cols2;
            ui_get_size(&term_rows2, &term_cols2);

            /* Shop loop */
            bool shop_open = true;
            char shop_msg[128] = "";
            short shop_msg_color = CP_WHITE;
            while (shop_open) {
                ui_clear();
                int srow = 1;

                attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
                mvprintw(srow++, 2, "=== %s's Shop ===", npc->label);
                attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
                srow++;
                attron(COLOR_PAIR(CP_WHITE));
                mvprintw(srow++, 2, "Your gold: %d    Bag: %d/%d", gs->gold, gs->num_items, MAX_INVENTORY);
                attroff(COLOR_PAIR(CP_WHITE));

                /* Show feedback from last action */
                if (shop_msg[0]) {
                    attron(COLOR_PAIR(shop_msg_color) | A_BOLD);
                    mvprintw(srow, 2, "%s", shop_msg);
                    attroff(COLOR_PAIR(shop_msg_color) | A_BOLD);
                    shop_msg[0] = 0;
                }
                srow++;
                mvprintw(srow++, 2, "Press a letter to buy:");
                attroff(COLOR_PAIR(CP_WHITE));

                int visible_count = 0;
                for (int si = 0; si < num_shop && srow < term_rows2 - 6; si++) {
                    if (shop_qty[si] <= 0) continue;
                    const ItemTemplate *it = &tmps[shop_items[si]];
                    int price = it->value;
                    if (npc->type == NPC_PAWN_SHOP) price = it->value * 4 / 5;
                    attron(COLOR_PAIR(it->color_pair));
                    mvprintw(srow++, 4, "%c) %-20s [%s] pow:%-2d  %3dg  (x%d)",
                             'a' + si, it->name, item_type_name(it->type), it->power, price, shop_qty[si]);
                    attroff(COLOR_PAIR(it->color_pair));
                    visible_count++;
                }

                if (visible_count == 0) {
                    attron(COLOR_PAIR(CP_GRAY));
                    mvprintw(srow++, 4, "(Shop is sold out!)");
                    attroff(COLOR_PAIR(CP_GRAY));
                }

                srow++;
                attron(COLOR_PAIR(CP_WHITE));
                mvprintw(srow++, 2, "[S] Sell    [h] Haggle    [t] Steal    [q] Leave");
                attroff(COLOR_PAIR(CP_WHITE));
                ui_refresh();

                int skey = ui_getkey();
                if (skey == 'q' || skey == 'Q' || skey == 27) {
                    shop_open = false;
                } else if (skey == 'h') {
                    /* Haggle -- INT + chivalry check for discount */
                    int haggle_chance = 30 + gs->intel * 3 + gs->chivalry / 5;
                    if (haggle_chance > 80) haggle_chance = 80;
                    if (rng_chance(haggle_chance)) {
                        int discount = rng_range(10, 20);
                        snprintf(shop_msg, sizeof(shop_msg),
                                 "\"You drive a hard bargain!\" %d%% off next purchase!", discount);
                        shop_msg_color = CP_GREEN;
                        /* Apply discount to all prices temporarily by adjusting gold */
                        /* Simple approach: give player bonus gold equivalent */
                        /* Better: track haggle discount -- but for simplicity, just give a gold bonus */
                        int bonus = gs->gold * discount / 100;
                        if (bonus < 5) bonus = 5;
                        if (bonus > 30) bonus = 30;
                        gs->gold += bonus;
                        log_add(&gs->log, gs->turn, CP_GREEN,
                                 "Successful haggle! The shopkeeper gives you a better deal. (+%dg credit)", bonus);
                    } else {
                        snprintf(shop_msg, sizeof(shop_msg),
                                 "The shopkeeper frowns at your offer. Prices stay the same.");
                        shop_msg_color = CP_RED;
                        /* Low chivalry makes it worse */
                        if (gs->chivalry < 30) {
                            snprintf(shop_msg, sizeof(shop_msg),
                                     "\"I don't haggle with the likes of you!\" Prices went UP.");
                            shop_msg_color = CP_RED;
                        }
                    }
                } else if (skey == 't') {
                    /* Steal -- SPD check, chivalry loss, potential ban */
                    if (visible_count == 0) {
                        snprintf(shop_msg, sizeof(shop_msg), "Nothing to steal -- the shop is empty!");
                        shop_msg_color = CP_GRAY;
                    } else if (gs->num_items >= MAX_INVENTORY) {
                        snprintf(shop_msg, sizeof(shop_msg), "Your inventory is full!");
                        shop_msg_color = CP_RED;
                    } else {
                        int steal_chance = 20 + gs->spd * 4;
                        if (steal_chance > 70) steal_chance = 70;
                        /* Pick a random item to try to steal */
                        int steal_idx = -1;
                        for (int tries = 0; tries < 20; tries++) {
                            int si = rng_range(0, num_shop - 1);
                            if (shop_qty[si] > 0) { steal_idx = si; break; }
                        }
                        if (steal_idx >= 0 && rng_chance(steal_chance)) {
                            /* Success! */
                            const ItemTemplate *it = &tmps[shop_items[steal_idx]];
                            gs->inventory[gs->num_items] = item_create(shop_items[steal_idx], -1, -1);
                            gs->inventory[gs->num_items].on_ground = false;
                            gs->num_items++;
                            shop_qty[steal_idx]--;
                            gs->chivalry -= 8;
                            if (gs->chivalry < 0) gs->chivalry = 0;
                            snprintf(shop_msg, sizeof(shop_msg),
                                     "You pocket the %s! (-8 chivalry)", it->name);
                            shop_msg_color = CP_MAGENTA;
                            log_add(&gs->log, gs->turn, CP_MAGENTA,
                                     "You stole a %s! The shopkeeper didn't notice... yet. (-8 chivalry)", it->name);
                        } else {
                            /* Caught! */
                            gs->chivalry -= 5;
                            if (gs->chivalry < 0) gs->chivalry = 0;
                            int guard_dmg = rng_range(3, 10);
                            gs->hp -= guard_dmg;
                            if (gs->hp < 1) gs->hp = 1;
                            snprintf(shop_msg, sizeof(shop_msg),
                                     "CAUGHT! Guards rough you up (-%d HP, -5 chivalry). Get out!",
                                     guard_dmg);
                            shop_msg_color = CP_RED;
                            log_add(&gs->log, gs->turn, CP_RED,
                                     "\"THIEF!\" The shopkeeper catches you! Guards attack! (-%d HP, -5 chivalry)", guard_dmg);
                            shop_open = false; /* kicked out */
                        }
                    }
                } else if (skey >= 'a' && skey < 'a' + num_shop) {
                    int si = skey - 'a';
                    if (si < num_shop && shop_qty[si] > 0) {
                        const ItemTemplate *it = &tmps[shop_items[si]];
                        int price = it->value;
                        if (npc->type == NPC_PAWN_SHOP) price = it->value * 4 / 5;

                        if (gs->gold < price) {
                            snprintf(shop_msg, sizeof(shop_msg), "You can't afford %s (%dg).", it->name, price);
                            shop_msg_color = CP_RED;
                        } else if (gs->num_items >= MAX_INVENTORY) {
                            snprintf(shop_msg, sizeof(shop_msg), "Your inventory is full!");
                            shop_msg_color = CP_RED;
                        } else {
                            gs->gold -= price;
                            gs->inventory[gs->num_items] = item_create(shop_items[si], -1, -1);
                            gs->inventory[gs->num_items].on_ground = false;
                            gs->num_items++;
                            shop_qty[si]--;
                            snprintf(shop_msg, sizeof(shop_msg), "Bought %s for %dg. Gold: %d", it->name, price, gs->gold);
                            shop_msg_color = CP_CYAN;
                            log_add(&gs->log, gs->turn, CP_CYAN, "Bought %s for %dg.", it->name, price);
                        }
                    }
                } else if (skey == 'S') {
                    if (gs->num_items == 0) {
                        log_add(&gs->log, gs->turn, CP_GRAY, "You have nothing to sell.");
                    } else {
                        ui_clear();
                        int sr = 1;
                        attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
                        mvprintw(sr++, 2, "=== Sell Items ===");
                        attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
                        sr++;
                        for (int ii = 0; ii < gs->num_items && sr < term_rows2 - 3; ii++) {
                            Item *inv = &gs->inventory[ii];
                            if (inv->template_id < 0) continue;
                            int sell_price = inv->value / 2;
                            if (npc->type == NPC_PAWN_SHOP) sell_price = inv->value * 2 / 5;
                            attron(COLOR_PAIR(inv->color_pair));
                            mvprintw(sr++, 4, "%c) %s -- %dg", 'a' + ii, inv->name, sell_price);
                            attroff(COLOR_PAIR(inv->color_pair));
                        }
                        sr++;
                        mvprintw(sr, 4, "Press a-z to sell, any other key to go back");
                        ui_refresh();

                        int sellkey = ui_getkey();
                        if (sellkey >= 'a' && sellkey < 'a' + gs->num_items) {
                            int idx = sellkey - 'a';
                            if (idx < gs->num_items && gs->inventory[idx].template_id >= 0) {
                                int sell_price = gs->inventory[idx].value / 2;
                                if (npc->type == NPC_PAWN_SHOP) sell_price = gs->inventory[idx].value * 2 / 5;
                                gs->gold += sell_price;
                                snprintf(shop_msg, sizeof(shop_msg), "Sold %s for %dg. Gold: %d", gs->inventory[idx].name, sell_price, gs->gold);
                                shop_msg_color = CP_YELLOW;
                                log_add(&gs->log, gs->turn, CP_YELLOW, "Sold %s for %dg.", gs->inventory[idx].name, sell_price);
                                for (int j = idx; j < gs->num_items - 1; j++)
                                    gs->inventory[j] = gs->inventory[j + 1];
                                gs->inventory[--gs->num_items].template_id = -1;
                            }
                        }
                    }
                }
            } /* end shop loop */
            }
        }
        break;
    case NPC_STABLE:
        if (gs->horse_type > 0) {
            const char *hnames[] = { "", "Pony", "Palfrey", "Destrier" };
            int sell_prices[] = { 0, 25, 50, 100 };
            int sell_val = sell_prices[gs->horse_type];
            log_add(&gs->log, gs->turn, CP_BROWN,
                     "\"Fine %s you have there! I'd buy it for %d gold. Press 's' to sell.\"",
                     hnames[gs->horse_type], sell_val);
            game_render(gs);
            int sk = ui_getkey();
            if (sk == 's') {
                gs->gold += sell_val;
                log_add(&gs->log, gs->turn, CP_YELLOW,
                         "You sell your %s for %d gold.", hnames[gs->horse_type], sell_val);
                gs->horse_type = 0;
                gs->riding = false;
            }
        } else {
            /* Price varies by town -- hash town name for deterministic variation */
            unsigned int th = 0;
            if (gs->current_town) {
                for (const char *p = gs->current_town->name; *p; p++)
                    th = th * 31 + (unsigned char)*p;
            }
            int pony_cost    = 40 + (int)(th % 25);          /* 40-64 */
            int palfrey_cost = 85 + (int)((th / 25) % 35);   /* 85-119 */
            int destrier_cost= 170 + (int)((th / 100) % 65); /* 170-234 */

            /* Show horse selection menu */
            ui_clear();
            int row = 2;
            attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
            mvprintw(row++, 4, "=== Stablemaster ===");
            attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
            row++;
            attron(COLOR_PAIR(CP_BROWN));
            mvprintw(row++, 4, "\"Welcome! I have fine steeds for sale:\"");
            attroff(COLOR_PAIR(CP_BROWN));
            row++;
            attron(COLOR_PAIR(CP_CYAN));
            mvprintw(row++, 6, "[1] Pony      - %3d gold  -- 1.5x speed, good in hills", pony_cost);
            mvprintw(row++, 6, "[2] Palfrey   - %3d gold  -- 2x speed, standard riding horse", palfrey_cost);
            mvprintw(row++, 6, "[3] Destrier  - %3d gold  -- 2x speed, +2 DEF (war horse)", destrier_cost);
            attroff(COLOR_PAIR(CP_CYAN));
            row++;
            attron(COLOR_PAIR(CP_GRAY));
            mvprintw(row++, 6, "Press 1-3 to buy, or q to leave.");
            mvprintw(row++, 6, "You have %d gold.", gs->gold);
            attroff(COLOR_PAIR(CP_GRAY));
            ui_refresh();

            int skey = ui_getkey();
            int cost = 0, htype = 0;
            const char *hname = "";
            if (skey == '1')      { cost = pony_cost;     htype = 1; hname = "Pony"; }
            else if (skey == '2') { cost = palfrey_cost;  htype = 2; hname = "Palfrey"; }
            else if (skey == '3') { cost = destrier_cost; htype = 3; hname = "Destrier"; }

            if (htype > 0) {
                if (gs->gold >= cost) {
                    gs->gold -= cost;
                    gs->horse_type = htype;
                    log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                             "You buy a %s! It will be waiting outside for you.", hname);
                    log_add(&gs->log, gs->turn, CP_WHITE,
                             "Press 'H' on the overworld to mount/dismount.");
                } else {
                    log_add(&gs->log, gs->turn, CP_RED,
                             "You don't have enough gold. (need %d, have %d)", cost, gs->gold);
                }
            }
        }
        break;
    case NPC_TOWNFOLK:
        {
            /* Grail hint from NPCs (30% chance of red herring) */
            if (gs->quests.grail_quest_active && !gs->has_grail &&
                !gs->quests.grail_quest_complete && rng_chance(25)) {
                const char *all_dungeons[] = {
                    "Camelot Catacombs", "Tintagel Caves", "Sherwood Depths",
                    "Mount Draig", "Glastonbury Tor", "White Cliffs Cave",
                    "Whitby Abbey", "Avalon Shrine", "Orkney Barrows"
                };
                const char *hint_dungeon;
                if (rng_chance(70)) {
                    /* True hint */
                    hint_dungeon = gs->grail_dungeon;
                } else {
                    /* Red herring */
                    hint_dungeon = all_dungeons[rng_range(0, 8)];
                }
                const char *intros[] = {
                    "\"I heard a holy light was seen deep beneath %s...\"",
                    "\"A pilgrim told me the Grail lies under %s.\"",
                    "\"Rumour has it a sacred chalice is hidden in %s.\"",
                    "\"The druids whisper of the Grail in %s...\"",
                };
                char hint_buf[128];
                snprintf(hint_buf, sizeof(hint_buf), intros[rng_range(0, 3)], hint_dungeon);
                log_add(&gs->log, gs->turn, CP_YELLOW, "%s", hint_buf);
            } else {
                const char *chatter[] = {
                    "\"Lovely day, isn't it?\"",
                    "\"Watch out for bandits on the roads.\"",
                    "\"Have you visited the inn? Best ale in England!\"",
                    "\"I heard there's treasure in the old ruins.\"",
                    "\"The King rode through here last week.\"",
                    "\"Strange lights in the forest last night...\"",
                };
                int n = sizeof(chatter) / sizeof(chatter[0]);
                log_add(&gs->log, gs->turn, CP_WHITE, "%s", chatter[rng_range(0, n - 1)]);
            }
        }
        break;
    case NPC_KING:
        if (gs->current_town && strcmp(gs->current_town->name, "Camelot Castle") == 0) {
            /* King Arthur at Camelot Castle */
            if (!gs->quests.grail_quest_active) {
                /* First visit: Arthur grants the Grail quest and knighthood */
                ui_clear();
                int row = 2;
                attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
                mvprintw(row++, 4, "King Arthur rises from the throne.");
                attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
                row++;
                attron(COLOR_PAIR(CP_YELLOW));
                mvprintw(row++, 4, "\"Welcome, %s. England is in peril.", gs->player_name);
                mvprintw(row++, 4, " The Holy Grail has been lost, and with it,");
                mvprintw(row++, 4, " the land's power fades. I charge you with");
                mvprintw(row++, 4, " this sacred quest: find the Grail and return");
                mvprintw(row++, 4, " it to Camelot.\"");
                row++;
                mvprintw(row++, 4, "Arthur draws his sword and taps your shoulders.");
                mvprintw(row++, 4, "\"I knight thee %s %s. Rise, and go forth!\"",
                         gs->player_gender == GENDER_MALE ? "Sir" : "Lady", gs->player_name);
                attroff(COLOR_PAIR(CP_YELLOW));
                row++;
                attron(COLOR_PAIR(CP_GREEN));
                mvprintw(row++, 4, "You receive: +2 DEF, +1 STR, the Grail Quest.");
                attroff(COLOR_PAIR(CP_GREEN));
                row++;
                attron(COLOR_PAIR(CP_GRAY));
                mvprintw(row, 4, "Press any key to continue.");
                attroff(COLOR_PAIR(CP_GRAY));
                ui_refresh();
                ui_getkey();

                gs->quests.grail_quest_active = true;
                gs->def += 2;
                gs->str += 1;
                gs->chivalry += 5;
                if (gs->chivalry > 100) gs->chivalry = 100;

                /* Randomly assign the Grail to a dungeon */
                {
                    const char *grail_dungeons[] = {
                        "Camelot Catacombs", "Tintagel Caves", "Sherwood Depths",
                        "Mount Draig", "Glastonbury Tor", "White Cliffs Cave",
                        "Whitby Abbey", "Avalon Shrine", "Orkney Barrows"
                    };
                    int gd = rng_range(0, 8);
                    snprintf(gs->grail_dungeon, MAX_NAME, "%s", grail_dungeons[gd]);
                }
                log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                         "King Arthur has knighted you and granted the Grail Quest!");
                log_add(&gs->log, gs->turn, CP_WHITE,
                         "Seek the Holy Grail! Ask Merlin or innkeepers for hints.");
            } else if (gs->has_grail && !gs->quests.grail_quest_complete) {
                /* Returning the Grail! */
                if (gs->chivalry < 30) {
                    log_add(&gs->log, gs->turn, CP_YELLOW,
                             "Arthur: \"You have the Grail, but you are not worthy to present it.\"");
                    log_add(&gs->log, gs->turn, CP_YELLOW,
                             "\"Prove your honour first. (Need chivalry 30+, have %d)\"", gs->chivalry);
                } else {
                    gs->quests.grail_quest_complete = true;
                    gs->has_grail = false;

                    ui_clear();
                    int row = 3;
                    attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
                    mvprintw(row++, 4, "=== THE HOLY GRAIL IS RETURNED! ===");
                    attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
                    row++;
                    attron(COLOR_PAIR(CP_YELLOW));
                    mvprintw(row++, 4, "King Arthur rises, tears in his eyes.");
                    mvprintw(row++, 4, "\"%s, you have done what no other could.\"", gs->player_name);
                    mvprintw(row++, 4, "\"The Holy Grail is restored to Camelot!\"");
                    mvprintw(row++, 4, "\"The land shall heal. You are truly the greatest");
                    mvprintw(row++, 4, " knight of the Round Table.\"");
                    attroff(COLOR_PAIR(CP_YELLOW));
                    row++;
                    attron(COLOR_PAIR(CP_GREEN));
                    mvprintw(row++, 4, "+5000 gold!");
                    mvprintw(row++, 4, "+10 chivalry!");
                    mvprintw(row++, 4, "Title: Grail Knight");
                    mvprintw(row++, 4, "+500 XP!");
                    attroff(COLOR_PAIR(CP_GREEN));
                    row += 2;
                    attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
                    mvprintw(row, 4, "Press any key to continue your adventure...");
                    attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
                    ui_refresh();
                    ui_getkey();

                    gs->gold += 5000;
                    gs->chivalry += 10;
                    if (gs->chivalry > 100) gs->chivalry = 100;
                    gs->xp += 500;
                    log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                             "*** THE HOLY GRAIL HAS BEEN RETURNED! You are the Grail Knight! ***");
                }
            } else if (gs->quests.grail_quest_complete) {
                log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                         "Arthur beams. \"You have saved the kingdom, %s!\"", gs->player_name);
            } else {
                int qc = quest_count_completed(&gs->quests);
                if (qc >= 3 && !gs->quests.grail_quest_complete) {
                    /* Arthur gives a royal sword after 3+ quests */
                    log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                             "\"You have proven yourself worthy. Take this royal blade.\"");
                    /* Give a Bastard Sword if room in inventory */
                    if (gs->num_items < MAX_INVENTORY) {
                        int tcount; const ItemTemplate *tmps = item_get_templates(&tcount);
                        for (int ti = 0; ti < tcount; ti++) {
                            if (strcmp(tmps[ti].name, "Bastard Sword") == 0) {
                                gs->inventory[gs->num_items] = item_create(ti, -1, -1);
                                gs->inventory[gs->num_items].on_ground = false;
                                gs->num_items++;
                                log_add(&gs->log, gs->turn, CP_YELLOW, "Received: Bastard Sword!");
                                break;
                            }
                        }
                    }
                } else {
                    const char *msgs[] = {
                        "\"The Grail awaits, brave knight. Seek it in the deep places.\"",
                        "\"England needs heroes. Do not fail us.\"",
                        "\"Have you news from the road? Speak!\"",
                        "\"Prove your worth and you shall sit at the Round Table.\"",
                        "\"Be vigilant. Dark forces stir in the land.\"",
                    };
                    int n = sizeof(msgs) / sizeof(msgs[0]);
                    log_add(&gs->log, gs->turn, CP_YELLOW_BOLD, "Arthur: %s", msgs[rng_range(0, n - 1)]);
                }
            }
        } else {
            /* Generic king at other castles */
            const char *title = gs->player_gender == GENDER_MALE ? "Sir" : "Lady";
            if (gs->chivalry >= 60) {
                log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                         "The King rises. \"Welcome, %s %s! Your fame precedes you.\"",
                         title, gs->player_name);
                /* Princess rescued -- offer reward/marriage */
                if (gs->princess_rescued && !gs->princess_married) {
                    ui_clear();
                    int row = 3;
                    attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
                    mvprintw(row++, 4, "\"You have rescued my daughter! You are a true hero!\"");
                    attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
                    row++;
                    attron(COLOR_PAIR(CP_YELLOW));
                    mvprintw(row++, 4, "\"She wishes to marry you. Will you accept?\"");
                    attroff(COLOR_PAIR(CP_YELLOW));
                    row++;
                    attron(COLOR_PAIR(CP_CYAN));
                    mvprintw(row++, 4, "[y] Accept -- become %s (TRUE ENDING, +10000 score)",
                             gs->player_gender == GENDER_MALE ? "King" : "Queen");
                    mvprintw(row++, 4, "[n] Decline -- +5 chivalry, +500 gold, continue playing");
                    attroff(COLOR_PAIR(CP_CYAN));
                    ui_refresh();
                    int mk = ui_getkey();
                    if (mk == 'y' || mk == 'Y') {
                        gs->princess_married = true;
                        gs->xp += 10000;
                        gs->chivalry = 100;
                        log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                                 "*** You married the Princess and became %s! THE TRUE ENDING! ***",
                                 gs->player_gender == GENDER_MALE ? "King" : "Queen");
                        log_add(&gs->log, gs->turn, CP_YELLOW,
                                 "+10000 score! Your quest is truly complete.");
                    } else {
                        gs->chivalry += 5;
                        if (gs->chivalry > 100) gs->chivalry = 100;
                        gs->gold += 500;
                        gs->princess_rescued = false; /* quest done, no re-offer */
                        log_add(&gs->log, gs->turn, CP_WHITE,
                                 "The princess is disappointed but understanding. +5 chivalry, +500g.");
                    }
                    break;
                }
                /* Offer princess quest if chivalry 50+ and not already active */
                if (gs->chivalry >= 50 && !gs->princess_quest_active && !gs->princess_rescued) {
                    log_add(&gs->log, gs->turn, CP_YELLOW,
                             "\"My daughter has been kidnapped by an Evil Sorcerer!\"");
                    log_add(&gs->log, gs->turn, CP_WHITE,
                             "\"She is locked in a tower. Will you rescue her? Press 'y' to accept.\"");
                    game_render(gs);
                    int pk = ui_getkey();
                    if (pk == 'y' || pk == 'Y') {
                        gs->princess_quest_active = true;
                        /* Place the tower as a dungeon entrance on the overworld */
                        snprintf(gs->princess_tower, MAX_NAME, "Sorcerer's Tower");
                        /* Find a spot on the overworld for the tower */
                        for (int tries = 0; tries < 200; tries++) {
                            int tx = rng_range(80, OW_WIDTH - 80);
                            int ty = rng_range(30, OW_HEIGHT - 30);
                            if (gs->overworld->map[ty][tx].passable &&
                                gs->overworld->map[ty][tx].type == TILE_HILLS &&
                                !overworld_location_at(gs->overworld, tx, ty)) {
                                overworld_add_location(gs->overworld, "Sorcerer's Tower",
                                                LOC_DUNGEON_ENTRANCE, tx, ty, '|', CP_MAGENTA_BOLD);
                                log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                                         "Quest accepted! The Sorcerer's Tower has appeared in the hills.");
                                log_add(&gs->log, gs->turn, CP_WHITE,
                                         "Find and enter it to rescue the princess.");
                                break;
                            }
                        }
                    }
                }
            } else if (gs->chivalry >= 30) {
                log_add(&gs->log, gs->turn, CP_YELLOW,
                         "The King nods. \"What brings you to my court, %s?\"", title);
            } else {
                log_add(&gs->log, gs->turn, CP_GRAY,
                         "The King frowns. \"I have heard troubling things about you...\"");
            }
        }
        break;
    case NPC_QUEEN:
        if (gs->current_town && strcmp(gs->current_town->name, "Camelot Castle") == 0) {
            /* Queen Guinevere at Camelot */
            if (gs->chivalry >= 50) {
                /* High chivalry: she gives a blessed amulet or shop discount */
                log_add(&gs->log, gs->turn, CP_MAGENTA_BOLD,
                         "Guinevere smiles. \"Your honour is an example to us all, %s.\"",
                         gs->player_name);
                if (gs->light_affinity >= 15 && gs->num_items < MAX_INVENTORY) {
                    /* Give Amulet of Insight if Light affinity is high enough */
                    int tcount; const ItemTemplate *tmps = item_get_templates(&tcount);
                    for (int ti = 0; ti < tcount; ti++) {
                        if (strcmp(tmps[ti].name, "Amulet of Insight") == 0) {
                            gs->inventory[gs->num_items] = item_create(ti, -1, -1);
                            gs->inventory[gs->num_items].on_ground = false;
                            gs->num_items++;
                            log_add(&gs->log, gs->turn, CP_CYAN,
                                     "\"Take this blessed amulet. It will guide you.\"");
                            break;
                        }
                    }
                }
            } else {
                log_add(&gs->log, gs->turn, CP_MAGENTA,
                         "Guinevere regards you coolly. \"Prove your honour, and we may speak further.\"");
            }
        } else {
            const char *msgs[] = {
                "The Queen smiles warmly. \"You are always welcome here.\"",
                "\"The court whispers of your deeds, brave one.\"",
                "\"Please, bring peace to our troubled land.\"",
            };
            int n = sizeof(msgs) / sizeof(msgs[0]);
            log_add(&gs->log, gs->turn, CP_MAGENTA_BOLD, "%s", msgs[rng_range(0, n - 1)]);
        }
        break;
    case NPC_QUEST_GIVER:
        {
            /* Find available quest for this town */
            Quest *q = quest_find_available(&gs->quests,
                                             gs->current_town->name, gs->chivalry);
            if (q) {
                ui_clear();
                int row = 2;
                attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
                mvprintw(row++, 2, "=== Quest Available ===");
                attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
                row++;
                attron(COLOR_PAIR(CP_WHITE));
                mvprintw(row++, 4, "A stranger approaches you in the inn.");
                row++;
                attron(COLOR_PAIR(CP_YELLOW));
                mvprintw(row++, 4, "\"%s\"", q->name);
                attroff(COLOR_PAIR(CP_YELLOW));
                row++;
                /* Word-wrap description */
                const char *desc = q->description;
                int dx = 4, max_w = 60;
                while (*desc) {
                    int len = (int)strlen(desc);
                    if (len > max_w) len = max_w;
                    mvprintw(row++, dx, "%.*s", len, desc);
                    desc += len;
                }
                row++;
                mvprintw(row++, 4, "Reward: %d gold", q->gold_reward);
                if (q->stat_reward >= 0) {
                    const char *sn[] = { "+1 STR", "+1 DEF", "+1 INT", "+1 SPD" };
                    mvprintw(row++, 4, "Bonus: %s", sn[q->stat_reward]);
                }
                row++;
                mvprintw(row++, 4, "[a] Accept quest");
                mvprintw(row++, 4, "[d] Decline");
                attroff(COLOR_PAIR(CP_WHITE));
                ui_refresh();

                int qkey = ui_getkey();
                if (qkey == 'a') {
                    q->state = QUEST_ACTIVE;
                    log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                             "Quest accepted: %s", q->name);
                } else {
                    log_add(&gs->log, gs->turn, CP_GRAY,
                             "You decline the quest.");
                }
            } else {
                /* Check if any completed quests can be turned in here */
                bool turned_in = false;
                for (int qi = 0; qi < gs->quests.num_quests; qi++) {
                    Quest *cq = &gs->quests.quests[qi];
                    if (cq->state == QUEST_COMPLETE &&
                        strcmp(cq->giver_town, gs->current_town->name) == 0) {
                        cq->state = QUEST_TURNED_IN;
                        gs->gold += cq->gold_reward;
                        if (cq->stat_reward >= 0) {
                            int *stats[] = { &gs->str, &gs->def, &gs->intel, &gs->spd };
                            (*stats[cq->stat_reward])++;
                        }
                        log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                                 "Quest turned in: %s! +%dg", cq->name, cq->gold_reward);
                        turned_in = true;
                        break;
                    }
                }
                if (!turned_in) {
                    log_add(&gs->log, gs->turn, CP_GRAY,
                             "\"I have nothing for you right now. Check other inns.\"");
                }
            }
        }
        break;
    case NPC_MONK:
        {
            const char *msgs[] = {
                "The monk bows silently. \"Peace be with you, traveller.\"",
                "\"We brew the finest ale in England. Try some at the inn.\"",
                "\"This abbey has stood for centuries. May it stand for centuries more.\"",
                "\"Seek the Lord's guidance and you shall not falter.\"",
                "\"Our potions are made from the rarest herbs. Visit the apothecary.\"",
                "The monk hums a hymn softly as he tends the garden.",
            };
            int n = sizeof(msgs) / sizeof(msgs[0]);
            log_add(&gs->log, gs->turn, CP_BROWN, "%s", msgs[rng_range(0, n - 1)]);
        }
        break;
    case NPC_NUN:
        {
            const char *msgs[] = {
                "The nun curtsies. \"Blessings upon you, child.\"",
                "\"Our abbey offers shelter to all who seek it.\"",
                "\"Pray at the church and your wounds shall heal.\"",
                "\"We tend the sick and the weary. You look like both.\"",
                "\"The ale here is brewed by the sisters. It is... quite strong.\"",
                "The nun smiles warmly and offers a silent prayer for you.",
            };
            int n = sizeof(msgs) / sizeof(msgs[0]);
            log_add(&gs->log, gs->turn, CP_WHITE, "%s", msgs[rng_range(0, n - 1)]);
        }
        break;
    case NPC_GUARD:
        {
            /* Check if castle has jousting -- offer tournament */
            bool is_castle = (gs->current_town &&
                              (strncmp(gs->current_town->name, "Castle", 6) == 0 ||
                               strcmp(gs->current_town->name, "Camelot Castle") == 0 ||
                               strcmp(gs->current_town->name, "Edinburgh Castle") == 0 ||
                               strcmp(gs->current_town->name, "Dover Castle") == 0));

            /* Tournament days: 4 days per 30-day cycle, different per castle */
            bool tournament_today = false;
            if (is_castle && gs->current_town) {
                unsigned int ch = 0;
                for (const char *p = gs->current_town->name; *p; p++)
                    ch = ch * 31 + (unsigned char)*p;
                int moon = game_get_moon_day(gs);
                int days[4] = {
                    (int)(ch % 7) + 1,
                    (int)((ch / 7) % 7) + 8,
                    (int)((ch / 49) % 7) + 15,
                    (int)((ch / 343) % 7) + 22
                };
                for (int d = 0; d < 4; d++)
                    if (moon == days[d]) { tournament_today = true; break; }
            }

            /* Check if already jousted at this castle today */
            bool already_jousted = (gs->last_joust_day == gs->day &&
                                    gs->current_town &&
                                    strcmp(gs->last_joust_castle, gs->current_town->name) == 0);

            if (is_castle && gs->horse_type > 0 && tournament_today && !already_jousted) {
                /* Offer jousting tournament */
                int entry_fee = 20 + rng_range(0, 30);
                log_add(&gs->log, gs->turn, CP_YELLOW,
                         "\"A jousting tournament today! Entry fee: %dg. Press 'j' to enter.\"", entry_fee);
                game_render(gs);
                int jk = ui_getkey();
                if (jk == 'j') {
                    if (gs->gold < entry_fee) {
                        log_add(&gs->log, gs->turn, CP_RED, "You can't afford the entry fee.");
                    } else {
                        gs->gold -= entry_fee;
                        gs->last_joust_day = gs->day;
                        snprintf(gs->last_joust_castle, MAX_NAME, "%s", gs->current_town->name);
                        /* 3 rounds of jousting */
                        const char *opponents[] = { "Local Squire", "Rival Knight", "Castle Champion" };
                        int opp_skill[] = { 30, 55, 75 };
                        int prizes[] = { 30, 80, 200 };
                        bool destrier_bonus = (gs->horse_type == 3);
                        int total_winnings = 0;

                        for (int round = 0; round < 3; round++) {
                            ui_clear();
                            int row = 3;
                            attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
                            mvprintw(row++, 4, "=== JOUSTING TOURNAMENT -- Round %d ===", round + 1);
                            attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
                            row++;
                            attron(COLOR_PAIR(CP_WHITE));
                            mvprintw(row++, 4, "Opponent: %s", opponents[round]);
                            row++;

                            /* ASCII jousting animation */
                            mvprintw(row++, 4, "You charge! Press SPACE at the right moment...");
                            mvprintw(row++, 4, "Choose aim: [h]ead (risky) [c]hest (balanced) [s]hield (safe)");
                            attroff(COLOR_PAIR(CP_WHITE));
                            ui_refresh();

                            int aim = ui_getkey();
                            int aim_bonus = 0, aim_dmg = 0;
                            if (aim == 'h') { aim_bonus = -15; aim_dmg = 30; } /* head: hard to hit, big damage */
                            else if (aim == 'c') { aim_bonus = 0; aim_dmg = 20; } /* chest: balanced */
                            else { aim_bonus = 15; aim_dmg = 10; } /* shield: easy, low damage */

                            /* Timing mini-game: show charge animation */
                            row++;
                            int charge_row = row;
                            attron(COLOR_PAIR(CP_WHITE));
                            mvprintw(charge_row, 4, "                                           ");
                            attroff(COLOR_PAIR(CP_WHITE));
                            ui_refresh();

                            /* Quick countdown with visual */
                            int timing = 0;
                            for (int tick = 0; tick < 20; tick++) {
                                mvprintw(charge_row, 4, "                                           ");
                                int px = 4 + tick;
                                int ox = 44 - tick;
                                attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
                                mvaddch(charge_row, px, '>');
                                attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
                                attron(COLOR_PAIR(CP_RED) | A_BOLD);
                                mvaddch(charge_row, ox, '<');
                                attroff(COLOR_PAIR(CP_RED) | A_BOLD);
                                ui_refresh();

                                /* Check for keypress (non-blocking would be ideal but use timeout) */
                                timeout(150);
                                int tkey = getch();
                                timeout(-1);
                                if (tkey == ' ') {
                                    /* Perfect timing is around tick 9-11 (middle) */
                                    int dist = abs(tick - 10);
                                    if (dist <= 1) timing = 2; /* perfect */
                                    else if (dist <= 3) timing = 1; /* good */
                                    else timing = 0; /* miss */
                                    break;
                                }
                            }
                            /* Always restore blocking mode after charge animation */
                            timeout(-1);
                            /* Flush any buffered input from the timing loop */
                            nodelay(stdscr, TRUE);
                            while (getch() != ERR) {}
                            nodelay(stdscr, FALSE);

                            row = charge_row + 2;
                            /* Calculate result -- timing is the main factor */
                            int player_hit = 10 + (timing * 30) + aim_bonus + gs->str / 2;
                            if (destrier_bonus) player_hit += 8;
                            int opp_hit_chance = opp_skill[round] + rng_range(0, 20);

                            bool player_wins = (player_hit > opp_hit_chance);

                            if (timing == 2) {
                                attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
                                mvprintw(row++, 4, "PERFECT TIMING!");
                                attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
                                player_wins = true; /* perfect always wins */
                            } else if (timing == 1) {
                                attron(COLOR_PAIR(CP_WHITE));
                                mvprintw(row++, 4, "Good hit!");
                                attroff(COLOR_PAIR(CP_WHITE));
                            } else {
                                attron(COLOR_PAIR(CP_GRAY));
                                mvprintw(row++, 4, "Poor timing...");
                                attroff(COLOR_PAIR(CP_GRAY));
                            }

                            row++;
                            if (player_wins) {
                                int prize = prizes[round];
                                gs->gold += prize;
                                total_winnings += prize;
                                gs->xp += 10 + round * 10;
                                attron(COLOR_PAIR(CP_GREEN) | A_BOLD);
                                mvprintw(row++, 4, "You unhorse the %s! +%dg, +%d XP!",
                                         opponents[round], prize, 10 + round * 10);
                                attroff(COLOR_PAIR(CP_GREEN) | A_BOLD);

                                if (round == 2) {
                                    /* Won the championship! */
                                    gs->chivalry += 5;
                                    if (gs->chivalry > 100) gs->chivalry = 100;
                                    row++;
                                    attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
                                    mvprintw(row++, 4, "TOURNAMENT CHAMPION! +5 chivalry!");
                                    attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
                                }
                            } else {
                                int dmg = rng_range(3, 10);
                                gs->hp -= dmg;
                                if (gs->hp < 1) gs->hp = 1;
                                gs->chivalry -= 1;
                                if (gs->chivalry < 0) gs->chivalry = 0;
                                attron(COLOR_PAIR(CP_RED));
                                mvprintw(row++, 4, "The %s unhorses you! -%d HP, -1 chivalry.",
                                         opponents[round], dmg);
                                attroff(COLOR_PAIR(CP_RED));
                            }

                            row += 2;
                            attron(COLOR_PAIR(CP_GRAY));
                            mvprintw(row, 4, "Press any key to continue...");
                            attroff(COLOR_PAIR(CP_GRAY));
                            ui_refresh();
                            /* Wait a moment and flush input before waiting for key */
                            napms(500);
                            flushinp();
                            ui_getkey();

                            if (!player_wins) {
                                log_add(&gs->log, gs->turn, CP_RED,
                                         "Eliminated in round %d. Total winnings: %dg.", round + 1, total_winnings);
                                break;
                            }
                            if (round == 2) {
                                log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                                         "Tournament Champion! Total winnings: %dg, +5 chivalry!", total_winnings);
                            }
                        }
                    }
                }
            } else if (is_castle && already_jousted) {
                log_add(&gs->log, gs->turn, CP_WHITE,
                         "\"You've already competed today! Come back for the next tournament.\"");
            } else if (is_castle && gs->horse_type > 0 && !tournament_today) {
                /* Tell player when next tournament is */
                unsigned int ch = 0;
                if (gs->current_town) {
                    for (const char *p = gs->current_town->name; *p; p++)
                        ch = ch * 31 + (unsigned char)*p;
                }
                int moon = game_get_moon_day(gs);
                int days[4] = {
                    (int)(ch % 7) + 1, (int)((ch / 7) % 7) + 8,
                    (int)((ch / 49) % 7) + 15, (int)((ch / 343) % 7) + 22
                };
                int next_day = 99;
                for (int d = 0; d < 4; d++) {
                    if (days[d] > moon && days[d] < next_day) next_day = days[d];
                }
                if (next_day > 30) next_day = days[0]; /* wrap to next cycle */
                int wait = next_day - moon;
                if (wait <= 0) wait += 30;
                log_add(&gs->log, gs->turn, CP_WHITE,
                         "\"No tournament today. The next one is in %d day%s.\"",
                         wait, wait == 1 ? "" : "s");
            } else if (is_castle && gs->horse_type == 0) {
                log_add(&gs->log, gs->turn, CP_WHITE,
                         "\"The tournament requires a horse. Visit a stable first.\"");
            } else {
                const char *msgs[] = {
                    "The guard salutes. \"All is well, m'lord.\"",
                    "\"Move along. The King's peace must be kept.\"",
                    "\"I've heard bandits roam the northern roads.\"",
                    "\"Stay out of trouble within these walls.\"",
                    "\"The castle is secure. Rest easy.\"",
                };
                int n = sizeof(msgs) / sizeof(msgs[0]);
                log_add(&gs->log, gs->turn, CP_WHITE, "%s", msgs[rng_range(0, n - 1)]);
            }
        }
        break;
    case NPC_DOG:
        {
            const char *barks[] = {
                "The dog barks happily and wags its tail!",
                "The dog sniffs your hand and licks it.",
                "Woof! The dog rolls over for a belly rub.",
                "The dog follows you for a moment, then gets distracted.",
            };
            int n = sizeof(barks) / sizeof(barks[0]);
            log_add(&gs->log, gs->turn, CP_BROWN, "%s", barks[rng_range(0, n - 1)]);
        }
        break;
    case NPC_CAT:
        if (!gs->has_cat) {
            log_add(&gs->log, gs->turn, CP_YELLOW,
                     "The cat purrs and rubs against your leg. It wants to follow you!");
            log_add(&gs->log, gs->turn, CP_WHITE,
                     "The cat joins you! (+1 SPD, no rat encounters while it stays)");
            gs->has_cat = true;
            gs->cat_turns = rng_range(100, 300);
            gs->cat_pos = gs->player_pos;
            gs->spd += 1;
        } else {
            log_add(&gs->log, gs->turn, CP_YELLOW,
                     "You already have a cat following you. It meows jealously.");
        }
        break;
    case NPC_CHICKEN:
        {
            const char *clucks[] = {
                "Bawk! The chicken flutters away in a panic.",
                "The chicken pecks at the ground, ignoring you.",
                "Cluck cluck! A feather drifts to the ground.",
            };
            int n = sizeof(clucks) / sizeof(clucks[0]);
            log_add(&gs->log, gs->turn, CP_WHITE, "%s", clucks[rng_range(0, n - 1)]);
        }
        break;
    case NPC_GRAVEYARD:
        {
            FallenHero heroes[MAX_FALLEN];
            int hcount = fallen_load(heroes);

            ui_clear();
            int row = 1;
            attron(COLOR_PAIR(CP_GRAY) | A_BOLD);
            mvprintw(row++, 4, "=== Canterbury Graveyard ===");
            attroff(COLOR_PAIR(CP_GRAY) | A_BOLD);
            row++;

            if (hcount == 0) {
                attron(COLOR_PAIR(CP_GRAY));
                mvprintw(row++, 6, "The graveyard is empty. No heroes have fallen yet.");
                attroff(COLOR_PAIR(CP_GRAY));
            } else {
                attron(COLOR_PAIR(CP_WHITE));
                mvprintw(row++, 4, "Rows of gravestones mark the fallen...");
                attroff(COLOR_PAIR(CP_WHITE));
                row++;
                int term_rows_g, term_cols_g;
                ui_get_size(&term_rows_g, &term_cols_g);
                for (int i = 0; i < hcount && row < term_rows_g - 4; i++) {
                    attron(COLOR_PAIR(CP_GRAY));
                    mvprintw(row++, 6, "+---------------------------+");
                    mvprintw(row++, 6, "| Here lies %-16s |", heroes[i].name);
                    mvprintw(row++, 6, "| %-10s Level %-3d       |", heroes[i].class_name, heroes[i].level);
                    mvprintw(row++, 6, "| %-27s |", heroes[i].cause);
                    mvprintw(row++, 6, "+---------------------------+");
                    attroff(COLOR_PAIR(CP_GRAY));
                    row++;
                }
            }
            gs->chivalry++;
            if (gs->chivalry > 100) gs->chivalry = 100;
            row++;
            attron(COLOR_PAIR(CP_WHITE));
            mvprintw(row++, 4, "You pay your respects to the fallen. (+1 chivalry)");
            attroff(COLOR_PAIR(CP_WHITE));
            attron(COLOR_PAIR(CP_GRAY));
            mvprintw(row, 4, "Press any key to leave.");
            attroff(COLOR_PAIR(CP_GRAY));
            ui_refresh();
            ui_getkey();
        }
        break;
    case NPC_HOT_SPRINGS:
        {
            ui_clear();
            int row = 2;
            attron(COLOR_PAIR(CP_CYAN_BOLD) | A_BOLD);
            mvprintw(row++, 4, "=== Roman Hot Springs ===");
            attroff(COLOR_PAIR(CP_CYAN_BOLD) | A_BOLD);
            row++;
            attron(COLOR_PAIR(CP_CYAN));
            mvprintw(row++, 4, "Steam rises from the ancient stone pools.");
            mvprintw(row++, 4, "The warm mineral waters are said to cure all ills.");
            attroff(COLOR_PAIR(CP_CYAN));
            row++;
            attron(COLOR_PAIR(CP_WHITE));
            mvprintw(row++, 4, "[b] Bathe in the hot springs (free)");
            mvprintw(row++, 4, "[q] Leave");
            attroff(COLOR_PAIR(CP_WHITE));
            ui_refresh();

            int bk = ui_getkey();
            if (bk == 'b') {
                gs->hp = gs->max_hp;
                gs->mp = gs->max_mp;
                bool cured = false;
                if (gs->has_lycanthropy) {
                    gs->has_lycanthropy = false;
                    gs->str -= 3; if (gs->str < 1) gs->str = 1;
                    gs->spd -= 2; if (gs->spd < 1) gs->spd = 1;
                    log_add(&gs->log, gs->turn, CP_GREEN,
                             "The sacred waters wash away your lycanthropy!");
                    cured = true;
                }
                if (gs->has_vampirism) {
                    gs->has_vampirism = false;
                    gs->str -= 3; if (gs->str < 1) gs->str = 1;
                    gs->spd -= 2; if (gs->spd < 1) gs->spd = 1;
                    log_add(&gs->log, gs->turn, CP_GREEN,
                             "The blessed waters purge the vampirism from your blood!");
                    cured = true;
                }
                if (gs->str < 5) { gs->str = 5; cured = true; }
                if (gs->def < 5) { gs->def = 5; cured = true; }
                if (gs->intel < 5) { gs->intel = 5; cured = true; }
                if (gs->spd < 5) { gs->spd = 5; cured = true; }

                if (cured) {
                    log_add(&gs->log, gs->turn, CP_CYAN_BOLD,
                             "The hot springs cure your ailments! Full HP/MP, curses lifted.");
                } else {
                    log_add(&gs->log, gs->turn, CP_CYAN,
                             "The warm waters soothe you. Full HP and MP restored.");
                }
            }
        }
        break;
    default:
        break;
    }
}

static void town_do_inn_rest(GameState *gs) {
    const TownDef *td = gs->current_town;
    if (gs->gold < td->inn_cost) {
        log_add(&gs->log, gs->turn, CP_RED, "You cannot afford to rest here (%dg).", td->inn_cost);
        return;
    }
    gs->gold -= td->inn_cost;
    gs->hp = gs->max_hp;
    gs->mp = gs->max_mp;

    /* Advance to next morning */
    int old_hour = gs->hour;
    if (gs->hour >= 6) { gs->day++; }
    gs->hour = 6;
    gs->minute = 0;
    advance_time(gs, 0);
    check_lunar_events(gs, old_hour);

    log_add(&gs->log, gs->turn, CP_GREEN,
             "You rest at the inn. A warm bed and a good meal. HP and MP fully restored.");
}

static void town_do_inn_beer(GameState *gs) {
    const TownDef *td = gs->current_town;
    if (gs->gold < td->beer_cost) {
        log_add(&gs->log, gs->turn, CP_RED, "You can't afford a drink.");
        return;
    }
    gs->gold -= td->beer_cost;
    gs->beers_drunk++;

    switch (gs->beers_drunk) {
    case 1:
        log_add(&gs->log, gs->turn, CP_YELLOW,
                 "You drink a pint of %s. You feel merry!", td->beer_name);
        break;
    case 2:
        log_add(&gs->log, gs->turn, CP_YELLOW,
                 "Another %s! You feel tipsy...", td->beer_name);
        gs->drunk_turns = 30;
        break;
    case 3:
        log_add(&gs->log, gs->turn, CP_YELLOW,
                 "A third %s! You are drunk. The room sways.", td->beer_name);
        gs->drunk_turns = 50;
        break;
    case 4:
        log_add(&gs->log, gs->turn, CP_RED,
                 "Yet another %s! You are very drunk!", td->beer_name);
        gs->drunk_turns = 70;
        break;
    default:
        log_add(&gs->log, gs->turn, CP_RED,
                 "You pass out face-first into your %s...", td->beer_name);
        gs->drunk_turns = 100;
        gs->hp = gs->max_hp / 2;
        /* Advance 8 hours */
        gs->hour += 8;
        if (gs->hour >= 24) { gs->hour -= 24; gs->day++; }
        log_add(&gs->log, gs->turn, CP_GRAY,
                 "You wake up outside the inn with a splitting headache.");
        gs->beers_drunk = 0;
        break;
    }
}

static void town_do_inn(GameState *gs) {
    ui_clear();
    int row = 2;
    const TownDef *td = gs->current_town;

    attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
    mvprintw(row++, 2, "=== The Inn ===");
    attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
    row++;
    attron(COLOR_PAIR(CP_WHITE));
    mvprintw(row++, 4, "The innkeeper nods as you enter.");
    row++;
    mvprintw(row++, 4, "[r] Rest until morning (%dg)", td->inn_cost);
    mvprintw(row++, 4, "[n] Rest until nightfall (%dg)", td->inn_cost);
    mvprintw(row++, 4, "[b] Buy a %s (%dg)", td->beer_name, td->beer_cost);
    mvprintw(row++, 4, "[q] Leave the inn");
    row++;
    mvprintw(row++, 4, "Gold: %d  |  Beers: %d", gs->gold, gs->beers_drunk);
    attroff(COLOR_PAIR(CP_WHITE));
    ui_refresh();

    int key = ui_getkey();
    if (key == 'r') town_do_inn_rest(gs);
    else if (key == 'n') {
        /* Rest until nightfall */
        const TownDef *td2 = gs->current_town;
        if (gs->gold < td2->inn_cost) {
            log_add(&gs->log, gs->turn, CP_RED, "You cannot afford to rest here.");
            return;
        }
        gs->gold -= td2->inn_cost;
        gs->hp = gs->max_hp;
        gs->mp = gs->max_mp;
        int old_hour = gs->hour;
        if (gs->hour < 19) { gs->hour = 19; } else { gs->day++; gs->hour = 19; }
        gs->minute = 0;
        advance_time(gs, 0);
        check_lunar_events(gs, old_hour);
        log_add(&gs->log, gs->turn, CP_GREEN,
                 "You rest until nightfall. HP and MP fully restored.");
    }
    else if (key == 'b') town_do_inn_beer(gs);
}

static void town_do_church(GameState *gs) {
    if (gs->church_looted) {
        log_add(&gs->log, gs->turn, CP_RED,
                 "The priest bars the door. \"Begone, thief! You are not welcome here!\"");
        return;
    }
    if (gs->has_vampirism) {
        gs->hp -= 5;
        if (gs->hp < 1) gs->hp = 1;
        log_add(&gs->log, gs->turn, CP_RED,
                 "Holy ground burns you! -5 HP. The priest recoils in horror.");
        log_add(&gs->log, gs->turn, CP_YELLOW,
                 "\"Creature of the night! I can cure you for 50 gold. Press 'u'.\"");
        game_render(gs);
        int vk = ui_getkey();
        if (vk == 'u') {
            if (gs->gold >= 50) {
                gs->gold -= 50;
                gs->has_vampirism = false;
                gs->str -= 3; if (gs->str < 1) gs->str = 1;
                gs->spd -= 2; if (gs->spd < 1) gs->spd = 1;
                gs->hp -= 20;
                if (gs->hp < 1) gs->hp = 1;
                log_add(&gs->log, gs->turn, CP_GREEN,
                         "The priest performs a painful exorcism. Vampirism cured! (-20 HP, -50g)");
            } else {
                log_add(&gs->log, gs->turn, CP_RED, "You don't have enough gold. (Need 50g)");
            }
        }
        return;
    }

    ui_clear();
    int row = 2;

    attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
    mvprintw(row++, 2, "=== The Church ===");
    attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
    row++;
    attron(COLOR_PAIR(CP_WHITE));
    mvprintw(row++, 4, "A priest tends the altar. Candles flicker softly.");
    row++;
    mvprintw(row++, 4, "[p] Pray (restore some HP/MP)");
    mvprintw(row++, 4, "[d] Donate gold (+1 chivalry per 20g)");
    if (!gs->confessed && gs->chivalry < 50)
        mvprintw(row++, 4, "[c] Confession (+3 chivalry)");
    mvprintw(row++, 4, "[u] Cure poison/curses (30 gold donation)");
    attron(COLOR_PAIR(CP_RED));
    mvprintw(row++, 4, "[l] Loot the church (-12 chivalry!)");
    attroff(COLOR_PAIR(CP_RED));
    mvprintw(row++, 4, "[q] Leave the church");
    row++;
    mvprintw(row++, 4, "Gold: %d  |  Chivalry: %d", gs->gold, gs->chivalry);
    attroff(COLOR_PAIR(CP_WHITE));
    ui_refresh();

    int key = ui_getkey();
    if (key == 'p') {
        gs->hp += gs->max_hp / 4;
        if (gs->hp > gs->max_hp) gs->hp = gs->max_hp;
        gs->mp += gs->max_mp / 4;
        if (gs->mp > gs->max_mp) gs->mp = gs->max_mp;
        log_add(&gs->log, gs->turn, CP_WHITE,
                 "You kneel and pray. A sense of peace washes over you.");
    } else if (key == 'd') {
        if (gs->gold < 20) {
            log_add(&gs->log, gs->turn, CP_RED, "You don't have enough gold to donate.");
        } else {
            int donate = 20;
            gs->gold -= donate;
            gs->chivalry++;
            if (gs->chivalry > 100) gs->chivalry = 100;
            log_add(&gs->log, gs->turn, CP_YELLOW,
                     "The priest blesses you for your generosity. (+1 chivalry)");
        }
    } else if (key == 'c' && !gs->confessed && gs->chivalry < 50) {
        gs->chivalry += 3;
        if (gs->chivalry > 100) gs->chivalry = 100;
        gs->confessed = true;
        log_add(&gs->log, gs->turn, CP_WHITE,
                 "\"Your sins are forgiven. Go forth and do good.\" (+3 chivalry)");
    } else if (key == 'u') {
        /* Cure poison and restore debuffed stats */
        if (gs->gold < 30) {
            log_add(&gs->log, gs->turn, CP_RED,
                     "The priest requires a 30 gold donation for healing.");
        } else {
            gs->gold -= 30;
            /* Restore any stats that might have been debuffed below base */
            /* Simple approach: +1 to any stat below 5 */
            bool cured = false;
            if (gs->str < 5) { gs->str++; cured = true; }
            if (gs->def < 5) { gs->def++; cured = true; }
            if (gs->intel < 5) { gs->intel++; cured = true; }
            if (gs->spd < 5) { gs->spd++; cured = true; }
            gs->hp = gs->max_hp;
            gs->mp = gs->max_mp;
            /* Cure lycanthropy */
            if (gs->has_lycanthropy) {
                gs->has_lycanthropy = false;
                gs->str -= 3; if (gs->str < 1) gs->str = 1;
                gs->spd -= 2; if (gs->spd < 1) gs->spd = 1;
                log_add(&gs->log, gs->turn, CP_GREEN,
                         "The priest performs a holy ritual. Lycanthropy cured!");
                cured = true;
            }
            if (cured) {
                log_add(&gs->log, gs->turn, CP_GREEN,
                         "The priest lays hands on you. Curses lifted! Stats restored. Full HP/MP.");
            } else {
                log_add(&gs->log, gs->turn, CP_GREEN,
                         "The priest blesses you. Full HP and MP restored.");
            }
            gs->chivalry++;
            if (gs->chivalry > 100) gs->chivalry = 100;
        }
    } else if (key == 'l') {
        int loot = rng_range(50, 100);
        gs->gold += loot;
        gs->chivalry -= 12;
        if (gs->chivalry < 0) gs->chivalry = 0;
        log_add(&gs->log, gs->turn, CP_RED,
                 "You steal %d gold from the collection plate! SACRILEGE! (-12 chivalry)", loot);
        log_add(&gs->log, gs->turn, CP_RED,
                 "The priest cries out in horror. You are no longer welcome here.");
        gs->church_looted = true;
    }
}

static void town_do_mystic(GameState *gs) {
    if (gs->gold < 5) {
        log_add(&gs->log, gs->turn, CP_RED,
                 "The mystic demands 5 gold. You cannot afford it.");
        return;
    }
    gs->gold -= 5;

    /* Random stat change */
    int *stats[] = { &gs->str, &gs->def, &gs->intel, &gs->spd };
    int stat_idx = rng_range(0, 3);
    bool good = rng_chance(60);  /* 60% positive */

    if (good) {
        (*stats[stat_idx])++;
        const char *msgs[] = {
            "\"The stars favour your strength...\" (+1 STR)",
            "\"Your shield shall not falter...\" (+1 DEF)",
            "\"Wisdom flows through you...\" (+1 INT)",
            "\"The wind is at your back...\" (+1 SPD)"
        };
        log_add(&gs->log, gs->turn, CP_MAGENTA, "%s", msgs[stat_idx]);
    } else {
        if (*stats[stat_idx] > 1) (*stats[stat_idx])--;
        const char *msgs[] = {
            "\"A dark cloud hangs over your might...\" (-1 STR)",
            "\"Your guard weakens...\" (-1 DEF)",
            "\"A fog clouds your mind...\" (-1 INT)",
            "\"Your steps grow heavy...\" (-1 SPD)"
        };
        log_add(&gs->log, gs->turn, CP_MAGENTA, "%s", msgs[stat_idx]);
    }
}

static void town_do_bank(GameState *gs) {
    ui_clear();
    int row = 2;

    /* TODO: load bank balance from ~/.camelot/bank.dat for persistence */
    static int bank_balance = 0;  /* placeholder until save system */

    attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
    mvprintw(row++, 2, "=== The Bank ===");
    attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
    row++;
    attron(COLOR_PAIR(CP_WHITE));
    mvprintw(row++, 4, "The banker greets you. \"How may I help?\"");
    row++;
    mvprintw(row++, 4, "Bank balance: %d gold", bank_balance);
    mvprintw(row++, 4, "Your gold:    %d", gs->gold);
    row++;
    mvprintw(row++, 4, "[d] Deposit 50 gold (5%% fee)");
    mvprintw(row++, 4, "[D] Deposit all gold (5%% fee)");
    mvprintw(row++, 4, "[w] Withdraw 50 gold");
    mvprintw(row++, 4, "[W] Withdraw all gold");
    mvprintw(row++, 4, "[q] Leave the bank");
    attroff(COLOR_PAIR(CP_WHITE));
    ui_refresh();

    int key = ui_getkey();
    if (key == 'd') {
        int amount = 50;
        if (gs->gold < amount) amount = gs->gold;
        if (amount <= 0) { log_add(&gs->log, gs->turn, CP_RED, "You have no gold to deposit."); return; }
        int fee = amount * 5 / 100;
        if (fee < 1) fee = 1;
        gs->gold -= amount;
        bank_balance += (amount - fee);
        log_add(&gs->log, gs->turn, CP_YELLOW,
                 "Deposited %d gold (fee: %d). Bank balance: %d", amount, fee, bank_balance);
    } else if (key == 'D') {
        int amount = gs->gold;
        if (amount <= 0) { log_add(&gs->log, gs->turn, CP_RED, "You have no gold to deposit."); return; }
        int fee = amount * 5 / 100;
        if (fee < 1) fee = 1;
        gs->gold = 0;
        bank_balance += (amount - fee);
        log_add(&gs->log, gs->turn, CP_YELLOW,
                 "Deposited %d gold (fee: %d). Bank balance: %d", amount, fee, bank_balance);
    } else if (key == 'w') {
        int amount = 50;
        if (bank_balance < amount) amount = bank_balance;
        if (amount <= 0) { log_add(&gs->log, gs->turn, CP_RED, "No gold in the bank."); return; }
        bank_balance -= amount;
        gs->gold += amount;
        log_add(&gs->log, gs->turn, CP_YELLOW,
                 "Withdrew %d gold. Bank balance: %d", amount, bank_balance);
    } else if (key == 'W') {
        if (bank_balance <= 0) { log_add(&gs->log, gs->turn, CP_RED, "No gold in the bank."); return; }
        gs->gold += bank_balance;
        log_add(&gs->log, gs->turn, CP_YELLOW,
                 "Withdrew %d gold. Bank balance: 0", bank_balance);
        bank_balance = 0;
    }
}

static void town_do_well(GameState *gs) {
    if (gs->well_explored) {
        log_add(&gs->log, gs->turn, CP_GRAY, "You already explored the well today.");
        return;
    }
    gs->well_explored = true;

    /* Determine what's in the well */
    int roll = rng_range(1, 100);
    bool has_treasure = (roll <= 40);
    bool has_rat = (roll > 40 && roll <= 65);
    (void)(roll); /* empty case handled by default */
    int gold_found = has_treasure ? rng_range(8, 40) : (has_rat ? rng_range(3, 12) : 0);

    /* Draw the well interior */
    ui_clear();
    int term_rows, term_cols;
    ui_get_size(&term_rows, &term_cols);

    /* Well map: 15 wide x 11 tall, centered */
    #define WELL_W 15
    #define WELL_H 11
    int ox = (term_cols - WELL_W) / 2;
    int oy = 2;

    /* Title */
    attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
    mvprintw(oy - 1, ox, "Bottom of the Well");
    attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);

    /* Draw circular well with walls, floor, and water puddles */
    for (int wy = 0; wy < WELL_H; wy++) {
        for (int wx = 0; wx < WELL_W; wx++) {
            int dx = wx - WELL_W / 2;
            int dy = wy - WELL_H / 2;
            double dist = (double)(dx * dx) / 25.0 + (double)(dy * dy) / 16.0;

            int sx = ox + wx, sy = oy + wy;

            if (dist > 1.1) {
                /* Outside -- dark stone */
                attron(COLOR_PAIR(CP_GRAY));
                mvaddch(sy, sx, '#');
                attroff(COLOR_PAIR(CP_GRAY));
            } else if (dist > 0.9) {
                /* Wall edge */
                attron(COLOR_PAIR(CP_BROWN));
                mvaddch(sy, sx, '#');
                attroff(COLOR_PAIR(CP_BROWN));
            } else {
                /* Floor -- damp stone with occasional water */
                int h = (wx * 31 + wy * 17) & 7;
                if (h == 0) {
                    attron(COLOR_PAIR(CP_BLUE));
                    mvaddch(sy, sx, '~');
                    attroff(COLOR_PAIR(CP_BLUE));
                } else {
                    attron(COLOR_PAIR(CP_GRAY));
                    mvaddch(sy, sx, '.');
                    attroff(COLOR_PAIR(CP_GRAY));
                }
            }
        }
    }

    /* Ladder coming down from above */
    int ladder_x = ox + WELL_W / 2;
    attron(COLOR_PAIR(CP_BROWN) | A_BOLD);
    mvaddch(oy, ladder_x, 'H');
    mvaddch(oy + 1, ladder_x, 'H');
    attroff(COLOR_PAIR(CP_BROWN) | A_BOLD);

    /* Player at bottom of ladder */
    attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
    mvaddch(oy + 2, ladder_x, '@');
    attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);

    /* Chest in the well */
    if (has_treasure || has_rat) {
        attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
        mvaddch(oy + WELL_H / 2 + 1, ox + WELL_W / 2 - 2, '=');
        attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);

        attron(COLOR_PAIR(CP_WHITE));
        mvprintw(oy + WELL_H / 2 + 1, ox + WELL_W / 2, " Chest");
        attroff(COLOR_PAIR(CP_WHITE));
    }

    /* Rat if present */
    if (has_rat) {
        attron(COLOR_PAIR(CP_RED) | A_BOLD);
        mvaddch(oy + WELL_H / 2, ox + WELL_W / 2 + 2, 'r');
        attroff(COLOR_PAIR(CP_RED) | A_BOLD);

        attron(COLOR_PAIR(CP_RED));
        mvprintw(oy + WELL_H / 2, ox + WELL_W / 2 + 4, "Rat!");
        attroff(COLOR_PAIR(CP_RED));
    }

    /* Description text */
    int text_y = oy + WELL_H + 1;
    attron(COLOR_PAIR(CP_WHITE));
    mvprintw(text_y++, ox - 5, "You climb down the slippery ladder...");
    mvprintw(text_y++, ox - 5, "The air is cold and damp. Water drips from above.");
    text_y++;

    if (has_treasure) {
        attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
        mvprintw(text_y++, ox - 5, "You find a chest! Inside: %d gold coins!", gold_found);
        attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
    } else if (has_rat) {
        attron(COLOR_PAIR(CP_RED));
        mvprintw(text_y++, ox - 5, "A rat lunges at you from the shadows!");
        attroff(COLOR_PAIR(CP_RED));
        int damage = rng_range(2, 6);
        gs->hp -= damage;
        if (gs->hp < 1) gs->hp = 1;
        attron(COLOR_PAIR(CP_RED));
        mvprintw(text_y++, ox - 5, "The rat bites you for %d damage!", damage);
        attroff(COLOR_PAIR(CP_RED));
        if (gold_found > 0) {
            attron(COLOR_PAIR(CP_YELLOW));
            mvprintw(text_y++, ox - 5, "You find %d gold in the chest after driving it off.", gold_found);
            attroff(COLOR_PAIR(CP_YELLOW));
        }
    } else {
        attron(COLOR_PAIR(CP_GRAY));
        mvprintw(text_y++, ox - 5, "The well is dry. Nothing but damp stones and silence.");
        attroff(COLOR_PAIR(CP_GRAY));
    }

    text_y++;
    mvprintw(text_y, ox - 5, "Press any key to climb back up...");
    attroff(COLOR_PAIR(CP_WHITE));

    ui_refresh();
    ui_getkey();

    /* Apply rewards */
    gs->gold += gold_found;
    if (has_treasure) {
        log_add(&gs->log, gs->turn, CP_YELLOW,
                 "Found %d gold in the well!", gold_found);
    } else if (has_rat) {
        log_add(&gs->log, gs->turn, CP_RED,
                 "Bitten by a rat in the well! Found %d gold.", gold_found);
    } else {
        log_add(&gs->log, gs->turn, CP_GRAY,
                 "The well was empty. Nothing but damp stones.");
    }
}

static void handle_town_input(GameState *gs, int key) {
    TownMap *tm = &gs->town_map;

    Direction dir = key_to_direction(key);
    if (dir != DIR_NONE) {
        int nx = gs->town_player_pos.x + dir_dx[dir];
        int ny = gs->town_player_pos.y + dir_dy[dir];

        if (nx < 0 || nx >= TOWN_MAP_W || ny < 0 || ny >= TOWN_MAP_H) return;

        /* Check for NPC bump */
        TownNPC *npc = town_npc_at(tm, nx, ny);
        if (npc) {
            town_interact_npc(gs, npc);
            advance_time(gs, 1);
            return;
        }

        /* Check for exit -- only through the gate (open door on bottom wall) */
        if (ny == TOWN_MAP_H - 1 && tm->map[ny][nx].type == TILE_DOOR_OPEN) {
            gs->mode = MODE_OVERWORLD;
            gs->current_town = NULL;
            gs->beers_drunk = 0;
            gs->well_explored = false;
            gs->confessed = false;
            log_add(&gs->log, gs->turn, CP_WHITE, "You leave through the town gate.");
            return;
        }

        Tile *target = &tm->map[ny][nx];
        if (target->passable) {
            gs->town_player_pos.x = nx;
            gs->town_player_pos.y = ny;
            advance_time(gs, 1);
            town_move_npcs(tm, gs->town_player_pos);  /* NPCs wander each turn */
        }
        return;
    }

    /* Player Home storage chest (V key in Camelot) */
    if (key == 'V' && gs->current_town && strcmp(gs->current_town->name, "Camelot") == 0) {
        StoredItem chest[MAX_STORED_ITEMS];
        int chest_count = home_chest_load(chest);

        ui_clear();
        int row = 1;
        attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
        mvprintw(row++, 4, "=== Your Home -- Storage Chest ===");
        attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
        row++;
        attron(COLOR_PAIR(CP_WHITE));
        mvprintw(row++, 4, "[s] Stash an item    [t] Take an item    [r] Rest until morning    [q] Leave");
        attroff(COLOR_PAIR(CP_WHITE));
        row++;

        if (chest_count == 0) {
            attron(COLOR_PAIR(CP_GRAY));
            mvprintw(row++, 6, "The chest is empty.");
            attroff(COLOR_PAIR(CP_GRAY));
        } else {
            attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
            mvprintw(row++, 4, "Chest contents (%d items):", chest_count);
            attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
            int term_rows_h, term_cols_h;
            ui_get_size(&term_rows_h, &term_cols_h);
            for (int i = 0; i < chest_count && row < term_rows_h - 3; i++) {
                attron(COLOR_PAIR(chest[i].color_pair));
                mvprintw(row++, 6, "%c) %s (pow:%d val:%dg) -- left by %s, day %d",
                         'a' + i, chest[i].name, chest[i].power, chest[i].value,
                         chest[i].left_by, chest[i].day_stored);
                attroff(COLOR_PAIR(chest[i].color_pair));
            }
        }
        ui_refresh();

        int hk = ui_getkey();
        if (hk == 's' && gs->num_items > 0) {
            /* Stash an item */
            if (chest_count >= MAX_STORED_ITEMS) {
                log_add(&gs->log, gs->turn, CP_RED, "The chest is full! (max %d items)", MAX_STORED_ITEMS);
            } else {
                log_add(&gs->log, gs->turn, CP_WHITE, "Select an item to stash (a-z):");
                game_render(gs);
                int sk = ui_getkey();
                if (sk >= 'a' && sk < 'a' + gs->num_items) {
                    int idx = sk - 'a';
                    Item *it = &gs->inventory[idx];
                    home_chest_add(it->name, gs->player_name, gs->day,
                                   it->type, it->power, it->value, it->weight,
                                   it->glyph, it->color_pair);
                    log_add(&gs->log, gs->turn, CP_CYAN,
                             "Stashed %s in the chest.", it->name);
                    for (int j = idx; j < gs->num_items - 1; j++)
                        gs->inventory[j] = gs->inventory[j + 1];
                    gs->inventory[--gs->num_items].template_id = -1;
                }
            }
        } else if (hk == 't' && chest_count > 0) {
            /* Take an item */
            if (gs->num_items >= MAX_INVENTORY) {
                log_add(&gs->log, gs->turn, CP_RED, "Your inventory is full!");
            } else {
                log_add(&gs->log, gs->turn, CP_WHITE, "Select an item to take (a-z):");
                game_render(gs);
                int tk = ui_getkey();
                if (tk >= 'a' && tk < 'a' + chest_count) {
                    int idx = tk - 'a';
                    /* Create item from stored data */
                    Item taken;
                    memset(&taken, 0, sizeof(taken));
                    taken.template_id = -7; /* special: from chest */
                    snprintf(taken.name, sizeof(taken.name), "%s", chest[idx].name);
                    taken.glyph = chest[idx].glyph;
                    taken.color_pair = chest[idx].color_pair;
                    taken.type = chest[idx].type;
                    taken.power = chest[idx].power;
                    taken.value = chest[idx].value;
                    taken.weight = chest[idx].weight;
                    taken.on_ground = false;
                    gs->inventory[gs->num_items++] = taken;
                    log_add(&gs->log, gs->turn, CP_CYAN,
                             "Took %s from the chest.", chest[idx].name);
                    /* Remove from chest */
                    for (int j = idx; j < chest_count - 1; j++)
                        chest[j] = chest[j + 1];
                    chest_count--;
                    home_chest_save(chest, chest_count);
                }
            }
        } else if (hk == 'r') {
            /* Rest at home -- free, advance to morning */
            gs->hp = gs->max_hp;
            gs->mp = gs->max_mp;
            gs->hour = 7; gs->minute = 0;
            log_add(&gs->log, gs->turn, CP_GREEN,
                     "You rest in your own bed. Full HP/MP restored. Good morning!");
        }
        return;
    }

    /* Leave town explicitly */
    if (key == 'q' || key == 'Q') {
        gs->mode = MODE_OVERWORLD;
        gs->current_town = NULL;
        gs->beers_drunk = 0;
        gs->well_explored = false;
        gs->confessed = false;
        if (gs->horse_type > 0) {
            gs->riding = true;
            log_add(&gs->log, gs->turn, CP_WHITE, "You leave the town and mount your horse.");
        } else {
            log_add(&gs->log, gs->turn, CP_WHITE, "You leave the town.");
        }
        return;
    }
}

/* ------------------------------------------------------------------ */
/* Dungeon input                                                       */
/* ------------------------------------------------------------------ */

static void handle_dungeon_input(GameState *gs, int key) {
    Tile (*tiles)[MAP_WIDTH] = current_dungeon_tiles(gs);
    if (!tiles) return;

    /* Ascend stairs */
    if (key == '<') {
        Tile *t = &tiles[gs->player_pos.y][gs->player_pos.x];
        if (t->type == TILE_STAIRS_UP) {
            if (gs->dungeon->current_level == 0) {
                /* Return to overworld */
                gs->player_pos = gs->ow_player_pos;
                gs->mode = MODE_OVERWORLD;
                if (gs->horse_type > 0) {
                    gs->riding = true;
                    log_add(&gs->log, gs->turn, CP_WHITE,
                             "You climb back to the surface and mount your horse.");
                } else {
                    log_add(&gs->log, gs->turn, CP_WHITE,
                             "You climb back to the surface.");
                }
                dungeon_free(gs->dungeon);
                gs->dungeon = NULL;
            } else {
                /* Figure out which stair set we're on (0 or 1) */
                DungeonLevel *cur = current_dungeon_level(gs);
                int stair_idx = 0;
                for (int si = 0; si < cur->num_stairs_up; si++) {
                    if (cur->stairs_up[si].x == gs->player_pos.x &&
                        cur->stairs_up[si].y == gs->player_pos.y) {
                        stair_idx = si;
                        break;
                    }
                }
                /* Go up one level, land at matching stairs down */
                gs->dungeon->current_level--;
                DungeonLevel *upper = current_dungeon_level(gs);
                if (stair_idx < upper->num_stairs_down)
                    gs->player_pos = upper->stairs_down[stair_idx];
                else
                    gs->player_pos = upper->stairs_down[0];
                log_add(&gs->log, gs->turn, CP_WHITE,
                         "You ascend to level %d.", gs->dungeon->current_level + 1);
                dungeon_update_fov(gs);
            }
            return;
        }
        log_add(&gs->log, gs->turn, CP_GRAY, "There are no stairs up here.");
        return;
    }

    /* Descend stairs */
    if (key == '>') {
        Tile *t = &tiles[gs->player_pos.y][gs->player_pos.x];
        if (t->type == TILE_STAIRS_DOWN) {
            /* Figure out which stair set we're on */
            DungeonLevel *cur = current_dungeon_level(gs);
            int stair_idx = 0;
            for (int si = 0; si < cur->num_stairs_down; si++) {
                if (cur->stairs_down[si].x == gs->player_pos.x &&
                    cur->stairs_down[si].y == gs->player_pos.y) {
                    stair_idx = si;
                    break;
                }
            }

            int next = gs->dungeon->current_level + 1;
            if (next >= gs->dungeon->max_depth) {
                log_add(&gs->log, gs->turn, CP_GRAY, "There's nothing below.");
                return;
            }
            gs->dungeon->current_level = next;
            DungeonLevel *dl = current_dungeon_level(gs);
            if (!dl->generated) {
                map_generate(dl, next, gs->dungeon->max_depth);
                entity_spawn_monsters(dl->monsters, &dl->num_monsters,
                                       dl->tiles, next, dl->stairs_up[0]);
                dungeon_spawn_items(dl);
                spawn_dungeon_boss(gs, dl, gs->dungeon->name, next, gs->dungeon->max_depth);
            }
            /* Land at matching stairs up */
            Vec2 landing = (stair_idx < dl->num_stairs_up) ?
                           dl->stairs_up[stair_idx] : dl->stairs_up[0];
            gs->player_pos = landing;
            /* Try adjacent floor */
            for (int d = 0; d < 8; d++) {
                int px = landing.x + dir_dx[d];
                int py = landing.y + dir_dy[d];
                if (px > 0 && px < MAP_WIDTH - 1 && py > 0 && py < MAP_HEIGHT - 1 &&
                    dl->tiles[py][px].type == TILE_FLOOR) {
                    gs->player_pos = (Vec2){ px, py };
                    break;
                }
            }
            log_add(&gs->log, gs->turn, CP_WHITE,
                     "You descend to level %d of %d.", next + 1, gs->dungeon->max_depth);
            dungeon_update_fov(gs);
            return;
        } else if (t->type == TILE_PORTAL) {
            /* Exit portal -- return to overworld */
            gs->player_pos = gs->ow_player_pos;
            gs->mode = MODE_OVERWORLD;
            log_add(&gs->log, gs->turn, CP_CYAN_BOLD,
                     "The portal flashes! You are transported back to the surface.");
            dungeon_free(gs->dungeon);
            gs->dungeon = NULL;
            return;
        }
        log_add(&gs->log, gs->turn, CP_GRAY, "There are no stairs down here.");
        return;
    }

    /* Open door */
    if (key == 'o') {
        log_add(&gs->log, gs->turn, CP_WHITE, "Open in which direction?");
        int dkey = ui_getkey();
        Direction dir = key_to_direction(dkey);
        if (dir != DIR_NONE) {
            int dx = gs->player_pos.x + dir_dx[dir];
            int dy = gs->player_pos.y + dir_dy[dir];
            if (dx > 0 && dx < MAP_WIDTH && dy > 0 && dy < MAP_HEIGHT) {
                Tile *door = &tiles[dy][dx];
                if (door->type == TILE_DOOR_CLOSED) {
                    door->type = TILE_DOOR_OPEN;
                    door->glyph = '/';
                    door->blocks_sight = false;
                    door->passable = true;
                    log_add(&gs->log, gs->turn, CP_WHITE, "You open the door.");
                    dungeon_update_fov(gs);
                } else if (door->type == TILE_DOOR_LOCKED) {
                    log_add(&gs->log, gs->turn, CP_RED,
                             "The door is locked! Try bashing it (walk into it) or find a key.");
                } else {
                    log_add(&gs->log, gs->turn, CP_GRAY, "There's no door there.");
                }
            }
        }
        return;
    }

    /* Close door */
    if (key == 'c') {
        log_add(&gs->log, gs->turn, CP_WHITE, "Close in which direction?");
        int dkey = ui_getkey();
        Direction dir = key_to_direction(dkey);
        if (dir != DIR_NONE) {
            int dx = gs->player_pos.x + dir_dx[dir];
            int dy = gs->player_pos.y + dir_dy[dir];
            if (dx > 0 && dx < MAP_WIDTH && dy > 0 && dy < MAP_HEIGHT) {
                Tile *door = &tiles[dy][dx];
                if (door->type == TILE_DOOR_OPEN) {
                    door->type = TILE_DOOR_CLOSED;
                    door->glyph = '+';
                    door->blocks_sight = true;
                    log_add(&gs->log, gs->turn, CP_WHITE, "You close the door.");
                    dungeon_update_fov(gs);
                } else {
                    log_add(&gs->log, gs->turn, CP_GRAY, "There's no open door there.");
                }
            }
        }
        return;
    }

    /* Search for secret doors */
    if (key == 's') {
        log_add(&gs->log, gs->turn, CP_WHITE, "You search the walls...");
        bool found_any = false;
        for (int d = 0; d < 8; d++) {
            int sx = gs->player_pos.x + dir_dx[d];
            int sy = gs->player_pos.y + dir_dy[d];
            if (sx < 1 || sx >= MAP_WIDTH - 1 || sy < 1 || sy >= MAP_HEIGHT - 1) continue;
            if (tiles[sy][sx].type != TILE_WALL) continue;

            /* Only reveal a door if there's a floor tile on the OTHER side */
            /* Check the tile beyond this wall (opposite direction from player) */
            int bx = sx + (sx - gs->player_pos.x);
            int by = sy + (sy - gs->player_pos.y);
            if (bx < 1 || bx >= MAP_WIDTH - 1 || by < 1 || by >= MAP_HEIGHT - 1) continue;

            bool floor_beyond = (tiles[by][bx].type == TILE_FLOOR);
            if (floor_beyond && rng_chance(15)) {
                tiles[sy][sx].type = TILE_DOOR_CLOSED;
                tiles[sy][sx].glyph = '+';
                tiles[sy][sx].color_pair = CP_BROWN;
                tiles[sy][sx].passable = false;
                log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                         "You find a hidden door!");
                found_any = true;
            }
        }
        if (!found_any) {
            log_add(&gs->log, gs->turn, CP_GRAY,
                     "You find nothing unusual.");
        }
        advance_time(gs, 1);
        return;
    }

    /* Pickup item (g key) */
    if (key == 'g') {
        DungeonLevel *dl = current_dungeon_level(gs);
        if (!dl) return;
        for (int i = 0; i < dl->num_ground_items; i++) {
            Item *it = &dl->ground_items[i];
            if (!it->on_ground) continue;
            if (it->pos.x != gs->player_pos.x || it->pos.y != gs->player_pos.y) continue;

            /* Gold goes straight to wallet */
            if (it->type == ITYPE_GOLD) {
                gs->gold += it->value;
                it->on_ground = false;
                log_add(&gs->log, gs->turn, CP_YELLOW,
                         "You pick up %d gold coins!", it->value);
                advance_time(gs, 1);
                return;
            }

            if (gs->num_items >= MAX_INVENTORY) {
                log_add(&gs->log, gs->turn, CP_RED, "Your inventory is full!");
                return;
            }
            /* Special: Holy Grail */
            if (it->type == ITYPE_QUEST && strcmp(it->name, "The Holy Grail") == 0) {
                gs->has_grail = true;
                it->on_ground = false;
                log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                         "*** You have found THE HOLY GRAIL! ***");
                log_add(&gs->log, gs->turn, CP_WHITE,
                         "Return it to King Arthur at Camelot Castle!");
                advance_time(gs, 1);
                return;
            }

            /* Add to inventory */
            gs->inventory[gs->num_items] = *it;
            gs->inventory[gs->num_items].on_ground = false;
            gs->inventory[gs->num_items].pos = (Vec2){ -1, -1 };
            gs->num_items++;
            it->on_ground = false;
            log_add(&gs->log, gs->turn, CP_CYAN,
                     "You pick up: %s", it->name);
            advance_time(gs, 1);
            return;
        }
        log_add(&gs->log, gs->turn, CP_GRAY, "There is nothing here to pick up.");
        return;
    }

    /* Rest to recover HP (R key) */
    if (key == 'R') {
        if (gs->hp >= gs->max_hp) {
            log_add(&gs->log, gs->turn, CP_GRAY, "You are already at full health.");
            return;
        }

        log_add(&gs->log, gs->turn, CP_WHITE, "You rest...");
        DungeonLevel *dl = current_dungeon_level(gs);
        int rested = 0;
        bool interrupted = false;

        while (gs->hp < gs->max_hp && rested < 100) {
            advance_time(gs, 1);
            rested++;

            /* Recover 1 HP every 5 turns resting */
            if (rested % 5 == 0) {
                gs->hp++;
                if (gs->hp > gs->max_hp) gs->hp = gs->max_hp;
            }

            /* Recover 1 MP every 8 turns resting */
            if (rested % 8 == 0) {
                gs->mp++;
                if (gs->mp > gs->max_mp) gs->mp = gs->max_mp;
            }

            /* Monster turns happen while resting */
            if (dl) dungeon_enemy_turns(gs);

            /* Check if any monster is now adjacent -- interrupts rest */
            if (dl) {
                for (int mi = 0; mi < dl->num_monsters; mi++) {
                    Entity *e = &dl->monsters[mi];
                    if (!e->alive) continue;
                    int dx = abs(e->pos.x - gs->player_pos.x);
                    int dy = abs(e->pos.y - gs->player_pos.y);
                    if (dx <= 1 && dy <= 1) {
                        log_add(&gs->log, gs->turn, CP_RED,
                                 "Your rest is interrupted by a %s!", e->name);
                        interrupted = true;
                        break;
                    }
                }
            }
            if (interrupted) break;

            /* Random chance of monster appearing (3% per turn) */
            if (dl && rng_chance(3)) {
                int radius = (gs->has_torch || gs->has_vampirism) ? FOV_RADIUS : 2;
                if (entity_spawn_one(dl->monsters, &dl->num_monsters,
                                      dl->tiles, dl->depth, gs->player_pos, radius)) {
                    log_add(&gs->log, gs->turn, CP_YELLOW,
                             "You hear something stirring in the darkness...");
                }
            }
        }

        if (!interrupted) {
            log_add(&gs->log, gs->turn, CP_GREEN,
                     "You finish resting. HP: %d/%d MP: %d/%d (%d turns)",
                     gs->hp, gs->max_hp, gs->mp, gs->max_mp, rested);
        }
        dungeon_update_fov(gs);
        return;
    }

    /* Toggle light source (T key) */
    if (key == 'T') {
        if (gs->has_torch) {
            /* Extinguish current light */
            gs->has_torch = false;
            const char *lname = gs->torch_type == 2 ? "lantern" : "torch";
            log_add(&gs->log, gs->turn, CP_GRAY,
                     "You extinguish your %s. Darkness closes in... (%d fuel left)",
                     lname, gs->torch_fuel);
        } else {
            /* Try to light something -- search inventory for torch, lantern, or oil */
            /* First check if we have fuel remaining from a previous light */
            if (gs->torch_fuel > 0) {
                gs->has_torch = true;
                const char *lname = gs->torch_type == 2 ? "lantern" : "torch";
                log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                         "You relight your %s. (%d turns of fuel)", lname, gs->torch_fuel);
            } else {
                /* Search inventory for a light source */
                int found_idx = -1, found_type = 0, found_fuel = 0;
                for (int ii = 0; ii < gs->num_items; ii++) {
                    if (strcmp(gs->inventory[ii].name, "Lantern") == 0) {
                        found_idx = ii; found_type = 2; found_fuel = gs->inventory[ii].power; break;
                    }
                }
                if (found_idx < 0) {
                    for (int ii = 0; ii < gs->num_items; ii++) {
                        if (strcmp(gs->inventory[ii].name, "Torch") == 0) {
                            found_idx = ii; found_type = 1; found_fuel = gs->inventory[ii].power; break;
                        }
                    }
                }
                /* Check for oil flask to refuel an existing lantern */
                if (found_idx < 0 && gs->torch_type == 2) {
                    for (int ii = 0; ii < gs->num_items; ii++) {
                        if (strcmp(gs->inventory[ii].name, "Oil Flask") == 0) {
                            gs->torch_fuel += gs->inventory[ii].power;
                            gs->has_torch = true;
                            log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                                     "You refuel your lantern with oil. (%d turns of fuel)", gs->torch_fuel);
                            /* Consume oil flask */
                            for (int j = ii; j < gs->num_items - 1; j++)
                                gs->inventory[j] = gs->inventory[j + 1];
                            gs->inventory[--gs->num_items].template_id = -1;
                            found_idx = -2; /* mark as handled */
                            break;
                        }
                    }
                }

                if (found_idx >= 0) {
                    gs->has_torch = true;
                    gs->torch_type = found_type;
                    gs->torch_fuel = found_fuel;
                    const char *lname = found_type == 2 ? "Lantern" : "Torch";
                    log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                             "You light the %s. (%d turns of fuel)", lname, found_fuel);
                    /* Consume the item from inventory */
                    for (int j = found_idx; j < gs->num_items - 1; j++)
                        gs->inventory[j] = gs->inventory[j + 1];
                    gs->inventory[--gs->num_items].template_id = -1;
                } else if (found_idx != -2) {
                    log_add(&gs->log, gs->turn, CP_GRAY,
                             "You have no torch, lantern, or oil to light.");
                }
            }
        }
        dungeon_update_fov(gs);
        return;
    }

    /* Disarm trap */
    if (key == 'D') {
        DungeonLevel *dl = current_dungeon_level(gs);
        if (!dl) return;
        bool found = false;
        for (int i = 0; i < dl->num_traps; i++) {
            Trap *t = &dl->traps[i];
            if (t->triggered || !t->revealed) continue;
            int dx = abs(t->pos.x - gs->player_pos.x);
            int dy = abs(t->pos.y - gs->player_pos.y);
            if (dx <= 1 && dy <= 1) {
                /* Attempt disarm: INT + SPD check */
                int skill = gs->intel + gs->spd;
                int difficulty = rng_range(5, 15);
                if (skill >= difficulty) {
                    t->triggered = true;
                    dl->tiles[t->pos.y][t->pos.x].glyph = '.';
                    dl->tiles[t->pos.y][t->pos.x].color_pair = CP_WHITE;
                    log_add(&gs->log, gs->turn, CP_GREEN,
                             "You carefully disarm the trap!");
                } else {
                    log_add(&gs->log, gs->turn, CP_RED,
                             "You fail to disarm the trap! It triggers!");
                    t->triggered = true;
                    /* Trigger the trap damage */
                    int dmg = rng_range(3, 8);
                    gs->hp -= dmg;
                    if (gs->hp < 1) gs->hp = 1;
                    log_add(&gs->log, gs->turn, CP_RED,
                             "The trap goes off! -%d HP", dmg);
                }
                found = true;
                break;
            }
        }
        if (!found) {
            log_add(&gs->log, gs->turn, CP_GRAY,
                     "There are no visible traps nearby to disarm.");
        }
        advance_time(gs, 1);
        return;
    }

    /* Movement */
    Direction dir = key_to_direction(key);
    if (dir != DIR_NONE) {
        int nx = gs->player_pos.x + dir_dx[dir];
        int ny = gs->player_pos.y + dir_dy[dir];

        if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT) {
            /* Check for monster -- bump to attack */
            Entity *mon = monster_at(gs, nx, ny);
            if (mon) {
                combat_attack_monster(gs, mon);
                advance_time(gs, 1);
                dungeon_enemy_turns(gs);
                return;
            }

            Tile *target = &tiles[ny][nx];
            if (target->passable) {
                /* Cat trails behind player in dungeons too */
                if (gs->has_cat) {
                    gs->cat_pos = gs->player_pos;
                }
                gs->player_pos.x = nx;
                gs->player_pos.y = ny;
                advance_time(gs, 1);
                check_traps(gs);
                check_chests(gs);
                dungeon_update_fov(gs);
                dungeon_enemy_turns(gs);

                /* Notify player of items on this tile */
                {
                    DungeonLevel *dl_step = current_dungeon_level(gs);
                    if (dl_step) {
                        for (int ii = 0; ii < dl_step->num_ground_items; ii++) {
                            Item *it = &dl_step->ground_items[ii];
                            if (it->on_ground && it->pos.x == nx && it->pos.y == ny) {
                                log_add(&gs->log, gs->turn, CP_CYAN,
                                         "You see here: %s (press g to pick up)", it->name);
                                break;  /* only show first item */
                            }
                        }
                    }
                }

                /* Shallow water damage */
                if (tiles[ny][nx].glyph == '~') {
                    gs->hp -= 1;
                    if (gs->hp < 1) gs->hp = 1;
                    const char *water_msgs[] = {
                        "You wade through freezing water. The cold bites at your legs. (-1 HP)",
                        "Filthy water splashes around your feet. It stings. (-1 HP)",
                        "The icy water soaks through your boots. (-1 HP)",
                        "You shiver as you slosh through the murky water. (-1 HP)",
                        "Something brushes your ankle in the dark water... (-1 HP)",
                    };
                    int n = sizeof(water_msgs) / sizeof(water_msgs[0]);
                    log_add(&gs->log, gs->turn, CP_BLUE, "%s", water_msgs[rng_range(0, n - 1)]);
                }

                /* Fungal growth -- poison risk */
                if (tiles[ny][nx].glyph == '"' && tiles[ny][nx].color_pair == CP_GREEN) {
                    if (rng_chance(20)) {
                        int dmg = rng_range(1, 3);
                        gs->hp -= dmg;
                        if (gs->hp < 1) gs->hp = 1;
                        log_add(&gs->log, gs->turn, CP_GREEN,
                                 "Fungal spores burst as you step through! You feel sick. (-%d HP)", dmg);
                    } else if (rng_chance(15)) {
                        log_add(&gs->log, gs->turn, CP_GREEN,
                                 "Strange mushrooms crunch underfoot. The air smells foul.");
                    }
                }

                /* Crystal formation -- visual only for now */
                if (tiles[ny][nx].glyph == '*' && tiles[ny][nx].color_pair == CP_CYAN_BOLD) {
                    if (rng_chance(20)) {
                        log_add(&gs->log, gs->turn, CP_CYAN_BOLD,
                                 "The crystals hum softly, casting eerie light across the chamber.");
                    }
                }

                /* Lava damage */
                if (tiles[ny][nx].glyph == '^' && tiles[ny][nx].color_pair == CP_RED_BOLD) {
                    int dmg = rng_range(5, 10);
                    gs->hp -= dmg;
                    if (gs->hp < 1) gs->hp = 1;
                    const char *lava_msgs[] = {
                        "You step on molten rock! Your feet burn! (-%d HP)",
                        "Lava scorches your boots! The heat is unbearable! (-%d HP)",
                        "The ground glows red-hot beneath you! (-%d HP)",
                    };
                    int n = sizeof(lava_msgs) / sizeof(lava_msgs[0]);
                    log_add(&gs->log, gs->turn, CP_RED_BOLD, lava_msgs[rng_range(0, n - 1)], dmg);
                }

                /* Ice -- slippery, chance to slide an extra tile */
                if (tiles[ny][nx].glyph == '_' && tiles[ny][nx].color_pair == CP_CYAN) {
                    if (rng_chance(30)) {
                        /* Slide one extra tile in the same direction */
                        int sx = nx + dir_dx[dir];
                        int sy = ny + dir_dy[dir];
                        if (sx > 0 && sx < MAP_WIDTH - 1 && sy > 0 && sy < MAP_HEIGHT - 1 &&
                            tiles[sy][sx].passable) {
                            gs->player_pos.x = sx;
                            gs->player_pos.y = sy;
                            log_add(&gs->log, gs->turn, CP_CYAN,
                                     "You slip on the ice and slide!");
                        } else {
                            log_add(&gs->log, gs->turn, CP_CYAN,
                                     "You slip on the ice and crash into the wall!");
                            gs->hp -= 1;
                            if (gs->hp < 1) gs->hp = 1;
                        }
                    } else {
                        if (rng_chance(40))
                            log_add(&gs->log, gs->turn, CP_CYAN,
                                     "The floor is slippery with ice. You tread carefully.");
                    }
                }

                /* Special room interactions */
                /* Altar -- pray for blessing */
                if (tiles[ny][nx].glyph == '_' && tiles[ny][nx].color_pair == CP_YELLOW_BOLD) {
                    int *stats[] = { &gs->str, &gs->def, &gs->intel, &gs->spd };
                    const char *names[] = { "STR", "DEF", "INT", "SPD" };
                    int pick = rng_range(0, 3);
                    (*stats[pick])++;
                    gs->hp = gs->max_hp;
                    tiles[ny][nx].glyph = '.';  /* altar used up */
                    log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                             "You kneel at the altar. A divine light! +1 %s, HP restored!", names[pick]);
                }
                /* Gold pile -- pick up */
                if (tiles[ny][nx].glyph == '$') {
                    int gold = rng_range(5, 20);
                    gs->gold += gold;
                    tiles[ny][nx].glyph = '.';
                    tiles[ny][nx].color_pair = CP_WHITE;
                    log_add(&gs->log, gs->turn, CP_YELLOW,
                             "You pick up %d gold coins!", gold);
                }
                /* Treasure room chest (golden) */
                if (tiles[ny][nx].glyph == '=' && tiles[ny][nx].color_pair == CP_YELLOW_BOLD) {
                    int gold = rng_range(20, 60);
                    gs->gold += gold;
                    tiles[ny][nx].glyph = '.';
                    tiles[ny][nx].color_pair = CP_WHITE;
                    log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                             "You open the treasure chest! Found %d gold!", gold);
                }
                /* Regular dungeon chest (brown) -- may be trapped */
                if (tiles[ny][nx].glyph == '=' && tiles[ny][nx].color_pair == CP_BROWN) {
                    tiles[ny][nx].glyph = '.';
                    tiles[ny][nx].color_pair = CP_WHITE;

                    if (rng_chance(20)) {
                        /* Trapped! */
                        int trap_type = rng_range(0, 2);
                        if (trap_type == 0) {
                            int dmg = rng_range(5, 10);
                            gs->hp -= dmg;
                            if (gs->hp < 1) gs->hp = 1;
                            log_add(&gs->log, gs->turn, CP_RED,
                                     "The chest is trapped! A dart hits you! -%d HP", dmg);
                        } else if (trap_type == 1) {
                            int dmg = rng_range(3, 8);
                            gs->hp -= dmg;
                            if (gs->hp < 1) gs->hp = 1;
                            log_add(&gs->log, gs->turn, CP_RED,
                                     "The chest explodes! -%d HP", dmg);
                        } else {
                            gs->mp -= gs->mp / 3;
                            log_add(&gs->log, gs->turn, CP_MAGENTA,
                                     "Gas pours from the chest! You feel confused and drained.");
                        }
                        /* Still get some loot */
                        int gold = rng_range(5, 15);
                        gs->gold += gold;
                        log_add(&gs->log, gs->turn, CP_YELLOW,
                                 "You salvage %d gold from the wreckage.", gold);
                    } else {
                        /* Normal loot */
                        int gold = rng_range(10, 40);
                        gs->gold += gold;
                        int bonus = rng_range(0, 3);
                        if (bonus == 0) {
                            gs->hp += 10;
                            if (gs->hp > gs->max_hp) gs->hp = gs->max_hp;
                            log_add(&gs->log, gs->turn, CP_YELLOW,
                                     "You open the chest! %d gold and a healing draught! (+10 HP)", gold);
                        } else if (bonus == 1) {
                            gs->mp += 8;
                            if (gs->mp > gs->max_mp) gs->mp = gs->max_mp;
                            log_add(&gs->log, gs->turn, CP_YELLOW,
                                     "You open the chest! %d gold and a mana vial! (+8 MP)", gold);
                        } else {
                            log_add(&gs->log, gs->turn, CP_YELLOW,
                                     "You open the chest! Found %d gold!", gold);
                        }
                    }
                }
                /* Bookshelf -- read for lore */
                if (tiles[ny][nx].glyph == '|') {
                    if (rng_chance(30)) {
                        const char *lore[] = {
                            "An old tome reads: \"Beware the dragon's breath...\"",
                            "A scroll mentions a hidden treasure near Stonehenge.",
                            "You read about the legend of Excalibur and the Lady of the Lake.",
                            "A dusty book describes ancient trap mechanisms.",
                            "Notes in the margin: \"The portal lies at the bottom.\"",
                            "A map fragment shows a secret passage behind the altar.",
                            "You find a prayer to ward off undead spirits.",
                        };
                        int n = sizeof(lore) / sizeof(lore[0]);
                        log_add(&gs->log, gs->turn, CP_BROWN, "%s", lore[rng_range(0, n - 1)]);
                    }
                }
                /* Coffin -- search for treasure or danger */
                if (tiles[ny][nx].glyph == '-' && tiles[ny][nx].color_pair == CP_GRAY) {
                    if (rng_chance(40)) {
                        int gold = rng_range(5, 15);
                        gs->gold += gold;
                        tiles[ny][nx].glyph = '.';
                        log_add(&gs->log, gs->turn, CP_YELLOW,
                                 "You search the coffin and find %d gold!", gold);
                    } else if (rng_chance(30)) {
                        int dmg = rng_range(3, 8);
                        gs->hp -= dmg;
                        if (gs->hp < 1) gs->hp = 1;
                        tiles[ny][nx].glyph = '.';
                        log_add(&gs->log, gs->turn, CP_RED,
                                 "Something lunges from the coffin! -%d HP!", dmg);
                    } else {
                        tiles[ny][nx].glyph = '.';
                        log_add(&gs->log, gs->turn, CP_GRAY,
                                 "The coffin is empty. Just dust and bones.");
                    }
                }

                /* Magic circle effect */
                if (tiles[ny][nx].glyph == '(') {
                    int circle_roll = rng_range(1, 100);

                    if (circle_roll <= 20) {
                        /* Circle of Healing */
                        gs->hp = gs->max_hp;
                        log_add(&gs->log, gs->turn, CP_WHITE_BOLD,
                                 "A Circle of Healing! White light surrounds you. HP fully restored!");
                    } else if (circle_roll <= 35) {
                        /* Circle of Mana */
                        gs->mp = gs->max_mp;
                        log_add(&gs->log, gs->turn, CP_CYAN_BOLD,
                                 "A Circle of Mana! Blue energy flows into you. MP fully restored!");
                    } else if (circle_roll <= 50) {
                        /* Circle of Blessing */
                        int *stats[] = { &gs->str, &gs->def, &gs->intel, &gs->spd };
                        const char *names[] = { "STR", "DEF", "INT", "SPD" };
                        int pick = rng_range(0, 3);
                        (*stats[pick])++;
                        log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                                 "A Circle of Blessing! Golden light! +1 %s!", names[pick]);
                    } else if (circle_roll <= 60) {
                        /* Circle of Gold */
                        int gold = rng_range(20, 80);
                        gs->gold += gold;
                        log_add(&gs->log, gs->turn, CP_YELLOW,
                                 "A Circle of Gold! Coins materialize! +%d gold!", gold);
                    } else if (circle_roll <= 70) {
                        /* Circle of Teleportation */
                        DungeonLevel *dl = current_dungeon_level(gs);
                        for (int t = 0; t < 200; t++) {
                            int tx = rng_range(3, MAP_WIDTH - 4);
                            int ty = rng_range(3, MAP_HEIGHT - 4);
                            if (dl->tiles[ty][tx].type == TILE_FLOOR &&
                                dl->tiles[ty][tx].glyph == '.') {
                                gs->player_pos = (Vec2){ tx, ty };
                                break;
                            }
                        }
                        log_add(&gs->log, gs->turn, CP_GREEN_BOLD,
                                 "A Circle of Teleportation! The world spins!");
                    } else if (circle_roll <= 80) {
                        /* Circle of Fire */
                        int dmg = rng_range(8, 15);
                        gs->hp -= dmg;
                        if (gs->hp < 1) gs->hp = 1;
                        log_add(&gs->log, gs->turn, CP_RED,
                                 "A Circle of Fire! Flames erupt! -%d HP!", dmg);
                    } else if (circle_roll <= 88) {
                        /* Circle of Draining */
                        int drain = gs->mp / 2;
                        gs->mp -= drain;
                        log_add(&gs->log, gs->turn, CP_MAGENTA,
                                 "A Circle of Draining! Your mana is siphoned away! -%d MP!", drain);
                    } else if (circle_roll <= 95) {
                        /* Circle of Confusion */
                        gs->drunk_turns += 15;
                        gs->beers_drunk = 2;  /* triggers stumbling */
                        log_add(&gs->log, gs->turn, CP_MAGENTA,
                                 "A Circle of Confusion! Your mind reels! Disoriented!");
                    } else {
                        /* Circle of Warding */
                        gs->hp += 20;
                        if (gs->hp > gs->max_hp) gs->hp = gs->max_hp;
                        gs->mp += 10;
                        if (gs->mp > gs->max_mp) gs->mp = gs->max_mp;
                        log_add(&gs->log, gs->turn, CP_WHITE_BOLD,
                                 "A Circle of Warding! A protective aura surrounds you!");
                    }

                    /* Circle fades after use */
                    tiles[ny][nx].glyph = '.';
                    tiles[ny][nx].color_pair = CP_WHITE;
                }

                /* Room entry flavour messages */
                /* Trigger when stepping into an open area (many floor neighbours) */
                {
                    int open = 0;
                    for (int d = 0; d < 8; d++) {
                        int ax = nx + dir_dx[d], ay = ny + dir_dy[d];
                        if (ax > 0 && ax < MAP_WIDTH - 1 && ay > 0 && ay < MAP_HEIGHT - 1 &&
                            tiles[ay][ax].type == TILE_FLOOR) open++;
                    }
                    /* Only trigger in rooms (6+ open neighbours), and rarely (15%) */
                    if (open >= 6 && rng_chance(15)) {
                        const char *flavour[] = {
                            "A cold draught chills your bones.",
                            "The air feels warm and heavy here.",
                            "A faint green glow emanates from the walls.",
                            "It feels spooky... the hairs on your neck stand up.",
                            "You hear dripping water in the darkness.",
                            "A musty smell of old stone fills your nostrils.",
                            "Cobwebs brush across your face.",
                            "The floor is damp and slippery underfoot.",
                            "You sense something watching you from the shadows.",
                            "Faint scratch marks cover the walls.",
                            "A low rumble echoes through the chamber.",
                            "The ceiling is low here. You duck instinctively.",
                            "Ancient runes are carved into the floor.",
                            "Bones are scattered in the corner of the room.",
                            "A strange symbol is painted on the wall in red.",
                            "The air smells of sulphur and decay.",
                            "Torchlight flickers from somewhere ahead.",
                            "You hear a distant scream... or was it the wind?",
                            "The stone walls glisten with moisture.",
                            "A cold breeze blows from a crack in the wall.",
                        };
                        int n = sizeof(flavour) / sizeof(flavour[0]);
                        log_add(&gs->log, gs->turn, CP_GRAY, "%s", flavour[rng_range(0, n - 1)]);
                    }
                }
            } else if (target->type == TILE_DOOR_CLOSED) {
                /* Auto-open doors and walk through in one move */
                target->type = TILE_DOOR_OPEN;
                target->glyph = '/';
                target->blocks_sight = false;
                target->passable = true;
                gs->player_pos.x = nx;
                gs->player_pos.y = ny;
                log_add(&gs->log, gs->turn, CP_WHITE, "You push open the door.");
                advance_time(gs, 1);
                check_traps(gs);
                check_chests(gs);
                dungeon_update_fov(gs);
            } else if (target->type == TILE_DOOR_LOCKED) {
                /* Locked door -- attempt to bash it open */
                int difficulty = rng_range(6, 14);
                if (gs->str >= difficulty) {
                    target->type = TILE_DOOR_OPEN;
                    target->glyph = '/';
                    target->color_pair = CP_BROWN;
                    target->blocks_sight = false;
                    target->passable = true;
                    gs->player_pos.x = nx;
                    gs->player_pos.y = ny;
                    log_add(&gs->log, gs->turn, CP_YELLOW,
                             "You smash the locked door open with brute force!");
                    dungeon_update_fov(gs);
                } else {
                    log_add(&gs->log, gs->turn, CP_RED,
                             "The door is locked! You shoulder it but it holds firm. (Need more STR)");
                }
                advance_time(gs, 1);
            } else {
                if (target->glyph == '~' && !target->passable) {
                    log_add(&gs->log, gs->turn, CP_BLUE,
                             "Deep water blocks your path. You cannot swim across.");
                } else if (target->glyph == '%') {
                    log_add(&gs->log, gs->turn, CP_BROWN,
                             "Rubble blocks the passage. You'd need a pickaxe to clear it.");
                } else {
                    log_add(&gs->log, gs->turn, CP_GRAY, "You bump into a wall.");
                }
            }
        }
        return;
    }
}

/* ------------------------------------------------------------------ */
/* Main input dispatch                                                 */
/* ------------------------------------------------------------------ */

void game_handle_input(GameState *gs, int key) {
    /* Character creation mode */
    if (gs->mode == MODE_CHARACTER_CREATE) {
        handle_character_create(gs, key);
        return;
    }

    /* Save game (S key on overworld) */
    if (key == 'S' && gs->mode == MODE_OVERWORLD) {
        if (save_game(gs)) {
            log_add(&gs->log, gs->turn, CP_GREEN, "Game saved.");
        } else {
            log_add(&gs->log, gs->turn, CP_RED, "Save failed!");
        }
        return;
    }

    /* Level-up stat choice screen */
    if (gs->pending_levelup) {
        ui_clear();
        int row = 2;
        gs->player_level++;

        /* HP/MP gains by class */
        int hp_gain = 0, mp_gain = 0;
        switch (gs->player_class) {
        case CLASS_KNIGHT: hp_gain = 3; mp_gain = 1; break;
        case CLASS_WIZARD: hp_gain = 1; mp_gain = 4; break;
        case CLASS_RANGER: hp_gain = 2; mp_gain = 2; break;
        }
        gs->max_hp += hp_gain;
        gs->max_mp += mp_gain;
        gs->hp = gs->max_hp;
        gs->mp = gs->max_mp;
        gs->max_weight += 2;

        attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
        mvprintw(row++, 4, "=== Level Up! Level %d ===", gs->player_level);
        attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
        row++;
        attron(COLOR_PAIR(CP_GREEN));
        mvprintw(row++, 6, "+%d max HP   +%d max MP   +2 carry capacity", hp_gain, mp_gain);
        attroff(COLOR_PAIR(CP_GREEN));
        row++;

        /* Class perks at milestone levels */
        const char *perk_msg = NULL;
        if (gs->player_level == 3) {
            gs->max_spells_capacity++;
            perk_msg = "Spell capacity +1!";
        } else if (gs->player_level == 5) {
            switch (gs->player_class) {
            case CLASS_KNIGHT: gs->max_hp += 2; perk_msg = "Knight perk: +2 max HP per level from now!"; break;
            case CLASS_RANGER: perk_msg = "Ranger perk: trap detection range doubled!"; break;
            case CLASS_WIZARD: perk_msg = "Wizard perk: -1 MP cost on all spells!"; break;
            }
        } else if (gs->player_level == 10) {
            switch (gs->player_class) {
            case CLASS_KNIGHT: perk_msg = "Knight perk: Shield Bash! 20% stun on melee."; break;
            case CLASS_RANGER: perk_msg = "Ranger perk: Double Shot! 30% double ranged hits."; break;
            case CLASS_WIZARD: perk_msg = "Wizard perk: Mana Shield! Absorb damage with MP at low HP."; break;
            }
        } else if (gs->player_level == 15) {
            switch (gs->player_class) {
            case CLASS_KNIGHT: perk_msg = "Knight mastery: Immunity to fear!"; break;
            case CLASS_RANGER: perk_msg = "Ranger mastery: Always act first in combat!"; break;
            case CLASS_WIZARD: perk_msg = "Wizard mastery: One free spell per turn (0 MP if INT > 10)!"; break;
            }
        }
        /* Extra +2 HP for knights at level 5+ */
        if (gs->player_level >= 6 && gs->player_class == CLASS_KNIGHT) {
            gs->max_hp += 2;
            gs->hp = gs->max_hp;
        }

        if (perk_msg) {
            attron(COLOR_PAIR(CP_CYAN_BOLD) | A_BOLD);
            mvprintw(row++, 6, "%s", perk_msg);
            attroff(COLOR_PAIR(CP_CYAN_BOLD) | A_BOLD);
            row++;
        }

        attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        mvprintw(row++, 4, "Choose a stat to increase by +1:");
        attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        row++;
        attron(COLOR_PAIR(CP_CYAN));
        mvprintw(row++, 6, "[1] STR: %d -> %d", gs->str, gs->str + 1);
        mvprintw(row++, 6, "[2] DEF: %d -> %d", gs->def, gs->def + 1);
        mvprintw(row++, 6, "[3] INT: %d -> %d", gs->intel, gs->intel + 1);
        mvprintw(row++, 6, "[4] SPD: %d -> %d", gs->spd, gs->spd + 1);
        attroff(COLOR_PAIR(CP_CYAN));
        ui_refresh();

        /* Wait for valid choice */
        while (1) {
            int lk = ui_getkey();
            if (lk == '1') { gs->str++; break; }
            if (lk == '2') { gs->def++; break; }
            if (lk == '3') { gs->intel++; break; }
            if (lk == '4') { gs->spd++; break; }
        }

        gs->pending_levelup = false;
        log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                 "You are now level %d!", gs->player_level);
        return;
    }

    /* Quit -- but not in town mode (q leaves town instead) */
    if ((key == 'q' || key == 'Q') && gs->mode != MODE_TOWN) {
        int term_rows, term_cols;
        ui_get_size(&term_rows, &term_cols);
        attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
        mvprintw(term_rows - 1, 0, "Quit? [s] Save & quit  [q] Quit without saving  [Esc] Cancel");
        attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
        ui_refresh();

        int qk = ui_getkey();
        if (qk == 's' || qk == 'S') {
            if (save_game(gs))
                log_add(&gs->log, gs->turn, CP_GREEN, "Game saved.");
            gs->running = false;
        } else if (qk == 'q' || qk == 'Q') {
            gs->running = false;
        }
        /* Esc or any other key = cancel, continue playing */
        return;
    }

    /* Spell casting (z key) */
    if (key == 'z' && (gs->mode == MODE_DUNGEON || gs->mode == MODE_OVERWORLD)) {
        if (gs->num_spells == 0) {
            log_add(&gs->log, gs->turn, CP_GRAY, "You don't know any spells.");
        } else {
            /* Show spell list */
            ui_clear();
            int row = 1;
            attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
            mvprintw(row++, 2, "=== Cast Spell ===");
            attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
            row++;

            for (int i = 0; i < gs->num_spells; i++) {
                const SpellDef *sp = spell_get(gs->spells_known[i]);
                if (!sp) continue;
                const char *aff_name = "Universal";
                short aff_color = CP_WHITE;
                if (sp->affiliation == AFF_LIGHT)  { aff_name = "Light";  aff_color = CP_YELLOW; }
                if (sp->affiliation == AFF_DARK)   { aff_name = "Dark";   aff_color = CP_MAGENTA; }
                if (sp->affiliation == AFF_NATURE) { aff_name = "Nature"; aff_color = CP_GREEN; }

                bool can_cast = (gs->mp >= sp->mp_cost);
                attron(COLOR_PAIR(can_cast ? aff_color : CP_GRAY));
                mvprintw(row++, 4, "[%c] %-20s  MP:%-3d  %s",
                         'a' + i, sp->name, sp->mp_cost, aff_name);
                attroff(COLOR_PAIR(can_cast ? aff_color : CP_GRAY));
            }
            row++;
            attron(COLOR_PAIR(CP_GRAY));
            mvprintw(row, 4, "Press a letter to cast, or Esc to cancel.");
            attroff(COLOR_PAIR(CP_GRAY));
            ui_refresh();

            /* Wait for selection */
            int skey = getch();
            if (skey >= 'a' && skey < 'a' + gs->num_spells) {
                int si = skey - 'a';
                const SpellDef *sp = spell_get(gs->spells_known[si]);
                if (!sp) {
                    log_add(&gs->log, gs->turn, CP_RED, "Invalid spell.");
                } else if (gs->mp < sp->mp_cost) {
                    log_add(&gs->log, gs->turn, CP_RED, "Not enough mana! (need %d MP)", sp->mp_cost);
                } else {
                    /* Check affiliation threshold */
                    bool aff_ok = true;
                    if (sp->threshold > 0) {
                        int player_aff = 0;
                        if (sp->affiliation == AFF_LIGHT)  player_aff = gs->light_affinity;
                        if (sp->affiliation == AFF_DARK)   player_aff = gs->dark_affinity;
                        if (sp->affiliation == AFF_NATURE) player_aff = gs->nature_affinity;
                        if (player_aff < sp->threshold) {
                            const char *aff_names[] = { "Universal", "Light", "Dark", "Nature" };
                            log_add(&gs->log, gs->turn, CP_RED,
                                     "Your %s affinity is too low! (need %d, have %d)",
                                     aff_names[sp->affiliation], sp->threshold, player_aff);
                            aff_ok = false;
                        }
                    }
                    if (aff_ok) {
                    /* Cast the spell */
                    gs->mp -= sp->mp_cost;
                    switch (sp->effect) {
                    case SEFF_DAMAGE:
                        if (gs->mode == MODE_DUNGEON) {
                            /* Damage nearest visible monster */
                            DungeonLevel *dl_sp = current_dungeon_level(gs);
                            if (dl_sp) {
                                Entity *nearest = NULL;
                                int best_dist = 9999;
                                for (int m = 0; m < dl_sp->num_monsters; m++) {
                                    Entity *em = &dl_sp->monsters[m];
                                    if (!em->alive) continue;
                                    if (!dl_sp->tiles[em->pos.y][em->pos.x].visible) continue;
                                    int d = abs(em->pos.x - gs->player_pos.x) + abs(em->pos.y - gs->player_pos.y);
                                    if (d <= sp->range && d < best_dist) {
                                        best_dist = d;
                                        nearest = em;
                                    }
                                }
                                if (nearest) {
                                    int dmg = sp->damage + gs->intel / 2 + rng_range(-2, 2);
                                    if (dmg < 1) dmg = 1;
                                    nearest->hp -= dmg;
                                    log_add(&gs->log, gs->turn, CP_CYAN,
                                             "You cast %s! The %s takes %d damage!", sp->name, nearest->name, dmg);
                                    if (nearest->hp <= 0) {
                                        nearest->alive = false;
                                        gs->xp += nearest->xp_reward;
                                        gs->kills++;
                                        log_add(&gs->log, gs->turn, CP_YELLOW,
                                                 "The %s is slain! +%d XP", nearest->name, nearest->xp_reward);
                                        ai_explode_on_death(gs, nearest);
                                    }
                                } else {
                                    log_add(&gs->log, gs->turn, CP_GRAY, "You cast %s but find no target.", sp->name);
                                    gs->mp += sp->mp_cost; /* refund */
                                }
                            }
                        }
                        break;
                    case SEFF_HEAL: {
                        int heal = sp->damage > 0 ? sp->damage : (sp->mp_cost * 3);
                        heal += gs->intel / 2;
                        gs->hp += heal;
                        if (gs->hp > gs->max_hp) gs->hp = gs->max_hp;
                        log_add(&gs->log, gs->turn, CP_GREEN,
                                 "You cast %s. Restored %d HP.", sp->name, heal);
                        break;
                    }
                    case SEFF_SHIELD:
                        gs->shield_absorb += 15 + gs->intel;
                        log_add(&gs->log, gs->turn, CP_CYAN,
                                 "You cast %s. A magical shield surrounds you. (%d absorb)",
                                 sp->name, gs->shield_absorb);
                        break;
                    case SEFF_BUFF_DEF:
                        gs->def += 3;
                        gs->buff_def_turns = sp->duration > 0 ? sp->duration : 15;
                        log_add(&gs->log, gs->turn, CP_CYAN,
                                 "You cast %s. DEF +3 for %d turns.", sp->name, gs->buff_def_turns);
                        break;
                    case SEFF_BUFF_STR:
                        gs->str += 3;
                        gs->buff_str_turns = sp->duration > 0 ? sp->duration : 15;
                        log_add(&gs->log, gs->turn, CP_CYAN,
                                 "You cast %s. STR +3 for %d turns.", sp->name, gs->buff_str_turns);
                        break;
                    case SEFF_BUFF_SPD:
                        gs->spd += 3;
                        gs->buff_spd_turns = sp->duration > 0 ? sp->duration : 10;
                        log_add(&gs->log, gs->turn, CP_CYAN,
                                 "You cast %s. SPD +3 for %d turns.", sp->name, gs->buff_spd_turns);
                        break;
                    case SEFF_TELEPORT:
                        if (gs->mode == MODE_DUNGEON) {
                            DungeonLevel *dl_tp = current_dungeon_level(gs);
                            if (dl_tp) {
                                for (int tries = 0; tries < 200; tries++) {
                                    int tx = rng_range(3, MAP_WIDTH - 4);
                                    int ty = rng_range(3, MAP_HEIGHT - 4);
                                    if (dl_tp->tiles[ty][tx].passable && !monster_at(gs, tx, ty)) {
                                        gs->player_pos = (Vec2){ tx, ty };
                                        dungeon_update_fov(gs);
                                        log_add(&gs->log, gs->turn, CP_CYAN, "You cast %s! You teleport!", sp->name);
                                        break;
                                    }
                                }
                            }
                        }
                        break;
                    case SEFF_REVEAL:
                        if (gs->mode == MODE_DUNGEON) {
                            DungeonLevel *dl_rv = current_dungeon_level(gs);
                            if (dl_rv) {
                                for (int ry = 0; ry < MAP_HEIGHT; ry++)
                                    for (int rx = 0; rx < MAP_WIDTH; rx++)
                                        dl_rv->tiles[ry][rx].revealed = true;
                                log_add(&gs->log, gs->turn, CP_CYAN, "You cast %s. The entire floor is revealed!", sp->name);
                            }
                        }
                        break;
                    case SEFF_DETECT:
                        if (gs->mode == MODE_DUNGEON) {
                            DungeonLevel *dl_dt = current_dungeon_level(gs);
                            if (dl_dt) {
                                int found = 0;
                                for (int t = 0; t < dl_dt->num_traps; t++) {
                                    dl_dt->traps[t].revealed = true;
                                    found++;
                                }
                                log_add(&gs->log, gs->turn, CP_CYAN, "You cast %s. %d traps revealed!", sp->name, found);
                            }
                        }
                        break;
                    case SEFF_FEAR:
                        if (gs->mode == MODE_DUNGEON) {
                            DungeonLevel *dl_fr = current_dungeon_level(gs);
                            if (dl_fr) {
                                int feared = 0;
                                for (int m = 0; m < dl_fr->num_monsters; m++) {
                                    Entity *em = &dl_fr->monsters[m];
                                    if (!em->alive) continue;
                                    int d = abs(em->pos.x - gs->player_pos.x) + abs(em->pos.y - gs->player_pos.y);
                                    if (d <= (sp->aoe > 0 ? sp->aoe + 2 : 5)) {
                                        em->ai_state = AI_STATE_FLEE;
                                        feared++;
                                    }
                                }
                                log_add(&gs->log, gs->turn, CP_CYAN, "You cast %s! %d enemies flee in terror!", sp->name, feared);
                            }
                        }
                        break;
                    case SEFF_DRAIN: {
                        if (gs->mode == MODE_DUNGEON) {
                            DungeonLevel *dl_dr = current_dungeon_level(gs);
                            if (dl_dr) {
                                Entity *nearest = NULL;
                                int best_dist = 9999;
                                for (int m = 0; m < dl_dr->num_monsters; m++) {
                                    Entity *em = &dl_dr->monsters[m];
                                    if (!em->alive) continue;
                                    if (!dl_dr->tiles[em->pos.y][em->pos.x].visible) continue;
                                    int d = abs(em->pos.x - gs->player_pos.x) + abs(em->pos.y - gs->player_pos.y);
                                    if (d <= sp->range && d < best_dist) {
                                        best_dist = d;
                                        nearest = em;
                                    }
                                }
                                if (nearest) {
                                    int dmg = sp->damage + gs->intel / 3;
                                    if (dmg < 1) dmg = 1;
                                    nearest->hp -= dmg;
                                    gs->hp += dmg / 2;
                                    if (gs->hp > gs->max_hp) gs->hp = gs->max_hp;
                                    log_add(&gs->log, gs->turn, CP_MAGENTA,
                                             "You cast %s! Drain %d from %s, heal %d.",
                                             sp->name, dmg, nearest->name, dmg / 2);
                                    if (nearest->hp <= 0) {
                                        nearest->alive = false;
                                        gs->xp += nearest->xp_reward;
                                        gs->kills++;
                                        log_add(&gs->log, gs->turn, CP_YELLOW,
                                                 "The %s is slain! +%d XP", nearest->name, nearest->xp_reward);
                                    }
                                } else {
                                    log_add(&gs->log, gs->turn, CP_GRAY, "No target for %s.", sp->name);
                                    gs->mp += sp->mp_cost;
                                }
                            }
                        }
                        break;
                    }
                    default:
                        log_add(&gs->log, gs->turn, CP_CYAN, "You cast %s!", sp->name);
                        break;
                    }
                    } /* end if (aff_ok) */
                }
            }
        }
        return;
    }

    /* Overworld look/identify mode (; key) */
    if (key == ';' && gs->mode == MODE_OVERWORLD) {
        log_add(&gs->log, gs->turn, CP_WHITE, "Look mode: move cursor to examine. Press ; or Esc to exit.");
        Vec2 cursor = gs->player_pos;

        while (1) {
            game_render(gs);

            /* Draw cursor */
            int term_rows_l, term_cols_l;
            ui_get_size(&term_rows_l, &term_cols_l);
            bool show_sb = (term_cols_l >= MIN_TERM_WIDTH);
            int vw = show_sb ? (term_cols_l - SIDEBAR_WIDTH - 1) : term_cols_l;
            int vh = term_rows_l - LOG_LINES - 2;
            if (vh > VIEW_HEIGHT_DEFAULT) vh = VIEW_HEIGHT_DEFAULT;
            if (vh < 5) vh = 5;
            int cam_x = gs->player_pos.x - vw / 2;
            int cam_y = gs->player_pos.y - vh / 2;
            if (cam_x < 0) cam_x = 0;
            if (cam_y < 0) cam_y = 0;
            if (cam_x + vw > OW_WIDTH) cam_x = OW_WIDTH - vw;
            if (cam_y + vh > OW_HEIGHT) cam_y = OW_HEIGHT - vh;

            int cx = cursor.x - cam_x;
            int cy = cursor.y - cam_y;
            if (cx >= 0 && cx < vw && cy >= 0 && cy < vh) {
                attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BLINK);
                mvaddch(cy, cx, 'X');
                attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BLINK);
            }

            /* Show info about cursor position */
            int info_row = vh + 1;
            attron(COLOR_PAIR(CP_WHITE));
            /* Check for location */
            Location *loc_l = overworld_location_at(gs->overworld, cursor.x, cursor.y);
            if (loc_l) {
                const char *type_names[] = {
                    "None", "Town", "Castle", "Abandoned Castle", "Landmark",
                    "Dungeon", "Cottage", "Cave", "Volcano", "Magic Circle", "Abbey"
                };
                const char *tname = (loc_l->type < 11) ? type_names[loc_l->type] : "Location";
                mvprintw(info_row, 1, "%-40s [%s]                    ", loc_l->name, tname);
            } else {
                /* Check for creature */
                OWCreature *cr = overworld_creature_at(gs->overworld, cursor.x, cursor.y);
                if (cr) {
                    if (cr->hostile) {
                        mvprintw(info_row, 1, "%-20s HP:%d/%d STR:%d DEF:%d XP:%d        ",
                                 cr->name, cr->hp, cr->max_hp, cr->str, cr->def, cr->xp_reward);
                    } else {
                        mvprintw(info_row, 1, "%-20s (friendly)                              ", cr->name);
                    }
                } else {
                    Tile *tl = &gs->overworld->map[cursor.y][cursor.x];
                    const char *tn = terrain_name(tl->type);
                    mvprintw(info_row, 1, "Terrain: %-20s                               ", tn);
                }
            }
            attroff(COLOR_PAIR(CP_WHITE));

            attron(COLOR_PAIR(CP_GRAY));
            mvprintw(info_row + 1, 1, "Look mode: move cursor to examine. Press ; or Esc to exit.");
            attroff(COLOR_PAIR(CP_GRAY));
            ui_refresh();

            int lkey = ui_getkey();
            if (lkey == 27 || lkey == ';') break;
            Direction ldir = key_to_direction(lkey);
            if (ldir != DIR_NONE) {
                cursor.x += dir_dx[ldir];
                cursor.y += dir_dy[ldir];
                if (cursor.x < 0) cursor.x = 0;
                if (cursor.y < 0) cursor.y = 0;
                if (cursor.x >= OW_WIDTH) cursor.x = OW_WIDTH - 1;
                if (cursor.y >= OW_HEIGHT) cursor.y = OW_HEIGHT - 1;
            }
        }
        return;
    }

    /* Dungeon look/identify mode (; key) -- examine visible tiles */
    if (key == ';' && gs->mode == MODE_DUNGEON && gs->dungeon) {
        log_add(&gs->log, gs->turn, CP_WHITE, "Look mode: move cursor to examine. Press ; or Esc to exit.");

        DungeonLevel *dl_look = current_dungeon_level(gs);
        if (dl_look) {
            Vec2 cursor = gs->player_pos;

            while (1) {
                /* Render the map normally */
                game_render(gs);

                /* Draw cursor -- match camera calculation from game_render */
                int term_rows, term_cols;
                ui_get_size(&term_rows, &term_cols);
                int vw = term_cols - SIDEBAR_WIDTH - 1;
                if (vw > MAP_WIDTH) vw = MAP_WIDTH;
                int vh = term_rows - LOG_LINES - 2;
                if (vh > VIEW_HEIGHT_DEFAULT) vh = VIEW_HEIGHT_DEFAULT;
                if (vh < 5) vh = 5;
                int cam_x = gs->player_pos.x - vw / 2;
                int cam_y = gs->player_pos.y - vh / 2;
                if (cam_x < 0) cam_x = 0;
                if (cam_y < 0) cam_y = 0;
                if (cam_x + vw > MAP_WIDTH) cam_x = MAP_WIDTH - vw;
                if (cam_y + vh > MAP_HEIGHT) cam_y = MAP_HEIGHT - vh;

                int cx = cursor.x - cam_x;
                int cy = cursor.y - cam_y;
                if (cx >= 0 && cx < vw && cy >= 0 && cy < vh) {
                    attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BLINK);
                    mvaddch(cy, cx, 'X');
                    attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BLINK);
                }

                /* Show info about cursor tile at bottom */
                Tile *ct = &dl_look->tiles[cursor.y][cursor.x];
                int info_row = vh + 1;

                attron(COLOR_PAIR(CP_WHITE));
                if (ct->visible) {
                    /* Check for monster */
                    Entity *mon = monster_at(gs, cursor.x, cursor.y);
                    if (mon) {
                        mvprintw(info_row, 1, "Monster: %s  HP:%d/%d  STR:%d  DEF:%d  SPD:%d  XP:%d    ",
                                 mon->name, mon->hp, mon->max_hp, mon->str, mon->def, mon->spd, mon->xp_reward);
                    } else {
                        /* Check for item */
                        bool found_item = false;
                        for (int ii = 0; ii < dl_look->num_ground_items; ii++) {
                            Item *it = &dl_look->ground_items[ii];
                            if (it->on_ground && it->pos.x == cursor.x && it->pos.y == cursor.y) {
                                mvprintw(info_row, 1, "Item: %s [%s]  Power:%d  Value:%dg  Weight:%d    ",
                                         it->name, item_type_name(it->type), it->power, it->value, it->weight / 10);
                                found_item = true;
                                break;
                            }
                        }
                        if (!found_item) {
                            /* Show tile info */
                            const char *tile_desc = "";
                            switch (ct->glyph) {
                            case '#': tile_desc = "Wall"; break;
                            case '.': tile_desc = "Floor"; break;
                            case '+': tile_desc = ct->color_pair == CP_RED ? "Locked door" : "Closed door"; break;
                            case '/': tile_desc = "Open door"; break;
                            case '<': tile_desc = "Stairs up"; break;
                            case '>': tile_desc = "Stairs down"; break;
                            case '0': tile_desc = "Exit portal"; break;
                            case '~': tile_desc = ct->passable ? "Shallow water" : "Deep water"; break;
                            case '^': tile_desc = ct->color_pair == CP_RED ? "Revealed trap" : "Lava"; break;
                            case '_': tile_desc = "Ice"; break;
                            case '"': tile_desc = "Fungal growth"; break;
                            case '%': tile_desc = "Rubble"; break;
                            case '*': tile_desc = "Crystal formation"; break;
                            case '(': tile_desc = "Magic circle"; break;
                            case 'O': tile_desc = "Pillar"; break;
                            case '$': tile_desc = "Gold pile"; break;
                            case '=': tile_desc = "Chest"; break;
                            case '|': tile_desc = "Bookshelf"; break;
                            case '-': tile_desc = "Coffin"; break;
                            default: tile_desc = "Unknown"; break;
                            }
                            mvprintw(info_row, 1, "Tile: %s ('%c')                                        ", tile_desc, ct->glyph);
                        }
                    }
                } else if (ct->revealed) {
                    mvprintw(info_row, 1, "Tile: (remembered, not currently visible)                    ");
                } else {
                    mvprintw(info_row, 1, "Tile: (unexplored)                                          ");
                }
                attroff(COLOR_PAIR(CP_WHITE));

                ui_refresh();

                int lkey = ui_getkey();
                if (lkey == 27 || lkey == ';') break;  /* Esc or ; to exit look mode */

                Direction ldir = key_to_direction(lkey);
                if (ldir != DIR_NONE) {
                    int nx2 = cursor.x + dir_dx[ldir];
                    int ny2 = cursor.y + dir_dy[ldir];
                    if (nx2 >= 0 && nx2 < MAP_WIDTH && ny2 >= 0 && ny2 < MAP_HEIGHT) {
                        cursor.x = nx2;
                        cursor.y = ny2;
                    }
                }
            }
        }
        return;
    }

    /* Dungeon minimap */
    if (key == 'M' && gs->mode == MODE_DUNGEON && gs->dungeon) {
        char info[256];
        snprintf(info, sizeof(info), "%s  Level %d/%d  |  Day %d %02d:%02d",
                 gs->dungeon->name,
                 gs->dungeon->current_level + 1, gs->dungeon->max_depth,
                 gs->day, gs->hour, gs->minute);
        Tile (*dtiles)[MAP_WIDTH] = current_dungeon_tiles(gs);
        if (dtiles) {
            ui_render_minimap((Tile *)dtiles, MAP_WIDTH, MAP_HEIGHT,
                              gs->player_pos, info);
            ui_getkey();
        }
        return;
    }

    /* Inventory screen (i key) -- works in all modes */
    if (key == 'i') {
        ui_clear();
        int term_rows, term_cols;
        ui_get_size(&term_rows, &term_cols);
        int row = 1;

        attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
        mvprintw(row++, 2, "=== Inventory ===");
        attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
        row++;

        attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        mvprintw(row++, 2, "Equipped:");
        attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        const char *slot_names[] = { "Weapon", "Armor", "Shield", "Helmet", "Boots", "Gloves", "Ring 1", "Ring 2", "Amulet" };
        for (int s = 0; s < NUM_SLOTS; s++) {
            if (gs->equipment[s].template_id >= 0) {
                attron(COLOR_PAIR(gs->equipment[s].color_pair));
                mvprintw(row++, 4, "%-8s: %s (pow:%d)",
                         slot_names[s], gs->equipment[s].name, gs->equipment[s].power);
                attroff(COLOR_PAIR(gs->equipment[s].color_pair));
            } else {
                attron(COLOR_PAIR(CP_GRAY));
                mvprintw(row++, 4, "%-8s: (empty)", slot_names[s]);
                attroff(COLOR_PAIR(CP_GRAY));
            }
        }
        row++;

        attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        mvprintw(row++, 2, "Bag (%d/%d):", gs->num_items, MAX_INVENTORY);
        attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        if (gs->num_items == 0) {
            attron(COLOR_PAIR(CP_GRAY));
            mvprintw(row++, 4, "(empty)");
            attroff(COLOR_PAIR(CP_GRAY));
        }
        for (int ii = 0; ii < gs->num_items && row < term_rows - 4; ii++) {
            Item *it = &gs->inventory[ii];
            if (it->template_id < 0) continue;

            /* Check for rotten fish */
            bool is_fish_item = (strstr(it->name, "Fish") != NULL ||
                                 strstr(it->name, "Salmon") != NULL);
            bool rotten = (is_fish_item && it->created_day > 0 &&
                           (gs->day - it->created_day) >= 2);

            if (rotten) {
                attron(COLOR_PAIR(CP_RED));
                mvprintw(row++, 4, "%c) %s (ROTTEN!) [%s]",
                         'a' + ii, it->name, item_type_name(it->type));
                attroff(COLOR_PAIR(CP_RED));
            } else {
                const char *disp_name = potion_display_name(gs, it);
                attron(COLOR_PAIR(it->color_pair));
                mvprintw(row++, 4, "%c) %s [%s] pow:%d val:%dg",
                         'a' + ii, disp_name, item_type_name(it->type),
                         it->power, it->value);
                attroff(COLOR_PAIR(it->color_pair));
            }
        }
        row++;
        attron(COLOR_PAIR(CP_WHITE));
        mvprintw(row++, 2, "Press a-z to select, then: e=equip d=drop u=use | q=close");
        attroff(COLOR_PAIR(CP_WHITE));
        ui_refresh();

        int ikey = ui_getkey();
        if (ikey >= 'a' && ikey <= 'z') {
            int idx = ikey - 'a';
            if (idx < gs->num_items && gs->inventory[idx].template_id >= 0) {
                Item *sel = &gs->inventory[idx];
                mvprintw(row, 4, "Selected: %s  [e]quip [d]rop [u]se [q]cancel", sel->name);
                ui_refresh();
                int akey = ui_getkey();
                if (akey == 'e') {
                    EquipSlot slot = item_get_slot(sel->type);
                    if (slot == SLOT_NONE) {
                        log_add(&gs->log, gs->turn, CP_GRAY, "You can't equip that.");
                    } else {
                        Item old_equip = gs->equipment[slot];
                        gs->equipment[slot] = *sel;
                        gs->equipment[slot].on_ground = false;
                        if (old_equip.template_id >= 0) {
                            *sel = old_equip;
                            log_add(&gs->log, gs->turn, CP_WHITE,
                                     "Equipped %s. Unequipped %s.", gs->equipment[slot].name, old_equip.name);
                        } else {
                            for (int j = idx; j < gs->num_items - 1; j++) gs->inventory[j] = gs->inventory[j+1];
                            gs->inventory[--gs->num_items].template_id = -1;
                            log_add(&gs->log, gs->turn, CP_WHITE, "Equipped %s.", gs->equipment[slot].name);
                        }
                    }
                } else if (akey == 'd') {
                    log_add(&gs->log, gs->turn, CP_WHITE, "Dropped %s.", sel->name);
                    for (int j = idx; j < gs->num_items - 1; j++) gs->inventory[j] = gs->inventory[j+1];
                    gs->inventory[--gs->num_items].template_id = -1;
                } else if (akey == 'u') {
                    if (sel->type == ITYPE_POTION) {
                        /* Identify this potion type */
                        bool was_unknown = (sel->template_id >= 0 && !gs->potions_identified[sel->template_id]);
                        if (sel->template_id >= 0)
                            gs->potions_identified[sel->template_id] = true;

                        /* Apply effect based on potion name */
                        bool is_mana = (strstr(sel->name, "Mana") != NULL);
                        bool is_str  = (strcmp(sel->name, "Strength Potion") == 0);
                        bool is_spd  = (strcmp(sel->name, "Speed Potion") == 0);
                        bool is_def  = (strcmp(sel->name, "Defence Potion") == 0);
                        bool is_int  = (strcmp(sel->name, "Intelligence Potion") == 0);

                        if (is_mana) {
                            gs->mp += sel->power;
                            if (gs->mp > gs->max_mp) gs->mp = gs->max_mp;
                            log_add(&gs->log, gs->turn, CP_BLUE, "You drink the %s. +%d MP!", sel->name, sel->power);
                        } else if (is_str) {
                            gs->str += sel->power;
                            gs->buff_str_turns = 50;
                            log_add(&gs->log, gs->turn, CP_YELLOW, "You drink the %s. +%d STR!", sel->name, sel->power);
                        } else if (is_spd) {
                            gs->spd += sel->power;
                            gs->buff_spd_turns = 50;
                            log_add(&gs->log, gs->turn, CP_CYAN, "You drink the %s. +%d SPD!", sel->name, sel->power);
                        } else if (is_def) {
                            gs->def += sel->power;
                            gs->buff_def_turns = 50;
                            log_add(&gs->log, gs->turn, CP_GRAY, "You drink the %s. +%d DEF!", sel->name, sel->power);
                        } else if (is_int) {
                            gs->intel += sel->power;
                            log_add(&gs->log, gs->turn, CP_MAGENTA, "You drink the %s. +%d INT!", sel->name, sel->power);
                        } else {
                            /* Default: healing */
                            gs->hp += sel->power;
                            if (gs->hp > gs->max_hp) gs->hp = gs->max_hp;
                            log_add(&gs->log, gs->turn, CP_GREEN, "You drink the %s. +%d HP!", sel->name, sel->power);
                        }
                        if (was_unknown)
                            log_add(&gs->log, gs->turn, CP_WHITE, "You identify it as: %s", sel->name);
                        for (int j = idx; j < gs->num_items - 1; j++) gs->inventory[j] = gs->inventory[j+1];
                        gs->inventory[--gs->num_items].template_id = -1;
                    } else if (sel->type == ITYPE_FOOD) {
                        /* Check for rotten fish */
                        bool is_fish = (strstr(sel->name, "Fish") != NULL ||
                                        strstr(sel->name, "Salmon") != NULL);
                        bool is_rotten = (is_fish && sel->created_day > 0 &&
                                          (gs->day - sel->created_day) >= 2);

                        if (is_rotten) {
                            /* Eating rotten fish poisons you */
                            int poison_dmg = rng_range(5, 12);
                            gs->hp -= poison_dmg;
                            if (gs->hp < 1) gs->hp = 1;
                            gs->str -= 1; if (gs->str < 1) gs->str = 1;
                            log_add(&gs->log, gs->turn, CP_RED,
                                     "The %s has gone rotten! You retch violently! -%d HP, -1 STR!",
                                     sel->name, poison_dmg);
                            log_add(&gs->log, gs->turn, CP_RED,
                                     "You are poisoned! You should have thrown that away...");
                        } else {
                            gs->hp += sel->power / 2;
                            if (gs->hp > gs->max_hp) gs->hp = gs->max_hp;
                            log_add(&gs->log, gs->turn, CP_GREEN, "You eat the %s. Delicious!", sel->name);
                        }
                        for (int j = idx; j < gs->num_items - 1; j++) gs->inventory[j] = gs->inventory[j+1];
                        gs->inventory[--gs->num_items].template_id = -1;
                    } else if (strcmp(sel->name, "Tattered Map") == 0) {
                        /* Show treasure map -- find closest unfound treasure */
                        int best = -1, best_dist = 999999;
                        for (int mi = 0; mi < gs->num_treasure_maps; mi++) {
                            if (gs->treasure_found[mi]) continue;
                            int ddx = gs->treasure_spots[mi].x - gs->player_pos.x;
                            int ddy = gs->treasure_spots[mi].y - gs->player_pos.y;
                            int d = ddx * ddx + ddy * ddy;
                            if (d < best_dist) { best_dist = d; best = mi; }
                        }
                        if (best >= 0) {
                            ui_clear();
                            int mrow = 2;
                            attron(COLOR_PAIR(CP_BROWN) | A_BOLD);
                            mvprintw(mrow++, 4, "=== Tattered Treasure Map ===");
                            attroff(COLOR_PAIR(CP_BROWN) | A_BOLD);
                            mrow++;

                            /* Draw a crude mini-map centred on the treasure */
                            Vec2 tpos = gs->treasure_spots[best];
                            int map_r = 15;
                            for (int my = -8; my <= 8; my++) {
                                for (int mx = -map_r; mx <= map_r; mx++) {
                                    int wx = tpos.x + mx, wy = tpos.y + my;
                                    int sx = 20 + mx + map_r, sy = mrow + my + 8;
                                    if (wx < 0 || wx >= OW_WIDTH || wy < 0 || wy >= OW_HEIGHT) {
                                        mvaddch(sy, sx, '~');
                                        continue;
                                    }
                                    if (wx == tpos.x && wy == tpos.y) {
                                        attron(COLOR_PAIR(CP_RED_BOLD) | A_BOLD);
                                        mvaddch(sy, sx, 'X');
                                        attroff(COLOR_PAIR(CP_RED_BOLD) | A_BOLD);
                                    } else {
                                        Tile *mt = &gs->overworld->map[wy][wx];
                                        attron(COLOR_PAIR(mt->color_pair));
                                        mvaddch(sy, sx, mt->glyph);
                                        attroff(COLOR_PAIR(mt->color_pair));
                                    }
                                }
                            }
                            mrow += 18;
                            attron(COLOR_PAIR(CP_YELLOW));
                            mvprintw(mrow++, 4, "The X marks the spot! Navigate there and press 'x' to dig.");
                            attroff(COLOR_PAIR(CP_YELLOW));
                            attron(COLOR_PAIR(CP_GRAY));
                            mvprintw(mrow++, 4, "Approximate location: %s",
                                     tpos.y < 80 ? "Northern England" :
                                     tpos.y < 140 ? "Central England" :
                                     tpos.y < 200 ? "Southern England" : "The South Coast");
                            mvprintw(mrow, 4, "Press any key to close.");
                            attroff(COLOR_PAIR(CP_GRAY));
                            ui_refresh();
                            ui_getkey();
                        } else {
                            log_add(&gs->log, gs->turn, CP_GRAY,
                                     "The map is faded and unreadable. All treasures have been found.");
                        }
                    } else if (sel->type == ITYPE_SPELL_SCROLL) {
                        /* Learn a spell from a scroll */
                        int spell_id = sel->power;
                        const SpellDef *sp = spell_get(spell_id);
                        if (!sp) {
                            log_add(&gs->log, gs->turn, CP_RED, "The scroll crumbles to dust. Nothing happens.");
                        } else {
                            /* Check if already known */
                            bool already_known = false;
                            for (int si = 0; si < gs->num_spells; si++) {
                                if (gs->spells_known[si] == spell_id) { already_known = true; break; }
                            }
                            if (already_known) {
                                log_add(&gs->log, gs->turn, CP_GRAY, "You already know %s.", sp->name);
                            } else if (gs->player_level < sp->level_required) {
                                log_add(&gs->log, gs->turn, CP_RED,
                                         "You cannot comprehend the magic within... (need level %d)", sp->level_required);
                            } else if (gs->num_spells < gs->max_spells_capacity) {
                                /* Learn directly */
                                gs->spells_known[gs->num_spells++] = spell_id;
                                log_add(&gs->log, gs->turn, CP_CYAN,
                                         "You study the scroll and learn %s!", sp->name);
                                /* Consume scroll */
                                for (int j = idx; j < gs->num_items - 1; j++) gs->inventory[j] = gs->inventory[j+1];
                                gs->inventory[--gs->num_items].template_id = -1;
                            } else {
                                /* At capacity -- replace a random spell */
                                int replaced = rng_range(0, gs->num_spells - 1);
                                const SpellDef *old_sp = spell_get(gs->spells_known[replaced]);
                                const char *old_name = old_sp ? old_sp->name : "???";
                                log_add(&gs->log, gs->turn, CP_YELLOW,
                                         "The new magic pushes an old spell from your mind...");
                                log_add(&gs->log, gs->turn, CP_YELLOW,
                                         "You forget %s and learn %s!", old_name, sp->name);
                                gs->spells_known[replaced] = spell_id;
                                for (int j = idx; j < gs->num_items - 1; j++) gs->inventory[j] = gs->inventory[j+1];
                                gs->inventory[--gs->num_items].template_id = -1;
                            }
                        }
                    } else {
                        log_add(&gs->log, gs->turn, CP_GRAY, "You can't use that.");
                    }
                }
            }
        }
        return;
    }

    /* Quest journal (J = Shift+J) */
    if (key == 'J') {
        ui_clear();
        int term_rows, term_cols;
        ui_get_size(&term_rows, &term_cols);

        int row = 1;
        attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
        mvprintw(row++, 2, "=== Quest Journal ===");
        attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
        row++;

        int active = 0, completed = 0;

        /* Main Quest: Holy Grail */
        if (gs->quests.grail_quest_active) {
            attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
            mvprintw(row++, 2, "Main Quest:");
            attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
            if (gs->quests.grail_quest_complete) {
                attron(COLOR_PAIR(CP_GREEN) | A_BOLD);
                mvprintw(row++, 4, "[COMPLETE] The Holy Grail -- Returned to King Arthur!");
                attroff(COLOR_PAIR(CP_GREEN) | A_BOLD);
            } else if (gs->has_grail) {
                attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
                mvprintw(row++, 4, "[GRAIL FOUND] Return the Holy Grail to King Arthur at Camelot Castle!");
                attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
            } else {
                attron(COLOR_PAIR(CP_YELLOW));
                mvprintw(row++, 4, "[ACTIVE] The Holy Grail");
                attroff(COLOR_PAIR(CP_YELLOW));
                attron(COLOR_PAIR(CP_WHITE));
                mvprintw(row++, 6, "Find the Holy Grail and return it to King Arthur.");
                mvprintw(row++, 6, "Ask townfolk and Merlin for hints about its location.");
                attroff(COLOR_PAIR(CP_WHITE));
            }
            row++;
        }

        /* Side quests */
        attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        mvprintw(row++, 2, "Active Quests:");
        attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        for (int i = 0; i < gs->quests.num_quests && row < term_rows - 4; i++) {
            Quest *q = &gs->quests.quests[i];
            if (q->state == QUEST_ACTIVE) {
                attron(COLOR_PAIR(CP_YELLOW));
                const char *tname = (q->type == QTYPE_DELIVERY) ? "Deliver" :
                                    (q->type == QTYPE_FETCH) ? "Fetch" : "Kill";
                mvprintw(row++, 4, "[%s] %s", tname, q->name);
                attroff(COLOR_PAIR(CP_YELLOW));
                attron(COLOR_PAIR(CP_WHITE));
                mvprintw(row++, 6, "%s", q->description);
                if (q->type == QTYPE_DELIVERY)
                    mvprintw(row++, 6, "Deliver to: %s  |  Return to: %s", q->target_town, q->giver_town);
                else if (q->type == QTYPE_FETCH)
                    mvprintw(row++, 6, "Location: %s  |  Return to: %s", q->target_dungeon, q->giver_town);
                else
                    mvprintw(row++, 6, "Return to: %s", q->giver_town);
                mvprintw(row++, 6, "Reward: %dg", q->gold_reward);
                attroff(COLOR_PAIR(CP_WHITE));
                row++;
                active++;
            } else if (q->state == QUEST_COMPLETE) {
                attron(COLOR_PAIR(CP_GREEN));
                mvprintw(row++, 4, "[DONE] %s -- return to %s for reward!", q->name, q->giver_town);
                attroff(COLOR_PAIR(CP_GREEN));
                active++;
            }
        }
        if (active == 0) {
            attron(COLOR_PAIR(CP_GRAY));
            mvprintw(row++, 4, "No active quests. Visit inns to find quest givers (!).");
            attroff(COLOR_PAIR(CP_GRAY));
        }

        row++;
        /* Completed quests */
        attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        mvprintw(row++, 2, "Completed Quests:");
        attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        for (int i = 0; i < gs->quests.num_quests && row < term_rows - 2; i++) {
            Quest *q = &gs->quests.quests[i];
            if (q->state == QUEST_TURNED_IN) {
                attron(COLOR_PAIR(CP_GREEN));
                mvprintw(row++, 4, "[DONE] %s (+%dg)", q->name, q->gold_reward);
                attroff(COLOR_PAIR(CP_GREEN));
                completed++;
            }
        }
        if (completed == 0) {
            attron(COLOR_PAIR(CP_GRAY));
            mvprintw(row++, 4, "None yet.");
            attroff(COLOR_PAIR(CP_GRAY));
        }

        attron(COLOR_PAIR(CP_GRAY));
        mvprintw(term_rows - 1, 2, "Press any key to close.");
        attroff(COLOR_PAIR(CP_GRAY));
        ui_refresh();
        ui_getkey();
        return;
    }

    /* Character sheet (@ key) */
    if (key == '@') {
        ui_clear();
        int row = 1;
        const char *class_names[] = { "Knight", "Wizard", "Ranger" };
        const char *gender_names[] = { "Male", "Female" };
        const char *hair[] = { "Black", "Brown", "Auburn", "Red", "Blonde", "Grey", "White" };
        const char *eyes[] = { "Blue", "Green", "Brown", "Grey", "Hazel", "Amber" };
        const char *build[] = { "Lean", "Average", "Stocky", "Tall", "Short", "Athletic" };
        const char *feature[] = { "Scar across cheek", "Braided hair", "Missing tooth",
                                   "Weathered face", "Freckles", "Piercing gaze",
                                   "Noble bearing", "Calloused hands" };

        attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
        mvprintw(row++, 2, "=== Character Sheet ===");
        attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
        row++;

        attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        mvprintw(row++, 4, "%s %s %s", gender_names[gs->player_gender],
                 class_names[gs->player_class], gs->player_name);
        attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        attron(COLOR_PAIR(CP_GRAY));
        mvprintw(row++, 4, "Level %d  |  %s  |  Chivalry: %d (%s)",
                 gs->player_level, chivalry_title(gs->chivalry),
                 gs->chivalry, chivalry_title(gs->chivalry));
        attroff(COLOR_PAIR(CP_GRAY));
        row++;

        /* Appearance */
        attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        mvprintw(row++, 4, "Appearance:");
        attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        attron(COLOR_PAIR(CP_CYAN));
        mvprintw(row++, 6, "Hair:    %s", hair[gs->appearance[0] % 7]);
        mvprintw(row++, 6, "Eyes:    %s", eyes[gs->appearance[1] % 6]);
        mvprintw(row++, 6, "Build:   %s", build[gs->appearance[2] % 6]);
        mvprintw(row++, 6, "Feature: %s", feature[gs->appearance[3] % 8]);
        attroff(COLOR_PAIR(CP_CYAN));
        row++;

        /* Stats */
        attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        mvprintw(row++, 4, "Stats:");
        attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        attron(COLOR_PAIR(CP_WHITE));
        mvprintw(row++, 6, "HP:  %d/%d    MP:  %d/%d", gs->hp, gs->max_hp, gs->mp, gs->max_mp);
        mvprintw(row++, 6, "STR: %-3d     DEF: %-3d", gs->str, gs->def);
        mvprintw(row++, 6, "INT: %-3d     SPD: %-3d", gs->intel, gs->spd);
        mvprintw(row++, 6, "Gold: %d     Weight: %d/%d", gs->gold, gs->weight, gs->max_weight);
        attroff(COLOR_PAIR(CP_WHITE));
        row++;

        /* Affinities */
        attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        mvprintw(row++, 4, "Affinities:");
        attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        attron(COLOR_PAIR(CP_YELLOW));
        mvprintw(row, 6, "Light: %d", gs->light_affinity);
        attroff(COLOR_PAIR(CP_YELLOW));
        attron(COLOR_PAIR(CP_MAGENTA));
        mvprintw(row, 20, "Dark: %d", gs->dark_affinity);
        attroff(COLOR_PAIR(CP_MAGENTA));
        attron(COLOR_PAIR(CP_GREEN));
        mvprintw(row++, 33, "Nature: %d", gs->nature_affinity);
        attroff(COLOR_PAIR(CP_GREEN));
        row++;

        /* Combat */
        attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        mvprintw(row++, 4, "Combat:");
        attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        attron(COLOR_PAIR(CP_WHITE));
        mvprintw(row++, 6, "XP: %d    Kills: %d    Quests: %d/%d",
                 gs->xp, gs->kills,
                 quest_count_completed(&gs->quests), gs->quests.num_quests);
        if (gs->shield_absorb > 0)
            mvprintw(row++, 6, "Shield absorb: %d remaining", gs->shield_absorb);
        if (gs->horse_type > 0) {
            const char *hnames[] = { "", "Pony", "Palfrey", "Destrier" };
            mvprintw(row++, 6, "Mount: %s%s", hnames[gs->horse_type],
                     gs->riding ? " (riding)" : " (dismounted)");
        }
        if (gs->has_lycanthropy)
            mvprintw(row++, 6, "Curse: LYCANTHROPY (beware the full moon!)");
        if (gs->has_vampirism)
            mvprintw(row++, 6, "Curse: VAMPIRISM (sunlight burns, drain HP on attacks)");
        mvprintw(row++, 6, "Spells: %d/%d known", gs->num_spells, gs->max_spells_capacity);
        attroff(COLOR_PAIR(CP_WHITE));

        int term_rows, term_cols;
        ui_get_size(&term_rows, &term_cols);
        attron(COLOR_PAIR(CP_GRAY));
        mvprintw(term_rows - 1, 2, "Press any key to close.");
        attroff(COLOR_PAIR(CP_GRAY));
        ui_refresh();
        ui_getkey();
        return;
    }

    /* Spellbook (Z = Shift+Z) */
    if (key == 'Z') {
        ui_clear();
        int term_rows, term_cols;
        ui_get_size(&term_rows, &term_cols);
        int row = 1;

        const char *class_names[] = { "Knight", "Wizard", "Ranger" };
        attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
        mvprintw(row++, 2, "=== Spellbook ===");
        attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
        row++;

        attron(COLOR_PAIR(CP_WHITE));
        mvprintw(row++, 2, "Class: %s | MP: %d/%d | Spells: %d/%d",
                 class_names[gs->player_class], gs->mp, gs->max_mp,
                 gs->num_spells, gs->max_spells_capacity);
        attroff(COLOR_PAIR(CP_WHITE));
        row++;

        if (gs->num_spells == 0) {
            attron(COLOR_PAIR(CP_GRAY));
            mvprintw(row++, 4, "You don't know any spells.");
            mvprintw(row++, 4, "Find Spell Scrolls in dungeons and use them to learn.");
            attroff(COLOR_PAIR(CP_GRAY));
        } else {
            /* Column headers */
            attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
            mvprintw(row++, 4, "%-22s %-10s  MP  Dmg  Range  School", "Name", "Effect");
            attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);

            for (int i = 0; i < gs->num_spells && row < term_rows - 3; i++) {
                const SpellDef *sp = spell_get(gs->spells_known[i]);
                if (!sp) continue;

                const char *aff_name;
                short aff_color;
                switch (sp->affiliation) {
                case AFF_LIGHT:   aff_name = "Light";   aff_color = CP_YELLOW; break;
                case AFF_DARK:    aff_name = "Dark";    aff_color = CP_MAGENTA; break;
                case AFF_NATURE:  aff_name = "Nature";  aff_color = CP_GREEN; break;
                default:          aff_name = "Universal"; aff_color = CP_WHITE; break;
                }

                const char *eff_name;
                switch (sp->effect) {
                case SEFF_DAMAGE:    eff_name = "Damage"; break;
                case SEFF_HEAL:      eff_name = "Heal"; break;
                case SEFF_BUFF_DEF:  eff_name = "Buff DEF"; break;
                case SEFF_BUFF_STR:  eff_name = "Buff STR"; break;
                case SEFF_BUFF_SPD:  eff_name = "Buff SPD"; break;
                case SEFF_CURE:      eff_name = "Cure"; break;
                case SEFF_FEAR:      eff_name = "Fear"; break;
                case SEFF_TELEPORT:  eff_name = "Teleport"; break;
                case SEFF_DETECT:    eff_name = "Detect"; break;
                case SEFF_REVEAL:    eff_name = "Reveal"; break;
                case SEFF_DOT:       eff_name = "Poison"; break;
                case SEFF_ROOT:      eff_name = "Root"; break;
                case SEFF_SHIELD:    eff_name = "Shield"; break;
                case SEFF_SUMMON:    eff_name = "Summon"; break;
                case SEFF_DRAIN:     eff_name = "Drain"; break;
                case SEFF_DEBUFF:    eff_name = "Debuff"; break;
                case SEFF_PUSH:      eff_name = "Push"; break;
                case SEFF_INVISIBLE: eff_name = "Invisible"; break;
                case SEFF_WATER_WALK:eff_name = "Water Walk"; break;
                default:             eff_name = "???"; break;
                }

                bool can_cast = (gs->mp >= sp->mp_cost);
                attron(COLOR_PAIR(can_cast ? aff_color : CP_GRAY));
                mvprintw(row++, 4, "[%c] %-20s %-10s %3d  %3d  %5d  %s",
                         'a' + i, sp->name, eff_name, sp->mp_cost,
                         sp->damage, sp->range, aff_name);
                attroff(COLOR_PAIR(can_cast ? aff_color : CP_GRAY));
            }
        }

        row = term_rows - 2;
        attron(COLOR_PAIR(CP_GRAY));
        mvprintw(row++, 2, "Press z to cast a spell. Use Spell Scrolls (i -> u) to learn new spells.");
        mvprintw(row, 2, "Press any key to close.");
        attroff(COLOR_PAIR(CP_GRAY));
        ui_refresh();
        ui_getkey();
        return;
    }

    /* Message history (P = Shift+P) */
    if (key == 'P') {
        ui_clear();
        int term_rows, term_cols;
        ui_get_size(&term_rows, &term_cols);

        int total = gs->log.count;
        int page_size = term_rows - 3;
        int offset = 0;

        while (1) {
            ui_clear();
            attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
            mvprintw(0, 1, "-- Message History (%d messages) -- [Up/Down to scroll, q to close]", total);
            attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);

            for (int i = 0; i < page_size && (offset + i) < total; i++) {
                const LogEntry *entry = log_get(&gs->log, offset + i);
                if (entry) {
                    attron(COLOR_PAIR(entry->color_pair));
                    mvprintw(2 + i, 1, "[%4d] %.*s",
                             entry->turn, term_cols - 10, entry->text);
                    attroff(COLOR_PAIR(entry->color_pair));
                }
            }

            attron(COLOR_PAIR(CP_GRAY));
            mvprintw(term_rows - 1, 1, "Showing %d-%d of %d",
                     offset + 1,
                     (offset + page_size < total) ? offset + page_size : total,
                     total);
            attroff(COLOR_PAIR(CP_GRAY));

            ui_refresh();
            int hkey = ui_getkey();
            if (hkey == 'q' || hkey == 'Q' || hkey == 27 || hkey == 'P') break;
            if ((hkey == KEY_DOWN || hkey == 'j') && offset + page_size < total) offset++;
            if ((hkey == KEY_UP || hkey == 'k') && offset > 0) offset--;
            if (hkey == KEY_NPAGE) { offset += page_size; if (offset + page_size > total) offset = total - page_size; if (offset < 0) offset = 0; }
            if (hkey == KEY_PPAGE) { offset -= page_size; if (offset < 0) offset = 0; }
        }
        return;
    }

    /* Wait */
    if (key == '.' || key == '5') {
        int wait_mins = (gs->mode == MODE_OVERWORLD) ? 10 : 1;
        advance_time(gs, wait_mins);
        log_add(&gs->log, gs->turn, CP_WHITE, "You wait...");
        return;
    }

    switch (gs->mode) {
    case MODE_OVERWORLD:
        handle_overworld_input(gs, key);
        break;
    case MODE_TOWN:
        handle_town_input(gs, key);
        break;
    case MODE_DUNGEON:
        handle_dungeon_input(gs, key);
        break;
    default:
        break;
    }
}

/* ------------------------------------------------------------------ */
/* Game init                                                           */
/* ------------------------------------------------------------------ */

/* Chivalry title based on current score */
static const char *chivalry_title(int chiv) {
    if (chiv >= 86) return "Paragon of Virtue";
    if (chiv >= 71) return "Champion";
    if (chiv >= 51) return "Noble Knight";
    if (chiv >= 31) return "Knight";
    if (chiv >= 16) return "Squire";
    return "Knave";
}

/* ------------------------------------------------------------------ */
/* Give starting gear based on class                                   */
/* ------------------------------------------------------------------ */
static void give_starting_gear(GameState *gs) {
    int tcount;
    const ItemTemplate *tmps = item_get_templates(&tcount);
    const char *weapon_name, *armor_name;

    switch (gs->player_class) {
    case CLASS_KNIGHT:
        weapon_name = "Longsword";
        armor_name = "Chainmail";
        break;
    case CLASS_WIZARD:
        weapon_name = "Wooden Staff";
        armor_name = "Tattered Robes";
        break;
    case CLASS_RANGER:
        weapon_name = "Short Sword";
        armor_name = "Leather Armor";
        break;
    }

    for (int i = 0; i < tcount; i++) {
        if (strcmp(tmps[i].name, weapon_name) == 0 && gs->equipment[SLOT_WEAPON].template_id < 0) {
            gs->equipment[SLOT_WEAPON] = item_create(i, -1, -1);
            gs->equipment[SLOT_WEAPON].on_ground = false;
        }
        if (strcmp(tmps[i].name, armor_name) == 0 && gs->equipment[SLOT_ARMOR].template_id < 0) {
            gs->equipment[SLOT_ARMOR] = item_create(i, -1, -1);
            gs->equipment[SLOT_ARMOR].on_ground = false;
        }
        if (strcmp(tmps[i].name, "Bread") == 0 && gs->num_items < 3) {
            gs->inventory[gs->num_items] = item_create(i, -1, -1);
            gs->inventory[gs->num_items].on_ground = false;
            gs->num_items++;
        }
        if (strcmp(tmps[i].name, "Torch") == 0 && gs->num_items < 4) {
            gs->inventory[gs->num_items] = item_create(i, -1, -1);
            gs->inventory[gs->num_items].on_ground = false;
            gs->num_items++;
        }
    }

    /* Give starting spell */
    int scount;
    const SpellDef *spells = spell_get_defs(&scount);
    const char *start_spell;
    switch (gs->player_class) {
    case CLASS_KNIGHT: start_spell = "Shield"; break;
    case CLASS_WIZARD: start_spell = "Magic Missile"; break;
    case CLASS_RANGER: start_spell = "Detect Traps"; break;
    }
    for (int i = 0; i < scount; i++) {
        if (strcmp(spells[i].name, start_spell) == 0) {
            gs->spells_known[gs->num_spells++] = i;
            break;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Finalize character creation and start the game                      */
/* ------------------------------------------------------------------ */
static void finalize_character(GameState *gs) {
    /* Stats were already set by roll_stats() including class/gender bonuses.
       Just set spell capacity here. */
    switch (gs->player_class) {
    case CLASS_KNIGHT: gs->max_spells_capacity = 4;  break;
    case CLASS_WIZARD: gs->max_spells_capacity = 15; break;
    case CLASS_RANGER: gs->max_spells_capacity = 6;  break;
    }

    gs->hp = gs->max_hp;
    gs->mp = gs->max_mp;
    gs->max_weight = 30 + gs->str * 3 + gs->player_level * 2;

    /* Initialize inventory */
    gs->num_items = 0;
    for (int i = 0; i < MAX_INVENTORY; i++) gs->inventory[i].template_id = -1;
    for (int i = 0; i < NUM_SLOTS; i++) gs->equipment[i].template_id = -1;

    give_starting_gear(gs);

    /* Initialize overworld */
    gs->overworld = calloc(1, sizeof(Overworld));
    overworld_init(gs->overworld);
    overworld_spawn_creatures(gs->overworld);
    town_init();
    quest_init(&gs->quests);

    /* Generate treasure map locations on passable overworld tiles */
    gs->num_treasure_maps = rng_range(5, 8);
    for (int i = 0; i < gs->num_treasure_maps; i++) {
        gs->treasure_found[i] = 0;
        for (int tries = 0; tries < 200; tries++) {
            int tx = rng_range(50, OW_WIDTH - 50);
            int ty = rng_range(20, OW_HEIGHT - 20);
            if (gs->overworld->map[ty][tx].passable &&
                !overworld_location_at(gs->overworld, tx, ty)) {
                gs->treasure_spots[i] = (Vec2){ tx, ty };
                break;
            }
        }
    }

    /* Place player at Camelot */
    gs->player_pos = (Vec2){ 211, 162 };
    gs->ow_player_pos = gs->player_pos;

    gs->mode = MODE_OVERWORLD;

    log_init(&gs->log);
    log_add(&gs->log, gs->turn, CP_WHITE,
            "Welcome to Knights of Camelot, %s! Press ? for help.",
            gs->player_name);
    log_add(&gs->log, gs->turn, CP_YELLOW,
            "You stand at the gates of Camelot. Seek King Arthur.");
}

/* ------------------------------------------------------------------ */
/* Random name loading                                                 */
/* ------------------------------------------------------------------ */
static char male_names[60][MAX_NAME];
static char female_names[60][MAX_NAME];
static int num_male_names = 0, num_female_names = 0;

static void load_names(void) {
    if (num_male_names > 0) return; /* already loaded */

    FILE *f = fopen("data/names.csv", "r");
    if (!f) {
        /* Fallback */
        snprintf(male_names[0], MAX_NAME, "Galahad"); num_male_names = 1;
        snprintf(female_names[0], MAX_NAME, "Elaine"); num_female_names = 1;
        return;
    }
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        line[strcspn(line, "\n\r")] = 0;
        char name[48], gender[16];
        if (sscanf(line, "%47[^,],%15s", name, gender) < 2) continue;
        if (strcmp(gender, "male") == 0 && num_male_names < 60)
            snprintf(male_names[num_male_names++], MAX_NAME, "%s", name);
        else if (strcmp(gender, "female") == 0 && num_female_names < 60)
            snprintf(female_names[num_female_names++], MAX_NAME, "%s", name);
    }
    fclose(f);
}

static const char *random_name(PlayerGender g) {
    load_names();
    if (g == GENDER_MALE && num_male_names > 0)
        return male_names[rng_range(0, num_male_names - 1)];
    if (g == GENDER_FEMALE && num_female_names > 0)
        return female_names[rng_range(0, num_female_names - 1)];
    return "Adventurer";
}

/* ------------------------------------------------------------------ */
/* Character creation screen rendering and input                       */
/* ------------------------------------------------------------------ */
static void render_character_create(GameState *gs) {
    ui_clear();
    int row = 2;

    attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
    mvprintw(0, 2, "=== Knights of Camelot - Character Creation ===");
    attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);

    if (gs->create_step == 0) {
        /* Class selection */
        attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        mvprintw(row, 4, "Choose your class:");
        attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        row += 2;
        attron(COLOR_PAIR(CP_CYAN));
        mvprintw(row++, 6, "[1] Knight  - Warrior of the realm. Strong and tough.");
        mvprintw(row++, 6, "              +2 STR, +2 DEF. HP:30  MP:8   Max spells: 4");
        row++;
        mvprintw(row++, 6, "[2] Wizard  - Master of the arcane arts. Powerful magic.");
        mvprintw(row++, 6, "              +3 INT, +1 SPD. HP:18  MP:30  Max spells: 15");
        row++;
        mvprintw(row++, 6, "[3] Ranger  - Swift and versatile. Balanced fighter.");
        mvprintw(row++, 6, "              +1 STR, +2 SPD. HP:24  MP:15  Max spells: 6");
        attroff(COLOR_PAIR(CP_CYAN));
    } else if (gs->create_step == 1) {
        /* Gender selection */
        attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        mvprintw(row, 4, "Choose your gender:");
        attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        row += 2;
        attron(COLOR_PAIR(CP_CYAN));
        mvprintw(row++, 6, "[1] Male   - +1 STR, +1 DEF");
        row++;
        mvprintw(row++, 6, "[2] Female - +1 INT, +1 SPD");
        attroff(COLOR_PAIR(CP_CYAN));
    } else if (gs->create_step == 2) {
        /* Name entry */
        attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        mvprintw(row, 4, "Enter your name:");
        attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        row += 2;
        attron(COLOR_PAIR(CP_WHITE));
        mvprintw(row, 6, "> %s_", gs->player_name);
        attroff(COLOR_PAIR(CP_WHITE));
        row += 2;
        attron(COLOR_PAIR(CP_GRAY));
        mvprintw(row++, 6, "Type a name and press Enter.");
        mvprintw(row++, 6, "Press [r] for a random name.  Press Backspace to delete.");
        attroff(COLOR_PAIR(CP_GRAY));
    } else if (gs->create_step == 3) {
        /* Appearance */
        const char *hair[] = { "Black", "Brown", "Auburn", "Red", "Blonde", "Grey", "White" };
        const char *eyes[] = { "Blue", "Green", "Brown", "Grey", "Hazel", "Amber" };
        const char *build[] = { "Lean", "Average", "Stocky", "Tall", "Short", "Athletic" };
        const char *feature[] = { "Scar across cheek", "Braided hair", "Missing tooth",
                                   "Weathered face", "Freckles", "Piercing gaze",
                                   "Noble bearing", "Calloused hands" };
        int nhair = 7, neyes = 6, nbuild = 6, nfeat = 8;

        attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        mvprintw(row, 4, "Customise your appearance:");
        attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        row += 2;
        attron(COLOR_PAIR(CP_CYAN));
        mvprintw(row++, 6, "[1] Hair:    %-12s", hair[gs->appearance[0] % nhair]);
        mvprintw(row++, 6, "[2] Eyes:    %-12s", eyes[gs->appearance[1] % neyes]);
        mvprintw(row++, 6, "[3] Build:   %-12s", build[gs->appearance[2] % nbuild]);
        mvprintw(row++, 6, "[4] Feature: %-20s", feature[gs->appearance[3] % nfeat]);
        attroff(COLOR_PAIR(CP_CYAN));
        row++;
        attron(COLOR_PAIR(CP_GRAY));
        mvprintw(row++, 6, "Press 1-4 to cycle options. [r] to randomise all.");
        mvprintw(row++, 6, "Press [Enter] to continue.");
        attroff(COLOR_PAIR(CP_GRAY));
    } else if (gs->create_step == 4) {
        /* Stats roll */
        const char *class_name[] = { "Knight", "Wizard", "Ranger" };
        const char *gender_name[] = { "Male", "Female" };
        const char *hair[] = { "Black", "Brown", "Auburn", "Red", "Blonde", "Grey", "White" };
        const char *eyes[] = { "Blue", "Green", "Brown", "Grey", "Hazel", "Amber" };
        const char *build[] = { "Lean", "Average", "Stocky", "Tall", "Short", "Athletic" };
        attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        mvprintw(row, 4, "%s - %s %s", gs->player_name,
                 gender_name[gs->player_gender], class_name[gs->player_class]);
        attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        row++;
        attron(COLOR_PAIR(CP_GRAY));
        mvprintw(row, 6, "%s hair, %s eyes, %s",
                 hair[gs->appearance[0] % 7], eyes[gs->appearance[1] % 6],
                 build[gs->appearance[2] % 6]);
        attroff(COLOR_PAIR(CP_GRAY));
        row += 2;
        attron(COLOR_PAIR(CP_CYAN));
        mvprintw(row++, 6, "STR: %-3d  DEF: %-3d", gs->str, gs->def);
        mvprintw(row++, 6, "INT: %-3d  SPD: %-3d", gs->intel, gs->spd);
        row++;
        mvprintw(row++, 6, "HP:  %-3d  MP:  %-3d", gs->max_hp, gs->max_mp);
        mvprintw(row++, 6, "Gold: %d", gs->gold);
        attroff(COLOR_PAIR(CP_CYAN));
        row++;
        attron(COLOR_PAIR(CP_GRAY));
        mvprintw(row++, 6, "Press [r] to re-roll stats.");
        mvprintw(row++, 6, "Press [Enter] to accept and begin.");
        mvprintw(row++, 6, "Press [C] for cheat options.");
        attroff(COLOR_PAIR(CP_GRAY));
    } else if (gs->create_step == 5) {
        /* Story screen */
        row = 4;
        attron(COLOR_PAIR(CP_YELLOW));
        mvprintw(row++, 4, "The kingdom of Camelot is in peril...");
        row++;
        mvprintw(row++, 4, "Dark forces gather in the shadows. Ancient evils stir");
        mvprintw(row++, 4, "beneath the hills of England. The Holy Grail, source of");
        mvprintw(row++, 4, "the land's power, has been lost.");
        row++;
        mvprintw(row++, 4, "King Arthur has summoned you to his court.");
        mvprintw(row++, 4, "Seek an audience with the King in the throne room");
        mvprintw(row++, 4, "of Camelot Castle to learn of your destiny.");
        row++;
        mvprintw(row++, 4, "Your journey begins at the gates of Camelot...");
        attroff(COLOR_PAIR(CP_YELLOW));
        row += 2;
        attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
        mvprintw(row, 4, "Press any key to begin your adventure.");
        attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
    }

    ui_refresh();
}

static void roll_stats(GameState *gs) {
    gs->str = rng_range(3, 8);
    gs->def = rng_range(3, 8);
    gs->intel = rng_range(3, 8);
    gs->spd = rng_range(3, 8);

    /* Apply class bonuses with random HP/MP variation */
    switch (gs->player_class) {
    case CLASS_KNIGHT:
        gs->str += 2; gs->def += 2;
        gs->max_hp = rng_range(26, 34); gs->max_mp = rng_range(6, 10);
        break;
    case CLASS_WIZARD:
        gs->intel += 3; gs->spd += 1;
        gs->max_hp = rng_range(14, 22); gs->max_mp = rng_range(26, 34);
        break;
    case CLASS_RANGER:
        gs->str += 1; gs->spd += 2;
        gs->max_hp = rng_range(20, 28); gs->max_mp = rng_range(12, 18);
        break;
    }

    /* Gender bonuses */
    if (gs->player_gender == GENDER_MALE) {
        gs->str += 1; gs->def += 1;
    } else {
        gs->intel += 1; gs->spd += 1;
    }

    gs->hp = gs->max_hp;
    gs->mp = gs->max_mp;
    gs->gold = rng_range(30, 200);
}

static void handle_character_create(GameState *gs, int key) {
    if (gs->create_step == 0) {
        /* Class */
        if (key == '1') { gs->player_class = CLASS_KNIGHT; gs->create_step = 1; }
        else if (key == '2') { gs->player_class = CLASS_WIZARD; gs->create_step = 1; }
        else if (key == '3') { gs->player_class = CLASS_RANGER; gs->create_step = 1; }
    } else if (gs->create_step == 1) {
        /* Gender */
        if (key == '1') { gs->player_gender = GENDER_MALE; gs->create_step = 2; gs->player_name[0] = '\0'; }
        else if (key == '2') { gs->player_gender = GENDER_FEMALE; gs->create_step = 2; gs->player_name[0] = '\0'; }
    } else if (gs->create_step == 2) {
        /* Name */
        int len = (int)strlen(gs->player_name);
        if (key == '\n' || key == '\r' || key == KEY_ENTER) {
            if (len > 0) {
                gs->create_step = 3;
                /* Randomise initial appearance */
                for (int i = 0; i < 4; i++) gs->appearance[i] = rng_range(0, 20);
            }
        } else if (key == 'r') {
            snprintf(gs->player_name, MAX_NAME, "%s", random_name(gs->player_gender));
        } else if (key == KEY_BACKSPACE || key == 127 || key == 8) {
            if (len > 0) gs->player_name[len - 1] = '\0';
        } else if (key >= 32 && key < 127 && len < MAX_NAME - 1) {
            gs->player_name[len] = (char)key;
            gs->player_name[len + 1] = '\0';
        }
    } else if (gs->create_step == 3) {
        /* Appearance */
        if (key == '1') gs->appearance[0]++;
        else if (key == '2') gs->appearance[1]++;
        else if (key == '3') gs->appearance[2]++;
        else if (key == '4') gs->appearance[3]++;
        else if (key == 'r') { for (int i = 0; i < 4; i++) gs->appearance[i] = rng_range(0, 20); }
        else if (key == '\n' || key == '\r' || key == KEY_ENTER) {
            gs->create_step = 4;
            roll_stats(gs);
        }
    } else if (gs->create_step == 4) {
        /* Stats */
        if (key == 'r') {
            roll_stats(gs);
        } else if (key == 'C') {
            /* Cheat mode! */
            ui_clear();
            int row = 2;
            attron(COLOR_PAIR(CP_RED_BOLD) | A_BOLD);
            mvprintw(row++, 4, "=== CHEAT MODE ===");
            attroff(COLOR_PAIR(CP_RED_BOLD) | A_BOLD);
            row++;
            attron(COLOR_PAIR(CP_YELLOW));
            mvprintw(row++, 6, "WARNING: Enabling cheats sets your score to 0");
            mvprintw(row++, 6, "and marks your character as CHEAT permanently.");
            attroff(COLOR_PAIR(CP_YELLOW));
            row++;
            attron(COLOR_PAIR(CP_CYAN));
            mvprintw(row++, 6, "[1] God Mode (HP 999)");
            mvprintw(row++, 6, "[2] Rich Start (Gold 9999)");
            mvprintw(row++, 6, "[3] Max Stats (all 15)");
            mvprintw(row++, 6, "[4] All of the above");
            mvprintw(row++, 6, "[q] Cancel -- no cheats");
            attroff(COLOR_PAIR(CP_CYAN));
            ui_refresh();

            int ck = ui_getkey();
            if (ck >= '1' && ck <= '4') {
                gs->cheat_mode = true;
                if (ck == '1' || ck == '4') {
                    gs->max_hp = 999; gs->hp = 999;
                    gs->max_mp = 999; gs->mp = 999;
                }
                if (ck == '2' || ck == '4') {
                    gs->gold = 9999;
                }
                if (ck == '3' || ck == '4') {
                    gs->str = 15; gs->def = 15; gs->intel = 15; gs->spd = 15;
                }
            }
        } else if (key == '\n' || key == '\r' || key == KEY_ENTER) {
            gs->create_step = 5;
        }
    } else if (gs->create_step == 5) {
        /* Story -- any key starts game */
        finalize_character(gs);
    }
}

void game_init(GameState *gs) {
    memset(gs, 0, sizeof(*gs));

    /* Initialize data */
    entity_init();
    item_init();
    spell_init();

    /* Default state */
    gs->player_level = 1;
    gs->gold = 50;
    gs->chivalry = 25;
    gs->turn = 0;
    gs->day = 1;
    gs->hour = 8;
    gs->minute = 0;
    gs->weather = WEATHER_CLEAR;
    gs->weather_turns_left = rng_range(100, 200);
    gs->has_torch = false;
    gs->torch_fuel = 0;
    gs->torch_type = 0;
    gs->dungeon = NULL;
    gs->running = true;

    /* Initialize spell slots as empty */
    for (int i = 0; i < MAX_SPELLS; i++) gs->spells_known[i] = -1;
    gs->num_spells = 0;

    /* Randomise potion colour descriptions */
    {
        int tcount;
        const ItemTemplate *tmps = item_get_templates(&tcount);
        /* Collect potion template IDs */
        int potion_ids[64];
        int num_potions = 0;
        for (int i = 0; i < tcount && num_potions < 64; i++) {
            if (tmps[i].type == ITYPE_POTION)
                potion_ids[num_potions++] = i;
        }
        /* Create a shuffled colour assignment */
        int colours[64];
        for (int i = 0; i < num_potions; i++) colours[i] = i;
        for (int i = num_potions - 1; i > 0; i--) {
            int j = rng_range(0, i);
            int tmp = colours[i]; colours[i] = colours[j]; colours[j] = tmp;
        }
        for (int i = 0; i < num_potions; i++)
            gs->potion_colour_map[potion_ids[i]] = colours[i];
    }

    /* Start in character creation */
    gs->mode = MODE_CHARACTER_CREATE;
    gs->create_step = 0;
}

void game_update(GameState *gs) {
    /* Tick weather countdown */
    if (gs->mode == MODE_OVERWORLD) {
        gs->weather_turns_left--;
        if (gs->weather_turns_left <= 0) {
            change_weather(gs);
        }
    }
}

/* ------------------------------------------------------------------ */
/* Rendering                                                           */
/* ------------------------------------------------------------------ */

static void game_render(GameState *gs) {
    /* Character creation has its own rendering */
    if (gs->mode == MODE_CHARACTER_CREATE) {
        render_character_create(gs);
        return;
    }

    ui_clear();

    int term_rows, term_cols;
    ui_get_size(&term_rows, &term_cols);

    bool show_sidebar = (term_cols >= MIN_TERM_WIDTH);
    int map_view_width = show_sidebar ? (term_cols - SIDEBAR_WIDTH - 1) : term_cols;
    int map_view_height = term_rows - LOG_LINES - 2;
    if (map_view_height > VIEW_HEIGHT_DEFAULT)
        map_view_height = VIEW_HEIGHT_DEFAULT;
    if (map_view_height < 5) map_view_height = 5;

    /* Render the appropriate map / screen */
    if (gs->mode == MODE_TOWN) {
        ui_clear();
        town_render(gs);

        /* Town name header */
        if (gs->current_town) {
            attron(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);
            mvprintw(0, TOWN_MAP_W + 2, "%s", gs->current_town->name);
            attroff(COLOR_PAIR(CP_YELLOW_BOLD) | A_BOLD);

            attron(COLOR_PAIR(CP_WHITE));
            mvprintw(2, TOWN_MAP_W + 2, "HP: %d/%d", gs->hp, gs->max_hp);
            mvprintw(3, TOWN_MAP_W + 2, "MP: %d/%d", gs->mp, gs->max_mp);
            mvprintw(4, TOWN_MAP_W + 2, "Gold: %d", gs->gold);
            mvprintw(5, TOWN_MAP_W + 2, "Chiv: %d (%s)", gs->chivalry, chivalry_title(gs->chivalry));
            mvprintw(7, TOWN_MAP_W + 2, "Day %d %02d:%02d",
                     gs->day, gs->hour, gs->minute);
            mvprintw(8, TOWN_MAP_W + 2, "%s", time_of_day_name(gs->hour));
            mvprintw(10, TOWN_MAP_W + 2, "Bump into NPCs");
            mvprintw(11, TOWN_MAP_W + 2, "to interact.");
            mvprintw(12, TOWN_MAP_W + 2, "q = leave town");
            attroff(COLOR_PAIR(CP_WHITE));
        }

        ui_render_log(&gs->log, TOWN_MAP_H + 1, term_cols, 3);
        ui_refresh();
        return;
    }

    if (gs->mode == MODE_OVERWORLD) {
        /* Clamp view to overworld dimensions */
        if (map_view_width > OW_WIDTH) map_view_width = OW_WIDTH;
        if (map_view_height > OW_HEIGHT) map_view_height = OW_HEIGHT;

        ui_render_map_generic((Tile *)gs->overworld->map, OW_WIDTH, OW_HEIGHT,
                              gs->player_pos, map_view_width, map_view_height);

        /* Override player glyph when mounted */
        if (gs->riding) {
            int cam_x_p = gs->player_pos.x - map_view_width / 2;
            int cam_y_p = gs->player_pos.y - map_view_height / 2;
            if (cam_x_p < 0) cam_x_p = 0;
            if (cam_y_p < 0) cam_y_p = 0;
            if (cam_x_p + map_view_width > OW_WIDTH) cam_x_p = OW_WIDTH - map_view_width;
            if (cam_y_p + map_view_height > OW_HEIGHT) cam_y_p = OW_HEIGHT - map_view_height;
            int ppx = gs->player_pos.x - cam_x_p;
            int ppy = gs->player_pos.y - cam_y_p;
            if (ppx >= 0 && ppx < map_view_width && ppy >= 0 && ppy < map_view_height) {
                attron(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
                mvaddch(ppy, ppx, 'H');
                attroff(COLOR_PAIR(CP_WHITE_BOLD) | A_BOLD);
            }
        }

        /* Draw cat trailing behind player on overworld */
        if (gs->has_cat) {
            int cam_x_c = gs->player_pos.x - map_view_width / 2;
            int cam_y_c = gs->player_pos.y - map_view_height / 2;
            if (cam_x_c < 0) cam_x_c = 0;
            if (cam_y_c < 0) cam_y_c = 0;
            if (cam_x_c + map_view_width > OW_WIDTH) cam_x_c = OW_WIDTH - map_view_width;
            if (cam_y_c + map_view_height > OW_HEIGHT) cam_y_c = OW_HEIGHT - map_view_height;
            int cx = gs->cat_pos.x - cam_x_c;
            int cy = gs->cat_pos.y - cam_y_c;
            if (cx >= 0 && cx < map_view_width && cy >= 0 && cy < map_view_height &&
                (gs->cat_pos.x != gs->player_pos.x || gs->cat_pos.y != gs->player_pos.y)) {
                attron(COLOR_PAIR(CP_YELLOW) | A_BOLD);
                mvaddch(cy, cx, 'c');
                attroff(COLOR_PAIR(CP_YELLOW) | A_BOLD);
            }
        }

        /* Draw wandering creatures on the overworld -- only within sight radius */
        {
            int cam_x = gs->player_pos.x - map_view_width / 2;
            int cam_y = gs->player_pos.y - map_view_height / 2;
            if (cam_x < 0) cam_x = 0;
            if (cam_y < 0) cam_y = 0;
            if (cam_x + map_view_width > OW_WIDTH) cam_x = OW_WIDTH - map_view_width;
            if (cam_y + map_view_height > OW_HEIGHT) cam_y = OW_HEIGHT - map_view_height;

            /* Sight radius varies by time of day and weather */
            int sight_radius;
            TimeOfDay tod = game_get_tod(gs);
            switch (tod) {
            case TOD_NIGHT:     sight_radius = 8;  break;
            case TOD_EVENING:   sight_radius = 12; break;
            case TOD_DAWN:
            case TOD_DUSK:      sight_radius = 15; break;
            default:            sight_radius = 20; break;
            }
            /* Weather reduces sight */
            if (gs->weather == WEATHER_FOG) sight_radius = sight_radius / 2;
            else if (gs->weather == WEATHER_RAIN) sight_radius = sight_radius * 3 / 4;
            else if (gs->weather == WEATHER_STORM) sight_radius = sight_radius / 2;
            else if (gs->weather == WEATHER_SNOW) sight_radius = sight_radius * 3 / 4;

            /* Friendly creatures visible slightly further, hostiles harder to spot */
            for (int i = 0; i < gs->overworld->num_creatures; i++) {
                OWCreature *c = &gs->overworld->creatures[i];
                int dx = c->pos.x - gs->player_pos.x;
                int dy = c->pos.y - gs->player_pos.y;
                /* Aspect correction for terminal chars */
                int adx = dx / 2;
                int dist_sq = adx * adx + dy * dy;

                int vis_range = sight_radius;
                if (c->hostile) vis_range = sight_radius * 3 / 4;  /* enemies harder to spot */

                if (dist_sq > vis_range * vis_range) continue;

                int sx = c->pos.x - cam_x;
                int sy = c->pos.y - cam_y;
                if (sx >= 0 && sx < map_view_width && sy >= 0 && sy < map_view_height) {
                    attron(COLOR_PAIR(c->color_pair));
                    mvaddch(sy, sx, c->glyph);
                    attroff(COLOR_PAIR(c->color_pair));
                }
            }
        }

        /* Draw rainbow end marker if active */
        if (gs->rainbow_active) {
            int cam_x2 = gs->player_pos.x - map_view_width / 2;
            int cam_y2 = gs->player_pos.y - map_view_height / 2;
            if (cam_x2 < 0) cam_x2 = 0;
            if (cam_y2 < 0) cam_y2 = 0;
            if (cam_x2 + map_view_width > OW_WIDTH) cam_x2 = OW_WIDTH - map_view_width;
            if (cam_y2 + map_view_height > OW_HEIGHT) cam_y2 = OW_HEIGHT - map_view_height;

            int rx = gs->rainbow_end.x - cam_x2;
            int ry = gs->rainbow_end.y - cam_y2;
            if (rx >= 0 && rx < map_view_width && ry >= 0 && ry < map_view_height) {
                /* Rotating rainbow colours based on turn counter */
                short rainbow_colors[] = {
                    CP_RED, CP_YELLOW, CP_GREEN, CP_CYAN,
                    CP_BLUE, CP_MAGENTA, CP_WHITE
                };
                short rc = rainbow_colors[gs->turn % 7];
                attron(COLOR_PAIR(rc) | A_BOLD);
                mvaddch(ry, rx, '=');
                attroff(COLOR_PAIR(rc) | A_BOLD);
            }
        }

        /* Apply night/dusk/dawn dimming -- spare a radius around the player */
        {
            TimeOfDay tod = game_get_tod(gs);
            bool do_dim = (tod == TOD_NIGHT || tod == TOD_EVENING ||
                           tod == TOD_DUSK  || tod == TOD_DAWN);
            if (do_dim) {
                /* Player position in viewport coordinates */
                int cam_x = gs->player_pos.x - map_view_width / 2;
                int cam_y = gs->player_pos.y - map_view_height / 2;
                if (cam_x < 0) cam_x = 0;
                if (cam_y < 0) cam_y = 0;
                if (cam_x + map_view_width > OW_WIDTH) cam_x = OW_WIDTH - map_view_width;
                if (cam_y + map_view_height > OW_HEIGHT) cam_y = OW_HEIGHT - map_view_height;
                int px = gs->player_pos.x - cam_x;
                int py = gs->player_pos.y - cam_y;

                /* Light radius: larger at dusk/dawn, smaller at night */
                int light_r;
                switch (tod) {
                case TOD_DAWN:    light_r = 6; break;
                case TOD_DUSK:    light_r = 5; break;
                case TOD_EVENING: light_r = 4; break;
                case TOD_NIGHT:   light_r = 3; break;
                default:          light_r = 6; break;
                }

                for (int vy = 0; vy < map_view_height; vy++) {
                    for (int vx = 0; vx < map_view_width; vx++) {
                        int dx = vx - px;
                        int dy = vy - py;
                        /* Aspect correction: terminal chars are ~2x taller than wide */
                        int adx = dx / 2;
                        int dist_sq = adx * adx + dy * dy;
                        if (dist_sq <= light_r * light_r) continue; /* inside light radius */

                        chtype ch = mvinch(vy, vx);
                        if ((ch & A_CHARTEXT) == '@') continue;
                        mvaddch(vy, vx, (ch & ~A_COLOR) | A_DIM | (ch & A_COLOR));
                    }
                }
            }
        }
    } else if (gs->mode == MODE_DUNGEON) {
        if (map_view_width > MAP_WIDTH) map_view_width = MAP_WIDTH;

        Tile (*dtiles)[MAP_WIDTH] = current_dungeon_tiles(gs);
        if (dtiles) {
            ui_render_map_generic((Tile *)dtiles, MAP_WIDTH, MAP_HEIGHT,
                                  gs->player_pos, map_view_width, map_view_height);

            /* Torch light effect -- tint visible tiles near player yellow */
            if (gs->has_torch) {
                int cam_x = gs->player_pos.x - map_view_width / 2;
                int cam_y = gs->player_pos.y - map_view_height / 2;
                if (cam_x < 0) cam_x = 0;
                if (cam_y < 0) cam_y = 0;
                if (cam_x + map_view_width > MAP_WIDTH) cam_x = MAP_WIDTH - map_view_width;
                if (cam_y + map_view_height > MAP_HEIGHT) cam_y = MAP_HEIGHT - map_view_height;

                int px = gs->player_pos.x - cam_x;
                int py = gs->player_pos.y - cam_y;

                for (int vy = 0; vy < map_view_height; vy++) {
                    for (int vx = 0; vx < map_view_width; vx++) {
                        int mx = cam_x + vx, my = cam_y + vy;
                        if (mx < 0 || mx >= MAP_WIDTH || my < 0 || my >= MAP_HEIGHT) continue;
                        Tile *t = &dtiles[my][mx];
                        if (!t->visible) continue;

                        int dx = vx - px, dy = vy - py;
                        /* Aspect correction for terminal chars */
                        int adx = dx / 2;
                        int dist_sq = adx * adx + dy * dy;

                        /* Inner glow: bright yellow close to player */
                        if (dist_sq <= 4) {
                            chtype ch = mvinch(vy, vx);
                            if ((ch & A_CHARTEXT) != '@') {
                                mvaddch(vy, vx, (ch & A_CHARTEXT) | COLOR_PAIR(CP_YELLOW) | A_BOLD);
                            }
                        }
                        /* Warm tint in mid range */
                        else if (dist_sq <= 25) {
                            chtype ch = mvinch(vy, vx);
                            if ((ch & A_CHARTEXT) != '@') {
                                mvaddch(vy, vx, (ch & A_CHARTEXT) | COLOR_PAIR(CP_YELLOW));
                            }
                        }
                        /* Outer edge: dimmer */
                        else if (dist_sq <= FOV_RADIUS * FOV_RADIUS) {
                            /* Leave as-is -- normal colour at edge of torchlight */
                        }
                    }
                }
            }

            /* Render ground items (visible in FOV) */
            {
                DungeonLevel *dl_items = current_dungeon_level(gs);
                if (dl_items) {
                    int icam_x = gs->player_pos.x - map_view_width / 2;
                    int icam_y = gs->player_pos.y - map_view_height / 2;
                    if (icam_x < 0) icam_x = 0;
                    if (icam_y < 0) icam_y = 0;
                    if (icam_x + map_view_width > MAP_WIDTH) icam_x = MAP_WIDTH - map_view_width;
                    if (icam_y + map_view_height > MAP_HEIGHT) icam_y = MAP_HEIGHT - map_view_height;

                    for (int i = 0; i < dl_items->num_ground_items; i++) {
                        Item *it = &dl_items->ground_items[i];
                        if (!it->on_ground) continue;
                        if (!dl_items->tiles[it->pos.y][it->pos.x].visible) continue;
                        /* Don't draw items under the player */
                        if (it->pos.x == gs->player_pos.x && it->pos.y == gs->player_pos.y) continue;

                        int sx = it->pos.x - icam_x;
                        int sy = it->pos.y - icam_y;
                        if (sx >= 0 && sx < map_view_width && sy >= 0 && sy < map_view_height) {
                            attron(COLOR_PAIR(it->color_pair) | A_BOLD);
                            mvaddch(sy, sx, it->glyph);
                            attroff(COLOR_PAIR(it->color_pair) | A_BOLD);
                        }
                    }
                }
            }

            /* Render monsters AFTER torch tint so they keep their colours */
            {
                DungeonLevel *dl2 = current_dungeon_level(gs);
                if (dl2) {
                    int mcam_x = gs->player_pos.x - map_view_width / 2;
                    int mcam_y = gs->player_pos.y - map_view_height / 2;
                    if (mcam_x < 0) mcam_x = 0;
                    if (mcam_y < 0) mcam_y = 0;
                    if (mcam_x + map_view_width > MAP_WIDTH) mcam_x = MAP_WIDTH - map_view_width;
                    if (mcam_y + map_view_height > MAP_HEIGHT) mcam_y = MAP_HEIGHT - map_view_height;

                    for (int i = 0; i < dl2->num_monsters; i++) {
                        Entity *e = &dl2->monsters[i];
                        if (!e->alive) continue;
                        if (!dl2->tiles[e->pos.y][e->pos.x].visible) continue;

                        int sx = e->pos.x - mcam_x;
                        int sy = e->pos.y - mcam_y;
                        if (sx >= 0 && sx < map_view_width && sy >= 0 && sy < map_view_height) {
                            attron(COLOR_PAIR(e->color_pair) | A_BOLD);
                            mvaddch(sy, sx, e->glyph);
                            attroff(COLOR_PAIR(e->color_pair) | A_BOLD);
                        }
                    }
                }
            }

            /* Draw cat trailing behind player in dungeon */
            if (gs->has_cat) {
                int dcam_x = gs->player_pos.x - map_view_width / 2;
                int dcam_y = gs->player_pos.y - map_view_height / 2;
                if (dcam_x < 0) dcam_x = 0;
                if (dcam_y < 0) dcam_y = 0;
                if (dcam_x + map_view_width > MAP_WIDTH) dcam_x = MAP_WIDTH - map_view_width;
                if (dcam_y + map_view_height > MAP_HEIGHT) dcam_y = MAP_HEIGHT - map_view_height;
                int ccx = gs->cat_pos.x - dcam_x;
                int ccy = gs->cat_pos.y - dcam_y;
                if (ccx >= 0 && ccx < map_view_width && ccy >= 0 && ccy < map_view_height &&
                    (gs->cat_pos.x != gs->player_pos.x || gs->cat_pos.y != gs->player_pos.y)) {
                    DungeonLevel *dl_cat = current_dungeon_level(gs);
                    if (dl_cat && dl_cat->tiles[gs->cat_pos.y][gs->cat_pos.x].visible) {
                        attron(COLOR_PAIR(CP_YELLOW) | A_BOLD);
                        mvaddch(ccy, ccx, 'c');
                        attroff(COLOR_PAIR(CP_YELLOW) | A_BOLD);
                    }
                }
            }
        }
    }

    /* Sidebar */
    if (show_sidebar) {
        int sidebar_col = map_view_width + 1;

        attron(COLOR_PAIR(CP_GRAY));
        for (int y = 0; y < map_view_height; y++)
            mvaddch(y, map_view_width, ACS_VLINE);
        attroff(COLOR_PAIR(CP_GRAY));

        ui_render_sidebar(sidebar_col, gs->player_name, gs->player_level,
                          gs->hp, gs->max_hp, gs->mp, gs->max_mp,
                          gs->str, gs->def, gs->intel, gs->spd,
                          gs->gold, gs->weight, gs->max_weight, gs->chivalry,
                          gs->turn, gs->day, gs->hour, gs->minute);
    }

    /* Status bar */
    int status_row = map_view_height;
    char status[256];

    if (gs->mode == MODE_OVERWORLD) {
        /* Show terrain and location info */
        Tile *t = &gs->overworld->map[gs->player_pos.y][gs->player_pos.x];
        Location *loc = overworld_location_at(gs->overworld,
                                               gs->player_pos.x, gs->player_pos.y);
        int moon = game_get_moon_day(gs);
        char tname_buf[64];
        if (gs->in_boat) {
            snprintf(tname_buf, sizeof(tname_buf), "Sailing");
        } else if (gs->riding) {
            const char *hnames[] = { "", "Pony", "Palfrey", "Destrier" };
            snprintf(tname_buf, sizeof(tname_buf), "%s (%s)", terrain_name(t->type), hnames[gs->horse_type]);
        } else {
            snprintf(tname_buf, sizeof(tname_buf), "%s", terrain_name(t->type));
        }
        const char *tname = tname_buf;
        if (loc) {
            snprintf(status, sizeof(status), "%s | %s | %s %s | Day %d %02d:%02d %s | %s",
                     loc->name, tname,
                     weather_icon(gs->weather), weather_name(gs->weather),
                     gs->day, gs->hour, gs->minute,
                     time_of_day_name(gs->hour),
                     moon_phase_name(moon));
        } else {
            snprintf(status, sizeof(status), "%s | %s %s | Day %d %02d:%02d %s | %s",
                     tname,
                     weather_icon(gs->weather), weather_name(gs->weather),
                     gs->day, gs->hour, gs->minute,
                     time_of_day_name(gs->hour),
                     moon_phase_name(moon));
        }
    } else {
        if (gs->dungeon) {
            const char *light_str = "";
            char light_buf[32] = "";
            if (gs->has_torch) {
                snprintf(light_buf, sizeof(light_buf), " | %s:%d",
                         gs->torch_type == 2 ? "Lamp" : "Torch", gs->torch_fuel);
                light_str = light_buf;
            }
            snprintf(status, sizeof(status), "%s Lvl %d/%d | Day %d %02d:%02d %s | Turn %d%s",
                     gs->dungeon->name,
                     gs->dungeon->current_level + 1, gs->dungeon->max_depth,
                     gs->day, gs->hour, gs->minute,
                     time_of_day_name(gs->hour), gs->turn, light_str);
        } else {
            snprintf(status, sizeof(status), "Dungeon | Day %d %02d:%02d %s | Turn %d",
                     gs->day, gs->hour, gs->minute,
                     time_of_day_name(gs->hour), gs->turn);
        }
    }

    ui_render_status_bar(status_row, term_cols, status);

    /* Message log */
    int log_row = status_row + 1;
    int log_lines = term_rows - log_row;
    if (log_lines > LOG_LINES) log_lines = LOG_LINES;
    if (log_lines > 0)
        ui_render_log(&gs->log, log_row, term_cols, log_lines);

    ui_refresh();
}

void game_run(GameState *gs) {
    while (gs->running) {
        game_render(gs);
        int key = ui_getkey();
        game_handle_input(gs, key);
        game_update(gs);
    }
}
