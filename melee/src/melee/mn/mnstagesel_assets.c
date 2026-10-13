#include "mnstagesel_assets.h"

#include <melee/lb/lbfile.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/memory.h>

enum {
    SSS_IMPORT_ARENA_SIZE = 16 * 1024 * 1024,
    SSS_IMPORT_RECORD_CAPACITY = 32768,
};

static HSD_Dat sss_dat;
static HSD_DatImport sss_import;

MnSelectStageDataTable* mnStageSelAssets_Load(const char* filename)
{
    size_t size = lbFileGetSize(filename);
    void* bytes = HSD_MemAlloc((size + 31) & ~(size_t) 31);
    void* arena = HSD_MemAlloc(SSS_IMPORT_ARENA_SIZE);
    u32 root_offset;

    lbFile_8001668C(filename, bytes, &size);
    HSD_ASSERT(0, HSD_DatOpen(&sss_dat, bytes, size) == HSD_DAT_OK);
    HSD_ASSERT(0, HSD_DatImportInit(&sss_import, &sss_dat, arena,
                                   SSS_IMPORT_ARENA_SIZE,
                                   SSS_IMPORT_RECORD_CAPACITY) == HSD_DAT_OK);
    HSD_ASSERT(0, HSD_DatFindRoot(&sss_dat, "MnSelectStageDataTable",
                                 &root_offset));
    MnSelectStageDataTable* table =
        mnStageSelAssets_Decode(&sss_import, root_offset);
    HSD_ASSERTREPORT(0, table != NULL,
                     "Stage-select conversion failed: %d at %lx",
                     sss_import.status, sss_import.error_offset);
    return table;
}
