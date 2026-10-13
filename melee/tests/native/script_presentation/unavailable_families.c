#include <melee/ft/native_script_bindings.h>
// This isolated executable tests presentation and scalar native bindings only.
// Other engine families fail explicitly if reached; no production target uses
// these deliberately unavailable execution boundaries.
#define READ(name) bool name(NativeScriptRuntimeContext* c,uint32_t s,NativeScriptValue* v,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)d;return false;}
#define WRITE(name) bool name(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)d;return false;}
#define CALL(name) bool name(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
READ(nativeScriptBindingKinetics_Read)
WRITE(nativeScriptBindingKinetics_Write)
CALL(nativeScriptBindingKinetics_Invoke)
READ(nativeScriptBindingState_Read)
CALL(nativeScriptBindingState_Invoke)
READ(nativeScriptBindingCombat_Read)
WRITE(nativeScriptBindingCombat_Write)
CALL(nativeScriptBindingCombat_Invoke)
bool nativeScriptBindingSetup_Read(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
bool nativeScriptBindingSetup_Write(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,const NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
CALL(nativeScriptBindingSetup_Invoke)
CALL(nativeScriptBindingTransition_Invoke)
