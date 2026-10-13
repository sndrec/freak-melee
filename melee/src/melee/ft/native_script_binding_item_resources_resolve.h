#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_ITEM_RESOURCES_RESOLVE_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_ITEM_RESOURCES_RESOLVE_H

#include "native_script_binding_item_resources.h"
#include "native_script_binding_spawn.h"

int nativeScriptBindingItemResources_TryResolve(void*,const NativeScriptIRArena*,
    const NativeScriptIRNode*,NativeScriptIRResolution*,NativeScriptDiagnostic*);
bool nativeScriptBindingItemResources_DescribeCall(void*,uint32_t,
    NativeScriptCallLayout*,NativeScriptDiagnostic*);
#endif
