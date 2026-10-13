#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_SIGNAL_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_SIGNAL_H
#include "native_script_vm.h"
// Encoded semantic selects a prepared callback; text names are load-only.
bool nativeScriptBindingSignal_Invoke(NativeScriptRuntimeContext*,uint32_t semantic,
 const NativeScriptValue* arguments,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic*);
#endif
