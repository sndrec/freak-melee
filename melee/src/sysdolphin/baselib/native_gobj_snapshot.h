#ifndef HSD_NATIVE_GOBJ_SNAPSHOT_H
#define HSD_NATIVE_GOBJ_SNAPSHOT_H

#include "gobj.h"
#include <stdint.h>
#include <stddef.h>

// Private process memory: frozen scene config/methods, scheduler controls and
// active table extents, followed by the two actual occupied pool components.
// Userdata, HSD graphs, VM actors and other pointed owners are companions.
typedef struct NativeGObjSnapshotHeader {
    uint64_t epoch;
    HSD_GObjLibInitDataType config;
    uint64_t process_mask;
    struct _unk_gobj_struct delayed;
    HSD_GObj* render_context;
    int32_t invocation_phase, current_priority;
    uint8_t camera_kind, joint_kind;
    int8_t light_kind, fog_kind;
    int render_flags[4];
    size_t object_bytes, proc_bytes;
} NativeGObjSnapshotHeader;

size_t nativeGObjPool_SnapshotBytes(void);
bool nativeGObjPool_Save(void*, size_t capacity, size_t* written);
bool nativeGObjPool_ValidateSnapshot(const void*, size_t bytes);
bool nativeGObjPool_Load(const void*, size_t bytes);

#endif
