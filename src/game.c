#include "game.h"
#include "ui.h"
#include "rng.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

/* Player attacks a monster */
static void combat_attack_monster(GameState *gs, Entity *target) {
    int damage = gs->str + rng_range(-2, 2) - target->def;
    if (damage < 0) damage = 0;

    target->hp -= damage;

    if (damage > 0) {
        log_add(&gs->log, gs->turn, CP_WHITE,
                 "You hit the %s for %d damage!", target->name, damage);
    } else {
        log_add(&gs->log, gs->turn, CP_GRAY,
                 "You attack the %s but deal no damage.", target->name);
    }

    if (target->hp <= 0) {
        target->alive = false;
        gs->xp += target->xp_reward;
        gs->kills++;
        log_add(&gs->log, gs->turn, CP_YELLOW,
                 "The %s is slain! +%d XP", target->name, target->xp_reward);
    }
}

/* Monster attacks the player */
static void combat_monster_attacks(GameState *gs, Entity *attacker) {
    int damage = attacker->str + rng_range(-2, 2) - gs->def;
    if (damage < 0) damage = 0;

    gs->hp -= damage;

    if (damage > 0) {
        log_add(&gs->log, gs->turn, CP_RED,
                 "The %s hits you for %d damage!", attacker->name, damage);
    } else {
        log_add(&gs->log, gs->turn, CP_GRAY,
                 "The %s attacks but you block it.", attacker->name);
    }

    if (gs->hp <= 0) {
        gs->hp = 0;
        log_add(&gs->log, gs->turn, CP_RED_BOLD,
                 "You have been slain by the %s!", attacker->name);
        /* TODO: death screen in Phase 13 */
        gs->hp = 1;  /* prevent death for now */
    }
}

/* Process enemy turns -- simple chase AI */
static void dungeon_enemy_turns(GameState *gs) {
    DungeonLevel *dl = current_dungeon_level(gs);
    if (!dl) return;

    for (int i = 0; i < dl->num_monsters; i++) {
        Entity *e = &dl->monsters[i];
        if (!e->alive) continue;

        /* Only act if within FOV radius + a few tiles (awake range) */
        int dx = e->pos.x - gs->player_pos.x;
        int dy = e->pos.y - gs->player_pos.y;
        int dist_sq = dx * dx + dy * dy;
        if (dist_sq > (FOV_RADIUS + 5) * (FOV_RADIUS + 5)) continue;

        /* If adjacent to player, attack */
        if (abs(dx) <= 1 && abs(dy) <= 1) {
            combat_monster_attacks(gs, e);
            continue;
        }

        /* Move toward player (simple: step in direction that reduces distance) */
        int step_x = 0, step_y = 0;
        if (dx > 0) step_x = -1;
        else if (dx < 0) step_x = 1;
        if (dy > 0) step_y = -1;
        else if (dy < 0) step_y = 1;

        /* Try to move, prefer diagonal, fall back to cardinal */
        int attempts[3][2] = {
            { e->pos.x + step_x, e->pos.y + step_y },  /* diagonal */
            { e->pos.x + step_x, e->pos.y },            /* horizontal */
            { e->pos.x, e->pos.y + step_y },             /* vertical */
        };

        for (int a = 0; a < 3; a++) {
            int nx = attempts[a][0], ny = attempts[a][1];
            if (nx < 1 || nx >= MAP_WIDTH - 1 || ny < 1 || ny >= MAP_HEIGHT - 1) continue;
            if (!dl->tiles[ny][nx].passable) continue;
            if (nx == gs->player_pos.x && ny == gs->player_pos.y) continue;
            if (monster_at(gs, nx, ny)) continue;

            e->pos.x = nx;
            e->pos.y = ny;
            break;
        }
    }
}

static void dungeon_update_fov(GameState *gs) {
    if (!gs->dungeon) return;
    DungeonLevel *dl = current_dungeon_level(gs);
    if (!dl) return;
    int radius = gs->has_torch ? FOV_RADIUS : 2;
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

/* Check for lunar event and notify player */
static void check_lunar_events(GameState *gs, int old_hour) {
    int moon = game_get_moon_day(gs);
    /* Only notify once per day at dawn */
    if (old_hour < 5 && gs->hour >= 5) {
        switch (moon) {
        case 1:
            log_add(&gs->log, gs->turn, CP_CYAN, "New Moon tonight. The darkness will be absolute.");
            break;
        case 5:
            log_add(&gs->log, gs->turn, CP_YELLOW, "Feast Day at Camelot! A grand celebration awaits.");
            break;
        case 10:
            log_add(&gs->log, gs->turn, CP_YELLOW, "Market Day! Merchants gather in Camelot and London.");
            break;
        case 12:
            log_add(&gs->log, gs->turn, CP_GREEN, "The Druid Gathering at Stonehenge begins today.");
            break;
        case 14:
            log_add(&gs->log, gs->turn, CP_WHITE, "Tomorrow is the Full Moon. Beware the night...");
            break;
        case 15:
            log_add(&gs->log, gs->turn, CP_YELLOW_BOLD, "FULL MOON tonight! Werewolves roam. Stay indoors.");
            break;
        case 18:
            log_add(&gs->log, gs->turn, CP_YELLOW, "Tournament Day! Jousting at a nearby castle.");
            break;
        case 20:
            log_add(&gs->log, gs->turn, CP_WHITE, "Holy Day. Visit a shrine for double blessings.");
            break;
        case 22:
            log_add(&gs->log, gs->turn, CP_MAGENTA, "The Witching Hour approaches. Dark magic stirs.");
            break;
        case 25:
            log_add(&gs->log, gs->turn, CP_YELLOW, "King's Court today! Arthur holds court at Camelot.");
            break;
        case 27:
            log_add(&gs->log, gs->turn, CP_CYAN, "Night of Spirits! The dead walk tonight.");
            break;
        case 30:
            log_add(&gs->log, gs->turn, CP_GREEN, "Harvest Moon. Food is plentiful today.");
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
    return base + weather_speed_penalty(gs->weather);
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
        if (creature && passable) {
            switch (creature->type) {
            case OW_NPC_TRAVELLER:
                {
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

        gs->player_pos.x = nx;
        gs->player_pos.y = ny;
        TileType stepped_on = gs->overworld->map[ny][nx].type;
        int travel_mins = overworld_travel_time(gs, stepped_on);
        int old_hour = gs->hour;
        advance_time(gs, travel_mins);
        check_lunar_events(gs, old_hour);

        /* Move overworld creatures */
        overworld_move_creatures(gs->overworld, gs->player_pos);

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
                         "The ruins of %s loom before you.", loc->name);
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
            if (loc->visited) {
                log_add(&gs->log, gs->turn, CP_GRAY,
                         "The magic circle has faded. Its power is spent.");
            } else {
                loc->visited = true;
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
            /* Night ambush chance */
            if (is_night(gs->hour) && rng_chance(15)) {
                log_add(&gs->log, gs->turn, CP_RED,
                         "You are awakened by sounds in the dark! (Ambush!)");
                /* TODO: trigger encounter when combat is implemented */
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

        ui_render_minimap((Tile *)gs->overworld->map, OW_WIDTH, OW_HEIGHT,
                          gs->player_pos, loc_info);
        ui_getkey();  /* wait for any key to close */
        return;
    }

    /* Enter dungeon */
    if (key == '>') {
        Location *loc = overworld_location_at(gs->overworld,
                                               gs->player_pos.x, gs->player_pos.y);
        if (loc && (loc->type == LOC_DUNGEON_ENTRANCE || loc->type == LOC_VOLCANO)) {
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

            /* Place player next to stairs up */
            Vec2 su = dl->stairs_up[0];
            gs->player_pos = su;
            /* Try adjacent floor tile */
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
            dungeon_update_fov(gs);
            log_add(&gs->log, gs->turn, CP_WHITE,
                     "You descend into %s... (%d levels deep)", loc->name, num_levels);
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
        town_do_mystic(gs);
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
        if (is_night(gs->hour)) {
            log_add(&gs->log, gs->turn, CP_GRAY,
                     "The %s's shop is closed for the night.", npc->label);
        } else {
            log_add(&gs->log, gs->turn, CP_GRAY,
                     "The %s says: \"Come back when I have stock!\"", npc->label);
        }
        break;
    case NPC_STABLE:
        log_add(&gs->log, gs->turn, CP_GRAY,
                 "The stablemaster says: \"No horses available yet.\"");
        break;
    case NPC_TOWNFOLK:
        {
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
        break;
    case NPC_KING:
        {
            const char *msgs[] = {
                "The King nods gravely. \"Seek the Holy Grail, brave knight.\"",
                "\"England needs heroes. Do not fail us.\"",
                "\"Have you news from the road? Speak!\"",
                "\"Prove your worth and you shall sit at the Round Table.\"",
                "\"Be vigilant. Dark forces stir in the land.\"",
            };
            int n = sizeof(msgs) / sizeof(msgs[0]);
            log_add(&gs->log, gs->turn, CP_YELLOW_BOLD, "%s", msgs[rng_range(0, n - 1)]);
        }
        break;
    case NPC_QUEEN:
        {
            const char *msgs[] = {
                "The Queen smiles warmly. \"You are always welcome here.\"",
                "\"The court whispers of your deeds, brave one.\"",
                "\"Please, bring peace to our troubled land.\"",
                "\"My lord the King places great faith in you.\"",
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
        {
            const char *meows[] = {
                "The cat purrs and rubs against your leg.",
                "The cat stares at you with disdain, then walks away.",
                "Meow! The cat yawns and stretches lazily.",
                "The cat hisses and darts behind a building.",
            };
            int n = sizeof(meows) / sizeof(meows[0]);
            log_add(&gs->log, gs->turn, CP_YELLOW, "%s", meows[rng_range(0, n - 1)]);
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

    /* Leave town explicitly */
    if (key == 'q' || key == 'Q') {
        gs->mode = MODE_OVERWORLD;
        gs->current_town = NULL;
        gs->beers_drunk = 0;
        gs->well_explored = false;
        gs->confessed = false;
        log_add(&gs->log, gs->turn, CP_WHITE, "You leave the town.");
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
                log_add(&gs->log, gs->turn, CP_WHITE,
                         "You climb back to the surface.");
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

    /* Toggle torch (T key) */
    if (key == 'T') {
        gs->has_torch = !gs->has_torch;
        if (gs->has_torch) {
            log_add(&gs->log, gs->turn, CP_YELLOW_BOLD,
                     "You light your torch. The dungeon glows with warm light.");
        } else {
            log_add(&gs->log, gs->turn, CP_GRAY,
                     "You extinguish your torch. Darkness closes in...");
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
                gs->player_pos.x = nx;
                gs->player_pos.y = ny;
                advance_time(gs, 1);
                check_traps(gs);
                dungeon_update_fov(gs);
                dungeon_enemy_turns(gs);

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
    /* Quit -- but not in town mode (q leaves town instead) */
    if ((key == 'q' || key == 'Q') && gs->mode != MODE_TOWN) {
        gs->running = false;
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
        /* Active quests */
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

void game_init(GameState *gs) {
    memset(gs, 0, sizeof(*gs));

    /* Player defaults */
    snprintf(gs->player_name, MAX_NAME, "Sir Nobody");
    gs->player_level = 1;
    gs->hp = 25;  gs->max_hp = 25;
    gs->mp = 10;  gs->max_mp = 10;
    gs->str = 6;  gs->def = 5;  gs->intel = 4;  gs->spd = 5;
    gs->gold = 50;
    gs->weight = 12; gs->max_weight = 48;
    gs->chivalry = 25;
    gs->turn = 0;
    gs->day = 1;
    gs->hour = 8;
    gs->minute = 0;
    gs->weather = WEATHER_CLEAR;
    gs->weather_turns_left = rng_range(100, 200);

    /* Start on overworld at Camelot */
    gs->mode = MODE_OVERWORLD;
    gs->running = true;

    /* Initialize overworld (heap allocated due to size) */
    gs->overworld = calloc(1, sizeof(Overworld));
    overworld_init(gs->overworld);
    overworld_spawn_creatures(gs->overworld);
    town_init();
    quest_init(&gs->quests);

    /* Place player at Camelot (scaled coordinates) */
    gs->player_pos = (Vec2){ 212, 162 };
    gs->ow_player_pos = gs->player_pos;
    gs->dungeon = NULL;
    gs->has_torch = true;  /* start with a torch for testing */

    /* Log */
    log_init(&gs->log);
    log_add(&gs->log, gs->turn, CP_WHITE,
            "Welcome to Knights of Camelot! Press ? for help.");
    log_add(&gs->log, gs->turn, CP_YELLOW,
            "You stand at the gates of Camelot. Seek King Arthur.");
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
            mvprintw(5, TOWN_MAP_W + 2, "Chiv: %d", gs->chivalry);
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

        /* Draw wandering creatures on the overworld */
        {
            int cam_x = gs->player_pos.x - map_view_width / 2;
            int cam_y = gs->player_pos.y - map_view_height / 2;
            if (cam_x < 0) cam_x = 0;
            if (cam_y < 0) cam_y = 0;
            if (cam_x + map_view_width > OW_WIDTH) cam_x = OW_WIDTH - map_view_width;
            if (cam_y + map_view_height > OW_HEIGHT) cam_y = OW_HEIGHT - map_view_height;

            for (int i = 0; i < gs->overworld->num_creatures; i++) {
                OWCreature *c = &gs->overworld->creatures[i];
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
                        int dist_sq = dx * dx + dy * dy;
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

            /* Render monsters visible in FOV */
            {
                DungeonLevel *dl = current_dungeon_level(gs);
                if (dl) {
                    int cam_x = gs->player_pos.x - map_view_width / 2;
                    int cam_y = gs->player_pos.y - map_view_height / 2;
                    if (cam_x < 0) cam_x = 0;
                    if (cam_y < 0) cam_y = 0;
                    if (cam_x + map_view_width > MAP_WIDTH) cam_x = MAP_WIDTH - map_view_width;
                    if (cam_y + map_view_height > MAP_HEIGHT) cam_y = MAP_HEIGHT - map_view_height;

                    for (int i = 0; i < dl->num_monsters; i++) {
                        Entity *e = &dl->monsters[i];
                        if (!e->alive) continue;
                        /* Only show if tile is visible (in FOV) */
                        if (!dl->tiles[e->pos.y][e->pos.x].visible) continue;

                        int sx = e->pos.x - cam_x;
                        int sy = e->pos.y - cam_y;
                        if (sx >= 0 && sx < map_view_width && sy >= 0 && sy < map_view_height) {
                            attron(COLOR_PAIR(e->color_pair) | A_BOLD);
                            mvaddch(sy, sx, e->glyph);
                            attroff(COLOR_PAIR(e->color_pair) | A_BOLD);
                        }
                    }
                }
            }

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
                        int dist_sq = dx * dx + dy * dy;

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
        const char *tname = gs->in_boat ? "Sailing" : terrain_name(t->type);
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
            snprintf(status, sizeof(status), "%s Lvl %d/%d | Day %d %02d:%02d %s | Turn %d",
                     gs->dungeon->name,
                     gs->dungeon->current_level + 1, gs->dungeon->max_depth,
                     gs->day, gs->hour, gs->minute,
                     time_of_day_name(gs->hour), gs->turn);
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
