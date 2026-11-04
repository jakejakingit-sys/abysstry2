#ifndef ASSETS_H
#define ASSETS_H
#include <stdint.h>
/* Every sprite: [width, height, indexed pixels...]; index 0 = transparent. Layout matches graphx gfx_sprite_t. */
enum { TS_SOLID, TS_SOLID_TOP, TS_ONEWAY, TS_SPIKE, TS_LADDER, TS_COIN0, TS_COIN1, TS_COIN2, TS_COIN3, TS_CHEST, TS_EXIT,
       TS_CRUMBLE, TS_RUNE_OFF, TS_RUNE_ON, TS_GATE, TS_TORCH0, TS_TORCH1, TS_SKULL, TS_COUNT };
#define TILE_SPR_BYTES (2 + 16 * 16)
extern const uint8_t PALETTE_RGB[32][3];
extern const uint8_t TILE_SPR[TS_COUNT][TILE_SPR_BYTES];
extern const uint8_t PLAYER_SPR[2][10][2 + 12 * 16];     /* [dir 0=right,1=left][frame] */
extern const uint8_t CRAWLER_SPR[2][2][2 + 14 * 10];     /* [dir][frame] */
extern const uint8_t BONE_SPR[2 + 6 * 12];
extern const uint8_t KING_SPR[2][2 + 40 * 40];
extern const uint8_t HEART_SPR[2 + 9 * 8];
#endif
