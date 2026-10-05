#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    // the boosts were already stolen before damage was calculated, this just announces it
    TryNewMoveEffect NEW_MOVE_EFFECT_SPECTRAL_THIEF_MESSAGE, _noSteal
    UpdateVar OPCODE_SET, BSCRIPT_VAR_MOVE_EFFECT_CHANCE, 1
    Call BATTLE_SUBSCRIPT_ATTACK_MESSAGE_AND_ANIMATION
    // {0} stole the target's boosted stats!
    PrintBufferedMessage
    Wait
    WaitButtonABTime 30
    GoTo _end

_noSteal:
    CompareVarToValue OPCODE_FLAG_SET, BSCRIPT_VAR_MOVE_STATUS_FLAGS, MOVE_STATUS_DID_NOT_HIT, _end
    Call BATTLE_SUBSCRIPT_ATTACK_MESSAGE_AND_ANIMATION

_end:
    End
