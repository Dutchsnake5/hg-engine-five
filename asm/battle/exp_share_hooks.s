.text
.align 2
.thumb

#include "../../include/config.h"
#include "../../include/constants/battle_message_constants.h"

// the Exp. Share key item (EXP_SHARE_KEY_ITEM in include/config.h): the bench's EXP messages. In Task_DistributeExp
// (ov12 0x02245898), step 0 has just added a Pokémon's EXP (r4 = exp calculator, r5 = its party slot, [sp, #0x38] = the
// EXP) and is about to show "<Pokémon> gained <n> Exp. Points!". ExpShare_GainMessage (src/battle/battle_script_commands.c)
// picks that message, one "rest of your team" message instead, or none.
#ifdef EXP_SHARE_KEY_ITEM

// ov12 0x02245B3E: replaces
//     ldr r0, [sp, #0x38] / cmp r0, #0 / beq 0x02245B76 / mov r1, #17 / add r0, sp, #0xB4
.global ExpShare_GainMessage_hook
ExpShare_GainMessage_hook:
    ldr r0, [sp, #0x38]
    cmp r0, #0
    beq _exp_share_no_exp
    mov r2, r0 // exp
    mov r0, r4 // exp calculator
    mov r1, r5 // party slot
    bl ExpShare_GainMessage
    cmp r0, #1
    beq _exp_share_team_message
    cmp r0, #2
    beq _exp_share_no_message
    // the game's own message: what the replaced code did, then on at 0x02245B48 (strb r1, [r0, #1])
    mov r1, #17
    add r0, sp, #0xB4
    ldr r2, =0x02245B48 | 1
    bx r2

_exp_share_team_message:
    // the message without buffers (tag 0): fill in the id and go print it at 0x02245B54
    add r0, sp, #0xB4
    mov r1, #0
    strb r1, [r0, #1]
    ldr r1, =BATTLE_MSG_EXP_SHARE_TEAM
    strh r1, [r0, #2]
    ldr r2, =0x02245B54 | 1
    bx r2

_exp_share_no_message:
    // straight on to step 3 (EXP bar if it's out, then the level-up checks), as if the message had been shown
    mov r0, #3
    str r0, [r4, #0x28]
    add sp, #0xD8
    pop {r3-r7, pc}

_exp_share_no_exp:
    ldr r2, =0x02245B76 | 1
    bx r2

.pool

#endif // EXP_SHARE_KEY_ITEM
