#include "game.h"
#include "ui.h"
#include "rng.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/* Tile helpers                                                        */
/* ------------------------------------------------------------------ */

static void tile_set(Tile *t, TileType type) {
    t->type = type;
    t->visible = true;
    t->revealed = true;

    switch (type) {
    case TILE_WALL:
        t->glyph = '#'; t->color_pair = CP_GRAY;
        t->passable = false; t->blocks_sight = true;
        break;
    case TILE_FLOOR:
        t->glyph = '.'; t->color_pair = CP_WHITE;
        t->passable = true; t->blocks_sight = false;
        break;
    case TILE_DOOR_CLOSED:
        t->glyph = '+'; t->color_pair = CP_BROWN;
        t->passable = true; t->blocks_sight = true;
        break;
    case TILE_DOOR_OPEN:
        t->glyph = '/'; t->color_pair = CP_BROWN;
        t->passable = true; t->blocks_sight = false;
        break;
    case TILE_STAIRS_DOWN:
        t->glyph = '>'; t->color_pair = CP_WHITE;
        t->passable = true; t->blocks_sight = false;
        break;
    case TILE_STAIRS_UP:
        t->glyph = '<'; t->color_pair = CP_WHITE;
        t->passable = true; t->blocks_sight = false;
        break;
    default:
        t->glyph = ' '; t->color_pair = CP_DEFAULT;
        t->passable = false; t->blocks_sight = true;
        break;
    }
}

/* ------------------------------------------------------------------ */
/* Test dungeon (Phase 1 holdover, will be replaced by BSP gen)        */
/* ------------------------------------------------------------------ */

void game_init_test_map(GameState *gs) {
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            tile_set(&gs->dungeon_map[y][x], TILE_WALL);

    /* Central room */
    for (int y = 5; y < 17; y++)
        for (int x = 20; x < 50; x++)
            tile_set(&gs->dungeon_map[y][x], TILE_FLOOR);

    /* Left room */
    for (int y = 8; y < 14; y++)
        for (int x = 5; x < 15; x++)
            tile_set(&gs->dungeon_map[y][x], TILE_FLOOR);

    /* Corridor left -> center */
    for (int x = 15; x < 20; x++)
        tile_set(&gs->dungeon_map[11][x], TILE_FLOOR);

    /* Right room */
    for (int y = 4; y < 12; y++)
        for (int x = 55; x < 67; x++)
            tile_set(&gs->dungeon_map[y][x], TILE_FLOOR);

    /* Corridor center -> right */
    for (int x = 50; x < 55; x++)
        tile_set(&gs->dungeon_map[8][x], TILE_FLOOR);

    /* Bottom room */
    for (int y = 17; y < 25; y++)
        for (int x = 30; x < 45; x++)
            tile_set(&gs->dungeon_map[y][x], TILE_FLOOR);

    /* Corridor center -> bottom */
    tile_set(&gs->dungeon_map[17][35], TILE_FLOOR);

    /* Door and stairs */
    tile_set(&gs->dungeon_map[11][15], TILE_DOOR_CLOSED);
    tile_set(&gs->dungeon_map[6][22], TILE_STAIRS_UP);
    tile_set(&gs->dungeon_map[23][42], TILE_STAIRS_DOWN);
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
        int nx = gs->player_pos.x + dir_dx[dir];
        int ny = gs->player_pos.y + dir_dy[dir];

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
            case LOC_CASTLE_ACTIVE:
                log_add(&gs->log, gs->turn, CP_WHITE,
                         "You arrive at %s. (Towns not yet implemented)", loc->name);
                break;
            case LOC_CASTLE_ABANDONED:
                log_add(&gs->log, gs->turn, CP_GRAY,
                         "The ruins of %s loom before you.", loc->name);
                break;
            case LOC_DUNGEON_ENTRANCE:
                log_add(&gs->log, gs->turn, CP_WHITE,
                         "A dungeon entrance: %s. Press > to enter.", loc->name);
                break;
            case LOC_LANDMARK:
                log_add(&gs->log, gs->turn, CP_YELLOW,
                         "You stand at %s.", loc->name);
                break;
            case LOC_VOLCANO:
                log_add(&gs->log, gs->turn, CP_RED,
                         "Mount Draig! Smoke rises from the volcano.");
                break;
            case LOC_COTTAGE:
                log_add(&gs->log, gs->turn, CP_BROWN,
                         "A small cottage sits by the path.");
                break;
            case LOC_CAVE:
                log_add(&gs->log, gs->turn, CP_GRAY,
                         "A dark cave entrance in the hillside.");
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
            gs->mode = MODE_DUNGEON;

            /* Find the up stairs and place player next to them */
            bool placed = false;
            for (int sy = 0; sy < MAP_HEIGHT && !placed; sy++) {
                for (int sx = 0; sx < MAP_WIDTH && !placed; sx++) {
                    if (gs->dungeon_map[sy][sx].type == TILE_STAIRS_UP) {
                        /* Place player on an adjacent floor tile */
                        for (int d = 0; d < 8 && !placed; d++) {
                            int px = sx + dir_dx[d];
                            int py = sy + dir_dy[d];
                            if (px >= 0 && px < MAP_WIDTH && py >= 0 && py < MAP_HEIGHT &&
                                gs->dungeon_map[py][px].passable &&
                                gs->dungeon_map[py][px].type == TILE_FLOOR) {
                                gs->player_pos = (Vec2){ px, py };
                                placed = true;
                            }
                        }
                        if (!placed) {
                            /* Fallback: stand on the stairs */
                            gs->player_pos = (Vec2){ sx, sy };
                            placed = true;
                        }
                    }
                }
            }
            if (!placed) {
                gs->player_pos = (Vec2){ 35, 10 };  /* ultimate fallback */
            }

            log_add(&gs->log, gs->turn, CP_WHITE,
                     "You descend into %s...", loc->name);
        } else {
            log_add(&gs->log, gs->turn, CP_GRAY, "There is no entrance here.");
        }
        return;
    }
}

/* ------------------------------------------------------------------ */
/* Dungeon input                                                       */
/* ------------------------------------------------------------------ */

static void handle_dungeon_input(GameState *gs, int key) {
    /* Ascend stairs to overworld */
    if (key == '<') {
        Tile *t = &gs->dungeon_map[gs->player_pos.y][gs->player_pos.x];
        if (t->type == TILE_STAIRS_UP) {
            gs->player_pos = gs->ow_player_pos;
            gs->mode = MODE_OVERWORLD;
            log_add(&gs->log, gs->turn, CP_WHITE,
                     "You climb back to the surface.");
            return;
        }
        log_add(&gs->log, gs->turn, CP_GRAY, "There are no stairs up here.");
        return;
    }

    Direction dir = key_to_direction(key);
    if (dir != DIR_NONE) {
        int nx = gs->player_pos.x + dir_dx[dir];
        int ny = gs->player_pos.y + dir_dy[dir];

        if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT) {
            Tile *target = &gs->dungeon_map[ny][nx];
            if (target->passable) {
                gs->player_pos.x = nx;
                gs->player_pos.y = ny;
                advance_time(gs, 1);  /* 1 minute per dungeon step */
            } else {
                log_add(&gs->log, gs->turn, CP_GRAY, "You bump into a wall.");
            }
        }
        return;
    }
}

/* ------------------------------------------------------------------ */
/* Main input dispatch                                                 */
/* ------------------------------------------------------------------ */

void game_handle_input(GameState *gs, int key) {
    if (key == 'q' || key == 'Q') {
        gs->running = false;
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

    /* Place player at Camelot (scaled coordinates) */
    gs->player_pos = (Vec2){ 212, 162 };
    gs->ow_player_pos = gs->player_pos;

    /* Initialize dungeon (test map for now) */
    game_init_test_map(gs);

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
    int map_view_height = MAP_HEIGHT;
    if (map_view_height > term_rows - LOG_LINES - 2)
        map_view_height = term_rows - LOG_LINES - 2;
    if (map_view_height < 5) map_view_height = 5;

    /* Render the appropriate map */
    if (gs->mode == MODE_OVERWORLD) {
        /* Clamp view to overworld dimensions */
        if (map_view_width > OW_WIDTH) map_view_width = OW_WIDTH;
        if (map_view_height > OW_HEIGHT) map_view_height = OW_HEIGHT;

        ui_render_map_generic((Tile *)gs->overworld->map, OW_WIDTH, OW_HEIGHT,
                              gs->player_pos, map_view_width, map_view_height);

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

        ui_render_map(gs->dungeon_map, gs->player_pos,
                      map_view_width, map_view_height);
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
        snprintf(status, sizeof(status), "Dungeon | Day %d %02d:%02d %s | Turn %d",
                 gs->day, gs->hour, gs->minute,
                 time_of_day_name(gs->hour), gs->turn);
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
