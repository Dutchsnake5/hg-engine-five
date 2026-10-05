#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    CalcCrit
    CalcDamage
    // later Rounds this turn are doubled and move right after this one
    TryNewMoveEffect NEW_MOVE_EFFECT_ROUND, _end

_end:
    End
