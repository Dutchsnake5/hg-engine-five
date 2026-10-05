#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    TryNewMoveEffect NEW_MOVE_EFFECT_INSTRUCT, _fail
    Call BATTLE_SUBSCRIPT_ATTACK_MESSAGE_AND_ANIMATION
    PrintBufferedMessage
    Wait
    WaitButtonABTime 30
    End

_fail:
    UpdateVar OPCODE_FLAG_ON, BSCRIPT_VAR_MOVE_STATUS_FLAGS, MOVE_STATUS_FAILED
    End
