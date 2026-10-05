#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    PrintAttackMessage
    Wait
    UpdateMonDataFromVar OPCODE_GET, BATTLER_CATEGORY_DEFENDER, BMON_DATA_MAXHP, BSCRIPT_VAR_HP_CALC
    // heals 2/3 of the target's max HP in Grassy Terrain instead of half
    GotoIfTerrainOverlayIsType GRASSY_TERRAIN, _healTwoThirds
    DivideVarByValueRoundUp BSCRIPT_VAR_HP_CALC, 2
    GoTo _heal

_healTwoThirds:
    UpdateVar OPCODE_MUL, BSCRIPT_VAR_HP_CALC, 2
    DivideVarByValueRoundUp BSCRIPT_VAR_HP_CALC, 3

_heal:
    UpdateVarFromVar OPCODE_SET, BSCRIPT_VAR_MSG_BATTLER_TEMP, BSCRIPT_VAR_BATTLER_TARGET
    Call BATTLE_SUBSCRIPT_RECOVER_HP
    End
