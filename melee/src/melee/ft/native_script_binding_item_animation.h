#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_ITEM_ANIMATION_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_ITEM_ANIMATION_H
#include "native_script_vm.h"
#include <melee/it/native_article_animation_demand.h>

struct ItemStateDesc;
struct NativeScriptItemProgram;
struct NativeScriptCompileBank;
typedef struct NativeScriptItemAnimationDescriptor {
    const struct ItemStateDesc* descriptor;
    const HSD_AnimJoint* anim;
    const HSD_MatAnimJoint* material;
    const HSD_ShapeAnimJoint* shape;
    const void* script;
    NativeArticleAnimationDemand set, add;
    bool set_valid, add_valid;
} NativeScriptItemAnimationDescriptor;

// Loading transition only: exact primary Article state triplets and demand.
bool nativeScriptBindingItemAnimation_PrepareDescriptors(
    const struct NativeScriptCompileBank*, struct NativeScriptItemProgram*,
    NativeScriptItemAnimationDescriptor*, uint32_t capacity,
    NativeScriptDiagnostic*);

// Direct interpreter calls use transition-prepared model/path/bundle selectors.
// Native playback is admitted against the complete model's immutable origins.
bool nativeScriptBindingItemAnimation_Invoke(NativeScriptRuntimeContext*,
    uint32_t semantic, NativeScriptValue*, uint32_t count, NativeScriptValue*,
    NativeScriptDiagnostic*);
#endif
