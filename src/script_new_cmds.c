#include "../include/config.h"
#include "../include/constants/file.h"
#include "../include/map_events_internal.h"
#include "../include/pokemon.h"
#include "../include/repel.h"
#include "../include/roamer.h"
#include "../include/script.h"
#include "../include/types.h"

#define SCRIPT_NEW_CMD_REPEL_USE 0
// Show one of the 14 mood bubbles (the set the following Pokemon uses) above a person on the map.
// From DSPRE: DummyTextTrap 1 <person id + 256 * bubble>; bubbles: 0 heart, 1-5 faces (1 laughing, 4 smiling),
// 6 music note, 7 ?, 8 !, 9 sweat drop, 10 worried, 11 poison, 12 "...", 13 zZ
#define SCRIPT_NEW_CMD_SHOW_BUBBLE 1
#define NUM_MOOD_BUBBLES 14

#define SCRIPT_NEW_CMD_MAX 256

// overlay 1: starts the bubble effect over a map object (ScrCmd_597 uses it for the follower with bubble 0)
void LONG_CALL ov01_02203AB4(FieldSystem *fsys, LocalMapObject *obj, int bubble);

// the active, visible map object with this id (as MapObjectManager_GetFirstActiveObjectByID)
static LocalMapObject *FindActiveMapObject(FieldSystem *fsys, u32 id)
{
    MapObjectMan *man = fsys->mapObjectMan;
    if (man == NULL) {
        return NULL;
    }
    LocalMapObject *obj = man->objects;
    for (u32 i = 0; i < man->object_count; i++, obj++) {
        if ((obj->flags & MAPOBJECTFLAG_ACTIVE) && !(obj->flags & (1 << 25)) && obj->id == (int)id) {
            return obj;
        }
    }
    return NULL;
}

BOOL Script_RunNewCmd(SCRIPTCONTEXT *ctx)
{
    u8 sw = ScriptReadByte(ctx);
    u16 arg0 = ScriptReadHalfword(ctx);

    switch (sw) {
    case SCRIPT_NEW_CMD_REPEL_USE:;
#ifdef IMPLEMENT_REUSABLE_REPELS
        u16 most_recent_repel = Repel_GetMostRecent();
        SetScriptVar(arg0, most_recent_repel);
        Repel_Use(most_recent_repel, HEAPID_MAIN_HEAP);
#endif
        break;

    case SCRIPT_NEW_CMD_SHOW_BUBBLE: {
        LocalMapObject *obj = FindActiveMapObject(ctx->fsys, arg0 & 0xFF);
        if (obj != NULL && (arg0 >> 8) < NUM_MOOD_BUBBLES) {
            // this starts the bubble as a task that the script's task waits on; stop running commands for this
            // frame (as ScrCmd_597 does) so nothing else touches the task stack before the bubble task takes over
            ov01_02203AB4(ctx->fsys, obj, arg0 >> 8);
            return TRUE;
        }
        break;
    }

    default:
        break;
    }

    return FALSE;
}

#ifdef EXPAND_ROAMERS
BOOL LONG_CALL ScrCmd_CreateRoamer(SCRIPTCONTEXT *ctx)
{
    u8 roamerNo = ScriptReadByte(ctx);
    Save_CreateRoamerByID(ctx->fsys->savedata, roamerNo);
    return FALSE;
}
#endif // EXPAND_ROAMERS
