#include <melee/ft/native_script_bindings.h>
#include <melee/ef/native_model_effect.h>
#include <melee/it/native_script_item_actor.h>
#include <stdlib.h>
#include <melee/gr/stage.h>
// Stage-kind reads are outside this command/scalar fixture's controlled leaves.
StKind Stage_80225194(void){abort();}
#define READ(name) bool name(NativeScriptRuntimeContext*c,uint32_t s,NativeScriptValue*v,NativeScriptDiagnostic*d){(void)c;(void)s;(void)v;(void)d;return false;}
#define WRITE(name) bool name(NativeScriptRuntimeContext*c,uint32_t s,const NativeScriptValue*v,NativeScriptDiagnostic*d){(void)c;(void)s;(void)v;(void)d;return false;}
#define CALL(name) bool name(NativeScriptRuntimeContext*c,uint32_t s,const NativeScriptValue*a,uint32_t n,NativeScriptValue*v,NativeScriptDiagnostic*d){(void)c;(void)s;(void)a;(void)n;(void)v;(void)d;return false;}
READ(nativeScriptBindingKinetics_Read) WRITE(nativeScriptBindingKinetics_Write) CALL(nativeScriptBindingKinetics_Invoke)
READ(nativeScriptBindingState_Read) CALL(nativeScriptBindingState_Invoke)
READ(nativeScriptBindingCombat_Read) WRITE(nativeScriptBindingCombat_Write) CALL(nativeScriptBindingCombat_Invoke)
CALL(nativeScriptBindingSetup_Invoke) CALL(nativeScriptBindingTransition_Invoke)
bool nativeScriptBindingSetup_Read(NativeScriptRuntimeContext*c,uint32_t s,const NativeScriptValue*a,uint32_t n,NativeScriptValue*v,NativeScriptDiagnostic*d){(void)c;(void)s;(void)a;(void)n;(void)v;(void)d;return false;}
bool nativeScriptBindingSetup_Write(NativeScriptRuntimeContext*c,uint32_t s,const NativeScriptValue*a,uint32_t n,const NativeScriptValue*v,NativeScriptDiagnostic*d){(void)c;(void)s;(void)a;(void)n;(void)v;(void)d;return false;}
bool nativeModelEffect_Snapshot(HSD_GObj* g,uint32_t* generation){(void)g;(void)generation;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* g,uint32_t generation){(void)g;(void)generation;return NULL;}
bool nativeScriptItem_IsAttached(Item_GObj* o){(void)o;abort();}
NativeScriptVMStatus nativeScriptItem_Status(Item_GObj* o){(void)o;abort();}
const NativeScriptItemProgram* nativeScriptItem_Spec(Item_GObj* o){(void)o;abort();}
bool nativeScriptItem_View(Item_GObj* o,uint32_t g,NativeScriptReceiverView* v){(void)o;(void)g;(void)v;abort();}
