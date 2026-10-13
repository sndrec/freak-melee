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
#include <melee/it/item.h>
#include <melee/it/itdraw.h>
#include <melee/it/it_3F2F.h>
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
static NativeScriptCompileBank bank;static NativeScriptFighterProgram root;static NativeScriptItemProgram programs[2];
static NativeScriptItemState states[2][4];static NativeScriptItemProjectile projectile;static NativeItemDefinition recipe;
static Article assets;static ItemAttr attributes;static HSD_Joint model_joint;static ItemModelDesc model={.x0_joint=&model_joint};static ItemStateDesc descriptors[4];static ItemStateArray animation_array={descriptors,4};
static Fighter actor;static FighterBone bones[140];static HSD_JObj bone_joint;static ftData data;static FighterPartsTable part_table={.parts_num=140};static FighterPartsTable* part_tables[Ft_Kind_RuntimeCapacity];
FighterPartsTable** ftPartsTable=part_tables;
static NativeScriptRuntimeContext context;
static NativeScriptItemProgram replacement;
extern bool render_swapped;
const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind kind){return kind==Ft_Kind_Mario?&root:NULL;}
const NativeScriptItemProgram* nativeScriptItem_Program(FighterKind author,uint32_t owner){for(unsigned i=0;i<2;++i)if(author==Ft_Kind_Mario&&programs[i].owner==owner)return render_swapped?&replacement:&programs[i];return NULL;}
static NativeScriptItemAsset owned_assets[2];
static NativeScriptItemMetadataContext metadata_context;
static NativeScriptItemPrograms entities;
static float creation_value(Item_GObj* object){float value;CHECK(nativeScriptItem_GetCreationValue(object,&value));return value;}
static void prepare(void)
{
 bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=N*2,.arguments=args,.argument_capacity=N*2},
  .declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=128,
  .instructions=instructions,.instruction_capacity=8192,.function_code=code,.function_code_capacity=F,.execution_functions=executions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,
  .storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=16},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* text="fighter:\n    behavior independent\n"
  "article Shot states 2:\n    render standard\n    state int count\n    on spawned:\n            item.state.count = 1\n            position.x = 101\n"
  "    on created:\n            item.state.count = item.state.count + 3\n            position.x = 103\n"
  "    on launched:\n            item.state.count = item.state.count + 7\n            position.x = 107\n"
  "    on destroyed:\n            item.state.count = item.state.count + 100\n    on picked_up:\n            item.state.count = item.state.count + 5\n            position.x = 109\n"
  "    state 0 anim -1:\n    state 1 anim -1:\n"
  "item Snack damage 4 angle 45 lifetime 90:\n    state int count\n    on tick:\n        item.state.count = item.state.count + 1\n";
 assets=(Article){.x0_common_attr=&attributes,.x10_modelDesc=&model,.xC_itemStates=&animation_array};
 owned_assets[0]=(NativeScriptItemAsset){.declaration="Shot",.kind=It_Kind_Mario_Fire,.article=&assets};
 owned_assets[1]=(NativeScriptItemAsset){.declaration="Snack",.kind=It_Kind_NativeHoldable,.article=&assets};
 metadata_context=(NativeScriptItemMetadataContext){.assets=owned_assets,.asset_count=2};
 NativeScriptCompileRequest request={.source=text,.source_bytes=(uint32_t)strlen(text),.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata_context};const NativeScriptCompileBank* accepted=NULL;
 CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));assets=(Article){.x0_common_attr=&attributes,.x10_modelDesc=&model,.xC_itemStates=&animation_array};
 CHECK(nativeScriptItemPrograms_Prepare(&bank,Ft_Kind_Mario,44,&metadata_context,&entities,&diagnostic));
 root=(NativeScriptFighterProgram){.bank=&bank,.identity=44};
 for(unsigned h=0;h<NativeScriptFighter_HookCount;++h)root.hooks[h]=UINT32_MAX;
 for(unsigned h=0;h<NativeScriptFighterCallback_Count;++h)root.callbacks[h]=UINT32_MAX;
 CHECK(entities.count==2);
 programs[0]=entities.programs[0];programs[1]=entities.programs[1];
 CHECK(programs[0].drawing==NativeScriptItemDrawing_Standard);
 CHECK(programs[1].drawing==NativeScriptItemDrawing_Billboard);
 projectile=(NativeScriptItemProjectile){.appearance_state=0,.item_state=0,.speed={2,UINT32_MAX},.angle={0,UINT32_MAX},.lifetime={60,UINT32_MAX},.initial_velocity=true,.initial_update=true};
 programs[0].projectile=&projectile;scheduler_spec=&programs[0];spawn_food_asset=&programs[0];
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
extern unsigned render_observations,render_wrong_order,render_display_calls;
extern const HSD_Joint* render_descriptor;
extern bool render_replace_program;
void render_bad_draw(HSD_GObj*,int);
int main(void)
{
 prepare();HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);scheduler_reset();
 // Deliberately incompatible physical-kind renderer: source selection must
 // bypass it while retaining actual link registration order.
 extern struct sdata_ItemGXLink it_803F2F28[118];
 it_803F2F28[It_Kind_Mario_Fire-It_Kind_Kuriboh].x0_renderFunc=render_bad_draw;
 SpawnItem spawn={.kind=It_Kind_Mario_Fire,.hold_kind=0,.facing_dir=1};
 Item_GObj* object=scheduler_construct(&spawn);CHECK(object);
 CHECK(object->render_cb==it_8026EECC&&object->gx_link==6&&object->render_priority==0);
 CHECK(render_observations==1&&!render_wrong_order&&render_descriptor==model.x0_joint);
 CHECK(nativeScriptItem_Status(object)==NativeScriptVM_Ok);
 CHECK(((Item*)object->user_data)->pos.x==101); // actual compiled Spawned body
 object->render_cb(object,0);CHECK(render_display_calls==1);
 ((Item*)object->user_data)->xDAA_flag.b7=false;
 object->render_cb(object,0);CHECK(render_display_calls==1); // actual draw visibility gate
 remove_item(object);++cases;
 unsigned before=render_observations;
 programs[0].drawing=(NativeScriptItemDrawing)99;
 CHECK(!scheduler_construct(&spawn)&&render_observations==before);
 programs[0].drawing=NativeScriptItemDrawing_Standard;
 replacement=programs[0];render_replace_program=true;
 CHECK(!scheduler_construct(&spawn)&&render_observations==before); // replacement before model stage
 render_replace_program=render_swapped=false;++cases;
 scheduler_remove=true;
 CHECK(!scheduler_construct(&spawn));CHECK(!nativeScriptItem_ReferencesProgram(&bank));++cases;
 CHECK(!nativeScriptItem_ReferencesProgram(&bank));
 CHECK(HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM]==NULL);
 printf("{\"checks\":%u,\"cases\":%u,\"native_draw_calls\":%u}\n",checks,cases,render_display_calls);
 return 0;
}
