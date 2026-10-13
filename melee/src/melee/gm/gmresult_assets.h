#ifndef MELEE_GM_GMRESULT_ASSETS_H
#define MELEE_GM_GMRESULT_ASSETS_H

#include <melee/sc/types.h>
#include <sysdolphin/baselib/dat_import.h>

enum {
    GM_RESULT_IMPORT_ARENA_SIZE = 8 * 1024 * 1024,
    GM_RESULT_IMPORT_RECORD_CAPACITY = 16384,
};

typedef struct GmResultAssets {
    void* owned_bytes;
    void* arena;
    HSD_Dat dat;
    HSD_DatImport import;
    SceneDesc panel;
    SceneDesc film;
} GmResultAssets;

bool gmResultAssets_Import(const void* bytes, size_t size, void* arena,
                           size_t arena_size, u32 record_capacity,
                           GmResultAssets* output);

#endif
