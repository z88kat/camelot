#ifndef AMMO_H
#define AMMO_H

typedef struct {
    char  name[32];
    char  glyph;
    short color_pair;
    int   damage_bonus;
    int   weight;
    int   value;
    int   recovery_chance; /* 0-100 */
} AmmoDef;

void ammo_init(void);

/* Find ammo def by substring match against name (case-sensitive).
 * Returns NULL if none found. */
const AmmoDef *ammo_find(const char *item_name);

/* Exact name lookup */
const AmmoDef *ammo_find_exact(const char *name);

int ammo_def_count(void);
const AmmoDef *ammo_get(int i);

#endif /* AMMO_H */
