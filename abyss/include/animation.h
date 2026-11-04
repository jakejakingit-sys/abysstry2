#ifndef ANIMATION_H
#define ANIMATION_H
#include "physics.h"
enum { FR_IDLE0 = 0, FR_IDLE1, FR_RUN0, FR_RUN1, FR_RUN2, FR_RUN3, FR_JUMP, FR_FALL, FR_CLIMB0, FR_CLIMB1, FR_COUNT };
int anim_player_frame(const Player *p, uint16_t tick);
#endif
