#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <melee/it/native_script_item_spawn.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_script_item_program.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/native_source_package.h>
#include <melee/it/item.h>
#include <melee/it/types.h>
#include <melee/it/native_script_item_actor_internal.h>
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
static NativeScriptCompileBank bank;static NativeScriptFighterProgram root;static NativeScriptItemProgram programs[2];
static NativeScriptItemState states[2][4];
static Article assets;static ItemAttr attributes;static HSD_Joint model_joint;static ItemModelDesc model={.x0_joint=&model_joint};static ItemStateDesc descriptors[4];static ItemStateArray animation_array={descriptors,4};
static Fighter actor;static FighterBone bones[140];static HSD_JObj bone_joint;static ftData data;static FighterPartsTable part_table={.parts_num=140};static FighterPartsTable* part_tables[Ft_Kind_RuntimeCapacity];
FighterPartsTable** ftPartsTable=part_tables;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];
MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
// This constructor fixture uses a stock-shaped Fighter rig and controlled
// assets; it does not claim archive-backed source-package layout acceptance.
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind){(void)kind;return NULL;}
static NativeScriptRuntimeContext context;
const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind kind){return kind==Ft_Kind_Mario?&root:NULL;}
const NativeScriptItemProgram* nativeScriptItem_Program(FighterKind author,uint32_t owner){for(unsigned i=0;i<2;++i)if(author==Ft_Kind_Mario&&programs[i].owner==owner)return &programs[i];return NULL;}
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
 bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=N*2,.arguments=args,.argument_capacity=N*2},
  .declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=128,
  .instructions=instructions,.instruction_capacity=8192,.function_code=code,.function_code_capacity=F,.execution_functions=executions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,
  .storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=16},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* text="fighter:\n    behavior independent\nstate int caller_count\nstate int unowned_handle\naction Caller:\n    on load:\n        state.caller_count = 1\nfunc SpawnUnowned():\n    state.unowned_handle = articles.create_unowned_at_position(Shot, 1.25, -0.0, 3.75, -2.5, 0.03125, -0.0, -1)\n"
  "article Shot states 2:\n    state int count\n    on spawned:\n            item.state.count = 1\n            position.x = 101\n"
  "    on created:\n            item.state.count = item.state.count + 3\n            position.x = 103\n"
  "    on launched:\n            item.state.count = item.state.count + 7\n            position.x = 107\n"
  "    on destroyed:\n            item.state.count = item.state.count + 100\n    on picked_up:\n            item.state.count = item.state.count + 5\n            position.x = 109\n"
  "    state 0 anim -1:\n    state 1 anim -1:\n"
  "article Child states 2:\n    state int count\n    on spawned:\n            position.x = 101\n    on created:\n            item.state.count = item.state.count + 5\n            position.x = 103\n    on destroyed:\n            item.state.count = item.state.count + 100\n    state 0 anim -1:\n    state 1 anim -1:\n";
 NativeScriptCompileRequest request={.source=text,.source_bytes=(uint32_t)strlen(text),.metadata=metadata};const NativeScriptCompileBank* accepted=NULL;
 CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));assets=(Article){.x0_common_attr=&attributes,.x10_modelDesc=&model,.xC_itemStates=&animation_array};
 root=(NativeScriptFighterProgram){.bank=&bank,.identity=44};for(unsigned h=0;h<NativeScriptFighter_HookCount;++h)root.hooks[h]=UINT32_MAX;for(unsigned h=0;h<NativeScriptFighterCallback_Count;++h)root.callbacks[h]=UINT32_MAX;
 unsigned entity=0;for(unsigned n=0;n<bank.tree.count;++n)if(nodes[n].kind==NativeScriptSource_Article||nodes[n].kind==NativeScriptSource_Item){
  programs[entity]=(NativeScriptItemProgram){.bank=&bank,.identity=100+entity,.author_kind=Ft_Kind_Mario,.kind=It_Kind_Mario_Fire,.recipe_index=UINT32_MAX,.owner=n,.article=&assets,.states=states[entity],.state_count=2,.recipe=NULL};
  for(unsigned h=0;h<NativeScriptItemHook_Count;++h)programs[entity].hooks[h]=UINT32_MAX;
  for(unsigned s=0;s<4;++s){states[entity][s].animation_id=-1;for(unsigned p=0;p<3;++p)states[entity][s].functions[p]=UINT32_MAX;}++entity;
 }
 for(unsigned f=0;f<bank.declarations.function_count;++f)if(functions[f].callback){NativeScriptEventResolution e;CHECK(nativeScriptEvents_Resolve(&bank.tree,&nodes[functions[f].source_node],&e,&diagnostic));
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
 actor.gobj=spawn_caller;actor.kind=Ft_Kind_Mario;actor.x8_spawnNum=777;actor.facing_dir=-1;actor.cur_pos=(Vec3){4,5,6};actor.parts=bones;actor.ft_data=&data;
 bone_joint.translate=(Vec3){10,11,12};bones[0].joint=&bone_joint;part_tables[Ft_Kind_Mario]=&part_table;
 static struct ftData_x8 part_info;data.x8=&part_info;data.x8->x10=0;
 GObj_InitUserData(spawn_caller,4,fighter_remove,&actor);CHECK(nativeScriptFighter_Attach(spawn_caller));
 context=(NativeScriptRuntimeContext){.source=spawn_caller,.source_class=NativeCallbackSource_Fighter,.generation=777,.actor_id=777,.scene_epoch=nativeGObjPool.lifetime};
 spawn_order_count=spawn_retire_stage=0;
}
static void remove_item(Item_GObj* item){if(item&&nativeScriptItem_IsAttached(item))Item_8026A8EC(item);else if(item)HSD_GObjFree(item);}

bool positional_oom;
SpawnItem positional_spawn,positional_initialized;
static HSD_GObj* expected_creator;
static HSD_GObj* damage_owner;
static HSD_GObj* parent_item;
static Fighter reflected_actor;
static unsigned mode,trigger,observed_spawned,observed_created;
static bool check_relations;
static bool attachment_cleared;
enum {RetireCaller=1,RetireOriginal,RetireDamage,RemoveCreated,FaultCreated,RemoveParent};
void positional_stage(unsigned stage,HSD_GObj* child)
{
    spawn_stage=stage;CHECK(spawn_order_count<128);spawn_order[spawn_order_count++]=stage;
    if(stage==StageSpawned){++observed_spawned;Item* data=child->user_data;
        if(check_relations){CHECK(data->native_creator==expected_creator);CHECK(nativeItem_Creator(child)==expected_creator);CHECK(data->owner==damage_owner);}}
    if(stage==StageCreated){++observed_created;Item* data=child->user_data;
        if(check_relations){CHECK(nativeScriptItem_AttachmentOwner(child)==(attachment_cleared?NULL:expected_creator));CHECK(nativeScriptItem_Creator(child)==expected_creator);CHECK(data->owner==damage_owner);}}
    if(mode&&stage==trigger){unsigned requested=mode;mode=0;
        if(requested==RetireCaller)nativeScriptFighter_Detach(spawn_caller);
        else if(requested==RetireOriginal)nativeScriptFighter_Detach(expected_creator);
        else if(requested==RetireDamage)++reflected_actor.x8_spawnNum;
        else if(requested==RemoveCreated)Item_8026A8EC(child);
        else if(requested==FaultCreated){for(unsigned i=0;i<NATIVE_SCRIPT_ITEM_ACTORS;++i)if(nativeScriptItemActors[i].active&&nativeScriptItemActors[i].object==child)nativeScriptItemActors[i].faulted=true;}
        else if(requested==RemoveParent){Item_8026A8EC(parent_item);parent_item=NULL;}
        else abort();
    }
}
static void reset_records(void){spawn_order_count=observed_spawned=observed_created=0;mode=trigger=0;positional_oom=false;attachment_cleared=false;memset(&positional_spawn,0,sizeof(positional_spawn));}
static HSD_GObj* find_item(uint32_t owner)
{for(HSD_GObj* o=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];o;o=o->next)if(((Item*)o->user_data)->native_script_owner==owner)return o;return NULL;}
static NativeScriptVMStatus at(NativeScriptRuntimeContext* c,unsigned entity,bool center,bool initial,Item_GObj** out)
{
    const Vec3 position={1.25f,-0.0f,3.75f},velocity={-2.5f,0.03125f,-0.0f};
    return nativeScriptItemSpawn_At(c,programs[entity].owner,&position,&velocity,-1,center,UINT32_C(0xFEDCBA98),7.125f,initial,true,out,&diagnostic);
}
static void spawn_fields(bool center,bool initial,HSD_GObj* creation_parent)
{
    const Vec3 supplied={1.25f,-0.0f,3.75f},velocity={-2.5f,0.03125f,-0.0f};
    CHECK(!memcmp(&positional_spawn.prev_pos,&supplied,sizeof(supplied)));
    CHECK(!memcmp(&positional_spawn.pos,center?&((Fighter*)damage_owner->user_data)->cur_pos:&supplied,sizeof(supplied)));
    CHECK(!memcmp(&positional_spawn.vel,&velocity,sizeof(velocity)));
    CHECK(positional_spawn.x0_parent_gobj==damage_owner&&positional_spawn.x4_parent_gobj2==creation_parent);
    CHECK(positional_spawn.facing_dir==-1&&positional_spawn.x44_flag.b0==initial);
    uint32_t group;memcpy(&group,&positional_spawn.x40,sizeof(group));CHECK(group==UINT32_C(0xFEDCBA98));
    CHECK(positional_spawn.x48_ground_or_air==0&&positional_spawn.hold_kind==0&&positional_spawn.x10==0&&positional_spawn.x3C_damage==0);
    CHECK(positional_initialized.x48_ground_or_air==GA_Air&&positional_initialized.x10==0);
}
static void child_context(NativeScriptRuntimeContext* result)
{
    check_relations=true;expected_creator=damage_owner=spawn_caller;reset_records();parent_item=NULL;
    CHECK(at(&context,0,false,true,&parent_item)==NativeScriptVM_Ok&&parent_item);
    Item* parent=parent_item->user_data;
    *result=(NativeScriptRuntimeContext){.source=parent_item,.source_class=NativeCallbackSource_Item,.generation=(uint32_t)parent->x1C,.actor_id=(uint32_t)parent->x1C,.scene_epoch=nativeGObjPool.lifetime};
}
static void reflector_remove(void* data){((Fighter*)data)->gobj=NULL;}
static HSD_GObj* reflector(void)
{
    HSD_GObj* object=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(object);
    memset(&reflected_actor,0,sizeof(reflected_actor));reflected_actor.kind=Ft_Kind_Ness;reflected_actor.gobj=object;reflected_actor.x8_spawnNum=888;
    reflected_actor.cur_pos=(Vec3){31,32,33};GObj_InitUserData(object,4,reflector_remove,&reflected_actor);return object;
}
static void normal(void)
{
    for(unsigned center=0;center<2;++center)for(unsigned initial=0;initial<2;++initial){caller();reset_records();expected_creator=damage_owner=spawn_caller;check_relations=true;Item_GObj* output=(void*)1;
        CHECK(at(&context,0,center,initial,&output)==NativeScriptVM_Ok&&output!=(void*)1&&output);
        spawn_fields(center,initial,spawn_caller);CHECK(observed_spawned==1&&observed_created==1);
        Item* data=output->user_data;CHECK(creation_value(output)==7.125f);CHECK(data->native_script_owner==programs[0].owner);
        CHECK(data->native_creator==spawn_caller&&data->native_creator_generation==777&&data->native_creator_definition==NULL);remove_item(output);++cases;}
    caller();NativeScriptRuntimeContext item_context;child_context(&item_context);HSD_GObj* foreign=reflector();
    ((Item*)parent_item->user_data)->owner=foreign;damage_owner=foreign;
    for(unsigned center=0;center<2;++center){reset_records();Item_GObj* child=(void*)1;
        CHECK(at(&item_context,1,center,true,&child)==NativeScriptVM_Ok&&child);spawn_fields(center,true,parent_item);
        Item* data=child->user_data;CHECK(data->native_script_owner==programs[1].owner&&data->native_creator==spawn_caller&&data->owner==foreign);
        CHECK(data->native_creator_kind==Ft_Kind_Mario&&data->native_article_author_kind==Ft_Kind_Mario);remove_item(child);++cases;}
    ((Item*)parent_item->user_data)->owner=NULL;damage_owner=NULL;reset_records();Item_GObj* child=(void*)1;
    CHECK(at(&item_context,1,false,false,&child)==NativeScriptVM_Ok&&child);spawn_fields(false,false,parent_item);remove_item(child);++cases;
    reset_records();child=(void*)1;CHECK(at(&item_context,1,true,false,&child)==NativeScriptVM_Error&&child==(void*)1&&!spawn_order_count);++cases;
    CHECK(nativeScriptItem_ClearAttachmentOwner(parent_item));reset_records();attachment_cleared=true;child=(void*)1;
    CHECK(at(&item_context,1,false,false,&child)==NativeScriptVM_Ok&&child);
    CHECK(nativeScriptItem_Creator(child)==spawn_caller&&!nativeScriptItem_AttachmentOwner(child));remove_item(child);++cases;
    // A genuinely creator-less actor is initialized that way at attachment;
    // clearing legacy provenance on an already live actor cannot rewrite its
    // immutable saved original creator relation.
    nativeScriptItem_Detach(parent_item);nativeItem_CaptureCreator(parent_item,NULL);
    CHECK(nativeScriptItem_Attach(parent_item,&programs[0]));expected_creator=NULL;reset_records();child=(void*)1;
    CHECK(at(&item_context,1,false,false,&child)==NativeScriptVM_Ok&&child&&((Item*)child->user_data)->native_creator==NULL);remove_item(child);++cases;
    remove_item(parent_item);parent_item=NULL;HSD_GObjFree(foreign);
}
static void failure_and_retirement(void)
{
    caller();reset_records();expected_creator=damage_owner=spawn_caller;check_relations=true;Item_GObj* output=(void*)1;positional_oom=true;
    CHECK(at(&context,0,false,true,&output)==NativeScriptVM_Ok&&!output&&!observed_spawned&&!observed_created);++cases;positional_oom=false;
    // Actual selector failure cleans the unpublished instance and is native
    // constructor failure (-1), not an invented successfully attached actor.
    reset_records();scheduler_fail_selector=true;output=(void*)1;unsigned freed=scheduler_frees;
    CHECK(at(&context,0,false,true,&output)==NativeScriptVM_Ok&&!output&&scheduler_frees==freed+1&&!observed_created);scheduler_fail_selector=false;++cases;
    for(unsigned stage=StageSpawned;stage<=StageCreated;stage+=StageCreated-StageSpawned){caller();reset_records();expected_creator=damage_owner=spawn_caller;mode=RetireCaller;trigger=stage;output=(void*)1;
        CHECK(at(&context,0,false,true,&output)==NativeScriptVM_SourceRetired&&output==(void*)1);
        HSD_GObj* survivor=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];CHECK(survivor&&nativeScriptItem_IsAttached(survivor));remove_item(survivor);++cases;}
    caller();reset_records();expected_creator=damage_owner=spawn_caller;mode=RemoveCreated;trigger=StageCreated;output=(void*)1;
    CHECK(at(&context,0,false,true,&output)==NativeScriptVM_SourceRetired&&output==(void*)1&&!HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM]);++cases;
    caller();reset_records();expected_creator=damage_owner=spawn_caller;mode=FaultCreated;trigger=StageCreated;output=(void*)1;
    CHECK(at(&context,0,false,true,&output)==NativeScriptVM_Error&&output==(void*)1);remove_item(HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM]);++cases;
    for(unsigned m=RetireOriginal;m<=RemoveParent;++m){if(m==RemoveCreated||m==FaultCreated)continue;
        caller();NativeScriptRuntimeContext c;child_context(&c);HSD_GObj* foreign=reflector();((Item*)parent_item->user_data)->owner=foreign;damage_owner=foreign;
        reset_records();mode=m;trigger=StageCreated;output=(void*)1;
        CHECK(at(&c,1,false,true,&output)==NativeScriptVM_SourceRetired&&output==(void*)1);
        HSD_GObj* survivor=find_item(programs[1].owner);CHECK(survivor&&nativeScriptItem_IsAttached(survivor));remove_item(survivor);if(parent_item)remove_item(parent_item);parent_item=NULL;HSD_GObjFree(foreign);++cases;}
    caller();NativeScriptRuntimeContext c;child_context(&c);reset_records();++actor.x8_spawnNum;output=(void*)1;
    CHECK(at(&c,1,false,true,&output)==NativeScriptVM_SourceRetired&&output==(void*)1&&!spawn_order_count);actor.x8_spawnNum=777;remove_item(parent_item);parent_item=NULL;++cases;
    caller();reset_records();expected_creator=damage_owner=spawn_caller;Vec3 good={1,2,3},bad={NAN,2,3};output=(void*)1;
    CHECK(nativeScriptItemSpawn_At(&context,programs[0].owner,&bad,&good,-1,false,0,0,true,true,&output,&diagnostic)==NativeScriptVM_Error&&!spawn_order_count&&output==(void*)1);++cases;
    CHECK(nativeScriptItemSpawn_At(&context,UINT32_MAX,&good,&good,-1,false,0,0,true,true,&output,&diagnostic)==NativeScriptVM_Error&&!spawn_order_count&&output==(void*)1);++cases;
    static NativeScriptCompileBank foreign_bank;const NativeScriptCompileBank* previous=programs[1].bank;programs[1].bank=&foreign_bank;
    CHECK(at(&context,1,false,true,&output)==NativeScriptVM_Error&&!spawn_order_count&&output==(void*)1);programs[1].bank=previous;++cases;
    caller();child_context(&c);HSD_GObj* foreign=reflector();((Item*)parent_item->user_data)->owner=foreign;HSD_GObjFree(foreign);reset_records();output=(void*)1;
    CHECK(at(&c,1,false,true,&output)==NativeScriptVM_SourceRetired&&!spawn_order_count&&output==(void*)1);remove_item(parent_item);parent_item=NULL;++cases;
    caller();nativeScriptFighter_Detach(spawn_caller);actor.x8_spawnNum=0;CHECK(nativeScriptFighter_Attach(spawn_caller));context.generation=context.actor_id=0;
    reset_records();expected_creator=damage_owner=spawn_caller;output=(void*)1;
    CHECK(at(&context,1,false,true,&output)==NativeScriptVM_Ok&&output&&((Item*)output->user_data)->native_creator_generation==0);
    remove_item(output);++cases;
}
static void unowned_fields(Item_GObj* object)
{
    const Vec3 position={1.25f,-0.0f,3.75f},velocity={-2.5f,0.03125f,-0.0f};
    CHECK(!memcmp(&positional_spawn.pos,&position,sizeof(position)));
    CHECK(!memcmp(&positional_spawn.prev_pos,&position,sizeof(position)));
    CHECK(!memcmp(&positional_spawn.vel,&velocity,sizeof(velocity)));
    CHECK(!positional_spawn.x0_parent_gobj&&!positional_spawn.x4_parent_gobj2);
    CHECK(positional_spawn.facing_dir==-1&&positional_spawn.x44_flag.b0);
    CHECK(positional_spawn.x40==0&&positional_spawn.x3C_damage==0);
    CHECK(observed_spawned==1&&observed_created==1);
    Item* data=object->user_data;
    CHECK(!data->owner&&!data->native_creator&&!nativeScriptItem_Creator(object));
    CHECK(!nativeScriptItem_AttachmentOwner(object));
    CHECK(data->native_article_author_kind==Ft_Kind_Mario&&data->native_script_owner==programs[0].owner);
    CHECK(nativeScriptItem_Spec(object)==&programs[0]&&creation_value(object)==0);
    NativeScriptReceiverView view;CHECK(nativeScriptItem_View(object,(uint32_t)data->x1C,&view));
    CHECK(view.program==&bank.declarations&&view.storage&&view.storage->live);
}
static int32_t saved_unowned_handle(void)
{
    NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(spawn_caller,777,&view));
    for(unsigned i=0;i<bank.declarations.value_count;++i)if(nativeScriptSource_SpanEquals(&bank.tree,values[i].name,"unowned_handle")){
        NativeScriptValue value;CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&value,&diagnostic));
        CHECK(value.type==NativeScriptType_I32);return (int32_t)value.as.i64;
    }
    CHECK(false);return 0;
}
static NativeScriptVMStatus run_unowned(void)
{
    for(unsigned i=0;i<bank.declarations.function_count;++i)if(nativeScriptSource_SpanEquals(&bank.tree,functions[i].name,"SpawnUnowned")){
        NativeScriptValue result;
        return nativeScriptFighter_Helper(spawn_caller,777,i,NULL,0,&result,&diagnostic);
    }
    CHECK(false);return NativeScriptVM_Error;
}
static void unowned(void)
{
    // Full public source frontend -> bytecode -> binding -> real constructor,
    // with NULL ownership observed inside both interpreted lifecycle hooks.
    caller();reset_records();expected_creator=damage_owner=NULL;check_relations=true;
    CHECK(run_unowned()==NativeScriptVM_Ok);
    HSD_GObj* object=find_item(programs[0].owner);CHECK(object);unowned_fields(object);
    CHECK(saved_unowned_handle()==((Item*)object->user_data)->x1C);
    nativeScriptFighter_Detach(spawn_caller);
    CHECK(nativeScriptItem_IsAttached(object)&&nativeScriptItem_Status(object)==NativeScriptVM_Ok);
    CHECK(!nativeScriptItem_Creator(object)&&nativeScriptItem_Spec(object)==&programs[0]);
    NativeScriptValue item_result;
    CHECK(nativeScriptItem_RunHook(object,NativeScriptItemHook_Created,NULL,0,&item_result)==NativeScriptVM_Ok);
    CHECK(nativeScriptItem_Status(object)==NativeScriptVM_Ok&&observed_created==2);
    remove_item(object);++cases;

    caller();reset_records();expected_creator=damage_owner=NULL;positional_oom=true;
    CHECK(run_unowned()==NativeScriptVM_Ok);
    CHECK(saved_unowned_handle()==-1&&!observed_spawned&&!observed_created&&!find_item(programs[0].owner));++cases;

    for(unsigned stage=StageSpawned;stage<=StageCreated;stage+=StageCreated-StageSpawned){
        caller();reset_records();expected_creator=damage_owner=NULL;mode=RetireCaller;trigger=stage;
        CHECK(run_unowned()==NativeScriptVM_SourceRetired);
        object=find_item(programs[0].owner);CHECK(object&&nativeScriptItem_IsAttached(object));remove_item(object);++cases;
    }
    for(unsigned operation=RemoveCreated;operation<=FaultCreated;++operation){
        caller();reset_records();expected_creator=damage_owner=NULL;mode=operation;trigger=StageCreated;
        CHECK(run_unowned()!=NativeScriptVM_Ok);
        object=find_item(programs[0].owner);if(operation==RemoveCreated)CHECK(!object);else{CHECK(object);remove_item(object);}++cases;
    }
    // The shared native constructor also accepts Item authors. No stale
    // gameplay relationship is inspected when ownership was explicitly absent.
    caller();NativeScriptRuntimeContext item_context;child_context(&item_context);
    reset_records();expected_creator=damage_owner=NULL;
    const Vec3 position={1.25f,-0.0f,3.75f},velocity={-2.5f,0.03125f,-0.0f};
    Item_GObj* output=(void*)1;
    CHECK(nativeScriptItemSpawn_At(&item_context,programs[0].owner,&position,&velocity,-1,false,0,0,true,false,&output,&diagnostic)==NativeScriptVM_Ok&&output);
    unowned_fields(output);remove_item(parent_item);parent_item=NULL;
    CHECK(nativeScriptItem_Status(output)==NativeScriptVM_Ok&&!nativeScriptItem_Creator(output));remove_item(output);++cases;

    caller();reset_records();output=(void*)1;
    CHECK(nativeScriptItemSpawn_At(&context,programs[0].owner,&position,&velocity,-1,true,0,0,true,false,&output,&diagnostic)==NativeScriptVM_Error);
    CHECK(output==(void*)1&&!spawn_order_count);++cases;
}
int main(void)
{
    prepare();HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);scheduler_reset();
    normal();failure_and_retirement();unowned();caller();expected_creator=damage_owner=spawn_caller;check_relations=true;
    for(unsigned cycle=0;cycle<1024;++cycle){reset_records();Item_GObj* out=(void*)1;CHECK(at(&context,1,false,true,&out)==NativeScriptVM_Ok&&out);NativeMatchPoolReference ref;CHECK(nativeMatchPool_Reference(&nativeGObjPool,out,&ref));remove_item(out);CHECK(!nativeMatchPool_Resolve(ref));}++cases;
    NativeScriptValue original={.type=NativeScriptType_F64,.as.f64=0.1},converted;CHECK(nativeScriptValue_Convert(&original,NativeScriptType_F32,&converted,&diagnostic));
    CHECK(converted.as.f32==(float)original.as.f64);reset_records();Vec3 position={converted.as.f32,-0.0f,3},velocity={0,0,0};Item_GObj* out=(void*)1;
    CHECK(nativeScriptItemSpawn_At(&context,programs[1].owner,&position,&velocity,converted.as.f32,false,0,converted.as.f32,true,true,&out,&diagnostic)==NativeScriptVM_Ok&&out);
    CHECK(!memcmp(&positional_spawn.prev_pos,&position,sizeof(position))&&creation_value(out)==converted.as.f32);remove_item(out);++cases;
    printf("{\"status\":\"PASS positional Article construction\",\"checks\":%u,\"cases\":%u,\"native_frees\":%u,\"reuse_cycles\":1024}\n",checks,cases,scheduler_frees);return 0;
}
