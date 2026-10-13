#include "mncharsel_assets.h"

#include <melee/lb/lbfile.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/memory.h>

enum {
    CSS_IMPORT_ARENA_SIZE = 16 * 1024 * 1024,
    CSS_IMPORT_RECORD_CAPACITY = 32768,
};

static HSD_Dat css_dat;
static HSD_DatImport css_import;

MnSelectChrDataTable* mnCharSelAssets_Load(const char* filename)
{
    size_t size = lbFileGetSize(filename);
    void* bytes = HSD_MemAlloc((size + 31) & ~(size_t) 31);
    void* arena = HSD_MemAlloc(CSS_IMPORT_ARENA_SIZE);
    u32 root_offset;

    lbFile_8001668C(filename, bytes, &size);
    HSD_ASSERT(0, HSD_DatOpen(&css_dat, bytes, size) == HSD_DAT_OK);
    HSD_ASSERT(0, HSD_DatImportInit(&css_import, &css_dat, arena,
                                   CSS_IMPORT_ARENA_SIZE,
                                   CSS_IMPORT_RECORD_CAPACITY) == HSD_DAT_OK);
    HSD_ASSERT(0, HSD_DatFindRoot(&css_dat, "MnSelectChrDataTable",
                                 &root_offset));
    MnSelectChrDataTable* table =
        mnCharSelAssets_Decode(&css_import, root_offset);
    HSD_ASSERTREPORT(0, table != NULL,
                     "CSS conversion failed: %d at %lx",
                     css_import.status, css_import.error_offset);
    return table;
}
