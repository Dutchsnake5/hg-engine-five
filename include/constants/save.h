#ifndef CONSTANTS_SAVE_H
#define CONSTANTS_SAVE_H

// 2026-10-11 (this hack): one more page (0x2F -> 0x30) for the enlarged bag (include/constants/item.h). Everything that
// depends on the page count moved with it: the struct offsets and sizes below +0x1000, the extra save data pages
// (SAVE_PAGE_MAX_n) +1, and heap 3 -0x1000 so the heaps use the same RAM as before. Each 0x40000 save copy has 64
// pages; the extra save data now ends at page 60. Old saves: Tools\convert_save.py in the project folder.
#define OFFSET_saveSlotSpecs    0x302B4
#define OFFSET_lastGoodSaveSlot 0x302F0
#define OFFSET_lastGoodSaveNo   0x302F4
#define OFFSET_sectorCleanFlag  0x30308
#define OFFSET_lastGoodSector   0x3030A

#define FULL_SAVE_SIZE               0x30600
#define DYNAMIC_REGION_SIZE          0x30000
#define DYNAMIC_REGION_BYTEWIDE_SIZE 0x30
#define NEW_HEAP3_SIZE               0x10F000

// done this way so scripts/make.py can understand
#define SAVE_PAGE_MAX    0x30
#define SAVE_PAGE_MAX_3  0x33
#define SAVE_PAGE_MAX_4  0x34
#define SAVE_PAGE_MAX_6  0x36
#define SAVE_PAGE_MAX_8  0x38
#define SAVE_PAGE_MAX_10 0x3A

#define SAVE_SECTOR_SIZE 0x1000
#define SAVE_BLOCK_NUM   42

#define NUM_PC_BOXES           30
#define NUM_PC_BOXES_MINUS_ONE 29

#endif // CONSTANTS_SAVE_H
