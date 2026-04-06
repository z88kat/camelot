#include "save.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static const char MAGIC[] = "CMLT";
static const int SAVE_VERSION = GAME_VERSION_MINOR;

static void get_save_dir(char *buf, int bufsize) {
    const char *home = getenv("HOME");
    if (!home) home = ".";
    snprintf(buf, bufsize, "%s/.camelot", home);
}

void save_ensure_dir(void) {
    char dir[256];
    get_save_dir(dir, sizeof(dir));
    mkdir(dir, 0755);
}

static void get_save_path(char *buf, int bufsize) {
    char dir[256];
    get_save_dir(dir, sizeof(dir));
    snprintf(buf, bufsize, "%s/save.dat", dir);
}

bool save_exists(void) {
    char path[300];
    get_save_path(path, sizeof(path));
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    /* Validate magic and version before reporting save exists */
    char magic[4];
    if (fread(magic, 1, 4, f) != 4 || memcmp(magic, MAGIC, 4) != 0) {
        fclose(f); remove(path); return false;
    }
    int ver;
    if (fread(&ver, sizeof(int), 1, f) != 1 || ver != SAVE_VERSION) {
        fclose(f); remove(path); return false;
    }
    int struct_size;
    if (fread(&struct_size, sizeof(int), 1, f) != 1 || struct_size != (int)sizeof(GameState)) {
        fclose(f); remove(path); return false;
    }

    fclose(f);
    return true;
}

bool save_game(const GameState *gs) {
    save_ensure_dir();
    char path[300];
    get_save_path(path, sizeof(path));
    FILE *f = fopen(path, "wb");
    if (!f) return false;

    fwrite(MAGIC, 1, 4, f);
    int ver = SAVE_VERSION;
    fwrite(&ver, sizeof(int), 1, f);
    int struct_size = (int)sizeof(GameState);
    fwrite(&struct_size, sizeof(int), 1, f);
    fwrite(gs, sizeof(GameState), 1, f);
    fclose(f);
    return true;
}

bool load_game(GameState *gs) {
    char path[300];
    get_save_path(path, sizeof(path));
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    char magic[4];
    fread(magic, 1, 4, f);
    if (memcmp(magic, MAGIC, 4) != 0) { fclose(f); return false; }

    int ver;
    fread(&ver, sizeof(int), 1, f);
    if (ver != SAVE_VERSION) { fclose(f); return false; }

    int struct_size;
    fread(&struct_size, sizeof(int), 1, f);
    if (struct_size != (int)sizeof(GameState)) { fclose(f); return false; }

    size_t read = fread(gs, 1, sizeof(GameState), f);
    if (read != sizeof(GameState)) { fclose(f); return false; }
    fclose(f);

    /* Pointers are invalid after load -- must reinit */
    gs->overworld = NULL;
    gs->dungeon = NULL;
    gs->current_town = NULL;

    return true;
}

void delete_save(void) {
    char path[300];
    get_save_path(path, sizeof(path));
    remove(path);
}

/* ------------------------------------------------------------------ */
/* High scores                                                         */
/* ------------------------------------------------------------------ */

static void get_scores_path(char *buf, int bufsize) {
    char dir[256];
    get_save_dir(dir, sizeof(dir));
    snprintf(buf, bufsize, "%s/scores.dat", dir);
}

int scores_load(HighScore scores[MAX_HIGH_SCORES]) {
    char path[300];
    get_scores_path(path, sizeof(path));
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    int count = 0;
    fread(&count, sizeof(int), 1, f);
    if (count > MAX_HIGH_SCORES) count = MAX_HIGH_SCORES;
    fread(scores, sizeof(HighScore), count, f);
    fclose(f);
    return count;
}

void scores_add(HighScore scores[], int *count, const HighScore *entry) {
    /* Insert sorted by score descending */
    int pos = *count;
    for (int i = 0; i < *count; i++) {
        if (entry->score > scores[i].score) { pos = i; break; }
    }
    if (pos >= MAX_HIGH_SCORES) return;

    /* Shift down */
    if (*count < MAX_HIGH_SCORES) (*count)++;
    for (int i = *count - 1; i > pos; i--)
        scores[i] = scores[i - 1];
    scores[pos] = *entry;
}

void scores_save(const HighScore scores[], int count) {
    save_ensure_dir();
    char path[300];
    get_scores_path(path, sizeof(path));
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fwrite(&count, sizeof(int), 1, f);
    fwrite(scores, sizeof(HighScore), count, f);
    fclose(f);
}

/* ------------------------------------------------------------------ */
/* Fallen heroes                                                       */
/* ------------------------------------------------------------------ */

static void get_fallen_path(char *buf, int bufsize) {
    char dir[256];
    get_save_dir(dir, sizeof(dir));
    snprintf(buf, bufsize, "%s/fallen_heroes.dat", dir);
}

int fallen_load(FallenHero heroes[MAX_FALLEN]) {
    char path[300];
    get_fallen_path(path, sizeof(path));
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    int count = 0;
    fread(&count, sizeof(int), 1, f);
    if (count > MAX_FALLEN) count = MAX_FALLEN;
    fread(heroes, sizeof(FallenHero), count, f);
    fclose(f);
    return count;
}

/* ------------------------------------------------------------------ */
/* Home chest storage                                                  */
/* ------------------------------------------------------------------ */

static void get_chest_path(char *buf, int bufsize) {
    char dir[256];
    get_save_dir(dir, sizeof(dir));
    snprintf(buf, bufsize, "%s/home_chest.dat", dir);
}

int home_chest_load(StoredItem items[MAX_STORED_ITEMS]) {
    char path[300];
    get_chest_path(path, sizeof(path));
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    int count = 0;
    fread(&count, sizeof(int), 1, f);
    if (count > MAX_STORED_ITEMS) count = MAX_STORED_ITEMS;
    fread(items, sizeof(StoredItem), count, f);
    fclose(f);
    return count;
}

void home_chest_save(const StoredItem items[], int count) {
    save_ensure_dir();
    char path[300];
    get_chest_path(path, sizeof(path));
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fwrite(&count, sizeof(int), 1, f);
    fwrite(items, sizeof(StoredItem), count, f);
    fclose(f);
}

void home_chest_add(const char *item_name, const char *player_name, int day,
                    int type, int power, int value, int weight, char glyph, short color) {
    StoredItem items[MAX_STORED_ITEMS];
    int count = home_chest_load(items);
    if (count >= MAX_STORED_ITEMS) return;

    StoredItem *si = &items[count];
    snprintf(si->name, sizeof(si->name), "%s", item_name);
    snprintf(si->left_by, MAX_NAME, "%s", player_name);
    si->day_stored = day;
    si->type = type;
    si->power = power;
    si->value = value;
    si->weight = weight;
    si->glyph = glyph;
    si->color_pair = color;
    count++;
    home_chest_save(items, count);
}

void fallen_add(const FallenHero *hero) {
    save_ensure_dir();
    FallenHero heroes[MAX_FALLEN];
    int count = fallen_load(heroes);

    /* Add to front (most recent first) */
    if (count < MAX_FALLEN) count++;
    for (int i = count - 1; i > 0; i--)
        heroes[i] = heroes[i - 1];
    heroes[0] = *hero;

    char path[300];
    get_fallen_path(path, sizeof(path));
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fwrite(&count, sizeof(int), 1, f);
    fwrite(heroes, sizeof(FallenHero), count, f);
    fclose(f);
}
