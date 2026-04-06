#include "save.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static const char MAGIC[] = "CMLT";
static const int SAVE_VERSION = 2;

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
    if (f) { fclose(f); return true; }
    return false;
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

    fread(gs, sizeof(GameState), 1, f);
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
