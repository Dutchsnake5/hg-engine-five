#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

// the berry has been put back in the mon's hand and TryFling has set up its effect
_000:
    AbilityPopup BATTLER_CATEGORY_MSG_BATTLER_TEMP
    Wait
    WaitButtonABTime 15
    CompareVarToValue OPCODE_EQU, BSCRIPT_VAR_FLING_SCRIPT, 0, _noEffect
    UpdateVarFromVar OPCODE_SET, BSCRIPT_VAR_HP_CALC, BSCRIPT_VAR_FLING_DATA
    // the berry scripts remove the berry at the end
    CallFromVar BSCRIPT_VAR_FLING_SCRIPT
    End

_noEffect:
    RemoveItem BATTLER_CATEGORY_MSG_TEMP
    End
