#include "fixture.h"
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftlib.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/native_item_owner.h>
#include <melee/it/native_article.h>
#include <melee/ef/native_model_effect.h>
#include <melee/it/native_holdable.h>
#include <melee/it/item.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/it/ithitbox.h>
#include <melee/it/inlines.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lbaudio_ax.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
static ItemCommonData lifecycle_common_item;
ItemCommonData* it_804D6D28=&lifecycle_common_item;
void OSReport(char* f,...){(void)f;}
__declspec(noreturn) void __assert(char* file,u32 line,char* reason)
{fprintf(stderr,"%s:%u: %s\n",file,(unsigned)line,reason);abort();}
__declspec(noreturn) void HSD_Panic(char* f,u32 l,char* r){__assert(f,l,r);}
bool nativeFighterPool_CanBeginScene(uint64_t e){return e!=0;}
bool nativeFighterPool_BeginScene(uint64_t e){return e!=0;}
bool nativeFighterPool_RetireScene(void){return true;}
bool nativeModelEffect_Snapshot(HSD_GObj* o,uint32_t* g){(void)o;(void)g;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* o,uint32_t g){(void)o;(void)g;return NULL;}
Fighter_GObj* nativeArticle_Creator(Item_GObj* o){(void)o;return NULL;}
Fighter_GObj* nativeModelEffect_Creator(EF_Effect* o){(void)o;return NULL;}
void HSD_JObjRemoveAll(HSD_JObj* o){(void)o;abort();}
void HSD_LObjRemoveAll(HSD_LObj* o){(void)o;abort();}
void HSD_CObjFree(HSD_CObj* o){(void)o;abort();}
void HSD_FogFree(HSD_Fog* o){(void)o;abort();}
void HSD_LObj_803668EC(HSD_LObj* o){(void)o;abort();}
HSD_CObj* HSD_CObjGetCurrent(void){abort();}
void HSD_LObjSetupInit(HSD_CObj* o){(void)o;abort();}
void HSD_JObjDispAll(HSD_JObj* o,Mtx m,u32 f,u32 d){(void)o;(void)m;(void)f;(void)d;abort();}
void HSD_FogSet(HSD_Fog* o){(void)o;abort();}
bool HSD_CObjSetCurrent(HSD_CObj* o){(void)o;abort();}
void HSD_CObjEndCurrent(void){abort();}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind k){(void)k;return NULL;}
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind k)
{return k==lifecycle_package.target?&lifecycle_package:NULL;}
bool nativeScriptBindings_Context(void* o,uint32_t k,uint32_t g,NativeScriptRuntimeContext* c,NativeScriptDiagnostic* d)
{(void)d;if(!nativeCallbackSource_Alive(o,g,k))return false;*c=(NativeScriptRuntimeContext){.source=o,.source_class=k,.generation=g,.actor_id=g};return true;}
#include "source_guards.inc"
bool nativeScriptBindings_Read(NativeScriptRuntimeContext* c,uint32_t id,const NativeScriptValue* s,uint32_t n,NativeScriptValue* v,NativeScriptDiagnostic* d)
{
 (void)s;(void)d;if(n||c->source_class!=NativeCallbackSource_Fighter||id!=NativeScriptBinding_PositionX)return false;
 v->type=NativeScriptType_F32;v->as.f32=((Fighter*)((HSD_GObj*)c->source)->user_data)->cur_pos.x;++status_reads;
 if(read_trigger&&status_reads==read_trigger)held_mutation(c->source);return true;
}
bool nativeScriptBindings_Write(NativeScriptRuntimeContext* c,uint32_t id,const NativeScriptValue* s,uint32_t n,const NativeScriptValue* v,NativeScriptDiagnostic* d)
{
 (void)s;(void)d;if(n||c->source_class!=NativeCallbackSource_Fighter||id!=NativeScriptBinding_PositionX||v->type!=NativeScriptType_F32)return false;
 ((Fighter*)((HSD_GObj*)c->source)->user_data)->cur_pos.x=v->as.f32;held_mutation(c->source);return true;
}
// Unrelated native families reject explicitly; interpreted callbacks never use
// these leaves. Creator bootstrap remains its actual native body below.
#define READ(n) bool n(NativeScriptRuntimeContext* c,uint32_t s,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)r;(void)d;return false;}
#define WRITE(n) bool n(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)r;(void)d;return false;}
#define CALL(n) bool n(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t z,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)z;(void)r;(void)d;return false;}
READ(nativeScriptBindingKinetics_Read) WRITE(nativeScriptBindingKinetics_Write) CALL(nativeScriptBindingKinetics_Invoke)
READ(nativeScriptBindingState_Read) CALL(nativeScriptBindingState_Invoke)
READ(nativeScriptBindingCombat_Read) WRITE(nativeScriptBindingCombat_Write) CALL(nativeScriptBindingCombat_Invoke)
READ(nativeScriptBindingPresentation_Read) CALL(nativeScriptBindingSetup_Invoke) CALL(nativeScriptBindingTransition_Invoke)
READ(nativeScriptBindingItem_Read) WRITE(nativeScriptBindingItem_Write) CALL(nativeScriptBindingItem_Invoke)
bool nativeScriptBindingPresentation_Invoke(NativeScriptRuntimeContext* c,uint32_t s,NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
bool nativeScriptBindingSetup_Read(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
bool nativeScriptBindingSetup_Write(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,const NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
#include <melee/ef/efsync.h>
const EF_ModelRecipe* efSync_ModelRecipe(s32 id){(void)id;return NULL;}

#include <melee/ft/ftcommon.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/it/native_held_article.h>
#include <melee/it/it_26B1.h>
#define UNUSED_PARASOL(name) void name(HSD_GObj* o,float f){(void)o;(void)f;abort();}
UNUSED_PARASOL(it_8028B718) UNUSED_PARASOL(it_8028B780) UNUSED_PARASOL(it_8028B7E8) UNUSED_PARASOL(it_8028B850)
UNUSED_PARASOL(it_8028B648) UNUSED_PARASOL(it_8028B6B0) UNUSED_PARASOL(it_8028B618) UNUSED_PARASOL(it_802BDD40) UNUSED_PARASOL(it_802BDDB4)
s32 it_8028B08C(void* o,s32 m){(void)o;(void)m;abort();}
float it_802BDA40(HSD_GObj* o,int m){(void)o;(void)m;abort();}
int itGetKind(HSD_GObj* o){return ((Item*)o->user_data)->kind;}
s32 itGetMotionId(HSD_GObj* o){return ((Item*)o->user_data)->msid;}
#include "native_held.inc"
