#include <melee/ft/native_script_binding_spawn.h>

// Older isolated native-family fixtures have no prepared source entity assets
// or factories. Keep that boundary explicitly unavailable; connected spawn
// fixtures link the substantive binding/factory units instead.
unsigned script_unavailable_spawn_calls;
uint32_t script_unavailable_spawn_semantic,script_unavailable_spawn_count;
NativeScriptRuntimeContext* script_unavailable_spawn_context;
NativeScriptValue script_unavailable_spawn_arguments[16];
bool nativeScriptBindingSpawn_Read(NativeScriptRuntimeContext* context,uint32_t semantic,
    NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{(void)context;(void)semantic;(void)result;(void)diagnostic;return false;}
bool nativeScriptBindingSpawn_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* arguments,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    ++script_unavailable_spawn_calls;script_unavailable_spawn_context=context;
    script_unavailable_spawn_semantic=semantic;script_unavailable_spawn_count=count;
    for(uint32_t i=0;i<count&&i<16;++i)script_unavailable_spawn_arguments[i]=arguments[i];
    (void)result;(void)diagnostic;return false;
}
