#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    AbilityPopup BATTLER_CATEGORY_MSG_TEMP
    TryNewMoveEffect NEW_MOVE_EFFECT_COSTAR, _end
    // {0} copied {1}'s stat changes!
    PrintBufferedMessage
    Wait
    WaitButtonABTime 30

_end:
    End
