#ifndef MELEE_EF_NATIVE_SCRIPT_EFFECT_SNAPSHOT_H
#define MELEE_EF_NATIVE_SCRIPT_EFFECT_SNAPSHOT_H
#include "native_script_effect_actor.h"
typedef struct NativeScriptEffectSnapshotHeader {uint64_t epoch;uint32_t count,bytes,initialized_count;} NativeScriptEffectSnapshotHeader;
typedef struct NativeScriptEffectSnapshotRecord {
    NativeMatchPoolReference creator,members[EF_MODEL_GROUP_MAX];
    HSD_JObj* origin;
    NativeScriptEffectOrigin origin_identity;
    uint64_t program_identity,layout_identity,parameter_identity,creator_layout,creator_parameters;
    uint32_t slot,author,owner,member_count,creator_generation,generations[EF_MODEL_GROUP_MAX];
    uint32_t tick,completed,faulted,state_bytes;
} NativeScriptEffectSnapshotRecord;
bool nativeScriptEffect_Save(void*,size_t capacity,size_t* written);
bool nativeScriptEffect_ValidateSnapshot(const void*,size_t bytes);
// Aggregate restore first resurrects native GObj/EF/JObj and creator interpreted
// peers. Load only rebinds this component's views; no constructors/callbacks.
bool nativeScriptEffect_Load(const void*,size_t bytes);
#endif
