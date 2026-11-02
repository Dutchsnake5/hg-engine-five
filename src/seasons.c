#include "config.h"
#include "types.h"

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
// cap how white the lit trees and roofs can get however white their textures are. Walking into tall grass in winter
// plays the snow footstep instead of the rustle (arm9 0x0205CD50, asm/season_hooks.s).

#ifdef SEASONAL_MAPS

u32 LONG_CALL GrabCurrentSeason(void); // src/pokemon.c: 0 spring, 1 summer, 2 autumn, 3 winter (the clock's month mod 4)
void LONG_CALL PlaySE(u16 seq);

#define SEQ_SE_DP_YUKIASHI 1566 // Diamond/Pearl's snow footstep
#define SEQ_SE_GS_KUSA2    2167 // the tall grass rustle

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
    return gSeasonOverride ? (u32)(gSeasonOverride - 1) : GrabCurrentSeason();
}

static BOOL sWinterArea; // the last area loaded is outdoors in winter: its lighting gets brightened

static void Seasons_AdjustAreaData(u16 *buildingTex, u16 *mapTexSet, BOOL outdoor)
{
    static const u8 variant[4] = { 0, 0xFF, 1, 2 }; // spring, summer (original), autumn, winter
    u32 season = Seasons_Current() & 3;
    u32 v = variant[season];
    u32 i;

    sWinterArea = FALSE;
    if (!outdoor) {
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
    ReadWholeNarcMemberByIdPair(manager + AREA_MANAGER_DATA, AREA_DATA_NARC, areaDataBank);
    *(u16 *)(manager + AREA_MANAGER_BUILDING_TEX) = *(u16 *)(manager + AREA_MANAGER_DATA); // the building set
    Seasons_AdjustAreaData((u16 *)(manager + AREA_MANAGER_BUILDING_TEX), (u16 *)(manager + AREA_MANAGER_DATA + 2), manager[AREA_MANAGER_OUTDOOR]);

    if (manager[AREA_MANAGER_OUTDOOR]) {
        *(void **)(manager + AREA_MANAGER_PROP_NARC) = NARC_New(0x6B, 4);
        sub[2] = (u32)NARC_New(0x28, 4);
    } else {
        *(void **)(manager + AREA_MANAGER_PROP_NARC) = NARC_New(0x6C, 4);
        sub[2] = (u32)NARC_New(0x94, 4);
    }
    return manager;
}

// walking into tall grass: in winter the grass is under snow, so it crunches like snow (asm/season_hooks.s)
void LONG_CALL Seasons_GrassStepSound(void)
{
    PlaySE(sWinterArea ? SEQ_SE_DP_YUKIASHI : SEQ_SE_GS_KUSA2);
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
