#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    TryNewMoveEffect NEW_MOVE_EFFECT_ORDER_UP, _end
    Call BATTLE_SUBSCRIPT_UPDATE_STAT_STAGE

_end:
    End
