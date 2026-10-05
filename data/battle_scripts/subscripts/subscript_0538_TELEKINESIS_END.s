#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    // {0} was freed from the telekinesis!
    PrintMessage 1827, TAG_NICKNAME, BATTLER_CATEGORY_MSG_TEMP
    Wait
    WaitButtonABTime 30
    End
