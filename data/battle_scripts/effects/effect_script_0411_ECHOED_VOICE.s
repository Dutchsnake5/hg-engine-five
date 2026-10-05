#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    TryNewMoveEffect NEW_MOVE_EFFECT_ECHOED_VOICE, _calc

_calc:
    CalcCrit
    CalcDamage
    End
