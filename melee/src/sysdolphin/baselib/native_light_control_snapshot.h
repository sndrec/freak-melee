#ifndef HSD_NATIVE_LIGHT_CONTROL_SNAPSHOT_H
#define HSD_NATIVE_LIGHT_CONTROL_SNAPSHOT_H
#include "lobj_state_internal.h"
#include "native_objalloc_snapshot.h"
#include "native_class_snapshot.h"

typedef struct NativeLightControlState {
    HSD_LObjInfo* definition;
    HSD_SList* current;
    HSD_LObj* active[MAX_GXLIGHT];
    s32 count,diffuse,specular,attenuation,alpha;
} NativeLightControlState;
typedef struct NativeLightControlSnapshot {
    NativeLightControlState state;
    NativeObjAllocReference list;
    NativeClassReference lights[MAX_GXLIGHT];
    HSD_ClassInfo* types[MAX_GXLIGHT];
} NativeLightControlSnapshot;
bool nativeLightControl_Save(void*,size_t capacity,size_t* written);
bool nativeLightControl_Validate(const void*,size_t bytes);
bool nativeLightControl_Load(const void*,size_t bytes);
#endif
