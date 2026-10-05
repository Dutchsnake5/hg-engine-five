#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    // Wonder Room wore off, and the Defense and Sp. Def stats returned to normal!
    PrintMessage 1858, TAG_NONE
    Wait
    WaitButtonABTime 30
    End
