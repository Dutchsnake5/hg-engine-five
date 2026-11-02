.text
.align 2
.thumb

#include "../include/config.h"

#ifdef SEASONAL_MAPS

// arm9 0x0205CD50, in the player's step handling (pret unk_0205CB48.s, sub_0205CC94): replaces
//   ldr r0, =SEQ_SE_GS_KUSA2 / bl PlaySE / add r0, r4, #0
// (the tall grass rustle) with Seasons_GrassStepSound, which plays the snow footstep instead in winter outdoors, then
// redoes the overwritten add r0, r4, #0 and goes back. r1 is free there (the game sets it next).
.global Seasons_GrassStepSound_hook
Seasons_GrassStepSound_hook:
bl Seasons_GrassStepSound
add r0, r4, #0
ldr r1, =0x0205CD58 | 1
bx r1

.pool

#endif // SEASONAL_MAPS
