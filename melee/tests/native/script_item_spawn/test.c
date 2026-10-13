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
static NativeScriptItemState states[2][4];static NativeScriptItemProjectile projectile;static NativeItemDefinition recipe;
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
 const char* text="fighter:\n    behavior independent\nstate int caller_count\naction Caller:\n    on load:\n        state.caller_count = 1\n"
  "article Shot states 2:\n    state int count\n    on spawned:\n            item.state.count = 1\n            position.x = 101\n"
  "    on created:\n            item.state.count = item.state.count + 3\n            position.x = 103\n"
  "    on launched:\n            item.state.count = item.state.count + 7\n            position.x = 107\n"
  "    on destroyed:\n            item.state.count = item.state.count + 100\n    on picked_up:\n            item.state.count = item.state.count + 5\n            position.x = 109\n"
  "    state 0 anim -1:\n    state 1 anim -1:\n"
  "item Snack damage 4 angle 45 lifetime 90:\n    state int count\n    on tick:\n        item.state.count = item.state.count + 1\n";
 NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};
 NativeScriptCompileRequest request={.motion_layout=&layout,.source=text,.source_bytes=(uint32_t)strlen(text),.metadata=metadata};const NativeScriptCompileBank* accepted=NULL;
 CHECK(nativeScriptCompile_Build(&fixture_bank,&request,&accepted,&diagnostic));assets=(Article){.x0_common_attr=&attributes,.x10_modelDesc=&model,.xC_itemStates=&animation_array};
 package.address=&package;package.ready=package.assets.ready=true;package.target=Ft_Kind_CustomFirst;package.assets.facts.moves=fixture_bank.motion_layout;package.assets.facts.joints=140;part_table.parts_num=140;package.program.identity=44;
 gFtDataList[Ft_Kind_CustomFirst]=&data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=package.assets.motions;
 root=(NativeScriptFighterProgram){.bank=&fixture_bank,.identity=44};for(unsigned h=0;h<NativeScriptFighter_HookCount;++h)root.hooks[h]=UINT32_MAX;for(unsigned h=0;h<NativeScriptFighterCallback_Count;++h)root.callbacks[h]=UINT32_MAX;
 unsigned entity=0;for(unsigned n=0;n<fixture_bank.tree.count;++n)if(nodes[n].kind==NativeScriptSource_Article||nodes[n].kind==NativeScriptSource_Item){
  programs[entity]=(NativeScriptItemProgram){.bank=&fixture_bank,.identity=100+entity,.author_kind=Ft_Kind_CustomFirst,.kind=entity?It_Kind_NativeHoldable:It_Kind_Mario_Fire,.recipe_index=entity?0:UINT32_MAX,.owner=n,.article=&assets,.states=states[entity],.state_count=entity?4:2,.recipe=entity?&recipe:NULL};
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
 projectile=(NativeScriptItemProjectile){.appearance_state=0,.item_state=0,.speed={2,UINT32_MAX},.angle={0,UINT32_MAX},.lifetime={60,UINT32_MAX},.initial_velocity=true,.initial_update=true};
 programs[0].projectile=&projectile;scheduler_spec=&programs[0];spawn_food_asset=&programs[0];
}
static void fighter_remove(void* data_ptr){Fighter* fp=data_ptr;nativeScriptFighter_Detach(fp->gobj);}
static void caller(void)
{
 if(spawn_caller){HSD_GObjFree(spawn_caller);spawn_caller=NULL;}
 memset(&actor,0,sizeof(actor));spawn_caller=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(spawn_caller);
 actor.gobj=spawn_caller;actor.kind=actor.behavior_kind=Ft_Kind_CustomFirst;actor.x8_spawnNum=777;actor.facing_dir=-1;actor.cur_pos=(Vec3){4,5,6};actor.parts=bones;actor.ft_data=&data;
 bone_joint.translate=(Vec3){10,11,12};bones[0].joint=&bone_joint;part_tables[Ft_Kind_CustomFirst]=&part_table;
 static struct ftData_x8 part_info;data.x8=&part_info;data.x8->x10=0;
 GObj_InitUserData(spawn_caller,4,fighter_remove,&actor);CHECK(nativeScriptFighter_Attach(spawn_caller));
 context=(NativeScriptRuntimeContext){.source=spawn_caller,.source_class=NativeCallbackSource_Fighter,.generation=777,.actor_id=777,.scene_epoch=nativeGObjPool.lifetime};
 spawn_order_count=spawn_retire_stage=0;
}
static void remove_item(Item_GObj* item){if(item&&nativeScriptItem_IsAttached(item))Item_8026A8EC(item);else if(item)HSD_GObjFree(item);}
int main(void)
{
 prepare();HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);scheduler_reset();caller();
 Vec3 point={1,2,3};Item_GObj* output=(void*)1;NativeScriptItemSpawnData create={.position=point,.previous=point,.velocity={2,3,4},.facing=-1,.creation_value=9,.state=1,.initial_collision=true,.initial_update=true,.damage_owned=true};
 CHECK(nativeScriptItemSpawn_Create(&context,programs[0].owner,&create,&output,&diagnostic)==NativeScriptVM_Ok&&output!=(void*)1);
 CHECK(((Item*)output->user_data)->x40_vel.x==2&&((Item*)output->user_data)->msid==1&&creation_value(output)==9);remove_item(output);++cases;
 output=(void*)1;spawn_order_count=0;
 CHECK(nativeScriptItemSpawn_Projectile(&context,programs[0].owner,&point,-1,3,NULL,&output,&diagnostic)==NativeScriptVM_Ok);
 Item* ip=output->user_data;CHECK(ip->x40_vel.x==-2&&ip->x40_vel.y==0&&ip->xD44_lifeTimer==60&&ip->msid==0&&ip->xDD4_itemVar.native_article.launch_speed==2);
 const unsigned expected[]={StageConstruct,StageSpawned,StageState,StageAppearance,StageCreated,StageDebug,StageUpdate,StageLaunched};
 CHECK(spawn_order_count==sizeof(expected)/sizeof(expected[0])&&!memcmp(spawn_order,expected,sizeof(expected)));remove_item(output);++cases;
 NativeScriptItemLaunchData launch={.angle=0,.speed=3,.state=1,.collision_group=17,.preserve_facing=false};
 output=(void*)1;CHECK(nativeScriptItemSpawn_Projectile(&context,programs[0].owner,&point,-1,0,&launch,&output,&diagnostic)==NativeScriptVM_Ok);
 CHECK(((Item*)output->user_data)->x40_vel.x==3&&((Item*)output->user_data)->xAC4_ignoreItemID==17);remove_item(output);++cases;
 output=(void*)1;CHECK(nativeScriptItemSpawn_Attached(&context,programs[0].owner,0,&output,&diagnostic)==NativeScriptVM_Ok&&actor.x1984_heldItemSpec==output);remove_item(output);actor.x1984_heldItemSpec=NULL;++cases;
 output=(void*)1;CHECK(nativeScriptItemSpawn_HeldAtCenter(&context,programs[0].owner,&point,0,-1,&output,&diagnostic)==NativeScriptVM_Ok&&actor.item_gobj==output);remove_item(output);actor.item_gobj=NULL;++cases;
 output=(void*)1;spawn_order_count=0;CHECK(nativeScriptItemSpawn_AttachedPresentation(&context,programs[0].owner,0,2,0,0,1,77,&output,&diagnostic)==NativeScriptVM_Ok);
 ip=output->user_data;CHECK(ip->msid==1&&ip->xD44_lifeTimer==77&&ip->on_accessory&&ip->owner==spawn_caller&&nativeScriptItem_Creator(output)==spawn_caller);
 const unsigned presentation[]={StageConstruct,StageSpawned,StageState,StageAppearance,StageState,StageCreated,StageDebug,StageUpdate};CHECK(spawn_order_count==8&&!memcmp(spawn_order,presentation,sizeof(presentation)));
 ip->on_accessory(output);CHECK(ip->pos.x==4&&ip->pos.y==5&&ip->pos.z==6);remove_item(output);++cases;
 output=(void*)1;spawn_order_count=0;CHECK(nativeScriptItemSpawn_PullHoldable(&context,programs[1].owner,&output,&diagnostic)==NativeScriptVM_Ok&&actor.item_gobj==output);
 ip=output->user_data;CHECK(ip->kind==It_Kind_NativeHoldable&&ip->msid==0&&ip->native_script_owner==programs[1].owner&&ip->xDD4_itemVar.native_holdable.definition_index==0);
 const unsigned pull[]={StageConstruct,StageState,StageAppearance,StageInventory};CHECK(spawn_order_count==4&&!memcmp(spawn_order,pull,sizeof(pull)));
 NativeScriptRuntimeContext item_context={.source=output,.source_class=NativeCallbackSource_Item,.generation=(uint32_t)ip->x1C,.actor_id=(uint32_t)ip->x1C,.scene_epoch=nativeGObjPool.lifetime};
 ip->pos=(Vec3){22,23,24};ip->x40_vel=(Vec3){10,11,12};Item_GObj* food=(void*)1;
 CHECK(nativeScriptItemSpawn_Food(&item_context,&food,&diagnostic)==NativeScriptVM_Ok&&food!=(void*)1);Item* meal=food->user_data;
 CHECK(meal->kind==It_Kind_Foods&&meal->pos.x==22&&meal->pos.y==23&&meal->x40_vel.x==3.5f&&meal->x40_vel.y==1.8f&&!nativeScriptItem_IsAttached(food));remove_item(food);remove_item(output);actor.item_gobj=NULL;++cases;
 for(unsigned failure=StageSpawned;failure<=StageLaunched;++failure){if(failure==StageInventory||failure==StagePickup)continue;caller();spawn_retire_stage=failure;output=(void*)1;
  CHECK(nativeScriptItemSpawn_Projectile(&context,programs[0].owner,&point,-1,0,NULL,&output,&diagnostic)==NativeScriptVM_SourceRetired&&output==(void*)1&&spawn_stage==failure);
  HSD_GObj* survivor=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];CHECK(survivor&&nativeScriptItem_IsAttached(survivor));remove_item(survivor);++cases;
 }
 caller();spawn_retire_stage=StagePickup;output=(void*)1;CHECK(nativeScriptItemSpawn_HeldAtCenter(&context,programs[0].owner,&point,0,-1,&output,&diagnostic)==NativeScriptVM_SourceRetired&&output==(void*)1&&actor.item_gobj==NULL);
 remove_item(HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM]);++cases;
 caller();spawn_retire_stage=StageInventory;output=(void*)1;CHECK(nativeScriptItemSpawn_PullHoldable(&context,programs[1].owner,&output,&diagnostic)==NativeScriptVM_SourceRetired&&output==(void*)1&&actor.item_gobj);
 remove_item(actor.item_gobj);actor.item_gobj=NULL;++cases;
 caller();spawn_fail_stage=StageDebug;output=(void*)1;CHECK(nativeScriptItemSpawn_Projectile(&context,programs[0].owner,&point,-1,0,NULL,&output,&diagnostic)==NativeScriptVM_Error&&output==(void*)1&&spawn_stage==StageDebug);
 remove_item(HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM]);++cases;
 caller();scheduler_fail_selector=true;output=(void*)1;unsigned before=scheduler_frees;
 CHECK(nativeScriptItemSpawn_Projectile(&context,programs[0].owner,&point,-1,0,NULL,&output,&diagnostic)==NativeScriptVM_Error&&output==(void*)1&&scheduler_frees==before+1);scheduler_fail_selector=false;++cases;
 caller();scheduler_fail_write=true;output=(void*)1;before=scheduler_frees;
 CHECK(nativeScriptItemSpawn_Projectile(&context,programs[0].owner,&point,-1,0,NULL,&output,&diagnostic)==NativeScriptVM_Error&&output==(void*)1&&scheduler_frees==before+1);scheduler_fail_write=false;++cases;
 caller();output=(void*)1;create.state=99;CHECK(nativeScriptItemSpawn_Create(&context,programs[0].owner,&create,&output,&diagnostic)==NativeScriptVM_Error&&output==(void*)1);++cases;
 CHECK(nativeScriptItemSpawn_Attached(&context,programs[0].owner,140,&output,&diagnostic)==NativeScriptVM_Error&&output==(void*)1);++cases;
 CHECK(nativeScriptItemSpawn_Food(&context,&output,&diagnostic)==NativeScriptVM_Error&&output==(void*)1);++cases;
 CHECK(nativeScriptItemSpawn_PullHoldable(&context,programs[0].owner,&output,&diagnostic)==NativeScriptVM_Error&&output==(void*)1);++cases;
 for(unsigned cycle=0;cycle<1024;++cycle){spawn_order_count=0;output=(void*)1;CHECK(nativeScriptItemSpawn_Projectile(&context,programs[0].owner,&point,-1,0,NULL,&output,&diagnostic)==NativeScriptVM_Ok);NativeMatchPoolReference ref;CHECK(nativeMatchPool_Reference(&nativeGObjPool,output,&ref));remove_item(output);CHECK(!nativeMatchPool_Resolve(ref));}++cases;
 printf("{\"status\":\"PASS connected source factories\",\"cases\":%u,\"checks\":%u,\"native_frees\":%u}\n",cases,checks,scheduler_frees);return 0;
}
