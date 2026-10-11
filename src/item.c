#include "../include/item.h"

#include "../include/config.h"
#include "../include/constants/file.h"
#include "../include/constants/item.h"
#include "../include/constants/moves.h"
#include "../include/message.h"
#include "../include/script.h"
#include "../include/types.h"

#define GFX_ITEM_DUMMY_ID  ((MAX_TOTAL_ITEM_NUM) * 2 + 2)
#define GFX_ITEM_RETURN_ID ((MAX_TOTAL_ITEM_NUM + 1) * 2 + 4)
#define NEW_ITEM_GFX       (797)

static const u16 sMachineMoves[] = {
    // vanilla TMs
    MOVE_FOCUS_PUNCH, // TM001
    MOVE_DRAGON_CLAW, // TM002
    MOVE_WATER_PULSE, // TM003
    MOVE_CALM_MIND, // TM004
    MOVE_ROAR, // TM005
    MOVE_TOXIC, // TM006
    MOVE_HAIL, // TM007
    MOVE_BULK_UP, // TM008
    MOVE_BULLET_SEED, // TM009
    MOVE_HIDDEN_POWER, // TM010
    MOVE_SUNNY_DAY, // TM011
    MOVE_TAUNT, // TM012
    MOVE_ICE_BEAM, // TM013
    MOVE_BLIZZARD, // TM014
    MOVE_HYPER_BEAM, // TM015
    MOVE_LIGHT_SCREEN, // TM016
    MOVE_PROTECT, // TM017
    MOVE_RAIN_DANCE, // TM018
    MOVE_GIGA_DRAIN, // TM019
    MOVE_SAFEGUARD, // TM020
    MOVE_FRUSTRATION, // TM021
    MOVE_SOLAR_BEAM, // TM022
    MOVE_IRON_TAIL, // TM023
    MOVE_THUNDERBOLT, // TM024
    MOVE_THUNDER, // TM025
    MOVE_EARTHQUAKE, // TM026
    MOVE_RETURN, // TM027
    MOVE_DIG, // TM028
    MOVE_PSYCHIC, // TM029
    MOVE_SHADOW_BALL, // TM030
    MOVE_BRICK_BREAK, // TM031
    MOVE_DOUBLE_TEAM, // TM032
    MOVE_REFLECT, // TM033
    MOVE_SHOCK_WAVE, // TM034
    MOVE_FLAMETHROWER, // TM035
    MOVE_SLUDGE_BOMB, // TM036
    MOVE_SANDSTORM, // TM037
    MOVE_FIRE_BLAST, // TM038
    MOVE_ROCK_TOMB, // TM039
    MOVE_AERIAL_ACE, // TM040
    MOVE_TORMENT, // TM041
    MOVE_FACADE, // TM042
    MOVE_SECRET_POWER, // TM043
    MOVE_REST, // TM044
    MOVE_ATTRACT, // TM045
    MOVE_THIEF, // TM046
    MOVE_STEEL_WING, // TM047
    MOVE_SKILL_SWAP, // TM048
    MOVE_SNATCH, // TM049
    MOVE_OVERHEAT, // TM050
    MOVE_ROOST, // TM051
    MOVE_FOCUS_BLAST, // TM052
    MOVE_ENERGY_BALL, // TM053
    MOVE_FALSE_SWIPE, // TM054
    MOVE_BRINE, // TM055
    MOVE_FLING, // TM056
    MOVE_CHARGE_BEAM, // TM057
    MOVE_ENDURE, // TM058
    MOVE_DRAGON_PULSE, // TM059
    MOVE_DRAIN_PUNCH, // TM060
    MOVE_WILL_O_WISP, // TM061
    MOVE_SILVER_WIND, // TM062
    MOVE_EMBARGO, // TM063
    MOVE_EXPLOSION, // TM064
    MOVE_SHADOW_CLAW, // TM065
    MOVE_PAYBACK, // TM066
    MOVE_RECYCLE, // TM067
    MOVE_GIGA_IMPACT, // TM068
    MOVE_ROCK_POLISH, // TM069
    MOVE_FLASH, // TM070
    MOVE_STONE_EDGE, // TM071
    MOVE_AVALANCHE, // TM072
    MOVE_THUNDER_WAVE, // TM073
    MOVE_GYRO_BALL, // TM074
    MOVE_SWORDS_DANCE, // TM075
    MOVE_STEALTH_ROCK, // TM076
    MOVE_PSYCH_UP, // TM077
    MOVE_CAPTIVATE, // TM078
    MOVE_DARK_PULSE, // TM079
    MOVE_ROCK_SLIDE, // TM080
    MOVE_X_SCISSOR, // TM081
    MOVE_SLEEP_TALK, // TM082
    MOVE_NATURAL_GIFT, // TM083
    MOVE_POISON_JAB, // TM084
    MOVE_DREAM_EATER, // TM085
    MOVE_GRASS_KNOT, // TM086
    MOVE_SWAGGER, // TM087
    MOVE_PLUCK, // TM088
    MOVE_U_TURN, // TM089
    MOVE_SUBSTITUTE, // TM090
    MOVE_FLASH_CANNON, // TM091
    MOVE_TRICK_ROOM, // TM092

    // vanilla HMs
    MOVE_CUT, // HM01
    MOVE_FLY, // HM02
    MOVE_SURF, // HM03
    MOVE_STRENGTH, // HM04
    MOVE_WHIRLPOOL, // HM05
    MOVE_ROCK_SMASH, // HM06
    MOVE_WATERFALL, // HM07
    MOVE_ROCK_CLIMB, // HM08

    // HM07 (ORAS)
    MOVE_DIVE, // HM07_ORAS

    // expansion TMs
    MOVE_MEGA_PUNCH, // TM00
    MOVE_NONE, // TM093: blank (repeat of TM091)
    MOVE_NONE, // TM094: blank (repeat of TM079)
    MOVE_LEECH_LIFE, // TM095
    MOVE_EERIE_IMPULSE, // TM096
    MOVE_NONE, // TM097: blank (repeat of HM02)
    MOVE_NONE, // TM098: blank (repeat of TM048)
    MOVE_IRON_HEAD, // TM099
    MOVE_CONFIDE, // TM100
    MOVE_DRAGON_DANCE, // TM100_SV
    MOVE_POWER_GEM, // TM101
    MOVE_GUNK_SHOT, // TM102
    MOVE_NONE, // TM103: blank (repeat of TM090)
    MOVE_IRON_DEFENSE, // TM104
    MOVE_NONE, // TM105: blank (repeat of TM081)
    MOVE_DRILL_RUN, // TM106
    MOVE_NONE, // TM107: blank (repeat of TM061)
    MOVE_CRUNCH, // TM108
    MOVE_TRICK, // TM109
    MOVE_LIQUIDATION, // TM110
    MOVE_NONE, // TM111: blank (repeat of TM019)
    MOVE_AURA_SPHERE, // TM112
    MOVE_TAILWIND, // TM113
    MOVE_NONE, // TM114: blank (repeat of TM030)
    MOVE_NONE, // TM115: blank (repeat of TM059)
    MOVE_NONE, // TM116: blank (repeat of TM076)
    MOVE_HYPER_VOICE, // TM117
    MOVE_HEAT_WAVE, // TM118
    MOVE_NONE, // TM119: blank (repeat of TM053)
    MOVE_NONE, // TM120: blank (repeat of TM029)
    MOVE_HEAVY_SLAM, // TM121
    MOVE_ENCORE, // TM122
    MOVE_NONE, // TM123: blank (repeat of HM03)
    MOVE_ICE_SPINNER, // TM124
    MOVE_NONE, // TM125: blank (repeat of TM035)
    MOVE_NONE, // TM126: blank (repeat of TM024)
    MOVE_PLAY_ROUGH, // TM127
    MOVE_AMNESIA, // TM128
    MOVE_NONE, // TM129: blank (repeat of TM004)
    MOVE_HELPING_HAND, // TM130
    MOVE_POLLEN_PUFF, // TM131
    MOVE_BATON_PASS, // TM132
    MOVE_EARTH_POWER, // TM133
    MOVE_REVERSAL, // TM134
    MOVE_NONE, // TM135: blank (repeat of TM013)
    MOVE_ELECTRIC_TERRAIN, // TM136
    MOVE_GRASSY_TERRAIN, // TM137
    MOVE_PSYCHIC_TERRAIN, // TM138
    MOVE_MISTY_TERRAIN, // TM139
    MOVE_NASTY_PLOT, // TM140
    MOVE_NONE, // TM141: blank (repeat of TM038)
    MOVE_HYDRO_PUMP, // TM142
    MOVE_NONE, // TM143: blank (repeat of TM014)
    MOVE_FIRE_PLEDGE, // TM144
    MOVE_WATER_PLEDGE, // TM145
    MOVE_GRASS_PLEDGE, // TM146
    MOVE_WILD_CHARGE, // TM147
    MOVE_NONE, // TM148: blank (repeat of TM036)
    MOVE_NONE, // TM149: blank (repeat of TM026)
    MOVE_NONE, // TM150: blank (repeat of TM071)
    MOVE_PHANTOM_FORCE, // TM151
    MOVE_NONE, // TM152: blank (repeat of TM068)
    MOVE_BLAST_BURN, // TM153
    MOVE_HYDRO_CANNON, // TM154
    MOVE_FRENZY_PLANT, // TM155
    MOVE_OUTRAGE, // TM156
    MOVE_NONE, // TM157: blank (repeat of TM050)
    MOVE_NONE, // TM158: blank (repeat of TM052)
    MOVE_LEAF_STORM, // TM159
    MOVE_HURRICANE, // TM160
    MOVE_NONE, // TM161: blank (repeat of TM092)
    MOVE_BUG_BUZZ, // TM162
    MOVE_NONE, // TM163: blank (repeat of TM015)
    MOVE_BRAVE_BIRD, // TM164
    MOVE_FLARE_BLITZ, // TM165
    MOVE_NONE, // TM166: blank (repeat of TM025)
    MOVE_CLOSE_COMBAT, // TM167
    MOVE_NONE, // TM168: blank (repeat of TM022)
    MOVE_DRACO_METEOR, // TM169
    MOVE_STEEL_BEAM, // TM170
    MOVE_TERA_BLAST, // TM171
    MOVE_NONE, // TM172: blank (repeat of TM005)
    MOVE_CHARGE, // TM173
    MOVE_HAZE, // TM174
    MOVE_NONE, // TM175: blank (repeat of TM006)
    MOVE_SAND_TOMB, // TM176
    MOVE_SPITE, // TM177
    MOVE_GRAVITY, // TM178
    MOVE_SMACK_DOWN, // TM179
    MOVE_NONE, // TM180: blank (repeat of TM074)
    MOVE_KNOCK_OFF, // TM181
    MOVE_BUG_BITE, // TM182
    MOVE_SUPER_FANG, // TM183
    MOVE_VACUUM_WAVE, // TM184
    MOVE_LUNGE, // TM185
    MOVE_HIGH_HORSEPOWER, // TM186
    MOVE_ICICLE_SPEAR, // TM187
    MOVE_SCALD, // TM188
    MOVE_HEAT_CRASH, // TM189
    MOVE_SOLAR_BLADE, // TM190
    MOVE_UPROAR, // TM191
    MOVE_NONE, // TM192: blank (repeat of TM001)
    MOVE_WEATHER_BALL, // TM193
    MOVE_GRASSY_GLIDE, // TM194
    MOVE_BURNING_JEALOUSY, // TM195
    MOVE_FLIP_TURN, // TM196
    MOVE_DUAL_WINGBEAT, // TM197
    MOVE_POLTERGEIST, // TM198
    MOVE_LASH_OUT, // TM199
    MOVE_SCALE_SHOT, // TM200
    MOVE_MISTY_EXPLOSION, // TM201
    MOVE_PAIN_SPLIT, // TM202
    MOVE_NONE, // TM203: blank (repeat of TM077)
    MOVE_DOUBLE_EDGE, // TM204
    MOVE_ENDEAVOR, // TM205
    MOVE_PETAL_BLIZZARD, // TM206
    MOVE_TEMPER_FLARE, // TM207
    MOVE_NONE, // TM208: blank (repeat of HM05)
    MOVE_MUDDY_WATER, // TM209
    MOVE_SUPERCELL_SLAM, // TM210
    MOVE_ELECTROWEB, // TM211
    MOVE_TRIPLE_AXEL, // TM212
    MOVE_COACHING, // TM213
    MOVE_SLUDGE_WAVE, // TM214
    MOVE_SCORCHING_SANDS, // TM215
    MOVE_FEATHER_DANCE, // TM216
    MOVE_FUTURE_SIGHT, // TM217
    MOVE_EXPANDING_FORCE, // TM218
    MOVE_SKITTER_SMACK, // TM219
    MOVE_METEOR_BEAM, // TM220
    MOVE_THROAT_CHOP, // TM221
    MOVE_BREAKING_SWIPE, // TM222
    MOVE_METAL_SOUND, // TM223
    MOVE_CURSE, // TM224
    MOVE_HARD_PRESS, // TM225
    MOVE_DRAGON_CHEER, // TM226
    MOVE_ALLURING_VOICE, // TM227
    MOVE_PSYCHIC_NOISE, // TM228
    MOVE_UPPER_HAND, // TM229

    // TRs. With TRS_ARE_TMS (include/config.h) the first 27 are TM230-TM256: the TR moves no TM teaches, in their old
    // TR order. The other 73 TRs taught a move a TM already teaches and are blanked (unused items).
    MOVE_BODY_SLAM, // TR00 = TM230 (was TR01)
    MOVE_LOW_KICK, // TR01 = TM231 (was TR07)
    MOVE_AGILITY, // TR02 = TM232 (was TR12)
    MOVE_FOCUS_ENERGY, // TR03 = TM233 (was TR13)
    MOVE_METRONOME, // TR04 = TM234 (was TR14)
    MOVE_TRI_ATTACK, // TR05 = TM235 (was TR19)
    MOVE_SPIKES, // TR06 = TM236 (was TR23)
    MOVE_PSYSHOCK, // TR07 = TM237 (was TR25)
    MOVE_MEGAHORN, // TR08 = TM238 (was TR28)
    MOVE_SUPERPOWER, // TR09 = TM239 (was TR39)
    MOVE_BLAZE_KICK, // TR10 = TM240 (was TR41)
    MOVE_COSMIC_POWER, // TR11 = TM241 (was TR44)
    MOVE_LEAF_BLADE, // TR12 = TM242 (was TR50)
    MOVE_TOXIC_SPIKES, // TR13 = TM243 (was TR54)
    MOVE_SEED_BOMB, // TR14 = TM244 (was TR59)
    MOVE_ZEN_HEADBUTT, // TR15 = TM245 (was TR69)
    MOVE_POWER_WHIP, // TR16 = TM246 (was TR72)
    MOVE_ELECTRO_BALL, // TR17 = TM247 (was TR80)
    MOVE_FOUL_PLAY, // TR18 = TM248 (was TR81)
    MOVE_STORED_POWER, // TR19 = TM249 (was TR82)
    MOVE_ALLY_SWITCH, // TR20 = TM250 (was TR83)
    MOVE_WORK_UP, // TR21 = TM251 (was TR85)
    MOVE_VENOM_DRENCH, // TR22 = TM252 (was TR91)
    MOVE_DAZZLING_GLEAM, // TR23 = TM253 (was TR92)
    MOVE_DARKEST_LARIAT, // TR24 = TM254 (was TR93)
    MOVE_PSYCHIC_FANGS, // TR25 = TM255 (was TR97)
    MOVE_BODY_PRESS, // TR26 = TM256 (was TR99)
    MOVE_NONE, // TR27: blank
    MOVE_NONE, // TR28: blank
    MOVE_NONE, // TR29: blank
    MOVE_NONE, // TR30: blank
    MOVE_NONE, // TR31: blank
    MOVE_NONE, // TR32: blank
    MOVE_NONE, // TR33: blank
    MOVE_NONE, // TR34: blank
    MOVE_NONE, // TR35: blank
    MOVE_NONE, // TR36: blank
    MOVE_NONE, // TR37: blank
    MOVE_NONE, // TR38: blank
    MOVE_NONE, // TR39: blank
    MOVE_NONE, // TR40: blank
    MOVE_NONE, // TR41: blank
    MOVE_NONE, // TR42: blank
    MOVE_NONE, // TR43: blank
    MOVE_NONE, // TR44: blank
    MOVE_NONE, // TR45: blank
    MOVE_NONE, // TR46: blank
    MOVE_NONE, // TR47: blank
    MOVE_NONE, // TR48: blank
    MOVE_NONE, // TR49: blank
    MOVE_NONE, // TR50: blank
    MOVE_NONE, // TR51: blank
    MOVE_NONE, // TR52: blank
    MOVE_NONE, // TR53: blank
    MOVE_NONE, // TR54: blank
    MOVE_NONE, // TR55: blank
    MOVE_NONE, // TR56: blank
    MOVE_NONE, // TR57: blank
    MOVE_NONE, // TR58: blank
    MOVE_NONE, // TR59: blank
    MOVE_NONE, // TR60: blank
    MOVE_NONE, // TR61: blank
    MOVE_NONE, // TR62: blank
    MOVE_NONE, // TR63: blank
    MOVE_NONE, // TR64: blank
    MOVE_NONE, // TR65: blank
    MOVE_NONE, // TR66: blank
    MOVE_NONE, // TR67: blank
    MOVE_NONE, // TR68: blank
    MOVE_NONE, // TR69: blank
    MOVE_NONE, // TR70: blank
    MOVE_NONE, // TR71: blank
    MOVE_NONE, // TR72: blank
    MOVE_NONE, // TR73: blank
    MOVE_NONE, // TR74: blank
    MOVE_NONE, // TR75: blank
    MOVE_NONE, // TR76: blank
    MOVE_NONE, // TR77: blank
    MOVE_NONE, // TR78: blank
    MOVE_NONE, // TR79: blank
    MOVE_NONE, // TR80: blank
    MOVE_NONE, // TR81: blank
    MOVE_NONE, // TR82: blank
    MOVE_NONE, // TR83: blank
    MOVE_NONE, // TR84: blank
    MOVE_NONE, // TR85: blank
    MOVE_NONE, // TR86: blank
    MOVE_NONE, // TR87: blank
    MOVE_NONE, // TR88: blank
    MOVE_NONE, // TR89: blank
    MOVE_NONE, // TR90: blank
    MOVE_NONE, // TR91: blank
    MOVE_NONE, // TR92: blank
    MOVE_NONE, // TR93: blank
    MOVE_NONE, // TR94: blank
    MOVE_NONE, // TR95: blank
    MOVE_NONE, // TR96: blank
    MOVE_NONE, // TR97: blank
    MOVE_NONE, // TR98: blank
    MOVE_NONE, // TR99: blank
};

u16 GetItemIndex(u16 item, u16 type);
void *GetItemNarcData(u16 item, u16 type, u32 heap_id);
u16 ItemToMachineMove(u16 itemId);
// void *LONG_CALL ItemDataTableLoad(int heapID);
void ItemMenuUseFunc_RevealGlass(struct ItemMenuUseData *data, const struct ItemCheckUseData *dat2);
BOOL ItemFieldUseFunc_RevealGlass(struct ItemFieldUseData *data);
void *_CreateRevealGlassWork(FieldSystem *fieldSystem);
void ItemMenuUseFunc_DNASplicers(struct ItemMenuUseData *data, const struct ItemCheckUseData *dat2);
BOOL ItemFieldUseFunc_DNASplicers(struct ItemFieldUseData *data);
void *_CreateDNASplicersWork(FieldSystem *fieldSystem);
void ItemMenuUseFunc_AbilityCapsule(struct ItemMenuUseData *data, const struct ItemCheckUseData *dat2);
void ItemMenuUseFunc_Mint(struct ItemMenuUseData *data, const struct ItemCheckUseData *dat2 UNUSED);
void ItemMenuUseFunc_Nectar(struct ItemMenuUseData *data, const struct ItemCheckUseData *dat2 UNUSED);
void ItemMenuUseFunc_RotomCatalog(struct ItemMenuUseData *data, const struct ItemCheckUseData *dat2 UNUSED);
#ifdef REGIONAL_CHARM
void ItemMenuUseFunc_RegionalCharm(struct ItemMenuUseData *data, const struct ItemCheckUseData *dat2);
BOOL ItemFieldUseFunc_RegionalCharm(struct ItemFieldUseData *data);
#endif
#ifdef EXP_SHARE_KEY_ITEM
void ItemMenuUseFunc_ExpShare(struct ItemMenuUseData *data, const struct ItemCheckUseData *dat2);
BOOL ItemFieldUseFunc_ExpShare(struct ItemFieldUseData *data);
#endif

// fieldUseFunc in data/itemdata/itemdata.c is NUM_VANILLA_FIELD_USE_FUNCS + the index here
const struct ItemUseFuncDat sNewItemFieldUseFuncs[] = {
    { ItemMenuUseFunc_RevealGlass, ItemFieldUseFunc_RevealGlass, NULL },
    { ItemMenuUseFunc_DNASplicers, ItemFieldUseFunc_DNASplicers, NULL },
    { ItemMenuUseFunc_AbilityCapsule, NULL, NULL },
    { ItemMenuUseFunc_Mint, NULL, NULL },
    { ItemMenuUseFunc_Nectar, NULL, NULL },
    { ItemMenuUseFunc_RotomCatalog, NULL, NULL },
#ifdef REGIONAL_CHARM
    { ItemMenuUseFunc_RegionalCharm, ItemFieldUseFunc_RegionalCharm, NULL }, // 36
#else
    { NULL, NULL, NULL }, // 36, kept so the rows after it keep their numbers
#endif
#ifdef EXP_SHARE_KEY_ITEM
    { ItemMenuUseFunc_ExpShare, ItemFieldUseFunc_ExpShare, NULL }, // 37
#endif
};

extern const struct ItemUseFuncDat sItemFieldUseFuncs[NUM_VANILLA_FIELD_USE_FUNCS];

void *LONG_CALL GetItemFieldUseFunc(int funcType, u16 itemType)
{
    const struct ItemUseFuncDat *useFunc;
    if (itemType >= NUM_VANILLA_FIELD_USE_FUNCS) {
        u16 index = itemType - NUM_VANILLA_FIELD_USE_FUNCS;
        if (index >= NELEMS(sNewItemFieldUseFuncs)) {
            return NULL;
        }
        useFunc = &sNewItemFieldUseFuncs[index];
    } else {
        useFunc = &sItemFieldUseFuncs[itemType];
    }
    if (funcType == USE_ITEM_TASK_MENU) {
        return useFunc->menu;
    } else if (funcType == USE_ITEM_TASK_FIELD) {
        return useFunc->field;
    } else {
        return useFunc->check;
    }
}

u16 GetItemIndex(u16 item, u16 type)
{
    u16 ret = 0;
    switch (type) {
    case ITEM_GET_DATA:
        if (item == ITEM_DUMMY_ID || item == ITEM_RETURN_ID) {
            break;
        }
        ret = item;
        return ret;

    case ITEM_GET_ICON_CGX:
        if (item == ITEM_DUMMY_ID) {
            return GFX_ITEM_DUMMY_ID;
        }
        if (item == ITEM_RETURN_ID) {
            return GFX_ITEM_RETURN_ID;
        }
        ret = item * 2 + 2;
        return ret;

    case ITEM_GET_ICON_PAL:
        if (item == ITEM_DUMMY_ID) {
            return GFX_ITEM_DUMMY_ID + 1;
        }
        if (item == ITEM_RETURN_ID) {
            return GFX_ITEM_RETURN_ID + 1;
        }
        ret = item * 2 + 3;
        return ret;

    case ITEM_GET_AGB_NUM: // for pal park purposes
        if (item == ITEM_DUMMY_ID || item == ITEM_RETURN_ID) {
            break;
        }
        if (item > MAX_TOTAL_ITEM_NUM) {
            ret = 0;
        } else {
            ret = ItemDataIndex[item].agb_id;
        }
        return ret;
    }
    return 0;
}

void *GetItemNarcData(u16 item, u16 type, u32 heapID)
{
    int dataID, picID, palID;

    dataID = item;
    picID = item * 2 + 2;
    palID = item * 2 + 3;

    switch (type) {
    case ITEM_GET_DATA:
        return AllocAndReadWholeNarcMemberByIdPair(ARC_ITEM_DATA, dataID, heapID);
    case ITEM_GET_ICON_CGX:
        return AllocAndReadWholeNarcMemberByIdPair(ARC_ITEM_GFX_DATA, picID, heapID);
    case ITEM_GET_ICON_PAL:
        return AllocAndReadWholeNarcMemberByIdPair(ARC_ITEM_GFX_DATA, palID, heapID);
    }
    return NULL;
}

void LONG_CALL GetItemDescIntoString(String *dest, u16 itemId, u16 heapId)
{
    enum ItemGeneration gen = ITEM_GENERATION(itemId);
    u32 fileId = (gen == CUSTOM)
        ? MSG_DATA_ITEM_DESCRIPTION_CUSTOM
        : MSG_DATA_ITEM_FILE(MSG_DATA_ITEM_DESCRIPTION_GEN4, gen);
    MsgData *msgData = NewMsgDataFromNarc(MSGDATA_LOAD_LAZY, ARC_MSG_DATA, fileId, heapId);
    u32 offset = ITEM_MSG_OFFSET(itemId);
    ReadMsgDataIntoString(msgData, offset, dest);
    DestroyMsgData(msgData);
}

void *LONG_CALL ItemDataTableLoad(int heapID)
{
    int max = GetItemIndex(MAX_TOTAL_ITEM_NUM, ITEM_GET_DATA) + 1;

    return AllocAndReadFromNarcMemberByIdPair(ARC_ITEM_DATA, 0, heapID, 0, sizeof(ITEMDATA) * max); // 800757Ch
}

/**
 * @brief converts an item id to its corresponding TM/HM/TR index within sMachineMoves
 * @see   pret/pokeheartgold ItemToTMHMId
 */
u16 ItemToMachineMoveIndex(u16 itemId)
{
    if (itemId >= ITEM_TM001 && itemId <= ITEM_HM08) {
        return itemId - ITEM_TM001;
    }
    if (itemId == ITEM_HM07_ORAS) {
        return 100;
    }
    if (itemId == ITEM_TM00) {
        return 101;
    }
    if (itemId >= ITEM_TM093 && itemId <= ITEM_TM095) {
        return itemId - ITEM_TM093 + 102;
    }
    if (itemId >= ITEM_TM096 && itemId <= ITEM_TM100) {
        return itemId - ITEM_TM096 + 105;
    }
    if (itemId >= ITEM_TM100_SV && itemId <= ITEM_TM229) {
        return itemId - ITEM_TM100_SV + 110;
    }
    if (itemId >= ITEM_TR00 && itemId <= ITEM_TR99) {
        return itemId - ITEM_TR00 + 240;
    }

    return 0;
}

static u16 sMachineNumbers[NELEMS(sMachineMoves)];
static BOOL sMachineNumbersReady = FALSE;

static void NumberMachines(u16 *next, u16 first, u16 last)
{
    for (u16 i = first; i <= last; i++) {
        sMachineNumbers[i] = sMachineMoves[i] == MOVE_NONE ? 0xFFFF : (*next)++;
    }
}

/**
 * @brief the number a TM shows in the bag (TM000, TM001, ...). TMs are numbered in their original order with the blank
 *        ones (MOVE_NONE in sMachineMoves: repeats removed on 2026-10-11) left out, so the TMs after a blank one move up.
 *        0xFFFF for a blank one. HMs aren't numbered here (see GetMachineMoveNumber in src/bag.c)
 */
u16 GetTMDisplayNumber(u16 itemId)
{
    if (!sMachineNumbersReady) {
        u16 next = 0;
        for (u32 i = 0; i < NELEMS(sMachineNumbers); i++) {
            sMachineNumbers[i] = 0xFFFF;
        }
        // in the original number order: TM00, TM001-TM092, TM093-TM095, TM096-TM100, TM100 (SV)-TM229, the TRs
        NumberMachines(&next, 101, 101);
        NumberMachines(&next, 0, 91);
        NumberMachines(&next, 102, 104);
        NumberMachines(&next, 105, 109);
        NumberMachines(&next, 110, 239);
#ifdef TRS_ARE_TMS
        NumberMachines(&next, 240, 240 + TRS_AS_TMS_COUNT - 1);
#endif
        sMachineNumbersReady = TRUE;
    }
    u16 index = ItemToMachineMoveIndex(itemId);
    return index < NELEMS(sMachineNumbers) ? sMachineNumbers[index] : 0xFFFF;
}

/**
 * @brief converts an item id to its corresponding TM/HM/TR move id
 * @see   pret/pokeheartgold TMHMGetMove
 */
u16 ItemToMachineMove(u16 itemId)
{
    if (itemId < ITEM_TM001) {
        return MOVE_NONE;
    }

    u16 index = ItemToMachineMoveIndex(itemId);
    if (index >= NELEMS(sMachineMoves)) {
        return MOVE_NONE;
    }
    return sMachineMoves[index];
}

BOOL MoveIsHM(u16 moveId)
{
    for (u8 i = 0; i < NUM_HMS; i++) {
        if (sMachineMoves[i + ITEM_HM01 - ITEM_TM001] == moveId) {
#if defined(REUSABLE_TMS) && defined(DELETABLE_HMS)
            return FALSE;
#else
            return TRUE;
#endif
        }
    }
    return FALSE;
}

s32 LONG_CALL GetItemAttr_PreloadedItemData(ITEMDATA *itemData, u16 attrno)
{
    switch (attrno) {
    case ITEM_PARAM_PRICE:
        return (u32)((itemData)->price) | ((u32)((itemData)->price_high) << 16);
    case ITEM_PARAM_HOLD_EFFECT:
        return itemData->holdEffect;
    case ITEM_PARAM_HOLD_EFFECT_PARAM:
        return itemData->holdEffectParam;
    case ITEM_PARAM_PREVENT_TOSS:
        return itemData->prevent_toss;
    case ITEM_PARAM_SELECTABLE:
        return itemData->selectable;
    case ITEM_PARAM_FIELD_POCKET:
        return itemData->fieldPocket;
    case ITEM_PARAM_FIELD:
        return itemData->fieldUseFunc;
    case ITEM_PARAM_BATTLE:
        return itemData->battleUseFunc;
    case ITEM_PARAM_PLUCK_EFFECT:
        return itemData->pluckEffect;
    case ITEM_PARAM_FLING_EFFECT:
        return itemData->flingEffect;
    case ITEM_PARAM_FLING_POWER:
        return itemData->flingPower;
    case ITEM_PARAM_NATURAL_GIFT_POWER:
        return itemData->naturalGiftPower;
    case ITEM_PARAM_NATURAL_GIFT_TYPE:
        return itemData->naturalGiftType;
    case ITEM_PARAM_BATTLE_POCKET:
        return itemData->battlePocket;
    case ITEM_PARAM_PARTY:
        return itemData->partyUse;
    default:
        switch (itemData->partyUse) {
        case 0:
            return itemData->dummy;
        case 1:
            return GetItemAttrSub(&itemData->partyUseParam, attrno);
        }
    }

    return 0;
}

void ItemMenuUseFunc_RevealGlass(struct ItemMenuUseData *data, const struct ItemCheckUseData *dat2 UNUSED)
{
    FieldSystem *fieldSystem = data->taskManager->fieldSystem; // TaskManager_GetFieldSystem(data->taskManager);
    struct BagViewAppWork *env = data->taskManager->env; // TaskManager_GetEnvironment(data->taskManager);
    env->atexit_TaskEnv = sub_0203FAE8(fieldSystem, HEAPID_WORLD, ITEM_REVEAL_GLASS);
    sub_0203C8F0(env, 0x0203CA9C | 1);
}

BOOL ItemFieldUseFunc_RevealGlass(struct ItemFieldUseData *data)
{
    RegisteredItem_CreateGoToAppTask(data, (FieldApplicationWorkCtor)_CreateRevealGlassWork, FALSE);
    return TRUE;
}

void *_CreateRevealGlassWork(FieldSystem *fieldSystem)
{
    return sub_0203FAE8(fieldSystem, HEAPID_WORLD, ITEM_REVEAL_GLASS);
}

void ItemMenuUseFunc_DNASplicers(struct ItemMenuUseData *data, const struct ItemCheckUseData *dat2 UNUSED)
{
    FieldSystem *fieldSystem = data->taskManager->fieldSystem; // TaskManager_GetFieldSystem(data->taskManager);
    struct BagViewAppWork *env = data->taskManager->env; // TaskManager_GetEnvironment(data->taskManager);
    env->atexit_TaskEnv = sub_0203FAE8(fieldSystem, HEAPID_WORLD, ITEM_DNA_SPLICERS_FUSE); // TODO: handle correct item
    sub_0203C8F0(env, 0x0203CA9C | 1);
}

BOOL ItemFieldUseFunc_DNASplicers(struct ItemFieldUseData *data)
{
    RegisteredItem_CreateGoToAppTask(data, (FieldApplicationWorkCtor)_CreateDNASplicersWork, FALSE);
    return TRUE;
}

void *_CreateDNASplicersWork(FieldSystem *fieldSystem)
{
    return sub_0203FAE8(fieldSystem, HEAPID_WORLD, ITEM_DNA_SPLICERS_FUSE); // TODO: handle correct item
}

void ItemMenuUseFunc_AbilityCapsule(struct ItemMenuUseData *data, const struct ItemCheckUseData *dat2 UNUSED)
{
    FieldSystem *fieldSystem = data->taskManager->fieldSystem; // TaskManager_GetFieldSystem(data->taskManager);
    struct BagViewAppWork *env = data->taskManager->env; // TaskManager_GetEnvironment(data->taskManager);
    env->atexit_TaskEnv = sub_0203FAE8(fieldSystem, HEAPID_WORLD, ITEM_ABILITY_CAPSULE);
    sub_0203C8F0(env, 0x0203CA9C | 1);
}

void ItemMenuUseFunc_Mint(struct ItemMenuUseData *data, const struct ItemCheckUseData *dat2 UNUSED)
{
    FieldSystem *fieldSystem = data->taskManager->fieldSystem; // TaskManager_GetFieldSystem(data->taskManager);
    struct BagViewAppWork *env = data->taskManager->env; // TaskManager_GetEnvironment(data->taskManager);
    env->atexit_TaskEnv = sub_0203FAE8(fieldSystem, HEAPID_WORLD, data->itemId);
    sub_0203C8F0(env, 0x0203CA9C | 1);
}

void ItemMenuUseFunc_Nectar(struct ItemMenuUseData *data, const struct ItemCheckUseData *dat2 UNUSED)
{
    FieldSystem *fieldSystem = data->taskManager->fieldSystem; // TaskManager_GetFieldSystem(data->taskManager);
    struct BagViewAppWork *env = data->taskManager->env; // TaskManager_GetEnvironment(data->taskManager);
    env->atexit_TaskEnv = sub_0203FAE8(fieldSystem, HEAPID_WORLD, data->itemId);
    sub_0203C8F0(env, 0x0203CA9C | 1);
}

void ItemMenuUseFunc_RotomCatalog(struct ItemMenuUseData *data, const struct ItemCheckUseData *dat2 UNUSED)
{

    FieldSystem *fieldSystem = data->taskManager->fieldSystem; // TaskManager_GetFieldSystem(data->taskManager);
    struct BagViewAppWork *env = data->taskManager->env; // TaskManager_GetEnvironment(data->taskManager);
    env->atexit_TaskEnv = sub_0203FAE8(fieldSystem, HEAPID_WORLD, ITEM_ROTOM_CATALOG);
    sub_0203C8F0(env, 0x0203CA9C | 1);
}

#ifdef REGIONAL_CHARM

#define REGIONAL_CHARM_SCRIPT 2075 // scr_seq_0003_075_regional_charm in data/scr_seq/scr_seq_00003_commonscript.s

/**
 * @brief closes the bag and runs the script that turns the Regional Charm on or off
 */
void ItemMenuUseFunc_RegionalCharm(struct ItemMenuUseData *data, const struct ItemCheckUseData *dat2)
{
    SetUpItemScript(data, dat2, REGIONAL_CHARM_SCRIPT);
}

/**
 * @brief the same from the field, once the Regional Charm is registered to Y
 * @return FALSE: there is no app to open, so the item use data can be freed right away
 */
BOOL ItemFieldUseFunc_RegionalCharm(struct ItemFieldUseData *data)
{
    EventSet_Script(data->fieldSystem, REGIONAL_CHARM_SCRIPT, NULL);
    return FALSE;
}

#endif // REGIONAL_CHARM

#ifdef EXP_SHARE_KEY_ITEM

#define EXP_SHARE_SCRIPT 2076 // scr_seq_0003_076_exp_share in data/scr_seq/scr_seq_00003_commonscript.s

/**
 * @brief closes the bag and runs the script that turns the Exp. Share on or off
 */
void ItemMenuUseFunc_ExpShare(struct ItemMenuUseData *data, const struct ItemCheckUseData *dat2)
{
    SetUpItemScript(data, dat2, EXP_SHARE_SCRIPT);
}

/**
 * @brief the same from the field, once the Exp. Share is registered to Y
 * @return FALSE: there is no app to open, so the item use data can be freed right away
 */
BOOL ItemFieldUseFunc_ExpShare(struct ItemFieldUseData *data)
{
    EventSet_Script(data->fieldSystem, EXP_SHARE_SCRIPT, NULL);
    return FALSE;
}

#endif // EXP_SHARE_KEY_ITEM
