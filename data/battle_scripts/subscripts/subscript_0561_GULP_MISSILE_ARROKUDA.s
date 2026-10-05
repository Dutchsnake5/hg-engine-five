#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    AbilityPopup BATTLER_CATEGORY_MSG_TEMP
    // Cramorant spits out what it caught and goes back to normal
    PlaySound BATTLER_CATEGORY_MSG_TEMP, 1980
    SetMosaic BATTLER_CATEGORY_MSG_TEMP, 8, 1
    Wait
    ChangeForm BATTLER_CATEGORY_MSG_TEMP
    PlaySound BATTLER_CATEGORY_MSG_TEMP, 1984
    SetMosaic BATTLER_CATEGORY_MSG_TEMP, 0, 1
    Wait
    UpdateVarFromVar OPCODE_SET, BSCRIPT_VAR_MSG_BATTLER_TEMP, BSCRIPT_VAR_BATTLER_ATTACKER
    CompareVarToValue OPCODE_EQU, BSCRIPT_VAR_HP_CALC, 0, _effect
    UpdateVar OPCODE_FLAG_ON, BSCRIPT_VAR_BATTLE_STATUS, BATTLE_STATUS_NO_BLINK
    Call BATTLE_SUBSCRIPT_UPDATE_HP

_effect:
    CompareMonDataToValue OPCODE_EQU, BATTLER_CATEGORY_SIDE_EFFECT_MON, BMON_DATA_HP, 0, _end
    // an Arrokuda lowers the attacker's Defense
    UpdateVar OPCODE_SET, BSCRIPT_VAR_SIDE_EFFECT_PARAM, MOVE_SUBSCRIPT_PTR_DEFENSE_DOWN_1_STAGE
    UpdateVar OPCODE_SET, BSCRIPT_VAR_SIDE_EFFECT_TYPE, SIDE_EFFECT_TYPE_ABILITY
    Call BATTLE_SUBSCRIPT_UPDATE_STAT_STAGE

_end:
    End
