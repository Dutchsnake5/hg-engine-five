#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    // The effects of the neutralizing gas wore off!
    PrintMessage 1802, TAG_NONE
    Wait
    WaitButtonABTime 30
    End
