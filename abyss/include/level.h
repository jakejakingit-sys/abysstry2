#ifndef LEVEL_H
#define LEVEL_H
#include "abyss.h"
typedef struct { uint8_t type, tx, ty; } EntDef;
typedef struct { uint8_t world, w, h, spawn_tx, spawn_ty, boss, nent; const uint8_t *tiles; const EntDef *ents; const char *name; } LevelDef;
typedef struct { int x, y, dx, base; } MPlat;       /* pixels; x = left edge */
#define BITS_BYTES ((MAX_LVL_W * MAX_LVL_H + 7) / 8)
typedef struct {
  const LevelDef *def;
  uint8_t taken[BITS_BYTES];     /* collected treasure (persists, mirrored into the save) */
  uint8_t rt[BITS_BYTES];        /* runtime: broken crumble tiles, active runes (reset on death) */
  uint8_t runes_on, runes_total; bool gates_open;
  MPlat plats[MAX_PLATS]; uint8_t nplats;
} LevelState;
extern const LevelDef LEVELS[LEVELS_IMPLEMENTED];
uint8_t level_raw(const LevelDef *d, int tx, int ty);
uint8_t level_tile(const LevelState *l, int tx, int ty);
void level_load(LevelState *l, int idx, uint32_t treasure_bits);
void level_reset_runtime(LevelState *l);
uint32_t level_treasure_bits(const LevelState *l);
int level_treasure_count(const LevelDef *d);
int level_take(LevelState *l, int tx, int ty);        /* returns coin value, 0 if nothing */
void level_break(LevelState *l, int tx, int ty);
bool level_activate_rune(LevelState *l, int tx, int ty);
#endif
