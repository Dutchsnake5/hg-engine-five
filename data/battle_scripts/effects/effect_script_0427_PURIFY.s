#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    TryNewMoveEffect NEW_MOVE_EFFECT_PURIFY, _fail
    Call BATTLE_SUBSCRIPT_ATTACK_MESSAGE_AND_ANIMATION
    // {0}'s status returned to normal!
    PrintBufferedMessage
    Wait
    WaitButtonABTime 30
    SetHealthbarStatus BATTLER_CATEGORY_DEFENDER, BATTLE_ANIMATION_NONE
    // the user regains half of its max HP
    UpdateVarFromVar OPCODE_SET, BSCRIPT_VAR_MSG_BATTLER_TEMP, BSCRIPT_VAR_BATTLER_ATTACKER
    Call BATTLE_SUBSCRIPT_RECOVER_HP
    End

_fail:
    UpdateVar OPCODE_FLAG_ON, BSCRIPT_VAR_MOVE_STATUS_FLAGS, MOVE_STATUS_FAILED
    End
