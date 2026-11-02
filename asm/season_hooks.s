.text
.align 2
.thumb

#include "../include/config.h"

#ifdef SEASONAL_MAPS

// arm9 0x0205CD3C, in the player's step handling (pret unk_0205CB48.s, sub_0205CC94): replaces
//   add r0, r5, #0 / bl MetatileBehavior_IsTallGrass / cmp r0, #1
// (the start of the tall grass check: r5 = the tile stepped from, r7 = the tile stepped onto) with
// Seasons_GrassStepSound, which does both tall grass checks and plays the rustle, or the snow footstep in winter
// outdoors, then goes on at 0x0205CD56 (after the game's own rustle). The hook must not cover 0x0205CD56: the game
// jumps there on every step that isn't in tall grass (the first version hooked 0x0205CD50 and broke walking).
.global Seasons_GrassStepSound_hook
Seasons_GrassStepSound_hook:
add r0, r5, #0
add r1, r7, #0
bl Seasons_GrassStepSound
ldr r0, =0x0205CD56 | 1
bx r0

.pool

#endif // SEASONAL_MAPS
