#include <melee/ft/native_script_binding_item_terrain.h>
#include <stdio.h>

// Console cohorts without terrain never invent a successful native contact.
bool nativeScriptBindingItemTerrain_Invoke(NativeScriptRuntimeContext* context,
    uint32_t semantic, const NativeScriptValue* arguments, uint32_t count,
    NativeScriptValue* output, NativeScriptDiagnostic* diagnostic)
{
    (void)context; (void)semantic; (void)arguments; (void)count; (void)output;
    if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),
        "Native Item terrain unavailable in this fixture");
    return false;
}
