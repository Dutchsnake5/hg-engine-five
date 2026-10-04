#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    PrintAttackMessage
    Wait
    WaitButtonABTime 30

    AbilityPopup BATTLER_CATEGORY_MSG_TEMP

    // {0} is protected by an aromatic veil!
    PrintMessage 1806, TAG_NICKNAME, BATTLER_CATEGORY_DEFENDER
    Wait
    WaitButtonABTime 30
    End
