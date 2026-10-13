#ifndef MELEE_EF_NATIVE_EFFECT_CONTROL_SNAPSHOT_H
#define MELEE_EF_NATIVE_EFFECT_CONTROL_SNAPSHOT_H
#include "eflib.h"
#include <melee/gm/native_match_pool.h>
#include <sysdolphin/baselib/native_class_snapshot.h>

enum { EFFECT_ANIMATION_QUEUE=32, EFFECT_PARAMETER_SLOTS=8 };
typedef struct NativeEffectControlSnapshot {
    uint64_t epoch;
    uint32_t load_kind, effect_count, allocation_serial;
    int32_t animation_count;
    EF_ParamEntry parameters[EFFECT_PARAMETER_SLOTS];
    HSD_JObj* animations[EFFECT_ANIMATION_QUEUE];
    NativeMatchPoolReference objects[EFFECT_PARAMETER_SLOTS];
    NativeClassReference joints[EFFECT_ANIMATION_QUEUE];
    HSD_ClassInfo* joint_types[EFFECT_ANIMATION_QUEUE];
} NativeEffectControlSnapshot;
void nativeEffectControl_BeginScene(void);
void nativeEffectControl_EndScene(void);
bool nativeEffectControl_Save(void*,size_t,size_t*);
bool nativeEffectControl_Validate(const void*,size_t);
bool nativeEffectControl_Load(const void*,size_t);
#endif
