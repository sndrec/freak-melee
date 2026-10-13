#ifndef MELEE_GM_GMPAUSE_ASSETS_H
#define MELEE_GM_GMPAUSE_ASSETS_H

#include <melee/sc/types.h>
#include <sysdolphin/baselib/dat_import.h>

enum {
    GM_PAUSE_IMPORT_ARENA_SIZE = 512 * 1024,
    GM_PAUSE_IMPORT_RECORD_CAPACITY = 2048,
};

typedef struct GmPauseAssets {
    void* owned_bytes;
    void* arena;
    HSD_Dat dat;
    HSD_DatImport import;
    SceneDesc scene;
    DynamicModelDesc model;
    DynamicModelDesc* models[2];
    HSD_MatAnimJoint* matanims[2];
} GmPauseAssets;

bool gmPauseAssets_Import(const void* bytes, size_t size, void* arena,
                          size_t arena_size, u32 record_capacity,
                          GmPauseAssets* output);

#endif
