#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    // fails unless someone is holding a berry
    TryNewMoveEffect NEW_MOVE_EFFECT_TEATIME_CHECK, _fail
    Call BATTLE_SUBSCRIPT_ATTACK_MESSAGE_AND_ANIMATION
    TryNewMoveEffect NEW_MOVE_EFFECT_ITERATOR_RESET, _loop

_loop:
    TryNewMoveEffect NEW_MOVE_EFFECT_NEXT_TEATIME, _end
    CompareVarToValue OPCODE_EQU, BSCRIPT_VAR_FLING_SCRIPT, 0, _noEffect
    UpdateVarFromVar OPCODE_SET, BSCRIPT_VAR_HP_CALC, BSCRIPT_VAR_FLING_DATA
    // the berry scripts remove the berry at the end
    CallFromVar BSCRIPT_VAR_FLING_SCRIPT
    GoTo _loop

_noEffect:
    RemoveItem BATTLER_CATEGORY_MSG_TEMP
    GoTo _loop

_end:
    End

_fail:
    UpdateVar OPCODE_FLAG_ON, BSCRIPT_VAR_MOVE_STATUS_FLAGS, MOVE_STATUS_FAILED
    End
