#include "../../include/types.h"
#include "../../include/battle.h"
#include "../../include/config.h"
#include "../../include/pokemon.h"
#include "../../include/sprite.h"
#include "../../include/constants/ability.h"
#include "../../include/constants/battle_constants.h"
#include "../../include/constants/battle_message_constants.h"
#include "../../include/constants/hold_item_effects.h"
#include "../../include/constants/move_effects.h"
#include "../../include/constants/moves.h"

#ifdef MOVE_EFFECTIVENESS_DISPLAY

/*
 * MOVE_EFFECTIVENESS_DISPLAY (see include/config.h): how well each move works against the opponents, shown while the
 * player picks a move.
 *
 * Drawing uses the battle menu's own text objects (pret pokeheartgold battle_input.c): BattleInput_CreateTextObject
 * makes a text sprite like the move names and PP. The fight menu already fills all 13 of the menu's slots, so ours
 * live in sEffText and are deleted together with the menu's (BattleInput_DeleteTextObjects_Extend, hooked over the
 * game's function at 0x02268684).
 * - Fight menu: EffDisplay_FightMenu, called at the end of BGCallback_Waza_Extend (src/battle/battle_input.c), which is
 *   the fight menu's "create objects" function.
 * - Target menu (doubles): BattleInput_CreateTargetMenuObjects_Extend replaces that pointer in the menu table
 *   (routinepointers). The move isn't in the target menu's data, so the fight menu's touch callback
 *   (BattleInput_TouchCallback_FightMenu_Extend, also via routinepointers) remembers the chosen one.
 *
 * The logic only reads the battle state (it never runs the game's real move checks, which start battle scripts).
 */

// a text sprite, laid out like the game's BattleInputTextObject
struct EffTextObj {
    void *textObj;
    u32 size;
    u32 offset;
    u16 vram;
    u16 isAtEnd;
    u16 fontLength;
};

#define BATTLE_INPUT_TEXTOBJ_OFFSET 0x4D0 // BattleInput.textObj[13], 20 bytes each
#define BATTLE_INPUT_TEXTOBJ_COUNT  13
#define BATTLE_INPUT_MENU_OFFSET    0x1C // BattleInput.menu (a union of the menus' data)
#define BATTLE_INPUT_PAL_ID         20023 // the text palette resource the menu's own text uses

#define EFF_TEXT_MAX 8

struct EffFightMenu {
    u16 moveNo[4];
    u8 pp[4];
    u8 ppMax[4];
    u8 battlerType;
};

struct EffTargetMenu {
    u8 targetMons[4][8];
    u8 battlerType;
    u8 selectionType;
};

enum {
    EFF_NONE = 0, // nothing to show
    EFF_IMMUNE_STATUS, // a status move that the target's typing or ability makes fail
    EFF_IMMUNE,
    EFF_X0125,
    EFF_X025,
    EFF_X05,
    EFF_X1,
    EFF_X2,
    EFF_X4,
    EFF_X8,
};

// what we know about a defender: its own types and ability, or its disguise's under Illusion
struct EffDefender {
    int id;
    u8 type1;
    u8 type2;
    u8 type3;
    u8 illusion;
    u16 ability;
};

struct EffTextObj sEffText[EFF_TEXT_MAX] __attribute__((section(".data"))) = { 0 };
u16 sEffChosenMove __attribute__((section(".data"))) = MOVE_NONE;

void LONG_CALL BattleInput_CreateTextObject(void *battleInput, struct EffTextObj *obj, void *str, u32 fontId, int textColor, int palOffset, int palId, int x, int y, int centerText, void *display);
void LONG_CALL BattleInput_CreateTargetMenuObjects(void *battleInput, int param1, int param2);
int LONG_CALL BattleInput_TouchCallback_FightMenu(void *battleInput, int touchInput, int param2);
void LONG_CALL FontOAM_Delete(void *textObj);
void LONG_CALL ObjCharTransfer_FreeBlock(void *alloc);
void LONG_CALL ov12_0223C1A0(void *bsys, u8 *buffer);
void LONG_CALL FontOAM_GetPosition(void *textObj, int *x, int *y);
void LONG_CALL FontOAM_SetPosition(void *textObj, int x, int y);
s32 Party_GetIllusionImitatedIndex(struct Party *party, s32 illusionIndex);

void EffDisplay_FightMenu(void *battleInput);

#define MAKE_TEXT_COLOR(fg, sh, bg) ((((fg) & 0xFF) << 16) | (((sh) & 0xFF) << 8) | (((bg) & 0xFF) << 0))

static void EffDisplay_DeleteText(void)
{
    for (int i = 0; i < EFF_TEXT_MAX; i++) {
        if (sEffText[i].textObj != NULL) {
            FontOAM_Delete(sEffText[i].textObj);
            ObjCharTransfer_FreeBlock(&sEffText[i].size);
            sEffText[i].textObj = NULL;
        }
    }
}

/**
 *  @brief the game's BattleInput_DeleteTextObjects (hooked at 0x02268684), plus ours
 *
 *  @param battleInput battle menu structure
 */
void LONG_CALL BattleInput_DeleteTextObjects_Extend(void *battleInput)
{
    struct EffTextObj *obj = (struct EffTextObj *)((u8 *)battleInput + BATTLE_INPUT_TEXTOBJ_OFFSET);

    for (int i = 0; i < BATTLE_INPUT_TEXTOBJ_COUNT; i++) {
        if (obj[i].textObj != NULL) {
            FontOAM_Delete(obj[i].textObj);
            ObjCharTransfer_FreeBlock(&obj[i].size);
            obj[i].textObj = NULL;
        }
    }
    EffDisplay_DeleteText();
}

// ---- the logic ------------------------------------------------------------------------------------------------------

static void EffDefender_Load(struct BattleSystem *bsys, struct BattleStruct *sp, int def, struct EffDefender *d)
{
    d->id = def;
    d->illusion = FALSE;
    d->ability = GetBattlerAbility(sp, def);
    if (sp->battlemon[def].is_currently_terastallized && GetSanitisedType(sp->battlemon[def].tera_type) != TYPE_STELLAR) {
        d->type1 = d->type2 = sp->battlemon[def].tera_type;
        d->type3 = TYPE_TYPELESS;
    } else {
        d->type1 = sp->battlemon[def].type1;
        d->type2 = sp->battlemon[def].type2;
        d->type3 = sp->battlemon[def].type3;
    }

    // under Illusion the player sees the disguise, so judge the move against the disguise's data
    if (IS_CLIENT_IN_ILLUSION(bsys, def)) {
        struct Party *party = BattleWorkPokePartyGet(bsys, def);
        struct PartyPokemon *disguise = Party_GetMonByIndex(party, Party_GetIllusionImitatedIndex(party, sp->sel_mons_no[def]));

        d->illusion = TRUE;
        d->type1 = GetMonData(disguise, MON_DATA_TYPE_1, NULL);
        d->type2 = GetMonData(disguise, MON_DATA_TYPE_2, NULL);
        d->type3 = TYPE_TYPELESS;
        d->ability = GetMonData(disguise, MON_DATA_ABILITY, NULL);
    }
}

static BOOL EffDefender_HasType(struct EffDefender *d, int type)
{
    return d->type1 == type || d->type2 == type || d->type3 == type;
}

// the defender has the ability and the attacker doesn't bypass it (Mold Breaker, Teravolt, Sunsteel Strike, ...)
static BOOL EffDefender_HasAbility(struct BattleStruct *sp, int attacker, struct EffDefender *d, u32 move, u32 ability)
{
    return MoldBreakerAbilityCheckInternal(attacker, d->id, GetBattlerAbility(sp, attacker), d->ability, move, sp->moveTbl[move].split, ability);
}

// the game's type chart for this move (Tera, Ring Target, Scrappy, Foresight, strong winds, Freeze-Dry, Tar Shot, ...)
static int EffDisplay_TypeChart(struct BattleSystem *bsys, struct BattleStruct *sp, int attacker, struct EffDefender *d, u32 move, u32 moveType)
{
    u8 type1 = sp->battlemon[d->id].type1, type2 = sp->battlemon[d->id].type2, type3 = sp->battlemon[d->id].type3;
    u32 tera = sp->battlemon[d->id].is_currently_terastallized;
    u32 savedMove = sp->current_move_index;
    u32 flag = 0;
    int eff;

    // GetTypeEffectiveness reads the defender's types from the battle state: give it the disguise's for a moment
    if (d->illusion) {
        sp->battlemon[d->id].type1 = d->type1;
        sp->battlemon[d->id].type2 = d->type2;
        sp->battlemon[d->id].type3 = TYPE_TYPELESS;
        sp->battlemon[d->id].is_currently_terastallized = 0;
    }
    sp->current_move_index = move;
    eff = GetTypeEffectiveness(bsys, sp, attacker, d->id, moveType, &flag);
    sp->current_move_index = savedMove;
    if (d->illusion) {
        sp->battlemon[d->id].type1 = type1;
        sp->battlemon[d->id].type2 = type2;
        sp->battlemon[d->id].type3 = type3;
        sp->battlemon[d->id].is_currently_terastallized = tera;
    }
    return eff;
}

// Levitate, Air Balloon, Magnet Rise and Telekinesis, unless something grounds the defender
static BOOL EffDisplay_FloatsOverGround(struct BattleStruct *sp, int attacker, struct EffDefender *d, u32 move)
{
    int def = d->id;
    int hold = HeldItemHoldEffectGet(sp, def);

    if (move == MOVE_THOUSAND_ARROWS
        || hold == HOLD_EFFECT_SPEED_DOWN_GROUNDED
        || (sp->battlemon[def].effect_of_moves & MOVE_EFFECT_FLAG_INGRAIN)
        || (sp->field_condition & FIELD_CONDITION_GRAVITY)
        || sp->moveConditionsFlags[def].grounded) {
        return FALSE;
    }
    return EffDefender_HasAbility(sp, attacker, d, move, ABILITY_LEVITATE)
        || EffDefender_HasAbility(sp, attacker, d, move, ABILITY_EELEVATE)
        || hold == HOLD_EFFECT_UNGROUND_DESTROYED_ON_HIT
        || sp->battlemon[def].moveeffect.magnetRiseTurns
        || sp->telekinesisTurns[def];
}

// Dazzling, Queenly Majesty and Armor Tail stop moves with raised priority
static BOOL EffDisplay_PriorityBlocked(struct BattleStruct *sp, int attacker, struct EffDefender *d, u32 move, u32 moveType)
{
    int priority = sp->moveTbl[move].priority;
    u32 ability = GetBattlerAbility(sp, attacker);

    if (ability == ABILITY_PRANKSTER && sp->moveTbl[move].split == SPLIT_STATUS) {
        priority++;
    }
    if (ability == ABILITY_GALE_WINGS && moveType == TYPE_FLYING && (u32)sp->battlemon[attacker].hp == sp->battlemon[attacker].maxhp) {
        priority++;
    }
    if (priority <= 0) {
        return FALSE;
    }
    return EffDefender_HasAbility(sp, attacker, d, move, ABILITY_DAZZLING)
        || EffDefender_HasAbility(sp, attacker, d, move, ABILITY_QUEENLY_MAJESTY)
        || EffDefender_HasAbility(sp, attacker, d, move, ABILITY_ARMOR_TAIL);
}

// abilities and moves that make a move fail whatever its category
static BOOL EffDisplay_AbilityBlocks(struct BattleStruct *sp, int attacker, struct EffDefender *d, u32 move, u32 moveType)
{
    if (IsMoveSoundBased(move) && EffDefender_HasAbility(sp, attacker, d, move, ABILITY_SOUNDPROOF)) {
        return TRUE;
    }
    if (IsBallOrBombMove(move) && EffDefender_HasAbility(sp, attacker, d, move, ABILITY_BULLETPROOF)) {
        return TRUE;
    }
    if (IsMoveWindMove(move) && EffDefender_HasAbility(sp, attacker, d, move, ABILITY_WIND_RIDER)) {
        return TRUE;
    }
    if (IsPowderMove(move)
        && (EffDefender_HasType(d, TYPE_GRASS) || EffDefender_HasAbility(sp, attacker, d, move, ABILITY_OVERCOAT)
            || HeldItemHoldEffectGet(sp, d->id) == HOLD_EFFECT_SPORE_POWDER_IMMUNITY)) {
        return TRUE;
    }
    return EffDisplay_PriorityBlocked(sp, attacker, d, move, moveType);
}

static int EffDisplay_Damaging(struct BattleSystem *bsys, struct BattleStruct *sp, int attacker, struct EffDefender *d, u32 move)
{
    u32 moveType = GetAdjustedMoveType(sp, attacker, move);
    u16 effect = sp->moveTbl[move].effect;
    int eff = EffDisplay_TypeChart(bsys, sp, attacker, d, move, moveType);

    if (eff == TYPE_MUL_NO_EFFECT || EffDisplay_AbilityBlocks(sp, attacker, d, move, moveType)) {
        return EFF_IMMUNE;
    }

    switch (moveType) {
    case TYPE_FIRE:
        if (EffDefender_HasAbility(sp, attacker, d, move, ABILITY_FLASH_FIRE) || EffDefender_HasAbility(sp, attacker, d, move, ABILITY_WELL_BAKED_BODY)) {
            return EFF_IMMUNE;
        }
        break;
    case TYPE_WATER:
        if (EffDefender_HasAbility(sp, attacker, d, move, ABILITY_WATER_ABSORB) || EffDefender_HasAbility(sp, attacker, d, move, ABILITY_STORM_DRAIN)
            || EffDefender_HasAbility(sp, attacker, d, move, ABILITY_DRY_SKIN)) {
            return EFF_IMMUNE;
        }
        break;
    case TYPE_ELECTRIC:
        if (EffDefender_HasAbility(sp, attacker, d, move, ABILITY_VOLT_ABSORB) || EffDefender_HasAbility(sp, attacker, d, move, ABILITY_LIGHTNING_ROD)
            || EffDefender_HasAbility(sp, attacker, d, move, ABILITY_MOTOR_DRIVE)) {
            return EFF_IMMUNE;
        }
        break;
    case TYPE_GRASS:
        if (EffDefender_HasAbility(sp, attacker, d, move, ABILITY_SAP_SIPPER)) {
            return EFF_IMMUNE;
        }
        break;
    case TYPE_GROUND:
        if (EffDefender_HasAbility(sp, attacker, d, move, ABILITY_EARTH_EATER) || EffDisplay_FloatsOverGround(sp, attacker, d, move)) {
            return EFF_IMMUNE;
        }
        break;
    }

    if (move != MOVE_STRUGGLE && eff < TYPE_MUL_SUPER_EFFECTIVE && EffDefender_HasAbility(sp, attacker, d, move, ABILITY_WONDER_GUARD)) {
        return EFF_IMMUNE;
    }

    switch (effect) {
    case MOVE_EFFECT_ONE_HIT_KO:
        if (EffDefender_HasAbility(sp, attacker, d, move, ABILITY_STURDY) || (move == MOVE_SHEER_COLD && EffDefender_HasType(d, TYPE_ICE))) {
            return EFF_IMMUNE;
        }
        return EFF_X1;
    // fixed damage: the type chart only decides whether it hits at all
    case MOVE_EFFECT_LEVEL_DAMAGE_FLAT:
    case MOVE_EFFECT_HALVE_HP:
    case MOVE_EFFECT_10_DAMAGE_FLAT:
    case MOVE_EFFECT_40_DAMAGE_FLAT:
    case MOVE_EFFECT_RANDOM_DAMAGE_1_TO_150_LEVEL:
    case MOVE_EFFECT_SET_HP_EQUAL_TO_USER:
    case MOVE_EFFECT_FINAL_GAMBIT:
    case MOVE_EFFECT_COUNTER:
    case MOVE_EFFECT_MIRROR_COAT:
    case MOVE_EFFECT_METAL_BURST:
        return EFF_X1;
    }

    switch (eff) {
    case TYPE_MUL_TRIPLE_SUPER_EFFECTIVE:
        return EFF_X8;
    case TYPE_MUL_DOUBLE_SUPER_EFFECTIVE:
        return EFF_X4;
    case TYPE_MUL_SUPER_EFFECTIVE:
        return EFF_X2;
    case TYPE_MUL_NOT_EFFECTIVE:
        return EFF_X05;
    case TYPE_MUL_DOUBLE_NOT_EFFECTIVE:
        return EFF_X025;
    case TYPE_MUL_TRIPLE_NOT_EFFECTIVE:
        return EFF_X0125;
    default:
        return EFF_X1;
    }
}

static BOOL EffDisplay_StatDropBlocked(struct BattleStruct *sp, int attacker, struct EffDefender *d, u32 move, u32 stat)
{
    if (EffDefender_HasAbility(sp, attacker, d, move, ABILITY_CLEAR_BODY) || EffDefender_HasAbility(sp, attacker, d, move, ABILITY_WHITE_SMOKE)
        || EffDefender_HasAbility(sp, attacker, d, move, ABILITY_FULL_METAL_BODY)
        || (EffDefender_HasType(d, TYPE_GRASS) && EffDefender_HasAbility(sp, attacker, d, move, ABILITY_FLOWER_VEIL))
        || HeldItemHoldEffectGet(sp, d->id) == HOLD_EFFECT_PREVENT_STAT_DROPS) {
        return TRUE;
    }
    switch (stat) {
    case STAT_ATTACK:
        return EffDefender_HasAbility(sp, attacker, d, move, ABILITY_HYPER_CUTTER);
    case STAT_DEFENSE:
        return EffDefender_HasAbility(sp, attacker, d, move, ABILITY_BIG_PECKS);
    case STAT_ACCURACY:
        return EffDefender_HasAbility(sp, attacker, d, move, ABILITY_KEEN_EYE) || EffDefender_HasAbility(sp, attacker, d, move, ABILITY_ILLUMINATE)
            || EffDefender_HasAbility(sp, attacker, d, move, ABILITY_MINDS_EYE);
    }
    return FALSE;
}

static BOOL EffDisplay_SleepBlocked(struct BattleStruct *sp, int attacker, struct EffDefender *d, u32 move)
{
    return EffDefender_HasAbility(sp, attacker, d, move, ABILITY_INSOMNIA) || EffDefender_HasAbility(sp, attacker, d, move, ABILITY_VITAL_SPIRIT)
        || EffDefender_HasAbility(sp, attacker, d, move, ABILITY_SWEET_VEIL);
}

// a status condition: Comatose, Purifying Salt, and Leaf Guard in harsh sunlight stop them all
static BOOL EffDisplay_StatusConditionBlocked(struct BattleStruct *sp, int attacker, struct EffDefender *d, u32 move)
{
    return EffDefender_HasAbility(sp, attacker, d, move, ABILITY_COMATOSE) || EffDefender_HasAbility(sp, attacker, d, move, ABILITY_PURIFYING_SALT)
        || ((sp->field_condition & FIELD_CONDITION_SUN_ALL) && EffDefender_HasAbility(sp, attacker, d, move, ABILITY_LEAF_GUARD));
}

static int EffDisplay_Status(struct BattleSystem *bsys, struct BattleStruct *sp, int attacker, struct EffDefender *d, u32 move)
{
    u32 moveType = GetAdjustedMoveType(sp, attacker, move);
    BOOL blocked = FALSE;

    if (EffDisplay_AbilityBlocks(sp, attacker, d, move, moveType)
        || EffDefender_HasAbility(sp, attacker, d, move, ABILITY_GOOD_AS_GOLD)
        || ((sp->moveTbl[move].flag & FLAG_MAGIC_COAT) && EffDefender_HasAbility(sp, attacker, d, move, ABILITY_MAGIC_BOUNCE))
        || (GetBattlerAbility(sp, attacker) == ABILITY_PRANKSTER && EffDefender_HasType(d, TYPE_DARK))) {
        return EFF_IMMUNE_STATUS;
    }

    switch (sp->moveTbl[move].effect) {
    case MOVE_EFFECT_STATUS_SLEEP:
    case MOVE_EFFECT_STATUS_SLEEP_NEXT_TURN:
        blocked = EffDisplay_SleepBlocked(sp, attacker, d, move) || EffDisplay_StatusConditionBlocked(sp, attacker, d, move);
        break;
    case MOVE_EFFECT_STATUS_PARALYZE:
        blocked = EffDefender_HasType(d, TYPE_ELECTRIC) || EffDefender_HasAbility(sp, attacker, d, move, ABILITY_LIMBER)
            || EffDisplay_StatusConditionBlocked(sp, attacker, d, move)
            // Thunder Wave also follows the type chart (Ground types)
            || (move == MOVE_THUNDER_WAVE && EffDisplay_TypeChart(bsys, sp, attacker, d, move, moveType) == TYPE_MUL_NO_EFFECT);
        break;
    case MOVE_EFFECT_STATUS_POISON:
    case MOVE_EFFECT_STATUS_BADLY_POISON:
        blocked = ((EffDefender_HasType(d, TYPE_POISON) || EffDefender_HasType(d, TYPE_STEEL)) && GetBattlerAbility(sp, attacker) != ABILITY_CORROSION)
            || EffDefender_HasAbility(sp, attacker, d, move, ABILITY_IMMUNITY) || EffDefender_HasAbility(sp, attacker, d, move, ABILITY_PASTEL_VEIL)
            || EffDisplay_StatusConditionBlocked(sp, attacker, d, move);
        break;
    case MOVE_EFFECT_STATUS_BURN:
        blocked = EffDefender_HasType(d, TYPE_FIRE) || EffDefender_HasAbility(sp, attacker, d, move, ABILITY_WATER_VEIL)
            || EffDefender_HasAbility(sp, attacker, d, move, ABILITY_WATER_BUBBLE) || EffDefender_HasAbility(sp, attacker, d, move, ABILITY_THERMAL_EXCHANGE)
            || EffDisplay_StatusConditionBlocked(sp, attacker, d, move);
        break;
    case MOVE_EFFECT_STATUS_CONFUSE:
    case MOVE_EFFECT_CONFUSE_ALL_ADJACENT:
    case MOVE_EFFECT_ATK_UP_2_STATUS_CONFUSION:
    case MOVE_EFFECT_SP_ATK_UP_CAUSE_CONFUSION:
        blocked = EffDefender_HasAbility(sp, attacker, d, move, ABILITY_OWN_TEMPO);
        break;
    case MOVE_EFFECT_STATUS_LEECH_SEED:
        blocked = EffDefender_HasType(d, TYPE_GRASS);
        break;
    case MOVE_EFFECT_INFATUATE:
    case MOVE_EFFECT_TAUNT:
        blocked = EffDefender_HasAbility(sp, attacker, d, move, ABILITY_OBLIVIOUS) || EffDefender_HasAbility(sp, attacker, d, move, ABILITY_AROMA_VEIL);
        break;
    case MOVE_EFFECT_ENCORE:
    case MOVE_EFFECT_TORMENT:
    case MOVE_EFFECT_DISABLE:
    case MOVE_EFFECT_PREVENT_HEALING:
        blocked = EffDefender_HasAbility(sp, attacker, d, move, ABILITY_AROMA_VEIL);
        break;
    case MOVE_EFFECT_PREVENT_ESCAPE:
        blocked = EffDefender_HasType(d, TYPE_GHOST);
        break;
    case MOVE_EFFECT_ATK_DOWN:
    case MOVE_EFFECT_ATK_DOWN_2:
    case MOVE_EFFECT_ATK_DOWN_3:
        blocked = EffDisplay_StatDropBlocked(sp, attacker, d, move, STAT_ATTACK);
        break;
    case MOVE_EFFECT_DEF_DOWN:
    case MOVE_EFFECT_DEF_DOWN_2:
    case MOVE_EFFECT_DEF_DOWN_3:
        blocked = EffDisplay_StatDropBlocked(sp, attacker, d, move, STAT_DEFENSE);
        break;
    case MOVE_EFFECT_ACC_DOWN:
    case MOVE_EFFECT_ACC_DOWN_2:
    case MOVE_EFFECT_ACC_DOWN_3:
        blocked = EffDisplay_StatDropBlocked(sp, attacker, d, move, STAT_ACCURACY);
        break;
    case MOVE_EFFECT_SPEED_DOWN:
    case MOVE_EFFECT_SPEED_DOWN_2:
    case MOVE_EFFECT_SPEED_DOWN_3:
    case MOVE_EFFECT_SP_ATK_DOWN:
    case MOVE_EFFECT_SP_ATK_DOWN_2:
    case MOVE_EFFECT_SP_ATK_DOWN_3:
    case MOVE_EFFECT_SP_DEF_DOWN:
    case MOVE_EFFECT_SP_DEF_DOWN_2:
    case MOVE_EFFECT_SP_DEF_DOWN_3:
    case MOVE_EFFECT_EVA_DOWN:
    case MOVE_EFFECT_EVA_DOWN_2:
    case MOVE_EFFECT_EVA_DOWN_3:
    case MOVE_EFFECT_ATK_DEF_DOWN:
    case MOVE_EFFECT_ATK_SP_ATK_DOWN:
        blocked = EffDisplay_StatDropBlocked(sp, attacker, d, move, STAT_SPEED);
        break;
    }
    return blocked ? EFF_IMMUNE_STATUS : EFF_NONE;
}

// moves aimed at the opponents (not the user, its side, an ally or the field)
static BOOL EffDisplay_TargetsOpponents(u32 range)
{
    return range == RANGE_SINGLE_TARGET || (range & (RANGE_SINGLE_TARGET_SPECIAL | RANGE_RANDOM_OPPONENT | RANGE_ADJACENT_OPPONENTS | RANGE_ALL_ADJACENT));
}

/**
 *  @brief how well a move works against one defender
 *
 *  @return EFF_* value
 */
static int EffDisplay_Evaluate(struct BattleSystem *bsys, int attacker, int defender, u32 move)
{
    struct BattleStruct *sp = bsys->sp;
    struct EffDefender d;

    if (move == MOVE_NONE || defender >= BattleWorkClientSetMaxGet(bsys) || sp->battlemon[defender].hp == 0
        || !EffDisplay_TargetsOpponents(sp->moveTbl[move].target)) {
        return EFF_NONE;
    }
    EffDefender_Load(bsys, sp, defender, &d);
    if (sp->moveTbl[move].split == SPLIT_STATUS) {
        return EffDisplay_Status(bsys, sp, attacker, &d, move);
    }
    return EffDisplay_Damaging(bsys, sp, attacker, &d, move);
}

// ---- drawing --------------------------------------------------------------------------------------------------------

static const u16 sEffFullMsg[] = {
    [EFF_IMMUNE_STATUS] = BATTLE_MSG_EFF_IMMUNE,
    [EFF_IMMUNE] = BATTLE_MSG_EFF_IMMUNE_X0,
    [EFF_X0125] = BATTLE_MSG_EFF_NOT_VERY_X0125,
    [EFF_X025] = BATTLE_MSG_EFF_NOT_VERY_X025,
    [EFF_X05] = BATTLE_MSG_EFF_NOT_VERY_X05,
    [EFF_X1] = BATTLE_MSG_EFF_NORMAL_X1,
    [EFF_X2] = BATTLE_MSG_EFF_SUPER_X2,
    [EFF_X4] = BATTLE_MSG_EFF_SUPER_X4,
    [EFF_X8] = BATTLE_MSG_EFF_SUPER_X8,
};

static const u16 sEffShortMsg[] = {
    [EFF_IMMUNE_STATUS] = BATTLE_MSG_EFF_IMMUNE,
    [EFF_IMMUNE] = BATTLE_MSG_EFF_IMMUNE,
    [EFF_X0125] = BATTLE_MSG_EFF_SHORT_X0125,
    [EFF_X025] = BATTLE_MSG_EFF_SHORT_X025,
    [EFF_X05] = BATTLE_MSG_EFF_SHORT_X05,
    [EFF_X1] = BATTLE_MSG_EFF_SHORT_X1,
    [EFF_X2] = BATTLE_MSG_EFF_SHORT_X2,
    [EFF_X4] = BATTLE_MSG_EFF_SHORT_X4,
    [EFF_X8] = BATTLE_MSG_EFF_SHORT_X8,
};

// colours. Move buttons: palOffset 4, the PP text's palette (sub screen OBJ palette 7), dark text with a lighter shadow
// like the PP numbers: dark green for super effective, grey for neutral, dark red for resisted, near-black (the PP
// numbers' colour, the palette's darkest) for immune. Target screen (orange buttons): palOffset 6, the opponents' names' white text with a dark shadow.
static void EffDisplay_Color(int eff, BOOL onTargetScreen, int *color, int *palOffset)
{
    if (onTargetScreen) {
        *palOffset = 6;
        *color = MAKE_TEXT_COLOR(1, 2, 0);
        return;
    }
    *palOffset = 4;
    if (eff >= EFF_X2) {
        *color = MAKE_TEXT_COLOR(13, 14, 0);
    } else if (eff == EFF_X1) {
        *color = MAKE_TEXT_COLOR(2, 11, 0);
    } else if (eff >= EFF_X0125) {
        *color = MAKE_TEXT_COLOR(8, 2, 0);
    } else {
        *color = MAKE_TEXT_COLOR(1, 2, 0);
    }
}

static void EffDisplay_Print(void *battleInput, int slot, int eff, BOOL full, BOOL onTargetScreen, int x, int y)
{
    struct BattleSystem *bsys = *(struct BattleSystem **)battleInput;
    int color, palOffset;
    void *str;

    if (eff == EFF_NONE || slot >= EFF_TEXT_MAX || sEffText[slot].textObj != NULL) {
        return;
    }
    EffDisplay_Color(eff, onTargetScreen, &color, &palOffset);
    str = NewString_ReadMsgData(BattleWorkFightMsgGet(bsys), full ? sEffFullMsg[eff] : sEffShortMsg[eff]);
    BattleInput_CreateTextObject(battleInput, &sEffText[slot], str, 0, color, palOffset, BATTLE_INPUT_PAL_ID, x, y, TRUE, NULL);
    String_Delete(str);
}

// moves one of the menu's own text objects (a move or Pokémon name) up or down
static void EffDisplay_ShiftMenuText(void *battleInput, int index, int dy)
{
    struct EffTextObj *obj = (struct EffTextObj *)((u8 *)battleInput + BATTLE_INPUT_TEXTOBJ_OFFSET) + index;
    int x, y;

    if (obj->textObj != NULL) {
        FontOAM_GetPosition(obj->textObj, &x, &y);
        FontOAM_SetPosition(obj->textObj, x, y + dy);
    }
}

// the opponents of a battler: in singles only the one across, in doubles both (left one first, as on screen)
static void EffDisplay_Opponents(struct BattleSystem *bsys, int attacker, int *left, int *right)
{
    int first = (attacker & 1) ? 0 : 1;

    if (BattleWorkClientSetMaxGet(bsys) <= 2) {
        *left = -1;
        *right = first;
    } else {
        *left = first + 2;
        *right = first;
    }
}

// Move buttons: the white part is one tile row (8 px) taller than in the game (scripts/build_effectiveness_buttons.py;
// the type tag and PP move down with the tan strip, see bytereplacement). It holds the move name and under it the
// effectiveness line (every button, so they all look the same).
// The game's positions are uneven: the buttons are exactly 64 px apart and level left/right, but the right column's
// text is 1 px higher than the left's and the bottom row's 63 px below the top row's, so the bottom right button's
// contents sat 2 px higher than the top left's. Everything here (and the tag/PP tables in bytereplacement) uses the top
// left button's spacing for all four: the names move to EFF_MOVE_NAME_Y.
#define EFF_DOUBLES_HALF_WIDTH 26

static const s16 sEffGameMoveNameY[4] = { 45, 44, 108, 107 }; // where the game puts the move names
static const s16 sEffMoveNameY[4] = { 38, 38, 102, 102 };
static const s16 sEffMoveButtonPos[4][2] = { // centre x and the effectiveness line's y
    { 64, 52 },
    { 192, 52 },
    { 64, 116 },
    { 192, 116 },
};

/**
 *  @brief adds the effectiveness lines to the move buttons; called at the end of BGCallback_Waza_Extend
 *
 *  @param battleInput battle menu structure
 */
void EffDisplay_FightMenu(void *battleInput)
{
    struct BattleSystem *bsys = *(struct BattleSystem **)battleInput;
    struct EffFightMenu *menu = (struct EffFightMenu *)((u8 *)battleInput + BATTLE_INPUT_MENU_OFFSET);
    int attacker = BattleWorkClientNoGet(bsys, menu->battlerType);
    int left, right;

    EffDisplay_DeleteText();
    EffDisplay_Opponents(bsys, attacker, &left, &right);
    for (int i = 0; i < 4; i++) {
        int x = sEffMoveButtonPos[i][0], y = sEffMoveButtonPos[i][1];

        EffDisplay_ShiftMenuText(battleInput, i, sEffMoveNameY[i] - sEffGameMoveNameY[i]); // MENUTXT_MOVE_1 + i
        if (left < 0) {
            EffDisplay_Print(battleInput, i, EffDisplay_Evaluate(bsys, attacker, right, menu->moveNo[i]), TRUE, FALSE, x, y);
        } else {
            // doubles: one short value per opponent, on the side where it stands
            EffDisplay_Print(battleInput, i * 2, EffDisplay_Evaluate(bsys, attacker, left, menu->moveNo[i]), FALSE, FALSE, x - EFF_DOUBLES_HALF_WIDTH, y);
            EffDisplay_Print(battleInput, i * 2 + 1, EffDisplay_Evaluate(bsys, attacker, right, menu->moveNo[i]), FALSE, FALSE, x + EFF_DOUBLES_HALF_WIDTH, y);
        }
    }
}

/**
 *  @brief the fight menu's touch callback (also used for button presses), remembering the chosen move for the
 *         target menu
 */
int LONG_CALL BattleInput_TouchCallback_FightMenu_Extend(void *battleInput, int touchInput, int param2)
{
    struct EffFightMenu *menu = (struct EffFightMenu *)((u8 *)battleInput + BATTLE_INPUT_MENU_OFFSET);
    int ret = BattleInput_TouchCallback_FightMenu(battleInput, touchInput, param2);

    if (ret >= 1 && ret <= 4) {
        sEffChosenMove = menu->moveNo[ret - 1];
    }
    return ret;
}

// Target menu: the opponents' buttons are slots 1 and 3 (MENUTXT_MON_NAME_1 + slot). Their orange top band has room
// above the name, so the name moves up by EFF_TARGET_NAME_SHIFT and the line goes under it, clear of the Pokémon icon.
#define EFF_TARGET_NAME_SHIFT (-9)

static const s16 sEffTargetLinePos[4][2] = {
    { 60, 122 },
    { 196, 38 },
    { 196, 122 },
    { 60, 38 },
};

/**
 *  @brief the game's target menu objects, plus the chosen move's effectiveness under each opponent's name
 */
void LONG_CALL BattleInput_CreateTargetMenuObjects_Extend(void *battleInput, int param1, int param2)
{
    struct BattleSystem *bsys = *(struct BattleSystem **)battleInput;
    struct EffTargetMenu *menu = (struct EffTargetMenu *)((u8 *)battleInput + BATTLE_INPUT_MENU_OFFSET);
    u8 battlerAt[6];
    int attacker;

    BattleInput_CreateTargetMenuObjects(battleInput, param1, param2);

    EffDisplay_DeleteText();
    attacker = BattleWorkClientNoGet(bsys, menu->battlerType);
    ov12_0223C1A0(bsys, battlerAt); // battlerAt[battler type] = battler id, as the game does for this menu
    int eff[4] = { EFF_NONE, EFF_NONE, EFF_NONE, EFF_NONE };

    for (int i = 1; i < 4; i += 2) {
        eff[i] = EffDisplay_Evaluate(bsys, attacker, battlerAt[2 + i], sEffChosenMove);
    }
    if (eff[1] == EFF_NONE && eff[3] == EFF_NONE) {
        return;
    }
    // both names move, so they stay level even when only one opponent has a line
    for (int i = 1; i < 4; i += 2) {
        EffDisplay_ShiftMenuText(battleInput, i, EFF_TARGET_NAME_SHIFT);
        EffDisplay_Print(battleInput, i, eff[i], TRUE, TRUE, sEffTargetLinePos[i][0], sEffTargetLinePos[i][1]);
    }
}

#endif // MOVE_EFFECTIVENESS_DISPLAY
