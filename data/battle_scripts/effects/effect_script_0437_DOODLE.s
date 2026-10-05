#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    TryNewMoveEffect NEW_MOVE_EFFECT_ITERATOR_RESET, _check

_check:
    TryNewMoveEffect NEW_MOVE_EFFECT_NEXT_DOODLE, _fail
    Call BATTLE_SUBSCRIPT_ATTACK_MESSAGE_AND_ANIMATION
    GoTo _print

_loop:
    TryNewMoveEffect NEW_MOVE_EFFECT_NEXT_DOODLE, _end

_print:
    // {0} copied {1}'s Ability!
    PrintBufferedMessage
    Wait
    WaitButtonABTime 30
    GoTo _loop

_end:
    End

_fail:
    UpdateVar OPCODE_FLAG_ON, BSCRIPT_VAR_MOVE_STATUS_FLAGS, MOVE_STATUS_FAILED
    End
