#ifndef FIELD_EXTENSION_ENTRIES_H
#define FIELD_EXTENSION_ENTRIES_H

#include "types.h"

struct PartyMenu;
struct TaskManager;
struct FieldSystem;

// Features whose code lives in the field extension (overlay 131) instead of the always-loaded code. The always-loaded
// code links before the field extension, so it cannot call those functions by name. It calls them through this table,
// which src/field/field_extension_entries.c puts at the very start of the field extension, after checking with
// FieldExtensionEntries_Get that the field extension is loaded. Every member is always present, NULL when its feature
// is off, so the layout does not depend on config.h.
struct FieldExtensionEntries {
    u32 magic;

    // party menu CUSTOMIZE (PARTY_MENU_CUSTOMIZE): src/field/party_customize_menu.c
    void (*partyCustomizeOpenContextMenu)(struct PartyMenu *partyMenu, u8 *items, u8 numItems);
    BOOL (*partyCustomizeIsCustomizeButton)(u32 action);

    // the start menu's PC icon (START_MENU_REMOTE_PC): src/field/start_menu_pc_field.c
    void (*pcCreateIcon)(void *work);
    void (*pcDestroyIcon)(void *work);
    void (*pcCheckTouch)(void *work);
    void (*pcUpdateVisuals)(void *work, BOOL menuOpen);
    void (*pcHandleDpad)(void *work);
    void (*pcBounceSelectedIcon)(void *work);
    BOOL (*pcIsSelected)(void);
    BOOL (*pcHandleKey)(void *taskManager, void *fieldSystem, void *startMenu);
    BOOL (*pcHandleTouch)(void *taskManager, void *fieldSystem, void *startMenu);
};

#define FIELD_EXTENSION_ENTRIES_MAGIC 0x45444C46 // "FLDE"
#define FIELD_EXTENSION_ENTRIES_ADDR  0x023C8000 // ORIGIN in src/field/linker.ld

/**
 * @brief the field extension's entry table, or NULL when the field extension is not loaded
 * @see   src/overlay.c
 */
const struct FieldExtensionEntries *FieldExtensionEntries_Get(void);

#endif // FIELD_EXTENSION_ENTRIES_H
