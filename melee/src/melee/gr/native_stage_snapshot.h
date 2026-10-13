#ifndef MELEE_GR_NATIVE_STAGE_SNAPSHOT_H
#define MELEE_GR_NATIVE_STAGE_SNAPSHOT_H
#include "stage_state_internal.h"
#include <melee/gm/native_match_pool.h>
#include <sysdolphin/baselib/native_class_snapshot.h>

typedef struct NativeStageClassRoot {
    NativeClassReference backing;
    HSD_ClassInfo* type;
} NativeStageClassRoot;

typedef struct NativeStageSnapshot {
    uint64_t epoch;
    StKind selected_stage;
    u32 started;
    StageInfo stage;
    u8 joint_updates[STAGE_MAP_OBJECT_COUNT];
    s16 joint_update_counter;
    VecMtx spawn_cache;
    float ground_scale;
    GXColor ground_colors[9];
    NativeMatchPoolReference map_objects[STAGE_MAP_OBJECT_COUNT];
    NativeMatchPoolReference map_userdata[STAGE_MAP_OBJECT_COUNT];
    NativeMatchPoolReference fog, special_object;
    NativeStageClassRoot joints[GROUND_JOINT_POINT_COUNT];
    NativeStageClassRoot cameras[STAGE_CAMERA_COUNT];
} NativeStageSnapshot;

void nativeStage_BeginScene(void);
void nativeStage_EndScene(void);
bool nativeStage_Save(void*, size_t capacity, size_t* written);
bool nativeStage_Validate(const void*, size_t bytes);
bool nativeStage_Load(const void*, size_t bytes);
#endif
