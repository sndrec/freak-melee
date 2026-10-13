#include "boundary.h"
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_script_receiver.h>
#include <melee/ft/native_script_actor_access.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/native_item_lifetime.h>
#include <melee/it/item.h>
#include <melee/it/native_item_pool.h>
#include <melee/it/inlines.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/it/it_279C.h>
#include <melee/it/it_3F14.h>
#include <melee/it/it_3F2F.h>
#include <melee/it/ithitbox.h>
#include <melee/it/itanimlist.h>
#include <melee/it/itgroundcoll.h>
#include <melee/it/itmaterial.h>
#include <melee/it/native_item_behavior.h>
#include <melee/it/native_item_owner.h>
#include <melee/it/native_holdable.h>
#include <melee/it/native_article_resource.h>
#include <melee/it/native_article_model.h>
#include <melee/mp/mpcoll.h>
#include <melee/mp/mplib.h>
#include <melee/gr/stage.h>
#include <melee/gr/grlib.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lb_00F9.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/cm/camera.h>
#include <melee/ef/efasync.h>
#include <melee/ef/eflib.h>
#include <melee/ft/ftlib.h>
#include <melee/db/db.h>
#include <melee/gm/native_replay.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#pragma clang section data = ""

unsigned scheduler_tail,scheduler_native,scheduler_frees,scheduler_models;
bool scheduler_remove,scheduler_nested,scheduler_fail_selector;int32_t scheduler_destroyed;
bool scheduler_fail_write;
bool scheduler_phase_transition;
HSD_GObj* scheduler_retire_target;
HSD_GObj* scheduler_remove_other;
HSD_GObj* scheduler_clear_owner;
HSD_GObj* (*scheduler_spawn_on_write)(void);
const NativeScriptItemProgram* scheduler_spec;
static ItemAttr attributes;static ItemCommonData common;
static HSD_JObj joints[32];static unsigned joint_count;
ItemCommonData* it_804D6D28=&common;u32 it_804D6D10;
Fighter_804D653C_t* it_804D6D04;
static Fighter_804D653C_t colors[1];
struct sdata_ItemGXLink it_803F2F28[118],it_803F4CA8[30],it_803F1418[43],it_803F2310[47];
static HSD_ObjAllocData item_alloc_data,item_dynamic_bones_alloc_data;
static bool itemCallbackCurrent(HSD_GObj*,uint32_t);
static bool Item_8026A810(HSD_GObj*);
static void Item_802675A8(HSD_GObj* o){(void)o;++scheduler_tail;}
static void func_8026A8EC_inline1(HSD_GObj* o){(void)o;}
static void func_8026A8EC_inline3(HSD_GObj* o){(void)o;}
static void DestroyItemInline(HSD_GObj* o,Item* i){(void)o;(void)i;}
static void ItemSwitch(HSD_GObj* o){(void)o;}
static void Item_8026A0A0(HSD_GObj* o){(void)o;}
static void Item_8026A0FC(HSD_GObj* o){(void)o;}
static void Item_80269CC4(HSD_GObj* o){(void)o;++scheduler_tail;}
static void Item_8026A788(HSD_GObj* o){(void)o;}
static void Item_80269C5C(HSD_GObj* o){(void)o;}
static bool Item_802696CC(HSD_GObj* o){(void)o;return false;}
static void func_8026A158_helper(HSD_GObj* o){(void)o;}
static void func_8026A1E8_inline(HSD_GObj* o){(void)o;}
static void Item_8026B0B4(HSD_GObj* o){(void)o;}
static bool Item_8026784C(enum_t type,int kind){(void)type;(void)kind;return false;}
static void Item_8026814C(HSD_GObj* o){(void)o;}
static void Item_80268560(HSD_GObj* o){(void)o;}
static bool Item_802682F0(HSD_GObj* o){(void)o;return true;}
static void Item_80267130(HSD_GObj* o,SpawnItem* s){(void)o;(void)s;}
static void Item_802676F4(HSD_GObj* o){(void)o;}
static void foobar(HSD_GObj* o){(void)o;++scheduler_tail;}
static void foobar2(HSD_GObj* o){(void)o;}
static void foobar3(HSD_GObj* o){(void)o;}

void scheduler_reset(void){scheduler_tail=scheduler_native=scheduler_models=0;scheduler_remove=scheduler_nested=scheduler_fail_selector=false;common.xD8=100;it_804D6D04=colors;joint_count=0;}
void OSReport(char* f,...){(void)f;}
__declspec(noreturn) void __assert(char* f,u32 l,char* r){fprintf(stderr,"%s:%u %s\n",f,(unsigned)l,r);abort();}
__declspec(noreturn) void HSD_Panic(char* f,u32 l,char* r){__assert(f,l,r);}
bool nativeFighterPool_CanBeginScene(uint64_t e){return e!=0;}
bool nativeFighterPool_BeginScene(uint64_t e){return e!=0;}
bool nativeFighterPool_RetireScene(void){return true;}
bool nativeScriptFighter_CanBeginScene(uint64_t e){return e&&nativeScriptRuntime_Idle();}
bool nativeScriptFighter_BeginScene(uint64_t e){return e&&nativeScriptRuntime_Idle();}
bool nativeScriptFighter_RetireScene(void){return nativeScriptRuntime_Idle();}
bool nativeScriptFighter_View(Fighter_GObj* a,uint32_t g,NativeScriptReceiverView* r){(void)a;(void)g;(void)r;return false;}
NativeScriptVMStatus nativeScriptFighter_Helper(Fighter_GObj* a,uint32_t g,uint32_t f,const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)a;(void)g;(void)f;(void)v;(void)n;(void)r;(void)d;return NativeScriptVM_Error;}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind k){(void)k;return NULL;}
bool nativeModelEffect_Snapshot(void* o,uint32_t* g){(void)o;(void)g;return false;}
void* nativeModelEffect_Resolve(void* o,uint32_t g){(void)o;(void)g;return NULL;}
Fighter_GObj* nativeModelEffect_Creator(void* o){(void)o;return NULL;}
Fighter_GObj* nativeArticle_Creator(Item_GObj* o){(void)o;return NULL;}
void HSD_JObjRemoveAll(HSD_JObj* o){(void)o;}
void HSD_LObjRemoveAll(HSD_LObj* o){(void)o;abort();}
void HSD_CObjFree(HSD_CObj* o){(void)o;abort();}
void HSD_FogFree(HSD_Fog* o){(void)o;abort();}
void HSD_LObj_803668EC(HSD_LObj* o){(void)o;abort();}
HSD_CObj* HSD_CObjGetCurrent(void){abort();}
void HSD_LObjSetupInit(HSD_CObj* o){(void)o;abort();}
void HSD_JObjDispAll(HSD_JObj* o,Mtx m,u32 f,u32 mode){(void)o;(void)m;(void)f;(void)mode;abort();}
void HSD_FogSet(HSD_Fog* o){(void)o;abort();}
bool HSD_CObjSetCurrent(HSD_CObj* o){(void)o;abort();}
void HSD_CObjEndCurrent(void){abort();}
void HSD_JObjSetMtxDirtySub(HSD_JObj* o){o->flags|=JOBJ_MTX_DIRTY;}
u32 HSD_JObjGetFlags(HSD_JObj* o){return o?o->flags:0;}
void HSD_JObjSetFlags(HSD_JObj* o,u32 f){if(o)o->flags|=f;}
void HSD_JObjAnimAll(HSD_JObj* o){(void)o;}
void HSD_JObjRemoveAnimAll(HSD_JObj* o){(void)o;}
HSD_JObj* HSD_JObjRemove(HSD_JObj* o){(void)o;return NULL;}
void* HSD_ObjAlloc(HSD_ObjAllocData* data){(void)data;abort();}
void HSD_ObjFree(HSD_ObjAllocData* data,void* object){(void)data;(void)object;++scheduler_frees;}
void nativeArticleResource_CancelGeneration(Item_GObj* o,uint32_t g){(void)o;(void)g;}
void nativeArticleModel_Destroy(Item_GObj* o,unsigned model){(void)o;(void)model;}
void nativeReplay_LogItemEvent(ItemKind kind,const char* event){(void)kind;(void)event;}
void efLib_DestroyAll(HSD_GObj* o){(void)o;}
void efAsync_QueueClear(void* q){(void)q;}
void efAsync_QueueFlush(HSD_GObj* o,void* q){(void)q;if(scheduler_nested){scheduler_nested=false;Item_80268E5C(o,1,0);}}
bool nativeScriptBindings_Context(void* o,uint32_t k,uint32_t g,NativeScriptRuntimeContext* c,NativeScriptDiagnostic* d){(void)d;if(!nativeCallbackSource_Alive(o,g,k))return false;*c=(NativeScriptRuntimeContext){.source=o,.source_class=k,.generation=g,.actor_id=g};return true;}
bool nativeScriptBindings_SourceAlive(NativeScriptRuntimeContext* c){return c&&nativeCallbackSource_Alive(c->source,c->generation,c->source_class);}
bool nativeScriptBindings_Read(NativeScriptRuntimeContext* c,uint32_t id,const NativeScriptValue* s,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)id;(void)s;(void)n;(void)r;(void)d;return false;}
bool nativeScriptBindings_Write(NativeScriptRuntimeContext* c,uint32_t id,const NativeScriptValue* s,uint32_t n,const NativeScriptValue* v,NativeScriptDiagnostic* d){(void)s;(void)d;if(id!=NativeScriptBinding_PositionX||n||v->type!=NativeScriptType_F32)return false;if(scheduler_fail_write)return false;Item* item=((HSD_GObj*)c->source)->user_data;item->pos.x=v->as.f32;if(scheduler_clear_owner==c->source){scheduler_clear_owner=NULL;item->owner=NULL;}if(scheduler_phase_transition){scheduler_phase_transition=false;Item_80268E5C(c->source,1,0);}if(scheduler_remove){scheduler_remove=false;Item_8026A8EC(c->source);}if(scheduler_retire_target){HSD_GObj* target=scheduler_retire_target;scheduler_retire_target=NULL;HSD_GObjFree(target);}if(scheduler_remove_other){HSD_GObj* other=scheduler_remove_other;scheduler_remove_other=NULL;Item_8026A8EC(other);}if(scheduler_spawn_on_write){HSD_GObj*(*create)(void)=scheduler_spawn_on_write;scheduler_spawn_on_write=NULL;create();}return true;}
// Unrelated native VM operation families are unavailable, never surrogate bodies.
#define READ(name) bool name(NativeScriptRuntimeContext* c,uint32_t s,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)r;(void)d;return false;}
#define WRITE(name) bool name(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)r;(void)d;return false;}
#define CALL(name) bool name(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
READ(nativeScriptBindingKinetics_Read) WRITE(nativeScriptBindingKinetics_Write) CALL(nativeScriptBindingKinetics_Invoke)
READ(nativeScriptBindingState_Read) CALL(nativeScriptBindingState_Invoke)
READ(nativeScriptBindingCombat_Read) WRITE(nativeScriptBindingCombat_Write) CALL(nativeScriptBindingCombat_Invoke)
CALL(nativeScriptBindingSetup_Invoke) CALL(nativeScriptBindingTransition_Invoke)
READ(nativeScriptBindingItem_Read) WRITE(nativeScriptBindingItem_Write) CALL(nativeScriptBindingItem_Invoke)
bool nativeScriptBindingSetup_Read(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
bool nativeScriptBindingSetup_Write(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,const NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}

#define LEAF(name) void name(HSD_GObj* o){(void)o;++scheduler_tail;}
LEAF(it_8026BDCC) LEAF(it_80274EF8) LEAF(it_80274740) LEAF(it_802725D4)
LEAF(it_802714C0) LEAF(it_802721B8) LEAF(it_80272298) LEAF(it_80279BE0) LEAF(it_802799E4)
LEAF(it_802756E0) LEAF(it_8027574C) LEAF(it_8026C368_stub)
LEAF(it_80271A58) LEAF(it_80272280) LEAF(it_802722B0) LEAF(it_8027137C) LEAF(it_8027146C)
LEAF(it_802701BC) LEAF(it_80273484) LEAF(it_8027346C) LEAF(it_80274A64)
LEAF(it_80273168) LEAF(it_802731A4) LEAF(it_802731E0) LEAF(it_802754D4) LEAF(it_80275390)
LEAF(it_80276CEC) LEAF(it_802734B4) LEAF(it_8027163C) LEAF(it_80275474) LEAF(it_80275504)
LEAF(it_802728C8)
void it_80279BBC(Item* item){(void)item;++scheduler_tail;}
void Item_8026B074(Item* item){(void)item;++scheduler_tail;}
bool it_80274C78(HSD_GObj* o){(void)o;return false;}
bool it_8026B6C8(HSD_GObj* o){(void)o;return false;}
bool it_8026D604(HSD_GObj* o){(void)o;return false;}
bool it_80277040(HSD_GObj* o){(void)o;return true;}
bool it_80272D1C(HSD_GObj* o){return o&&o->classifier==HSD_GOBJ_CLASS_ITEM;}
void it_802762B0(Item* item){item->ground_or_air=GA_Air;}
float it_8026B424(int value){return (float)value;}
void it_8027737C(HSD_GObj* o,Vec3* v){(void)o;(void)v;}
void it_80273748(HSD_GObj* o,Vec3* a,Vec3* b){(void)o;(void)a;(void)b;}
void it_80273B50(HSD_GObj* o,Vec3* p){(void)o;(void)p;}
void it_80274198(HSD_GObj* o,bool flag){(void)o;(void)flag;++scheduler_tail;}
void it_802741F4(HSD_GObj* o,bool flag){(void)o;(void)flag;++scheduler_tail;}
void it_802742F4(HSD_GObj* o,HSD_GObj* target,Fighter_Part part){(void)part;((Item*)o->user_data)->owner=target;}
void it_80272460(HitCapsule* hit,u32 damage,HSD_GObj* o){(void)o;hit->damage=(float)damage;}
void Item_802694CC(HSD_GObj* o){(void)o;}
void Item_80268D34(HSD_GObj* o,ItemStateDesc* descriptor){(void)o;(void)descriptor;}
unsigned scheduler_stock_assets;
void Item_80267978(HSD_GObj* o){++scheduler_stock_assets;((Item*)o->user_data)->xC4_article_data=(Article*)scheduler_spec->article;}
bool nativeItemBehavior_Select(Item_GObj* o,const SpawnItem* spawn,NativeItemBehaviorSelection s,FighterKind author,uint32_t owner)
{(void)spawn;(void)s;Item* i=o->user_data;i->native_article_author_kind=author;i->native_script_owner=owner;i->xC4_article_data=(Article*)scheduler_spec->article;i->xB8_itemLogicTable=NULL;i->xBC_itemStateContainer=NULL;if(!nativeScriptItem_Attach(o,scheduler_spec))return false;return !scheduler_fail_selector;}
void nativeItem_CaptureCreator(Item_GObj* o,Fighter_GObj* f)
{(void)f;Item* item=o->user_data;item->native_creator=NULL;item->native_creator_generation=0;item->native_creator_kind=Ft_Kind_None;item->native_creator_definition=NULL;}
Fighter_GObj* nativeItem_Creator(Item_GObj* o){(void)o;return NULL;}
void Item_802680CC(HSD_GObj* o){if(joint_count==32)abort();memset(&joints[joint_count],0,sizeof(joints[0]));o->hsd_obj=&joints[joint_count++];++scheduler_models;}
void Item_8026849C(HSD_GObj* o){(void)o;}
bool ftLib_80086960(HSD_GObj* o){(void)o;return false;}
s32 ftLib_80086EB4(HSD_GObj* o){(void)o;return 0;}
ItemKind itGetKind(HSD_GObj* o){return ((Item*)o->user_data)->kind;}
u8 itGetTeamId(HSD_GObj* o){(void)o;return 0;}
u32 db_ShowItemPickupRange(void){return false;}
void it_80279B64(Item* i){(void)i;}
void it_80275158(HSD_GObj* o,float life){((Item*)o->user_data)->xD44_lifeTimer=life;}
bool grLib_801C9E60(Vec3* v){(void)v;return false;}
bool mpGetSpeed(int line,Vec3* point,Vec3* result){(void)line;(void)point;*result=(Vec3){0};return false;}
void C_VECAdd(Vec3* a,Vec3* b,Vec3* r){*r=(Vec3){a->x+b->x,a->y+b->y,a->z+b->z};}
float Stage_GetBlastZoneRightOffset(void){return 1000;}
float Stage_GetBlastZoneLeftOffset(void){return -1000;}
float Stage_GetBlastZoneBottomOffset(void){return -1000;}
void it_8027D730(HSD_GObj* o){(void)o;abort();}
void it_8028FE90(HSD_GObj* o){(void)o;abort();}
void it_8029D968(HSD_GObj* o){(void)o;abort();}
void it_802D09D0(HSD_GObj* o){(void)o;abort();}
void db_80225DD8(HSD_GObj* o,HSD_GObj* other){(void)o;(void)other;}
void lb_8000FD18(DynamicsDesc* d){(void)d;}
void nativeHoldable_Draw(HSD_GObj* o,int pass){(void)o;(void)pass;}
void it_8026EECC(HSD_GObj* o,int pass){(void)o;(void)pass;abort();}
// Workshop shared protocol is deliberately absent in this article scheduler
// executable; the root's substantive protocol fixture covers those branches.
#define HOLD(name) NativeScriptVMStatus name(Item_GObj* o,NativeScriptValue* r){(void)o;(void)r;return NativeScriptVM_Error;}
HOLD(nativeHoldable_Animation) HOLD(nativeHoldable_Physics) HOLD(nativeHoldable_Collision)
HOLD(nativeHoldable_PickedUp) HOLD(nativeHoldable_Dropped) HOLD(nativeHoldable_Thrown)
HOLD(nativeHoldable_Hit) HOLD(nativeHoldable_Clanked) HOLD(nativeHoldable_Reflected) HOLD(nativeHoldable_ShieldBounced)

#include "actual_scheduler.h"
void scheduler_userdata_remove(void* data){
    Item* item=data;
    for(unsigned i=0;i<NATIVE_SCRIPT_ITEM_ACTORS;++i){NativeScriptItemActor* actor=&nativeScriptItemActors[i];if(!actor->active||actor->object!=item->entity)continue;
        const NativeScriptCompileBank* b=actor->program->bank;for(unsigned f=0;f<b->declarations.value_count;++f){const NativeScriptValueDeclaration* d=&b->declarations.values[f];if(d->owner==actor->program->owner&&nativeScriptSource_SpanEquals(&b->tree,d->name,"count"))memcpy(&scheduler_destroyed,actor->bytes+d->byte_offset,4);}}
    Item_OnUserDataRemove(data);
}
void scheduler_animation(HSD_GObj* o){Item_80269528(o);}
void scheduler_accessory(HSD_GObj* o){Item_80269A9C(o);}
void scheduler_hitlag(HSD_GObj* o,bool enter){if(enter)Item_8026A158(o);else Item_8026A1E8(o);}
void scheduler_damage_pipeline(HSD_GObj* o){Item_8026A294(o);}
void scheduler_grab(HSD_GObj* o){Item_80269BE4(o);}
void scheduler_jumped(HSD_GObj* o){Item_80269B60(o);}
void scheduler_damage(HSD_GObj* o,uint32_t hook){Item* i=o->user_data;
    switch(hook){case NativeScriptItemHook_DamageReceived:(void)OnTakeDamageThink(o,i);break;
    case NativeScriptItemHook_DamageDealt:(void)OnGiveDamageThink(o,i);break;
    case NativeScriptItemHook_Clanked:(void)OnClankThink(o,i);break;
    case NativeScriptItemHook_Reflected:(void)Item_80269F14(o);break;
    case NativeScriptItemHook_ShieldBounced:i->xDCE_flag.b5=true;i->xDCE_flag.b4=true;i->ground_or_air=GA_Air;(void)Item_80269DC8(o);break;
    case NativeScriptItemHook_HitShield:i->xDCE_flag.b5=false;(void)Item_80269DC8(o);break;
    case NativeScriptItemHook_Absorbed:i->xC90_absorbGObj=(void*)1;Item_8026A294(o);break;
    default:(void)processSourceCallback(o,hook);break;}}
void scheduler_collision(HSD_GObj* o){Item_80269978(o);}
void scheduler_physics(HSD_GObj* o){Item_802697D4(o);}
void scheduler_state(HSD_GObj* o,int state){Item_80268E5C(o,state,0);}
bool scheduler_spawned(HSD_GObj* o){bool result=Item_8026A810(o);if(!result)fprintf(stderr,"spawned VM: %s\n",nativeScriptRuntime_VM()->diagnostic.message);return result;}
HSD_GObj* scheduler_construct(SpawnItem* s){return Item_8026862C(s,NativeItemBehavior_Author,scheduler_spec->author_kind,scheduler_spec->owner);}

// This fixture has no class-backed practice-grid cells. Unexpected resolution
// is a failure; class ownership itself is exercised by the HSD owner fixtures.
#include <sysdolphin/baselib/native_class_snapshot.h>
NativeClassReference nativeClass_Reference(const void* p,size_t n){(void)p;(void)n;abort();}
void* nativeClass_Resolve(NativeClassReference r){(void)r;abort();}
bool nativeClass_TypeKnown(const HSD_ClassInfo* p,size_t n){(void)p;(void)n;abort();}
