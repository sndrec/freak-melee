#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_binding_presentation.h>
#include <melee/it/native_script_item_actor.h>
// Deliberately absent unrelated engine families in this isolated Item fixture.
// These boundaries are never compiled into the game or an authored body.
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
READ(nativeScriptBindingPresentation_Read)
CALL(nativeScriptBindingPresentation_AccessoryInvoke)
bool nativeScriptBindingPresentation_Invoke(NativeScriptRuntimeContext* c,uint32_t s,NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
bool nativeScriptBindingSetup_Read(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
bool nativeScriptBindingSetup_Write(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,const NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
CALL(nativeScriptBindingSetup_Invoke)
CALL(nativeScriptBindingTransition_Invoke)

// This primitive-only Item fixture has no authored actor/storage publication.
// Callback registration must reject, rather than inventing a source hook.
bool nativeScriptItem_IsAttached(Item_GObj* object)
{(void)object;return false;}
NativeScriptVMStatus nativeScriptItem_Status(Item_GObj* object)
{(void)object;return NativeScriptVM_SourceRetired;}
const NativeScriptItemProgram* nativeScriptItem_Spec(Item_GObj* object)
{(void)object;return NULL;}
bool nativeScriptItem_View(Item_GObj* object,uint32_t generation,NativeScriptReceiverView* view)
{(void)object;(void)generation;(void)view;return false;}
uint32_t nativeScriptItem_CallbackFunction(Item_GObj* object,uint32_t slot)
{(void)object;(void)slot;return UINT32_MAX;}
bool nativeScriptItem_BindCallback(Item_GObj* object,uint32_t slot,uint32_t function)
{(void)object;(void)slot;(void)function;return false;}
