#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    // The rainbow on the team's side disappeared!
    PrintMessage 1891, TAG_NONE_SIDE, BATTLER_CATEGORY_MSG_TEMP
    Wait
    WaitButtonABTime 30
    End
