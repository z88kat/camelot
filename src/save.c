#include "save.h"
#include "common.h"
#include "map.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* ------------------------------------------------------------------ */
/* Forward-compatible block helpers                                    */
/*                                                                     */
/* Each block is prefixed with the on-disk element size and a count.   */
/* On load we read MIN(file_es, current_es) bytes per element and      */
/* zero-pad anything past it -- so adding new fields to a struct that  */
/* a block holds will not break older saves.                            */
/* ------------------------------------------------------------------ */
static void write_block(FILE *f, const void *ptr, size_t elem_size, int count) {
    int es = (int)elem_size;
    fwrite(&es, sizeof(int), 1, f);
    fwrite(&count, sizeof(int), 1, f);
    if (count > 0) fwrite(ptr, elem_size, count, f);
}

/* Reads a block into dst. dst must be sized for `max_count` * cur_es bytes.
 * On success, *out_count holds the actual count from the file. */
static bool read_block(FILE *f, void *dst, size_t cur_es, int *out_count, int max_count) {
    int file_es, count;
    if (fread(&file_es, sizeof(int), 1, f) != 1) return false;
    if (fread(&count, sizeof(int), 1, f) != 1) return false;
    if (count < 0 || count > max_count || file_es <= 0) return false;
    int copy = file_es < (int)cur_es ? file_es : (int)cur_es;
    memset(dst, 0, cur_es * (size_t)count);
    for (int i = 0; i < count; i++) {
        if (fread((char*)dst + (size_t)i * cur_es, 1, (size_t)copy, f) != (size_t)copy)
            return false;
        if (file_es > (int)cur_es)
            fseek(f, file_es - (int)cur_es, SEEK_CUR);
    }
    *out_count = count;
    return true;
}

static const char MAGIC[] = "CMLT";
/* Save format version. Bump when GameState layout changes in a
 * breaking way (fields removed or reordered). Adding NEW fields at
 * the end of GameState does NOT require a bump -- the loader pads
 * shorter saves with zeros and tolerates longer ones. */
#define SAVE_FORMAT_VERSION 100

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

/* Non-destructive existence check. Verifies magic and format version
 * only -- never deletes the file. If the file is corrupt or from an
 * incompatible format version, returns false but leaves the file in
 * place so the player can inspect or back it up. */
bool save_exists(void) {
    char path[300];
    get_save_path(path, sizeof(path));
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    char magic[4];
    if (fread(magic, 1, 4, f) != 4 || memcmp(magic, MAGIC, 4) != 0) {
        fclose(f); return false;
    }
    int ver;
    if (fread(&ver, sizeof(int), 1, f) != 1 || ver != SAVE_FORMAT_VERSION) {
        fclose(f); return false;
    }
    fclose(f);
    return true;
}

/* ------------------------------------------------------------------ */
/* Dungeon serialization                                               */
/*                                                                     */
/* DungeonLevel is a flat struct (all inline arrays) so we serialize   */
/* each visited level as a single block. Adding new fields to          */
/* DungeonLevel/Tile/Entity/Item is forward-compatible because         */
/* read_block zero-pads any missing trailing bytes.                    */
/*                                                                     */
/* Note: items reference templates by template_id. If items.csv is     */
/* edited between save and load, those ids may shift -- known          */
/* limitation, not handled.                                            */
/* ------------------------------------------------------------------ */
static void dungeon_save_to_file(FILE *f, const Dungeon *d) {
    /* Header: a fixed scalar block describing the dungeon. */
    struct DungeonHeader {
        char name[MAX_NAME];
        int  max_depth;
        int  current_level;
        int  has_portal;
    } hdr;
    memset(&hdr, 0, sizeof(hdr));
    snprintf(hdr.name, sizeof(hdr.name), "%s", d->name);
    hdr.max_depth = d->max_depth;
    hdr.current_level = d->current_level;
    hdr.has_portal = d->has_portal ? 1 : 0;
    write_block(f, &hdr, sizeof(hdr), 1);

    /* For each level slot, write a 1-int presence flag followed by the
     * level itself (as a single-element block) if generated. */
    for (int i = 0; i < d->max_depth; i++) {
        int present = d->levels[i].generated ? 1 : 0;
        fwrite(&present, sizeof(int), 1, f);
        if (present) {
            write_block(f, &d->levels[i], sizeof(DungeonLevel), 1);
        }
    }
}

static bool dungeon_load_from_file(FILE *f, Dungeon **out) {
    struct DungeonHeader {
        char name[MAX_NAME];
        int  max_depth;
        int  current_level;
        int  has_portal;
    } hdr;
    int n;
    if (!read_block(f, &hdr, sizeof(hdr), &n, 1) || n != 1) return false;
    if (hdr.max_depth <= 0 || hdr.max_depth > 64) return false;
    if (hdr.name[0] == '\0') return false;

    Dungeon *d = (Dungeon *)calloc(1, sizeof(Dungeon));
    if (!d) return false;
    snprintf(d->name, sizeof(d->name), "%s", hdr.name);
    d->max_depth = hdr.max_depth;
    d->current_level = hdr.current_level;
    d->has_portal = hdr.has_portal != 0;
    d->levels = (DungeonLevel *)calloc((size_t)d->max_depth, sizeof(DungeonLevel));
    if (!d->levels) { free(d); return false; }

    for (int i = 0; i < d->max_depth; i++) {
        int present = 0;
        if (fread(&present, sizeof(int), 1, f) != 1) {
            free(d->levels); free(d); return false;
        }
        if (present) {
            int lc;
            if (!read_block(f, &d->levels[i], sizeof(DungeonLevel), &lc, 1) || lc != 1) {
                free(d->levels); free(d); return false;
            }
            d->levels[i].generated = true;
        }
    }
    *out = d;
    return true;
}

bool save_game(const GameState *gs) {
    save_ensure_dir();
    char path[300];
    get_save_path(path, sizeof(path));
    FILE *f = fopen(path, "wb");
    if (!f) return false;

    fwrite(MAGIC, 1, 4, f);
    int ver = SAVE_FORMAT_VERSION;
    fwrite(&ver, sizeof(int), 1, f);
    /* Payload length lets future builds with a larger GameState load
     * this save by zero-padding the missing trailing bytes. */
    int payload_len = (int)sizeof(GameState);
    fwrite(&payload_len, sizeof(int), 1, f);
    fwrite(gs, sizeof(GameState), 1, f);

    /* Optional trailing dungeon block. Older readers will simply hit
     * EOF after the GameState payload and ignore it. */
    int has_dungeon = (gs->dungeon != NULL) ? 1 : 0;
    fwrite(&has_dungeon, sizeof(int), 1, f);
    if (has_dungeon) {
        dungeon_save_to_file(f, gs->dungeon);
    }

    fclose(f);
    return true;
}

bool load_game(GameState *gs) {
    char path[300];
    get_save_path(path, sizeof(path));
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    char magic[4];
    if (fread(magic, 1, 4, f) != 4 || memcmp(magic, MAGIC, 4) != 0) {
        fclose(f); return false;
    }

    int ver;
    if (fread(&ver, sizeof(int), 1, f) != 1 || ver != SAVE_FORMAT_VERSION) {
        fclose(f); return false;
    }

    int payload_len;
    if (fread(&payload_len, sizeof(int), 1, f) != 1 || payload_len <= 0) {
        fclose(f); return false;
    }

    /* Forward/backward tolerant: zero-init the destination, then read
     * MIN(payload_len, sizeof(GameState)). Older saves with fewer
     * trailing fields leave them zeroed; newer saves with extra fields
     * have the extras silently discarded. */
    memset(gs, 0, sizeof(*gs));
    int to_read = payload_len < (int)sizeof(GameState) ? payload_len : (int)sizeof(GameState);
    size_t read = fread(gs, 1, (size_t)to_read, f);
    if (read != (size_t)to_read) { fclose(f); return false; }

    /* If the file has extra trailing bytes (newer save format), skip
     * them so we leave the stream in a clean state. */
    if (payload_len > (int)sizeof(GameState)) {
        fseek(f, payload_len - (int)sizeof(GameState), SEEK_CUR);
    }

    /* Pointers are invalid after load -- must reinit */
    gs->overworld = NULL;
    gs->dungeon = NULL;
    gs->current_town = NULL;

    /* Sanity: a successful load should have a non-empty player name.
     * If not, the file was corrupt (or zero-padded by an earlier bug). */
    if (gs->player_name[0] == '\0') { fclose(f); return false; }

    /* Optional trailing dungeon block. Old saves end here. */
    int has_dungeon = 0;
    if (fread(&has_dungeon, sizeof(int), 1, f) == 1 && has_dungeon) {
        Dungeon *d = NULL;
        if (dungeon_load_from_file(f, &d)) {
            gs->dungeon = d;
        }
        /* If dungeon load fails we just leave gs->dungeon NULL; main.c
         * will fall back to the overworld. */
    }

    fclose(f);
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
