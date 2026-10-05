#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    AbilityPopup BATTLER_RELATIVE_ALLY|BATTLER_CATEGORY_MSG_TEMP
    TryNewMoveEffect NEW_MOVE_EFFECT_SYMBIOSIS, _end
    // {0} received the {1} from its ally!
    PrintBufferedMessage
    Wait
    WaitButtonABTime 30

_end:
    End
