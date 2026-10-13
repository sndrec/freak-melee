#include <melee/ft/native_script_binding_pose.h>
// This compiler/metadata fixture does not execute poses. Actual engine operations
// are covered by the separate direct-pose runtime fixture, never a success stub.
bool nativeScriptBindingPose_Invoke(NativeScriptRuntimeContext* c,uint32_t s,
 const NativeScriptValue* a,uint32_t n,NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)a;(void)n;(void)v;(void)d;return false;}

bool nativeScriptBindingTransition_Invoke(NativeScriptRuntimeContext* c,uint32_t s,
 const NativeScriptValue* a,uint32_t n,NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)a;(void)n;(void)v;(void)d;return false;}
