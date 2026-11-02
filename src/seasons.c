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

static BOOL sWinterArea; // the last area loaded is outdoors in winter: its lighting gets brightened
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
    if (sWinterArea) {
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

#endif // SEASONAL_MAPS

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
