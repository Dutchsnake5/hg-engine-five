#include "../include/types.h"
#include "../include/config.h"
#include "../include/system.h"
#include "../include/field_extension_entries.h"

// The start menu's PC icon (see src/field/start_menu_pc_field.c, in the field extension). The hooks and trampolines
// (hooks, asm/start_menu_pc_hooks.s) land here, in the always-loaded code, and this passes them on through the field
// extension's entry table. Overlay 27 only runs on the field, where the field extension is loaded; if it ever is not,
// there is no PC icon, and the two hooks that replace overlay 27 code fall back to the game's own behavior.

#ifdef START_MENU_REMOTE_PC

void LONG_CALL PlaySE(u16 se);
void LONG_CALL Sprite_SetAnimCtrlSeq(void *sprite, int seq);
void LONG_CALL Sprite_TryChangeAnimSeq(void *sprite, int seq);

// overlay 27's work data and cursor helpers
#define OV27_FIELD_SYSTEM(work)      (*(void **)((u8 *)(work) + 0x10))
#define OV27_CURSOR_SLOT(work)       (*(int *)((u8 *)(work) + 0x14))
#define OV27_ICON_SPRITE(work, slot) (((void **)((u8 *)(work) + 0x390))[slot])
#define OV27_SLOTS(work)             ((u8 *)(work) + 0x470)
#define FIELD_START_MENU_CURSOR(fsys) (*(u8 *)((u8 *)(fsys) + 0xD3))
#define ov27_FindSlotInDirection ((int (*)(int slot, int direction, void *slots))(0x0225B360 | 1))
#define ov27_HighlightSlot       ((void (*)(void *work, int slot))(0x0225B398 | 1))
#define ov27_SlotToMenuIndex     ((int (*)(void *work, int slot))(0x0225C170 | 1))
#define SEQ_SE_DP_CURSOR 0x5E0
#define ANIM_SEQ_PICKED  2

void StartMenuPC_CreateIcon(void *work)
{
    const struct FieldExtensionEntries *entries = FieldExtensionEntries_Get();
    if (entries != NULL) {
        entries->pcCreateIcon(work);
    }
}

void StartMenuPC_DestroyIcon(void *work)
{
    const struct FieldExtensionEntries *entries = FieldExtensionEntries_Get();
    if (entries != NULL) {
        entries->pcDestroyIcon(work);
    }
}

void StartMenuPC_CheckTouch(void *work)
{
    const struct FieldExtensionEntries *entries = FieldExtensionEntries_Get();
    if (entries != NULL) {
        entries->pcCheckTouch(work);
    }
}

void StartMenuPC_UpdateVisuals(void *work, BOOL menuOpen)
{
    const struct FieldExtensionEntries *entries = FieldExtensionEntries_Get();
    if (entries != NULL) {
        entries->pcUpdateVisuals(work, menuOpen);
    }
}

/**
 * @brief replaces overlay 27's d-pad cursor movement
 * @see   overlay 27 0x0225B404
 */
void StartMenuPC_HandleDpad(void *work)
{
    const struct FieldExtensionEntries *entries = FieldExtensionEntries_Get();
    if (entries != NULL) {
        entries->pcHandleDpad(work);
        return;
    }
    // overlay 27's own movement
    int keys = *(int *)((u8 *)&gSystem + 0x48);
    int direction = (keys & 0x40) ? 0 : (keys & 0x80) ? 1 : (keys & 0x20) ? 2 : (keys & 0x10) ? 3 : -1;
    if (direction < 0) {
        return;
    }
    int slot = OV27_CURSOR_SLOT(work);
    int next = ov27_FindSlotInDirection(slot, direction, OV27_SLOTS(work));
    if (next < 0 || next == slot) {
        return;
    }
    OV27_CURSOR_SLOT(work) = next;
    PlaySE(SEQ_SE_DP_CURSOR);
    Sprite_SetAnimCtrlSeq(OV27_ICON_SPRITE(work, next), direction <= 1 ? 3 : 1);
    ov27_HighlightSlot(work, next);
    FIELD_START_MENU_CURSOR(OV27_FIELD_SYSTEM(work)) = ov27_SlotToMenuIndex(work, next);
}

/**
 * @brief replaces overlay 27's bounce of the picked icon
 * @see   overlay 27 0x0225A36E
 */
void StartMenuPC_BounceSelectedIcon(void *work)
{
    const struct FieldExtensionEntries *entries = FieldExtensionEntries_Get();
    if (entries != NULL) {
        entries->pcBounceSelectedIcon(work);
        return;
    }
    Sprite_TryChangeAnimSeq(OV27_ICON_SPRITE(work, OV27_CURSOR_SLOT(work)), ANIM_SEQ_PICKED);
}

BOOL StartMenuPC_IsSelected(void)
{
    const struct FieldExtensionEntries *entries = FieldExtensionEntries_Get();
    return entries != NULL && entries->pcIsSelected();
}

// only called while the PC icon is selected or was touched, so the field extension is loaded
BOOL StartMenuPC_HandleKey(void *taskManager, void *fieldSystem, void *startMenu)
{
    return FieldExtensionEntries_Get()->pcHandleKey(taskManager, fieldSystem, startMenu);
}

BOOL StartMenuPC_HandleTouch(void *taskManager, void *fieldSystem, void *startMenu)
{
    return FieldExtensionEntries_Get()->pcHandleTouch(taskManager, fieldSystem, startMenu);
}

#endif // START_MENU_REMOTE_PC
