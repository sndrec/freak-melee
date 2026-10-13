#include "fixture.h"
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/native_script_binding_article_motion.h>
#include <melee/ft/native_script_binding_common_properties.h>
#include <sysdolphin/baselib/jobj.h>
#include <melee/it/itgroundcoll.h>
#include <melee/mp/mpcoll.h>

#include <melee/it/itcoll.h>
#include <melee/it/it_2725.h>
#include <melee/it/inlines.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lbcollision.h>
#include <melee/db/db.h>

#include <melee/ft/fighter.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/it/types.h>
#include <melee/it/native_holdable.h>
#include <melee/it/ithitbox.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/ef/native_model_effect.h>
#include <melee/ef/efsync.h>
#include <sysdolphin/baselib/gobj.h>
#include <stdio.h>
#include <stdlib.h>
#pragma clang section data = ""
void OSReport(char* f,...){(void)f;}
__declspec(noreturn) void __assert(char* f,u32 l,char* r){fprintf(stderr,"%s:%u %s\n",f,(unsigned)l,r);abort();}
__declspec(noreturn) void HSD_Panic(char* f,u32 l,char* r){__assert(f,l,r);}
bool nativeFighterPool_CanBeginScene(uint64_t e){return e!=0;}
bool nativeFighterPool_BeginScene(uint64_t e){return e!=0;}
bool nativeFighterPool_RetireScene(void){return true;}
bool nativeModelEffect_Snapshot(HSD_GObj* o,uint32_t* g){(void)o;(void)g;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* o,uint32_t g){(void)o;(void)g;return NULL;}
const EF_ModelRecipe* efSync_ModelRecipe(s32 id){(void)id;return NULL;}
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
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind k){return k==motion_package.target?&motion_package:NULL;}
bool nativeScriptBindings_Context(void* o,uint32_t k,uint32_t g,NativeScriptRuntimeContext* c,NativeScriptDiagnostic* d)
{(void)d;if(!nativeCallbackSource_Alive(o,g,k))return false;*c=(NativeScriptRuntimeContext){.source=o,.source_class=k,.generation=g,.actor_id=g};return true;}
#include "source_guards.inc"
// All unrelated native families are explicitly unavailable. Target output
// publication goes through real Execution/Storage, without a successful stub.
bool nativeScriptBindings_Write(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,const NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
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
int db_ShowItemCollisionBubbles;
void it_80276214(Item_GObj* o){(void)o;motion_calls=motion_calls*10+1;}
// Attachment release has its own grouped native proof; it is unselected here.
void it_8027429C(Item_GObj* o,Vec3* v){(void)o;(void)v;abort();}
bool mpColl_800477E0(CollData* c){motion_path=1;motion_calls=motion_calls*10+2;c->cur_pos=(Vec3){4,5,6};c->floor.index=17;return collision_result;}
bool mpColl_800471F8(CollData* c){motion_path=2;motion_calls=motion_calls*10+3;c->cur_pos=(Vec3){7,8,9};c->floor.index=23;return collision_result;}
#include "native_motion.inc"
// The selected reflected entry runs actual native ordering and the actual
// authored callback path. Hitbox damage, removal and NativeHoldable branches
// are outside this cohort and abort if reached.
static ItemCommonData motion_item_common={.xD8=100};
ItemCommonData* it_804D6D28=&motion_item_common;
void it_80272460(HitCapsule* h,u32 damage,Item_GObj* o){(void)h;(void)damage;(void)o;abort();}
void Item_8026A8EC(Item_GObj* o){(void)o;abort();}
#define HOLDABLE_UNAVAILABLE(n) NativeScriptVMStatus n(Item_GObj* o,NativeScriptValue* r){(void)o;(void)r;abort();}
HOLDABLE_UNAVAILABLE(nativeHoldable_Hit)
HOLDABLE_UNAVAILABLE(nativeHoldable_Clanked)
HOLDABLE_UNAVAILABLE(nativeHoldable_Reflected)
HOLDABLE_UNAVAILABLE(nativeHoldable_ShieldBounced)
#include "native_reflected.inc"

// Unselected common scalar/class paths remain unavailable in this relationship cohort.
ftCommonData* p_ftCommonData;
HSD_JObjInfo hsdJObj;
bool hsdObjIsDescendantOf(HSD_Obj* o,HSD_ClassInfo* p){(void)o;(void)p;abort();}
