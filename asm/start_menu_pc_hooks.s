.text
.align 2
.thumb

#include "../include/config.h"

// trampolines for the start menu's remote PC icon (src/start_menu_pc.c). these live in the always-loaded code,
// since overlay 27 runs whenever the field is up
#ifdef START_MENU_REMOTE_PC

// touch screen start menu (overlay 27): after it creates its icon sprites, add the PC icon
.global StartMenuPC_Ov27Init_hook
StartMenuPC_Ov27Init_hook:
add r0, r4, #0
bl 0x0225B010
add r0, r4, #0
bl StartMenuPC_CreateIcon
// replaced instructions
ldr r1, [r4, #0x10]
add r0, r4, #0
ldr r2, =0x0225A078 | 1
bx r2

// touch screen start menu: after it checks its icons for touches, check the PC icon
.global StartMenuPC_Ov27Touch_hook
StartMenuPC_Ov27Touch_hook:
cmp r4, #0
bne StartMenuPC_Ov27Touch_hook_skip
add r0, r5, #0
bl 0x0225B4D8
add r0, r5, #0
bl StartMenuPC_CheckTouch
StartMenuPC_Ov27Touch_hook_skip:
add r0, r5, #0
add r1, r4, #0
bl StartMenuPC_UpdateVisuals
ldr r0, =0x0225A412 | 1
bx r0

// touch screen start menu: while a script or other field task runs, overlay 27 only runs its task-time update
// (0x0225A7FC, which slides newly unlocked icons in) and skips the rest of its frame, PC icon hooks included. run that
// update, then let the PC icon slide in the same way when a script has just set its flag
.global StartMenuPC_Ov27Script_hook
StartMenuPC_Ov27Script_hook:
// replaced instructions: beq 0x0225A3DA; add r0, r5, #0; bl 0x0225A7FC; then b 0x0225A412
cmp r0, #0
beq StartMenuPC_Ov27Script_hook_noTask
add r0, r5, #0
bl 0x0225A7FC
add r0, r5, #0
bl StartMenuPC_CheckAppear
ldr r0, =0x0225A412 | 1
bx r0
StartMenuPC_Ov27Script_hook_noTask:
ldr r0, =0x0225A3DA | 1
bx r0

// touch screen start menu: while a picked icon's selection plays out, bounce that icon, which can be the PC icon
.global StartMenuPC_Ov27Bounce_hook
StartMenuPC_Ov27Bounce_hook:
add r0, r5, #0
bl StartMenuPC_BounceSelectedIcon
ldr r0, =0x0225A38A | 1
bx r0

// touch screen start menu: before it frees its resources, free the PC icon's
.global StartMenuPC_Ov27Teardown_hook
StartMenuPC_Ov27Teardown_hook:
add r0, r6, #0
bl StartMenuPC_DestroyIcon
// replaced instructions
mov r0, #0x52
lsl r0, r0, #4
add r0, r6, r0
bl 0x0225BEB0
ldr r0, =0x0225A1D2 | 1
bx r0

// start menu touch input: the PC icon's touch code opens the remote PC, everything else goes to the original
.global StartMenu_HandleTouchInput_hook
StartMenu_HandleTouchInput_hook:
push {r3-r7, lr}
add r4, r1, #0
add r7, r0, #0
add r0, r4, #0
add r0, #0xd0
ldrh r0, [r0]
cmp r0, #12
beq StartMenu_HandleTouchInput_hook_pc
add r0, r4, #0
ldr r3, =0x0203C5AC | 1
bx r3
StartMenu_HandleTouchInput_hook_pc:
add r0, r7, #0
add r1, r4, #0
bl StartMenuPC_HandleTouch
pop {r3-r7, pc}

// start menu A button: with the cursor on the PC icon, open the remote PC; everything else goes to the original
.global StartMenu_HandleKeyInput_hook
StartMenu_HandleKeyInput_hook:
push {r3-r7, lr}
add r5, r1, #0
add r7, r0, #0
push {r0-r2}
bl StartMenuPC_IsSelected
cmp r0, #0
pop {r0-r2}
bne StartMenu_HandleKeyInput_hook_pc
add r0, r5, #0
ldr r3, =0x0203C510 | 1
bx r3
StartMenu_HandleKeyInput_hook_pc:
add r0, r7, #0
add r1, r5, #0
bl StartMenuPC_HandleKey
pop {r3-r7, pc}

.pool

#endif // START_MENU_REMOTE_PC
