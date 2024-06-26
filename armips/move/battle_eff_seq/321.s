.nds
.thumb

.include "armips/include/battlescriptcmd.s"
.include "armips/include/abilities.s"
.include "armips/include/itemnums.s"
.include "armips/include/monnums.s"
.include "armips/include/movenums.s"

.create "build/move/battle_eff_seq/0_321", 0

a030_321:
    ifmonstat IF_NOTEQUAL, BATTLER_DEFENDER, MON_DATA_STATUS_1, 0, Status
    changevar VAR_OP_SET, VAR_DAMAGE_MULT, 0xA
    goto _end

Status:
    changevar VAR_OP_SET, VAR_DAMAGE_MULT, 0x14

_end:
    changevar VAR_OP_SET, VAR_ADD_STATUS2, 0x80000003
    critcalc
    damagecalc
    endscript

.close