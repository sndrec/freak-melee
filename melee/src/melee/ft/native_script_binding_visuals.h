#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_VISUALS_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_VISUALS_H
#include "native_script_vm.h"
// Stored requested held-item visibility, independent of held-item/model presence.
// This read requires the caller's exact live source actor/program/storage.
bool nativeScriptBindingVisuals_Read(NativeScriptRuntimeContext*,uint32_t semantic,
 NativeScriptValue* result,NativeScriptDiagnostic*);
// Fighter-only owned source operations; numeric selectors keep original width.
// Native primitives mutate only after every argument/table check succeeds.
// Current selection writes idx and invalidates channel0; saved selection writes
// prev. Queries return the stored signed8 value, or -1 for any out-of-range index.
bool nativeScriptBindingVisuals_Invoke(NativeScriptRuntimeContext*,uint32_t semantic,
 const NativeScriptValue* arguments,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic*);
#endif
