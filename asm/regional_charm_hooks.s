.text
.align 2
.thumb

#include "../include/config.h"

// the Regional Charm (REGIONAL_CHARM in include/config.h) changes random wild Pokémon only. scripted wild battles
// (static encounters, legendaries, event Pokémon) make their Pokémon through the same AddWildPartyPokemon, so mark
// them here, at the start of the one overlay 2 function the field's scripted battle setups call
#ifdef REGIONAL_CHARM

// ov02 0x02247F48, in the scripted wild Pokémon setup (ov02 0x02247F30): replaces
// adds r4, r0, #0 / adds r0, r5, #0 / adds r1, r4, #0 / movs r2, #0, which set up its call to build the encounter info
.global RegionalCharm_ScriptedEncounter_hook
RegionalCharm_ScriptedEncounter_hook:
ldr r2, =gRegionalCharmScriptedEncounter
mov r1, #1
str r1, [r2]
// replaced instructions
add r4, r0, #0
add r0, r5, #0
add r1, r4, #0
mov r2, #0
ldr r3, =0x02247F50 | 1 // r3 is set again right there
bx r3

.pool

.data

.align 2

// set while a scripted wild battle makes its Pokémon; AddWildPartyPokemon reads and clears it
.global gRegionalCharmScriptedEncounter
gRegionalCharmScriptedEncounter:
.word 0

#endif // REGIONAL_CHARM
