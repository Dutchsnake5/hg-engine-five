#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    TryNewMoveEffect NEW_MOVE_EFFECT_CORE_ENFORCER, _end
    // {0}'s Ability was suppressed!
    PrintBufferedMessage
    Wait
    WaitButtonABTime 30

_end:
    End
