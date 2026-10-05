#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    Call BATTLE_SUBSCRIPT_ATTACK_MESSAGE_AND_ANIMATION
    UpdateVarFromVar OPCODE_SET, BSCRIPT_VAR_BATTLER_STAT_CHANGE, BSCRIPT_VAR_BATTLER_ATTACKER
    UpdateVar OPCODE_SET, BSCRIPT_VAR_SIDE_EFFECT_TYPE, SIDE_EFFECT_TYPE_MOVE_EFFECT
    Call BATTLE_SUBSCRIPT_BOOST_ALL_STATS
    TryNewMoveEffect NEW_MOVE_EFFECT_NO_RETREAT, _end
    // {0} can no longer escape because it used No Retreat!
    PrintBufferedMessage
    Wait
    WaitButtonABTime 30

_end:
    End
