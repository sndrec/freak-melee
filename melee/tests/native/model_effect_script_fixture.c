#include <melee/ef/native_script_effect_actor.h>
#include <stdlib.h>

// These native policy tests construct effects without a script actor.
bool nativeScriptEffect_IsAttached(HSD_GObj* object) { return false; }
EF_Effect* nativeScriptEffect_Resolve(HSD_GObj* object, uint32_t generation) { abort(); }
bool nativeScriptEffect_Snapshot(HSD_GObj* object, uint32_t* generation) { abort(); }
Fighter_GObj* nativeScriptEffect_Creator(HSD_GObj* object, uint32_t generation) { abort(); }
HSD_JObj* nativeScriptEffect_Joint(HSD_GObj* object, uint32_t generation, unsigned i) { abort(); }
NativeScriptVMStatus nativeScriptEffect_Run(HSD_GObj* object) { abort(); }
NativeScriptVMStatus nativeScriptEffect_ProcessDeferred(Fighter_GObj* object,
    const struct EF_QueuedEffect* request) { abort(); }
