#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    IncrementGameStat BATTLER_CATEGORY_FAINTED_MON, BATTLER_TYPE_SOLO_ENEMY, 42
    IncrementGameStat BATTLER_CATEGORY_FAINTED_MON, BATTLER_TYPE_SOLO_PLAYER, 97
    PlayFaintAnimation 
    Wait 
    HealthbarSlideOut BATTLER_CATEGORY_FAINTED_MON
    // {0} fainted!
    PrintMessage 30, TAG_NICKNAME, BATTLER_CATEGORY_FAINTED_MON
    Wait 
    WaitButtonABTime 30
    Call BATTLE_SUBSCRIPT_TRY_CLEAR_PRIMAL_WEATHERS_FAINTING
    Call BATTLE_SUBSCRIPT_SOUL_HEART
    // Receiver and Power of Alchemy take over the fainted ally's ability
    TryNewMoveEffect NEW_MOVE_EFFECT_RECEIVER_CHECK, _noReceiver
    AbilityPopup BATTLER_CATEGORY_MSG_TEMP
    TryNewMoveEffect NEW_MOVE_EFFECT_RECEIVER_APPLY, _noReceiver
    PrintBufferedMessage
    Wait
    WaitButtonABTime 30

_noReceiver:
    // a commanding Tatsugiri comes out of a fainted Dondozo's mouth
    TryNewMoveEffect NEW_MOVE_EFFECT_COMMANDER_RELEASE, _noCommander
    ToggleVanish BATTLER_CATEGORY_MSG_TEMP, FALSE
    Wait

_noCommander:
    CompareVarToValue OPCODE_FLAG_SET, BSCRIPT_VAR_BATTLE_STATUS_2, BATTLE_STATUS2_NO_EXP_GAINED, _025

_025:
    End 
