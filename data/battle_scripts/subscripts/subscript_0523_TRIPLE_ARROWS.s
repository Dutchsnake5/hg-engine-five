#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    // 50% chance to lower the target's Defense by 1 stage
    CheckEffectActivationWithChance 50, _Flinch
    UpdateVar OPCODE_SET, BSCRIPT_VAR_SIDE_EFFECT_PARAM, MOVE_SUBSCRIPT_PTR_DEFENSE_DOWN_1_STAGE
    Call BATTLE_SUBSCRIPT_UPDATE_STAT_STAGE

_Flinch:
    // 30% chance to make the target flinch
    CheckEffectActivationWithChance 30, _End
    Call BATTLE_SUBSCRIPT_FLINCH_MON

_End:
    End 
