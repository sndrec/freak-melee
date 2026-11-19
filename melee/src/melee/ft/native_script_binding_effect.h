#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_EFFECT_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_EFFECT_H
#include "native_script_binding_spawn.h"
#define NATIVE_SCRIPT_EFFECT_DOMAIN UINT32_C(0x03000000)
#define NATIVE_SCRIPT_EFFECT_DOMAIN_MASK UINT32_C(0xff000000)
#define NATIVE_SCRIPT_EFFECT_PROPERTY UINT32_C(0x03400000)
#define NATIVE_SCRIPT_EFFECT_PROPERTY_MASK UINT32_C(0xfff00000)
#define NATIVE_SCRIPT_EFFECT_INDEX_MASK UINT32_C(0x000fffff)
#define NATIVE_SCRIPT_EFFECT_SPAWN UINT32_C(0x03100000)
#define NATIVE_SCRIPT_EFFECT_SPAWN_ON_PART UINT32_C(0x03500000)
#define NATIVE_SCRIPT_EFFECT_SPAWN_DEFERRED_POSITION UINT32_C(0x03600000)
#define NATIVE_SCRIPT_EFFECT_SPAWN_ITEM_POSITION UINT32_C(0x03700000)
#define NATIVE_SCRIPT_EFFECT_CANCEL UINT32_C(0x03200000)
enum {NativeScriptEffectNamespace=0x03000001,NativeScriptEffectJointCall,
    NativeScriptEffectJointNamespace,NativeScriptEffectRotationNamespace,
    NativeScriptEffectSpawnCall,NativeScriptEffectCancelCall,
    NativeScriptEffectTranslationNamespace,NativeScriptEffectScaleNamespace};
// Uses the same substantive indexed-program/scope context as source factories.
int nativeScriptBindingEffect_TryResolve(void*,const NativeScriptIRArena*,const NativeScriptIRNode*,NativeScriptIRResolution*,NativeScriptDiagnostic*);
bool nativeScriptBindingEffect_DescribeCall(void*,uint32_t,NativeScriptCallLayout*,NativeScriptDiagnostic*);
bool nativeScriptBindingEffect_Read(NativeScriptRuntimeContext*,uint32_t,const NativeScriptValue*,uint32_t,NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingEffect_Write(NativeScriptRuntimeContext*,uint32_t,const NativeScriptValue*,uint32_t,const NativeScriptValue*,NativeScriptDiagnostic*);
// Engine spawn_on_part is synchronous, uses a signed raw rig slot and never
// remaps a semantic bone role. spawn_between consumes two raw slots through
// its exact synchronous two-joint ABI. An absent slot is a healthy no-op.
bool nativeScriptBindingEffect_Invoke(NativeScriptRuntimeContext*,uint32_t,const NativeScriptValue*,uint32_t,NativeScriptValue*,uint32_t,NativeScriptValue*,NativeScriptDiagnostic*);
struct NativeScriptEffectProgram;
struct NativeScriptCompileBank;
bool nativeScriptBindingEffect_PrepareSelectors(const struct NativeScriptCompileBank*,struct NativeScriptEffectProgram*,NativeScriptDiagnostic*);
#endif
