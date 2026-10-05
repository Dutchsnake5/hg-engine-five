#include "../include/types.h"
#include "../include/bag.h"
#include "../include/party_menu.h"
#include "../include/config.h"
#include "../include/pokemon.h"
#include "../include/save.h"
#include "../include/script.h"
#include "../include/constants/file.h"
#include "../include/constants/item.h"
#include "../include/constants/moves.h"

#define PARTY_SLOT_NONE 0xFF

u16 ItemToMachineMove(u16 itemId);
u16 ItemToMachineMoveIndex(u16 itemId);
BOOL GetMonMachineMoveCompat(struct PartyPokemon *pp, u16 machineMoveIndex);

static BOOL MonKnowsMove(struct PartyPokemon *pp, u16 move)
{
    for (int i = 0; i < MAX_MON_MOVES; i++) {
        if (GetMonData(pp, MON_DATA_MOVE1 + i, NULL) == move) {
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * @brief the HM item that teaches a move, or ITEM_NONE if the move is not taught by an HM
 */
u16 FieldMove_GetHMItem(u16 move)
{
    for (u16 item = ITEM_HM01; item <= ITEM_HM08; item++) {
        if (ItemToMachineMove(item) == move) {
            return item;
        }
    }
    return ITEM_NONE;
}

/**
 * @brief whether the player can use a move outside of battle because the HM that teaches it is in the bag
 */
BOOL FieldMove_CanUseFromBag(u16 move)
{
#ifdef HM_MOVES_FROM_BAG
    u16 item = FieldMove_GetHMItem(move);
    return item != ITEM_NONE && Bag_HasItem(Sav2_Bag_get(SaveBlock2_get()), item, 1, HEAPID_WORLD);
#else
    (void)move;
    return FALSE;
#endif
}

/**
 * @brief whether a party Pokémon could learn the HM that teaches a move
 */
BOOL FieldMove_MonCanLearnHM(struct PartyPokemon *pp, u16 move)
{
    u16 item = FieldMove_GetHMItem(move);
    return item != ITEM_NONE && GetMonMachineMoveCompat(pp, ItemToMachineMoveIndex(item));
}

/**
 * @brief the party slot of the Pokémon that will use a field move, or PARTY_SLOT_NONE.
 *        a Pokémon that knows the move comes first; with the HM in the bag, the first one that could learn it,
 *        then the first Pokémon that is not an egg
 */
int FieldMove_FindPartySlot(struct Party *party, u16 move)
{
    int count = party->count;
    struct PartyPokemon *pp;

    for (int i = 0; i < count; i++) {
        pp = Party_GetMonByIndex(party, i);
        if (!GetMonData(pp, MON_DATA_IS_EGG, NULL) && MonKnowsMove(pp, move)) {
            return i;
        }
    }

    if (!FieldMove_CanUseFromBag(move)) {
        return PARTY_SLOT_NONE;
    }

    int firstMon = PARTY_SLOT_NONE;
    for (int i = 0; i < count; i++) {
        pp = Party_GetMonByIndex(party, i);
        if (GetMonData(pp, MON_DATA_IS_EGG, NULL)) {
            continue;
        }
        if (FieldMove_MonCanLearnHM(pp, move)) {
            return i;
        }
        if (firstMon == PARTY_SLOT_NONE) {
            firstMon = i;
        }
    }
    return firstMon;
}

#ifdef HM_MOVES_FROM_BAG

/**
 * @brief replaces the overworld's search for a party Pokémon that knows a move (Surf at the shore, Waterfall)
 * @see   arm9 0x020542E8
 */
int LONG_CALL Party_FindMonWithFieldMove(struct Party *party, u16 move)
{
    return FieldMove_FindPartySlot(party, move);
}

/**
 * @brief replaces script command 141 (CheckMoveInParty), which the HM obstacle scripts use.
 *        stores the party slot of the Pokémon that will use the move, or 6 if none can
 */
BOOL LONG_CALL ScrCmd_CheckMoveInParty(SCRIPTCONTEXT *ctx)
{
    u16 *result = ScriptGetVarPointer(ctx);
    u16 move = ScriptGetVar(ctx);

    int slot = FieldMove_FindPartySlot(SaveData_GetPlayerPartyPtr(ctx->fsys->savedata), move);
    *result = (slot == PARTY_SLOT_NONE) ? 6 : slot;
    return FALSE;
}

#endif // HM_MOVES_FROM_BAG
