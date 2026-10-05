#include "types.h"
#include "bag.h"
#include "battle.h"
#include "config.h"
#include "message.h"
#include "party_menu.h"
#include "pokemon.h"
#include "save.h"
#include "task.h"
#include "constants/ability.h"
#include "constants/file.h"
#include "constants/moves.h"
#include "constants/generated/learnsets.h"
#include "overlay.h"

#ifdef PARTY_MENU_CUSTOMIZE

// the "Customize" menus of the party menu: change a Pokémon's nature, pick any of its species' abilities, relearn moves,
// or give it a new nickname like the Name Rater does. they live in the field extension instead of the always-loaded
// code; src/party_customize.c loads the field extension while the party menu is open and calls in through the field
// extension's entry table (include/field_extension_entries.h)

// party menu text (data/text/300.txt)
#define MSG_CUSTOMIZE           227
#define MSG_NATURE              228
#define MSG_ABILITY             229
#define MSG_MOVES               230
#define MSG_PROMPT_CHANGE_WHAT  231
#define MSG_PROMPT_RAISE        232
#define MSG_PROMPT_LOWER        233
#define MSG_PROMPT_NATURE       234
#define MSG_PROMPT_ABILITY      235
#define MSG_STAT_ATTACK         236 // ATTACK, DEFENSE, SPEED, SP. ATK, SP. DEF follow in nature stat order
#define MSG_STAT_NEUTRAL        241
#define MSG_BUFFERED_NAME       242
#define MSG_NATURE_CHANGED      243
#define MSG_ABILITY_CHANGED     244
#define MSG_NO_MOVES_TO_RELEARN 245
#define MSG_ONLY_ONE_ABILITY    246
#define MSG_NICKNAME            247
#define MSG_NAMED_BY_OTHER_TRAINER 248

// natures are ordered raised stat * 5 + lowered stat, with stats in this order
enum {
    NATURE_STAT_ATTACK,
    NATURE_STAT_DEFENSE,
    NATURE_STAT_SPEED,
    NATURE_STAT_SP_ATTACK,
    NATURE_STAT_SP_DEFENSE,
    NATURE_STAT_COUNT,
    NATURE_STAT_NEUTRAL = NATURE_STAT_COUNT,
};

// the button in this list position is QUIT, so every list keeps QUIT in the same place as the main menu
#define QUIT_BUTTON_POS 3
#define NO_CHOICE 0xFF

void LONG_CALL BufferAbilityName(MessageFormat *msgFmt, u32 fieldno, u32 ability);
void LONG_CALL BufferNatureName(MessageFormat *msgFmt, u32 fieldno, u32 nature);

static void PartyMonContextMenuAction_Customize(struct PartyMenu *partyMenu, int *pState);
static void PartyMonContextMenuAction_Nature(struct PartyMenu *partyMenu, int *pState);
static void PartyMonContextMenuAction_NatureRaise(struct PartyMenu *partyMenu, int *pState);
static void PartyMonContextMenuAction_NatureLower(struct PartyMenu *partyMenu, int *pState);
static void PartyMonContextMenuAction_NatureNeutral(struct PartyMenu *partyMenu, int *pState);
static void PartyMonContextMenuAction_Ability(struct PartyMenu *partyMenu, int *pState);
static void PartyMonContextMenuAction_SetAbility(struct PartyMenu *partyMenu, int *pState);
static void PartyMonContextMenuAction_Moves(struct PartyMenu *partyMenu, int *pState);
static void PartyMonContextMenuAction_Nickname(struct PartyMenu *partyMenu, int *pState);

// choices made in earlier steps of a submenu: list position -> value
static u8 sListChoices[MAX_BUTTONS_IN_PARTY_MENU];
static u8 sRaisedStat;

static struct PartyPokemon *PartyMenu_GetSelectedMon(struct PartyMenu *partyMenu)
{
    return Party_GetMonByIndex(partyMenu->args->party, partyMenu->partyMonIndex);
}

/**
 * @brief the distinct abilities a Pokémon's species and form can have: ability 1, ability 2 and the hidden ability
 * @return the number of abilities written to abilities[]
 */
static int GetSelectableAbilities(struct PartyPokemon *pp, u16 *abilities, u8 *slots)
{
    u32 species = GetMonData(pp, MON_DATA_SPECIES, NULL);
    u32 form = GetMonData(pp, MON_DATA_FORM, NULL);
    u16 options[3] = {
        PokeFormNoPersonalParaGet(species, form, PERSONAL_ABILITY_1),
        PokeFormNoPersonalParaGet(species, form, PERSONAL_ABILITY_2),
        GetMonHiddenAbility(species, form),
    };
    int count = 0;
    for (int slot = 0; slot < 3; slot++) {
        BOOL duplicate = options[slot] == ABILITY_NONE;
        for (int j = 0; j < count && !duplicate; j++) {
            duplicate = abilities[j] == options[slot];
        }
        if (!duplicate) {
            abilities[count] = options[slot];
            slots[count] = slot;
            count++;
        }
    }
    return count;
}

/**
 * @brief give a Pokémon the ability in a species ability slot (0 = ability 1, 1 = ability 2, 2 = hidden ability)
 *        by setting the same bits that the Ability Capsule and Ability Patch use
 */
static void SetMonAbilitySlot(struct PartyPokemon *pp, u8 slot)
{
    u8 reserved113 = GetMonData(pp, MON_DATA_RESERVED_113, NULL);
    u16 reserved114 = GetMonData(pp, MON_DATA_RESERVED_114, NULL);
    u32 personality = GetMonData(pp, MON_DATA_PERSONALITY, NULL);

    if (slot == 2) {
        reserved113 |= DUMMY_P2_1_HIDDEN_ABILITY_MASK;
    } else {
        reserved113 &= ~DUMMY_P2_1_HIDDEN_ABILITY_MASK;
        // the personality's low bit picks ability 1 or 2, and the swap bit flips that choice
        reserved114 &= ~DUMMY_P2_2_CHANGE_ABILITY_SLOT;
        if ((personality & 1) != slot) {
            reserved114 |= DUMMY_P2_2_CHANGE_ABILITY_SLOT;
        }
    }
    SetMonData(pp, MON_DATA_RESERVED_113, &reserved113);
    SetMonData(pp, MON_DATA_RESERVED_114, &reserved114);
    ResetPartyPokemonAbility(pp);
}

/**
 * @brief add a button whose text is a party menu message, or a buffered name when msgId is MSG_BUFFERED_NAME
 */
static void AddButton(struct PartyMenu *partyMenu, int msgId, void (*action)(struct PartyMenu *, int *))
{
    String *template = NewString_ReadMsgData(partyMenu->msgData, msgId);
    StringExpandPlaceholders(partyMenu->msgFormat, partyMenu->unformattedStrBuf, template);
    ListMenuItems_AddItem(partyMenu->listMenuItems, partyMenu->unformattedStrBuf, (u32)action);
    String_Delete(template);
}

static void AddQuitButton(struct PartyMenu *partyMenu)
{
    ListMenuItems_AddItem(partyMenu->listMenuItems, partyMenu->contextMenuStrings[PARTY_MON_CONTEXT_MENU_QUIT], LIST_CANCEL);
}

/**
 * @brief replace the open context menu with a new list of buttons, laid out like the main context menu
 */
static void BeginList(struct PartyMenu *partyMenu, int numItems, int promptMsgId)
{
    PartyMenu_DeleteContextMenuAndList(partyMenu);
    PartyMenu_PrintMessageOnWindow33(partyMenu, promptMsgId, FALSE);
    partyMenu->listMenuItems = ListMenuItems_New(numItems, HEAP_ID_PARTY_MENU);
    for (int i = 0; i < MAX_BUTTONS_IN_PARTY_MENU; i++) {
        sListChoices[i] = NO_CHOICE;
    }
}

static void ShowList(struct PartyMenu *partyMenu, int numItems, int *pState)
{
    struct PartyMenuContextMenu contextMenu;
    contextMenu.items = partyMenu->listMenuItems;
    contextMenu.window = &partyMenu->levelUpStatsWindow[0];
    contextMenu.unk_08 = 0;
    contextMenu.unk_09 = 1;
    contextMenu.numItems = numItems;
    contextMenu.unk_0B_0 = 0;
    contextMenu.unk_0B_4 = 0;
    contextMenu.scrollEnabled = numItems >= 4;
    sub_0207E54C(partyMenu, numItems, 0, 0);
    partyMenu->contextMenuCursor = PartyMenu_CreateContextMenuCursor(partyMenu, &contextMenu, 0, HEAP_ID_PARTY_MENU, 0);
    *pState = PARTY_MENU_STATE_HANDLE_CONTEXT_MENU_INPUT;
}

/**
 * @brief add the choice buttons, putting QUIT in its usual place; choices[] are stored by list position
 */
static int AddChoiceButtons(struct PartyMenu *partyMenu, const u8 *choices, int count, int (*msgFor)(struct PartyMenu *, u8), void (*action)(struct PartyMenu *, int *))
{
    int pos = 0;
    for (int i = 0; i < count; i++) {
        if (pos == QUIT_BUTTON_POS) {
            AddQuitButton(partyMenu);
            pos++;
        }
        AddButton(partyMenu, msgFor(partyMenu, choices[i]), action);
        sListChoices[pos++] = choices[i];
    }
    if (pos <= QUIT_BUTTON_POS) {
        AddQuitButton(partyMenu);
        pos++;
    }
    return pos;
}

static u8 SelectedChoice(struct PartyMenu *partyMenu)
{
    u8 sel = partyMenu->contextMenuButtonAnim.selection;
    return sel < MAX_BUTTONS_IN_PARTY_MENU ? sListChoices[sel] : NO_CHOICE;
}

/**
 * @brief print a message about the selected Pokémon, then go back to choosing a Pokémon
 */
static void FinishWithMessage(struct PartyMenu *partyMenu, int msgId, int *pState)
{
    ClearFrameAndWindow2(&partyMenu->windows[PARTY_MENU_WINDOW_ID_33], TRUE);
    PartyMenu_DeleteContextMenuAndList(partyMenu);
    PartyMenu_DisableMainScreenBlend_AfterYesNo();
    String *template = NewString_ReadMsgData(partyMenu->msgData, msgId);
    BufferBoxMonNickname(partyMenu->msgFormat, 0, &PartyMenu_GetSelectedMon(partyMenu)->box);
    StringExpandPlaceholders(partyMenu->msgFormat, partyMenu->formattedStrBuf, template);
    String_Delete(template);
    PartyMenu_PrintMessageOnWindow34(partyMenu, -1, TRUE);
    *pState = PARTY_MENU_STATE_PRINT_TAKE_ITEM_MESSAGE;
}

static void PartyMonContextMenuAction_Customize(struct PartyMenu *partyMenu, int *pState)
{
    BeginList(partyMenu, 5, MSG_PROMPT_CHANGE_WHAT);
    AddButton(partyMenu, MSG_NATURE, PartyMonContextMenuAction_Nature);
    AddButton(partyMenu, MSG_ABILITY, PartyMonContextMenuAction_Ability);
    AddButton(partyMenu, MSG_MOVES, PartyMonContextMenuAction_Moves);
    AddQuitButton(partyMenu); // QUIT_BUTTON_POS, the small corner button
    AddButton(partyMenu, MSG_NICKNAME, PartyMonContextMenuAction_Nickname);
    ShowList(partyMenu, 5, pState);
}

static int MsgForStat(struct PartyMenu *partyMenu UNUSED, u8 stat)
{
    return stat == NATURE_STAT_NEUTRAL ? MSG_STAT_NEUTRAL : MSG_STAT_ATTACK + stat;
}

static int MsgForNature(struct PartyMenu *partyMenu, u8 nature)
{
    BufferNatureName(partyMenu->msgFormat, 0, nature);
    return MSG_BUFFERED_NAME;
}

static void PartyMonContextMenuAction_Nature(struct PartyMenu *partyMenu, int *pState)
{
    static const u8 stats[] = { NATURE_STAT_ATTACK, NATURE_STAT_DEFENSE, NATURE_STAT_SPEED, NATURE_STAT_SP_ATTACK, NATURE_STAT_SP_DEFENSE, NATURE_STAT_NEUTRAL };
    BeginList(partyMenu, NELEMS(stats) + 1, MSG_PROMPT_RAISE);
    int n = AddChoiceButtons(partyMenu, stats, NELEMS(stats), MsgForStat, PartyMonContextMenuAction_NatureRaise);
    ShowList(partyMenu, n, pState);
}

static void PartyMonContextMenuAction_NatureRaise(struct PartyMenu *partyMenu, int *pState)
{
    u8 raised = SelectedChoice(partyMenu);
    u8 choices[NATURE_STAT_COUNT];
    int count = 0;

    if (raised == NATURE_STAT_NEUTRAL) {
        // the neutral natures raise and lower the same stat
        for (u8 stat = 0; stat < NATURE_STAT_COUNT; stat++) {
            choices[count++] = stat * NATURE_STAT_COUNT + stat;
        }
        BeginList(partyMenu, count + 1, MSG_PROMPT_NATURE);
        int n = AddChoiceButtons(partyMenu, choices, count, MsgForNature, PartyMonContextMenuAction_NatureNeutral);
        ShowList(partyMenu, n, pState);
        return;
    }

    sRaisedStat = raised;
    for (u8 stat = 0; stat < NATURE_STAT_COUNT; stat++) {
        if (stat != raised) {
            choices[count++] = stat;
        }
    }
    BeginList(partyMenu, count + 1, MSG_PROMPT_LOWER);
    int n = AddChoiceButtons(partyMenu, choices, count, MsgForStat, PartyMonContextMenuAction_NatureLower);
    ShowList(partyMenu, n, pState);
}

static void SetNatureAndFinish(struct PartyMenu *partyMenu, u8 nature, int *pState)
{
    struct PartyPokemon *pp = PartyMenu_GetSelectedMon(partyMenu);
    SET_MON_NATURE_OVERRIDE(pp, nature);
    RecalcPartyPokemonStats(pp);
    BufferNatureName(partyMenu->msgFormat, 1, nature);
    FinishWithMessage(partyMenu, MSG_NATURE_CHANGED, pState);
}

static void PartyMonContextMenuAction_NatureLower(struct PartyMenu *partyMenu, int *pState)
{
    SetNatureAndFinish(partyMenu, sRaisedStat * NATURE_STAT_COUNT + SelectedChoice(partyMenu), pState);
}

static void PartyMonContextMenuAction_NatureNeutral(struct PartyMenu *partyMenu, int *pState)
{
    SetNatureAndFinish(partyMenu, SelectedChoice(partyMenu), pState);
}

static u16 sAbilityChoices[3];

static int MsgForAbilitySlot(struct PartyMenu *partyMenu, u8 slot)
{
    BufferAbilityName(partyMenu->msgFormat, 0, sAbilityChoices[slot]);
    return MSG_BUFFERED_NAME;
}

static void PartyMonContextMenuAction_Ability(struct PartyMenu *partyMenu, int *pState)
{
    u16 abilities[3];
    u8 slots[3];
    int count = GetSelectableAbilities(PartyMenu_GetSelectedMon(partyMenu), abilities, slots);
    if (count < 2) {
        FinishWithMessage(partyMenu, MSG_ONLY_ONE_ABILITY, pState);
        return;
    }
    for (int i = 0; i < 3; i++) {
        sAbilityChoices[i] = ABILITY_NONE;
    }
    for (int i = 0; i < count; i++) {
        sAbilityChoices[slots[i]] = abilities[i];
    }
    BeginList(partyMenu, count + 1, MSG_PROMPT_ABILITY);
    int n = AddChoiceButtons(partyMenu, slots, count, MsgForAbilitySlot, PartyMonContextMenuAction_SetAbility);
    ShowList(partyMenu, n, pState);
}

static void PartyMonContextMenuAction_SetAbility(struct PartyMenu *partyMenu, int *pState)
{
    u8 slot = SelectedChoice(partyMenu);
    struct PartyPokemon *pp = PartyMenu_GetSelectedMon(partyMenu);
    SetMonAbilitySlot(pp, slot);
    BufferAbilityName(partyMenu->msgFormat, 1, GetMonData(pp, MON_DATA_ABILITY, NULL));
    FinishWithMessage(partyMenu, MSG_ABILITY_CHANGED, pState);
}

static void PartyMonContextMenuAction_Moves(struct PartyMenu *partyMenu, int *pState)
{
    u16 *moves = MoveRelearner_GetEligibleLevelUpMoves(PartyMenu_GetSelectedMon(partyMenu), HEAP_ID_PARTY_MENU);
    BOOL any = moves[0] != LEVEL_UP_LEARNSET_END;
    sys_FreeMemoryEz(moves);
    if (!any) {
        FinishWithMessage(partyMenu, MSG_NO_MOVES_TO_RELEARN, pState);
        return;
    }
    // close the party menu; the start menu opens the move relearner and then reopens the party menu
    PartyMenu_DeleteContextMenuAndList(partyMenu);
    PartyMenu_DisableMainScreenBlend_AfterYesNo();
    partyMenu->args->partySlot = partyMenu->partyMonIndex;
    partyMenu->args->selectedAction = PARTY_MENU_ACTION_RETURN_RELEARN_MOVES;
    *pState = PARTY_MENU_STATE_BEGIN_EXIT;
}

/**
 * @brief like the Name Rater, only rename a Pokémon whose original trainer is the player
 */
static BOOL IsNamedByPlayer(struct PartyPokemon *pp, struct PlayerProfile *profile)
{
    u16 otName[8];
    if ((u32)GetMonData(pp, MON_DATA_OTID, NULL) != profile->id) {
        return FALSE;
    }
    GetMonData(pp, MON_DATA_OT_NAME, otName);
    for (int i = 0; i < 8; i++) {
        if (otName[i] != profile->name[i]) {
            return FALSE;
        }
        if (otName[i] == 0xFFFF) {
            break;
        }
    }
    return TRUE;
}

static void PartyMonContextMenuAction_Nickname(struct PartyMenu *partyMenu, int *pState)
{
    if (!IsNamedByPlayer(PartyMenu_GetSelectedMon(partyMenu), Sav2_PlayerData_GetProfileAddr(SaveBlock2_get()))) {
        FinishWithMessage(partyMenu, MSG_NAMED_BY_OTHER_TRAINER, pState);
        return;
    }
    // close the party menu; the start menu opens the naming screen and then reopens the party menu
    PartyMenu_DeleteContextMenuAndList(partyMenu);
    PartyMenu_DisableMainScreenBlend_AfterYesNo();
    partyMenu->args->partySlot = partyMenu->partyMonIndex;
    partyMenu->args->selectedAction = PARTY_MENU_ACTION_RETURN_NICKNAME;
    *pState = PARTY_MENU_STATE_BEGIN_EXIT;
}

/**
 * @brief whether a context menu button belongs to the customize menus, which use plain white text
 */
BOOL PartyCustomizeMenu_IsCustomizeButton(u32 action)
{
    return action == (u32)PartyMonContextMenuAction_Customize
        || action == (u32)PartyMonContextMenuAction_Nature
        || action == (u32)PartyMonContextMenuAction_NatureRaise
        || action == (u32)PartyMonContextMenuAction_NatureLower
        || action == (u32)PartyMonContextMenuAction_NatureNeutral
        || action == (u32)PartyMonContextMenuAction_Ability
        || action == (u32)PartyMonContextMenuAction_SetAbility
        || action == (u32)PartyMonContextMenuAction_Moves
        || action == (u32)PartyMonContextMenuAction_Nickname;
}

/**
 * @brief open the field party menu's context menu; like PartyMenu_OpenContextMenu, but buttons marked
 *        PARTY_MON_CONTEXT_MENU_CUSTOMIZE_MARKER become the CUSTOMIZE button
 * @see   pret/pokeheartgold PartyMenu_OpenContextMenu
 */
void PartyCustomizeMenu_OpenContextMenu(struct PartyMenu *partyMenu, u8 *items, u8 numItems)
{
    partyMenu->listMenuItems = ListMenuItems_New(numItems, HEAP_ID_PARTY_MENU);
    int numFieldMoves = 0;
    for (int i = 0; i < numItems; i++) {
        if (items[i] == PARTY_MON_CONTEXT_MENU_CUSTOMIZE_MARKER) {
            AddButton(partyMenu, MSG_CUSTOMIZE, PartyMonContextMenuAction_Customize);
        } else if (items[i] >= PARTY_MON_CONTEXT_MENU_FIELD_MOVES_BEGIN) {
            ListMenuItems_AddItem(partyMenu->listMenuItems, partyMenu->contextMenuStrings[PARTY_MON_CONTEXT_MENU_FIELD_MOVES_BEGIN + numFieldMoves], GetPartyMenuContextMenuActionFunc(items[i]));
            numFieldMoves++;
        } else {
            ListMenuItems_AddItem(partyMenu->listMenuItems, partyMenu->contextMenuStrings[items[i]], GetPartyMenuContextMenuActionFunc(items[i]));
        }
    }

    struct PartyMenuContextMenu contextMenu;
    contextMenu.items = partyMenu->listMenuItems;
    contextMenu.window = &partyMenu->levelUpStatsWindow[0];
    contextMenu.unk_08 = 0;
    contextMenu.unk_09 = 1;
    contextMenu.numItems = numItems;
    contextMenu.unk_0B_0 = 0;
    contextMenu.unk_0B_4 = 0;
    contextMenu.scrollEnabled = numItems >= 4;
    sub_0207E54C(partyMenu, numItems, 0, 0);
    partyMenu->contextMenuCursor = PartyMenu_CreateContextMenuCursor(partyMenu, &contextMenu, 0, HEAP_ID_PARTY_MENU, 0);
}

#endif // PARTY_MENU_CUSTOMIZE
