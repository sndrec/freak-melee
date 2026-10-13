#ifndef MELEE_IF_IFCOGET_ASSETS_H
#define MELEE_IF_IFCOGET_ASSETS_H

#include <melee/sc/types.h>
#include <sysdolphin/baselib/dat_import.h>

enum {
    IF_COGET_IMPORT_ARENA_SIZE = 512 * 1024,
    IF_COGET_IMPORT_RECORD_CAPACITY = 2048,
};

typedef struct IfCoGetAssets {
    void* owned_bytes;
    void* arena;
    HSD_Dat dat;
    HSD_DatImport import;
    SceneDesc scene;
    DynamicModelDesc model;
    DynamicModelDesc* models[2];
    HSD_AnimJoint* anims[2];
    struct SceneCameraDesc cameras[2];
    struct LightList light_nodes[2];
    struct LightList* lights[3];
} IfCoGetAssets;

bool ifCoGetAssets_Import(const void* bytes, size_t size, void* arena,
                          size_t arena_size, u32 record_capacity,
                          IfCoGetAssets* output);

#endif
