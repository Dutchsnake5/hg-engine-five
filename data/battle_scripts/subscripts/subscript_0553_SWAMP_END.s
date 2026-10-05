#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    // The swamp around the team disappeared!
    PrintMessage 1887, TAG_NONE_SIDE, BATTLER_CATEGORY_MSG_TEMP
    Wait
    WaitButtonABTime 30
    End
