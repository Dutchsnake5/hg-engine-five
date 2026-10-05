#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    // The sea of fire around the team disappeared!
    PrintMessage 1883, TAG_NONE_SIDE, BATTLER_CATEGORY_MSG_TEMP
    Wait
    WaitButtonABTime 30
    End
