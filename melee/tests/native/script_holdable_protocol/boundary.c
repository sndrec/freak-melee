#include "boundary.h"
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_script_receiver.h>
#include <melee/ft/native_script_actor_access.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ft_0881.h>
#include <melee/ft/ftchangeparam.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/item.h>
#include <melee/it/inlines.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/it/it_279C.h>
#include <melee/it/it_3F14.h>
#include <melee/it/it_3F2F.h>
#include <melee/it/ithitbox.h>
#include <melee/it/itcoll.h>
#include <melee/lb/lbcollision.h>
#include <melee/lb/lbvector.h>
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
unsigned holdable_animation_leaves,holdable_collision_response,holdable_collision_queries;int holdable_contact;bool holdable_retire_in_state,holdable_fail_in_state;
bool holdable_retire_in_contact,holdable_retire_in_response;
static Item constructor_item;static ItemAttr attributes;static ItemCommonData common;
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

void scheduler_reset(void){scheduler_tail=scheduler_native=scheduler_models=0;scheduler_remove=scheduler_nested=scheduler_fail_selector=false;common.xD8=100;common.x58_float=-0.5f;common.x5C_float=-0.25f;common.x60_float=0.75f;common.x68_float=37.5f;it_804D6D04=colors;joint_count=0;}
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
static NativeFighterDefinition legacy_fighter;
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind k){return k==Ft_Kind_Mario?&legacy_fighter:NULL;}
void holdable_legacy_recipe(const NativeItemDefinition* recipe){legacy_fighter.items=recipe;legacy_fighter.item_count=1;}
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
void HSD_JObjRemoveAnimAll(HSD_JObj* o){(void)o;++holdable_animation_leaves;}
void HSD_JObjAddAnimAll(HSD_JObj* o,HSD_AnimJoint* a,HSD_MatAnimJoint* b,HSD_ShapeAnimJoint* c){(void)o;(void)a;(void)b;(void)c;}
void HSD_JObjReqAnimAll(HSD_JObj* o,float f){(void)o;(void)f;}
void lb_8000B804(HSD_JObj* o,HSD_Joint* j){(void)o;(void)j;}
void lb_8000BA0C(HSD_JObj* o,float f){(void)o;(void)f;}
HSD_JObj* HSD_JObjRemove(HSD_JObj* o){(void)o;return NULL;}
void* HSD_ObjAlloc(HSD_ObjAllocData* data){(void)data;memset(&constructor_item,0xCC,sizeof(constructor_item));return &constructor_item;}
void HSD_ObjFree(HSD_ObjAllocData* data,void* object){(void)data;(void)object;++scheduler_frees;}
void nativeArticleResource_CancelGeneration(Item_GObj* o,uint32_t g){(void)o;(void)g;}
void nativeArticleModel_Destroy(Item_GObj* o,unsigned model){(void)o;(void)model;}
void nativeReplay_LogItemEvent(ItemKind kind,const char* event){(void)kind;(void)event;}
void efLib_DestroyAll(HSD_GObj* o){(void)o;}
void efAsync_QueueClear(void* q){(void)q;}
void efAsync_QueueFlush(HSD_GObj* o,void* q){(void)q;if(holdable_retire_in_state){holdable_retire_in_state=false;Item_8026A8EC(o);}if(holdable_fail_in_state){holdable_fail_in_state=false;scheduler_fail_write=true;NativeScriptValue result;nativeScriptItem_RunHook(o,NativeScriptItemHook_WorkshopTick,NULL,0,&result);scheduler_fail_write=false;}if(scheduler_nested){scheduler_nested=false;Item_80268E5C(o,1,0);}}
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
READ(nativeScriptBindingPresentation_Read) CALL(nativeScriptBindingSetup_Invoke) CALL(nativeScriptBindingTransition_Invoke)
READ(nativeScriptBindingItem_Read) WRITE(nativeScriptBindingItem_Write) CALL(nativeScriptBindingItem_Invoke)
READ(nativeScriptBindingSpawn_Read) CALL(nativeScriptBindingSpawn_Invoke)
bool nativeScriptBindingSetup_Read(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
bool nativeScriptBindingSetup_Write(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,const NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
bool nativeScriptBindingPresentation_Invoke(NativeScriptRuntimeContext* c,uint32_t s,NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}

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

void Item_802694CC(HSD_GObj* o){(void)o;}
void Item_80268D34(HSD_GObj* o,ItemStateDesc* descriptor){(void)o;(void)descriptor;}
void Item_80267978(HSD_GObj* o){((Item*)o->user_data)->xC4_article_data=(Article*)scheduler_spec->article;}
bool nativeItemBehavior_Select(Item_GObj* o,const SpawnItem* spawn,NativeItemBehaviorSelection s,FighterKind author,uint32_t owner)
{(void)spawn;(void)s;Item* i=o->user_data;i->native_article_author_kind=author;i->native_script_owner=owner;i->xB8_itemLogicTable=NULL;i->xBC_itemStateContainer=NULL;if(!nativeScriptItem_Attach(o,scheduler_spec))return false;return !scheduler_fail_selector;}
void nativeItem_CaptureCreator(Item_GObj* o,Fighter_GObj* f){(void)o;(void)f;}
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
#include "actual_holdable.h"
#include "actual_scheduler.h"

s32 it_8026DAA8(HSD_GObj* object){++holdable_collision_queries;if(holdable_retire_in_contact){holdable_retire_in_contact=false;Item_8026A8EC(object);}return holdable_contact;}
void it_80276FC4(HSD_GObj* object,s32 contact){(void)contact;++holdable_collision_response;if(holdable_retire_in_response){holdable_retire_in_response=false;Item_8026A8EC(object);}}
float ftCo_CalcYScaledKnockback(float damage,float scale,float base){(void)damage;(void)scale;(void)base;abort();}
float ft_80089228(Fighter* fighter,int a,int b,float damage){(void)fighter;(void)a;(void)b;(void)damage;abort();}
struct Fighter_804D6524_t* Fighter_804D6524;
void scheduler_userdata_remove(void* data){Item_OnUserDataRemove(data);}
void holdable_legacy_pick(HSD_GObj* o){legacy_picked_up(o);}
void holdable_legacy_drop(HSD_GObj* o){legacy_dropped(o);}
void holdable_legacy_throw(HSD_GObj* o){legacy_thrown(o);}
bool holdable_legacy_animation(HSD_GObj* o){HSD_GObjPredicate fn=legacy_states[GET_ITEM(o)->msid].animated;return fn?fn(o):false;}
void holdable_legacy_physics(HSD_GObj* o){HSD_GObjEvent fn=legacy_states[GET_ITEM(o)->msid].physics_updated;if(fn)fn(o);}
bool holdable_legacy_collision(HSD_GObj* o){HSD_GObjPredicate fn=legacy_states[GET_ITEM(o)->msid].collided;return fn?fn(o):false;}
bool holdable_legacy_hit(HSD_GObj* o){return legacy_hit(o);}
bool holdable_legacy_clank(HSD_GObj* o){return legacy_clanked(o);}
bool holdable_legacy_reflect(HSD_GObj* o){return legacy_reflected(o);}
bool holdable_legacy_shield(HSD_GObj* o){return legacy_shield_bounced(o);}
const ItemStateTable* holdable_legacy_states(void){return legacy_states;}
