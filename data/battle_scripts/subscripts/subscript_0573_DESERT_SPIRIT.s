#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

// Desert Spirit: the Ground type was added on entry (SwitchInAbilityCheck.c sets the third type)
_000:
    AbilityPopup BATTLER_CATEGORY_MSG_TEMP
    // Ground type was added to {0}!
    PrintMessage 1953, TAG_NICKNAME, BATTLER_CATEGORY_MSG_TEMP
    Wait
    WaitButtonABTime 30
    End
