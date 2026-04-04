#ifndef FOV_H
#define FOV_H

#include "common.h"

#define FOV_RADIUS 10

/* Compute field of view using recursive shadowcasting.
   Sets tile->visible for tiles in LOS, and tile->revealed for ever-seen tiles.
   Clears all visible flags first, then marks visible tiles. */
void fov_compute(Tile *tiles, int map_w, int map_h, Vec2 origin, int radius);

#endif /* FOV_H */
