#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_HELD_ITEMS_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_HELD_ITEMS_H
#include "native_script_vm.h"
// Physical Fighter slot observations, including foreign/ordinary recipients.
// Missing/unlinked/removing items return native defaults. Captured live source
// or slot retirement faults without publishing a replacement/default result.
bool nativeScriptBindingHeldItems_Read(NativeScriptRuntimeContext*,uint32_t,
    NativeScriptValue*,NativeScriptDiagnostic*);
#endif
