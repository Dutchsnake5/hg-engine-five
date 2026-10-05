#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    TryNewMoveEffect NEW_MOVE_EFFECT_SALT_CURE, _end
    PrintBufferedMessage
    Wait
    WaitButtonABTime 30

_end:
    End
