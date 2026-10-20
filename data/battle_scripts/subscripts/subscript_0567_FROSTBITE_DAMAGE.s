#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

// FROSTBITE (include/config.h): 1/16 of max HP at the end of the turn, like a burn (Heatproof doesn't apply)
_000:
    CheckAbility CHECK_OPCODE_HAVE, BATTLER_CATEGORY_MSG_TEMP, ABILITY_MAGIC_GUARD, _end
    UpdateMonDataFromVar OPCODE_GET, BATTLER_CATEGORY_MSG_TEMP, BMON_DATA_MAXHP, BSCRIPT_VAR_HP_CALC
    DivideVarByValue BSCRIPT_VAR_HP_CALC, 16
    UpdateVar OPCODE_MUL, BSCRIPT_VAR_HP_CALC, -1
    // {0} is hurt by its frostbite!
    PrintMessage 111, TAG_NICKNAME, BATTLER_CATEGORY_MSG_TEMP
    Wait
    WaitButtonABTime 30
    PlayBattleAnimation BATTLER_CATEGORY_MSG_TEMP, BATTLE_ANIMATION_FROZEN
    Wait
    UpdateVar OPCODE_FLAG_ON, BSCRIPT_VAR_BATTLE_STATUS, BATTLE_STATUS_NO_BLINK
    Call BATTLE_SUBSCRIPT_UPDATE_HP
    TryActivateEmergencyExit BATTLER_CATEGORY_MSG_BATTLER_TEMP, _end
    Call BATTLE_SUBSCRIPT_HANDLE_WIMP_OUT_EMERGENCY_EXIT

_end:
    End
