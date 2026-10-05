#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    // a combined pledge makes a field effect after it hits
    UpdateVar OPCODE_SET, BSCRIPT_VAR_SIDE_EFFECT_FLAGS_INDIRECT, MOVE_SIDE_EFFECT_TO_ATTACKER|MOVE_SIDE_EFFECT_ON_HIT|MOVE_SUBSCRIPT_PTR_PLEDGE_EFFECT
    CalcCrit
    CalcDamage
    End
