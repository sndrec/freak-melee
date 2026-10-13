#ifndef MELEE_EF_NATIVE_SCRIPT_EFFECT_INTERNAL_H
#define MELEE_EF_NATIVE_SCRIPT_EFFECT_INTERNAL_H
#include "native_script_effect_actor.h"
#include <melee/ft/native_script_execution.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
typedef struct NativeScriptEffectActor {
    const NativeScriptEffectProgram* program;
    NativeMatchPoolReference creator,members[EF_MODEL_GROUP_MAX];
    uint32_t creator_generation,generations[EF_MODEL_GROUP_MAX],member_count;
    uint64_t program_identity,creator_layout,creator_parameters;
    NativeScriptReceiverView creator_view;
    HSD_JObj* origin;
    NativeScriptEffectOrigin origin_identity;
    uint32_t tick,completed,running,remaining;
    bool active,faulted,member_live[EF_MODEL_GROUP_MAX];
    uint8_t bytes[NATIVE_SCRIPT_EFFECT_STATE_BYTES];
    NativeScriptStorageInstance storage;
    NativeScriptExecution execution;
} NativeScriptEffectActor;
extern NativeScriptEffectActor nativeScriptEffectActors[NATIVE_SCRIPT_EFFECT_ACTORS];
extern uint16_t nativeScriptEffectByObject[NATIVE_GOBJ_CAPACITY];
extern uint32_t nativeScriptEffectInitializedCount;
extern uint64_t nativeScriptEffectEpoch;
bool nativeScriptEffect_Boundary(void);
bool nativeScriptEffect_Peers(const NativeScriptEffectActor*);
bool nativeScriptEffect_Bind(NativeScriptEffectActor*,const NativeScriptEffectProgram*,uint32_t generation,NativeScriptDiagnostic*);
#endif
