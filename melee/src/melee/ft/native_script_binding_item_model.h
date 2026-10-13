#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_ITEM_MODEL_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_ITEM_MODEL_H
#include "native_script_vm.h"
#include <sysdolphin/baselib/forward.h>
enum { NATIVE_SCRIPT_ITEM_MODEL_SELECTORS=128 };
typedef struct NativeScriptItemModelAnimationTarget {
    const HSD_Joint* descriptor;
    uint32_t controllers,tracks,uncovered;
    bool valid;
} NativeScriptItemModelAnimationTarget;
typedef struct NativeScriptItemModelSelector {
    uint32_t semantic,path_offset,path_length;
    uint32_t animation; // UINT32_MAX except a prepared named Add operation.
    uint8_t depth,path[12],channel,argument_count,model;
    uint8_t dynamic_bone; // Literal native dynamic table index, independent of path/model.
    NativeScriptItemModelAnimationTarget animation_targets[9];
} NativeScriptItemModelSelector;
struct NativeScriptCompileBank;
struct NativeScriptItemProgram;
bool nativeScriptBindingItemModel_PrepareSelectors(const struct NativeScriptCompileBank*,
    struct NativeScriptItemProgram*,NativeScriptItemModelSelector* destination,
    uint32_t capacity,NativeScriptDiagnostic*);
bool nativeScriptBindingItemModel_Read(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,uint32_t,NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingItemModel_Write(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,uint32_t,const NativeScriptValue*,NativeScriptDiagnostic*);
// Model operations use the exact prepared Article descriptor and literal model
// selectors. Secondary roots retain their registered scene allocation identity.
bool nativeScriptBindingItemModel_Invoke(NativeScriptRuntimeContext*,uint32_t,
    NativeScriptValue*,uint32_t,NativeScriptValue*,NativeScriptDiagnostic*);
#endif
