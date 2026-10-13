#include "boundary.h"
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_script_receiver.h>
#include <melee/ft/native_script_actor_access.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftchangeparam.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ftcpuattack.h>
#include <melee/ft/ftcmdscript.h>
#include <sysdolphin/baselib/random.h>
#include <melee/ft/kinds/ftPeach/ftpeachspecialhi.h>
#include <melee/ft/ftcpu_data.h>
#include <melee/ft/kinds/ftCommon/ftCo_HammerWait.h>
#include <melee/ft/kinds/ftCommon/ftCo_0A01.h>
#include <melee/ft/kinds/ftCommon/ftCo_09F7.h>
#include <melee/ft/kinds/ftCommon/ftCo_SpecialS.h>
#include <melee/ft/kinds/ftDonkey/types.h>
#include <melee/pl/plbonuslib.h>
#include <melee/mp/mpisland.h>
#include <melee/gr/stage.h>
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
bool scheduler_overwrite_landing;
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
void efAsync_QueueFlush(HSD_GObj* o,void* q){(void)o;(void)q;}
bool nativeScriptBindings_Context(void* o,uint32_t k,uint32_t g,NativeScriptRuntimeContext* c,NativeScriptDiagnostic* d){(void)d;if(!nativeCallbackSource_Alive(o,g,k))return false;*c=(NativeScriptRuntimeContext){.source=o,.source_class=k,.generation=g,.actor_id=g};return true;}
bool nativeScriptBindings_SourceAlive(NativeScriptRuntimeContext* c){return c&&nativeCallbackSource_Alive(c->source,c->generation,c->source_class);}
const NativeScriptMoveLayout* nativeScriptBindings_SourceFighterLayout(NativeScriptRuntimeContext* c,NativeScriptDiagnostic* d){(void)c;(void)d;abort();}
void ftCommon_8007F76C(Fighter_GObj* o){(void)o;abort();}
void ftCommon_UnlockECB(Fighter* f){(void)f;abort();}
bool nativeScriptBindings_Read(NativeScriptRuntimeContext* c,uint32_t id,const NativeScriptValue* s,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)id;(void)s;(void)n;(void)r;(void)d;return false;}
bool nativeScriptBindings_Write(NativeScriptRuntimeContext* c,uint32_t id,const NativeScriptValue* s,uint32_t n,const NativeScriptValue* v,NativeScriptDiagnostic* d){(void)s;(void)d;if(c->source_class!=NativeCallbackSource_Fighter||id!=NativeScriptBinding_PositionX||n||v->type!=NativeScriptType_F32)return false;if(scheduler_fail_write)return false;Fighter* fighter=((HSD_GObj*)c->source)->user_data;fighter->cur_pos.x=v->as.f32;scheduler_aux_seen_first=fighter->x197C;scheduler_aux_seen_second=fighter->x1980;scheduler_aux_seen_timer=fighter->x2014;if(scheduler_overwrite_landing)fighter->mv.co.landing.allow_interrupt=false;if(scheduler_remove){scheduler_remove=false;HSD_GObjFree(c->source);}if(scheduler_retire_target){HSD_GObj* target=scheduler_retire_target;scheduler_retire_target=NULL;HSD_GObjFree(target);}if(scheduler_spawn_on_write){HSD_GObj*(*spawn)(void)=scheduler_spawn_on_write;scheduler_spawn_on_write=NULL;spawn();}return true;}
// Unrelated native VM operation families are unavailable, never surrogate bodies.
#define READ(name) bool name(NativeScriptRuntimeContext* c,uint32_t s,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)r;(void)d;return false;}
#define WRITE(name) bool name(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)r;(void)d;return false;}
#define CALL(name) bool name(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
READ(nativeScriptBindingKinetics_Read) WRITE(nativeScriptBindingKinetics_Write) CALL(nativeScriptBindingKinetics_Invoke)
READ(nativeScriptBindingState_Read) CALL(nativeScriptBindingState_Invoke)
READ(nativeScriptBindingCombat_Read) WRITE(nativeScriptBindingCombat_Write) CALL(nativeScriptBindingCombat_Invoke)
READ(nativeScriptBindingPresentation_Read) CALL(nativeScriptBindingTransition_Invoke)
READ(nativeScriptBindingItem_Read) WRITE(nativeScriptBindingItem_Write) CALL(nativeScriptBindingItem_Invoke)
READ(nativeScriptBindingSpawn_Read) CALL(nativeScriptBindingSpawn_Invoke)
CALL(nativeScriptBindingActions_Invoke)
bool nativeScriptBindingEffect_Invoke(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* a,uint32_t count,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)a;(void)count;(void)r;(void)d;return false;}
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
void it_8026EECC(HSD_GObj* o,int pass){(void)o;(void)pass;abort();}

void scheduler_userdata_remove(void* data){(void)data;}

#include <melee/ef/efsync.h>
const EF_ModelRecipe* efSync_ModelRecipe(s32 id){(void)id;return NULL;}
bool nativeScriptEffect_HasActors(FighterKind k){(void)k;return false;}
bool nativeScriptEffect_View(HSD_GObj* o,uint32_t g,NativeScriptReceiverView* v){(void)o;(void)g;(void)v;return false;}
bool nativeScriptEffect_CanBeginScene(uint64_t e){return e&&nativeScriptRuntime_Idle();}
bool nativeScriptEffect_BeginScene(uint64_t e){return e&&nativeScriptRuntime_Idle();}
bool nativeScriptEffect_RetireScene(void){return nativeScriptRuntime_Idle();}
Fighter_ItemEvent ftData_OnItemDropExt[Ft_Kind_RuntimeCapacity],ftData_OnItemDrop[Ft_Kind_RuntimeCapacity],ftData_OnItemPickup[Ft_Kind_RuntimeCapacity],ftData_OnItemPickupExt[Ft_Kind_RuntimeCapacity];
HSD_GObjEvent ftData_OnItemInvisible[Ft_Kind_RuntimeCapacity],ftData_OnItemVisible[Ft_Kind_RuntimeCapacity],ftData_OnKnockbackEnter[Ft_Kind_RuntimeCapacity],ftData_OnKnockbackExit[Ft_Kind_RuntimeCapacity],ftKindCalcIndiviParamTable[Ft_Kind_RuntimeCapacity],ftData_UnkMotionStates1[Ft_Kind_RuntimeCapacity],ftData_UnkMotionStates2[Ft_Kind_RuntimeCapacity];
bool ftCo_800C5240(HSD_GObj* o){(void)o;return false;}
void ftCo_800C5500(HSD_GObj* o){(void)o;abort();}
unsigned fighter_stock_calls,fighter_cpu_tail,fighter_bookkeeping,fighter_accessory_tail;
static bool stock_cpu(HSD_GObj* object){(void)object;++fighter_stock_calls;return true;}
static void stock_event(HSD_GObj* object){(void)object;++fighter_stock_calls;}
static void stock_flag(HSD_GObj* object,bool flag){(void)object;(void)flag;++fighter_stock_calls;}
void fighter_scheduler_legacy(void){
    for(unsigned i=0;i<Ft_Kind_RuntimeCapacity;++i){ftData_OnItemDropExt[i]=ftData_OnItemDrop[i]=ftData_OnItemPickup[i]=ftData_OnItemPickupExt[i]=stock_flag;ftData_OnItemInvisible[i]=ftData_OnItemVisible[i]=ftData_OnKnockbackEnter[i]=ftData_OnKnockbackExit[i]=ftKindCalcIndiviParamTable[i]=ftData_UnkMotionStates1[i]=ftData_UnkMotionStates2[i]=stock_event;}
    legacy_fighter.on_cpu_capture_decision=stock_cpu;legacy_fighter.on_cpu_ranged_candidates=stock_event;legacy_fighter.on_landing_action=stock_event;
    legacy_fighter.on_wait_input=stock_cpu;legacy_fighter.on_action_status=stock_cpu;
}
void pl_8003EA08(int id,int value){(void)id;(void)value;++fighter_bookkeeping;}
void pl_8003E17C(int id,int value,HSD_GObj* item){(void)id;(void)value;(void)item;++fighter_bookkeeping;}

void ftCommon_8007D7FC(Fighter* fp){fp->ground_or_air=GA_Ground;}
void Fighter_ChangeMotionState(Fighter_GObj* o,FtMotionId id,MotionFlags flags,float start,float speed,float blend,Fighter_GObj* target){scheduler_jab_flags=flags;(void)start;(void)speed;(void)blend;(void)target;((Fighter*)o->user_data)->motion_id=id;nativeScriptFighter_SelectMotion(o,id);}
void ftPe_8011D598(HSD_GObj* o){(void)o;abort();}
void ftCo_800CF6E8(ftCo_DatAttrs* attrs,float scale){(void)attrs;(void)scale;abort();}
struct Fighter_804D6518_t* Fighter_804D6518;
struct Fighter_804D651C_t* Fighter_804D651C;
static struct Fighter_804D6520_t bunny_attributes;
struct Fighter_804D6520_t* Fighter_804D6520=&bunny_attributes;
void ftCo_800D0CBC(HSD_GObj* o){(void)o;abort();}
void ftCo_800A0C8C(Fighter* fp){(void)fp;++fighter_cpu_tail;}
float HSD_Randf(void){++scheduler_tail;return 0.9f;}
void ftCo_800B4880(Fighter* fp,int script){(void)fp;(void)script;abort();}
mp_UnkStruct0* mpIsland_8005AC14(Vec3* v,float height){(void)v;(void)height;return NULL;}
void mpIsland_8005ACE8(mp_UnkStruct0* island,Vec3* a,Vec3* b){(void)island;(void)a;(void)b;abort();}
void ftCo_800B46B8(Fighter* fp,cmd_t cmd,arg_t arg){(void)fp;(void)cmd;(void)arg;++scheduler_tail;}
void ftCo_800B463C(Fighter* fp,cmd_t cmd){(void)fp;(void)cmd;++scheduler_tail;}
bool ftCo_IsGrabbing(Fighter* fp){(void)fp;return false;}
bool ftCo_800A1C44(Fighter* fp){(void)fp;return false;}
bool ftCo_CpuRangedAttackUnsafe(Fighter* a,Fighter* b,float distance){(void)a;(void)b;(void)distance;abort();}
bool mpCheckAll(Vec3* out,int* line,u32* flags,Vec3* normal,int skip,int only,float a,float b,float c,float d){(void)out;(void)line;(void)flags;(void)normal;(void)skip;(void)only;(void)a;(void)b;(void)c;(void)d;abort();}
bool ftCo_800A0FB0(Vec3* out,int* line,u32* flags,Vec3* normal,int a,int b,int c,float d,float e,float f,float g,float h){(void)out;(void)line;(void)flags;(void)normal;(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;abort();}
void Fighter_UnkApplyTransformation_8006C0F0(HSD_GObj* o){(void)o;}
void ft_8007C224(HSD_GObj* o){(void)o;}
void ft_8007C6DC(HSD_GObj* o){(void)o;}
float Stage_GetCamBoundsBottomOffset(void){return -1000;}
int ifMagnify_802FB6E8(int player){(void)player;return 0;}
void Stage_UnkSetVec3TCam_Offset(Vec3* v){*v=(Vec3){0};}
void ftColl_8007AE80(HSD_GObj* o){(void)o;++fighter_accessory_tail;}
#include "input_leaves.h"
#include "auxiliary_leaves.h"
#include "actual_scheduler.h"

// Unrelated Setup presentation leaves are deliberately unavailable in this
// motion-registration fixture. Reaching one is a test failure, not success.
bool itIsHeavy(Item_GObj* object){(void)object;abort();}
Item_HoldKinds itGetHoldKind(Item_GObj* object){(void)object;abort();}
void ftAnim_80070C48(Fighter_GObj* object,s32 index){(void)object;(void)index;abort();}
void ftAnim_80070CC4(Fighter_GObj* object,int index){(void)object;(void)index;abort();}
void ftAnim_80070FB4(Fighter_GObj* object,s32 first,s32 second){(void)object;(void)first;(void)second;abort();}
void ftAnim_800704F0(Fighter_GObj* object,int index,float frame){(void)object;(void)index;(void)frame;abort();}
CALL(nativeScriptBindingJump_Invoke)
CALL(nativeScriptBindingForms_Invoke)
CALL(nativeScriptBindingLedge_Invoke)
