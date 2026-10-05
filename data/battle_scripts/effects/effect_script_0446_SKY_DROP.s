#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    CompareMonDataToValue OPCODE_FLAG_SET, BATTLER_CATEGORY_ATTACKER, BMON_DATA_STATUS2, STATUS2_LOCKED_INTO_MOVE, _drop
    // first turn: carry the target into the sky
    TryNewMoveEffect NEW_MOVE_EFFECT_SKY_DROP_LIFT, _tooHeavy
    PrintAttackMessage
    Wait
    WaitButtonABTime 30
    UpdateMonData OPCODE_FLAG_ON, BATTLER_CATEGORY_ATTACKER, BMON_DATA_MOVE_EFFECT, MOVE_EFFECT_FLAG_FLY
    UpdateVar OPCODE_FLAG_ON, BSCRIPT_VAR_BATTLE_STATUS, BATTLE_STATUS_CHARGE_TURN|BATTLE_STATUS_CHECK_LOOP_ONLY_ONCE|BATTLE_STATUS_NO_ATTACK_MESSAGE
    PlayMoveAnimation BATTLER_CATEGORY_ATTACKER
    Wait
    // {0} was taken into the sky!
    PrintBufferedMessage
    Wait
    WaitButtonABTime 30
    LockMoveChoice BATTLER_CATEGORY_ATTACKER
    ToggleVanish BATTLER_CATEGORY_ATTACKER, TRUE
    ToggleVanish BATTLER_CATEGORY_DEFENDER, TRUE
    End

_drop:
    ToggleVanish BATTLER_CATEGORY_ATTACKER, FALSE
    CalcCrit
    CalcDamage
    UpdateMonData OPCODE_FLAG_OFF, BATTLER_CATEGORY_ATTACKER, BMON_DATA_MOVE_EFFECT, MOVE_EFFECT_FLAG_SEMI_INVULNERABLE
    Call BATTLE_SUBSCRIPT_CHARGE_MOVE_CLEANUP
    End

_tooHeavy:
    PrintAttackMessage
    Wait
    WaitButtonABTime 30
    // {0} is too heavy to be lifted!
    PrintBufferedMessage
    Wait
    WaitButtonABTime 30
    UpdateVar OPCODE_FLAG_ON, BSCRIPT_VAR_MOVE_STATUS_FLAGS, MOVE_STATUS_NO_MORE_WORK
    End
