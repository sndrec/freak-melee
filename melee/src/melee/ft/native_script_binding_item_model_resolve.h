#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_ITEM_MODEL_RESOLVE_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_ITEM_MODEL_RESOLVE_H
#include "native_script_binding_spawn.h"
#include "native_script_binding_item_model.h"
#include "native_script_model_metadata.h"
#define NATIVE_SCRIPT_ITEM_MODEL_DOMAIN UINT32_C(0x06000000)
#define NATIVE_SCRIPT_ITEM_MODEL_DOMAIN_MASK UINT32_C(0xff000000)
#define NATIVE_SCRIPT_ITEM_MODEL_PROPERTY UINT32_C(0x06400000)
#define NATIVE_SCRIPT_ITEM_MODEL_WORLD_POSITION UINT32_C(0x06500000)
#define NATIVE_SCRIPT_ITEM_MODEL_TRANSFORM UINT32_C(0x06600000)
#define NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADD UINT32_C(0x06700000)
#define NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_REQUEST UINT32_C(0x06800000)
#define NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADVANCE UINT32_C(0x06900000)
#define NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_REMOVE UINT32_C(0x06a00000)
#define NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_RATE UINT32_C(0x06b00000)
#define NATIVE_SCRIPT_ITEM_MODEL_ASYNC_EFFECT UINT32_C(0x06c00000)
#define NATIVE_SCRIPT_ITEM_MODEL_DYNAMIC_PROPERTY UINT32_C(0x06d00000)
#define NATIVE_SCRIPT_ITEM_MODEL_MATRIX_ROTATION UINT32_C(0x06e00000)
#define NATIVE_SCRIPT_ITEM_MODEL_SYNC_EFFECT UINT32_C(0x06f00000)
#define NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK UINT32_C(0xfff00000)
#define NATIVE_SCRIPT_ITEM_MODEL_INDEX_MASK UINT32_C(0x000fffff)
enum { NativeScriptItemModel_JointCall=0x06000001,NativeScriptItemModel_JointNamespace,
    NativeScriptItemModel_RotationNamespace,NativeScriptItemModel_ScaleNamespace,
    NativeScriptItemModel_TranslationNamespace,NativeScriptItemModel_WorldPositionCall,
    NativeScriptItemModel_QuaternionNamespace,NativeScriptItemModel_DynamicJointCall,
    NativeScriptItemModel_DynamicJointNamespace };
// Parse one exact root/bundle declaration and its family-scoped ordinal.
// No imported graph or live HSD class is accessed by this source-only parser.
bool nativeScriptBindingItemModel_Declaration(const NativeScriptSourceTree*,
    uint32_t owner,uint32_t node,NativeScriptModelMetadata*,
    uint32_t* ordinal,NativeScriptDiagnostic*);
// Source-aware overload layouts use the exact indexed invocation at preparation.
bool nativeScriptBindingItemModel_DescribeCall(void*,uint32_t,
    NativeScriptCallLayout*,NativeScriptDiagnostic*);
int nativeScriptBindingItemModel_TryResolve(void*,const NativeScriptIRArena*,
    const NativeScriptIRNode*,NativeScriptIRResolution*,NativeScriptDiagnostic*);
#endif
