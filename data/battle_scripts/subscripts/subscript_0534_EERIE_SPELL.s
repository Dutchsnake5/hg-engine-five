#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    TryNewMoveEffect NEW_MOVE_EFFECT_EERIE_SPELL, _end
    // {0} lost 3 PP from {1}!
    PrintBufferedMessage
    Wait
    WaitButtonABTime 30

_end:
    End
