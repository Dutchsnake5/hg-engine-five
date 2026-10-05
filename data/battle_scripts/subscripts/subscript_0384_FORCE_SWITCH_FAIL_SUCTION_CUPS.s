#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    CheckAbility CHECK_OPCODE_HAVE, BATTLER_CATEGORY_MSG_TEMP, ABILITY_GUARD_DOG, _guardDog
    // {0} is anchored in place with its suction cups!
    BufferMessage 659, TAG_NICKNAME_ABILITY, BATTLER_CATEGORY_DEFENDER, BATTLER_CATEGORY_MSG_TEMP
    GoTo _print

_guardDog:
    // {0} won't budge thanks to its Guard Dog!
    BufferMessage 1911, TAG_NICKNAME, BATTLER_CATEGORY_MSG_TEMP

_print:
    PrintAttackMessage 
    Wait 
    WaitButtonABTime 30
    PrintBufferedMessage 
    Wait 
    WaitButtonABTime 30
    End
