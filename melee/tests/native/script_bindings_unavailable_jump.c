#include <melee/ft/native_script_binding_jump.h>
// Pure frontend and unrelated primitive fixtures have no jump engine/view.
// Returning false preserves all caller outputs and cannot fake source success.
bool nativeScriptBindingJump_Read(NativeScriptRuntimeContext* c,uint32_t s,
    NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)v;(void)d;return false;}
bool nativeScriptBindingJump_Write(NativeScriptRuntimeContext* c,uint32_t s,
    const NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)v;(void)d;return false;}
bool nativeScriptBindingJump_Invoke(NativeScriptRuntimeContext* c,uint32_t s,
    const NativeScriptValue* a,uint32_t n,NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)a;(void)n;(void)v;(void)d;return false;}
