#include <melee/ft/native_fighter_runtime.h>
#include "../script_item_actor/boundary.h"
#include "fixture.h"
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef MELEE_NATIVE
#pragma clang section data = ""
#endif
bool fixture_remove,fixture_transition;
unsigned fixture_native_frees,fixture_destroy_writes;
int32_t fixture_destroy_count;
bool fixture_destroy_marked,fixture_deferred_alive;
void OSReport(char* format,...){(void)format;}
__declspec(noreturn) void __assert(char* file,u32 line,char* reason)
{fprintf(stderr,"%s:%u: %s\n",file,(unsigned)line,reason);abort();}
__declspec(noreturn) void HSD_Panic(char* file,u32 line,char* reason){__assert(file,line,reason);}
void fixture_userdata_remove(void* data)
{
    Item* item=data;nativeScriptItem_Detach(item->entity);++fixture_native_frees;
}
bool nativeFighterPool_CanBeginScene(uint64_t epoch){return epoch!=0;}
bool nativeFighterPool_BeginScene(uint64_t epoch){return epoch!=0;}
bool nativeFighterPool_RetireScene(void){return true;}



bool nativeModelEffect_Snapshot(void* object,uint32_t* generation){(void)object;(void)generation;return false;}
void* nativeModelEffect_Resolve(void* object,uint32_t generation){(void)object;(void)generation;return NULL;}
void HSD_JObjRemoveAll(HSD_JObj* object){(void)object;abort();}
void HSD_LObjRemoveAll(HSD_LObj* object){(void)object;abort();}
void HSD_CObjFree(HSD_CObj* object){(void)object;abort();}
void HSD_FogFree(HSD_Fog* object){(void)object;abort();}
void HSD_LObj_803668EC(HSD_LObj* object){(void)object;abort();}
HSD_CObj* HSD_CObjGetCurrent(void){abort();}
void HSD_LObjSetupInit(HSD_CObj* object){(void)object;abort();}
void HSD_JObjDispAll(HSD_JObj* object,Mtx matrix,u32 flags,u32 mode){(void)object;(void)matrix;(void)flags;(void)mode;abort();}
void HSD_FogSet(HSD_Fog* object){(void)object;abort();}
bool HSD_CObjSetCurrent(HSD_CObj* object){(void)object;abort();}
void HSD_CObjEndCurrent(void){abort();}
bool nativeScriptBindings_Context(void* object,uint32_t kind,uint32_t generation,
    NativeScriptRuntimeContext* context,NativeScriptDiagnostic* diagnostic)
{
    (void)diagnostic;if(!nativeCallbackSource_Alive(object,generation,kind))return false;
    *context=(NativeScriptRuntimeContext){.source=object,.source_class=kind,.generation=generation,.actor_id=generation};return true;
}
bool nativeScriptBindings_SourceAlive(NativeScriptRuntimeContext* context)
{return context&&nativeCallbackSource_Alive(context->source,context->generation,context->source_class);}
bool nativeScriptBindings_Read(NativeScriptRuntimeContext* c,uint32_t id,const NativeScriptValue* selectors,
    uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* d)
{(void)c;(void)id;(void)selectors;(void)count;(void)result;(void)d;return false;}
bool nativeScriptBindings_Write(NativeScriptRuntimeContext* c,uint32_t id,const NativeScriptValue* selectors,uint32_t count,const NativeScriptValue* value,NativeScriptDiagnostic* d){
 (void)selectors;(void)d;if(id!=NativeScriptBinding_PositionX||count||value->type!=NativeScriptType_F32||c->source_class!=NativeCallbackSource_Fighter)return false;
 ((Fighter*)((HSD_GObj*)c->source)->user_data)->cur_pos.x=value->as.f32;
 if(reference_mode){HSD_GObj* target=reference_victim;reference_mode=0;HSD_GObjFree(target);if(reference_reuse)reference_reuse();}return true;
}

// Unrelated native leaf families are unavailable in this owner/VM fixture.
#define READ(name) bool name(NativeScriptRuntimeContext* c,uint32_t s,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)r;(void)d;return false;}
#define WRITE(name) bool name(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)r;(void)d;return false;}
#define CALL(name) bool name(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
READ(nativeScriptBindingKinetics_Read) WRITE(nativeScriptBindingKinetics_Write) CALL(nativeScriptBindingKinetics_Invoke)
READ(nativeScriptBindingState_Read) CALL(nativeScriptBindingState_Invoke)
READ(nativeScriptBindingCombat_Read) WRITE(nativeScriptBindingCombat_Write) CALL(nativeScriptBindingCombat_Invoke)
READ(nativeScriptBindingPresentation_Read)
CALL(nativeScriptBindingSetup_Invoke) CALL(nativeScriptBindingTransition_Invoke)
bool nativeScriptBindingSetup_Read(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
bool nativeScriptBindingSetup_Write(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,const NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
bool nativeScriptBindingPresentation_Invoke(NativeScriptRuntimeContext* c,uint32_t s,NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
READ(nativeScriptBindingItem_Read) WRITE(nativeScriptBindingItem_Write) CALL(nativeScriptBindingItem_Invoke)

// This fixture has only Item actors; Fighter views/helpers and entity creators
// are absent. Item lookup still executes the actual actor-owned storage path.



const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind){(void)kind;return NULL;}

#include <melee/ef/efsync.h>
// No effect declarations exist in this actor signal fixture.
const EF_ModelRecipe* efSync_ModelRecipe(s32 id){(void)id;return NULL;}
