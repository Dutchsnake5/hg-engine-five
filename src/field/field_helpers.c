#include "config.h"
#include "debug.h"
#include "types.h"

#include "constants/item.h"
#include "constants/moves.h"
#include "constants/species.h"

#include "pokemon.h"

// helpers that only field code (the field extension) calls, kept here instead of in the always-loaded code. they were
// in src/pokemon.c

/**
 *  @brief clear a PartyPokemon's moves by setting them to zero
 *
 *  @param pokemon PartyPokemon whose moves to clear
 */
void LONG_CALL ClearMonMoves(struct PartyPokemon *pokemon)
{
    int null = 0;
    for (int i = 0; i < 4; i++) {
        SetMonData(pokemon, MON_DATA_MOVE1 + i, &null);
    }
}

void LONG_CALL ChangeToBattleForm(struct PartyPokemon *pp)
{
    int monsNo = GetMonData(pp, MON_DATA_SPECIES, NULL);
    int formNo = GetMonData(pp, MON_DATA_FORM, NULL);

    RevertFormChange(pp, monsNo, formNo);

    switch (monsNo) {
    case SPECIES_XERNEAS:
        formNo = 1;
        ChangePartyPokemonToForm(pp, formNo);
        break;
    case SPECIES_ZACIAN:
        if (GetMonData(pp, MON_DATA_HELD_ITEM, NULL) == ITEM_RUSTED_SWORD) {
            formNo = 1;
            ChangePartyPokemonToForm(pp, formNo);
            correct_zacian_zamazenta_kyurem_moves_for_form(pp, formNo, 0);
        }
        break;
    case SPECIES_ZAMAZENTA:
        if (GetMonData(pp, MON_DATA_HELD_ITEM, NULL) == ITEM_RUSTED_SHIELD) {
            formNo = 1;
            ChangePartyPokemonToForm(pp, formNo);
            correct_zacian_zamazenta_kyurem_moves_for_form(pp, formNo, 0);
        }
        break;

    default:
        break;
    }
}
