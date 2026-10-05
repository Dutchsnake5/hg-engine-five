#include "constants/battle_constants.h"
.include "battle_commands.inc"

.data

_000:
    // Jungle Healing / Lunar Blessing: the user and its ally regain 1/4 of their max HP and are cured of status conditions
    Call BATTLE_SUBSCRIPT_ATTACK_MESSAGE_AND_ANIMATION
    TryNewMoveEffect NEW_MOVE_EFFECT_ITERATOR_RESET, _loop

_loop:
    TryNewMoveEffect NEW_MOVE_EFFECT_NEXT_SELF_OR_ALLY, _end
    TryNewMoveEffect NEW_MOVE_EFFECT_HEAL_QUARTER, _cure
    UpdateVar OPCODE_FLAG_ON, BSCRIPT_VAR_BATTLE_STATUS, BATTLE_STATUS_NO_BLINK
    Call BATTLE_SUBSCRIPT_UPDATE_HP
    // {0} regained health!
    PrintMessage 184, TAG_NICKNAME, BATTLER_CATEGORY_MSG_TEMP
    Wait
    WaitButtonABTime 30

_cure:
    TryNewMoveEffect NEW_MOVE_EFFECT_CURE_STATUS, _loop
    // {0}'s status returned to normal!
    PrintBufferedMessage
    Wait
    WaitButtonABTime 30
    SetHealthbarStatus BATTLER_CATEGORY_MSG_TEMP, BATTLE_ANIMATION_NONE
    GoTo _loop

_end:
    End
