#include "tileset.h"
#include <stdio.h>
#include <string.h>

bool tileset_load(Tileset *ts, const char *path) {
    ts->count = 0;
    FILE *f = fopen(path, "r");
    if (!f) return false;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        /* Skip comments and blank lines */
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;

        /* Strip trailing newline */
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        nl = strchr(line, '\r');
        if (nl) *nl = '\0';

        if (line[0] == '\0') continue;

        TileEntry *e = &ts->entries[ts->count];
        int tx, ty;
        if (sscanf(line, "%63[^,],%63[^,],%63[^,],%d,%d",
                   e->id, e->name, e->sheet, &tx, &ty) == 5) {
            e->tx = tx;
            e->ty = ty;
            ts->count++;
            if (ts->count >= MAX_TILE_ENTRIES) break;
        }
    }

    fclose(f);
    return ts->count > 0;
}

const TileEntry *tileset_find(const Tileset *ts, const char *id) {
    for (int i = 0; i < ts->count; i++) {
        if (strcmp(ts->entries[i].id, id) == 0)
            return &ts->entries[i];
    }
    return NULL;
}

const TileEntry *tileset_find_by_name(const Tileset *ts, const char *name) {
    for (int i = 0; i < ts->count; i++) {
        if (strcmp(ts->entries[i].name, name) == 0)
            return &ts->entries[i];
    }
    return NULL;
}
