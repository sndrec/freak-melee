#include <melee/ft/native_script_expression.h>
#include <melee/ft/native_script_vm.h>
#include <melee/ft/native_source_package.h>
extern NativeSourcePackage constructor_package;
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_binding_fighter_observations.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/ft/fighter.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/it/types.h>
#include <melee/ef/native_model_effect.h>
#include <melee/ef/efsync.h>
#include <sysdolphin/baselib/gobj.h>
#include <stdio.h>
#include <stdlib.h>
#pragma clang section data = ""
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind k){(void)k;return NULL;}
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind k)
{return k==constructor_package.target&&constructor_package.ready?&constructor_package:NULL;}
bool nativeScriptBindings_Context(void* o,uint32_t k,uint32_t g,NativeScriptRuntimeContext* c,NativeScriptDiagnostic* d)
{(void)d;if(!nativeCallbackSource_Alive(o,g,k))return false;*c=(NativeScriptRuntimeContext){.source=o,.source_class=k,.generation=g,.actor_id=g,.scene_epoch=nativeGObjPool.lifetime};return true;}
#include "source_guards.inc"
void constructor_probe(HSD_GObj*);
bool nativeScriptBindings_Read(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
bool nativeScriptBindings_Write(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,const NativeScriptValue* r,NativeScriptDiagnostic* d)
{
 (void)d;if(s!=NativeScriptBinding_Command||n!=1||!v||!r||!nativeScriptExpression_Assignable(NativeScriptType_I32,r->type)||!nativeScriptBindings_SourceAlive(c))return false;
 NativeScriptValue converted;if(!nativeScriptValue_Convert(r,NativeScriptType_I32,&converted,d))return false;
 Fighter* f=((HSD_GObj*)c->source)->user_data;f->cmd_vars[0]=(uint32_t)converted.as.i64;constructor_probe(c->source);return true;
}
#define READ(n) bool n(NativeScriptRuntimeContext* c,uint32_t s,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)r;(void)d;return false;}
#define WRITE(n) bool n(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)r;(void)d;return false;}
#define CALL(n) bool n(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t z,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)z;(void)r;(void)d;return false;}
READ(nativeScriptBindingKinetics_Read) WRITE(nativeScriptBindingKinetics_Write) CALL(nativeScriptBindingKinetics_Invoke)
READ(nativeScriptBindingState_Read) CALL(nativeScriptBindingState_Invoke)
READ(nativeScriptBindingCombat_Read) WRITE(nativeScriptBindingCombat_Write) CALL(nativeScriptBindingCombat_Invoke)
CALL(nativeScriptBindingSetup_Invoke) CALL(nativeScriptBindingTransition_Invoke)
bool nativeScriptBindingSetup_Read(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
bool nativeScriptBindingSetup_Write(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,const NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
