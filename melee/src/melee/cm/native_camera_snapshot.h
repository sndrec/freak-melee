#ifndef MELEE_CM_NATIVE_CAMERA_SNAPSHOT_H
#define MELEE_CM_NATIVE_CAMERA_SNAPSHOT_H
#include "types.h"
#include <melee/gm/native_match_pool.h>
#include <sysdolphin/baselib/native_class_snapshot.h>

typedef struct NativeCameraSnapshot {
    uint64_t epoch;
    Camera camera;
    CameraDebugMode debug;
    CameraUnkGlobals coefficients;
    HSD_CObj* auxiliary;
    NativeClassReference primary_backing, auxiliary_backing;
    NativeMatchPoolReference primary_object, quake_object;
} NativeCameraSnapshot;
void nativeCamera_BeginScene(void);
void nativeCamera_EndScene(void);
bool nativeCamera_Save(void*, size_t capacity, size_t* written);
bool nativeCamera_Validate(const void*, size_t bytes);
bool nativeCamera_Load(const void*, size_t bytes);
// Own validated snapshot plus prepared ordinary-class candidate; no live object reads.
bool nativeCamera_ValidateClassSnapshot(const void*,size_t,NativeClassSnapshotView);
#endif
