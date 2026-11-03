#include "config.h"
#include "types.h"
#include "save.h"
#include "system.h"
#include "constants/buttons.h"

// Seasonal map textures (SEASONAL_MAPS in include/config.h, proof of concept). HeartGold picks an area's map texture
// set from its area data (archive a/0/4/2, 8 bytes: building texture set, map texture set, light archive, outdoor
// flag). AreaDataManager_Alloc (ov01 0x021FB888, pret field/area_data) is replaced here by a copy that reads the
// same data and then, outside summer, swaps the map texture set for its seasonal version. scripts/build_seasons.py
// appends the seasonal versions to the map texture archive (a/0/4/4) after its original members: for the n-th set in
// sSeasonalMapTexSets, members SEASONAL_FIRST_MEMBER + 3n (spring), + 3n + 1 (autumn), + 3n + 2 (winter).
// Outdoor areas also swap their building texture set (a/0/7/0) in autumn (flower planters wilt) and winter (planters
// wilt, roofs get snow): set s -> SEASONAL_FIRST_BUILDING + 2s (autumn) / + 2s + 1 (winter). The area data's building
// set also picks the area's building list (a/0/4/3, 104 members), so it stays as it is: the texture set goes in 4 extra
// bytes at the end of the manager (AREA_MANAGER_BUILDING_TEX), and bytereplacement makes AreaDataManager_Load read the
// texture set from there (ov01 0x021FBA82: adds r1, #0x14 -> #0x28, i.e. offset 0x89C + 0x28). In winter, outdoor area
// lighting is brightened and cooled (AreaLightManager_New, ov01 0x021EA220, is replaced too), because the map's lights
// cap how white the lit trees and roofs can get however white their textures are. In winter, walking on plain ground
// that is snow (grass in summer; dirt roads and paths stay silent) plays the snow footstep; tall grass keeps its rustle.

#ifdef SEASONAL_MAPS

u32 LONG_CALL GrabCurrentSeason(void); // src/pokemon.c: 0 spring, 1 summer, 2 autumn, 3 winter (the clock's month mod 4)
void LONG_CALL PlaySE(u16 seq);
u32 LONG_CALL NARC_GetMemberSize(void *narc, u32 member);
void LONG_CALL NARC_ReadWholeMember(void *narc, u32 member, void *dest);

#define SEQ_SE_DP_YUKIASHI 1566 // Diamond/Pearl's snow footstep
#define SEQ_SE_GS_KUSA2    2167 // the tall grass rustle
#define SEQ_SE_DP_KUSA     1618 // the very tall grass rustle

#define SEQ_SE_DP_SELECT   1500 // menu beep (DEBUG_SEASON_SELECT)
#define SEQ_SE_DP_PINPON   1521 // two-tone chime (DEBUG_SEASON_SELECT: back to the clock)

// the map texture sets with seasonal versions; scripts/build_seasons.py reads this list
// (sets 2-20: every town and route in Johto and Kanto; dense forests (Ilex 74, Viridian 103), the Safari Zone and Pal
// Park (75), caves, towers and places that already have a fixed season (Mt. Silver's snow 96, the autumn forest 88)
// are left out)
static const u16 sSeasonalMapTexSets[] = {
    2,   // New Bark, Cherrygrove, Routes 26, 27, 29-31, Battle Tower area
    3,   // Violet, Route 32, Ruins of Alph
    4,   // Azalea, Route 33
    5,   // Goldenrod, Route 34
    6,   // Ecruteak, Routes 35-37
    7,   // Olivine, Cianwood, Routes 38-41
    8,   // Mahogany, Lake of Rage, Routes 42-44
    9,   // Blackthorn, Routes 45, 46
    10,  // Viridian, Indigo Plateau, Mt. Silver outside, Routes 2, 22, 28
    11,  // Vermilion, Routes 6, 11
    12,  // Saffron
    13,  // Cerulean, Routes 4, 5, 9, 10, 24, 25
    14,  // Lavender, Routes 8, 10, 12-15
    15,  // Celadon, Routes 7, 16
    16,  // Fuchsia, Routes 16-18, 20
    17,  // Pewter, Routes 2, 3
    18,  // Pallet, Cinnabar, Routes 1, 19-21
    19,  // Routes 47, 48, Safari Zone Gate
    20,  // Ruins of Alph
    71,  // National Park
    92,  // Battle Frontier
    93,  // Frontier Access
    97,  // Mt. Moon Square
    102, // Mt. Silver Cave (grassy outside)
    74,  // Ilex Forest (winter: frost on the floor, snow on the trees: season_recolor FROST_SETS)
    103, // Viridian Forest (likewise). Added last so every earlier set keeps its members
};

#define SEASONAL_FIRST_MEMBER   106 // the map texture archive's original member count
#define SEASONAL_FIRST_BUILDING 104 // the building texture archive's original member count
#define SEASONAL_FIRST_EFFECT   169 // the field effect archive's (a/1/0/3) original member count

// field effect graphics that are grass (a/1/0/3): the shake when walking into tall grass (25 kusaeff texture, 126 its
// model), the leaves (0, 1, 16 e_kusaeff textures, 127 a model) and the grass drawn over the player's legs (21 lgrass,
// 22 ngrass textures, 122, 123 their models). scripts/build_seasons.py appends a spring, autumn and winter copy of each,
// recoloured like the map's grass: the n-th one's at SEASONAL_FIRST_EFFECT + 3n (spring), + 3n + 1 (autumn), + 3n + 2
// (winter). The field effect loaders swap them in for areas with seasonal textures.
static const u8 sSeasonalEffectMembers[] = { 0, 1, 16, 21, 22, 25, 122, 123, 126, 127 };
// after them: data/seasons/snow_ground.bin (scripts/season_snow_ground.py): u16 map file count, u16 0, then 128 bytes
// per map file, 1 bit per tile (z * 32 + x) = plain ground that is snow in winter (grass, not a path)
#define SNOW_GROUND_MEMBER (SEASONAL_FIRST_EFFECT + NELEMS(sSeasonalEffectMembers) * 3)

// 0 = follow the clock; 1-4 = force spring, summer, autumn, winter (set in RAM to preview a season)
u8 gSeasonOverride;

#define AREA_DATA_NARC         42  // a/0/4/2
#define AREA_MANAGER_SIZE      0x8C4
#define AREA_MANAGER_DATA      0x8B0 // the area data entry
#define AREA_MANAGER_OUTDOOR   0x8B6
#define AREA_MANAGER_SUB       0x8B8
#define AREA_MANAGER_PROP_NARC 0x8C0
#define AREA_MANAGER_BUILDING_TEX 0x8C4 // added: the building texture set to load

static u32 Seasons_Current(void)
{
#ifdef DEBUG_SEASON_SELECT
    u32 forced = GetScriptVar(SEASON_DEBUG_VAR);
    if (!gSeasonOverride && forced >= 1 && forced <= 4) {
        return forced - 1;
    }
#endif
    return gSeasonOverride ? (u32)(gSeasonOverride - 1) : GrabCurrentSeason();
}

#ifdef DEBUG_SEASON_SELECT
// SELECT + R held while the overworld loads: next season (clock -> spring -> summer -> autumn -> winter -> clock)
static void Seasons_DebugSelect(void)
{
    u32 forced;

    if ((gSystem.heldKeys & (PAD_BUTTON_SELECT | PAD_BUTTON_R)) != (PAD_BUTTON_SELECT | PAD_BUTTON_R)) {
        return;
    }
    forced = GetScriptVar(SEASON_DEBUG_VAR);
    forced = forced >= 4 ? 0 : forced + 1;
    SetScriptVar(SEASON_DEBUG_VAR, forced);
    PlaySE(forced ? SEQ_SE_DP_SELECT : SEQ_SE_DP_PINPON);
}
#endif

static BOOL sWinterArea; // the last area loaded is outdoors in winter (snow footsteps)
static BOOL sWinterLight; // ... and its lighting gets brightened: not in the forests (Ilex, Viridian), which keep their shade
static u8 sAreaVariant = 0xFF; // the last area's seasonal variant (0 spring, 1 autumn, 2 winter), 0xFF = original
static u16 sSnowGroundMap = 0xFFFF; // map file whose row of the snow ground table is in sSnowGround
static u8 sSnowGround[128];

#define MAP_TEX_NARC      44 // a/0/4/4
#define BUILDING_TEX_NARC 70 // a/0/7/0
#define FIELD_EFFECT_NARC 103 // a/1/0/3
#define NARC_FILE_COUNT   0x50 // u16 member count in the game's open-archive struct (pret NARC_GetFileCount)

static u16 Seasons_NarcCount(u32 narcId)
{
    void *narc = NARC_New(narcId, 4);
    u16 count = *(u16 *)((u8 *)narc + NARC_FILE_COUNT);
    NARC_Delete(narc);
    return count;
}

// TRUE when both texture archives really have the seasonal members (checked once). A ROM saved by a tool that rebuilt
// them without those members (DSPRE's Save ROM before sync_build.py copied them into its folders) stays in summer
// instead of loading members that aren't there.
static BOOL Seasons_Available(void)
{
    static u8 state; // 0 = not checked yet, 1 = available, 2 = missing
    if (state == 0) {
        BOOL maps = Seasons_NarcCount(MAP_TEX_NARC) >= SEASONAL_FIRST_MEMBER + NELEMS(sSeasonalMapTexSets) * 3;
        BOOL buildings = Seasons_NarcCount(BUILDING_TEX_NARC) >= SEASONAL_FIRST_BUILDING * 3;
        BOOL effects = Seasons_NarcCount(FIELD_EFFECT_NARC) >= SNOW_GROUND_MEMBER + 1;
        state = (maps && buildings && effects) ? 1 : 2;
    }
    return state == 1;
}

static void Seasons_AdjustAreaData(u16 *buildingTex, u16 *mapTexSet, BOOL outdoor)
{
    static const u8 variant[4] = { 0, 0xFF, 1, 2 }; // spring, summer (original), autumn, winter
    u32 season = Seasons_Current() & 3;
    u32 v = variant[season];
    u32 i;

    sWinterArea = FALSE;
    sWinterLight = FALSE;
    sAreaVariant = 0xFF;
    if (!outdoor || !Seasons_Available()) {
        return;
    }
    for (i = 0; i < NELEMS(sSeasonalMapTexSets); i++) {
        if (sSeasonalMapTexSets[i] == *mapTexSet) {
            break;
        }
    }
    if (i == NELEMS(sSeasonalMapTexSets)) {
        return; // not a seasonal area (forests, the Safari Zone, caves...): its buildings, light and sounds stay too
    }
    sWinterArea = season == 3;
    sWinterLight = sWinterArea && *mapTexSet != 74 && *mapTexSet != 103;   // season_recolor FROST_SETS
    sAreaVariant = v;
    if (v == 0xFF) {
        return;
    }
    *mapTexSet = SEASONAL_FIRST_MEMBER + i * 3 + v;
    if (v != 0 && *buildingTex < SEASONAL_FIRST_BUILDING) {
        *buildingTex = SEASONAL_FIRST_BUILDING + *buildingTex * 2 + (v - 1); // autumn, winter
    }
}

void *LONG_CALL AreaDataManager_Alloc_Seasonal(u8 areaDataBank)
{
    u8 *manager = sys_AllocMemory(4, AREA_MANAGER_SIZE + 4);
    u32 *sub = sys_AllocMemoryLo(4, 0x10);

    *(u32 **)(manager + AREA_MANAGER_SUB) = sub;
    sub[0] = areaDataBank;
    sub[1] = 0;
#ifdef DEBUG_SEASON_SELECT
    Seasons_DebugSelect();
#endif
    ReadWholeNarcMemberByIdPair(manager + AREA_MANAGER_DATA, AREA_DATA_NARC, areaDataBank);
    *(u16 *)(manager + AREA_MANAGER_BUILDING_TEX) = *(u16 *)(manager + AREA_MANAGER_DATA); // the building set
    Seasons_AdjustAreaData((u16 *)(manager + AREA_MANAGER_BUILDING_TEX), (u16 *)(manager + AREA_MANAGER_DATA + 2), manager[AREA_MANAGER_OUTDOOR]);
    sSnowGroundMap = 0xFFFF; // read the snow ground table again for the next step

    if (manager[AREA_MANAGER_OUTDOOR]) {
        *(void **)(manager + AREA_MANAGER_PROP_NARC) = NARC_New(0x6B, 4);
        sub[2] = (u32)NARC_New(0x28, 4);
    } else {
        *(void **)(manager + AREA_MANAGER_PROP_NARC) = NARC_New(0x6C, 4);
        sub[2] = (u32)NARC_New(0x94, 4);
    }
    return manager;
}

// field effects (pret field effect manager, ov01): the manager keeps a/1/0/3 open at +0x18; every member it reads goes
// through one of these three functions, replaced here so the grass graphics follow the area's season
#define FieldEffect_Alloc   ((void *(*)(void *, u32, u32))(0x021F141C | 1))            // ov01: manager, size, at end
#define FieldEffect_LoadRes ((void *(*)(void *, u32, void *, u32, u32))(0x021FC5CC | 1)) // ov01: res list, slot, narc, member, 0

static u32 Seasons_EffectMember(u32 member)
{
    if (sAreaVariant != 0xFF) {
        for (u32 i = 0; i < NELEMS(sSeasonalEffectMembers); i++) {
            if (sSeasonalEffectMembers[i] == member) {
                return SEASONAL_FIRST_EFFECT + i * 3 + sAreaVariant;
            }
        }
    }
    return member;
}

// ov01 0x021F14B4: reads a member into memory from the manager's heap (textures and other raw resources)
void *LONG_CALL FieldEffect_ReadMember_Seasonal(u8 *manager, u32 member, u32 atEnd)
{
    void *narc = *(void **)(manager + 0x18);
    void *data;

    member = Seasons_EffectMember(member);
    data = FieldEffect_Alloc(manager, NARC_GetMemberSize(narc, member), atEnd);
    NARC_ReadWholeMember(narc, member, data);
    return data;
}

// ov01 0x021F18D4 and 0x021F1908: load a member into a resource slot of the model list (+0x10) or the animation list
// (+0x14) of the manager's resource set (+0x20)
void *LONG_CALL FieldEffect_LoadModel_Seasonal(u8 *manager, u32 slot, u32 member)
{
    return FieldEffect_LoadRes(*(void **)(*(u8 **)(manager + 0x20) + 0x10), slot, *(void **)(manager + 0x18), Seasons_EffectMember(member), 0);
}

void *LONG_CALL FieldEffect_LoadAnim_Seasonal(u8 *manager, u32 slot, u32 member)
{
    return FieldEffect_LoadRes(*(void **)(*(u8 **)(manager + 0x20) + 0x14), slot, *(void **)(manager + 0x18), Seasons_EffectMember(member), 0);
}

// area lighting (pret field/area_light): a manager of 0x14 bytes (entry count, entries, active entry, model attributes,
// flag) with one 0x30-byte entry per time of day, read from data/areaNNlight.txt: u32 start time (seconds / 2), u8 light
// mask, 4 light colours (u16 at +6), 4 light vectors, then diffuse +0x26, ambient +0x28, specular +0x2A, emission +0x2C
struct AreaLightManagerRaw {
    u32 count;
    u8 *entries;
    u32 active;
    void *modelAttributes;
    u32 flag;
};

#define AREA_LIGHT_ENTRY_SIZE 0x30
#define AreaLight_ReadFile    ((u32 (*)(const char *, u8 **))(0x021EA3E0 | 1))       // ov01, parses the text file
#define AreaLight_Apply       ((void (*)(struct AreaLightManagerRaw *))(0x021EA398 | 1)) // ov01
#define RTC_TimeToSec         ((s32 (*)(void))(0x020147A4 | 1))                          // GF_RTC_TimeToSec
#define AREA_LIGHT_FILES      ((const char *const *)0x02206450)                          // ov01, 5 file names

static u16 Seasons_AddColor(u16 c, u32 r, u32 g, u32 b)
{
    r += c & 31;
    g += (c >> 5) & 31;
    b += (c >> 10) & 31;
    return (c & 0x8000) | (r > 31 ? 31 : r) | ((g > 31 ? 31 : g) << 5) | ((b > 31 ? 31 : b) << 10);
}

// winter: more ambient and emission, a little more in blue, scaled by how bright the main light is at that time
static void Seasons_BrightenLight(struct AreaLightManagerRaw *m)
{
    for (u32 i = 0; i < m->count; i++) {
        u8 *e = m->entries + i * AREA_LIGHT_ENTRY_SIZE;
        u16 light = *(u16 *)(e + 6);
        u32 day = ((light & 31) + ((light >> 5) & 31) + ((light >> 10) & 31)) / 3; // 0-31
        *(u16 *)(e + 0x28) = Seasons_AddColor(*(u16 *)(e + 0x28), day * 2 / 31, day * 2 / 31, day * 3 / 31);
        *(u16 *)(e + 0x2C) = Seasons_AddColor(*(u16 *)(e + 0x2C), day * 4 / 31, day * 5 / 31, day * 7 / 31);
    }
}

void *LONG_CALL AreaLightManager_New_Seasonal(void *modelAttributes, u32 archiveID)
{
    struct AreaLightManagerRaw *m = sys_AllocMemory(4, sizeof(struct AreaLightManagerRaw));
    m->modelAttributes = modelAttributes;
    m->count = AreaLight_ReadFile(AREA_LIGHT_FILES[archiveID < 5 ? archiveID : 0], &m->entries);
    m->active = 0;
    if (sWinterLight) {
        Seasons_BrightenLight(m);
    }
    s32 now = RTC_TimeToSec() / 2;
    for (u32 i = 0; i < m->count; i++) {
        if ((s32)*(u32 *)(m->entries + i * AREA_LIGHT_ENTRY_SIZE) > now) {
            m->active = i;
            break;
        }
    }
    m->flag = 1;
    AreaLight_Apply(m);
    return m;
}

// the player's step sounds (arm9 sub_0205CC94, pret unk_0205CB48.s), replaced whole (hooked at its first instruction):
// the original's puddle, shallow water, mud and grass sounds, plus in winter the snow footstep on plain ground that is
// snow (grass under it in summer, from the snow ground table); dirt roads, paths and sand stay silent and grass keeps
// its rustle
#define PlayerAvatar_GetMoveState        ((u32 (*)(void *))(0x0205C6CC | 1))
#define PlayerAvatar_GetMapObject        ((u8 *(*)(void *))(0x0205C6DC | 1))
#define PlayerAvatar_GetState            ((u32 (*)(void *))(0x0205C700 | 1))
#define MapObject_GetTileBehavior        ((u16 (*)(void *))(0x0205F504 | 1))
#define MapObject_GetMovementCommand     ((u32 (*)(void *))(0x0205F4D4 | 1))
#define MovementCommand_GetDirection     ((s32 (*)(u32))(0x02062390 | 1))
#define MapObject_GetTileBehaviorAhead   ((u16 (*)(void *, s32))(0x02060FA8 | 1))
#define MapObject_GetX                   ((u32 (*)(void *))(0x0205F914 | 1))
#define MapObject_GetZ                   ((u32 (*)(void *))(0x0205F934 | 1))
#define MovementCommand_sub_0205DE64           ((BOOL (*)(u32))(0x0205DE64 | 1))
#define MetatileBehavior_IsTallGrass     ((BOOL (*)(u8))(0x0205B6E8 | 1))
#define MetatileBehavior_IsVeryTallGrass ((BOOL (*)(u8))(0x0205B6F4 | 1))
#define MetatileBehavior_IsSand          ((BOOL (*)(u8))(0x0205B798 | 1))
#define MetatileBehavior_IsShallowWater  ((BOOL (*)(u8))(0x0205B7A4 | 1))
#define MetatileBehavior_IsMud           ((BOOL (*)(u8))(0x0205B8AC | 1))
#define MetatileBehavior_IsPuddle        ((BOOL (*)(u8))(0x0205B984 | 1))
#define PlayerAvatar_StepEffects         ((void (*)(void *, void *))(0x0205CD70 | 1))
#define SEQ_SE_DP_FOOT3_0    1600
#define SEQ_SE_DP_FOOT3_1    1601
#define SEQ_SE_DP_MARSH_WALK 1620
#define MAP_OBJECT_MANAGER   0xB4  // map object -> its manager
#define MANAGER_FIELD_SYSTEM 0x128 // manager -> field system
#define FIELD_MAP_MATRIX     0x30  // field system -> map matrix (u8 width at +0, map file numbers u16 at +0x964)
#define MAP_MATRIX_MODELS    0x964

static BOOL Seasons_IsSnowGround(u8 *object)
{
    u8 *matrix = *(u8 **)(*(u8 **)(*(u8 **)(object + MAP_OBJECT_MANAGER) + MANAGER_FIELD_SYSTEM) + FIELD_MAP_MATRIX);
    u32 x = MapObject_GetX(object), z = MapObject_GetZ(object);
    u16 map = ((u16 *)(matrix + MAP_MATRIX_MODELS))[(z / 32) * matrix[0] + x / 32];
    u32 tile = (z % 32) * 32 + x % 32;

    if (map != sSnowGroundMap) {
        u16 count;
        ReadFromNarcMemberByIdPair(&count, FIELD_EFFECT_NARC, SNOW_GROUND_MEMBER, 0, 2);
        if (map < count) {
            ReadFromNarcMemberByIdPair(sSnowGround, FIELD_EFFECT_NARC, SNOW_GROUND_MEMBER, 4 + map * 128, 128);
        } else {
            memset(sSnowGround, 0, sizeof(sSnowGround));
        }
        sSnowGroundMap = map;
    }
    return (sSnowGround[tile >> 3] >> (tile & 7)) & 1;
}

void LONG_CALL PlayerAvatar_StepSounds_Seasonal(void *avatar)
{
    u8 *object;
    u8 here, ahead;
    s32 dir;
    u32 command;

    if (PlayerAvatar_GetMoveState(avatar) != 1) {
        return;
    }
    object = PlayerAvatar_GetMapObject(avatar);
    here = MapObject_GetTileBehavior(object);
    dir = MovementCommand_GetDirection(MapObject_GetMovementCommand(object));
    ahead = dir == -1 ? here : MapObject_GetTileBehaviorAhead(object, dir);
    if (MetatileBehavior_IsPuddle(here)) {
        PlaySE(SEQ_SE_DP_FOOT3_0);
    }
    if (MetatileBehavior_IsShallowWater(here)) {
        PlaySE(SEQ_SE_DP_FOOT3_1);
    }
    MetatileBehavior_IsSand(here);
    if (MetatileBehavior_IsMud(here)) {
        PlaySE(SEQ_SE_DP_MARSH_WALK);
    }
    command = MapObject_GetMovementCommand(object);
    PlayerAvatar_GetState(avatar);
    if (!MovementCommand_sub_0205DE64(command)) {
        if (MetatileBehavior_IsVeryTallGrass(here) || MetatileBehavior_IsVeryTallGrass(ahead)) {
            PlaySE(SEQ_SE_DP_KUSA);
        }
        if (MetatileBehavior_IsTallGrass(here) || MetatileBehavior_IsTallGrass(ahead)) {
            PlaySE(SEQ_SE_GS_KUSA2);
        } else if (sWinterArea && here == 0 && !MetatileBehavior_IsVeryTallGrass(ahead) && Seasons_IsSnowGround(object)) {
            PlaySE(SEQ_SE_DP_YUKIASHI);
        }
    }
    PlayerAvatar_StepEffects(object, avatar);
}

// autumn trees in several colours: each seasonal autumn map texture set holds its tree textures (scripts/season_trees.py
// TREE_TEXTURES) as SEASON_TREE_VERSIONS colour versions stacked from top to bottom (64 x 256, 64 rows each), and every
// tree quad of a map model is moved onto one version (texture coordinate V + 64 * version) when the model is loaded
// into one of the four map model buffers, before its textures are bound. Pieces of trees in one row that touch (a tree
// can be the end of one quad and the start of the next) share their group's version; whole trees each get their own.
// scripts/season_trees.py patch_model does the same for previews; keep the two in step.
#define SEASON_TREE_VERSIONS  4
#define SEASON_TREE_STRIP_RIGHT 2      // a strip version's right-hand tree has version (k + 2) % 4
#define MAP_MODEL_BUFFERS     0x02209B70 // ov01: four buffers of 0xF000 bytes (pret ov01_021FAC44)
#define MAP_MODEL_BUFFER_SIZE 0xF000
#define MAX_TREE_QUADS        192        // the most in any map model: 160
#define TREE_ROW              (4 << 12)  // bases this close (model units, fx12) stand in one row
#define TREE_TOUCH            (2 << 12)  // and pieces this close side by side touch
#define NNS_G3dGetMdlSet      ((void *(*)(void *))0x020C3B40)         // ARM code
#define NNS_G3dBindMdlSet     ((void (*)(void *, void *))0x020BED00)  // ARM code
#define DC_FlushRange         ((void (*)(const void *, u32))0x020D2894) // ARM code

struct TreeQuad {
    u32 *tc[4]; // the quad's texture coordinate words
    s32 x0, x1, zb; // x range and base z, model units (fx12)
    s32 xa, xb; // x where the texture coordinate U is smallest / largest
    s16 ua, ub; // those U (1/16 texels)
    u8 parent, piece, strip, ntc; // piece: a strip piece or a part of the single-tree texture, not a whole tree
};

static const char sTreeTextures[][16] = { "tree01gs", "tree01_re", "treekn", "treekn_re", "d05tree01", "d05tree01_re", "dtf_f09", "dtf_f10" };
static const u8 sTreeIsStrip[] = { 0, 1, 0, 1, 0, 1, 0, 1 };   // Ilex Forest's trees (d05), Viridian Forest's (dtf_f09/10)

// parameter words of the packed display list commands (0xFF = not one the map models use)
static const u8 sGxParams[0x73] = {
    [0x00] = 0, [0x01 ... 0x0F] = 0xFF, [0x10] = 1, [0x11] = 0, [0x12] = 1, [0x13] = 1, [0x14] = 1, [0x15] = 0,
    [0x16] = 16, [0x17] = 12, [0x18] = 16, [0x19] = 12, [0x1A] = 9, [0x1B] = 3, [0x1C] = 3, [0x1D ... 0x1F] = 0xFF,
    [0x20] = 1, [0x21] = 1, [0x22] = 1, [0x23] = 2, [0x24] = 1, [0x25] = 1, [0x26] = 1, [0x27] = 1, [0x28] = 1,
    [0x29] = 1, [0x2A] = 1, [0x2B] = 1, [0x2C ... 0x2F] = 0xFF, [0x30] = 1, [0x31] = 1, [0x32] = 1, [0x33] = 1,
    [0x34] = 32, [0x35 ... 0x3F] = 0xFF, [0x40] = 1, [0x41] = 0, [0x42 ... 0x4F] = 0xFF, [0x50] = 1,
    [0x51 ... 0x5F] = 0xFF, [0x60] = 1, [0x61 ... 0x6F] = 0xFF, [0x70] = 3, [0x71] = 2, [0x72] = 1,
};

static s32 SignExtend(u32 v, u32 bits)
{
    return (s32)(v << (32 - bits)) >> (32 - bits);
}

static s32 FloorDiv(s64 a, s64 b)
{
    s64 q = a / b;
    return (s32)((a % b != 0 && ((a < 0) != (b < 0))) ? q - 1 : q);
}

// an NNS dictionary: its entries, entry size and the start of its 16-byte names
static u8 *NnsDict(u8 *dict, u32 *count, u32 *entrySize, u8 **names)
{
    u8 *p = dict + *(u16 *)(dict + 6);
    *count = dict[1];
    *entrySize = *(u16 *)p;
    *names = p + *(u16 *)(p + 2);
    return p + 4;
}

static u32 Seasons_TreeVersion(s32 x, s32 z, u32 seed)
{
    u32 h = ((u32)(x >> 12) * 0x9E3779B1u) ^ ((u32)(z >> 12) * 0x85EBCA77u) ^ (seed * 0xC2B2AE3Du);
    h ^= h >> 16; // murmur3's finaliser: positions are whole units, so mix well
    h *= 0x85EBCA6Bu;
    h ^= h >> 13;
    h *= 0xC2B2AE35u;
    h ^= h >> 16;
    return (h >> 16) % SEASON_TREE_VERSIONS;
}

// the quads of one tree shape's display list
static u32 Seasons_TreeQuads(u32 *dl, u32 size, BOOL strip, s32 scale, struct TreeQuad *quads, u32 n)
{
    u32 *p = dl, *end = (u32 *)((u8 *)dl + size);
    u32 *tc = NULL, prim = 0xFF, nv = 0;
    s32 x = 0, z = 0, xs[4], zs[4];
    u32 *tcs[4];

    while (p < end && n < MAX_TREE_QUADS) {
        u32 cmds = *p++;
        for (u32 i = 0; i < 4; i++) {
            u32 c = (cmds >> (8 * i)) & 0xFF;
            u32 np = c < NELEMS(sGxParams) ? sGxParams[c] : 0xFF;
            u32 w0 = np ? p[0] : 0;
            if (np == 0xFF) {
                return n;
            }
            if (c == 0x40) {
                prim = w0 & 3;
                nv = 0;
            } else if (c == 0x22) {
                tc = p;
            } else if (c >= 0x23 && c <= 0x28) {
                if (c == 0x23) {
                    x = SignExtend(w0, 16);
                    z = SignExtend(p[1], 16);
                } else if (c == 0x24) {
                    x = SignExtend(w0, 10) << 6;
                    z = SignExtend(w0 >> 20, 10) << 6;
                } else if (c == 0x25) {
                    x = SignExtend(w0, 16);
                } else if (c == 0x26) {
                    x = SignExtend(w0, 16);
                    z = SignExtend(w0 >> 16, 16);
                } else if (c == 0x27) {
                    z = SignExtend(w0 >> 16, 16);
                } else {
                    x += SignExtend(w0, 10);
                    z += SignExtend(w0 >> 20, 10);
                }
                if (prim == 1 && tc != NULL) {
                    tcs[nv] = tc;
                    xs[nv] = (s32)(((s64)x * scale) >> 12);
                    zs[nv] = (s32)(((s64)z * scale) >> 12);
                    if (++nv == 4) {
                        struct TreeQuad *q = &quads[n++];
                        s32 umin = 0x7FFF, umax = -0x8000;
                        q->ntc = 0;
                        q->x0 = q->x1 = xs[0];
                        q->zb = zs[0];
                        for (u32 v = 0; v < 4; v++) {
                            s32 u = SignExtend(*tcs[v], 16);
                            u32 t;
                            for (t = 0; t < q->ntc && q->tc[t] != tcs[v]; t++) {
                            }
                            if (t == q->ntc) {
                                q->tc[q->ntc++] = tcs[v];
                            }
                            if (xs[v] < q->x0) q->x0 = xs[v];
                            if (xs[v] > q->x1) q->x1 = xs[v];
                            if (zs[v] > q->zb) q->zb = zs[v];
                            if (u < umin) {
                                umin = u;
                                q->xa = xs[v];
                            }
                            if (u > umax) {
                                umax = u;
                                q->xb = xs[v];
                            }
                        }
                        q->ua = umin;
                        q->ub = umax;
                        q->strip = strip;
                        q->piece = strip || umin != 0 || umax != 64 * 16;
                        nv = 0;
                    }
                }
            }
            p += np;
        }
    }
    return n;
}

static u32 TreeRoot(struct TreeQuad *q, u32 i)
{
    while (q[i].parent != i) {
        q[i].parent = q[q[i].parent].parent;
        i = q[i].parent;
    }
    return i;
}

static void Seasons_AutumnTrees(u8 *bmd)
{
    u8 isTree[32] = { 0 }, isStrip[32] = { 0 };
    u8 *mdl0, *model, *mats, *shapes, *names, *entries, *p, *end;
    u32 count, esize, n = 0, mat = 0xFFFF, i, j, any = 0;
    s32 scale;
    struct TreeQuad *quads;

    if (*(u32 *)bmd != 0x30444D42) { // 'BMD0'
        return;
    }
    mdl0 = bmd + *(u32 *)(bmd + 0x10);
    entries = NnsDict(mdl0 + 8, &count, &esize, &names);
    model = mdl0 + *(u32 *)entries;
    scale = *(s32 *)(model + 0x1C);
    mats = model + *(u32 *)(model + 8);
    entries = NnsDict(mats + *(u16 *)mats, &count, &esize, &names);
    for (i = 0; i < count; i++) {
        for (j = 0; j < NELEMS(sTreeTextures); j++) {
            u32 c;
            for (c = 0; c < 16 && names[16 * i + c] == (u8)sTreeTextures[j][c]; c++) {
            }
            if (c == 16) {
                u8 *e = entries + esize * i, *list = mats + *(u16 *)e;
                for (u32 k = 0; k < e[2]; k++) {
                    isTree[list[k] >> 3] |= 1 << (list[k] & 7);
                    if (sTreeIsStrip[j]) {
                        isStrip[list[k] >> 3] |= 1 << (list[k] & 7);
                    }
                    any = 1;
                }
            }
        }
    }
    if (!any) {
        return;
    }
    quads = sys_AllocMemoryLo(4, sizeof(struct TreeQuad) * MAX_TREE_QUADS);
    if (quads == NULL) {
        return;
    }
    shapes = model + *(u32 *)(model + 0xC);
    entries = NnsDict(shapes, &count, &esize, &names);
    {
        u8 done[32] = { 0 };
        p = model + *(u32 *)(model + 4);
        end = mats;
        while (p < end) {
            u32 c = p[0] & 0x1F, var = p[0] >> 5;
            if (c == 0x00) {
                p += 1;
            } else if (c == 0x01) {
                break;
            } else if (c == 0x02) {
                p += 3;
            } else if (c == 0x03) {
                p += 2;
            } else if (c == 0x04) {
                mat = p[1];
                p += 2;
            } else if (c == 0x05) {
                u32 s = p[1];
                if (mat < 256 && (isTree[mat >> 3] >> (mat & 7) & 1) && !(done[s >> 3] >> (s & 7) & 1) && s < count) {
                    u8 *hdr = shapes + *(u32 *)(entries + esize * s);
                    done[s >> 3] |= 1 << (s & 7);
                    n = Seasons_TreeQuads((u32 *)(hdr + *(u32 *)(hdr + 8)), *(u32 *)(hdr + 12),
                                          isStrip[mat >> 3] >> (mat & 7) & 1, scale, quads, n);
                }
                p += 2;
            } else if (c == 0x06) {
                p += 4 + (var & 1) + ((var >> 1) & 1);
            } else if (c == 0x07 || c == 0x08) {
                p += 2 + (var & 1) + ((var >> 1) & 1);
            } else if (c == 0x09) {
                p += 3 + p[2] * 3;
            } else if (c == 0x0B) {
                p += 1;
            } else if (c == 0x0C || c == 0x0D) {
                p += 3;
            } else {
                break;
            }
        }
    }
    // groups: pieces in one row that touch, at least one of them a piece of a tree
    for (i = 0; i < n; i++) {
        quads[i].parent = i;
    }
    for (i = 0; i < n; i++) {
        for (j = i + 1; j < n; j++) {
            struct TreeQuad *a = &quads[i], *b = &quads[j];
            s32 dz = a->zb - b->zb;
            if ((a->piece || b->piece) && dz <= TREE_ROW && dz >= -TREE_ROW && a->x0 <= b->x1 + TREE_TOUCH && b->x0 <= a->x1 + TREE_TOUCH) {
                u32 ri = TreeRoot(quads, i), rj = TreeRoot(quads, j);
                if (ri != rj) {
                    quads[ri > rj ? ri : rj].parent = ri < rj ? ri : rj;
                }
            }
        }
    }
    {
        u8 root[MAX_TREE_QUADS], first[MAX_TREE_QUADS];
        // each group's version comes from its leftmost piece (then the one furthest back)
        for (i = 0; i < n; i++) {
            root[i] = TreeRoot(quads, i);
            first[i] = 0xFF;
        }
        for (i = 0; i < n; i++) {
            u32 r = root[i], b = first[r];
            if (b == 0xFF || quads[i].x0 < quads[b].x0 || (quads[i].x0 == quads[b].x0 && quads[i].zb < quads[b].zb)) {
                first[r] = i;
            }
        }
        for (i = 0; i < n; i++) {
            struct TreeQuad *q = &quads[i];
            u32 b = first[root[i]], k = Seasons_TreeVersion(quads[b].x0, quads[b].zb, *(u32 *)model);
            u16 base = 0xFFFF;
            if (!q->strip && q->ub != q->ua) {
                // a single tree in a group with strip pieces: the nearest strip's texture, carried on to the tree's
                // middle, says whether it is one of the strip's left trees or right trees
                s32 xc = q->xa + FloorDiv((s64)(32 * 16 - q->ua) * (q->xb - q->xa), q->ub - q->ua);
                s32 bestDist = 0x7FFFFFFF;
                u32 best = 0xFF;
                for (j = 0; j < n; j++) {
                    struct TreeQuad *t = &quads[j];
                    if (t->strip && root[j] == root[i] && t->xb != t->xa) {
                        s32 lo = t->xa < t->xb ? t->xa : t->xb, hi = t->xa < t->xb ? t->xb : t->xa;
                        s32 dist = lo - xc > xc - hi ? lo - xc : xc - hi;
                        if (dist < 0) {
                            dist = 0;
                        }
                        if (dist < bestDist) {
                            bestDist = dist;
                            best = j;
                        }
                    }
                }
                if (best != 0xFF) {
                    struct TreeQuad *t = &quads[best];
                    s32 u = t->ua + FloorDiv((s64)(xc - t->xa) * (t->ub - t->ua), t->xb - t->xa);
                    k = (k + SEASON_TREE_STRIP_RIGHT * (FloorDiv(u, 32 * 16) & 1)) % SEASON_TREE_VERSIONS;
                }
            }
            for (j = 0; j < q->ntc; j++) {
                u16 t = *q->tc[j] >> 16;
                if (t < base) {
                    base = t;
                }
            }
            base = (base / 1024) * 1024;
            for (j = 0; j < q->ntc; j++) {
                u32 w = *q->tc[j];
                *q->tc[j] = (w & 0xFFFF) | ((u32)(u16)((w >> 16) - base + k * 1024) << 16);
            }
        }
    }
    sys_FreeMemoryEz(quads);
    DC_FlushRange(bmd, *(u32 *)(bmd + 8));
}

// arm9 0x0201F64C (pret GF3dRender_BindModelSet), replaced: autumn map models get their trees moved first
BOOL LONG_CALL GF3dRender_BindModelSet_Seasonal(void *header, void *tex)
{
    void *mdlSet;

    if (sAreaVariant == 1 && (u32)header >= MAP_MODEL_BUFFERS && (u32)header < MAP_MODEL_BUFFERS + 4 * MAP_MODEL_BUFFER_SIZE) {
        Seasons_AutumnTrees(header);
    }
    mdlSet = NNS_G3dGetMdlSet(header);
    if (tex != NULL) {
        NNS_G3dBindMdlSet(mdlSet, tex);
        return TRUE;
    }
    return FALSE;
}

#endif // SEASONAL_MAPS
