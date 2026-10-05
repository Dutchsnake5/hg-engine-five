#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    Call BATTLE_SUBSCRIPT_ATTACK_MESSAGE_AND_ANIMATION
    TryNewMoveEffect NEW_MOVE_EFFECT_ALLY_SWITCH, _fail
    // redraw both Pokemon in their new places
    ChangeForm BATTLER_CATEGORY_MSG_TEMP
    HealthbarSlideIn BATTLER_CATEGORY_MSG_TEMP
    Wait
    UpdateVarFromVar OPCODE_SET, BSCRIPT_VAR_MSG_BATTLER_TEMP, BSCRIPT_VAR_BATTLER_TARGET
    ChangeForm BATTLER_CATEGORY_MSG_TEMP
    HealthbarSlideIn BATTLER_CATEGORY_MSG_TEMP
    Wait
    // {0} and {1} switched places!
    PrintBufferedMessage
    Wait
    WaitButtonABTime 30
    End

_fail:
    UpdateVar OPCODE_FLAG_ON, BSCRIPT_VAR_MOVE_STATUS_FLAGS, MOVE_STATUS_FAILED
    End
