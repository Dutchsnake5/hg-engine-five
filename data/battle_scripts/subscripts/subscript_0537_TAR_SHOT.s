#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    UpdateVar OPCODE_SET, BSCRIPT_VAR_SIDE_EFFECT_PARAM, MOVE_SUBSCRIPT_PTR_SPEED_DOWN_1_STAGE
    Call BATTLE_SUBSCRIPT_UPDATE_STAT_STAGE
    TryNewMoveEffect NEW_MOVE_EFFECT_TAR_SHOT, _end
    // {0} became weaker to fire!
    PrintBufferedMessage
    Wait
    WaitButtonABTime 30

_end:
    End
