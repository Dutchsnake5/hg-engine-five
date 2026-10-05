#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    // {0} is hurt by the sea of fire!
    PrintMessage 1878, TAG_NICKNAME, BATTLER_CATEGORY_MSG_TEMP
    Wait
    WaitButtonABTime 30
    UpdateVar OPCODE_FLAG_ON, BSCRIPT_VAR_BATTLE_STATUS, BATTLE_STATUS_NO_BLINK
    GoToSubscript BATTLE_SUBSCRIPT_UPDATE_HP
    End
