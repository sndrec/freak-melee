#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "fixture.h"
#include <melee/ft/native_script_binding_article_lifecycle.h>
#include <melee/ft/native_script_binding_fighter_items.h>
#include <melee/ft/native_script_binding_held_items.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/ft/native_script_fighter_snapshot.h>
#include <melee/ft/native_script_binding_fighter_items.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/kinds/ftCommon/ftpickupitem.h>
#include <melee/ft/kinds/ftCommon/ftCo_ItemThrow.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/native_script_item_snapshot.h>
#include <melee/it/native_item_owner.h>
#include <melee/it/item.h>
#include <melee/it/it_2725.h>
#include <melee/it/kinds/itfoods.h>
#include <melee/it/it_3F14.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum{N=2048,I=8192,F=64,V=128};
NativeSourcePackage lifecycle_package;
#define bank lifecycle_package.compiler.bank
static char source_bytes[32768];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode ir[I];static uint32_t arguments[I];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];
static NativeScriptLocal locals[V];static NativeScriptInstruction instructions[I];static NativeScriptFunctionCode codes[F];static NativeScriptExecutionFunction entries[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[V];static NativeScriptParameterSlot slots[V];
static Article article;static ItemAttr attributes;static ItemModelDesc model;
static Fighter fighters[128];static Item items[256];static FighterBone parts[4];static HSD_JObj joints[4],roots[256];
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
unsigned lifecycle_mode,lifecycle_pickups,lifecycle_tails,lifecycle_calls;HSD_GObj* lifecycle_creator;HSD_GObj*(*lifecycle_reuse)(void);
static unsigned checks,fighter_count,item_count,comparisons,cases,vm_calls,held_comparisons,held_receiver_calls;
static unsigned held_selection_mutation;static HSD_GObj* held_replacement;
void held_after_capture(HSD_GObj* source)
{
 unsigned mutation=held_selection_mutation;held_selection_mutation=0;
 if(mutation==1)++((Fighter*)source->user_data)->x8_spawnNum;
 if(mutation==2)((Fighter*)source->user_data)->item_gobj=held_replacement;
 if(mutation==3)((Fighter*)source->user_data)->kind=Ft_Kind_Samus;
 if(mutation==4)HSD_GObjFree(source);
}
unsigned presentation_mode,presentation_bonus,presentation_writes;HSD_GObj* presentation_target;
unsigned controls_debug_calls;int controls_raw_joint;HSD_JObj* controls_joint;
unsigned release_transforms,release_owners,release_hitboxes,release_heavy_tails;
unsigned policies_stats,policies_motion,policies_animation,policies_accessory,policies_stage,policies_mode,policies_constructs;
SpawnItem policies_spawn;HSD_GObj* policies_created;
static HSD_GObj* foods_caller;static unsigned raw_mode,raw_pickups;
static ItemLogicTable raw_logic,raw_other_logic;static Article raw_other_article;static ItemAttr raw_other_attributes;static ItemModelDesc raw_other_descriptor;
static HSD_GObj* controls_reflected_owner;static uint32_t next_handle;static HSD_GObj* replacement;
static void* retired_held_page;static DWORD retired_held_protection;static SIZE_T retired_page_bytes;
static uint8_t saved[131072];static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"lifecycle %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
static void prepare(void)
{
    bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=I,.arguments=arguments,.argument_capacity=I},
        .declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=V,
        .instructions=instructions,.instruction_capacity=I,.function_code=codes,.function_code_capacity=F,.execution_functions=entries,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,
        .storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=V},.parameter_slots=slots,.parameter_slot_capacity=V};
    const char* source="state int hold_category\nfunc HoldCategory():\n    state.hold_category = items.hold_kind\n"
        "fighter:\n    behavior independent\nstate int handle\nstate bool bound\nstate int held\nstate int auxiliary\nstate int held_kind\nstate int held_motion\nstate float held_frame\nstate int use_category\nstate int food_order\nstate int engine_kind\nstate int pickup_count\nstate bool pickup_flag\nstate int group\nstate int created\nstate int throw_motion\nstate int drop_count\nstate bool drop_flag\nfunc Query():\n    state.held = items.held_id\n    state.auxiliary = items.auxiliary_id\n    state.held_kind = items.held_kind\n    state.held_motion = items.held_motion\n    state.held_frame = items.held_frame\n    state.use_category = items.held_use_category\nfunc Hide():\n    state.bound = items.set_visible(state.handle, false)\nfunc Show():\n    state.bound = items.set_visible(state.handle, true)\nfunc Pause():\n    state.bound = items.set_paused(state.handle, true)\nfunc Resume():\n    state.bound = items.set_paused(state.handle, false)\nfunc NotifyDrop():\n    state.bound = items.notify_drop(state.handle, true)\nfunc ClearHeld():\n    items.clear_held()\nfunc ClearAux():\n    items.clear_auxiliary()\nfunc BindHeld():\n    state.bound = items.bind_held(state.handle)\nfunc BindAux():\n    state.bound = items.bind_auxiliary(state.handle)\nfunc ExistingHeld():\n    state.bound = items.bind_existing_held(state.handle)\nfunc ExistingAux():\n    state.bound = items.bind_existing_auxiliary(state.handle)\nfunc ArticleHeld():\n    state.bound = articles.bind_held(state.handle)\nfunc Attach():\n    state.bound = items.attach(state.handle, 1, false)\nfunc AttachNotify():\n    state.bound = items.attach(state.handle, 1, true)\nfunc Scale():\n    state.bound = items.inherit_owner_scale(state.handle, true)\nfunc PickupTrue():\n    items.notify_pickup(true)\nfunc PickupFalse():\n    items.notify_pickup(false)\nfunc NeutralNotify():\n    state.bound = items.notify_held_neutral()\nfunc Reopen():\n    items.reopen_blocked = true\n    state.bound = items.reopen_blocked\nfunc Close():\n    items.reopen_blocked = false\n    state.bound = items.reopen_blocked\nfunc Release():\n    items.release_held(-4, 5, -6, 1.75)\nfunc Throw():\n    items.throw_held(state.throw_motion)\nfunc EngineCreate():\n    state.created = items.create_engine(5, 1.25, -2.5, 3.75, -4, 5, -6, -1, state.group, 7.875)\nfunc FoodX() -> float:\n    state.food_order = state.food_order * 10 + 1\n    return 1.25\nfunc FoodY() -> float:\n    state.food_order = state.food_order * 10 + 2\n    return -2.5\nfunc FoodZ() -> float:\n    state.food_order = state.food_order * 10 + 3\n    return 3.75\nfunc SpawnFood():\n    state.food_order = 0\n    state.bound = items.spawn_food(FoodX(), FoodY(), FoodZ())\nfunc CreateAndControl():\n    state.created = items.create_engine(state.engine_kind, 1.25, -2.5, 3.75, 0, 0, 0, -1, 0, 0)\n    state.handle = state.created\n    if state.created != -1:\n        state.bound = items.attach(state.handle, 1, false)\n        if state.bound:\n            if state.engine_kind != 12:\n                state.bound = items.inherit_owner_scale(state.handle, true)\n            if state.bound:\n                items.clear_held()\n                state.bound = items.bind_held(state.handle)\naction Hooks:\n    on tick:\n        state.bound = articles.bind_auxiliary(state.handle)\n    on item_pickup_ext:\n        state.pickup_count = state.pickup_count + 1\n        state.pickup_flag = event.flag\n        position.x = 99\n    on item_drop_ext:\n        state.drop_count = state.drop_count + 1\n        state.drop_flag = event.flag\n        position.x = 88\narticle Probe states 1:\n    state int pickups\n    state int observed_id\n    state int observed_kind\n    state int observed_motion\n    state float observed_frame\n    state int damage_motion\n    on signal Observe:\n        if holder.exists:\n            item.state.observed_id = holder.items.held_id\n            item.state.observed_kind = holder.items.held_kind\n            item.state.observed_motion = holder.items.held_motion\n            item.state.observed_frame = holder.items.held_frame\n        if current_owner.exists:\n            item.state.damage_motion = current_owner.items.held_motion\n    on dropped:\n        item.state.pickups = item.state.pickups + 1\n        position.x = 9\n    on holder_neutral:\n        item.state.pickups = item.state.pickups + 1\n        position.x = 8\n    on picked_up:\n        item.state.pickups = item.state.pickups + 1\n        position.x = 7\n    on signal Attach:\n        item.attach_to_owner(1, true)\n    on signal DefaultAttach:\n        item.attach_to_owner(2)\n    on signal ClearCurrent:\n        item.clear_current_owner()\n    on signal ClearAttachment:\n        item.clear_attachment_owner()\n    on signal Detach:\n        item.detach_owner()\n    on signal Unbind:\n        item.owner_bound = false\n    state 0 anim -1:\narticle Silent states 1:\n    state 0 anim -1:\n";
    article=(Article){.x0_common_attr=&attributes,.x10_modelDesc=&model};
    NativeScriptItemAsset assets[]={{.declaration="Probe",.kind=It_Kind_Seak_Chain,.article=&article},{.declaration="Silent",.kind=It_Kind_Seak_Chain,.article=&article}};
    NativeScriptItemMetadataContext metadata={.assets=assets,.asset_count=2};
    NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};
    NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout,.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};
    const NativeScriptCompileBank* accepted=NULL;
    CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));
    CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,&bank,&layout,&metadata,&lifecycle_package.program,&diagnostic));
    lifecycle_package.address=&lifecycle_package;lifecycle_package.target=Ft_Kind_CustomFirst;lifecycle_package.ready=lifecycle_package.assets.ready=true;
    lifecycle_package.assets.facts.moves=layout;lifecycle_package.assets.facts.joints=4;lifecycle_package.assets.parts.parts_num=4;
    tables[Ft_Kind_CustomFirst]=&lifecycle_package.assets.parts;gFtDataList[Ft_Kind_CustomFirst]=&lifecycle_package.assets.character.data;
    ftData_CharacterStateTables[Ft_Kind_CustomFirst]=lifecycle_package.assets.motions;
    for(unsigned i=0;i<4;++i)parts[i].joint=&joints[i];
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&lifecycle_package.program);
}
static void fighter_remove(void* p){nativeScriptFighter_Detach(((Fighter*)p)->gobj);}
static void item_remove(void* p){nativeScriptItem_Detach(((Item*)p)->entity);}
static HSD_GObj* fighter(bool owned)
{
    CHECK(fighter_count<128);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);Fighter* f=&fighters[fighter_count++];
    *f=(Fighter){.gobj=o,.kind=owned?Ft_Kind_CustomFirst:Ft_Kind_Link,.behavior_kind=owned?Ft_Kind_CustomFirst:Ft_Kind_Link,
        .x8_spawnNum=19+fighter_count,.ft_data=&lifecycle_package.assets.character.data,.parts=parts,.player_id=2,.team=3,.x34_scale={1,2.5f,1}};
    GObj_InitUserData(o,4,fighter_remove,f);if(owned)CHECK(nativeScriptFighter_Attach(o));return o;
}
static HSD_GObj* item(HSD_GObj* creator)
{
    CHECK(item_count<256);const NativeScriptItemProgram* p=&nativeScriptFighter_Program(Ft_Kind_CustomFirst)->items.programs[0];
    HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);Item* i=&items[item_count];HSD_JObj* r=&roots[item_count++];
    *i=(Item){.entity=o,.kind=p->kind,.msid=-1,.native_script_owner=p->owner,.native_article_author_kind=p->author_kind,
        .x1C=next_handle?next_handle:100+item_count,.owner=creator,.xC4_article_data=&article,.xCC_item_attr=&attributes,.sfx_unk2=12};
    o->hsd_obj=r;GObj_InitUserData(o,6,item_remove,i);nativeItem_CaptureCreator(o,creator);CHECK(nativeScriptItem_Attach(o,p));return o;
}
static HSD_GObj* reuse(void){replacement=item(lifecycle_creator);return replacement;}
static NativeScriptValue field(HSD_GObj* o,const char* name,const NativeScriptValue* input)
{
    uint32_t kind=o->classifier==HSD_GOBJ_CLASS_FIGHTER?NativeCallbackSource_Fighter:NativeCallbackSource_Item;
    uint32_t gen=kind==NativeCallbackSource_Fighter?((Fighter*)o->user_data)->x8_spawnNum:((Item*)o->user_data)->x1C;
    NativeScriptReceiverView view;CHECK(kind==NativeCallbackSource_Fighter?nativeScriptFighter_View(o,gen,&view):nativeScriptItem_View(o,gen,&view));
    for(unsigned n=0;n<view.program->value_count;++n)if(values[n].owner==view.storage->owner&&nativeScriptSource_SpanEquals(&bank.tree,values[n].name,name)){
        NativeScriptValue result;if(input)CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,input,&diagnostic));
        CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&result,&diagnostic));return result;
    }CHECK(false);return(NativeScriptValue){0};
}
static uint32_t hash(const char* name){uint32_t h=2166136261u;while(*name){h^=(unsigned char)*name++;h*=16777619u;}return h;}
static void signal(HSD_GObj* o,const char* name)
{NativeScriptValue v;CHECK(nativeScriptItem_RunSignal(o,hash(name),0,&v)==NativeScriptVM_Ok);}
static NativeScriptRuntimeContext context(HSD_GObj* o)
{
    bool f=o->classifier==HSD_GOBJ_CLASS_FIGHTER;uint32_t gen=f?((Fighter*)o->user_data)->x8_spawnNum:((Item*)o->user_data)->x1C;
    return(NativeScriptRuntimeContext){.source=o,.source_class=f?NativeCallbackSource_Fighter:NativeCallbackSource_Item,.generation=gen,.actor_id=gen,.scene_epoch=nativeGObjPool.lifetime};
}
static bool invoke(HSD_GObj* o,uint32_t semantic,const NativeScriptValue* args,uint32_t count,NativeScriptValue* out)
{NativeScriptRuntimeContext c=context(o);return (semantic==NativeScriptBinding_ArticlesBindAuxiliary || semantic==NativeScriptBinding_ArticlesBindHeld || semantic>=NativeScriptBinding_ItemsClearHeld)?nativeScriptBindingFighterItems_Invoke(&c,semantic,args,count,out,&diagnostic):nativeScriptBindingArticleLifecycle_Invoke(&c,semantic,args,count,out,&diagnostic);}
void lifecycle_mutation(HSD_GObj* object)
{
    unsigned mode=lifecycle_mode;if(mode!=9)lifecycle_mode=0;
    if(mode==1){((Item*)object->user_data)->owner=NULL;CHECK(nativeScriptItem_ClearAttachmentOwner(object));}
    if(mode==2||mode==7){NativeScriptValue ignored;if(nativeScriptItem_IsAttached(object))CHECK(nativeScriptItem_RunHook(object,NativeScriptItemHook_Destroyed,NULL,0,&ignored)==NativeScriptVM_Ok);HSD_GObjFree(object);if(mode==7&&lifecycle_reuse)lifecycle_reuse();}
    if(mode==3)HSD_GObjFree(lifecycle_creator);
    if(mode==14){
        Fighter* retired=lifecycle_creator->user_data;HSD_GObjFree(lifecycle_creator);
        SYSTEM_INFO info;GetSystemInfo(&info);retired_page_bytes=info.dwPageSize;
        retired_held_page=(void*)((uintptr_t)&retired->item_gobj/retired_page_bytes*retired_page_bytes);
        CHECK(VirtualProtect(retired_held_page,retired_page_bytes,PAGE_NOACCESS,&retired_held_protection));
    }
    if(mode==4){for(unsigned i=0;i<NATIVE_SCRIPT_ITEM_ACTORS;++i)if(nativeScriptItemActors[i].object==object&&nativeScriptItemActors[i].active)nativeScriptItemActors[i].faulted=true;}
    if(mode==5){for(unsigned i=0;i<NATIVE_SCRIPT_ITEM_ACTORS;++i)if(nativeScriptItemActors[i].object==object&&nativeScriptItemActors[i].active)nativeScriptItemActors[i].storage.owner=0;}
    if(mode==6)((Item*)object->user_data)->x1C++;
    if(mode==10)((Item*)object->user_data)->owner=controls_reflected_owner;
    if(mode==11){CHECK(nativeScriptItem_ClearAttachmentOwner(object));((Item*)object->user_data)->owner=controls_reflected_owner;}
    if(mode==12)((Item*)object->user_data)->native_removal_in_progress=true;
    if(mode==13)object->classifier=HSD_GOBJ_CLASS_FIGHTER;
}

static uint32_t function_id(const char* name)
{for(uint32_t n=0;n<bank.declarations.function_count;++n)if(nativeScriptSource_SpanEquals(&bank.tree,functions[n].name,name))return n;CHECK(false);return UINT32_MAX;}
static void run(HSD_GObj* object,const char* name)
{NativeScriptValue result;CHECK(nativeScriptFighter_Helper(object,((Fighter*)object->user_data)->x8_spawnNum,function_id(name),NULL,0,&result,&diagnostic)==NativeScriptVM_Ok);CHECK(result.type==NativeScriptType_Void);++vm_calls;}
static void set_handle(HSD_GObj* object,HSD_GObj* target)
{NativeScriptValue value={.type=NativeScriptType_I32,.as.i64=target?(int32_t)((Item*)target->user_data)->x1C:-1};field(object,"handle",&value);}
static NativeScriptValue sentinel(void){return(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0xDEADABCD1234FFFF)};}
static void raw_picked_up(HSD_GObj* object)
{
 ++raw_pickups;Item* data=object->user_data;
 if(raw_mode==1)data->native_creator=NULL;
 if(raw_mode==2)data->native_creator_generation++;
 if(raw_mode==3)data->native_creator_kind=Ft_Kind_Link;
 if(raw_mode==4)object->hsd_obj=&joints[3];
 if(raw_mode==5)data->xC4_article_data=&raw_other_article;
 if(raw_mode==6)data->xCC_item_attr=&raw_other_attributes;
 if(raw_mode==7)data->xB8_itemLogicTable=&raw_other_logic;
 if(raw_mode==8)HSD_GObjFree(lifecycle_creator);
 if(raw_mode==9)HSD_GObjFree(object);
 if(raw_mode==10){++data->x1C;}
 if(raw_mode==11){data->owner=controls_reflected_owner;}
 if(raw_mode==12)data->native_removal_in_progress=true;
 if(raw_mode==13){data->xC4_article_data->x10_modelDesc=&raw_other_descriptor;}
}
HSD_GObj* controls_engine_constructor(SpawnItem* spawn)
{
 ++policies_constructs;policies_spawn=*spawn;
 if(policies_mode==1)return NULL;
 HSD_GObj* result=item(spawn->x0_parent_gobj);nativeScriptItem_Detach(result);Item* data=result->user_data;
 data->native_script_owner=0;data->native_article_author_kind=Ft_Kind_None;data->kind=spawn->kind;data->owner=spawn->x0_parent_gobj;raw_logic.picked_up=raw_picked_up;data->xB8_itemLogicTable=&raw_logic;data->scl=1;
 policies_created=result;
 if(policies_mode==2)HSD_GObjFree(spawn->x0_parent_gobj?spawn->x0_parent_gobj:foods_caller);
 if(policies_mode==3){nativeScriptFighter_Detach(spawn->x0_parent_gobj?spawn->x0_parent_gobj:foods_caller);}
 if(policies_mode==4){HSD_GObjFree(result);policies_created=NULL;return NULL;}
 return result;
}
void controls_policy_stage(unsigned stage,HSD_GObj* object)
{
 if(stage!=policies_stage)return;
 if(policies_mode==5)HSD_GObjFree(object);
 if(policies_mode==6)nativeScriptFighter_Detach(object);
 if(policies_mode==7)HSD_GObjFree(((Fighter*)object->user_data)->item_gobj);
}
static HSD_GObj* raw_item(HSD_GObj* creator)
{
 HSD_GObj* object=item(creator);nativeScriptItem_Detach(object);Item* data=object->user_data;
 data->native_script_owner=0;data->kind=It_Kind_Foods;data->xB8_itemLogicTable=&raw_logic;raw_logic.picked_up=raw_picked_up;return object;
}
static void bindings(HSD_GObj* caller,HSD_GObj* target,HSD_GObj* foreign)
{
 Fighter* f=caller->user_data;Item* data=target->user_data;set_handle(caller,target);
 const char* names[]={"BindHeld","BindAux","ArticleHeld"};
 for(unsigned n=0;n<3;++n){run(caller,names[n]);CHECK(field(caller,"bound",NULL).as.u64);CHECK((n==1?f->x1984_heldItemSpec:f->item_gobj)==target);++cases;}
 NativeScriptValue ignored;CHECK(nativeScriptFighter_Run(caller,NativeScriptFighter_Tick,NULL,0, NULL,&ignored)==NativeScriptVM_Ok&&f->x1984_heldItemSpec==target);++vm_calls;
 data->owner=foreign;CHECK(nativeScriptItem_ClearAttachmentOwner(target));run(caller,"BindHeld");CHECK(f->item_gobj==target&&data->owner==foreign&&!nativeScriptItem_AttachmentOwner(target));++cases;
 HSD_GObj* owned_elsewhere=item(foreign);HSD_GObj* raw=raw_item(foreign);
 const HSD_GObj* selected[]={owned_elsewhere,raw};
 for(unsigned n=0;n<2;++n){f->item_gobj=target;set_handle(caller,(HSD_GObj*)selected[n]);run(caller,"BindHeld");CHECK(!field(caller,"bound",NULL).as.u64&&f->item_gobj==target);
  run(caller,"ExistingHeld");run(caller,"ExistingAux");CHECK(field(caller,"bound",NULL).as.u64&&f->item_gobj==selected[n]&&f->x1984_heldItemSpec==selected[n]);++cases;
 }
 set_handle(caller,NULL);run(caller,"ExistingHeld");CHECK(!field(caller,"bound",NULL).as.u64&&f->item_gobj==raw);++cases;
 run(caller,"ClearHeld");run(caller,"ClearAux");CHECK(!f->item_gobj&&!f->x1984_heldItemSpec&&((Item*)raw->user_data)->owner==foreign);++cases;
 NativeScriptValue wide={.type=NativeScriptType_U32,.as.u64=UINT32_C(0xFEDCBA98)},result;
 uint32_t old=data->x1C; // rebind actor to native identity by actual detach/attach
 const NativeScriptItemProgram* spec=nativeScriptItem_Spec(target);data->x1C=old;nativeScriptItem_Detach(target);data->x1C=(uint32_t)wide.as.u64;CHECK(nativeScriptItem_Attach(target,spec));
 CHECK(invoke(caller,NativeScriptBinding_ItemsBindHeld,&wide,1,&result)&&result.as.u64&&f->item_gobj==target);
 set_handle(caller,target);run(caller,"BindAux");CHECK(f->x1984_heldItemSpec==target&&field(caller,"bound",NULL).as.u64);++cases;
 HSD_GObjFree(raw);HSD_GObjFree(owned_elsewhere);
}
static void attachment(HSD_GObj* caller,HSD_GObj* target,HSD_GObj* foreign)
{
 Fighter* f=caller->user_data;Item* data=target->user_data;set_handle(caller,target);f->item_gobj=foreign;
 NativeScriptValue args[]={{.type=NativeScriptType_U32,.as.u64=(uint32_t)data->x1C},{.type=NativeScriptType_I32,.as.i64=-1},{.type=NativeScriptType_Bool,.as.u64=0}},output=sentinel();
 CHECK(invoke(caller,NativeScriptBinding_ItemsAttach,args,3,&output)&&output.type==NativeScriptType_Bool&&!output.as.u64&&f->item_gobj==foreign);++cases;
 args[1].as.i64=4;CHECK(invoke(caller,NativeScriptBinding_ItemsAttach,args,3,&output)&&!output.as.u64&&f->item_gobj==foreign);++cases;
 args[1].as.i64=1;HSD_JObj* saved_joint=parts[1].joint;parts[1].joint=NULL;CHECK(invoke(caller,NativeScriptBinding_ItemsAttach,args,3,&output)&&!output.as.u64);parts[1].joint=saved_joint;++cases;
 static u8 semantic[FT_COMMON_PART_COUNT];semantic[1]=3;lifecycle_package.assets.parts.part_to_joint=semantic;
 controls_debug_calls=0;CHECK(nativeScriptItem_ClearAttachmentOwner(target));run(caller,"Attach");CHECK(field(caller,"bound",NULL).as.u64&&controls_raw_joint==1&&controls_joint==&joints[1]);
 CHECK(!nativeScriptItem_AttachmentOwner(target)&&nativeScriptItem_Creator(target)==caller&&data->owner==caller&&f->item_gobj==foreign&&controls_debug_calls==1);++cases;
 CHECK(nativeScriptItem_SetAttachmentOwner(target,caller));lifecycle_mode=1;run(caller,"AttachNotify");CHECK(!nativeScriptItem_AttachmentOwner(target)&&!data->owner&&field(caller,"bound",NULL).as.u64&&controls_debug_calls==3);++cases;
 controls_reflected_owner=foreign;lifecycle_mode=10;run(caller,"Attach");CHECK(data->owner==foreign&&field(caller,"bound",NULL).as.u64&&!nativeScriptItem_AttachmentOwner(target));++cases;
 CHECK(nativeScriptItem_SetAttachmentOwner(target,caller));lifecycle_mode=11;run(caller,"Attach");CHECK(data->owner==foreign&&!nativeScriptItem_AttachmentOwner(target)&&field(caller,"bound",NULL).as.u64);++cases;
}
static void scale_comparison(HSD_GObj* caller,HSD_GObj* target,HSD_GObj* foreign)
{
 Item* data=target->user_data;Fighter* current_owner=foreign->user_data;data->owner=foreign;set_handle(caller,target);
 const float factors[]={0.0f,-0.0f,1.0f,2.5f,-3.0f};
 for(unsigned n=0;n<ARRAY_SIZE(factors);++n)for(unsigned remember=0;remember<2;++remember){
  current_owner->x34_scale.y=factors[n];data->scl=1.25f;data->throw_scale=9.0f;data->x3C=3.75f;data->xBCC_unk=(Vec2){2,3};data->xBD4_grabRange=(Vec2){4,5};data->xBEC.top=6;data->xBDC.left=7;
  Item before=*data;HSD_JObj root_before=*(HSD_JObj*)target->hsd_obj;
  it_80274594(target);if(remember)data->throw_scale=data->scl;Item expected=*data;HSD_JObj expected_joint=*(HSD_JObj*)target->hsd_obj;
  *data=before;*(HSD_JObj*)target->hsd_obj=root_before;
  NativeScriptValue args[]={{.type=NativeScriptType_U32,.as.u64=(uint32_t)data->x1C},{.type=NativeScriptType_Bool,.as.u64=remember}},output;
  CHECK(invoke(caller,NativeScriptBinding_ItemsInheritOwnerScale,args,2,&output)&&output.type==NativeScriptType_Bool&&output.as.u64);
  CHECK(!memcmp(&data->scl,&expected.scl,sizeof(float))&&!memcmp(&data->throw_scale,&expected.throw_scale,sizeof(float)));
  CHECK(!memcmp(&data->xBCC_unk,&expected.xBCC_unk,sizeof(Vec2))&&!memcmp(&data->xBD4_grabRange,&expected.xBD4_grabRange,sizeof(Vec2)));
  CHECK(!memcmp(&((HSD_JObj*)target->hsd_obj)->scale,&expected_joint.scale,sizeof(Vec3))&&data->xBEC.top==expected.xBEC.top&&data->xBDC.left==expected.xBDC.left);
  CHECK(nativeScriptItem_Creator(target)==caller&&!nativeScriptItem_AttachmentOwner(target));++comparisons;
 }
 current_owner->x34_scale.y=4;data->scl=2;run(caller,"Scale");CHECK(data->scl==8&&data->throw_scale==8&&field(caller,"bound",NULL).as.u64);++cases;
 data->owner=NULL;run(caller,"Scale");CHECK(!field(caller,"bound",NULL).as.u64&&data->scl==8);++cases;data->owner=foreign;
}
static void rejection_and_lifetime(HSD_GObj* caller)
{
 NativeScriptValue before=sentinel(),output=before;const NativeScriptValue handles[]={{.type=NativeScriptType_F32,.as.f32=1},{.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000000)},{.type=NativeScriptType_I64,.as.i64=INT64_C(-2147483649)}};
 for(unsigned n=0;n<3;++n){CHECK(!invoke(caller,NativeScriptBinding_ItemsBindHeld,&handles[n],1,&output)&&!memcmp(&output,&before,sizeof(output)));++cases;}
 for(unsigned mode=1;mode<=9;++mode){if(mode==8)continue;
  HSD_GObj* target=item(caller);set_handle(caller,target);lifecycle_creator=caller;lifecycle_mode=mode;lifecycle_reuse=mode==7?reuse:NULL;
  NativeScriptValue args[]={{.type=NativeScriptType_U32,.as.u64=(uint32_t)((Item*)target->user_data)->x1C},{.type=NativeScriptType_I32,.as.i64=1},{.type=NativeScriptType_Bool,.as.u64=0}};output=before;
  bool ok=invoke(caller,NativeScriptBinding_ItemsAttach,args,3,&output);
  if(mode==1){CHECK(ok&&output.type==NativeScriptType_Bool&&output.as.u64&&!nativeScriptItem_AttachmentOwner(target));HSD_GObjFree(target);}
  else {CHECK(!ok&&!memcmp(&output,&before,sizeof(output)));if(mode==3){HSD_GObjFree(target);caller=fighter(true);lifecycle_creator=caller;}else if(mode==7){CHECK(replacement==target);HSD_GObjFree(replacement);}else if(mode!=2)HSD_GObjFree(target);}
  lifecycle_mode=0;lifecycle_reuse=NULL;++cases;
 }
 HSD_GObj* queued=item(caller);((Item*)queued->user_data)->native_removal_in_progress=true;
 NativeScriptValue queued_handle={.type=NativeScriptType_U32,.as.u64=(uint32_t)((Item*)queued->user_data)->x1C};output=before;
 CHECK(invoke(caller,NativeScriptBinding_ItemsBindHeld,&queued_handle,1,&output)&&output.type==NativeScriptType_Bool&&output.as.u64);
 CHECK(invoke(caller,NativeScriptBinding_ItemsBindExistingHeld,&queued_handle,1,&output)&&output.as.u64);++cases;
 ((Item*)queued->user_data)->native_removal_in_progress=false;lifecycle_mode=12;
 NativeScriptValue queued_args[]={queued_handle,{.type=NativeScriptType_I32,.as.i64=1},{.type=NativeScriptType_Bool,.as.u64=0}};output=before;
 CHECK(!invoke(caller,NativeScriptBinding_ItemsAttach,queued_args,3,&output)&&!memcmp(&output,&before,sizeof(output)));
 CHECK(((Item*)queued->user_data)->native_removal_in_progress);HSD_GObjFree(queued);++cases;
 HSD_GObj* mistyped=item(caller);NativeScriptValue mistyped_handle={.type=NativeScriptType_U32,.as.u64=(uint32_t)((Item*)mistyped->user_data)->x1C};
 mistyped->classifier=HSD_GOBJ_CLASS_FIGHTER;output=before;
 CHECK(!invoke(caller,NativeScriptBinding_ItemsBindExistingHeld,&mistyped_handle,1,&output)&&!memcmp(&output,&before,sizeof(output)));
 mistyped->classifier=HSD_GOBJ_CLASS_ITEM;lifecycle_mode=13;
 NativeScriptValue mistyped_args[]={mistyped_handle,{.type=NativeScriptType_I32,.as.i64=1},{.type=NativeScriptType_Bool,.as.u64=0}};output=before;
 CHECK(!invoke(caller,NativeScriptBinding_ItemsAttach,mistyped_args,3,&output)&&!memcmp(&output,&before,sizeof(output)));
 mistyped->classifier=HSD_GOBJ_CLASS_ITEM;HSD_GObjFree(mistyped);++cases;
 NativeScriptValue bad_bool={.type=NativeScriptType_Bool,.as.u64=2};output=before;
 CHECK(!invoke(caller,NativeScriptBinding_ItemsBindHeld,&bad_bool,1,&output)&&!memcmp(&output,&before,sizeof(output)));
 const uint32_t bool_ops[]={NativeScriptBinding_ItemsAttach,NativeScriptBinding_ItemsInheritOwnerScale,NativeScriptBinding_ItemsSetVisible,NativeScriptBinding_ItemsSetPaused,NativeScriptBinding_ItemsNotifyDrop};
 for(unsigned n=0;n<5;++n){NativeScriptValue invalid_flags[]={{.type=NativeScriptType_I32,.as.i64=-1},{.type=NativeScriptType_I32,.as.i64=1},bad_bool};
  unsigned argc=n==0?3:2;if(n)invalid_flags[1]=bad_bool;
  CHECK(!invoke(caller,bool_ops[n],invalid_flags,argc,&output)&&!memcmp(&output,&before,sizeof(output)));}
 ++cases;
 HSD_GObj* faulted=item(caller);for(unsigned n=0;n<NATIVE_SCRIPT_ITEM_ACTORS;++n)if(nativeScriptItemActors[n].active&&nativeScriptItemActors[n].object==faulted)nativeScriptItemActors[n].faulted=true;
 NativeScriptValue handle={.type=NativeScriptType_U32,.as.u64=(uint32_t)((Item*)faulted->user_data)->x1C};output=before;CHECK(!invoke(caller,NativeScriptBinding_ItemsBindExistingHeld,&handle,1,&output)&&!memcmp(&output,&before,sizeof(output)));++cases;
 HSD_GObjFree(faulted);HSD_GObjFree(caller);
}
static void snapshots(void)
{
 HSD_GObj* caller=fighter(true),*target=item(caller);set_handle(caller,target);run(caller,"BindHeld");Fighter* f=caller->user_data;Item* data=target->user_data;
 run(caller,"Reopen");CHECK(f->x2221_b5);CHECK(nativeScriptItem_ClearAttachmentOwner(target));size_t item_bytes,fighter_bytes;static uint8_t fighter_saved[65536];
 CHECK(nativeScriptItem_Save(saved,sizeof(saved),&item_bytes)&&nativeScriptFighter_Save(fighter_saved,sizeof(fighter_saved),&fighter_bytes));Fighter fighter_peer=*f;Item item_peer=*data;HSD_JObj root_peer=*(HSD_JObj*)target->hsd_obj;
 run(caller,"ClearHeld");CHECK(nativeScriptItem_SetAttachmentOwner(target,caller));run(caller,"AttachNotify");
 *f=fighter_peer;*data=item_peer;*(HSD_JObj*)target->hsd_obj=root_peer;
 CHECK(nativeScriptItem_Load(saved,item_bytes)&&nativeScriptFighter_Load(fighter_saved,fighter_bytes));CHECK(f->x2221_b5&&f->item_gobj==target&&!nativeScriptItem_AttachmentOwner(target)&&nativeScriptItem_Creator(target)==caller);++cases;
 run(caller,"Attach");CHECK(field(caller,"bound",NULL).as.u64&&!nativeScriptItem_AttachmentOwner(target));HSD_GObjFree(target);HSD_GObjFree(caller);
}
static void presentation_mutation(HSD_GObj* object)
{
 ++presentation_writes;
 if(presentation_mode==1){presentation_mode=0;HSD_GObjFree(object);}
 else if(presentation_mode==2){presentation_mode=0;HSD_GObjFree(presentation_target);}
 else if(presentation_mode==4){presentation_mode=0;Fighter* data=object->user_data;++data->x8_spawnNum;nativeScriptFighter_NewLife(object,data->x8_spawnNum);}
}
void controls_drop_write(HSD_GObj* object){presentation_mutation(object);}
static void held_policies(void)
{
 extern Article** it_804D6D24;
 HSD_GObj* caller=fighter(true),*target=item(caller);Fighter* f=caller->user_data;
 f->item_gobj=target;unsigned stats=policies_stats;
 run(caller,"PickupTrue");CHECK(field(caller,"pickup_count",NULL).as.i64==1&&field(caller,"pickup_flag",NULL).as.u64&&policies_stats==stats+1);
 run(caller,"PickupFalse");CHECK(field(caller,"pickup_count",NULL).as.i64==2&&!field(caller,"pickup_flag",NULL).as.u64&&policies_stats==stats+2);++cases;
 f->x1978=target;stats=policies_stats;run(caller,"PickupTrue");CHECK(field(caller,"pickup_count",NULL).as.i64==2&&policies_stats==stats+1);f->x1978=NULL;++cases;
 unsigned old=(unsigned)field(target,"pickups",NULL).as.i64;
 run(caller,"NeutralNotify");CHECK(field(caller,"bound",NULL).as.u64&&field(target,"pickups",NULL).as.i64==old+1);++cases;
 CHECK(nativeScriptItem_ClearAttachmentOwner(target));run(caller,"NeutralNotify");CHECK(field(caller,"bound",NULL).as.u64&&!nativeScriptItem_AttachmentOwner(target));++cases;
 HSD_GObj* silent=item(caller);nativeScriptItem_Detach(silent);Item* si=silent->user_data;
 const NativeScriptItemProgram* sp=&nativeScriptFighter_Program(Ft_Kind_CustomFirst)->items.programs[1];si->native_script_owner=sp->owner;CHECK(nativeScriptItem_Attach(silent,sp));
 f->item_gobj=silent;run(caller,"NeutralNotify");CHECK(!field(caller,"bound",NULL).as.u64);HSD_GObjFree(silent);++cases;
 HSD_GObj* raw=raw_item(caller);f->item_gobj=raw;run(caller,"NeutralNotify");CHECK(!field(caller,"bound",NULL).as.u64);HSD_GObjFree(raw);
 f->item_gobj=NULL;stats=policies_stats;run(caller,"PickupTrue");run(caller,"NeutralNotify");CHECK(!field(caller,"bound",NULL).as.u64&&policies_stats==stats);++cases;
 run(caller,"Reopen");CHECK(f->x2221_b5&&field(caller,"bound",NULL).as.u64);run(caller,"Close");CHECK(!f->x2221_b5&&!field(caller,"bound",NULL).as.u64);++cases;
 NativeScriptRuntimeContext c=context(caller);NativeScriptValue bad={.type=NativeScriptType_Bool,.as.u64=2},out=sentinel(),before=out;
 CHECK(!nativeScriptBindingFighterItems_Write(&c,NativeScriptBinding_ItemsReopenBlocked,&bad,&diagnostic)&&!f->x2221_b5);
 CHECK(!invoke(caller,NativeScriptBinding_ItemsNotifyPickup,&bad,1,&out)&&!memcmp(&out,&before,sizeof(out)));++cases;
 f->item_gobj=target;attributes.x1C_damage_mul=2.0f;
 for(int motion=ftCo_MS_LightThrowF;motion<=ftCo_MS_HeavyThrowLw4;++motion){
  Fighter peer=*f;f->facing_dir=-1;f->cmd_vars[0]=77;f->cmd_vars[1]=88;f->throw_flags=0xFF;
  ftCo_800957F4(caller,motion);float expected=f->frame_speed_mul,expected_facing=f->mv.co.itemthrow.facing_dir;
  *f=peer;f->facing_dir=-1;f->cmd_vars[0]=77;f->cmd_vars[1]=88;f->throw_flags=0xFF;
  NativeScriptValue m={.type=NativeScriptType_I32,.as.i64=motion};field(caller,"throw_motion",&m);run(caller,"Throw");
  CHECK(f->motion_id==motion&&!f->cmd_vars[0]&&!f->cmd_vars[1]&&!f->throw_flags&&f->accessory4_cb&&
        !memcmp(&f->frame_speed_mul,&expected,sizeof(expected))&&!memcmp(&f->mv.co.itemthrow.facing_dir,&expected_facing,sizeof(expected_facing)));++comparisons;
 }++cases;
 NativeScriptValue invalid={.type=NativeScriptType_I32,.as.i64=341};unsigned motions=policies_motion;
 CHECK(!invoke(caller,NativeScriptBinding_ItemsThrowHeld,&invalid,1,&out)&&!memcmp(&out,&before,sizeof(out))&&policies_motion==motions);++cases;
 f->item_gobj=NULL;invalid.as.i64=ftCo_MS_LightThrowF;CHECK(invoke(caller,NativeScriptBinding_ItemsThrowHeld,&invalid,1,&out)&&out.type==NativeScriptType_Void&&policies_motion==motions);++cases;
 it_804D6D24[5]=&article;NativeScriptValue group={.type=NativeScriptType_U32,.as.u64=UINT32_C(0xFEDCBA98)};NativeScriptValue stored_group={.type=NativeScriptType_I32,.as.i64=(int32_t)UINT32_C(0xFEDCBA98)};field(caller,"group",&stored_group);
 run(caller,"EngineCreate");CHECK(policies_created&&field(caller,"created",NULL).as.i64==(int32_t)((Item*)policies_created->user_data)->x1C);
 CHECK(policies_spawn.kind==5&&policies_spawn.x0_parent_gobj==caller&&policies_spawn.x4_parent_gobj2==caller&&policies_spawn.pos.x==1.25f&&policies_spawn.pos.y==-2.5f&&policies_spawn.pos.z==3.75f&&
       !memcmp(&policies_spawn.pos,&policies_spawn.prev_pos,sizeof(Vec3))&&policies_spawn.vel.x==-4&&policies_spawn.vel.y==5&&policies_spawn.vel.z==-6&&policies_spawn.facing_dir==-1&&
       policies_spawn.x40==UINT32_C(0xFEDCBA98)&&policies_spawn.x3C_damage==7&&policies_spawn.x44_flag.b0&&policies_spawn.x48_ground_or_air==GA_Air&&!policies_spawn.x10);HSD_GObjFree(policies_created);++cases;
 policies_mode=1;run(caller,"EngineCreate");CHECK(field(caller,"created",NULL).as.i64==-1);policies_mode=0;++cases;
 it_804D6D24[5]=NULL;unsigned constructors=policies_constructs;run(caller,"EngineCreate");CHECK(field(caller,"created",NULL).as.i64==-1&&policies_constructs==constructors);it_804D6D24[5]=&article;++cases;
 policies_mode=4;run(caller,"EngineCreate");CHECK(field(caller,"created",NULL).as.i64==-1);policies_mode=0;++cases;
 NativeScriptValue spawn_args[]={{.type=NativeScriptType_I32,.as.i64=5},{.type=NativeScriptType_F32,.as.f32=1.25f},{.type=NativeScriptType_F32,.as.f32=-2.5f},{.type=NativeScriptType_F32,.as.f32=3.75f},{.type=NativeScriptType_F32,.as.f32=-4},{.type=NativeScriptType_F32,.as.f32=5},{.type=NativeScriptType_F32,.as.f32=-6},{.type=NativeScriptType_F32,.as.f32=-1},group,{.type=NativeScriptType_F32,.as.f32=7.875f}};
 spawn_args[8]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=(int32_t)UINT32_C(0xFEDCBA98)};
 CHECK(invoke(caller,NativeScriptBinding_ItemsCreateEngine,spawn_args,10,&out)&&policies_spawn.x40==UINT32_C(0xFEDCBA98));HSD_GObjFree(policies_created);++cases;
 constructors=policies_constructs;out=before;spawn_args[8]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000000)};
 CHECK(!invoke(caller,NativeScriptBinding_ItemsCreateEngine,spawn_args,10,&out)&&!memcmp(&out,&before,sizeof(out))&&policies_constructs==constructors);spawn_args[8]=group;spawn_args[9].as.f32=32768;
 CHECK(!invoke(caller,NativeScriptBinding_ItemsCreateEngine,spawn_args,10,&out)&&!memcmp(&out,&before,sizeof(out))&&policies_constructs==constructors);spawn_args[9].as.f32=7.875f;spawn_args[1].as.f32=__builtin_nanf("");
 CHECK(!invoke(caller,NativeScriptBinding_ItemsCreateEngine,spawn_args,10,&out)&&!memcmp(&out,&before,sizeof(out))&&policies_constructs==constructors);spawn_args[1].as.f32=1.25f;
 spawn_args[8]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=123};CHECK(!invoke(caller,NativeScriptBinding_ItemsCreateEngine,spawn_args,10,&out)&&!memcmp(&out,&before,sizeof(out))&&policies_constructs==constructors);spawn_args[8]=group;++cases;
 HSD_GObjFree(target);HSD_GObjFree(caller);
 for(unsigned mode=1;mode<=4;++mode){caller=fighter(true);target=item(caller);f=caller->user_data;f->item_gobj=target;presentation_mode=mode;presentation_target=target;stats=policies_stats;
  out=before;NativeScriptValue flag={.type=NativeScriptType_Bool,.as.u64=1};CHECK(!invoke(caller,NativeScriptBinding_ItemsNotifyPickup,&flag,1,&out)&&!memcmp(&out,&before,sizeof(out))&&policies_stats==stats);
  if(mode==1)HSD_GObjFree(target);else{if(mode!=2)HSD_GObjFree(target);HSD_GObjFree(caller);}presentation_mode=0;++cases;
 }
 for(unsigned mode=2;mode<=4;mode+=2){caller=fighter(true);target=item(caller);f=caller->user_data;f->item_gobj=target;lifecycle_creator=caller;lifecycle_mode=mode;
  out=before;CHECK(!invoke(caller,NativeScriptBinding_ItemsNotifyHeldNeutral,NULL,0,&out)&&!memcmp(&out,&before,sizeof(out)));if(mode!=2)HSD_GObjFree(target);HSD_GObjFree(caller);lifecycle_mode=0;++cases;
 }
 caller=fighter(true);target=item(caller);f=caller->user_data;f->item_gobj=target;lifecycle_creator=caller;lifecycle_mode=14;
 out=before;bool neutral_result=invoke(caller,NativeScriptBinding_ItemsNotifyHeldNeutral,NULL,0,&out);
 DWORD ignored_protection;CHECK(retired_held_page&&VirtualProtect(retired_held_page,retired_page_bytes,retired_held_protection,&ignored_protection));retired_held_page=NULL;
 CHECK(!neutral_result&&!memcmp(&out,&before,sizeof(out))&&strstr(diagnostic.message,"neutral caller retired")&&
       nativeScriptItem_Status(target)==NativeScriptVM_Ok&&field(target,"pickups",NULL).as.i64==1);
 HSD_GObjFree(target);lifecycle_mode=0;++cases;
 for(unsigned stage=1;stage<=2;++stage)for(unsigned mode=5;mode<=7;++mode){caller=fighter(true);target=item(caller);f=caller->user_data;f->item_gobj=target;
  policies_stage=stage;policies_mode=mode;unsigned animations=policies_animation,accessories=policies_accessory;out=before;invalid.as.i64=ftCo_MS_LightThrowF;
  CHECK(!invoke(caller,NativeScriptBinding_ItemsThrowHeld,&invalid,1,&out)&&!memcmp(&out,&before,sizeof(out))&&policies_accessory==accessories&&policies_animation==animations+(stage==2));
  if(mode!=7)HSD_GObjFree(target);if(mode!=5)HSD_GObjFree(caller);policies_mode=policies_stage=0;++cases;
 }
 for(unsigned mode=2;mode<=3;++mode){caller=fighter(true);policies_mode=mode;out=before;CHECK(!invoke(caller,NativeScriptBinding_ItemsCreateEngine,spawn_args,10,&out)&&!memcmp(&out,&before,sizeof(out))&&policies_created);
  CHECK(nativeCallbackSource_Alive(policies_created,((Item*)policies_created->user_data)->x1C,NativeCallbackSource_Item));HSD_GObjFree(policies_created);if(mode==3)HSD_GObjFree(caller);policies_mode=0;++cases;
 }
}
static void ordinary_created_and_food(void)
{
 extern Article** it_804D6D24;extern RandomItemSpawner it_804A0E30;extern unsigned foods_no_stage;extern int foods_frequency;
 HSD_GObj* caller=fighter(true);Fighter* f=caller->user_data;f->facing_dir=-1;
 const int kinds[]={It_Kind_BombHei,It_Kind_Dosei,It_Kind_Sword};
 for(unsigned n=0;n<3;++n){it_804D6D24[kinds[n]]=&article;NativeScriptValue kind={.type=NativeScriptType_I32,.as.i64=kinds[n]};field(caller,"engine_kind",&kind);
  unsigned picked=raw_pickups;run(caller,"CreateAndControl");HSD_GObj* object=policies_created;Item* data=object->user_data;
  CHECK(field(caller,"bound",NULL).as.u64&&field(caller,"created",NULL).as.i64==(int32_t)data->x1C&&f->item_gobj==object&&raw_pickups==picked+1);
  CHECK(!nativeScriptItem_IsAttached(object)&&data->native_creator==caller&&data->native_creator_generation==(uint32_t)f->x8_spawnNum&&data->native_creator_kind==f->kind&&controls_raw_joint==1&&controls_joint==parts[1].joint);
  CHECK(data->scl==(n==2?1.0f:2.5f)&&(n==2||data->throw_scale==data->scl));
  run(caller,"BindAux");CHECK(f->x1984_heldItemSpec==object);run(caller,"ClearAux");run(caller,"ClearHeld");HSD_GObjFree(object);++cases;
 }
 HSD_GObj* target=raw_item(caller);Item* data=target->user_data;set_handle(caller,target);
 Fighter peer=*f;Item item_peer=*data;HSD_JObj root_peer=*(HSD_JObj*)target->hsd_obj;
 Item_8026AB54(target,caller,(Fighter_Part)1);Item expected=*data;unsigned picked=raw_pickups;
 *f=peer;*data=item_peer;*(HSD_JObj*)target->hsd_obj=root_peer;run(caller,"Attach");
 CHECK(field(caller,"bound",NULL).as.u64&&raw_pickups==picked+1&&data->owner==expected.owner&&data->hold_kind==expected.hold_kind&&data->sfx_unk2==expected.sfx_unk2);++comparisons;++cases;
 HSD_GObj* foreign=fighter(false);controls_reflected_owner=foreign;raw_mode=11;run(caller,"Attach");raw_mode=0;
 CHECK(field(caller,"bound",NULL).as.u64&&data->owner==foreign&&data->native_creator==caller);
 data->scl=1;run(caller,"Scale");CHECK(field(caller,"bound",NULL).as.u64&&data->scl==2.5f&&data->throw_scale==2.5f);++cases;
 const uint32_t strict_ops[]={NativeScriptBinding_ItemsBindHeld,NativeScriptBinding_ItemsBindAuxiliary,NativeScriptBinding_ItemsAttach,NativeScriptBinding_ItemsInheritOwnerScale};
 HSD_GObj* unowned=raw_item(foreign);NativeScriptValue out,before=sentinel();
 for(unsigned n=0;n<4;++n){NativeScriptValue args[]={{.type=NativeScriptType_U32,.as.u64=(uint32_t)((Item*)unowned->user_data)->x1C},{.type=NativeScriptType_I32,.as.i64=1},{.type=NativeScriptType_Bool,.as.u64=0}};
  unsigned count=n<2?1:n==2?3:2;if(n==3)args[1]=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=1};
  CHECK(invoke(caller,strict_ops[n],args,count,&out)&&out.type==NativeScriptType_Bool&&!out.as.u64);++cases;
 }
 HSD_GObjFree(unowned);
 uint32_t creator_generation=data->native_creator_generation;data->native_creator_generation++;
 run(caller,"BindHeld");CHECK(!field(caller,"bound",NULL).as.u64);data->native_creator_generation=creator_generation;
 FighterKind creator_kind=data->native_creator_kind;data->native_creator_kind=Ft_Kind_Link;run(caller,"BindAux");CHECK(!field(caller,"bound",NULL).as.u64);data->native_creator_kind=creator_kind;++cases;
 data->native_removal_in_progress=true;run(caller,"BindHeld");CHECK(field(caller,"bound",NULL).as.u64&&f->item_gobj==target);data->native_removal_in_progress=false;++cases;
 run(caller,"BindAux");Fighter saved_f=*f;Item saved_i=*data;run(caller,"ClearHeld");run(caller,"ClearAux");*f=saved_f;*data=saved_i;
 CHECK(f->item_gobj==target&&f->x1984_heldItemSpec==target&&data->native_creator==caller);++cases;
 f->item_gobj=f->x1984_heldItemSpec=NULL;HSD_GObjFree(target);HSD_GObjFree(foreign);
 for(unsigned mode=1;mode<=13;++mode){if(mode==11)continue;target=raw_item(caller);data=target->user_data;set_handle(caller,target);lifecycle_creator=caller;raw_mode=mode;
  NativeScriptValue args[]={{.type=NativeScriptType_U32,.as.u64=(uint32_t)data->x1C},{.type=NativeScriptType_I32,.as.i64=1},{.type=NativeScriptType_Bool,.as.u64=0}};out=before;
  CHECK(!invoke(caller,NativeScriptBinding_ItemsAttach,args,3,&out)&&!memcmp(&out,&before,sizeof(out)));
  raw_mode=0;if(mode!=9)HSD_GObjFree(target);if(mode==8){caller=fighter(true);f=caller->user_data;}article.x10_modelDesc=&model;++cases;
 }
 HSD_GObj* stale=raw_item(caller);uint32_t old_generation=((Fighter*)caller->user_data)->x8_spawnNum;HSD_GObj* old=caller;HSD_GObjFree(caller);caller=fighter(true);f=caller->user_data;
 CHECK(caller==old&&(uint32_t)f->x8_spawnNum!=old_generation);set_handle(caller,stale);run(caller,"BindHeld");CHECK(!field(caller,"bound",NULL).as.u64);HSD_GObjFree(stale);++cases;
 it_804D6D24[It_Kind_Foods]=&article;it_804A0E30.x18=UINT64_C(1)<<It_Kind_Foods;foods_frequency=0;foods_no_stage=0;foods_caller=caller;
 f->cur_pos=(Vec3){100,200,300};f->coll_data.ecb.top.y=20;f->coll_data.ecb.bottom.y=-10;
 s32 counter=it_804D6D0C;run(caller,"SpawnFood");CHECK(field(caller,"bound",NULL).as.u64&&field(caller,"food_order",NULL).as.i64==123&&it_804D6D0C==counter+1);
 CHECK(policies_created&&policies_spawn.kind==It_Kind_Foods&&!policies_spawn.x0_parent_gobj&&!policies_spawn.x4_parent_gobj2&&policies_spawn.facing_dir==-1&&policies_spawn.x3C_damage==0&&policies_spawn.x40==0&&policies_spawn.x44_flag.b0&&policies_spawn.x48_ground_or_air==GA_Air&&policies_spawn.hold_kind==ITEM_HOLD_2);
 CHECK(policies_spawn.prev_pos.x==1.25f&&policies_spawn.prev_pos.y==-2.5f&&policies_spawn.prev_pos.z==3.75f&&policies_spawn.pos.x==100&&policies_spawn.pos.y==205&&policies_spawn.pos.z==300&&policies_spawn.vel.x==0&&policies_spawn.vel.y==0&&policies_spawn.vel.z==0&&!((Item*)policies_created->user_data)->native_creator);HSD_GObjFree(policies_created);++cases;
 Vec3 supplied={1.25f,-2.5f,3.75f};HSD_GObj* native_food=it_8028FAF4(caller,&supplied);SpawnItem expected_spawn=policies_spawn;HSD_GObjFree(native_food);
 run(caller,"SpawnFood");CHECK(!memcmp(&policies_spawn.pos,&expected_spawn.pos,sizeof(Vec3))&&!memcmp(&policies_spawn.prev_pos,&expected_spawn.prev_pos,sizeof(Vec3))&&policies_spawn.facing_dir==expected_spawn.facing_dir&&policies_spawn.x44_flag.b0==expected_spawn.x44_flag.b0);HSD_GObjFree(policies_created);++comparisons;++cases;
 for(unsigned mode=0;mode<3;++mode){counter=it_804D6D0C;unsigned constructors=policies_constructs;if(mode==0)it_804A0E30.x18=0;if(mode==1)foods_no_stage=1;if(mode==2)foods_frequency=-1;
  run(caller,"SpawnFood");CHECK(!field(caller,"bound",NULL).as.u64&&field(caller,"food_order",NULL).as.i64==123&&it_804D6D0C==counter&&policies_constructs==constructors);
  it_804A0E30.x18=UINT64_C(1)<<It_Kind_Foods;foods_no_stage=0;foods_frequency=0;++cases;
 }
 for(unsigned mode=1;mode<=4;mode+=3){policies_mode=mode;counter=it_804D6D0C;run(caller,"SpawnFood");CHECK(!field(caller,"bound",NULL).as.u64&&it_804D6D0C==counter);policies_mode=0;++cases;}
 NativeScriptValue coords[]={{.type=NativeScriptType_F32,.as.f32=1},{.type=NativeScriptType_F32,.as.f32=__builtin_nanf("")},{.type=NativeScriptType_F32,.as.f32=3}};
 unsigned constructors=policies_constructs;out=before;CHECK(!invoke(caller,NativeScriptBinding_ItemsSpawnFood,coords,3,&out)&&!memcmp(&out,&before,sizeof(out))&&policies_constructs==constructors);coords[1].as.f32=2;++cases;
 HSD_GObjFree(caller);
 for(unsigned mode=2;mode<=3;++mode){caller=fighter(true);foods_caller=caller;policies_mode=mode;counter=it_804D6D0C;out=before;
  CHECK(!invoke(caller,NativeScriptBinding_ItemsSpawnFood,coords,3,&out)&&!memcmp(&out,&before,sizeof(out))&&policies_created&&it_804D6D0C==counter+1);
  CHECK(nativeCallbackSource_Alive(policies_created,((Item*)policies_created->user_data)->x1C,NativeCallbackSource_Item));HSD_GObjFree(policies_created);if(mode==3)HSD_GObjFree(caller);policies_mode=0;++cases;
 }
 foods_caller=NULL;
}
static void held_observations(void)
{
 HSD_GObj* original=fighter(true),*foreign=fighter(false),*observer=item(original),*ordinary=raw_item(original);
 Fighter* holder=foreign->user_data;Item* article_data=observer->user_data,*data=ordinary->user_data;
 article_data->owner=foreign;holder->item_gobj=observer;
 const uint32_t frame_bits[]={0,UINT32_C(0x80000000),UINT32_C(0x3f200000),UINT32_C(0xc0200000),
  UINT32_C(0x7f800000),UINT32_C(0xff800000),UINT32_C(0x7fc01234)};
 const uint32_t ids[]={NativeScriptBinding_ItemsHeldId,NativeScriptBinding_ItemsHeldKind,NativeScriptBinding_ItemsHeldMotion,NativeScriptBinding_ItemsHeldFrame};
 for(unsigned n=0;n<4;++n){const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(ids[n]);
  CHECK(descriptor&&descriptor->result_type==(n==3?NativeScriptType_F32:NativeScriptType_I32));
  CHECK((descriptor->flags&(NativeScriptFlag_Readable|NativeScriptFlag_Pure))==(NativeScriptFlag_Readable|NativeScriptFlag_Pure));
  CHECK(!(descriptor->flags&(NativeScriptFlag_Writable|NativeScriptFlag_SourceOnly)));}
 NativeScriptRuntimeContext c=context(foreign);
 for(unsigned n=0;n<ARRAY_SIZE(frame_bits);++n){
  article_data->msid=(int)n-2;memcpy(&article_data->x5CC_currentAnimFrame,&frame_bits[n],4);
  signal(observer,"Observe");++held_receiver_calls;
  CHECK(field(observer,"observed_id",NULL).as.i64==(int32_t)article_data->x1C);
  CHECK(field(observer,"observed_kind",NULL).as.i64==article_data->kind);
  CHECK(field(observer,"observed_motion",NULL).as.i64==article_data->msid);
  NativeScriptValue frame=field(observer,"observed_frame",NULL);CHECK(frame.type==NativeScriptType_F32&&!memcmp(&frame.as.f32,&frame_bits[n],4));
  for(unsigned op=0;op<4;++op){
   NativeScriptValue result=sentinel();CHECK(nativeScriptBindingHeldItems_Read(&c,ids[op],&result,&diagnostic));
   if(op==3){float reference=nativeFighterTarget_HeldFrame((uint32_t)holder->x8_spawnNum);CHECK(result.type==NativeScriptType_F32&&!memcmp(&reference,&result.as.f32,4));}
   else {int32_t reference=op==0?nativeFighterTarget_HeldId((uint32_t)holder->x8_spawnNum):
    op==1?nativeFighterTarget_HeldKind((uint32_t)holder->x8_spawnNum):nativeFighterTarget_HeldMotion((uint32_t)holder->x8_spawnNum);
    CHECK(result.type==NativeScriptType_I32&&result.as.i64==reference);}
   ++held_comparisons;
  }
 }
 // Current damage owner can hold another ordinary engine Item. The holder
 // relation then does not point at this observer, but current_owner does.
 holder->item_gobj=ordinary;data->msid=19;data->x1C=(s32)UINT32_C(0xFEDCBA98);data->x5CC_currentAnimFrame=-0.0f;
 signal(observer,"Observe");++held_receiver_calls;CHECK(field(observer,"damage_motion",NULL).as.i64==19);
 NativeScriptValue result;CHECK(nativeScriptBindingHeldItems_Read(&c,NativeScriptBinding_ItemsHeldId,&result,&diagnostic));
 CHECK((uint32_t)(int32_t)result.as.i64==UINT32_C(0xFEDCBA98));
 for(unsigned missing=0;missing<3;++missing){
  holder->item_gobj=missing?ordinary:NULL;data->native_removal_in_progress=missing==1;
  HSD_GObj* head=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];
  if(missing==2)HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM]=observer; // ordinary is unlinked from visible membership.
  HSD_GObj* next=observer->next;if(missing==2)observer->next=NULL;
  for(unsigned op=0;op<4;++op){result=sentinel();CHECK(nativeScriptBindingHeldItems_Read(&c,ids[op],&result,&diagnostic));
   if(op==3)CHECK(result.type==NativeScriptType_F32&&result.as.f32==0&&!signbit(result.as.f32));
   else CHECK(result.type==NativeScriptType_I32&&result.as.i64==-1);}
  observer->next=next;HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM]=head;
 }
 data->native_removal_in_progress=false;holder->item_gobj=ordinary;
 for(unsigned bad=0;bad<6;++bad){
  NativeScriptRuntimeContext saved=c;Item before=*data;u16 classifier=ordinary->classifier;
  if(bad==0)++c.generation;if(bad==1)++c.scene_epoch;if(bad==2)c.source_class=NativeCallbackSource_Item;
  if(bad==3)ordinary->classifier=HSD_GOBJ_CLASS_FIGHTER;if(bad==4)data->entity=observer;
  if(bad==5)holder->item_gobj=original;
  result=sentinel();NativeScriptValue unchanged=result;
  CHECK(!nativeScriptBindingHeldItems_Read(&c,NativeScriptBinding_ItemsHeldMotion,&result,&diagnostic));CHECK(!memcmp(&result,&unchanged,sizeof(result)));
  c=saved;*data=before;ordinary->classifier=classifier;holder->item_gobj=ordinary;
 }
 for(unsigned mutation=1;mutation<=4;++mutation){
  HSD_GObj* recipient=fighter(false);((Fighter*)recipient->user_data)->item_gobj=ordinary;
  NativeScriptRuntimeContext retained=context(recipient);held_replacement=observer;held_selection_mutation=mutation;
  result=sentinel();NativeScriptValue unchanged=result;
  CHECK(!nativeScriptBindingHeldItems_Read(&retained,NativeScriptBinding_ItemsHeldId,&result,&diagnostic));
  CHECK(!memcmp(&result,&unchanged,sizeof(result)));if(mutation!=4)HSD_GObjFree(recipient);
 }
 holder->item_gobj=NULL;HSD_GObjFree(ordinary);HSD_GObjFree(observer);HSD_GObjFree(foreign);HSD_GObjFree(original);++cases;
}
static void query_presentation(void)
{
 HSD_GObj* caller=fighter(true),*foreign=fighter(false),*target=raw_item(foreign);Fighter* f=caller->user_data;Item* data=target->user_data;
 f->item_gobj=target;f->x1984_heldItemSpec=target;set_handle(caller,target);attributes.use_kind=3;
 run(caller,"Query");CHECK(field(caller,"held",NULL).as.i64==(int32_t)data->x1C&&field(caller,"auxiliary",NULL).as.i64==(int32_t)data->x1C);
 CHECK(field(caller,"held_kind",NULL).as.i64==It_Kind_Foods&&field(caller,"use_category",NULL).as.i64==3);++cases;
 data->x1C=(s32)UINT32_C(0xFEDCBA98);set_handle(caller,target);run(caller,"Query");CHECK((uint32_t)(int32_t)field(caller,"held",NULL).as.i64==UINT32_C(0xFEDCBA98));++cases;
 const char* operations[]={"Hide","Show","Pause","Resume"};
 for(unsigned op=0;op<4;++op){HSD_JObj* root=target->hsd_obj;root->flags=JOBJ_HIDDEN;data->xDC8_word.flags.x3=0;data->xDC8_word.flags.x5=0;data->xDC8_word.flags.x7=1;
  HSD_JObj root_before=*root;Item item_before=*data;
  if(op==0)it_8026BB44(target);if(op==1)it_8026BB20(target);if(op==2)it_8026B724(target);if(op==3)it_8026B73C(target);
  HSD_JObj expected=*root;u32 expected_word;memcpy(&expected_word,&data->xDC8_word,4);
  *root=root_before;*data=item_before;run(caller,operations[op]);CHECK(field(caller,"bound",NULL).as.u64&&root->flags==expected.flags&&!memcmp(&data->xDC8_word,&expected_word,4));++comparisons;
 }
 run(caller,"Hide");run(caller,"Pause");unsigned bonus=presentation_bonus;run(caller,"NotifyDrop");
 CHECK(field(caller,"bound",NULL).as.u64&&!f->item_gobj&&data->owner==foreign&&presentation_bonus==bonus+1&&field(caller,"drop_count",NULL).as.i64==1&&field(caller,"drop_flag",NULL).as.u64);++cases;
 run(caller,"ExistingHeld");run(caller,"Show");run(caller,"Resume");CHECK(f->item_gobj==target&&!(((HSD_JObj*)target->hsd_obj)->flags&JOBJ_HIDDEN)&&!data->xDC8_word.flags.x3);++cases;
 f->item_gobj=NULL;f->x1984_heldItemSpec=NULL;run(caller,"Query");CHECK(field(caller,"held",NULL).as.i64==-1&&field(caller,"auxiliary",NULL).as.i64==-1&&field(caller,"held_kind",NULL).as.i64==-1&&field(caller,"use_category",NULL).as.i64==-1);++cases;
 f->item_gobj=target;HSD_GObjFree(target);NativeScriptRuntimeContext c=context(caller);NativeScriptValue value;
 CHECK(nativeScriptBindingHeldItems_Read(&c,NativeScriptBinding_ItemsHeldId,&value,&diagnostic)&&value.as.i64==-1);f->item_gobj=NULL;++cases;
 HSD_GObj* faulted=item(caller);for(unsigned n=0;n<NATIVE_SCRIPT_ITEM_ACTORS;++n)if(nativeScriptItemActors[n].active&&nativeScriptItemActors[n].object==faulted)nativeScriptItemActors[n].faulted=true;
 f->item_gobj=faulted;value=sentinel();NativeScriptValue before=value;
 CHECK(nativeScriptBindingHeldItems_Read(&c,NativeScriptBinding_ItemsHeldId,&value,&diagnostic)&&value.as.i64==((Item*)faulted->user_data)->x1C);f->item_gobj=NULL;HSD_GObjFree(faulted);++cases;
 HSD_GObj* queued=item(caller);((Item*)queued->user_data)->native_removal_in_progress=true;f->item_gobj=queued;
 CHECK(nativeScriptBindingHeldItems_Read(&c,NativeScriptBinding_ItemsHeldId,&value,&diagnostic)&&value.as.i64==-1);f->item_gobj=NULL;HSD_GObjFree(queued);++cases;
 HSD_GObjFree(foreign);HSD_GObjFree(caller);
 for(unsigned mode=1;mode<=4;++mode){caller=fighter(true);target=item(caller);f=caller->user_data;f->item_gobj=target;set_handle(caller,target);presentation_target=target;presentation_mode=mode;
  unsigned oldbonus=presentation_bonus;NativeScriptValue args[]={{.type=NativeScriptType_U32,.as.u64=(uint32_t)((Item*)target->user_data)->x1C},{.type=NativeScriptType_Bool,.as.u64=1}};value=sentinel();before=value;
  CHECK(!invoke(caller,NativeScriptBinding_ItemsNotifyDrop,args,2,&value)&&!memcmp(&value,&before,sizeof(value)));
  if(mode!=2)CHECK(presentation_bonus==oldbonus); // Item retirement leaves healthy caller/common side effects
  if(mode==1)HSD_GObjFree(target);else{if(mode!=2)HSD_GObjFree(target);HSD_GObjFree(caller);}presentation_mode=0;++cases;
 }
}
// Isolated release tests share this fixture's real programs and actors.
#include "release_cases.h"
#include "hold_category_cases.h"

int main(void)
{
 prepare();HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);
 lifecycle_creator=fighter(true);HSD_GObj* foreign=fighter(false),*target=item(lifecycle_creator);
 bindings(lifecycle_creator,target,foreign);attachment(lifecycle_creator,target,foreign);scale_comparison(lifecycle_creator,target,foreign);
 HSD_GObjFree(target);HSD_GObjFree(foreign);rejection_and_lifetime(lifecycle_creator);snapshots();query_presentation();held_policies();ordinary_created_and_food();held_observations();hold_category_cases();release_cases();CHECK(nativeScriptRuntime_Idle());
 printf("{\"status\":\"PASS connected Fighter Item controls\",\"checks\":%u,\"cases\":%u,\"native_comparisons\":%u,\"source_calls\":%u,\"held_native_comparisons\":%u,\"held_receiver_signals\":%u}\n",checks,cases,comparisons,vm_calls,held_comparisons,held_receiver_calls);return 0;
}
