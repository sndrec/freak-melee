#include "fixture.h"
#include <melee/ft/native_script_binding_fighter_items.h>
#include <melee/ft/native_script_binding_held_items.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/ft/ftlib.h>
extern void held_after_capture(HSD_GObj*);
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/ft_0C88.h>
#include <melee/ft/kinds/ftCommon/ftCo_HammerWait.h>
#include <melee/pl/plbonuslib.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/inlines.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/kinds/ftCommon/ftpickupitem.h>
#include <melee/ft/kinds/ftCommon/ftCo_ItemThrow.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/it/native_item_behavior.h>
#include <melee/it/kinds/itfoods.h>
#include <melee/it/itspawn.h>
#include <melee/it/it_3F14.h>
#include <melee/gr/ground.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/native_item_owner.h>
#include <melee/it/native_item_lifetime.h>
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
#include "item_properties.inc"
bool nativeScriptBindings_Read(NativeScriptRuntimeContext* c,uint32_t id,const NativeScriptValue* s,uint32_t n,NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)s;if(n)return false;if(c->source_class==NativeCallbackSource_Fighter){if(id==NativeScriptBinding_ItemsHeldId||id==NativeScriptBinding_ItemsHeldKind||id==NativeScriptBinding_ItemsHeldMotion||id==NativeScriptBinding_ItemsHeldFrame)return nativeScriptBindingHeldItems_Read(c,id,v,d);return nativeScriptBindingFighterItems_Read(c,id,v,d);}if(c->source_class!=NativeCallbackSource_Item||id!=NativeScriptBinding_ItemOwnerBound)return false;return nativeScriptBindingItem_Read(c,id,v,d);}
bool nativeScriptBindings_Write(NativeScriptRuntimeContext* c,uint32_t id,const NativeScriptValue* s,uint32_t n,const NativeScriptValue* v,NativeScriptDiagnostic* d)
{
    (void)s;(void)d;if(n)return false;
    if(c->source_class==NativeCallbackSource_Fighter){if(id==NativeScriptBinding_ItemsReopenBlocked)return nativeScriptBindingFighterItems_Write(c,id,v,d);if(id!=NativeScriptBinding_PositionX||v->type!=NativeScriptType_F32)return false;((Fighter*)((HSD_GObj*)c->source)->user_data)->cur_pos.x=v->as.f32;controls_drop_write(c->source);return presentation_mode!=3;}
    if(c->source_class!=NativeCallbackSource_Item)return false;
    Item* item=((HSD_GObj*)c->source)->user_data;
    if(id==NativeScriptBinding_ItemOwnerBound&&v->type==NativeScriptType_Bool)return nativeScriptBindingItem_Write(c,id,v,d);
    if(id!=NativeScriptBinding_PositionX||v->type!=NativeScriptType_F32)return false;
    item->pos.x=v->as.f32;++lifecycle_pickups;lifecycle_mutation(c->source);return lifecycle_mode!=9;
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
CALL(nativeScriptBindingItem_Invoke)
bool nativeScriptBindingPresentation_Invoke(NativeScriptRuntimeContext* c,uint32_t s,NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
bool nativeScriptBindingSetup_Read(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
bool nativeScriptBindingSetup_Write(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,const NativeScriptValue* r,NativeScriptDiagnostic* d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
#include <melee/ef/efsync.h>
const EF_ModelRecipe* efSync_ModelRecipe(s32 id){(void)id;return NULL;}
// Controlled rendering/audio/collision leaves of the actual pickup body.
void lb_8000B804(HSD_JObj* o,HSD_Joint* j){(void)o;(void)j;++lifecycle_calls;}
void Item_8026849C(HSD_GObj* o){(void)o;++lifecycle_calls;}
bool ftLib_80086960(HSD_GObj* o){uint32_t g;return nativeCallbackSource_Snapshot(o,NativeCallbackSource_Fighter,&g);}
u8 ftLib_80086BE0(HSD_GObj* o){return ((Fighter*)o->user_data)->player_id;}
s32 ftLib_80086EB4(HSD_GObj* o){return ((Fighter*)o->user_data)->team;}
void HSD_JObjSetMtxDirtySub(HSD_JObj* object){object->flags|=JOBJ_MTX_DIRTY;}
void it_802756D0(HSD_GObj* o){(void)o;++lifecycle_calls;}
void db_80225DD8(Item_GObj* o,Fighter_GObj* f){(void)o;(void)f;++lifecycle_calls;++controls_debug_calls;}
void it_80279BBC(Item* i){(void)i;++lifecycle_calls;}
void it_80275158(HSD_GObj* o,f32 f){((Item*)o->user_data)->xD44_lifeTimer=f;}
void it_80274F48(Item_GObj* o,s32 b,HSD_GObj* f,Fighter_Part p){(void)o;(void)b;controls_raw_joint=p;controls_joint=((Fighter*)f->user_data)->parts[p].joint;++lifecycle_calls;}
void Item_8026AE84(Item* i,enum_t s,u8 a,u8 b){(void)i;(void)s;(void)a;(void)b;++lifecycle_calls;}
int lbAudioAx_800236B8(int id){(void)id;++lifecycle_tails;return 0;}
NativeScriptVMStatus nativeHoldable_PickedUp(Item_GObj* o,NativeScriptValue* v){(void)o;(void)v;abort();}
NativeScriptVMStatus nativeHoldable_Dropped(Item_GObj* o,NativeScriptValue* v){(void)o;(void)v;abort();}
// Controlled transform/damage-owner/collision leaves of actual Item_8026ABD8.
// The release dispatcher and authored Dropped hook execute production code.
extern unsigned release_transforms,release_owners,release_hitboxes,release_heavy_tails;
void it_80273B50(Item_GObj* o,Vec3* velocity)
{Item* i=o->user_data;i->x40_vel=*velocity;++release_transforms;}
void it_80274198(Item_GObj* o,bool restore)
{if(!restore)abort();(void)o;++release_owners;}
void it_802754D4(Item_GObj* o){(void)o;++release_hitboxes;}
void it_80275390(Item_GObj* o){(void)o;++release_heavy_tails;}
bool it_8026B6C8(Item_GObj* o){return ((Item*)o->user_data)->kind==It_Kind_Taru;}
// Controlled constructor and throw-motion/animation/accessory leaves. The
// common entry/default initialization, pickup notification and throw setup are
// exact extracted production bodies; these leaves do not claim DAT/render or
// actual throw-release acceptance.
extern unsigned policies_stats,policies_motion,policies_animation,policies_accessory;
HSD_GObj* controls_engine_constructor(SpawnItem*);
void controls_policy_stage(unsigned,HSD_GObj*);
static ftCommonData policies_common={.x400=1.75f};
ftCommonData* p_ftCommonData=&policies_common;
Article* it_804D6D24_storage[It_Kind_Kuriboh];
Article** it_804D6D24=it_804D6D24_storage;
RandomItemSpawner it_804A0E30;
s32 it_804D6D0C;
unsigned foods_no_stage;int foods_frequency;
s32* Ground_801C2AD8(void){static s32 policy;return foods_no_stage?NULL:&policy;}
s32 gm_GetItemFrequency(void){return foods_frequency;}
static Item_GObj* Item_8026862C(SpawnItem* spawn,NativeItemBehaviorSelection selection,FighterKind author,uint32_t owner)
{if((selection!=NativeItemBehavior_Engine && (selection!=NativeItemBehavior_Parent || spawn->kind!=It_Kind_Foods || spawn->x0_parent_gobj || spawn->x4_parent_gobj2)) || author!=Ft_Kind_None || owner)abort();return controls_engine_constructor(spawn);}
void Fighter_ChangeMotionState(Fighter_GObj* object,int motion,u32 flags,float start,float speed,float blend,HSD_GObj* other)
{(void)flags;(void)start;(void)blend;(void)other;++policies_motion;Fighter* f=object->user_data;f->motion_id=motion;f->frame_speed_mul=speed;controls_policy_stage(1,object);}
void ftAnim_8006EBA4(Fighter_GObj* object){++policies_animation;controls_policy_stage(2,object);}
static void ftCo_80095EFC(Fighter_GObj* object){++policies_accessory;controls_policy_stage(3,object);}
void pl_8003E17C(int player,int secondary,HSD_GObj* item){(void)player;(void)secondary;(void)item;++policies_stats;}
Fighter_ItemEvent ftData_OnItemPickupExt[Ft_Kind_RuntimeCapacity];
#include "native_pickup.inc"


void (HSD_JObjSetMtxDirty)(HSD_JObj* joint){joint->flags|=JOBJ_MTX_DIRTY;}

bool ftCo_800C5240(HSD_GObj* object){(void)object;return false;}
void ftCo_800C5500(HSD_GObj* object){(void)object;abort();}
void pl_8003EA08(int player,int secondary){(void)player;(void)secondary;++presentation_bonus;}

Fighter_ItemEvent ftData_OnItemDropExt[Ft_Kind_RuntimeCapacity];
