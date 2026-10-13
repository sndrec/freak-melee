#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <melee/it/native_script_item_spawn.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_script_item_program.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftdata.h>
#include <melee/it/item.h>
#include <melee/it/types.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/native_item_lifetime.h>
#include <melee/ft/native_script_binding_spawn.h>
#include <melee/ft/native_script_actor_access.h>
#include <melee/it/native_item_owner.h>
#include <math.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/jobj.h>
#include "boundary.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum { StageConstruct=1,StageSpawned,StageState,StageAppearance,StageCreated,StageDebug,StageUpdate,StageLaunched,StagePickup,StageInventory };
unsigned spawn_stage,spawn_retire_stage,spawn_fail_stage,spawn_order[128],spawn_order_count;
HSD_GObj* spawn_caller;
const NativeScriptItemProgram* spawn_food_asset;
static unsigned checks,cases;static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"spawn %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
enum { N=4096,F=64,V=128 };
static char source_bytes[65536];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode ir[N*2];static uint32_t args[N*2];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[128];
static NativeScriptInstruction instructions[8192];static NativeScriptFunctionCode code[F];static NativeScriptExecutionFunction executions[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[16];static NativeScriptParameterSlot slots[V];
static NativeSourcePackage package;
#define fixture_bank package.compiler.bank
static NativeScriptFighterProgram root;static NativeScriptItemProgram programs[2];
static NativeScriptItemState states[2][4];
static Article assets;static ItemAttr attributes;static HSD_Joint model_joint;static ItemModelDesc model={.x0_joint=&model_joint};static ItemStateDesc descriptors[4];static ItemStateArray animation_array={descriptors,4};
static Fighter actor;static FighterBone bones[140];static HSD_JObj bone_joint;
#define data package.assets.character.data
#define part_table package.assets.parts
static FighterPartsTable* part_tables[Ft_Kind_RuntimeCapacity];
FighterPartsTable** ftPartsTable=part_tables;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];
MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind){return kind==Ft_Kind_CustomFirst?&package:NULL;}
static NativeScriptRuntimeContext context;
const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind kind){return kind==Ft_Kind_CustomFirst?&root:NULL;}
const NativeScriptItemProgram* nativeScriptItem_Program(FighterKind author,uint32_t owner){for(unsigned i=0;i<2;++i)if(author==Ft_Kind_CustomFirst&&programs[i].owner==owner)return &programs[i];return NULL;}
static bool metadata(void* unused,const NativeScriptSourceTree* tree,uint32_t n,NativeScriptDiagnostic* d)
{
 (void)unused;const NativeScriptSourceNode* node=&tree->nodes[n];
 if(node->kind==NativeScriptSource_Article)return nativeScriptSource_SpanEquals(tree,node->value,"states 2");
 if(node->kind==NativeScriptSource_Item)return nativeScriptSource_SpanEquals(tree,node->value,"damage 4 angle 45 lifetime 90");
 if(node->kind==NativeScriptSource_ArticleState)return true;
 if(node->kind==NativeScriptSource_State||node->kind==NativeScriptSource_Function)return true;
 if(node->kind==NativeScriptSource_Callback){NativeScriptEventResolution e;return nativeScriptEvents_Resolve(tree,node,&e,d);}
 return nativeScriptCompile_MinimalMetadata(NULL,tree,n,d);
}
static float creation_value(Item_GObj* object){float value;CHECK(nativeScriptItem_GetCreationValue(object,&value));return value;}
static void prepare(void)
{
 fixture_bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=N*2,.arguments=args,.argument_capacity=N*2},
  .declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=128,
  .instructions=instructions,.instruction_capacity=8192,.function_code=code,.function_code_capacity=F,.execution_functions=executions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,
  .storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=16},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* text="fighter:\n    behavior independent\nstate int caller_count\nstate int handle\nstate int tail\n"
  "action Caller:\n    on load:\n        state.caller_count = 1\n"
  "func Spawn(float x, float y, float z, int raw, float face) -> int:\n    state.handle = articles.create_held_at_center(It_Kind_Fox_Laser, x, y, z, raw, face)\n    state.tail = state.tail + 1\n    return state.handle\n"
  "func Wide():\n    state.handle = articles.create_held_at_center(It_Kind_Fox_Laser, 1, 2, 3, 4294967296, 1)\n    state.tail = 77\n"
  "func UnsignedWide():\n    state.handle = articles.create_held_at_center(It_Kind_Fox_Laser, 1, 2, 3, 0xFFFFFFFFFFFFFFFF, 1)\n    state.tail = 77\n"
  "article It_Kind_Fox_Laser states 2:\n    state int order\n    state float spawned_value\n    state float created_value\n    on spawned:\n            item.state.order = 1\n            item.state.spawned_value = item.creation_value\n            position.x = 101\n"
  "    on created:\n            item.state.order = item.state.order * 10 + 2\n            item.state.created_value = item.creation_value\n            position.x = 103\n"
  "    on destroyed:\n            item.state.order = item.state.order + 1000\n    on picked_up:\n            item.state.order = item.state.order * 10 + 3\n            position.x = 109\n"
  "    state 0 anim -1:\n    state 1 anim -1:\n"
  "article Twin states 2:\n    state int count\n    state 0 anim -1:\n    state 1 anim -1:\n";
 NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};
 NativeScriptCompileRequest request={.motion_layout=&layout,.source=text,.source_bytes=(uint32_t)strlen(text),.metadata=metadata};const NativeScriptCompileBank* accepted=NULL;
 CHECK(nativeScriptCompile_Build(&fixture_bank,&request,&accepted,&diagnostic));assets=(Article){.x0_common_attr=&attributes,.x10_modelDesc=&model,.xC_itemStates=&animation_array};
 package.address=&package;package.ready=package.assets.ready=true;package.target=Ft_Kind_CustomFirst;package.assets.facts.moves=fixture_bank.motion_layout;package.assets.facts.joints=140;part_table.parts_num=140;package.program.identity=44;
 gFtDataList[Ft_Kind_CustomFirst]=&data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=package.assets.motions;
 root=(NativeScriptFighterProgram){.bank=&fixture_bank,.identity=44};for(unsigned h=0;h<NativeScriptFighter_HookCount;++h)root.hooks[h]=UINT32_MAX;for(unsigned h=0;h<NativeScriptFighterCallback_Count;++h)root.callbacks[h]=UINT32_MAX;
 unsigned entity=0;for(unsigned n=0;n<fixture_bank.tree.count;++n)if(nodes[n].kind==NativeScriptSource_Article||nodes[n].kind==NativeScriptSource_Item){
  programs[entity]=(NativeScriptItemProgram){.bank=&fixture_bank,.identity=100+entity,.author_kind=Ft_Kind_CustomFirst,.kind=It_Kind_Mario_Fire,.recipe_index=UINT32_MAX,.owner=n,.article=&assets,.states=states[entity],.state_count=2,.recipe=NULL};
  programs[entity].drawing=nodes[n].kind==NativeScriptSource_Item?NativeScriptItemDrawing_Billboard:NativeScriptItemDrawing_Standard;
  for(unsigned h=0;h<NativeScriptItemHook_Count;++h)programs[entity].hooks[h]=UINT32_MAX;
  for(unsigned s=0;s<4;++s){states[entity][s].animation_id=-1;for(unsigned p=0;p<3;++p)states[entity][s].functions[p]=UINT32_MAX;}++entity;
 }
 for(unsigned f=0;f<fixture_bank.declarations.function_count;++f)if(functions[f].callback){NativeScriptEventResolution e;CHECK(nativeScriptEvents_Resolve(&fixture_bank.tree,&nodes[functions[f].source_node],&e,&diagnostic));
  if(!functions[f].owner){root.hooks[NativeScriptFighter_Load]=f;continue;}
  unsigned h=!strcmp(e.descriptor->name,"spawned")?NativeScriptItemHook_Spawned:!strcmp(e.descriptor->name,"created")?NativeScriptItemHook_Created:
    !strcmp(e.descriptor->name,"launched")?NativeScriptItemHook_Launched:!strcmp(e.descriptor->name,"destroyed")?NativeScriptItemHook_Destroyed:
    !strcmp(e.descriptor->name,"picked_up")?NativeScriptItemHook_PickedUp:NativeScriptItemHook_WorkshopTick;
  for(unsigned p=0;p<2;++p)if(programs[p].owner==functions[f].owner)programs[p].hooks[h]=f;
 }
 scheduler_spec=&programs[0];spawn_food_asset=&programs[0];
}
static void fighter_remove(void* data_ptr){Fighter* fp=data_ptr;nativeScriptFighter_Detach(fp->gobj);}
static void caller(void)
{
 if(spawn_caller){HSD_GObjFree(spawn_caller);spawn_caller=NULL;}
 memset(&actor,0,sizeof(actor));spawn_caller=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(spawn_caller);
 actor.gobj=spawn_caller;actor.kind=actor.behavior_kind=Ft_Kind_CustomFirst;actor.x8_spawnNum=777;actor.facing_dir=-1;actor.cur_pos=(Vec3){4,5,6};actor.coll_data.ecb.top.y=8;actor.coll_data.ecb.bottom.y=-2;actor.parts=bones;actor.ft_data=&data;
 bone_joint.translate=(Vec3){10,11,12};memset(bones,0,sizeof(bones));bones[0].joint=&bone_joint;bones[64].joint=&bone_joint;part_tables[Ft_Kind_CustomFirst]=&part_table;
 static struct ftData_x8 part_info;data.x8=&part_info;data.x8->x10=0;
 GObj_InitUserData(spawn_caller,4,fighter_remove,&actor);CHECK(nativeScriptFighter_Attach(spawn_caller));
 context=(NativeScriptRuntimeContext){.source=spawn_caller,.source_class=NativeCallbackSource_Fighter,.generation=777,.actor_id=777,.scene_epoch=nativeGObjPool.lifetime};
 spawn_order_count=spawn_retire_stage=0;
}
static void remove_item(Item_GObj* item){if(item&&nativeScriptItem_IsAttached(item))Item_8026A8EC(item);else if(item)HSD_GObjFree(item);}

bool positional_oom;
SpawnItem positional_spawn,positional_initialized;
unsigned held_sound_tail;
static unsigned vm_calls,pairs,mode,trigger,raw_seen,attach_count;
static Item_GObj* last_child;
static HSD_JObj other_joint;
static FighterBone other_parts[MAX_FT_PARTS];
static Item replacement_data;
static Item* original_payload;
static HSD_JObj* original_model;
static const NativeScriptStorageLayout* saved_layout;
static const NativeScriptStorageParameters* saved_parameters;
static NativeScriptStorageLayout changed_layout;
static NativeScriptStorageParameters changed_parameters;
enum {RetireCaller=1,RemoveChild,FaultChild,ChangeParts,ChangeJoint,ChangeModel,
    ChangeClass,ChangeUserdata,ChangeProgram,ChangeSchema,ChangeParameters,
    ChangeStateOwner,ClearAttachment,NestedCreation,ReuseChild,ReuseCaller,VMFault,ChangeJointClass,ChangeFighterModel};
static NativeScriptItemActor* actor_for(HSD_GObj* object)
{for(unsigned i=0;i<NATIVE_SCRIPT_ITEM_ACTORS;++i)if(nativeScriptItemActors[i].active&&nativeScriptItemActors[i].object==object)return &nativeScriptItemActors[i];return NULL;}
static NativeScriptVMStatus invoke(Item_GObj** output,uint32_t raw)
{Vec3 previous={1.25f,-0.0f,3.75f};return nativeScriptItemSpawn_HeldAtCenter(&context,programs[0].owner,&previous,raw,-1,output,&diagnostic);}
void held_native_attach(HSD_GObj* object,HSD_GObj* parent,Fighter_Part raw)
{++attach_count;raw_seen=(uint32_t)raw;((Item*)object->user_data)->owner=parent;}
void positional_stage(unsigned stage,HSD_GObj* child)
{
 spawn_stage=stage;CHECK(spawn_order_count<128);spawn_order[spawn_order_count++]=stage;
 if(stage==StageSpawned || stage==StageCreated || stage==StagePickup){
  CHECK(child&&nativeScriptItem_IsAttached(child));last_child=child;
  CHECK(nativeScriptItem_Creator(child)==spawn_caller);
  CHECK(nativeScriptItem_Spec(child)==&programs[0]);
  CHECK(creation_value(child)==0);
 }
 if(stage==StageSpawned || stage==StageCreated)CHECK(!attach_count);
 if(stage==StagePickup)CHECK(attach_count==1);
 if(!mode || stage!=trigger)return;
 unsigned action=mode;mode=0;
 if(action==RetireCaller)nativeScriptFighter_Detach(spawn_caller);
 else if(action==RemoveChild)Item_8026A8EC(child);
 else if(action==FaultChild){NativeScriptItemActor* a=actor_for(child);CHECK(a);a->faulted=true;}
 else if(action==VMFault)scheduler_fail_write=true;
 else if(action==ChangeJointClass)bone_joint.object.parent.class_info=(HSD_ClassInfo*)&other_joint;
 else if(action==ChangeFighterModel)spawn_caller->hsd_obj=&other_joint;
 else if(action==ChangeParts)actor.parts=other_parts;
 else if(action==ChangeJoint)bones[64].joint=&other_joint;
 else if(action==ChangeModel){original_model=child->hsd_obj;child->hsd_obj=&other_joint;}
 else if(action==ChangeClass)child->classifier=HSD_GOBJ_CLASS_FIGHTER;
 else if(action==ChangeUserdata){original_payload=child->user_data;replacement_data=*original_payload;child->user_data=&replacement_data;}
 else if(action==ChangeProgram)++programs[0].identity;
 else if(action==ChangeSchema){NativeScriptItemActor* a=actor_for(child);CHECK(a);saved_layout=a->storage.layout;changed_layout=*saved_layout;++changed_layout.identity;a->storage.layout=&changed_layout;}
 else if(action==ChangeParameters){NativeScriptItemActor* a=actor_for(child);CHECK(a);saved_parameters=a->storage.parameters;changed_parameters=*saved_parameters;++changed_parameters.identity;a->storage.parameters=&changed_parameters;}
 else if(action==ChangeStateOwner){((Item*)child->user_data)->owner=NULL;Item_80268E5C(child,1,0);}
 else if(action==ClearAttachment)CHECK(nativeScriptItem_ClearAttachmentOwner(child));
 else if(action==NestedCreation){Item_GObj* inner=NULL;unsigned old_attach=attach_count;CHECK(invoke(&inner,0)==NativeScriptVM_Ok&&inner);CHECK(actor.item_gobj==inner);attach_count=old_attach;}
 else if(action==ReuseChild){NativeMatchPoolReference old;CHECK(nativeMatchPool_Reference(&nativeGObjPool,child,&old));Item_8026A8EC(child);CHECK(!nativeMatchPool_Resolve(old));HSD_GObj* replacement=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(replacement==child);}
 else if(action==ReuseCaller){NativeMatchPoolReference old;CHECK(nativeMatchPool_Reference(&nativeGObjPool,spawn_caller,&old));HSD_GObjFree(spawn_caller);CHECK(!nativeMatchPool_Resolve(old));HSD_GObj* replacement=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(replacement==spawn_caller);}
 else CHECK(false);
}
#include "native_reference.inc"
static void clear_items(void)
{while(HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM])remove_item(HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM]);actor.item_gobj=NULL;}
static void reset(void)
{
 clear_items();caller();mode=trigger=spawn_order_count=attach_count=raw_seen=held_sound_tail=0;
 positional_oom=false;last_child=NULL;saved_layout=NULL;saved_parameters=NULL;original_payload=NULL;original_model=NULL;
 scheduler_fail_write=scheduler_fail_selector=false;diagnostic=(NativeScriptDiagnostic){0};
 bones[64].joint=&bone_joint;bone_joint.object.parent.class_info=NULL;actor.parts=bones;part_table.parts_num=140;package.assets.facts.joints=140;package.assets.facts.omission_count=0;
}
static int64_t state_value(HSD_GObj* object,uint32_t owner,const char* name)
{
 NativeScriptReceiverView view;uint32_t generation=(uint32_t)(owner?((Item*)object->user_data)->x1C:actor.x8_spawnNum);
 CHECK(nativeScriptActors_Lookup(NULL,object,owner?NativeCallbackSource_Item:NativeCallbackSource_Fighter,generation,&view));
 for(unsigned i=0;i<fixture_bank.declarations.value_count;++i)if(values[i].owner==owner&&nativeScriptSource_SpanEquals(&fixture_bank.tree,values[i].name,name)){
  NativeScriptValue value;CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&value,&diagnostic));return value.as.i64;
 }
 CHECK(false);return 0;
}
static NativeScriptVMStatus public_spawn(int32_t raw,NativeScriptValue* out)
{
 uint32_t id=UINT32_MAX;for(unsigned i=0;i<fixture_bank.declarations.function_count;++i)if(nativeScriptSource_SpanEquals(&fixture_bank.tree,functions[i].name,"Spawn"))id=i;
 CHECK(id!=UINT32_MAX);NativeScriptValue a[]={{.type=NativeScriptType_F32,.as.f32=1.25f},{.type=NativeScriptType_F32,.as.f32=-0.0f},
  {.type=NativeScriptType_F32,.as.f32=3.75f},{.type=NativeScriptType_I32,.as.i64=raw},{.type=NativeScriptType_F32,.as.f32=-1}};
 ++vm_calls;return nativeScriptFighter_Helper(spawn_caller,context.generation,id,a,5,out,&diagnostic);
}
static void healthy(void)
{
 reset();NativeScriptValue output={.type=NativeScriptType_I32,.as.i64=-91};CHECK(public_spawn(64,&output)==NativeScriptVM_Ok);
 Item_GObj* object=actor.item_gobj;CHECK(object&&output.as.i64==(int32_t)((Item*)object->user_data)->x1C);
 CHECK(raw_seen==64&&attach_count==1&&held_sound_tail==1);
 CHECK(state_value(object,programs[0].owner,"order")==123&&state_value(spawn_caller,0,"tail")==1);
 CHECK(nativeScriptItem_Creator(object)==spawn_caller&&nativeScriptItem_AttachmentOwner(object)==spawn_caller);
 Vec3 previous={1.25f,-0.0f,3.75f};SpawnItem reference=native_spawn_arguments(spawn_caller,&previous,64,programs[0].kind,-1);
 CHECK(!memcmp(&reference.prev_pos,&positional_spawn.prev_pos,sizeof(Vec3))&&!memcmp(&reference.pos,&positional_spawn.pos,sizeof(Vec3)));
 CHECK(positional_spawn.pos.x==4&&positional_spawn.pos.y==8&&positional_spawn.pos.z==6);
 CHECK(!memcmp(&reference.vel,&positional_spawn.vel,sizeof(Vec3))&&reference.kind==positional_spawn.kind&&reference.facing_dir==positional_spawn.facing_dir);
 CHECK(positional_spawn.x0_parent_gobj==spawn_caller&&positional_spawn.x4_parent_gobj2==spawn_caller&&positional_spawn.x44_flag.b0&&positional_initialized.x48_ground_or_air==GA_Air);
 const unsigned order[]={StageConstruct,StageSpawned,StageCreated,StagePickup};CHECK(spawn_order_count==4&&!memcmp(order,spawn_order,sizeof(order)));++pairs;++cases;
 for(unsigned i=0;i<64;++i){reset();actor.cur_pos=(Vec3){i*0.125f-4,-((float)i),i*0.75f};actor.coll_data.ecb.top.y=i*0.5f;actor.coll_data.ecb.bottom.y=-i*0.125f;
  previous=(Vec3){i*0.375f,-0.0f,i*0.25f};reference=native_spawn_arguments(spawn_caller,&previous,64,programs[0].kind,(i&1)?-1:1);object=NULL;
  CHECK(nativeScriptItemSpawn_HeldAtCenter(&context,programs[0].owner,&previous,64,(i&1)?-1:1,&object,&diagnostic)==NativeScriptVM_Ok&&object);
  CHECK(!memcmp(&reference.pos,&positional_spawn.pos,sizeof(Vec3))&&!memcmp(&reference.prev_pos,&positional_spawn.prev_pos,sizeof(Vec3)));++pairs;
 }++cases;
}
static void absent_and_invalid(void)
{
 for(unsigned i=0;i<3;++i){reset();NativeScriptValue output={.type=NativeScriptType_I32,.as.i64=-91};int32_t raw=i==0?-1:i==1?140:63;
  CHECK(public_spawn(raw,&output)==NativeScriptVM_Ok&&output.as.i64==-1&&state_value(spawn_caller,0,"tail")==1&&!spawn_order_count);++cases;}
 reset();positional_oom=true;NativeScriptValue out={.type=NativeScriptType_I32,.as.i64=-91};CHECK(public_spawn(64,&out)==NativeScriptVM_Ok&&out.as.i64==-1&&!actor.item_gobj);++cases;
 for(unsigned flag=0;flag<2;++flag){reset();if(flag)scheduler_fail_write=true;else scheduler_fail_selector=true;out=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=-91};
  CHECK(public_spawn(64,&out)==NativeScriptVM_Ok&&out.as.i64==-1&&!actor.item_gobj&&!HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM]);++cases;}
 reset();Vec3 p={1,NAN,3};Item_GObj* native=(void*)1;CHECK(nativeScriptItemSpawn_HeldAtCenter(&context,programs[0].owner,&p,64,-1,&native,&diagnostic)==NativeScriptVM_Error&&native==(void*)1&&!spawn_order_count);++cases;
 reset();part_table.parts_num=139;CHECK(invoke(&native,64)==NativeScriptVM_Error&&native==(void*)1);part_table.parts_num=140;++cases;
 reset();actor.parts=NULL;CHECK(invoke(&native,64)==NativeScriptVM_Error&&native==(void*)1);actor.parts=bones;++cases;
 reset();package.assets.facts.joints=139;package.assets.facts.omission_count=1;native=NULL;CHECK(invoke(&native,64)==NativeScriptVM_Ok&&native);++cases;
 reset();package.assets.facts.joints=140;package.assets.facts.omission_count=1;native=(void*)1;CHECK(invoke(&native,64)==NativeScriptVM_Error&&native==(void*)1);package.assets.facts.omission_count=0;++cases;
 reset();native=NULL;p=(Vec3){1,2,3};CHECK(nativeScriptItemSpawn_HeldAtCenter(&context,programs[1].owner,&p,64,-1,&native,&diagnostic)==NativeScriptVM_Ok&&native&&nativeScriptItem_Spec(native)==&programs[1]);++cases;
 reset();NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=programs[0].owner},{.type=NativeScriptType_F32,.as.f32=1},{.type=NativeScriptType_F32,.as.f32=2},{.type=NativeScriptType_F32,.as.f32=3},{.type=NativeScriptType_I32,.as.i64=INT64_C(4294967296)},{.type=NativeScriptType_F32,.as.f32=-1}};
 uint32_t semantic=NATIVE_SCRIPT_SPAWN_DOMAIN|(NativeScriptSpawn_HeldAtCenter<<20)|programs[0].owner;out=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=-91};
 CHECK(!nativeScriptBindingSpawn_Invoke(&context,semantic,args,6,&out,&diagnostic)&&out.as.i64==-91&&!spawn_order_count);
 args[4].as.i64=64;args[0].as.i64=INT64_MAX;CHECK(!nativeScriptBindingSpawn_Invoke(&context,semantic,args,6,&out,&diagnostic)&&out.as.i64==-91);++cases;
}
static void wide_inputs(void)
{
 const char* names[]={"Wide","UnsignedWide"};
 for(unsigned n=0;n<2;++n){reset();uint32_t id=UINT32_MAX;for(unsigned i=0;i<fixture_bank.declarations.function_count;++i)if(nativeScriptSource_SpanEquals(&fixture_bank.tree,functions[i].name,names[n]))id=i;CHECK(id!=UINT32_MAX);
  NativeScriptValue out={.type=NativeScriptType_I32,.as.i64=-91};++vm_calls;
  CHECK(nativeScriptFighter_Helper(spawn_caller,context.generation,id,NULL,0,&out,&diagnostic)==NativeScriptVM_Error&&out.as.i64==-91&&!spawn_order_count&&!actor.item_gobj);++cases;
 }
}
static void lifetimes(void)
{
 const unsigned actions[]={RetireCaller,RemoveChild,FaultChild,ChangeParts,ChangeJoint,ChangeModel,ChangeClass,ChangeUserdata,ChangeProgram,ChangeSchema,ChangeParameters,ReuseChild,ReuseCaller,VMFault,ChangeJointClass,ChangeFighterModel};
 for(unsigned stage=StageSpawned;stage<=StagePickup;++stage){if(stage!=StageSpawned&&stage!=StageCreated&&stage!=StagePickup)continue;
  for(unsigned i=0;i<sizeof(actions)/sizeof(actions[0]);++i){
   if(stage==StageSpawned&&(actions[i]==ChangeModel||actions[i]==ChangeClass||actions[i]==ChangeUserdata||actions[i]==ChangeSchema||actions[i]==ChangeParameters||actions[i]==ReuseChild))continue;reset();mode=actions[i];trigger=stage;
   NativeScriptValue out={.type=NativeScriptType_I32,.as.i64=-91};uint64_t identity=programs[0].identity;
   NativeScriptVMStatus result=public_spawn(64,&out);
   bool ctor_null=stage==StageSpawned&&(actions[i]==RemoveChild||actions[i]==ChangeProgram||actions[i]==VMFault);
   if(ctor_null){if(result!=NativeScriptVM_Ok||out.as.i64!=-1)fprintf(stderr,"stage%u action%u status%d out%lld\n",stage,actions[i],result,(long long)out.as.i64);CHECK(result==NativeScriptVM_Ok&&out.as.i64==-1);}
   else{if(result==NativeScriptVM_Ok||out.as.i64!=-91||actor.item_gobj)fprintf(stderr,"stage%u action%u status%d out%lld held%p\n",stage,actions[i],result,(long long)out.as.i64,(void*)actor.item_gobj);CHECK(result!=NativeScriptVM_Ok&&out.as.i64==-91&&!actor.item_gobj);}
   programs[0].identity=identity;
   if(last_child&&nativeScriptItem_IsAttached(last_child)){
    NativeScriptItemActor* a=actor_for(last_child);CHECK(a);
    // Capture the controlled original payload from the actor's retained context
    // before each case, rather than accepting a replacement as the created Item.
    if(actions[i]==ChangeUserdata&&original_payload)last_child->user_data=original_payload;
    if(actions[i]==ChangeModel&&original_model)last_child->hsd_obj=original_model;
    if(actions[i]==ChangeClass)last_child->classifier=HSD_GOBJ_CLASS_ITEM;
    if(actions[i]==ChangeSchema&&saved_layout)a->storage.layout=saved_layout;
    if(actions[i]==ChangeParameters&&saved_parameters)a->storage.parameters=saved_parameters;
   }
   ++cases;
  }
 }
 for(unsigned i=0;i<3;++i){reset();mode=i==0?ChangeStateOwner:i==1?ClearAttachment:NestedCreation;trigger=StageCreated;
  NativeScriptValue out={.type=NativeScriptType_I32,.as.i64=-91};CHECK(public_spawn(64,&out)==NativeScriptVM_Ok&&actor.item_gobj&&out.as.i64>=0);
  if(i==0)CHECK(((Item*)actor.item_gobj->user_data)->msid==1);
  if(i==1)CHECK(nativeScriptItem_AttachmentOwner(actor.item_gobj)==NULL&&nativeScriptItem_Creator(actor.item_gobj)==spawn_caller);
  CHECK(state_value(spawn_caller,0,"tail")==1);++cases;
 }
}
static void full_handles(void)
{
 extern u32 it_804D6D10;const uint32_t ids[]={UINT32_C(0x80000000),UINT32_MAX};
 for(unsigned n=0;n<2;++n){reset();uint32_t saved=it_804D6D10;it_804D6D10=ids[n];
  NativeScriptValue out={.type=NativeScriptType_I32,.as.i64=-91};CHECK(public_spawn(64,&out)==NativeScriptVM_Ok&&actor.item_gobj);
  int32_t signed_id;memcpy(&signed_id,&ids[n],sizeof(signed_id));
  CHECK(out.as.i64==signed_id&&(uint32_t)((Item*)actor.item_gobj->user_data)->x1C==ids[n]);
  CHECK(state_value(spawn_caller,0,"handle")==signed_id&&state_value(spawn_caller,0,"tail")==1);
  it_804D6D10=saved;++cases;
 }
}
static void compiler_rejections(void)
{
 const char* bodies[]={
  "func Bad():\n    articles.create_held_at_center(It_Kind_Fox_Laser, 1, 2, 3, 0.5, 1)\n",
  "func Bad():\n    articles.create_held_at_center(It_Kind_Fox_Laser, 1, 2, 3, 64)\n",
  "parameter int selected_kind = 72\nfunc Bad():\n    articles.create_held_at_center(attrs.selected_kind, 1, 2, 3, 64, 1)\n",
  "func Bad():\n    articles.create_held_at_center(72, 1, 2, 3, 64, 1)\n",
  "state int It_Kind_Fox_Laser\nfunc Bad():\n    articles.create_held_at_center(It_Kind_Fox_Laser, 1, 2, 3, 64, 1)\n",
  "article Other states 2:\n    on created:\n            articles.create_held_at_center(It_Kind_Fox_Laser, 1, 2, 3, 64, 1)\n    state 0 anim -1:\n    state 1 anim -1:\n",
 };
 for(unsigned i=0;i<sizeof(bodies)/sizeof(bodies[0]);++i){char input[2048];
  snprintf(input,sizeof(input),"fighter:\n    behavior independent\n%sarticle It_Kind_Fox_Laser states 2:\n    state 0 anim -1:\n    state 1 anim -1:\n",bodies[i]);
  NativeScriptCompileRequest request={.source=input,.source_bytes=(uint32_t)strlen(input),.metadata=metadata};
  const NativeScriptCompileBank* accepted=NULL;
  CHECK(!nativeScriptCompile_Build(&fixture_bank,&request,&accepted,&diagnostic)&&accepted==NULL&&diagnostic.message[0]);++cases;
 }
 prepare();
}
int main(void)
{
 prepare();compiler_rejections();HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);scheduler_reset();
 healthy();absent_and_invalid();wide_inputs();full_handles();lifetimes();clear_items();HSD_GObjFree(spawn_caller);spawn_caller=NULL;
 printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"native_argument_pairs\":%u,\"vm_calls\":%u}\n",checks,cases,pairs,vm_calls);return 0;
}
