#include "types.h"
#include "config.h"
#include "pokemon.h"
#include "save.h"
#include "script.h"
#include "system.h"
#include "task.h"
#include "message.h"
#include "window.h"

// A PC icon in the empty bottom-right slot of the touch screen start menu (overlay 27). Touching it opens the
// Pokémon Storage System's menu (Withdraw / Deposit / Move Pokémon / Move Items) through script 2074.
// This lives in the field extension, which is loaded whenever overlay 27 runs; src/start_menu_pc.c, in the
// always-loaded code, receives the hooks and calls in through the field extension's entry table.

#ifdef START_MENU_REMOTE_PC

#define REMOTE_PC_SCRIPT     2074
#define REMOTE_PC_TOUCH_CODE 12 // fieldSystem->lastTouchMenuInput; 1-11 are used by the game

#define NARC_TOUCH_MENU      14 // a/0/1/4
#define PC_ICON_CHAR_FILE    78
#define ICON_PLTT_FILE       14
#define ICON_CELL_ANIM_ID    0x64 // overlay 27's shared icon cell and animation resources
#define PC_ICON_RES_ID       0x80
#define HEAP_ID_TOUCH_MENU   8
#define VRAM_SUB_OBJ         2
#define MSG_START_MENU_PC      37 // data/text/196.txt, the bank overlay 27 takes its icon labels from
#define PC_LABEL_WINDOW        7  // overlay 27 makes a label window for every slot, but never uses the 8th
#define LABEL_WINDOW_WIDTH_PX  0x48
#define LABEL_TEXT_COLOR       0x000E0200
#define FONT_SYSTEM            0
#define LABEL_TEXT_SPEED       0xFF // as overlay 27 prints its labels

// overlay 27's work data
#define OV27_FIELD_SYSTEM(work)     (*(FieldSystem **)((u8 *)(work) + 0x10))
#define OV27_TOUCH_INPUT(work)      (*(u16 **)((u8 *)(work) + 0x0C))
#define OV27_SPRITE_LIST(work)      (*(void **)((u8 *)(work) + 0x18))
#define OV27_RES_MANAGERS(work)     ((void **)((u8 *)(work) + 0x144))
#define OV27_MSG_DATA(work)         (*(MsgData **)((u8 *)(work) + 0x4A8))
#define OV27_LABEL_WINDOW(work, n)  ((void *)((u8 *)(work) + 0x3F0 + (n) * 0x10))
#define OV27_FLAGS(work)            (*(u32 *)((u8 *)(work) + 0x51C))
#define OV27_MENU_MODE(work)        ((OV27_FLAGS(work) >> 1) & 0xF) // row of the icon layout table; 0 is the normal field menu
#define OV27_BUSY(work)             ((OV27_FLAGS(work) >> 5) & 1)

// field system data
#define FIELD_PLAYER_AVATAR(fsys)        (*(void **)((u8 *)(fsys) + 0x40))
#define FIELD_LAST_TOUCH_MENU_INPUT(fsys) (*(u16 *)((u8 *)(fsys) + 0xD0))
#define FIELD_MENU_FLAGS(fsys)           (*(u8 *)((u8 *)(fsys) + 0xD2))
#define FIELD_MENU_INPUT_STATE(fsys)     ((void *)((u8 *)(fsys) + 0x10C))
#define FIELD_LAST_START_MENU_ACTION(fsys) (*(int *)((u8 *)(fsys) + 0xE0))

#define PAD_KEYS_DPAD 0xF0
#define OAM_MODE_NORMAL 0
#define OAM_MODE_DIMMED 1 // semi-transparent: dimmed while overlay 27 blends, as with a text box or the cursor elsewhere
#define SEQ_SE_DP_SELECT 1500

typedef struct SpriteTemplate {
    void *spriteList;
    void *header;
    int x, y, z;
    int scaleX, scaleY, scaleZ;
    u16 rotation;
    int drawPriority;
    int whichScreen;
    int heapId;
} SpriteTemplate;

typedef struct TouchscreenHitbox {
    u8 top, bottom, left, right;
} TouchscreenHitbox;

void *LONG_CALL Create2DGfxResObjMan(int capacity, int type, int heapId);
void LONG_CALL Destroy2DGfxResObjMan(void *manager);
void *LONG_CALL AddCharResObjFromNarc(void *manager, int narcId, int fileId, BOOL compressed, int id, int vramType, int heapId);
void *LONG_CALL AddPlttResObjFromNarc(void *manager, int narcId, int fileId, BOOL compressed, int id, int vramType, int numPltt, int heapId);
BOOL LONG_CALL SpriteTransfer_CreateCharTransferTask_AllocAtEnd(void *resource);
BOOL LONG_CALL SpriteTransfer_CreatePlttTransferTask(void *resource);
void LONG_CALL SpriteTransfer_DeleteCharTransferTask(void *resource);
void LONG_CALL SpriteTransfer_DeletePlttTransferTask(void *resource);
void LONG_CALL sub_0200A740(void *resource);
void LONG_CALL CreateSpriteResourcesHeader(void *header, int charId, int plttId, int cellId, int animId, int multiCellId, int multiAnimId, int vramTransfer, int priority,
    void *charMan, void *plttMan, void *cellMan, void *animMan, void *multiCellMan, void *multiAnimMan);
void *LONG_CALL Sprite_CreateAffine(const SpriteTemplate *template);
void LONG_CALL Sprite_SetAnimActiveFlag(void *sprite, int active);
void LONG_CALL Sprite_SetPriority(void *sprite, int priority);
void LONG_CALL Sprite_SetAffineOverwriteMode(void *sprite, int mode);
void LONG_CALL Sprite_SetDrawFlag(void *sprite, int draw);
int LONG_CALL TouchscreenHitbox_FindRectAtTouchNew(const TouchscreenHitbox *hitboxes);
BOOL LONG_CALL IsPaletteFadeFinished(void);
int LONG_CALL PlayerAvatar_GetPlayerMoveState(void *playerAvatar);
BOOL LONG_CALL FieldSystem_ShouldDrawStartMenuIcon(FieldSystem *fieldSystem, int icon);
u32 LONG_CALL FontID_String_GetWidth(int fontId, String *string, int letterSpacing);
BOOL LONG_CALL CheckScriptFlag(u16 flag_id);
void LONG_CALL PlaySE(u16 se);
void LONG_CALL MenuInputStateMgr_SetState(void *state, int value);
void LONG_CALL sub_0203DF64(FieldSystem *fieldSystem, int a1);
void LONG_CALL ov01_021F6B50(FieldSystem *fieldSystem);
void LONG_CALL sub_0203C38C(void *startMenu, FieldSystem *fieldSystem);
void LONG_CALL StartScriptFromMenu(TaskManager *taskManager, u16 script, void *lastInteracted);
void LONG_CALL Sprite_SetAnimCtrlSeq(void *sprite, int seq);
void LONG_CALL Sprite_TryChangeAnimSeq(void *sprite, int seq);
void LONG_CALL Sprite_SetPalOffsetRespectVramOffset(void *sprite, u8 offset);
void LONG_CALL GXS_LoadOBJPltt(const void *src, u32 offset, u32 size);
void LONG_CALL Sprite_SetOamMode(void *sprite, int mode);

// overlay 27's cursor helpers
#define ov27_FindSlotInDirection ((int (*)(int slot, int direction, void *slots))(0x0225B360 | 1))
#define ov27_HighlightSlot       ((void (*)(void *work, int slot))(0x0225B398 | 1))
#define ov27_SlotToMenuIndex     ((int (*)(void *work, int slot))(0x0225C170 | 1))
#define OV27_ICON_LAYOUTS        ((const u8 *)0x0225CFC8) // icon in each slot, 8 per menu mode
#define OV27_ICON_GRAPHICS       ((const u16 *)0x0225CF94) // per icon: graphics file, label
#define OV27_ICON_SPRITE(work, slot) (((void **)((u8 *)(work) + 0x390))[slot])
#define OV27_SLOTS(work)         ((u8 *)(work) + 0x470) // 8 bytes per slot; the first is nonzero when it has an icon
#define OV27_CURSOR_SLOT(work)   (*(int *)((u8 *)(work) + 0x14))
#define OV27_ICON_PALETTES(work) ((u8 *)(work) + 0x4CC) // normal palette, then the selected one
#define FIELD_START_MENU_CURSOR(fsys) (*(u8 *)((u8 *)(fsys) + 0xD3))
#define ICON_NONE 0xD
#define SEQ_SE_DP_CURSOR 0x5E0
#define PC_SLOT_FROM_LEFT 3  // POKéGEAR, left of the PC
#define PC_SLOT_FROM_ABOVE 6 // OPTIONS, above the PC

enum { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };

// the empty bottom-right slot of the 2x4 icon grid, in the same layout as the other icons
static const TouchscreenHitbox sPcIconHitbox[] = {
    { 0x8E, 0xAE, 0x60, 0x9C },
    { 0xFF, 0, 0, 0 },
};
#define PC_ICON_X 0x68
#define PC_ICON_Y 0x8E

struct {
    void *work;
    void *charManager;
    void *plttManager;
    void *charRes;
    void *plttRes;
    void *sprite;
    BOOL selected;
    int picked; // PICKED_*: the PC icon was picked, so its selection bounce plays instead of the cursor slot's
    u32 header[0x24 / 4]; // the game writes it a word at a time, so it must stay aligned
} sPcIcon;

enum {
    PICKED_NONE,
    PICKED_WAITING, // picked, overlay 27 has not started the selection bounce yet
    PICKED_BOUNCING,
};
#define ANIM_SEQ_PICKED 2

/**
 * @brief called after overlay 27 creates its icon sprites: add the PC icon in the normal field menu
 */
void StartMenuPCField_CreateIcon(void *work)
{
    sPcIcon.work = NULL;
    sPcIcon.sprite = NULL;
    sPcIcon.selected = FALSE;
    sPcIcon.picked = PICKED_NONE;
    // like the game's own icons, the PC icon appears once a flag is set, from any script. it is left out of the
    // menus with their own icon layouts (overlay 27 0x0225BD50): the Safari Zone, the Bug-Catching Contest, Pal Park
    // and the other special areas, whose menu is rebuilt with that layout when the area loads
    if (OV27_MENU_MODE(work) != 0 || !CheckScriptFlag(START_MENU_REMOTE_PC_FLAG)) {
        return;
    }

    void **managers = OV27_RES_MANAGERS(work);
    // overlay 27's own managers are sized exactly for its icons, so the PC icon gets its own
    sPcIcon.charManager = Create2DGfxResObjMan(1, 0, HEAP_ID_TOUCH_MENU);
    sPcIcon.plttManager = Create2DGfxResObjMan(1, 1, HEAP_ID_TOUCH_MENU);
    sPcIcon.charRes = AddCharResObjFromNarc(sPcIcon.charManager, NARC_TOUCH_MENU, PC_ICON_CHAR_FILE, TRUE, PC_ICON_RES_ID, VRAM_SUB_OBJ, HEAP_ID_TOUCH_MENU);
    sPcIcon.plttRes = AddPlttResObjFromNarc(sPcIcon.plttManager, NARC_TOUCH_MENU, ICON_PLTT_FILE, FALSE, PC_ICON_RES_ID, VRAM_SUB_OBJ, 2, HEAP_ID_TOUCH_MENU);
    SpriteTransfer_CreateCharTransferTask_AllocAtEnd(sPcIcon.charRes);
    sub_0200A740(sPcIcon.charRes);
    SpriteTransfer_CreatePlttTransferTask(sPcIcon.plttRes);
    sub_0200A740(sPcIcon.plttRes);

    CreateSpriteResourcesHeader(sPcIcon.header, PC_ICON_RES_ID, PC_ICON_RES_ID, ICON_CELL_ANIM_ID, ICON_CELL_ANIM_ID, -1, -1, 0, 0,
        sPcIcon.charManager, sPcIcon.plttManager, managers[2], managers[3], NULL, NULL);

    SpriteTemplate template;
    template.spriteList = OV27_SPRITE_LIST(work);
    template.header = sPcIcon.header;
    // like overlay 27: half-pixel rounding, and the sub screen sits 256 pixels below the main screen
    template.x = (PC_ICON_X << 12) + 0x800;
    template.y = (PC_ICON_Y << 12) + 0x800 + (256 << 12);
    template.z = 0;
    template.scaleX = template.scaleY = template.scaleZ = 1 << 12;
    template.rotation = 0;
    template.drawPriority = 1;
    template.whichScreen = 2;
    template.heapId = HEAP_ID_TOUCH_MENU;
    sPcIcon.sprite = Sprite_CreateAffine(&template);
    Sprite_SetAnimActiveFlag(sPcIcon.sprite, TRUE);
    Sprite_SetPriority(sPcIcon.sprite, 0);
    Sprite_SetAffineOverwriteMode(sPcIcon.sprite, 1);
    Sprite_SetDrawFlag(sPcIcon.sprite, TRUE);
    // semi-transparent like the game's icons, so it greys out whenever overlay 27 turns on its blending
    Sprite_SetOamMode(sPcIcon.sprite, OAM_MODE_DIMMED);
    sPcIcon.work = work;

    // its label is printed like the other icons' labels
    // see overlay 27 0x0225BCE8
    String *label = NewString_ReadMsgData(OV27_MSG_DATA(work), MSG_START_MENU_PC);
    void *window = OV27_LABEL_WINDOW(work, PC_LABEL_WINDOW);
    FillWindowPixelBuffer(window, 0);
    AddTextPrinterParameterizedWithColor(window, FONT_SYSTEM, label, (LABEL_WINDOW_WIDTH_PX - FontID_String_GetWidth(FONT_SYSTEM, label, 0)) / 2,
        0, LABEL_TEXT_SPEED, LABEL_TEXT_COLOR, NULL);
    CopyWindowToVram(window);
    String_Delete(label);
}

/**
 * @brief called before overlay 27 frees its resources: free the PC icon's. its sprite goes with overlay 27's sprite list
 */
void StartMenuPCField_DestroyIcon(void *work)
{
    if (sPcIcon.work != work) {
        return;
    }
    SpriteTransfer_DeleteCharTransferTask(sPcIcon.charRes);
    SpriteTransfer_DeletePlttTransferTask(sPcIcon.plttRes);
    Destroy2DGfxResObjMan(sPcIcon.charManager);
    Destroy2DGfxResObjMan(sPcIcon.plttManager);
    sPcIcon.work = NULL;
    sPcIcon.sprite = NULL;
}

/**
 * @brief called each frame after overlay 27 checks its icons for touches: report a touch on the PC icon
 */
void StartMenuPCField_CheckTouch(void *work)
{
    if (sPcIcon.work != work || sPcIcon.sprite == NULL) {
        return;
    }
    FieldSystem *fieldSystem = OV27_FIELD_SYSTEM(work);
    u16 *touchInput = OV27_TOUCH_INPUT(work);
    if (*touchInput != 0) {
        // another icon was touched, which picks that icon instead
        sPcIcon.selected = FALSE;
        return;
    }
    if (OV27_BUSY(work) || !IsPaletteFadeFinished()) {
        return;
    }
    if (PlayerAvatar_GetPlayerMoveState(FIELD_PLAYER_AVATAR(fieldSystem)) != 0 || (*(int *)((u8 *)&gSystem + 0x48) & PAD_KEYS_DPAD)) {
        return;
    }
    if (TouchscreenHitbox_FindRectAtTouchNew(sPcIconHitbox) == 0) {
        *touchInput = REMOTE_PC_TOUCH_CODE;
    }
}

void StartMenuPCField_UpdateVisuals(void *work, BOOL menuOpen);
void StartMenuPCField_CheckTouch(void *work);

static BOOL SlotHasIcon(void *work, int slot)
{
    return OV27_SLOTS(work)[slot * 8] != 0;
}

/**
 * @brief show every grid icon unselected, like overlay 27's highlight with no slot picked
 * @see   overlay 27 0x0225B398
 */
static void UnhighlightAllSlots(void *work)
{
    const u8 *layout = OV27_ICON_LAYOUTS + OV27_MENU_MODE(work) * 8;
    for (int slot = 0; slot < 7; slot++) {
        u8 icon = layout[slot];
        if (icon != ICON_NONE && OV27_ICON_GRAPHICS[icon * 2] != 0xFFFF) {
            GXS_LoadOBJPltt(OV27_ICON_PALETTES(work), slot * 0x20, 0x20);
        }
    }
}

static void SelectPcIcon(void *work, int direction)
{
    sPcIcon.selected = TRUE;
    PlaySE(SEQ_SE_DP_CURSOR);
    UnhighlightAllSlots(work);
    Sprite_SetPalOffsetRespectVramOffset(sPcIcon.sprite, 1);
    Sprite_SetAnimCtrlSeq(sPcIcon.sprite, direction <= DIR_DOWN ? 3 : 1);
}

#define OV27_NUM_SPRITES 9

/**
 * @brief overlay 27 bounces the picked icon each frame until it is done with it, and takes it to be the one under its
 *        cursor; bounce the PC icon instead when the PC was picked
 * @see   overlay 27 0x0225A36E
 */
void StartMenuPCField_BounceSelectedIcon(void *work)
{
    if (sPcIcon.work == work && sPcIcon.sprite != NULL && sPcIcon.picked != PICKED_NONE) {
        sPcIcon.picked = PICKED_BOUNCING;
        Sprite_TryChangeAnimSeq(sPcIcon.sprite, ANIM_SEQ_PICKED);
        // and the PC icon stays lit instead of the icon under the cursor, which overlay 27 has just lit
        Sprite_SetOamMode(OV27_ICON_SPRITE(work, OV27_CURSOR_SLOT(work)), OAM_MODE_DIMMED);
        Sprite_SetOamMode(sPcIcon.sprite, OAM_MODE_NORMAL);
        return;
    }
    Sprite_TryChangeAnimSeq(OV27_ICON_SPRITE(work, OV27_CURSOR_SLOT(work)), ANIM_SEQ_PICKED);
}

/**
 * @brief called each frame after overlay 27 dims the icons the cursor is not on: dim or show the PC icon the same way,
 *        and while the cursor is on the PC, dim every other icon. overlay 27 only greys out the semi-transparent
 *        sprites, by blending, so the PC icon stays semi-transparent unless the cursor is on it
 * @see   overlay 27 0x0225A8E8
 */
void StartMenuPCField_UpdateVisuals(void *work, BOOL menuOpen)
{
    if (sPcIcon.sprite == NULL) {
        // the flag can be set by a script while the menu is up (Elm gives the PC in his lab), so add the icon as soon
        // as it is, like the game's own icons, instead of waiting for the menu to be rebuilt on the next map
        if (OV27_MENU_MODE(work) == 0 && CheckScriptFlag(START_MENU_REMOTE_PC_FLAG)) {
            StartMenuPCField_CreateIcon(work);
        }
        return;
    }
    if (sPcIcon.work != work) {
        return;
    }
    if (sPcIcon.picked == PICKED_BOUNCING && !OV27_BUSY(work)) {
        sPcIcon.picked = PICKED_NONE;
    }
    BOOL cursorShown = menuOpen && (FIELD_MENU_FLAGS(OV27_FIELD_SYSTEM(work)) & 0x3F) != 0;
    if (!cursorShown) {
        if (sPcIcon.selected) {
            sPcIcon.selected = FALSE;
            Sprite_SetPalOffsetRespectVramOffset(sPcIcon.sprite, 0);
        }
        Sprite_SetOamMode(sPcIcon.sprite, OAM_MODE_DIMMED);
        return;
    }
    if (sPcIcon.selected) {
        for (int i = 0; i < OV27_NUM_SPRITES; i++) {
            if (OV27_ICON_SPRITE(work, i) != NULL) {
                Sprite_SetOamMode(OV27_ICON_SPRITE(work, i), OAM_MODE_DIMMED);
            }
        }
        Sprite_SetOamMode(sPcIcon.sprite, OAM_MODE_NORMAL);
    } else {
        Sprite_SetOamMode(sPcIcon.sprite, OAM_MODE_DIMMED);
    }
}

/**
 * @brief overlay 27's d-pad cursor movement, which can also move onto the PC icon: right from POKéGEAR or down
 *        from OPTIONS, and back again with left or up
 * @see   overlay 27 0x0225B404
 */
void StartMenuPCField_HandleDpad(void *work)
{
    // with the menu open, overlay 27 calls this every frame, right after dimming the icons the cursor is not on
    StartMenuPCField_UpdateVisuals(work, TRUE);
    StartMenuPCField_CheckTouch(work);

    int keys = *(int *)((u8 *)&gSystem + 0x48);
    int direction = (keys & 0x40) ? DIR_UP : (keys & 0x80) ? DIR_DOWN : (keys & 0x20) ? DIR_LEFT : (keys & 0x10) ? DIR_RIGHT : -1;
    if (direction < 0) {
        return;
    }
    BOOL hasPc = sPcIcon.work == work && sPcIcon.sprite != NULL;
    int slot = OV27_CURSOR_SLOT(work);
    int next;

    if (hasPc && sPcIcon.selected) {
        if (direction == DIR_UP && SlotHasIcon(work, PC_SLOT_FROM_ABOVE)) {
            next = PC_SLOT_FROM_ABOVE;
        } else if (direction == DIR_LEFT && SlotHasIcon(work, PC_SLOT_FROM_LEFT)) {
            next = PC_SLOT_FROM_LEFT;
        } else {
            return;
        }
        sPcIcon.selected = FALSE;
        Sprite_SetPalOffsetRespectVramOffset(sPcIcon.sprite, 0);
    } else {
        if (hasPc && ((slot == PC_SLOT_FROM_LEFT && direction == DIR_RIGHT) || (slot == PC_SLOT_FROM_ABOVE && direction == DIR_DOWN))) {
            SelectPcIcon(work, direction);
            return;
        }
        next = ov27_FindSlotInDirection(slot, direction, OV27_SLOTS(work));
        if (next < 0 || next == slot) {
            return;
        }
    }

    OV27_CURSOR_SLOT(work) = next;
    PlaySE(SEQ_SE_DP_CURSOR);
    Sprite_SetAnimCtrlSeq(OV27_ICON_SPRITE(work, next), direction <= DIR_DOWN ? 3 : 1);
    ov27_HighlightSlot(work, next);
    FIELD_START_MENU_CURSOR(OV27_FIELD_SYSTEM(work)) = ov27_SlotToMenuIndex(work, next);
}

BOOL StartMenuPCField_IsSelected(void)
{
    return sPcIcon.work != NULL && sPcIcon.selected;
}

/**
 * @brief open the remote PC from the start menu: close the menu and start its script
 * @see   arm9 0x0203D488 Task_StartMenu_HandleSelection_Retire
 */
static BOOL StartMenuPC_Open(TaskManager *taskManager, FieldSystem *fieldSystem, void *startMenu)
{
    FIELD_LAST_TOUCH_MENU_INPUT(fieldSystem) = 0;
    ov01_021F6B50(fieldSystem);
    FIELD_LAST_START_MENU_ACTION(fieldSystem) = 13;

    sub_0203C38C(startMenu, fieldSystem);
    FIELD_MENU_FLAGS(fieldSystem) &= ~0x3F;
    StartScriptFromMenu(taskManager, REMOTE_PC_SCRIPT, NULL);
    sys_FreeMemoryEz(startMenu);
    return FALSE;
}

/**
 * @brief the start menu's handling of A with the cursor on the PC icon
 * @see   arm9 0x0203C508 StartMenu_HandleKeyInput
 */
BOOL StartMenuPCField_HandleKey(void *taskManagerPtr, void *fieldSystemPtr, void *startMenu)
{
    TaskManager *taskManager = taskManagerPtr;
    FieldSystem *fieldSystem = fieldSystemPtr;
    sPcIcon.picked = PICKED_WAITING;
    PlaySE(SEQ_SE_DP_SELECT);
    MenuInputStateMgr_SetState(FIELD_MENU_INPUT_STATE(fieldSystem), 0);
    sub_0203DF64(fieldSystem, 0);
    return StartMenuPC_Open(taskManager, fieldSystem, startMenu);
}

/**
 * @brief the start menu's handling of the PC icon's touch code, like any other icon: close the menu and start the
 *        remote PC script
 * @see   arm9 0x0203C5A4 StartMenu_HandleTouchInput, 0x0203D488 Task_StartMenu_HandleSelection_Retire
 */
BOOL StartMenuPCField_HandleTouch(void *taskManagerPtr, void *fieldSystemPtr, void *startMenu)
{
    TaskManager *taskManager = taskManagerPtr;
    FieldSystem *fieldSystem = fieldSystemPtr;
    sPcIcon.picked = PICKED_WAITING;
    MenuInputStateMgr_SetState(FIELD_MENU_INPUT_STATE(fieldSystem), 1);
    PlaySE(SEQ_SE_DP_SELECT);
    sub_0203DF64(fieldSystem, 1);
    return StartMenuPC_Open(taskManager, fieldSystem, startMenu);
}

#endif // START_MENU_REMOTE_PC
