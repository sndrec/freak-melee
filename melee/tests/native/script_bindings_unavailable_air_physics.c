#include <melee/ft/native_script_binding_air_physics.h>
// Engine-free fixtures reject the unavailable native physics family.
bool nativeScriptBindingAirPhysics_Invoke(NativeScriptRuntimeContext* c,uint32_t id,
    const NativeScriptValue* in,uint32_t n,NativeScriptValue* out,NativeScriptDiagnostic* d)
{(void)c;(void)id;(void)in;(void)n;(void)out;(void)d;return false;}
