#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    // only confuses targets whose stats were raised this turn
    TryNewMoveEffect NEW_MOVE_EFFECT_STATS_RAISED_CHECK, _end
    Call BATTLE_SUBSCRIPT_CONFUSE

_end:
    End
