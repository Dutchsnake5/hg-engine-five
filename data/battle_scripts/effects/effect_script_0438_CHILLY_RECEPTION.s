#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    PrintAttackMessage
    Wait
    WaitButtonABTime 30
    // the snow still has to try to start even if the user can't switch out
    Call BATTLE_SUBSCRIPT_HANDLE_SNOW_TEMPORARY
    UpdateVar OPCODE_FLAG_OFF, BSCRIPT_VAR_MOVE_STATUS_FLAGS, MOVE_STATUS_FAILED
    TryReplaceFaintedMon BATTLER_CATEGORY_ATTACKER, TRUE, _end
    TryRestoreStatusOnSwitch BATTLER_CATEGORY_ATTACKER, _switch
    UpdateMonData OPCODE_SET, BATTLER_CATEGORY_ATTACKER, BMON_DATA_STATUS, STATUS_NONE

_switch:
    DeletePokemon BATTLER_CATEGORY_ATTACKER
    Wait
    HealthbarSlideOut BATTLER_CATEGORY_ATTACKER
    Wait
    UpdateVar OPCODE_FLAG_ON, BSCRIPT_VAR_BATTLE_STATUS_2, BATTLE_STATUS2_UTURN
    UpdateVar OPCODE_FLAG_OFF, BSCRIPT_VAR_BATTLE_STATUS, BATTLE_STATUS_SYNCRONIZE
    UpdateVar OPCODE_SET, BSCRIPT_VAR_ATTACKER_SELF_TURN_STATUS_FLAGS, SELF_TURN_FLAG_CLEAR
    GoToSubscript BATTLE_SUBSCRIPT_SHOW_PARTY_LIST

_end:
    End
