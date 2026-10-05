#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    // with a commanding Tatsugiri in its mouth, Dondozo raises a stat depending on the Tatsugiri's form
    UpdateVar OPCODE_SET, BSCRIPT_VAR_SIDE_EFFECT_FLAGS_INDIRECT, MOVE_SIDE_EFFECT_TO_ATTACKER|MOVE_SIDE_EFFECT_ON_HIT|MOVE_SUBSCRIPT_PTR_ORDER_UP
    CalcCrit
    CalcDamage
    End
