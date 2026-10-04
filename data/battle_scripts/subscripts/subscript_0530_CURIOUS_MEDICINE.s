#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    AbilityPopup BATTLER_CATEGORY_MSG_TEMP
    // the message is about the ally of the ability holder
    UpdateVar OPCODE_BITWISE_XOR, BSCRIPT_VAR_MSG_BATTLER_TEMP, 2
    // {0}'s stat changes were removed!
    PrintMessage 1809, TAG_NICKNAME, BATTLER_CATEGORY_MSG_TEMP
    Wait
    WaitButtonABTime 30
    End
