#include "config.h"
#include "types.h"

#include "constants/species.h"

#include "pokemon.h"

// The Regional Charm (REGIONAL_CHARM in include/config.h): while it is turned on, a random wild Pokémon whose species
// has a regional form appears in it. AddWildPartyPokemon (src/field/enemy_party.c) asks for the form here.

#ifdef REGIONAL_CHARM

struct RegionalForm {
    u16 base;
    u16 regional;
};

// only true regional variants: no totem, noble, Mega or cosmetic forms, which share the species number ranges
static const struct RegionalForm sRegionalForms[] = {
    // Alolan
    { SPECIES_RATTATA, SPECIES_RATTATA_ALOLAN },
    { SPECIES_RATICATE, SPECIES_RATICATE_ALOLAN },
    { SPECIES_RAICHU, SPECIES_RAICHU_ALOLAN },
    { SPECIES_SANDSHREW, SPECIES_SANDSHREW_ALOLAN },
    { SPECIES_SANDSLASH, SPECIES_SANDSLASH_ALOLAN },
    { SPECIES_VULPIX, SPECIES_VULPIX_ALOLAN },
    { SPECIES_NINETALES, SPECIES_NINETALES_ALOLAN },
    { SPECIES_DIGLETT, SPECIES_DIGLETT_ALOLAN },
    { SPECIES_DUGTRIO, SPECIES_DUGTRIO_ALOLAN },
    { SPECIES_MEOWTH, SPECIES_MEOWTH_ALOLAN },
    { SPECIES_PERSIAN, SPECIES_PERSIAN_ALOLAN },
    { SPECIES_GEODUDE, SPECIES_GEODUDE_ALOLAN },
    { SPECIES_GRAVELER, SPECIES_GRAVELER_ALOLAN },
    { SPECIES_GOLEM, SPECIES_GOLEM_ALOLAN },
    { SPECIES_GRIMER, SPECIES_GRIMER_ALOLAN },
    { SPECIES_MUK, SPECIES_MUK_ALOLAN },
    { SPECIES_EXEGGUTOR, SPECIES_EXEGGUTOR_ALOLAN },
    { SPECIES_MAROWAK, SPECIES_MAROWAK_ALOLAN },

    // Galarian
    { SPECIES_MEOWTH, SPECIES_MEOWTH_GALARIAN },
    { SPECIES_PONYTA, SPECIES_PONYTA_GALARIAN },
    { SPECIES_RAPIDASH, SPECIES_RAPIDASH_GALARIAN },
    { SPECIES_SLOWPOKE, SPECIES_SLOWPOKE_GALARIAN },
    { SPECIES_SLOWBRO, SPECIES_SLOWBRO_GALARIAN },
    { SPECIES_FARFETCHD, SPECIES_FARFETCHD_GALARIAN },
    { SPECIES_WEEZING, SPECIES_WEEZING_GALARIAN },
    { SPECIES_MR_MIME, SPECIES_MR_MIME_GALARIAN },
    { SPECIES_ARTICUNO, SPECIES_ARTICUNO_GALARIAN },
    { SPECIES_ZAPDOS, SPECIES_ZAPDOS_GALARIAN },
    { SPECIES_MOLTRES, SPECIES_MOLTRES_GALARIAN },
    { SPECIES_SLOWKING, SPECIES_SLOWKING_GALARIAN },
    { SPECIES_CORSOLA, SPECIES_CORSOLA_GALARIAN },
    { SPECIES_ZIGZAGOON, SPECIES_ZIGZAGOON_GALARIAN },
    { SPECIES_LINOONE, SPECIES_LINOONE_GALARIAN },
    { SPECIES_DARUMAKA, SPECIES_DARUMAKA_GALARIAN },
    { SPECIES_DARMANITAN, SPECIES_DARMANITAN_GALARIAN },
    { SPECIES_YAMASK, SPECIES_YAMASK_GALARIAN },
    { SPECIES_STUNFISK, SPECIES_STUNFISK_GALARIAN },

    // Hisuian
    { SPECIES_GROWLITHE, SPECIES_GROWLITHE_HISUIAN },
    { SPECIES_ARCANINE, SPECIES_ARCANINE_HISUIAN },
    { SPECIES_VOLTORB, SPECIES_VOLTORB_HISUIAN },
    { SPECIES_ELECTRODE, SPECIES_ELECTRODE_HISUIAN },
    { SPECIES_TYPHLOSION, SPECIES_TYPHLOSION_HISUIAN },
    { SPECIES_QWILFISH, SPECIES_QWILFISH_HISUIAN },
    { SPECIES_SNEASEL, SPECIES_SNEASEL_HISUIAN },
    { SPECIES_SAMUROTT, SPECIES_SAMUROTT_HISUIAN },
    { SPECIES_LILLIGANT, SPECIES_LILLIGANT_HISUIAN },
    { SPECIES_ZORUA, SPECIES_ZORUA_HISUIAN },
    { SPECIES_ZOROARK, SPECIES_ZOROARK_HISUIAN },
    { SPECIES_BRAVIARY, SPECIES_BRAVIARY_HISUIAN },
    { SPECIES_SLIGGOO, SPECIES_SLIGGOO_HISUIAN },
    { SPECIES_GOODRA, SPECIES_GOODRA_HISUIAN },
    { SPECIES_AVALUGG, SPECIES_AVALUGG_HISUIAN },
    { SPECIES_DECIDUEYE, SPECIES_DECIDUEYE_HISUIAN },

    // Paldean
    { SPECIES_WOOPER, SPECIES_WOOPER_PALDEAN },
    { SPECIES_TAUROS, SPECIES_TAUROS_COMBAT },
    { SPECIES_TAUROS, SPECIES_TAUROS_BLAZE },
    { SPECIES_TAUROS, SPECIES_TAUROS_AQUA },
};

#define MAX_REGIONAL_FORMS_PER_SPECIES 4
#define MAX_FORMS_PER_SPECIES          32 // rows of PokeFormDataTbl

/**
 * @brief the form number of a regional form of a species, picked at random when it has several
 *
 * The form number is looked up in the form table instead of being written down, because which forms a species has
 * before its regional one depends on config.h (Raichu's Mega forms, for example).
 *
 * @param species base species of a wild Pokémon
 * @return form number to give it, or 0 when its species has no regional form
 */
u8 RegionalForms_PickWildForm(u16 species)
{
    u8 forms[MAX_REGIONAL_FORMS_PER_SPECIES];
    u32 count = 0;

    for (u32 i = 0; i < NELEMS(sRegionalForms) && count < MAX_REGIONAL_FORMS_PER_SPECIES; i++) {
        if (sRegionalForms[i].base != species) {
            continue;
        }
        for (u32 form = 1; form < MAX_FORMS_PER_SPECIES; form++) {
            u16 formSpecies = GetSpeciesBasedOnForm(species, form);
            if (formSpecies == species) { // past the species' last form
                break;
            }
            if (formSpecies == sRegionalForms[i].regional) {
                forms[count++] = form;
                break;
            }
        }
    }

    if (count == 0) {
        return 0;
    }
    return forms[gf_rand() % count];
}

#endif // REGIONAL_CHARM
