#include "config.h"
#include "debug.h"
#include "types.h"

#include "constants/ability.h"
#include "constants/battle_message_constants.h"
#include "constants/battle_script_constants.h"
#include "constants/hold_item_effects.h"
#include "constants/item.h"
#include "constants/move_effects.h"
#include "constants/moves.h"
#include "constants/species.h"

#include "battle.h"
#include "pokemon.h"

/**
 *  This overlay holds the handlers for the TryNewMoveEffect battle script command.
 *  Each handler returns TRUE on success.  On failure the script jumps to the command's fail address.
 *
 *  Handlers that print a message set up sp->mp, so the script only has to use PrintBufferedMessage.
 *  Nothing in here may call into another overlay loaded at 0x023C0400 (CalcBaseDamage, SwitchInAbilityCheck, ...).
 */

#define BATTLE_MSG_PP_REDUCED         398
#define BATTLE_MSG_ABILITY_SUPPRESSED 1012
#define BATTLE_MSG_SPECTRAL_THIEF     1812
#define BATTLE_MSG_STATUS_CURED       491
#define BATTLE_MSG_PUMPED             276
#define BATTLE_MSG_COPIED_ABILITY     523
#define BATTLE_MSG_REFLECT_TYPE       1815
#define BATTLE_MSG_TOPSY_TURVY        1818
#define BATTLE_MSG_SPEED_SWAP         1821
#define BATTLE_MSG_TELEKINESIS        1824
#define BATTLE_MSG_ELECTRIFY          1830
#define BATTLE_MSG_NO_RETREAT         1833
#define BATTLE_MSG_TAR_SHOT           1836
#define BATTLE_MSG_OCTOLOCK           1839
#define BATTLE_MSG_COURT_CHANGE       1842
#define BATTLE_MSG_SALT_CURE          1845
#define BATTLE_MSG_SYRUP_BOMB         1851
#define BATTLE_MSG_CORROSIVE_GAS      1854
#define BATTLE_MSG_WONDER_ROOM        1857
#define BATTLE_MSG_WONDER_ROOM_END    1858
#define BATTLE_MSG_FAIRY_LOCK         1859
#define BATTLE_MSG_INSTRUCT           1869
#define BATTLE_MSG_REVIVAL            1872
#define BATTLE_MSG_SEA_OF_FIRE        1881
#define BATTLE_MSG_SWAMP              1885
#define BATTLE_MSG_RAINBOW            1889
#define BATTLE_MSG_SKY_DROP           1893
#define BATTLE_MSG_SKY_DROP_HEAVY     1896
#define BATTLE_MSG_COPIED_STAT_CHANGES 452
#define BATTLE_MSG_COMMANDER          1899
#define BATTLE_MSG_RECEIVER           1902
#define BATTLE_MSG_SYMBIOSIS          1905
#define BATTLE_MSG_ALLY_SWITCH        1914

#define EXTRA_ACTION_TYPE_INSTRUCT 1

static BOOL HandleRound(void *bw, struct BattleStruct *sp);
static BOOL HandleSpectralThief(void *bw, struct BattleStruct *sp);
static BOOL HandleCoreEnforcer(void *bw, struct BattleStruct *sp);
static BOOL HandleEerieSpell(void *bw, struct BattleStruct *sp);
static BOOL HandleReflectType(struct BattleStruct *sp);
static BOOL HandleTopsyTurvy(struct BattleStruct *sp);
static BOOL HandlePurify(struct BattleStruct *sp);
static BOOL HandleCourtChange(struct BattleStruct *sp);
static BOOL HandleNextBattler(void *bw, struct BattleStruct *sp, int effect);
static BOOL HandleDoodle(void *bw, struct BattleStruct *sp);
static BOOL HandleCorrosiveGas(struct BattleStruct *sp);
static void SetNicknameMessage(struct BattleStruct *sp, int id, int battler);
static BOOL HandleNextTeatime(void *bw, struct BattleStruct *sp);
static BOOL HandleInstruct(void *bw, struct BattleStruct *sp);
static BOOL HandleRevivalBlessing(void *bw, struct BattleStruct *sp);
static BOOL HandlePledgeEffect(void *bw, struct BattleStruct *sp);
static BOOL HandleNextOpportunist(struct BattleStruct *sp);
static BOOL HandleAllySwitch(void *bw, struct BattleStruct *sp);

BOOL __attribute__((section(".init"))) btl_scr_cmd_12D_TryNewMoveEffectInternal(void *bw, struct BattleStruct *sp)
{
    IncrementBattleScriptPtr(sp, 1);
    int effect = read_battle_script_param(sp);
    int failAddress = read_battle_script_param(sp);
    BOOL success = FALSE;

    switch (effect) {
    case NEW_MOVE_EFFECT_ROUND:
        success = HandleRound(bw, sp);
        break;
    case NEW_MOVE_EFFECT_ECHOED_VOICE:
        sp->echoedVoiceUsedThisTurn = TRUE;
        success = TRUE;
        break;
    case NEW_MOVE_EFFECT_STATS_RAISED_CHECK:
        success = sp->moveConditionsFlags[sp->state_client].statRaisedThisTurn;
        break;
    case NEW_MOVE_EFFECT_SPECTRAL_THIEF:
        success = HandleSpectralThief(bw, sp);
        break;
    case NEW_MOVE_EFFECT_SPECTRAL_THIEF_MESSAGE:
        success = sp->spectralThiefStole;
        sp->spectralThiefStole = FALSE;
        sp->mp.id = BATTLE_MSG_SPECTRAL_THIEF;
        sp->mp.tag = TAG_NICKNAME;
        sp->mp.param[0] = CreateNicknameTag(sp, sp->attack_client);
        break;
    case NEW_MOVE_EFFECT_CORE_ENFORCER:
        success = HandleCoreEnforcer(bw, sp);
        break;
    case NEW_MOVE_EFFECT_EERIE_SPELL:
        success = HandleEerieSpell(bw, sp);
        break;
    case NEW_MOVE_EFFECT_REFLECT_TYPE:
        success = HandleReflectType(sp);
        break;
    case NEW_MOVE_EFFECT_TOPSY_TURVY:
        success = HandleTopsyTurvy(sp);
        break;
    case NEW_MOVE_EFFECT_SPEED_SWAP: {
        u16 speed = sp->battlemon[sp->attack_client].speed;
        sp->battlemon[sp->attack_client].speed = sp->battlemon[sp->defence_client].speed;
        sp->battlemon[sp->defence_client].speed = speed;
        SetNicknameMessage(sp, BATTLE_MSG_SPEED_SWAP, sp->attack_client);
        success = TRUE;
        break;
    }
    case NEW_MOVE_EFFECT_PURIFY:
        success = HandlePurify(sp);
        break;
    case NEW_MOVE_EFFECT_TELEKINESIS:
        if (sp->telekinesisTurns[sp->defence_client] == 0) {
            sp->telekinesisTurns[sp->defence_client] = 3;
            SetNicknameMessage(sp, BATTLE_MSG_TELEKINESIS, sp->defence_client);
            success = TRUE;
        }
        break;
    case NEW_MOVE_EFFECT_WONDER_ROOM:
        // using Wonder Room while it is active ends it early
        sp->mp.tag = TAG_NONE;
        if (sp->wonderRoomTurns) {
            sp->wonderRoomTurns = 0;
            sp->mp.id = BATTLE_MSG_WONDER_ROOM_END;
        } else {
            sp->wonderRoomTurns = 5;
            sp->mp.id = BATTLE_MSG_WONDER_ROOM;
        }
        success = TRUE;
        break;
    case NEW_MOVE_EFFECT_ELECTRIFY:
        sp->electrified |= No2Bit(sp->defence_client);
        SetNicknameMessage(sp, BATTLE_MSG_ELECTRIFY, sp->defence_client);
        success = TRUE;
        break;
    case NEW_MOVE_EFFECT_FAIRY_LOCK:
        if (sp->fairyLockTurns == 0) {
            // lasts through the end of the next turn
            sp->fairyLockTurns = 2;
            sp->mp.id = BATTLE_MSG_FAIRY_LOCK;
            sp->mp.tag = TAG_NONE;
            success = TRUE;
        }
        break;
    case NEW_MOVE_EFFECT_NO_RETREAT:
        if (!(sp->noRetreat & No2Bit(sp->attack_client))) {
            sp->noRetreat |= No2Bit(sp->attack_client);
            SetNicknameMessage(sp, BATTLE_MSG_NO_RETREAT, sp->attack_client);
            success = TRUE;
        }
        break;
    case NEW_MOVE_EFFECT_TAR_SHOT:
        if (!(sp->tarShot & No2Bit(sp->defence_client)) && !sp->battlemon[sp->defence_client].is_currently_terastallized) {
            sp->tarShot |= No2Bit(sp->defence_client);
            SetNicknameMessage(sp, BATTLE_MSG_TAR_SHOT, sp->defence_client);
            success = TRUE;
        }
        break;
    case NEW_MOVE_EFFECT_OCTOLOCK:
        if (sp->octolockedBy[sp->defence_client] == 0) {
            sp->octolockedBy[sp->defence_client] = sp->attack_client + 1;
            SetNicknameMessage(sp, BATTLE_MSG_OCTOLOCK, sp->defence_client);
            success = TRUE;
        }
        break;
    case NEW_MOVE_EFFECT_COURT_CHANGE:
        success = HandleCourtChange(sp);
        break;
    case NEW_MOVE_EFFECT_DRAGON_CHEER:
        if (sp->dragonCheerBoost[sp->defence_client] == 0) {
            // Dragon types get pumped up more
            sp->dragonCheerBoost[sp->defence_client] = HasType(sp, sp->defence_client, TYPE_DRAGON) ? 2 : 1;
            SetNicknameMessage(sp, BATTLE_MSG_PUMPED, sp->defence_client);
            success = TRUE;
        }
        break;
    case NEW_MOVE_EFFECT_ITERATOR_RESET:
        sp->fieldIterator = 0;
        success = TRUE;
        break;
    case NEW_MOVE_EFFECT_NEXT_PLUS_MINUS:
    case NEW_MOVE_EFFECT_NEXT_FLOWER_SHIELD:
    case NEW_MOVE_EFFECT_NEXT_ROTOTILLER:
    case NEW_MOVE_EFFECT_NEXT_SELF_OR_ALLY:
        success = HandleNextBattler(bw, sp, effect);
        break;
    case NEW_MOVE_EFFECT_HEAL_QUARTER: {
        struct BattlePokemon *mon = &sp->battlemon[sp->state_client];
        if (mon->hp && mon->hp < (s32)mon->maxhp && mon->moveeffect.healBlockTurns == 0) {
            sp->hp_calc_work = (mon->maxhp + 3) / 4;
            success = TRUE;
        }
        break;
    }
    case NEW_MOVE_EFFECT_CURE_STATUS:
        if (sp->battlemon[sp->state_client].condition & STATUS_ALL) {
            sp->battlemon[sp->state_client].condition = 0;
            SetNicknameMessage(sp, BATTLE_MSG_STATUS_CURED, sp->state_client);
            success = TRUE;
        }
        break;
    case NEW_MOVE_EFFECT_NEXT_DOODLE:
        success = HandleDoodle(bw, sp);
        break;
    case NEW_MOVE_EFFECT_SALT_CURE:
        if (!(sp->saltCure & No2Bit(sp->state_client)) && sp->battlemon[sp->state_client].hp) {
            sp->saltCure |= No2Bit(sp->state_client);
            SetNicknameMessage(sp, BATTLE_MSG_SALT_CURE, sp->state_client);
            success = TRUE;
        }
        break;
    case NEW_MOVE_EFFECT_SYRUP_BOMB:
        if (sp->syrupBombTurns[sp->state_client] == 0 && sp->battlemon[sp->state_client].hp) {
            sp->syrupBombTurns[sp->state_client] = 3;
            sp->syrupBombSource[sp->state_client] = sp->attack_client;
            SetNicknameMessage(sp, BATTLE_MSG_SYRUP_BOMB, sp->state_client);
            success = TRUE;
        }
        break;
    case NEW_MOVE_EFFECT_CORROSIVE_GAS:
        success = HandleCorrosiveGas(sp);
        break;
    case NEW_MOVE_EFFECT_TEATIME_CHECK: {
        int maxBattlers = BattleWorkClientSetMaxGet(bw);
        sp->teatimeAttacker = sp->attack_client;
        for (int i = 0; i < maxBattlers; i++) {
            if (sp->battlemon[i].hp && IS_ITEM_BERRY(sp->battlemon[i].item)) {
                success = TRUE;
            }
        }
        break;
    }
    case NEW_MOVE_EFFECT_NEXT_TEATIME:
        success = HandleNextTeatime(bw, sp);
        break;
    case NEW_MOVE_EFFECT_INSTRUCT:
        success = HandleInstruct(bw, sp);
        break;
    case NEW_MOVE_EFFECT_REVIVAL_BLESSING:
        success = HandleRevivalBlessing(bw, sp);
        break;
    case NEW_MOVE_EFFECT_PLEDGE_EFFECT:
        success = HandlePledgeEffect(bw, sp);
        break;
    case NEW_MOVE_EFFECT_SKY_DROP_LIFT: {
        // the target is too heavy to be lifted
        int target = sp->defence_client;
        if (GetPokemonWeight(bw, sp, sp->attack_client, target) >= 2000) {
            SetNicknameMessage(sp, BATTLE_MSG_SKY_DROP_HEAVY, target);
            break;
        }
        sp->skyDropTarget[sp->attack_client] = target + 1;
        sp->skyDroppedBy[target] = sp->attack_client + 1;
        // the post-move vanish check brings the target back down once this is cleared from effect_of_moves
        sp->battlemon[target].effect_of_moves |= MOVE_EFFECT_FLAG_FLY;
        sp->battlemon[target].effect_of_moves_temp |= MOVE_EFFECT_FLAG_FLY;
        // nothing hits on the first turn, so there is no effectiveness to report
        sp->moveStatusFlagForSpreadMoves[target] &= ~(MOVE_STATUS_SUPER_EFFECTIVE | MOVE_STATUS_NOT_VERY_EFFECTIVE);
        sp->waza_status_flag &= ~(MOVE_STATUS_SUPER_EFFECTIVE | MOVE_STATUS_NOT_VERY_EFFECTIVE);
        SetNicknameMessage(sp, BATTLE_MSG_SKY_DROP, target);
        success = TRUE;
        break;
    }
    case NEW_MOVE_EFFECT_POISON_PUPPETEER: {
        // Pecharunt's poisonous moves also confuse
        int target = sp->state_client;
        if (GetBattlerAbility(sp, sp->attack_client) == ABILITY_POISON_PUPPETEER
            && sp->battlemon[sp->attack_client].species == SPECIES_PECHARUNT
            && target != sp->attack_client
            && (sp->addeffect_type == SIDE_EFFECT_TYPE_DIRECT || sp->addeffect_type == SIDE_EFFECT_TYPE_INDIRECT)
            && sp->battlemon[target].hp
            && !(sp->battlemon[target].condition2 & STATUS2_CONFUSION)) {
            sp->puppeteerSavedEffectType = sp->addeffect_type;
            sp->addeffect_type = SIDE_EFFECT_TYPE_INDIRECT;
            success = TRUE;
        }
        break;
    }
    case NEW_MOVE_EFFECT_PUPPETEER_RESTORE:
        sp->addeffect_type = sp->puppeteerSavedEffectType;
        success = TRUE;
        break;
    case NEW_MOVE_EFFECT_RECEIVER_CHECK: {
        // Receiver and Power of Alchemy take over a fainted ally's ability
        int fainted = sp->fainting_client;
        int ally = BATTLER_ALLY(fainted);
        u32 ability = sp->battlemon[fainted].ability;
        if (ally < BattleWorkClientSetMaxGet(bw)
            && sp->battlemon[ally].hp
            && (GetBattlerAbility(sp, ally) == ABILITY_RECEIVER || GetBattlerAbility(sp, ally) == ABILITY_POWER_OF_ALCHEMY)
            && ability != ABILITY_NONE
            && !GetAbilityFlags(ability).failsReceiver) {
            sp->battlerIdTemp = ally;
            sp->mp.id = BATTLE_MSG_RECEIVER;
            sp->mp.tag = TAG_NICKNAME_ABILITY;
            sp->mp.param[0] = CreateNicknameTag(sp, fainted);
            sp->mp.param[1] = ability;
            success = TRUE;
        }
        break;
    }
    case NEW_MOVE_EFFECT_RECEIVER_APPLY:
        sp->battlemon[sp->battlerIdTemp].ability = sp->battlemon[sp->fainting_client].ability;
        sp->battlemon[sp->battlerIdTemp].ability_activated_flag = 0;
        success = TRUE;
        break;
    case NEW_MOVE_EFFECT_COSTAR: {
        // copy the ally's stat changes and critical hit boosts
        int battler = sp->battlerIdTemp;
        int ally = BATTLER_ALLY(battler);
        for (int stat = STAT_ATTACK; stat <= STAT_EVASION; stat++) {
            sp->battlemon[battler].states[stat] = sp->battlemon[ally].states[stat];
        }
        sp->battlemon[battler].condition2 = (sp->battlemon[battler].condition2 & ~STATUS2_FOCUS_ENERGY) | (sp->battlemon[ally].condition2 & STATUS2_FOCUS_ENERGY);
        sp->dragonCheerBoost[battler] = sp->dragonCheerBoost[ally];
        sp->mp.id = BATTLE_MSG_COPIED_STAT_CHANGES;
        sp->mp.tag = TAG_NICKNAME_NICKNAME;
        sp->mp.param[0] = CreateNicknameTag(sp, battler);
        sp->mp.param[1] = CreateNicknameTag(sp, ally);
        success = TRUE;
        break;
    }
    case NEW_MOVE_EFFECT_COMMANDER: {
        // Tatsugiri hides in Dondozo's mouth, and Dondozo's stats are raised next
        int tatsugiri = sp->battlerIdTemp;
        int dondozo = BATTLER_ALLY(tatsugiri);
        sp->commanding |= No2Bit(tatsugiri);
        sp->commandedBy[dondozo] = tatsugiri + 1;
        sp->state_client = dondozo;
        sp->addeffect_type = SIDE_EFFECT_TYPE_ABILITY;
        SetNicknameMessage(sp, BATTLE_MSG_COMMANDER, tatsugiri);
        success = TRUE;
        break;
    }
    case NEW_MOVE_EFFECT_COMMANDER_RELEASE: {
        // Tatsugiri comes back out when its Dondozo faints
        int dondozo = sp->fainting_client;
        if (sp->commandedBy[dondozo]) {
            int tatsugiri = sp->commandedBy[dondozo] - 1;
            sp->commanding &= ~No2Bit(tatsugiri);
            sp->commandedBy[dondozo] = 0;
            sp->battlerIdTemp = tatsugiri;
            success = TRUE;
        }
        break;
    }
    case NEW_MOVE_EFFECT_NEXT_OPPORTUNIST:
        success = HandleNextOpportunist(sp);
        break;
    case NEW_MOVE_EFFECT_SYMBIOSIS: {
        // the Symbiosis ally hands over its own item
        int recipient = sp->battlerIdTemp;
        int giver = BATTLER_ALLY(recipient);
        sp->battlemon[recipient].item = sp->battlemon[giver].item;
        sp->battlemon[giver].item = ITEM_NONE;
        CopyBattleMonToPartyMon(bw, sp, recipient);
        CopyBattleMonToPartyMon(bw, sp, giver);
        sp->mp.id = BATTLE_MSG_SYMBIOSIS;
        sp->mp.tag = TAG_NICKNAME_ITEM;
        sp->mp.param[0] = CreateNicknameTag(sp, recipient);
        sp->mp.param[1] = sp->battlemon[recipient].item;
        success = TRUE;
        break;
    }
    case NEW_MOVE_EFFECT_ORDER_UP: {
        // the commanding Tatsugiri's form decides which stat goes up: Curly Attack, Droopy Defense, Stretchy Speed
        int dondozo = sp->attack_client;
        if (sp->commandedBy[dondozo]) {
            int tatsugiri = sp->commandedBy[dondozo] - 1;
            sp->addeffect_param = MOVE_SUBSCRIPT_PTR_ATTACK_UP_1_STAGE + (sp->battlemon[tatsugiri].form_no % 3);
            sp->state_client = dondozo;
            sp->addeffect_type = SIDE_EFFECT_TYPE_MOVE_EFFECT;
            success = TRUE;
        }
        break;
    }
    case NEW_MOVE_EFFECT_ALLY_SWITCH:
        success = HandleAllySwitch(bw, sp);
        break;
    case NEW_MOVE_EFFECT_GRAVITY_TELEKINESIS:
        // gravity brings down anything held up by Telekinesis
        if (sp->telekinesisTurns[sp->battlerIdTemp]) {
            sp->telekinesisTurns[sp->battlerIdTemp] = 0;
            success = TRUE;
        }
        break;
    default:
        break;
    }

    if (!success) {
        IncrementBattleScriptPtr(sp, failAddress);
    }

    return FALSE;
}

/**
 *  @brief the move a battler is still going to use this turn, or MOVE_NONE if it already acted or isn't using a move
 */
static u32 GetPendingMove(struct BattleStruct *sp, int battler)
{
    if (sp->playerActions[battler][0] != CONTROLLER_COMMAND_FIGHT_INPUT) {
        return MOVE_NONE;
    }
    if (sp->oneTurnFlag[battler].struggle_flag) {
        return MOVE_STRUGGLE;
    }
    return sp->battlemon[battler].move[sp->waza_no_pos[battler]];
}

/**
 *  @brief whether a battler has already taken its action this turn
 */
static BOOL HasActedThisTurn(void *bw, struct BattleStruct *sp, int battler)
{
    int maxBattlers = BattleWorkClientSetMaxGet(bw);
    for (int i = 0; i < maxBattlers; i++) {
        if (sp->executionOrder[i] == battler) {
            return i < sp->executionIndex;
        }
    }
    return FALSE;
}

/**
 *  @brief Round: later Rounds are doubled in power, and everyone else using Round this turn goes next
 */
static BOOL HandleRound(void *bw, struct BattleStruct *sp)
{
    int maxBattlers = BattleWorkClientSetMaxGet(bw);

    sp->roundUsedThisTurn = TRUE;
    for (int i = 0; i < maxBattlers; i++) {
        if (i != sp->attack_client && sp->battlemon[i].hp && GetPendingMove(sp, i) == MOVE_ROUND) {
            sp->oneTurnFlag[i].forceExecutionOrderFlag = EXECUTION_ORDER_AFTER_YOU;
        }
    }
    return TRUE;
}

/**
 *  @brief Spectral Thief: steal the target's stat boosts before attacking
 */
static BOOL HandleSpectralThief(void *bw UNUSED, struct BattleStruct *sp)
{
    struct BattlePokemon *attacker = &sp->battlemon[sp->attack_client];
    struct BattlePokemon *defender = &sp->battlemon[sp->defence_client];
    BOOL stole = FALSE;

    for (int stat = STAT_ATTACK; stat <= STAT_EVASION; stat++) {
        if (defender->states[stat] > 6) {
            int boost = defender->states[stat] - 6;
            attacker->states[stat] += boost;
            if (attacker->states[stat] > 12) {
                attacker->states[stat] = 12;
            }
            defender->states[stat] = 6;
            stole = TRUE;
        }
    }

    sp->spectralThiefStole = stole;
    return stole;
}

/**
 *  @brief Core Enforcer: suppress the target's ability if it already acted this turn
 */
static BOOL HandleCoreEnforcer(void *bw, struct BattleStruct *sp)
{
    int target = sp->state_client;

    if (sp->battlemon[target].hp == 0
        || !HasActedThisTurn(bw, sp, target)
        || (sp->battlemon[target].effect_of_moves & MOVE_EFFECT_FLAG_ABILITY_SUPPRESSED)
        || GetAbilityFlags(sp->battlemon[target].ability).failsSuppress) {
        return FALSE;
    }

    sp->battlemon[target].effect_of_moves |= MOVE_EFFECT_FLAG_ABILITY_SUPPRESSED;
    sp->mp.id = BATTLE_MSG_ABILITY_SUPPRESSED;
    sp->mp.tag = TAG_NICKNAME;
    sp->mp.param[0] = CreateNicknameTag(sp, target);
    return TRUE;
}

/**
 *  @brief Eerie Spell: the target's last used move loses 3 PP
 */
static BOOL HandleEerieSpell(void *bw, struct BattleStruct *sp)
{
    int target = sp->state_client;
    u32 move = sp->waza_no_old[target];

    if (sp->battlemon[target].hp == 0 || move == MOVE_NONE) {
        return FALSE;
    }

    for (int i = 0; i < 4; i++) {
        if (sp->battlemon[target].move[i] == move) {
            int reduction = sp->battlemon[target].pp[i] < 3 ? sp->battlemon[target].pp[i] : 3;
            if (reduction == 0) {
                return FALSE;
            }
            sp->battlemon[target].pp[i] -= reduction;
            CopyBattleMonToPartyMon(bw, sp, target);
            sp->mp.id = BATTLE_MSG_PP_REDUCED;
            sp->mp.tag = TAG_NICKNAME_MOVE_NUMBER;
            sp->mp.param[0] = CreateNicknameTag(sp, target);
            sp->mp.param[1] = move;
            sp->mp.param[2] = reduction;
            return TRUE;
        }
    }
    return FALSE;
}

static void SetNicknameMessage(struct BattleStruct *sp, int id, int battler)
{
    sp->mp.id = id;
    sp->mp.tag = TAG_NICKNAME;
    sp->mp.param[0] = CreateNicknameTag(sp, battler);
}

/**
 *  @brief Reflect Type: the user's types become the target's
 */
static BOOL HandleReflectType(struct BattleStruct *sp)
{
    struct BattlePokemon *attacker = &sp->battlemon[sp->attack_client];
    struct BattlePokemon *defender = &sp->battlemon[sp->defence_client];

    attacker->type1 = defender->type1;
    attacker->type2 = defender->type2;
    attacker->type3 = defender->type3;
    SetNicknameMessage(sp, BATTLE_MSG_REFLECT_TYPE, sp->attack_client);
    return TRUE;
}

/**
 *  @brief Topsy-Turvy: invert the target's stat changes
 */
static BOOL HandleTopsyTurvy(struct BattleStruct *sp)
{
    struct BattlePokemon *defender = &sp->battlemon[sp->defence_client];
    BOOL changed = FALSE;

    for (int stat = STAT_ATTACK; stat <= STAT_EVASION; stat++) {
        if (defender->states[stat] != 6) {
            defender->states[stat] = 12 - defender->states[stat];
            changed = TRUE;
        }
    }
    SetNicknameMessage(sp, BATTLE_MSG_TOPSY_TURVY, sp->defence_client);
    return changed;
}

/**
 *  @brief Purify: cure the target's status, and the user regains half its HP if that worked
 */
static BOOL HandlePurify(struct BattleStruct *sp)
{
    struct BattlePokemon *defender = &sp->battlemon[sp->defence_client];
    struct BattlePokemon *attacker = &sp->battlemon[sp->attack_client];

    if (!(defender->condition & STATUS_ALL)) {
        return FALSE;
    }
    defender->condition = 0;
    defender->condition2 &= ~STATUS2_NIGHTMARE;
    SetNicknameMessage(sp, BATTLE_MSG_STATUS_CURED, sp->defence_client);
    sp->hp_calc_work = (attacker->maxhp + 1) / 2;
    sp->battlerIdTemp = sp->attack_client;
    return TRUE;
}

/**
 *  @brief Court Change: swap the screens, hazards and other effects on each side of the field
 */
static BOOL HandleCourtChange(struct BattleStruct *sp)
{
    u32 swapMask = SIDE_STATUS_REFLECT | SIDE_STATUS_LIGHT_SCREEN | SIDE_STATUS_SPIKES | SIDE_STATUS_SAFEGUARD
        | SIDE_STATUS_MIST | SIDE_STATUS_STEALTH_ROCK | SIDE_STATUS_TOXIC_SPIKES | SIDE_STATUS_STICKY_WEB
        | SIDE_STATUS_LUCKY_CHANT | SIDE_STATUS_AURORA_VEIL;
    u32 side0 = sp->side_condition[0];
    u32 side1 = sp->side_condition[1];
    struct side_condition_work original0 = sp->scw[0];
    struct side_condition_work original1 = sp->scw[1];
    u8 temp;

    sp->side_condition[0] = (side0 & ~swapMask) | (side1 & swapMask);
    sp->side_condition[1] = (side1 & ~swapMask) | (side0 & swapMask);

    sp->scw[0] = original1;
    sp->scw[1] = original0;
    // follow me and the knocked off items stay with their own side
    sp->scw[0].followMeFlag = original0.followMeFlag;
    sp->scw[0].battlerIdFollowMe = original0.battlerIdFollowMe;
    sp->scw[0].knockoff_item = original0.knockoff_item;
    sp->scw[1].followMeFlag = original1.followMeFlag;
    sp->scw[1].battlerIdFollowMe = original1.battlerIdFollowMe;
    sp->scw[1].knockoff_item = original1.knockoff_item;

    temp = sp->tailwindCount[0];
    sp->tailwindCount[0] = sp->tailwindCount[1];
    sp->tailwindCount[1] = temp;

    for (int i = 0; i < NUM_HAZARD_IDX; i++) {
        temp = sp->entryHazardQueue[0][i];
        sp->entryHazardQueue[0][i] = sp->entryHazardQueue[1][i];
        sp->entryHazardQueue[1][i] = temp;
    }

    SetNicknameMessage(sp, BATTLE_MSG_COURT_CHANGE, sp->attack_client);
    return TRUE;
}

static BOOL IsBattlerSemiInvulnerable(struct BattleStruct *sp, int battler)
{
    return (sp->battlemon[battler].effect_of_moves & MOVE_EFFECT_FLAG_SEMI_INVULNERABLE) != 0;
}

/**
 *  @brief step sp->fieldIterator to the next battler affected by a move that hits several battlers one at a time.
 *         sets that battler up as the side effect target
 */
static BOOL HandleNextBattler(void *bw, struct BattleStruct *sp, int effect)
{
    int maxBattlers = BattleWorkClientSetMaxGet(bw);
    BOOL selfAndAllyOnly = (effect == NEW_MOVE_EFFECT_NEXT_PLUS_MINUS || effect == NEW_MOVE_EFFECT_NEXT_SELF_OR_ALLY);

    while (sp->fieldIterator < maxBattlers) {
        int battler;
        if (selfAndAllyOnly) {
            // the user first, then its ally
            if (sp->fieldIterator >= 2) {
                break;
            }
            battler = sp->fieldIterator == 0 ? sp->attack_client : BATTLER_ALLY(sp->attack_client);
            if (battler >= maxBattlers) {
                sp->fieldIterator++;
                continue;
            }
        } else {
            battler = sp->fieldIterator;
        }
        sp->fieldIterator++;

        if (sp->battlemon[battler].hp == 0) {
            continue;
        }

        BOOL affected = FALSE;
        switch (effect) {
        case NEW_MOVE_EFFECT_NEXT_PLUS_MINUS: {
            u32 ability = GetBattlerAbility(sp, battler);
            affected = (ability == ABILITY_PLUS || ability == ABILITY_MINUS) && !IsBattlerSemiInvulnerable(sp, battler);
            break;
        }
        case NEW_MOVE_EFFECT_NEXT_FLOWER_SHIELD:
            affected = HasType(sp, battler, TYPE_GRASS) && !IsBattlerSemiInvulnerable(sp, battler);
            break;
        case NEW_MOVE_EFFECT_NEXT_ROTOTILLER:
            affected = HasType(sp, battler, TYPE_GRASS) && IsClientGrounded(sp, battler) && !IsBattlerSemiInvulnerable(sp, battler);
            break;
        case NEW_MOVE_EFFECT_NEXT_SELF_OR_ALLY:
            affected = TRUE;
            break;
        }

        if (affected) {
            sp->state_client = battler;
            sp->battlerIdTemp = battler;
            sp->addeffect_type = SIDE_EFFECT_TYPE_MOVE_EFFECT;
            return TRUE;
        }
    }
    return FALSE;
}

/**
 *  @brief Doodle: the user and its ally get the target's ability.  steps sp->fieldIterator over the user and its ally
 */
static BOOL HandleDoodle(void *bw, struct BattleStruct *sp)
{
    int maxBattlers = BattleWorkClientSetMaxGet(bw);
    u32 newAbility = sp->battlemon[sp->defence_client].ability;

    while (sp->fieldIterator < 2) {
        int battler = sp->fieldIterator == 0 ? sp->attack_client : BATTLER_ALLY(sp->attack_client);
        sp->fieldIterator++;

        if (battler >= maxBattlers || sp->battlemon[battler].hp == 0
            || sp->battlemon[battler].ability == newAbility
            || GetAbilityFlags(sp->battlemon[battler].ability).failsSuppress) {
            continue;
        }

        sp->battlemon[battler].ability = newAbility;
        sp->battlemon[battler].ability_activated_flag = 0;
        sp->mp.id = BATTLE_MSG_COPIED_ABILITY;
        sp->mp.tag = TAG_NICKNAME_NICKNAME;
        sp->mp.param[0] = CreateNicknameTag(sp, battler);
        sp->mp.param[1] = CreateNicknameTag(sp, sp->defence_client);
        sp->battlerIdTemp = battler;
        return TRUE;
    }
    return FALSE;
}

/**
 *  @brief Corrosive Gas: melt away the target's held item
 */
static BOOL HandleCorrosiveGas(struct BattleStruct *sp)
{
    int target = sp->defence_client;
    struct BattlePokemon *mon = &sp->battlemon[target];

    if (mon->item == ITEM_NONE
        || !CanItemBeRemovedFromClient(mon->species, mon->item, mon->form_no)
        || GetBattlerAbility(sp, target) == ABILITY_STICKY_HOLD) {
        return FALSE;
    }

    sp->mp.id = BATTLE_MSG_CORROSIVE_GAS;
    sp->mp.tag = TAG_NICKNAME_ITEM;
    sp->mp.param[0] = CreateNicknameTag(sp, target);
    sp->mp.param[1] = mon->item;
    mon->item = ITEM_NONE;
    return TRUE;
}

/**
 *  @brief Teatime: everyone on the field eats their berry, in speed order.
 *         the berry's effect is set up the same way as when it is flung, and the eater stands in as attacker and defender
 */
static BOOL HandleNextTeatime(void *bw, struct BattleStruct *sp)
{
    int maxBattlers = BattleWorkClientSetMaxGet(bw);

    while (sp->fieldIterator < maxBattlers) {
        int battler = sp->turnOrder[sp->fieldIterator];
        sp->fieldIterator++;

        if (sp->battlemon[battler].hp && IS_ITEM_BERRY(sp->battlemon[battler].item)) {
            sp->attack_client = battler;
            sp->defence_client = battler;
            sp->battlerIdTemp = battler;
            sp->item_work = sp->battlemon[battler].item;
            TryFling(bw, sp, battler);
            return TRUE;
        }
    }

    // everyone is done eating
    sp->attack_client = sp->teatimeAttacker;
    sp->defence_client = sp->teatimeAttacker;
    return FALSE;
}

/**
 *  @brief Instruct: the target uses its last move again right away.
 *         the move is queued up as an extra action, the same way Dancer does it
 */
static BOOL HandleInstruct(void *bw UNUSED, struct BattleStruct *sp)
{
    int target = sp->defence_client;
    u32 move = sp->waza_no_old[target];
    int slot;

    if (move == MOVE_NONE || sp->battlemon[target].hp == 0 || sp->dancerContext.isActive) {
        return FALSE;
    }

    switch (move) {
    case MOVE_INSTRUCT:
    case MOVE_ASSIST:
    case MOVE_BEAK_BLAST:
    case MOVE_BELCH:
    case MOVE_BIDE:
    case MOVE_COPYCAT:
    case MOVE_FOCUS_PUNCH:
    case MOVE_ICE_BALL:
    case MOVE_KINGS_SHIELD:
    case MOVE_ME_FIRST:
    case MOVE_METRONOME:
    case MOVE_MIMIC:
    case MOVE_MIRROR_MOVE:
    case MOVE_NATURE_POWER:
    case MOVE_OBSTRUCT:
    case MOVE_OUTRAGE:
    case MOVE_PETAL_DANCE:
    case MOVE_THRASH:
    case MOVE_ROLLOUT:
    case MOVE_SHELL_TRAP:
    case MOVE_SKETCH:
    case MOVE_SLEEP_TALK:
    case MOVE_STRUGGLE:
    case MOVE_TRANSFORM:
    case MOVE_UPROAR:
        return FALSE;
    default:
        break;
    }
    if (CheckMoveIsChargeMove(sp, move) || sp->moveTbl[move].effect == MOVE_EFFECT_RECHARGE_AFTER) {
        return FALSE;
    }

    for (slot = 0; slot < 4; slot++) {
        if (sp->battlemon[target].move[slot] == move) {
            break;
        }
    }
    if (slot == 4 || sp->battlemon[target].pp[slot] == 0) {
        return FALSE;
    }

    int moveTarget = sp->lastMoveTarget[target];
    if (sp->battlemon[moveTarget].hp == 0) {
        moveTarget = BATTLER_OPPONENT(target);
    }

    sp->dancerContext.isActive = TRUE;
    sp->dancerContext.originalAttacker = sp->attack_client;
    sp->dancerContext.originalDefender = sp->defence_client;
    for (int i = 0; i < CLIENT_MAX; i++) {
        if (sp->dancerContext.extraActions[i].moveNumberOrAction == 0) {
            sp->dancerContext.extraActions[i].moveNumberOrAction = move;
            sp->dancerContext.extraActions[i].attacker = target;
            sp->dancerContext.extraActions[i].defender = moveTarget;
            sp->dancerContext.extraActions[i].type = EXTRA_ACTION_TYPE_INSTRUCT;
            break;
        }
    }

    SetNicknameMessage(sp, BATTLE_MSG_INSTRUCT, target);
    return TRUE;
}

/**
 *  @brief Revival Blessing: bring back the first fainted party member with half of its HP
 */
static BOOL HandleRevivalBlessing(void *bw, struct BattleStruct *sp)
{
    int client = sp->attack_client;
    struct Party *party = BattleWorkPokePartyGet(bw, client);

    for (int i = 0; i < party->count; i++) {
        struct PartyPokemon *mon = Party_GetMonByIndex(party, i);
        if (GetMonData(mon, MON_DATA_SPECIES, NULL) == SPECIES_NONE
            || GetMonData(mon, MON_DATA_IS_EGG, NULL)
            || GetMonData(mon, MON_DATA_HP, NULL) != 0) {
            continue;
        }

        u32 hp = GetMonData(mon, MON_DATA_MAXHP, NULL) / 2;
        u32 status = 0;
        if (hp == 0) {
            hp = 1;
        }
        SetMonData(mon, MON_DATA_HP, &hp);
        SetMonData(mon, MON_DATA_STATUS, &status);

        sp->mp.id = BATTLE_MSG_REVIVAL;
        sp->mp.tag = TAG_NICKNAME;
        sp->mp.param[0] = client | (i << 8);
        return TRUE;
    }
    return FALSE;
}

/**
 *  @brief set up the field effect of a combined pledge.  the ally's held back pledge decides which one
 */
static BOOL HandlePledgeEffect(void *bw, struct BattleStruct *sp)
{
    int attacker = sp->attack_client;
    int ally = BATTLER_ALLY(attacker);
    u32 heldBack = sp->pledgeWaitingMove[ally];
    u32 move = sp->current_move_index;
    int userSide = IsClientEnemy(bw, attacker);
    int targetSide = userSide ^ 1;
    int sideBattler;

    if (heldBack == MOVE_NONE || heldBack == move) {
        return FALSE;
    }
    sp->pledgeWaitingMove[ally] = MOVE_NONE;

    if ((move == MOVE_FIRE_PLEDGE && heldBack == MOVE_GRASS_PLEDGE) || (move == MOVE_GRASS_PLEDGE && heldBack == MOVE_FIRE_PLEDGE)) {
        sp->seaOfFireTurns[targetSide] = 4;
        sp->mp.id = BATTLE_MSG_SEA_OF_FIRE;
        sideBattler = BATTLER_OPPONENT(attacker);
    } else if ((move == MOVE_GRASS_PLEDGE && heldBack == MOVE_WATER_PLEDGE) || (move == MOVE_WATER_PLEDGE && heldBack == MOVE_GRASS_PLEDGE)) {
        sp->swampTurns[targetSide] = 4;
        sp->mp.id = BATTLE_MSG_SWAMP;
        sideBattler = BATTLER_OPPONENT(attacker);
    } else {
        sp->rainbowTurns[userSide] = 4;
        sp->mp.id = BATTLE_MSG_RAINBOW;
        sideBattler = attacker;
    }
    sp->mp.tag = TAG_NONE_SIDE;
    sp->mp.param[0] = sideBattler;
    return TRUE;
}

/**
 *  @brief apply the next stat boost an Opportunist battler copied from its opponents
 */
static BOOL HandleNextOpportunist(struct BattleStruct *sp)
{
    int battler = sp->battlerIdTemp;

    for (int stat = STAT_ATTACK; stat <= STAT_EVASION; stat++) {
        int boost = sp->opportunistBoosts[battler][stat];
        if (boost) {
            if (boost > 3) {
                boost = 3;
            }
            sp->opportunistBoosts[battler][stat] -= boost;
            switch (boost) {
            case 1:
                sp->addeffect_param = MOVE_SUBSCRIPT_PTR_ATTACK_UP_1_STAGE;
                break;
            case 2:
                sp->addeffect_param = MOVE_SUBSCRIPT_PTR_ATTACK_UP_2_STAGES;
                break;
            default:
                sp->addeffect_param = MOVE_SUBSCRIPT_PTR_ATTACK_UP_3_STAGES;
                break;
            }
            sp->addeffect_param += stat - STAT_ATTACK;
            sp->state_client = battler;
            sp->addeffect_type = SIDE_EFFECT_TYPE_ABILITY;
            sp->opportunistApplying = TRUE;
            return TRUE;
        }
    }
    sp->opportunistApplying = FALSE;
    return FALSE;
}

#define SWAP(a, b)              \
    do {                        \
        __typeof__(a) swapTemp = (a); \
        (a) = (b);              \
        (b) = swapTemp;         \
    } while (0)

static u8 SwapBitmaskBits(u8 mask, int a, int b)
{
    u8 bitA = (mask >> a) & 1;
    u8 bitB = (mask >> b) & 1;
    mask &= ~(No2Bit(a) | No2Bit(b));
    return mask | (bitA << b) | (bitB << a);
}

/**
 *  @brief Ally Switch: the user and its ally trade places.
 *         everything that belongs to the Pokemon moves with it, including its pending action this turn
 */
static BOOL HandleAllySwitch(void *bw, struct BattleStruct *sp)
{
    int a = sp->attack_client;
    int b = BATTLER_ALLY(a);
    int maxBattlers = BattleWorkClientSetMaxGet(bw);

    if (b >= maxBattlers || sp->battlemon[b].hp == 0) {
        return FALSE;
    }

    SWAP(sp->battlemon[a], sp->battlemon[b]);
    SWAP(sp->sel_mons_no[a], sp->sel_mons_no[b]);
    for (int i = 0; i < 4; i++) {
        SWAP(sp->playerActions[a][i], sp->playerActions[b][i]);
    }
    SWAP(sp->waza_no_pos[a], sp->waza_no_pos[b]);
    SWAP(sp->waza_no_select[a], sp->waza_no_select[b]);
    SWAP(sp->waza_no_old[a], sp->waza_no_old[b]);
    SWAP(sp->waza_no_keep[a], sp->waza_no_keep[b]);
    SWAP(sp->oneTurnFlag[a], sp->oneTurnFlag[b]);
    SWAP(sp->oneSelfFlag[a], sp->oneSelfFlag[b]);
    SWAP(sp->moveConditionsFlags[a], sp->moveConditionsFlags[b]);
    SWAP(sp->clientPriority[a], sp->clientPriority[b]);
    SWAP(sp->agi_rand[a], sp->agi_rand[b]);
    SWAP(sp->binding_turns[a], sp->binding_turns[b]);
    SWAP(sp->protectSuccessTurns[a], sp->protectSuccessTurns[b]);
    SWAP(sp->recycle_item[a], sp->recycle_item[b]);
    SWAP(sp->numberOfTurnsClientHasCurrentAbility[a], sp->numberOfTurnsClientHasCurrentAbility[b]);
    SWAP(sp->paradoxBoostedStat[a], sp->paradoxBoostedStat[b]);
    SWAP(sp->boosterEnergyActivated[a], sp->boosterEnergyActivated[b]);
    SWAP(sp->telekinesisTurns[a], sp->telekinesisTurns[b]);
    SWAP(sp->octolockedBy[a], sp->octolockedBy[b]);
    SWAP(sp->dragonCheerBoost[a], sp->dragonCheerBoost[b]);
    SWAP(sp->syrupBombTurns[a], sp->syrupBombTurns[b]);
    SWAP(sp->syrupBombSource[a], sp->syrupBombSource[b]);
    SWAP(sp->lastMoveTarget[a], sp->lastMoveTarget[b]);
    SWAP(sp->pledgeWaitingMove[a], sp->pledgeWaitingMove[b]);
    for (int i = 0; i < 8; i++) {
        SWAP(sp->opportunistBoosts[a][i], sp->opportunistBoosts[b][i]);
    }
    sp->electrified = SwapBitmaskBits(sp->electrified, a, b);
    sp->noRetreat = SwapBitmaskBits(sp->noRetreat, a, b);
    sp->tarShot = SwapBitmaskBits(sp->tarShot, a, b);
    sp->saltCure = SwapBitmaskBits(sp->saltCure, a, b);
    sp->beakBlastCharging = SwapBitmaskBits(sp->beakBlastCharging, a, b);
    sp->shellTrapSet = SwapBitmaskBits(sp->shellTrapSet, a, b);
    sp->shellTrapTriggered = SwapBitmaskBits(sp->shellTrapTriggered, a, b);

    // the turn order refers to positions, so swap them there too
    for (int i = 0; i < maxBattlers; i++) {
        if (sp->executionOrder[i] == a) {
            sp->executionOrder[i] = b;
        } else if (sp->executionOrder[i] == b) {
            sp->executionOrder[i] = a;
        }
        if (sp->turnOrder[i] == a) {
            sp->turnOrder[i] = b;
        } else if (sp->turnOrder[i] == b) {
            sp->turnOrder[i] = a;
        }
    }

    // the user carries on from its new position
    sp->attack_client = b;
    sp->defence_client = a;
    sp->battlerIdTemp = b;
    sp->mp.id = BATTLE_MSG_ALLY_SWITCH;
    sp->mp.tag = TAG_NICKNAME_NICKNAME;
    sp->mp.param[0] = CreateNicknameTag(sp, b);
    sp->mp.param[1] = CreateNicknameTag(sp, a);
    return TRUE;
}
