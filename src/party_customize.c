#include "../include/types.h"
#include "../include/bag.h"
#include "../include/battle.h"
#include "../include/config.h"
#include "../include/message.h"
#include "../include/party_menu.h"
#include "../include/pokemon.h"
#include "../include/save.h"
#include "../include/task.h"
#include "../include/constants/ability.h"
#include "../include/constants/file.h"
#include "../include/constants/moves.h"
#include "../include/constants/generated/learnsets.h"
#include "../include/overlay.h"
#include "../include/field_extension_entries.h"
#include "../include/constants/file.h"

#ifdef PARTY_MENU_CUSTOMIZE

// "Customize" in the party menu. the menus themselves are in the field extension (src/field/party_customize_menu.c);
// this keeps what has to stay loaded: the move relearner's eligible-move list (the game's own relearner uses it too),
// the start menu tasks that open the move relearner and the naming screen between party menu visits, and loading the
// field extension while the party menu is open

typedef struct MoveRelearnerArgs {
    struct PartyPokemon *mon;
    void *profile;
    void *options;
    void *menuInputStatePtr;
    u16 *eligibleMoves;
    u8 filler_14[5];
    u8 type;
    u8 padding_1A[2];
} MoveRelearnerArgs;

#define MOVE_RELEARNER_TYPE_RELEARN 1

// see arm9 0x020830D8
typedef struct NamingScreenArgs {
    int type;
    int species;
    int form;
    int maxLen;
    int gender;
    int outcome;
    String *textInput;
    u16 nameInputFlat[20];
    u8 filler_44[8];
    void *options;
    void *menuInputStatePtr;
} NamingScreenArgs;

#define NAMING_SCREEN_TYPE_POKEMON 1
#define POKEMON_NICKNAME_LENGTH    10
#define NAMING_SCREEN_APP_TEMPLATE ((const void *)0x02102610)
#define FIELD_MENU_INPUT_STATE(fieldSystem) ((void *)((u8 *)(fieldSystem) + 0x10C))

typedef struct StartMenuTaskData {
    u8 filler_000[0x354];
    TaskFunc exitTaskFunc;
    u8 filler_358[0x370 - 0x358];
    u8 fieldMoveCheckData[0x380 - 0x370];
    void *exitTaskEnvironment;
    void *exitTaskEnvironment2;
} StartMenuTaskData;

MoveRelearnerArgs *LONG_CALL MoveRelearner_New(int heapId);
void LONG_CALL MoveRelearner_LaunchApp(FieldSystem *fieldSystem, MoveRelearnerArgs *args);
void *LONG_CALL PartyMenu_LaunchApp_Unk1(FieldSystem *fieldSystem, void *fieldMoveCheckData, u8 partySlot);
void LONG_CALL StartMenu_SetExitTaskFunc(StartMenuTaskData *startMenu, TaskFunc func);
BOOL LONG_CALL Task_StartMenu_HandleReturn_Pokemon(TaskManager *taskManager);
void *LONG_CALL Save_PlayerData_GetOptionsAddr(void *saveData);
void LONG_CALL FieldSystem_LaunchApplication(FieldSystem *fieldSystem, const void *template, void *args);
NamingScreenArgs *LONG_CALL NamingScreen_CreateArgs(int heapId, int type, int species, int maxLen, void *options, void *menuInputStatePtr);
void LONG_CALL NamingScreen_DeleteArgs(NamingScreenArgs *args);
BOOL LONG_CALL PokeListProc_Init(void *proc, int *seq);

/**
 * @brief the move relearner's list of moves a Pokémon can relearn: level-up moves at or below its level that it does
 *        not know, ending with LEVEL_UP_LEARNSET_END. replaces the vanilla version, which reads the vanilla learnset
 *        format instead of hg-engine's
 * @see   arm9 0x0209176C, pret/pokeheartgold MoveRelearner_GetEligibleLevelUpMoves
 */
u16 *LONG_CALL MoveRelearner_GetEligibleLevelUpMoves(struct PartyPokemon *pp, int heapId)
{
    u32 species = GetMonData(pp, MON_DATA_SPECIES, NULL);
    u32 form = GetMonData(pp, MON_DATA_FORM, NULL);
    u32 level = GetMonData(pp, MON_DATA_LEVEL, NULL);
    u16 known[MAX_MON_MOVES];
    for (int i = 0; i < MAX_MON_MOVES; i++) {
        known[i] = GetMonData(pp, MON_DATA_MOVE1 + i, NULL);
    }

    u32 *learnset = sys_AllocMemory(heapId, MAX_LEVELUP_MOVES * sizeof(u32));
    u16 *moves = sys_AllocMemory(heapId, (MAX_LEVELUP_MOVES + 1) * sizeof(u16));
    LoadLevelUpLearnset_HandleAlternateForm(species, form, learnset);

    int count = 0;
    for (int i = 0; i < MAX_LEVELUP_MOVES; i++) {
        u16 move = LEVEL_UP_LEARNSET_MOVE(learnset[i]);
        if (move == LEVEL_UP_LEARNSET_END) {
            break;
        }
        if (move == MOVE_NONE || LEVEL_UP_LEARNSET_LEVEL(learnset[i]) > level) {
            continue;
        }
        BOOL skip = FALSE;
        for (int j = 0; j < MAX_MON_MOVES && !skip; j++) {
            skip = known[j] == move;
        }
        for (int j = 0; j < count && !skip; j++) {
            skip = moves[j] == move;
        }
        if (!skip) {
            moves[count++] = move;
        }
    }
    moves[count] = LEVEL_UP_LEARNSET_END;
    sys_FreeMemoryEz(learnset);
    return moves;
}


static BOOL Task_StartMenu_ReturnFromMoveRelearner(TaskManager *taskManager);
static BOOL Task_StartMenu_ReturnFromNamingScreen(TaskManager *taskManager);
static void StartMenu_OpenNamingScreen(FieldSystem *fieldSystem, StartMenuTaskData *startMenu, u8 slot);

/**
 * @brief wraps the start menu's handling of the party menu closing, to open the move relearner or the naming screen
 *        from Customize
 * @see   arm9 0x0203CA9C Task_StartMenu_HandleReturn_Pokemon
 */
BOOL LONG_CALL Task_StartMenu_HandleReturn_Pokemon_Extend(TaskManager *taskManager)
{
    FieldSystem *fieldSystem = taskManager->fieldSystem;
    StartMenuTaskData *startMenu = taskManager->env;
    PartyMenuArgs *partyMenuArgs = startMenu->exitTaskEnvironment;
    u32 action = partyMenuArgs->selectedAction;

    if (action != PARTY_MENU_ACTION_RETURN_RELEARN_MOVES && action != PARTY_MENU_ACTION_RETURN_NICKNAME) {
        return Task_StartMenu_HandleReturn_Pokemon(taskManager);
    }

    u8 slot = partyMenuArgs->partySlot;
    sys_FreeMemoryEz(partyMenuArgs);

    if (action == PARTY_MENU_ACTION_RETURN_NICKNAME) {
        StartMenu_OpenNamingScreen(fieldSystem, startMenu, slot);
        return FALSE;
    }

    struct PartyPokemon *pp = Party_GetMonByIndex(SaveData_GetPlayerPartyPtr(fieldSystem->savedata), slot);
    MoveRelearnerArgs *relearner = MoveRelearner_New(HEAPID_WORLD);
    relearner->mon = pp;
    relearner->profile = Sav2_PlayerData_GetProfileAddr(fieldSystem->savedata);
    relearner->options = Save_PlayerData_GetOptionsAddr(fieldSystem->savedata);
    relearner->eligibleMoves = MoveRelearner_GetEligibleLevelUpMoves(pp, HEAPID_WORLD);
    relearner->type = MOVE_RELEARNER_TYPE_RELEARN;
    // the list is freed after the relearner closes, so it is valid whenever the relearner reads it
    MoveRelearner_LaunchApp(fieldSystem, relearner);

    u8 *slotStore = sys_AllocMemory(HEAPID_WORLD, sizeof(u8));
    *slotStore = slot;
    startMenu->exitTaskEnvironment = relearner;
    startMenu->exitTaskEnvironment2 = slotStore;
    StartMenu_SetExitTaskFunc(startMenu, Task_StartMenu_ReturnFromMoveRelearner);
    return FALSE;
}

/**
 * @brief after the move relearner closes, reopen the party menu on the same Pokémon
 */
static BOOL Task_StartMenu_ReturnFromMoveRelearner(TaskManager *taskManager)
{
    FieldSystem *fieldSystem = taskManager->fieldSystem;
    StartMenuTaskData *startMenu = taskManager->env;
    u8 *slotStore = startMenu->exitTaskEnvironment2;
    u8 slot = *slotStore;
    MoveRelearnerArgs *relearner = startMenu->exitTaskEnvironment;

    sys_FreeMemoryEz(relearner->eligibleMoves);
    sys_FreeMemoryEz(relearner);
    sys_FreeMemoryEz(slotStore);
    startMenu->exitTaskEnvironment2 = NULL;
    startMenu->exitTaskEnvironment = PartyMenu_LaunchApp_Unk1(fieldSystem, startMenu->fieldMoveCheckData, slot);
    StartMenu_SetExitTaskFunc(startMenu, Task_StartMenu_HandleReturn_Pokemon_Extend);
    return FALSE;
}

/**
 * @brief open the naming screen for a party Pokémon, set up like the Name Rater's
 * @see   arm9 0x0203F6E0 CallTask_NamingScreen
 */
static void StartMenu_OpenNamingScreen(FieldSystem *fieldSystem, StartMenuTaskData *startMenu, u8 slot)
{
    struct PartyPokemon *pp = Party_GetMonByIndex(SaveData_GetPlayerPartyPtr(fieldSystem->savedata), slot);
    NamingScreenArgs *naming = NamingScreen_CreateArgs(HEAPID_WORLD, NAMING_SCREEN_TYPE_POKEMON, GetMonData(pp, MON_DATA_SPECIES, NULL),
        POKEMON_NICKNAME_LENGTH, Save_PlayerData_GetOptionsAddr(fieldSystem->savedata), FIELD_MENU_INPUT_STATE(fieldSystem));
    naming->gender = GetMonData(pp, MON_DATA_GENDER, NULL);
    naming->form = GetMonData(pp, MON_DATA_FORM, NULL);
    FieldSystem_LaunchApplication(fieldSystem, NAMING_SCREEN_APP_TEMPLATE, naming);

    u8 *slotStore = sys_AllocMemory(HEAPID_WORLD, sizeof(u8));
    *slotStore = slot;
    startMenu->exitTaskEnvironment = naming;
    startMenu->exitTaskEnvironment2 = slotStore;
    StartMenu_SetExitTaskFunc(startMenu, Task_StartMenu_ReturnFromNamingScreen);
}

/**
 * @brief after the naming screen closes, give the Pokémon the name entered, then reopen the party menu on it.
 *        an empty name or the same name leaves it alone, as with the Name Rater
 * @see   arm9 0x0203F5DA, 0x0203F6A2
 */
static BOOL Task_StartMenu_ReturnFromNamingScreen(TaskManager *taskManager)
{
    FieldSystem *fieldSystem = taskManager->fieldSystem;
    StartMenuTaskData *startMenu = taskManager->env;
    u8 *slotStore = startMenu->exitTaskEnvironment2;
    u8 slot = *slotStore;
    NamingScreenArgs *naming = startMenu->exitTaskEnvironment;
    struct PartyPokemon *pp = Party_GetMonByIndex(SaveData_GetPlayerPartyPtr(fieldSystem->savedata), slot);

    if (naming->nameInputFlat[0] != 0xFFFF) {
        u16 oldName[POKEMON_NICKNAME_LENGTH + 1];
        GetMonData(pp, MON_DATA_NICKNAME, oldName);
        BOOL same = TRUE;
        for (int i = 0; i <= POKEMON_NICKNAME_LENGTH; i++) {
            if (naming->nameInputFlat[i] != oldName[i]) {
                same = FALSE;
                break;
            }
            if (oldName[i] == 0xFFFF) {
                break;
            }
        }
        if (!same) {
            SetMonData(pp, MON_DATA_NICKNAME_2, naming->nameInputFlat);
        }
    }

    NamingScreen_DeleteArgs(naming);
    sys_FreeMemoryEz(slotStore);
    startMenu->exitTaskEnvironment2 = NULL;
    startMenu->exitTaskEnvironment = PartyMenu_LaunchApp_Unk1(fieldSystem, startMenu->fieldMoveCheckData, slot);
    StartMenu_SetExitTaskFunc(startMenu, Task_StartMenu_HandleReturn_Pokemon_Extend);
    return FALSE;
}

// whether PokeListProc_Init_Extend loaded the field extension, so it is unloaded again when the party menu closes
static BOOL sLoadedFieldExtension;

/**
 * @brief whether the CUSTOMIZE menus can be used: the field extension, which holds them, is loaded
 */
BOOL PartyCustomize_IsAvailable(void)
{
    return FieldExtensionEntries_Get() != NULL;
}

/**
 * @brief the party menu's init: load the field extension first, for the CUSTOMIZE menus. the field is shut down while
 *        the party menu runs, so its extension is not loaded unless something else kept it
 * @see   arm9 0x02078E30 PokeListProc_Init
 */
BOOL LONG_CALL PokeListProc_Init_Extend(void *proc, int *seq)
{
    if (!sLoadedFieldExtension && !IsOverlayLoaded(OVERLAY_FIELD_EXTENSION)) {
        sLoadedFieldExtension = HandleLoadOverlay(OVERLAY_FIELD_EXTENSION, 2);
    }
    return PokeListProc_Init(proc, seq);
}

/**
 * @brief when the party menu has closed: unload the field extension if PokeListProc_Init_Extend loaded it
 */
void PartyCustomize_ReleaseFieldExtension(void)
{
    if (sLoadedFieldExtension) {
        UnloadOverlayByID(OVERLAY_FIELD_EXTENSION);
        sLoadedFieldExtension = FALSE;
    }
}

/**
 * @brief open the field party menu's context menu with the CUSTOMIZE button, or the normal one if CUSTOMIZE is
 *        unavailable (no button was added for it then; see sub_0207B0B0)
 */
void PartyMenu_OpenContextMenuWithCustomize(struct PartyMenu *partyMenu, u8 *items, u8 numItems)
{
    const struct FieldExtensionEntries *entries = FieldExtensionEntries_Get();
    if (entries != NULL) {
        entries->partyCustomizeOpenContextMenu(partyMenu, items, numItems);
    } else {
        PartyMenu_OpenContextMenu(partyMenu, items, numItems);
    }
}

/**
 * @brief whether a context menu button belongs to the customize menus, which use plain white text
 */
BOOL PartyMenu_IsCustomizeButton(u32 action)
{
    const struct FieldExtensionEntries *entries = FieldExtensionEntries_Get();
    return entries != NULL && entries->partyCustomizeIsCustomizeButton(action);
}

#endif // PARTY_MENU_CUSTOMIZE
