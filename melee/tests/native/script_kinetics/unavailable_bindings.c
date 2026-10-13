#include <melee/ft/native_script_bindings.h>
#include <stdlib.h>

// Used only by the standalone primitive test, which does not link these families.
// Setup entry is outside this kinetic fixture; never simulate success.
bool nativeScriptBindingSetup_Invoke(NativeScriptRuntimeContext* c,uint32_t s,
    const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;abort();}

#define UNAVAILABLE_INVOKE(name) \
 bool name(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d) \
 {(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;abort();}
UNAVAILABLE_INVOKE(nativeScriptBindingState_Invoke)
UNAVAILABLE_INVOKE(nativeScriptBindingCombat_Invoke)
UNAVAILABLE_INVOKE(nativeScriptBindingTransition_Invoke)
#undef UNAVAILABLE_INVOKE
#define UNAVAILABLE_READ(name) \
 bool name(NativeScriptRuntimeContext* c,uint32_t s,NativeScriptValue* r,NativeScriptDiagnostic* d) \
 {(void)c;(void)s;(void)r;(void)d;abort();}
UNAVAILABLE_READ(nativeScriptBindingState_Read)
UNAVAILABLE_READ(nativeScriptBindingCombat_Read)
#undef UNAVAILABLE_READ
bool nativeScriptBindingSetup_Read(NativeScriptRuntimeContext* c,uint32_t s,
 const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;abort();}
bool nativeScriptBindingSetup_Write(NativeScriptRuntimeContext* c,uint32_t s,
 const NativeScriptValue* v,uint32_t n,const NativeScriptValue* r,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;abort();}
bool nativeScriptBindingCombat_Write(NativeScriptRuntimeContext* c,uint32_t s,
 const NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)v;(void)d;abort();}
