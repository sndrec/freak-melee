#ifndef MELEE_GM_NATIVE_GM_LOOP_SNAPSHOT_H
#define MELEE_GM_NATIVE_GM_LOOP_SNAPSHOT_H

#include "types.h"
#include "native_match_pool.h"
#include <sysdolphin/baselib/gobj.h>

// Private idle-boundary component. GObj payload/topology must restore before
// Load resolves these roots; scene descriptors and callback code stay resident.
typedef struct NativeGmLoopSnapshot {
    uint64_t epoch, object_epoch;
    GameSceneInfo* scene;
    struct gm_80479D58_t loop;
    HSD_GObjLibInitDataType config;
    u32 config_pad;
    HSD_GObj* scheduler_object;
    HSD_GObj* debug_display;
    Event debug_callback;
    NativeMatchPoolReference scheduler_reference, debug_reference;
    u64 cadence_masks[2];
} NativeGmLoopSnapshot;

bool nativeGmLoop_Save(void*, size_t capacity, size_t* written);
bool nativeGmLoop_SnapshotBoundary(void);
bool nativeGmLoop_ValidateSnapshot(const void*, size_t bytes);
bool nativeGmLoop_Load(const void*, size_t bytes);

#endif
