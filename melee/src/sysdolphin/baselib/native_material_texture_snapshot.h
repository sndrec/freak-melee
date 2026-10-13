#ifndef HSD_NATIVE_MATERIAL_TEXTURE_SNAPSHOT_H
#define HSD_NATIVE_MATERIAL_TEXTURE_SNAPSHOT_H
#include "native_class_snapshot.h"
#include "tobj.h"

enum { NATIVE_MATERIAL_SHADOW_LIMIT=4096 };
typedef struct NativeMaterialTextureSnapshot {
    uint64_t object_epoch;
    HSD_TObj* roots[2]; // actual shadow chain and lazy toon texture
    NativeClassReference backing[2];
    HSD_ClassInfo* types[2];
    uint32_t shadows,reserved;
} NativeMaterialTextureSnapshot;
bool nativeMaterialTexture_Save(void*,size_t,size_t*);
bool nativeMaterialTexture_Validate(const void*,size_t);
bool nativeMaterialTexture_ValidateClass(const void*,size_t,NativeClassSnapshotView);
bool nativeMaterialTexture_Load(const void*,size_t);
#endif
