#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    // {0} started heating up its beak!
    PrintMessage 1860, TAG_NICKNAME, BATTLER_CATEGORY_MSG_TEMP
    Wait
    WaitButtonABTime 30
    End
