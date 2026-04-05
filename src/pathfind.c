#include "pathfind.h"
#include <string.h>
#include <stdlib.h>

/* ------------------------------------------------------------------ */
/* A* pathfinding with binary-heap priority queue                      */
/* ------------------------------------------------------------------ */

typedef struct {
    int x, y;
    int g;    /* cost from start */
    int f;    /* g + heuristic */
} ANode;

/* Binary min-heap on f-cost */
#define HEAP_CAP 2048

typedef struct {
    ANode nodes[HEAP_CAP];
    int   size;
} Heap;

static void heap_swap(Heap *h, int a, int b) {
    ANode tmp = h->nodes[a];
    h->nodes[a] = h->nodes[b];
    h->nodes[b] = tmp;
}

static void heap_push(Heap *h, ANode n) {
    if (h->size >= HEAP_CAP) return;
    h->nodes[h->size] = n;
    int i = h->size++;
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (h->nodes[i].f < h->nodes[parent].f) {
            heap_swap(h, i, parent);
            i = parent;
        } else break;
    }
}

static ANode heap_pop(Heap *h) {
    ANode top = h->nodes[0];
    h->nodes[0] = h->nodes[--h->size];
    int i = 0;
    while (1) {
        int l = 2 * i + 1, r = 2 * i + 2, best = i;
        if (l < h->size && h->nodes[l].f < h->nodes[best].f) best = l;
        if (r < h->size && h->nodes[r].f < h->nodes[best].f) best = r;
        if (best == i) break;
        heap_swap(h, i, best);
        i = best;
    }
    return top;
}

/* Chebyshev distance heuristic (8-directional) */
static int heuristic(int x1, int y1, int x2, int y2) {
    int dx = abs(x1 - x2), dy = abs(y1 - y2);
    return (dx > dy) ? dx : dy;
}

int pathfind_astar(Tile tiles[MAP_HEIGHT][MAP_WIDTH],
                   int sx, int sy, int gx, int gy,
                   int *out_x, int *out_y,
                   bool ghost_mode) {
    /* Limit search area to a box around start/goal for performance */
    int min_x = (sx < gx ? sx : gx) - 20;
    int max_x = (sx > gx ? sx : gx) + 20;
    int min_y = (sy < gy ? sy : gy) - 20;
    int max_y = (sy > gy ? sy : gy) + 20;
    if (min_x < 0) min_x = 0;
    if (min_y < 0) min_y = 0;
    if (max_x >= MAP_WIDTH) max_x = MAP_WIDTH - 1;
    if (max_y >= MAP_HEIGHT) max_y = MAP_HEIGHT - 1;

    int box_w = max_x - min_x + 1;
    int box_h = max_y - min_y + 1;
    if (box_w <= 0 || box_h <= 0) return 0;

    /* Use static arrays to avoid malloc in hot path */
    static int g_cost[80][200];  /* [local_y][local_x] */
    static int came_from[80][200]; /* encoded as x + y * MAP_WIDTH, or -1 */

    /* Ensure box fits in static arrays */
    if (box_w > 200 || box_h > 80) {
        /* Fallback: box too large, use direct step */
        *out_x = sx; *out_y = sy;
        return 0;
    }

    /* Initialize */
    for (int ly = 0; ly < box_h; ly++)
        for (int lx = 0; lx < box_w; lx++) {
            g_cost[ly][lx] = 999999;
            came_from[ly][lx] = -1;
        }

    int lsx = sx - min_x, lsy = sy - min_y;
    int lgx = gx - min_x, lgy = gy - min_y;
    g_cost[lsy][lsx] = 0;

    static Heap heap;
    heap.size = 0;
    ANode start = { sx, sy, 0, heuristic(sx, sy, gx, gy) };
    heap_push(&heap, start);

    int dx8[] = {0, 1, 1, 1, 0, -1, -1, -1};
    int dy8[] = {-1, -1, 0, 1, 1, 1, 0, -1};

    int iterations = 0;
    while (heap.size > 0 && iterations < 2000) {
        iterations++;
        ANode cur = heap_pop(&heap);

        if (cur.x == gx && cur.y == gy) {
            /* Reconstruct path to find first step */
            int px = gx, py = gy;
            while (1) {
                int lpx = px - min_x, lpy = py - min_y;
                int prev = came_from[lpy][lpx];
                if (prev < 0) break;
                int ppx = prev % MAP_WIDTH, ppy = prev / MAP_WIDTH;
                if (ppx == sx && ppy == sy) {
                    *out_x = px;
                    *out_y = py;
                    return g_cost[lgy][lgx];
                }
                px = ppx;
                py = ppy;
            }
            /* Goal is adjacent to start */
            *out_x = gx;
            *out_y = gy;
            return 1;
        }

        int lcx = cur.x - min_x, lcy = cur.y - min_y;
        if (cur.g > g_cost[lcy][lcx]) continue; /* stale entry */

        for (int d = 0; d < 8; d++) {
            int nx = cur.x + dx8[d], ny = cur.y + dy8[d];
            if (nx < min_x || nx > max_x || ny < min_y || ny > max_y) continue;

            /* Check passability (ghost mode ignores walls) */
            if (!ghost_mode && !tiles[ny][nx].passable) {
                /* Allow destination tile even if blocked (for targeting) */
                if (nx != gx || ny != gy) continue;
            }

            int lnx = nx - min_x, lny = ny - min_y;
            int new_g = cur.g + 1;
            if (new_g < g_cost[lny][lnx]) {
                g_cost[lny][lnx] = new_g;
                came_from[lny][lnx] = cur.x + cur.y * MAP_WIDTH;
                ANode next = { nx, ny, new_g, new_g + heuristic(nx, ny, gx, gy) };
                heap_push(&heap, next);
            }
        }
    }

    /* No path found */
    *out_x = sx;
    *out_y = sy;
    return 0;
}
