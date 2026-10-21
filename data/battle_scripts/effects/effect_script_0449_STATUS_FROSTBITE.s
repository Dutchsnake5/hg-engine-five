#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

// Jack Frost (FROSTBITE in include/config.h): the frostbite counterpart of Will-O-Wisp (effect 167, STATUS_BURN)
_000:
    UpdateVar OPCODE_SET, BSCRIPT_VAR_SIDE_EFFECT_FLAGS_DIRECT, MOVE_SIDE_EFFECT_TO_DEFENDER|MOVE_SUBSCRIPT_PTR_FREEZE
    End
