#ifndef MELEE_IF_IFALL_ASSETS_H
#define MELEE_IF_IFALL_ASSETS_H

#include <melee/sc/types.h>
#include <sysdolphin/baselib/dat_import.h>

enum {
    IF_ALL_IMPORT_ARENA_SIZE = 2 * 1024 * 1024,
    IF_ALL_IMPORT_RECORD_CAPACITY = 4096,
};

typedef enum IfAllModelRoot {
    IfAllModel_DamageMark,
    IfAllModel_DamageNumber,
    IfAllModel_Count,
    IfAllModel_PlayerName,
    IfAllModel_StockIndicator,
    IfAllModel_Timer,
    IfAllModel_RareArrow,
    IfAllModel_Stock,
    IfAllModel_Magnifier,
    IfAllModel_Countdown,
    IfAllModel_RootCount,
} IfAllModelRoot;

typedef struct IfAllAssets {
    void* owned_bytes;
    void* arena;
    HSD_Dat dat;
    HSD_DatImport import;
    DynamicModelDesc** models[IfAllModel_RootCount];
    SceneDesc scene;
    struct SceneCameraDesc cameras[2];
    struct LightList light;
    struct LightList* lights[2];
} IfAllAssets;

bool ifAllAssets_Import(const void* bytes, size_t size, void* arena,
                        size_t arena_size, u32 record_capacity,
                        IfAllAssets* output);

#endif
