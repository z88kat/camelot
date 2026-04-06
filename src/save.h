#ifndef SAVE_H
#define SAVE_H

#include "game.h"

/* Save/load game state to ~/.camelot/save.dat */
bool save_game(const GameState *gs);
bool load_game(GameState *gs);
void delete_save(void);
bool save_exists(void);

/* High scores */
typedef struct {
    char name[MAX_NAME];
    char class_name[16];
    int  score;
    char cause[64];
    bool found_grail;
} HighScore;

#define MAX_HIGH_SCORES 10

int  scores_load(HighScore scores[MAX_HIGH_SCORES]);
void scores_add(HighScore scores[], int *count, const HighScore *entry);
void scores_save(const HighScore scores[], int count);

/* Fallen heroes */
typedef struct {
    char name[MAX_NAME];
    char class_name[16];
    int  level;
    int  score;
    int  turns;
    int  kills;
    char cause[64];
    char title[32];
} FallenHero;

#define MAX_FALLEN 50

int  fallen_load(FallenHero heroes[MAX_FALLEN]);
void fallen_add(const FallenHero *hero);

/* Home chest storage (persists across deaths) */
typedef struct {
    char name[48];
    char left_by[MAX_NAME];
    int  day_stored;
    int  type;       /* ItemType */
    int  power;
    int  value;
    int  weight;
    char glyph;
    short color_pair;
} StoredItem;

#define MAX_STORED_ITEMS 50

int  home_chest_load(StoredItem items[MAX_STORED_ITEMS]);
void home_chest_save(const StoredItem items[], int count);
void home_chest_add(const char *item_name, const char *player_name, int day,
                    int type, int power, int value, int weight, char glyph, short color);

/* Ensure ~/.camelot directory exists */
void save_ensure_dir(void);

#endif /* SAVE_H */
