#include "../include/types.h"
#include "../include/config.h"
#include "../include/save.h"
#include "../include/system.h"

// the Options screen's GAME SPEED row (GAME_SPEED_OPTION in include/config.h). it replaces TEXT SPEED: the game keeps
// that row's value in the save's 4 text speed bits (Options, pret pokeheartgold include/options.h), so game speed is
// stored there as GAME_SPEED_STORED_BASE + choice and older saves, which hold a text speed of 0-2, read as x1

#ifdef GAME_SPEED_OPTION

#define GAME_SPEED_STORED_BASE 4
#define GAME_SPEED_X1          0
#define GAME_SPEED_X1_5        1
#define GAME_SPEED_X2          2

#define TEXT_FRAME_DELAY_FAST  1

void *LONG_CALL Save_PlayerData_GetOptionsAddr(void *saveData);

static u32 GameSpeed_FromOptions(u16 *options)
{
    u32 stored = *options & 0xF;
    if (stored >= GAME_SPEED_STORED_BASE && stored <= GAME_SPEED_STORED_BASE + GAME_SPEED_X2) {
        return stored - GAME_SPEED_STORED_BASE;
    }
    return GAME_SPEED_X1;
}

/**
 * @brief replaces Options_GetTextSpeed (arm9 0x0202AD20). only the Options screen (overlay 54) reads it; the Safari Zone
 *        area picker (overlay 108) did too and is patched to fast text in bytereplacement
 */
u32 Options_GetTextSpeed_GameSpeed(u16 *options)
{
    return GameSpeed_FromOptions(options);
}

/**
 * @brief replaces Options_SetTextSpeed (arm9 0x0202AD28), which the Options screen calls with the row's choice
 */
void Options_SetTextSpeed_GameSpeed(u16 *options, u32 choice)
{
    if (choice > GAME_SPEED_X2) {
        choice = GAME_SPEED_X1;
    }
    *options = (*options & ~0xF) | (GAME_SPEED_STORED_BASE + choice);
}

/**
 * @brief replaces Options_GetTextFrameDelay (arm9 0x0202AD3C): every text box prints at the fast speed
 */
u8 Options_GetTextFrameDelay_GameSpeed(u16 *options UNUSED)
{
    return TEXT_FRAME_DELAY_FAST;
}

/**
 * @brief the main loop's first wait for a screen refresh (arm9 0x02000DF4, in NitroMain; called from
 *        GameSpeed_MainLoopWait_hook). the game waits here, unless the step already took longer than a refresh, and again
 *        at the end of the loop. skipping this wait runs the next game step one refresh later instead of two
 */
void GameSpeed_MainLoopWait(void)
{
    static u8 sStepPhase = 0;
    u32 speed = GameSpeed_FromOptions(Save_PlayerData_GetOptionsAddr(SaveBlock2_get()));
    BOOL skip;

    if (speed == GAME_SPEED_X2) {
        skip = TRUE;
    } else if (speed == GAME_SPEED_X1_5) {
        // 3 steps in 4 refreshes
        skip = sStepPhase != 0;
        sStepPhase = (sStepPhase + 1) % 3;
    } else {
        skip = FALSE;
    }

    if (!skip && gSystem.frameCounter == 0) {
        OS_WaitIrq(TRUE, 1); // OS_IE_V_BLANK
        gSystem.vblankCounter++;
    }
}

#endif // GAME_SPEED_OPTION
