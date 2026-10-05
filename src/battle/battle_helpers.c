#include "config.h"
#include "debug.h"
#include "types.h"

#include "constants/file.h"

#include "io_reg.h"
#include "pokemon.h"
#include "save.h"

// helpers that only battle code (the battle extension and the battle-only individual overlays) calls, kept here
// instead of in the always-loaded code. they were in src/save.c and src/pokemon.c

// hardware sqrt implementation using the gpio registers + debug options
u32 LONG_CALL sqrt(u32 num)
{
    reg_CP_SQRT_PARAM_L = num;
    reg_CP_SQRTCNT = 0; // start sqrt calculation

    u8 buf[64];
#ifdef DEBUG_SQRT
    sprintf(buf, "[SQRT]   PARAM = %08X\n", reg_CP_SQRT_PARAM_L);
    debugsyscall(buf);
#endif

    while ((reg_CP_SQRTCNT & (1 << 15)) != 0) {
#ifdef DEBUG_SQRT
        sprintf(buf, "[SQRT] SQRTCNT = %08X\n", reg_CP_SQRTCNT);
        debugsyscall(buf);
#endif
    }

    sprintf(buf, "[SQRT]  RESULT = %08X\n", reg_CP_SQRT_RESULT); // need to have something here so that it won't return 0
#ifdef DEBUG_SQRT
    debugsyscall(buf);
#endif

    return reg_CP_SQRT_RESULT;
}

/**
 *  @brief check if an element of an array exists byte-for-byte in the buf sent to it
 *
 *  @param array pointer to any type array
 *  @param element pointer to any element of an array
 *  @param len number of elements in the overall array
 *  @param size size of each individual element, used both as length of element and length of members of array
 *  @return TRUE if the element exists verbatim inside of the array; FALSE otherwise
 */
BOOL LONG_CALL IsElementInArray(const void *array, void *element, u32 len, u32 size)
{
    u32 i, j;
    const u8 *arr = array;
    u8 *elem = element;
    // u8 buf[64];
    // sprintf(buf, "Called IsElementInArray(0x%08X, 0x%08X, 0x%X, 0x%X)\n", (const u32)array, (u32)element, len, size);
    // debugsyscall(buf);
    for (i = 0; i < len; i++) {
        for (j = 0; j < size; j++) {
            const u8 *currElem = &arr[i * size];
            if (j[currElem] != elem[j]) {
                break;
            }
        }
        if (j == size) {
            return TRUE;
        }
    }
    // debugsyscall("Element is not in array!");
    return FALSE;
}

/**
 *  @brief get species base experience, modified for form.  base experience is no longer in personal
 *
 *  @param species species index
 *  @param form form number
 *  @return base experience
 */
u32 LONG_CALL GetSpeciesBaseExp(u32 species, u32 form)
{
    u16 baseExp;
    species = PokeOtherFormMonsNoGet(species, form); // for whatever reason alternate formes can have different base experiences
    ReadFromNarcMemberByIdPair(&baseExp, ARC_CODE_ADDONS, CODE_ADDON_BASE_EXPERIENCE_LIST, sizeof(u16) * species, sizeof(u16));
    return baseExp;
}
