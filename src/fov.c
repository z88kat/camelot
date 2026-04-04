#include "fov.h"
#include <math.h>

/* ------------------------------------------------------------------ */
/* Simple raycasting FOV                                               */
/* Cast rays from origin in all directions, mark tiles as visible      */
/* until hitting an opaque tile (which is also marked visible).         */
/* ------------------------------------------------------------------ */

void fov_compute(Tile *tiles, int map_w, int map_h, Vec2 origin, int radius) {
    /* Clear all visible flags */
    for (int y = 0; y < map_h; y++) {
        for (int x = 0; x < map_w; x++) {
            tiles[y * map_w + x].visible = false;
        }
    }

    /* Origin is always visible */
    if (origin.x >= 0 && origin.x < map_w && origin.y >= 0 && origin.y < map_h) {
        tiles[origin.y * map_w + origin.x].visible = true;
        tiles[origin.y * map_w + origin.x].revealed = true;
    }

    /* Cast rays in 360 degrees */
    int num_rays = 360;
    for (int r = 0; r < num_rays; r++) {
        double angle = (double)r * 3.14159265 * 2.0 / num_rays;
        double dx = cos(angle);
        double dy = sin(angle);

        double fx = (double)origin.x + 0.5;
        double fy = (double)origin.y + 0.5;

        for (int step = 0; step <= radius; step++) {
            int tx = (int)fx;
            int ty = (int)fy;

            if (tx < 0 || tx >= map_w || ty < 0 || ty >= map_h) break;

            Tile *t = &tiles[ty * map_w + tx];
            t->visible = true;
            t->revealed = true;

            /* Stop at opaque tiles (but mark them visible first so walls show) */
            if (t->blocks_sight) break;

            fx += dx;
            fy += dy;
        }
    }
}
