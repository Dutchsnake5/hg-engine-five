#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    TryNewMoveEffect NEW_MOVE_EFFECT_SYRUP_BOMB, _end
    PrintBufferedMessage
    Wait
    WaitButtonABTime 30

_end:
    End
