#ifndef PATHFIND_H
#define PATHFIND_H

#include "common.h"

/* A* pathfinding on the dungeon grid.
   Returns the number of steps (0 = no path found).
   out_x/out_y receive the FIRST step toward the goal. */
int pathfind_astar(Tile tiles[MAP_HEIGHT][MAP_WIDTH],
                   int sx, int sy, int gx, int gy,
                   int *out_x, int *out_y,
                   bool ghost_mode);  /* ghost_mode: walk through walls */

#endif /* PATHFIND_H */
