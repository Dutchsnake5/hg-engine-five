#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    TryNewMoveEffect NEW_MOVE_EFFECT_PLEDGE_EFFECT, _end
    // A sea of fire / swamp enveloped the team! / A rainbow appeared in the sky on the team's side!
    PrintBufferedMessage
    Wait
    WaitButtonABTime 30

_end:
    End
