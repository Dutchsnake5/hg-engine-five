#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    // steal the target's boosts first so that they count for this hit
    TryNewMoveEffect NEW_MOVE_EFFECT_SPECTRAL_THIEF, _setMessage

_setMessage:
    UpdateVar OPCODE_SET, BSCRIPT_VAR_SIDE_EFFECT_FLAGS_DIRECT, MOVE_SIDE_EFFECT_TO_DEFENDER|MOVE_SIDE_EFFECT_BREAK_SCREENS|MOVE_SUBSCRIPT_PTR_SPECTRAL_THIEF
    CalcCrit
    CalcDamage
    End
