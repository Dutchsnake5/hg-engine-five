#include "config.h"
#include "types.h"

#include "field_extension_entries.h"

// the field extension's entry table; see include/field_extension_entries.h. the linker script puts .init first, so
// this sits at the start of the field extension

#ifdef PARTY_MENU_CUSTOMIZE
void PartyCustomizeMenu_OpenContextMenu(struct PartyMenu *partyMenu, u8 *items, u8 numItems);
BOOL PartyCustomizeMenu_IsCustomizeButton(u32 action);
#endif

#ifdef START_MENU_REMOTE_PC
void StartMenuPCField_CreateIcon(void *work);
void StartMenuPCField_DestroyIcon(void *work);
void StartMenuPCField_CheckTouch(void *work);
void StartMenuPCField_UpdateVisuals(void *work, BOOL menuOpen);
void StartMenuPCField_HandleDpad(void *work);
void StartMenuPCField_BounceSelectedIcon(void *work);
BOOL StartMenuPCField_IsSelected(void);
BOOL StartMenuPCField_HandleKey(void *taskManager, void *fieldSystem, void *startMenu);
BOOL StartMenuPCField_HandleTouch(void *taskManager, void *fieldSystem, void *startMenu);
#endif

const struct FieldExtensionEntries gFieldExtensionEntries __attribute__((section(".init"))) = {
    .magic = FIELD_EXTENSION_ENTRIES_MAGIC,
#ifdef PARTY_MENU_CUSTOMIZE
    .partyCustomizeOpenContextMenu = PartyCustomizeMenu_OpenContextMenu,
    .partyCustomizeIsCustomizeButton = PartyCustomizeMenu_IsCustomizeButton,
#endif
#ifdef START_MENU_REMOTE_PC
    .pcCreateIcon = StartMenuPCField_CreateIcon,
    .pcDestroyIcon = StartMenuPCField_DestroyIcon,
    .pcCheckTouch = StartMenuPCField_CheckTouch,
    .pcUpdateVisuals = StartMenuPCField_UpdateVisuals,
    .pcHandleDpad = StartMenuPCField_HandleDpad,
    .pcBounceSelectedIcon = StartMenuPCField_BounceSelectedIcon,
    .pcIsSelected = StartMenuPCField_IsSelected,
    .pcHandleKey = StartMenuPCField_HandleKey,
    .pcHandleTouch = StartMenuPCField_HandleTouch,
#endif
};
