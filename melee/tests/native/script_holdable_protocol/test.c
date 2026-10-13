#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "boundary.h"
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_script_item_program.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/native_holdable.h>
#include <melee/it/item.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum{N=4096,F=64,V=128};
static char source_bytes[65536];static NativeScriptSourceNode syntax[N];static NativeScriptIRNode expressions[N*2];static uint32_t arguments[N*2];
static NativeScriptFunctionDeclaration declarations[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[64];
static NativeScriptInstruction instructions[8192];static NativeScriptFunctionCode code[F];static NativeScriptExecutionFunction functions[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[8];static NativeScriptParameterSlot slots[V];
static NativeScriptCompileBank bank;static NativeScriptItemPrograms prepared;static Article assets;static ItemModelDesc model;static ItemAttr attributes;
static ItemStateDesc animation;static ItemStateArray animation_array={&animation,1};static HSD_Joint joint;static Item instances[4];static HSD_JObj joints[4];static ItemLogicTable native_logic;
static unsigned checks,cases,pairs,generation;static bool retired_result;static unsigned retired_leaves;static NativeScriptVMStatus retired_status;
static const NativeScriptItemProgram *plain,*custom,*unbounded;
static unsigned reentry_mode;static HSD_GObj* surviving_child;static bool reentry_guarded;
#define CHECK(v)do{++checks;if(!(v)){fprintf(stderr,"holdable check %u: %s\n",__LINE__,#v);exit(1);}}while(0)
const NativeScriptItemProgram* nativeScriptItem_Program(FighterKind author,uint32_t owner)
{for(unsigned i=0;i<prepared.count;++i)if(prepared.programs[i].author_kind==author&&prepared.programs[i].owner==owner)return &prepared.programs[i];return NULL;}
static void prepare(void)
{
 bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=syntax,.capacity=N},.expressions={.nodes=expressions,.node_capacity=N*2,.arguments=arguments,.argument_capacity=N*2},.declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=64,.instructions=instructions,.instruction_capacity=8192,.function_code=code,.function_code_capacity=F,.execution_functions=functions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=8},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* text="fighter:\n    behavior independent\nitem Plain damage 11 angle 37 lifetime 5:\n    hitbox_size 2.75\n    knockback_growth 83\n    base_knockback 21\nitem Custom damage 11 angle 37 lifetime 5:\n    hitbox_size 2.75\n    knockback_growth 83\n    base_knockback 21\n    state int count\n    state bool remove\n    on thrown:\n        item.state.count = item.state.count + 1\n        position.x = item.state.count\n    on tick:\n        item.state.count = item.state.count + 2\n        position.x = item.state.count\n    on hit:\n        item.state.count = item.state.count + 10\n        position.x = item.state.count\n        return item.state.remove\n    on ground_collision:\n        item.state.count = item.state.count + 100\n        position.x = item.state.count\n        return item.state.remove\nitem Unbounded damage 11 angle 37 lifetime 0:\n    hitbox_size 2.75\narticle EmptyArticle states 1:\n    state 0 anim -1:\n";
 assets=(Article){.x0_common_attr=&attributes,.x10_modelDesc=&model,.xC_itemStates=&animation_array};model.x0_joint=&joint;
 NativeScriptItemAsset imported[4]={{.declaration="Plain",.kind=It_Kind_NativeHoldable,.article=&assets},{.declaration="Custom",.kind=It_Kind_NativeHoldable,.article=&assets},{.declaration="Unbounded",.kind=It_Kind_NativeHoldable,.article=&assets},{.declaration="EmptyArticle",.kind=It_Kind_Mario_Fire,.article=&assets}};
 NativeScriptItemMetadataContext metadata={.assets=imported,.asset_count=4};NativeScriptCompileRequest request={.source=text,.source_bytes=(uint32_t)strlen(text),.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};NativeScriptDiagnostic d={0};const NativeScriptCompileBank* accepted=NULL;
 if(!nativeScriptCompile_Build(&bank,&request,&accepted,&d)||!nativeScriptItemPrograms_Prepare(&bank,Ft_Kind_Mario,99,&metadata,&prepared,&d)){fprintf(stderr,"prepare %u:%u %s\n",d.source.line,d.source.column,d.message);exit(1);}
 CHECK(prepared.count==4&&prepared.recipe_count==3);plain=&prepared.programs[0];custom=&prepared.programs[1];unbounded=&prepared.programs[2];
 CHECK(bank.declarations.value_count==2&&bank.declarations.function_count==4&&bank.storage_layout.owner_count==5);
 const unsigned empty[]={0,2,3};for(unsigned i=0;i<3;++i){const NativeScriptItemProgram* spec=&prepared.programs[empty[i]];const NativeScriptStorageOwner* owner=nativeScriptStorage_Owner(&bank.storage_layout,spec->owner);CHECK(owner&&owner->state_bytes==0&&owner->parameter_count==0);CHECK(nativeScriptItem_ProgramValid(spec));for(unsigned h=0;h<NativeScriptItemHook_Count;++h)CHECK(spec->hooks[h]==UINT32_MAX);}
 for(unsigned i=0;i<3;++i){CHECK(nativeScriptItem_ProgramValid(&prepared.programs[i]));CHECK(prepared.programs[i].recipe&&!prepared.programs[i].recipe->on_tick&&!prepared.programs[i].recipe->on_thrown&&!prepared.programs[i].recipe->on_hit&&!prepared.programs[i].recipe->on_ground_collision);}
 attributes.x10_fall_speed=0.25f;attributes.x14_fall_speed_max=3.5f;
}
static HSD_GObj* create(const NativeScriptItemProgram* spec,unsigned slot)
{
 HSD_GObj* object=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(object);Item* item=&instances[slot];memset(item,0,sizeof(*item));memset(&joints[slot],0,sizeof(joints[slot]));
 item->entity=object;item->kind=It_Kind_NativeHoldable;item->x1C=++generation;item->msid=-1;item->native_article_author_kind=Ft_Kind_Mario;item->native_script_owner=spec?spec->owner:0;item->xC4_article_data=&assets;item->xCC_item_attr=&attributes;item->xBC_itemStateContainer=(ItemStateTable*)holdable_legacy_states();item->xB8_itemLogicTable=&native_logic;
 item->xDD4_itemVar.native_holdable.owner_kind=Ft_Kind_Mario;item->xDD4_itemVar.native_holdable.definition_index=spec?spec->recipe_index:0;item->ground_or_air=GA_Air;item->scl=1.25f;item->spin_spd=2.5f;item->facing_dir=1;item->xC70=0.75f;item->xD48_halfLifeTimer=42;item->x5D0_animFrameSpeed=1;
 object->hsd_obj=&joints[slot];GObj_InitUserData(object,6,scheduler_userdata_remove,item);if(spec)CHECK(nativeScriptItem_Attach(object,spec));Item_80268E5C(object,1,0);return object;
}
static NativeScriptValue state(HSD_GObj* object,const char* name,const NativeScriptValue* replacement)
{
 for(unsigned a=0;a<NATIVE_SCRIPT_ITEM_ACTORS;++a)if(nativeScriptItemActors[a].active&&nativeScriptItemActors[a].object==object){NativeScriptItemActor* actor=&nativeScriptItemActors[a];for(unsigned f=0;f<bank.declarations.value_count;++f)if(values[f].owner==actor->program->owner&&nativeScriptSource_SpanEquals(&bank.tree,values[f].name,name)){NativeScriptValue result;NativeScriptDiagnostic d;if(replacement)CHECK(nativeScriptStorage_Write(&actor->storage,NATIVE_SCRIPT_STATE_SEMANTIC|f,NULL,0,replacement,&d));CHECK(nativeScriptStorage_Read(&actor->storage,NATIVE_SCRIPT_STATE_SEMANTIC|f,NULL,0,&result,&d));return result;}}
 CHECK(false);return(NativeScriptValue){0};
}
static void compare(HSD_GObj* source,HSD_GObj* native)
{
 Item *a=source->user_data,*b=native->user_data;CHECK(a->msid==b->msid&&a->anim_id==b->anim_id&&a->xDD4_itemVar.native_holdable.age==b->xDD4_itemVar.native_holdable.age&&a->xDD4_itemVar.native_holdable.collision_flags==b->xDD4_itemVar.native_holdable.collision_flags);
 CHECK(!memcmp(&a->x40_vel,&b->x40_vel,sizeof(Vec3))&&a->facing_dir==b->facing_dir&&a->xD3C_spinSpeed==b->xD3C_spinSpeed&&a->xD44_lifeTimer==b->xD44_lifeTimer);
 CHECK(a->x378_itemColl.facing_dir==b->x378_itemColl.facing_dir&&a->xDC8_word.flags.x16==b->xDC8_word.flags.x16&&a->xDAA_flag.b2==b->xDAA_flag.b2);
 HitCapsule x=a->x5D4_hitboxes[0].hit,y=b->x5D4_hitboxes[0].hit;x.jobj=y.jobj=NULL;CHECK(!memcmp(&x,&y,sizeof(x)));++pairs;
}
static void dispose(HSD_GObj* a,HSD_GObj* b){Item_8026A8EC(a);if(b)Item_8026A8EC(b);}
static void generic_pairs(void)
{
 holdable_legacy_recipe(plain->recipe);NativeScriptValue result;
 for(unsigned trial=0;trial<32;++trial){HSD_GObj *a=create(plain,0),*b=create(NULL,1);Item *x=a->user_data,*y=b->user_data;
  x->scl=y->scl=0.25f+(float)trial/8;x->xDD4_itemVar.native_holdable.age=y->xDD4_itemVar.native_holdable.age=17;
  CHECK(nativeHoldable_PickedUp(a,&result)==NativeScriptVM_Ok&&result.type==NativeScriptType_Void);holdable_legacy_pick(b);compare(a,b);CHECK(x->xDD4_itemVar.native_holdable.age==17);
  CHECK(nativeHoldable_Dropped(a,&result)==NativeScriptVM_Ok);holdable_legacy_drop(b);compare(a,b);CHECK(x->msid==3&&x->xDD4_itemVar.native_holdable.age==0);
  x->xDD4_itemVar.native_holdable.age=y->xDD4_itemVar.native_holdable.age=23;CHECK(nativeHoldable_Thrown(a,&result)==NativeScriptVM_Ok);holdable_legacy_throw(b);compare(a,b);CHECK(x->x5D4_hitboxes[0].hit.unk_count==11&&x->x5D4_hitboxes[0].hit.damage==11);
  x->x40_vel=y->x40_vel=(Vec3){(float)trial/3-5,(float)trial/4-4,0};x->facing_dir=y->facing_dir=trial&1?-1:1;
  for(unsigned frame=0;frame<8;++frame){CHECK(nativeHoldable_Animation(a,&result)==NativeScriptVM_Ok&&result.as.u64==holdable_legacy_animation(b));CHECK(nativeHoldable_Physics(a,&result)==NativeScriptVM_Ok);holdable_legacy_physics(b);compare(a,b);}
  x->xC58=y->xC58=(Vec3){trial&1?1:0,trial&1?0:1,0};CHECK(nativeHoldable_ShieldBounced(a,&result)==NativeScriptVM_Ok&&result.as.u64==holdable_legacy_shield(b));compare(a,b);
  CHECK(nativeHoldable_Reflected(a,&result)==NativeScriptVM_Ok&&result.as.u64==holdable_legacy_reflect(b));compare(a,b);
  CHECK(nativeHoldable_Clanked(a,&result)==NativeScriptVM_Ok&&result.as.u64==holdable_legacy_clank(b));compare(a,b);
  CHECK(nativeHoldable_Hit(a,&result)==NativeScriptVM_Ok&&result.as.u64==holdable_legacy_hit(b));compare(a,b);
  for(int contact=0;contact<16;++contact){Item_80268E5C(a,2,0);Item_80268E5C(b,2,0);holdable_contact=contact;CHECK(nativeHoldable_Collision(a,&result)==NativeScriptVM_Ok&&result.as.u64==holdable_legacy_collision(b));compare(a,b);}
  dispose(a,b);
 }
 ++cases;
}
static void custom_hooks(void)
{
 HSD_GObj* a=create(custom,0);Item* item=a->user_data;NativeScriptValue result;CHECK(nativeHoldable_Thrown(a,&result)==NativeScriptVM_Ok&&state(a,"count",NULL).as.i64==1&&item->pos.x==1);
 item->x40_vel=(Vec3){2,3,0};CHECK(nativeHoldable_Physics(a,&result)==NativeScriptVM_Ok&&item->x40_vel.y==3&&item->xD3C_spinSpeed==0);
 CHECK(nativeHoldable_Animation(a,&result)==NativeScriptVM_Ok&&!result.as.u64&&state(a,"count",NULL).as.i64==3&&item->xDD4_itemVar.native_holdable.age==1);++cases;
 holdable_contact=0xE;unsigned bounce=holdable_collision_response;CHECK(nativeHoldable_Collision(a,&result)==NativeScriptVM_Ok&&!result.as.u64&&item->msid==2&&holdable_collision_response==bounce&&state(a,"count",NULL).as.i64==103&&item->xDD4_itemVar.native_holdable.collision_flags==0xE);
 CHECK(nativeHoldable_Hit(a,&result)==NativeScriptVM_Ok&&!result.as.u64&&item->msid==2&&state(a,"count",NULL).as.i64==113);++cases;
 NativeScriptValue remove={.type=NativeScriptType_Bool,.as.u64=1};state(a,"remove",&remove);CHECK(nativeHoldable_Hit(a,&result)==NativeScriptVM_Ok&&result.as.u64);CHECK(nativeHoldable_Collision(a,&result)==NativeScriptVM_Ok&&result.as.u64);++cases;
 Item_80268E5C(a,0,0);unsigned age=item->xDD4_itemVar.native_holdable.age;int64_t count=state(a,"count",NULL).as.i64;CHECK(nativeHoldable_Animation(a,&result)==NativeScriptVM_Ok&&!result.as.u64&&item->xDD4_itemVar.native_holdable.age==age&&state(a,"count",NULL).as.i64==count);dispose(a,NULL);++cases;
 a=create(unbounded,0);item=a->user_data;item->xDD4_itemVar.native_holdable.age=UINT16_MAX;CHECK(nativeHoldable_Animation(a,&result)==NativeScriptVM_Ok&&!result.as.u64&&item->xDD4_itemVar.native_holdable.age==0);dispose(a,NULL);++cases;
}
static void faulted_actor(void)
{
 NativeScriptValue result;HSD_GObj*a=create(custom,0);scheduler_fail_write=true;result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=123};CHECK(nativeHoldable_Animation(a,&result)==NativeScriptVM_Error&&result.type==NativeScriptType_U64&&result.as.u64==123);scheduler_fail_write=false;CHECK(nativeScriptItem_IsAttached(a));CHECK(nativeHoldable_Animation(a,&result)==NativeScriptVM_Error);
 NativeScriptVMStatus (*blocked[])(HSD_GObj*,NativeScriptValue*)={nativeHoldable_Physics,nativeHoldable_PickedUp,nativeHoldable_Dropped,nativeHoldable_Thrown,nativeHoldable_Collision,nativeHoldable_Hit,nativeHoldable_Clanked,nativeHoldable_Reflected,nativeHoldable_ShieldBounced};
 for(unsigned i=0;i<sizeof(blocked)/sizeof(blocked[0]);++i){NativeScriptVMStatus status=blocked[i](a,&result);if(status!=NativeScriptVM_Error)fprintf(stderr,"faulted operation %u returned %u\n",i,status);CHECK(status==NativeScriptVM_Error&&result.type==NativeScriptType_U64&&result.as.u64==123);}
 dispose(a,NULL);++cases;
}
static void held_collision(void)
{
 NativeScriptValue result;holdable_legacy_recipe(plain->recipe);
 for(int selected=0;selected<2;++selected){HSD_GObj *a=create(plain,0),*b=create(NULL,1);Item_80268E5C(a,selected,0);Item_80268E5C(b,selected,0);for(int contact=0;contact<16;++contact){holdable_contact=contact;unsigned before=holdable_collision_queries;CHECK(nativeHoldable_Collision(a,&result)==NativeScriptVM_Ok&&result.as.u64==holdable_legacy_collision(b));compare(a,b);CHECK(holdable_collision_queries==before);}dispose(a,b);}++cases;
}
static void deferred_protocol(HSD_GObj* object)
{
 NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=123};holdable_retire_in_state=true;unsigned before=holdable_animation_leaves;retired_status=nativeHoldable_PickedUp(object,&result);retired_leaves=holdable_animation_leaves-before;retired_result=result.type==NativeScriptType_U64&&result.as.u64==123;
}
static void deferred_hook(HSD_GObj* object)
{
 NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=123};scheduler_remove=true;NativeScriptVMStatus status;
 switch(reentry_mode){
 case 0:status=nativeHoldable_Animation(object,&result);break;
 case 1:status=nativeHoldable_Thrown(object,&result);break;
 case 2:status=nativeHoldable_Hit(object,&result);break;
 default:status=nativeHoldable_Collision(object,&result);break;
 }
 reentry_guarded=status==NativeScriptVM_SourceRetired&&result.type==NativeScriptType_U64&&result.as.u64==123&&nativeScriptItem_Status(object)==NativeScriptVM_SourceRetired&&nativeScriptItem_Spec(object)!=NULL;
}
static void deferred_collision(HSD_GObj* object)
{
 Item* item=object->user_data;item->xDD4_itemVar.native_holdable.collision_flags=33;holdable_contact=14;
 if(reentry_mode==0)holdable_retire_in_contact=true;else holdable_retire_in_response=true;
 NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=123};NativeScriptVMStatus status=nativeHoldable_Collision(object,&result);
 reentry_guarded=status==NativeScriptVM_SourceRetired&&result.type==NativeScriptType_U64&&result.as.u64==123&&item->msid==2&&item->xDD4_itemVar.native_holdable.collision_flags==(reentry_mode==0?33:14);
}
static void retired_caller(HSD_GObj* object)
{
 scheduler_remove_other=object;NativeScriptValue result;
 NativeScriptVMStatus status=nativeHoldable_Thrown(surviving_child,&result);
 reentry_guarded=status==NativeScriptVM_Ok&&result.type==NativeScriptType_Void&&state(surviving_child,"count",NULL).as.i64==1&&nativeScriptItem_Status(object)==NativeScriptVM_SourceRetired&&nativeScriptItem_Status(surviving_child)==NativeScriptVM_Ok;
}
static void deferred_fault_remove(HSD_GObj* object)
{
 NativeScriptValue result;scheduler_fail_write=true;NativeScriptVMStatus status=nativeHoldable_Animation(object,&result);scheduler_fail_write=false;
 CHECK(status==NativeScriptVM_Error&&nativeScriptItem_Status(object)==NativeScriptVM_Error);Item_8026A8EC(object);
 reentry_guarded=nativeScriptItem_Status(object)==NativeScriptVM_SourceRetired&&nativeScriptItem_Spec(object)!=NULL;
}
static void reentrant_cases(void)
{
 NativeMatchPoolReference ref;
 for(reentry_mode=0;reentry_mode<4;++reentry_mode){HSD_GObj* object=create(custom,0);Item_80268E5C(object,2,0);CHECK(nativeMatchPool_Reference(&nativeGObjPool,object,&ref));reentry_guarded=false;HSD_GObj_SetupProc(object,deferred_hook,0);HSD_GObj_80390CFC();CHECK(reentry_guarded&&nativeMatchPool_Resolve(ref)==NULL&&nativeScriptRuntime_Idle());++cases;}
 for(reentry_mode=0;reentry_mode<2;++reentry_mode){HSD_GObj* object=create(plain,0);Item_80268E5C(object,2,0);CHECK(nativeMatchPool_Reference(&nativeGObjPool,object,&ref));reentry_guarded=false;HSD_GObj_SetupProc(object,deferred_collision,0);HSD_GObj_80390CFC();CHECK(reentry_guarded&&nativeMatchPool_Resolve(ref)==NULL);++cases;}
 HSD_GObj* object=create(custom,0);NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=123};holdable_fail_in_state=true;unsigned leaves=holdable_animation_leaves;NativeScriptVMStatus fault_status=nativeHoldable_PickedUp(object,&result);CHECK(fault_status==NativeScriptVM_Error&&result.type==NativeScriptType_U64&&result.as.u64==123&&nativeScriptItem_Status(object)==NativeScriptVM_Error&&holdable_animation_leaves==leaves);dispose(object,NULL);++cases;
 object=create(plain,0);surviving_child=create(custom,2);CHECK(nativeMatchPool_Reference(&nativeGObjPool,object,&ref));reentry_guarded=false;HSD_GObj_SetupProc(object,retired_caller,0);HSD_GObj_80390CFC();CHECK(reentry_guarded&&nativeMatchPool_Resolve(ref)==NULL&&nativeScriptItem_Status(surviving_child)==NativeScriptVM_Ok);dispose(surviving_child,NULL);++cases;
 object=create(custom,0);CHECK(nativeMatchPool_Reference(&nativeGObjPool,object,&ref));reentry_guarded=false;HSD_GObj_SetupProc(object,deferred_fault_remove,0);HSD_GObj_80390CFC();CHECK(reentry_guarded&&nativeMatchPool_Resolve(ref)==NULL);++cases;
}
int main(int argc,char** argv)
{
 prepare();HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);scheduler_reset();const char* selected=argc>1?argv[1]:"all";bool all=!strcmp(selected,"all");if(all||!strcmp(selected,"healthy")){generic_pairs();custom_hooks();}if(all||!strcmp(selected,"held"))held_collision();if(all||!strcmp(selected,"fault"))faulted_actor();if(all||!strcmp(selected,"retirement")){reentrant_cases();
 HSD_GObj* a=create(plain,0);NativeMatchPoolReference ref;CHECK(nativeMatchPool_Reference(&nativeGObjPool,a,&ref));HSD_GObj_SetupProc(a,deferred_protocol,0);HSD_GObj_80390CFC();CHECK(nativeMatchPool_Resolve(ref)==NULL);if(retired_status!=NativeScriptVM_SourceRetired||!retired_result||retired_leaves)fprintf(stderr,"retired status=%u output_untouched=%u animation_leaves=%u\n",retired_status,retired_result,retired_leaves);CHECK(retired_status==NativeScriptVM_SourceRetired&&retired_result&&retired_leaves==0);++cases;}
 CHECK(nativeScriptRuntime_Idle());printf("{\"status\":\"PASS holdable source protocol\",\"checks\":%u,\"cases\":%u,\"native_pairs\":%u}\n",checks,cases,pairs);return 0;
}
