.text
.align 2
.thumb

#include "../include/config.h"

#ifdef GAME_SPEED_OPTION

// arm9 0x02000DF4, in the main loop (NitroMain): replaces
//   ldr r0, [r4, #0x30] / cmp r0, #0 / bne 0x02000E08 / movs r0, #1 / adds r1, r0, #0 / blx OS_WaitIrq /
//   ldr r0, [r4, #0x2C] / adds r0, #1 / str r0, [r4, #0x2C]
// (wait for a refresh unless gSystem.frameCounter says one already went by) with GameSpeed_MainLoopWait, which decides
// whether to wait. r4 = &gSystem and r5 = 0 stay as they are (the C code saves them)
.global GameSpeed_MainLoopWait_hook
GameSpeed_MainLoopWait_hook:
bl GameSpeed_MainLoopWait
ldr r0, =0x02000E08 | 1
bx r0

.pool

#endif // GAME_SPEED_OPTION
