#include <melee/gm/native_match_arena.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/fighter.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/ftdata.h>
#include <sysdolphin/baselib/class.h>
#include <melee/ef/native_script_effect_internal.h>
#include <melee/ef/native_script_effect_snapshot.h>
#include <melee/ef/native_model_effect.h>
#include <melee/ef/eflib.h>
#include <melee/ef/efasync.h>
#include <melee/gm/native_rollback_state.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/jobj.h>
#include "boundary.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#pragma clang section data = ""
unsigned spawn_stage,spawn_retire_stage,spawn_fail_stage,spawn_order[128],spawn_order_count;
HSD_GObj* spawn_caller;const NativeScriptItemProgram* spawn_food_asset;
unsigned effect_models,effect_frees,effect_animations,effect_retire_on_create,effect_retire_on_anim,effect_fail_model_at;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];
MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
HSD_JObjInfo hsdJObj;
static bool rig_available;
static NativeSourcePackage rig_package;
static FighterPartsTable* rig_tables[Ft_Kind_RuntimeCapacity];
static FighterBone rig_parts[2];
static HSD_JObj rig_joint;
bool hsdObjIsDescendantOf(HSD_Obj* object,HSD_ClassInfo* parent)
{return parent==HSD_CLASS_INFO(&hsdJObj)&&HSD_CLASS_METHOD(object)==parent;}
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind){return rig_available&&kind==Ft_Kind_Mario?&rig_package:NULL;}
extern EF_DAT_Entry efAsync_DatEntries[51];
extern unsigned effect_queue_frees;
static void copy_effect_history(void);
static unsigned checks,cases;static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"effect %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
enum{N=4096,F=64,V=128};
static char source_bytes[65536];static NativeScriptSourceNode syntax[N];static NativeScriptIRNode ir[N*2];static uint32_t arguments[N*2];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[128];
static NativeScriptInstruction instructions[8192];static NativeScriptFunctionCode code[F];static NativeScriptExecutionFunction executions[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[16];static NativeScriptParameterSlot slots[V];static NativeScriptCompileBank bank;
static NativeScriptFighterProgram prepared;static Fighter fighter;static HSD_JObj creator_joint;static HSD_Joint model_descriptor;static EF_EffectDesc descriptors[7];
FighterPartsTable** ftPartsTable;
static uint8_t packed[8*1024*1024],second[8*1024*1024],owner_before[sizeof(nativeScriptEffectActors)];
static uint16_t index_before[NATIVE_GOBJ_CAPACITY];
static uint8_t oracle[24*1024*1024];
static uint8_t scene_oracle[NATIVE_MATCH_SCENE_BYTES],payload_oracle[2*1024*1024];
extern void effect_peer_checkpoint(bool restore);
extern void effect_reference_tornado(EF_Effect* effect);
static NativeScriptEffectPrograms candidate,before_candidate;
static NativeScriptSourceNode rejected_nodes[N];static NativeScriptExecutionFunction rejected_functions[F];
static bool metadata(void* ignored,const NativeScriptSourceTree* tree,uint32_t n,NativeScriptDiagnostic* d)
{(void)ignored;return nativeScriptEffectMetadata_Owns(tree,n)?nativeScriptEffectMetadata_Validate(tree,n,d):nativeScriptCompile_MinimalMetadata(NULL,tree,n,d);}
static const char* source="fighter:\n    behavior independent\nstate bool tornado_collision\naction Tornado:\n    on load:\n        state.tornado_collision = true\n    on tick:\n        effects.spawn_root(TornadoSwirl)\n"
 "func SpawnAtPart(int raw):\n    effects.spawn_on_part(TornadoSwirl, raw)\n"
 "func SpawnDeferredAtPart(int raw):\n    effects.spawn_async_position_on_part(Point, raw)\n"
 "model_effect TornadoSwirl 0x47c:\n    state int updates\n    state bool marked\n    func Mark():\n        state.updates = state.updates + 1\n    on update:\n        Mark()\n        state.marked = true\n        effect.joint(\"1\").visible = creator.motion == 349\n        if creator.command3 and creator.state.tornado_collision:\n            effect.joint(\"\").rotation.z = -atan2_float(creator.floor.normal.x, creator.floor.normal.y)\n        else:\n            effect.joint(\"\").rotation.z = 0\n"
 "model_effect Cancelled 0x47c:\n    state bool reached\n    on update:\n        effect.cancel()\n        state.reached = true\n"
 "model_effect Bare 0x47c:\n    pass\nmodel_effect Pair 0x4da:\n    on update:\n        effect.joint(\"\", 1).visible = true\n"
 "model_effect Axes 0x47c:\n    on update:\n        effect.joint(\"\").rotation.x = 0.25\n        effect.joint(\"\").rotation.y = -0.5\n        effect.joint(\"\").rotation.z = effect.joint(\"\").rotation.x + effect.joint(\"\").rotation.y\n"
 "model_effect Point 0x4cc:\n    on update:\n        effect.joint(\"\").rotation.z = 0.75\n";
static void prepare(void)
{
 bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=syntax,.capacity=N},.expressions={.nodes=ir,.node_capacity=N*2,.arguments=arguments,.argument_capacity=N*2},
  .declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=128,.instructions=instructions,.instruction_capacity=8192,
  .function_code=code,.function_code_capacity=F,.execution_functions=executions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=16},.parameter_slots=slots,.parameter_slot_capacity=V};
 NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.metadata=metadata};const NativeScriptCompileBank* accepted=NULL;
 CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_Mario,&bank,NULL,NULL,&prepared,&diagnostic));CHECK(prepared.effects.count==6);
 NativeScriptCompileBank rejected=bank;rejected.tree.nodes=rejected_nodes;memcpy(rejected_nodes,syntax,sizeof(syntax));rejected.execution_functions=rejected_functions;memcpy(rejected_functions,executions,sizeof(executions));
 memset(&candidate,0x5A,sizeof(candidate));before_candidate=candidate;
 uint32_t effect_owner=prepared.effects.programs[0].owner;NativeScriptSourceSpan original_value=rejected_nodes[effect_owner].value;
 rejected_nodes[effect_owner].value=(NativeScriptSourceSpan){.offset=(uint32_t)(strstr(source_bytes,"349")-source_bytes),.length=3};
 CHECK(!nativeScriptEffectPrograms_Prepare(&rejected,Ft_Kind_Mario,99,&candidate,&diagnostic)&&!memcmp(&candidate,&before_candidate,sizeof(candidate)));rejected_nodes[effect_owner].value=original_value;++cases;
 NativeScriptSourceSpan original_name=rejected_nodes[prepared.effects.programs[2].owner].name;rejected_nodes[prepared.effects.programs[2].owner].name=rejected_nodes[effect_owner].name;
 CHECK(!nativeScriptEffectPrograms_Prepare(&rejected,Ft_Kind_Mario,99,&candidate,&diagnostic)&&!memcmp(&candidate,&before_candidate,sizeof(candidate)));rejected_nodes[prepared.effects.programs[2].owner].name=original_name;++cases;
 unsigned update=prepared.effects.programs[0].update;rejected_functions[update].source_class=NativeCallbackSource_Fighter;
 CHECK(!nativeScriptEffectPrograms_Prepare(&rejected,Ft_Kind_Mario,99,&candidate,&diagnostic)&&!memcmp(&candidate,&before_candidate,sizeof(candidate)));++cases;
 CHECK(nativeScriptFighter_CanPublish(Ft_Kind_Mario));nativeScriptFighter_PublishProgram(Ft_Kind_Mario,&prepared);memset(&prepared,0xCC,sizeof(prepared));
 CHECK(nativeScriptFighter_Program(Ft_Kind_Mario)->effects.count==6);descriptors[1].lifetime=0;descriptors[1].model_desc.joint=&model_descriptor;descriptors[2]=descriptors[1];descriptors[6]=descriptors[1];efAsync_DatEntries[1]=(EF_DAT_Entry){.data=descriptors,.effect_count=2};efAsync_DatEntries[12]=(EF_DAT_Entry){.data=descriptors,.effect_count=3};efAsync_DatEntries[8]=(EF_DAT_Entry){.data=descriptors,.effect_count=7};
}
static uint32_t owner(unsigned index){return nativeScriptFighter_Program(Ft_Kind_Mario)->effects.programs[index].owner;}
static void remove_fighter(void* data){nativeScriptFighter_Detach(((Fighter*)data)->gobj);}
static void caller(void)
{
 if(spawn_caller){HSD_GObjFree(spawn_caller);spawn_caller=NULL;}memset(&fighter,0,sizeof(fighter));memset(&creator_joint,0,sizeof(creator_joint));creator_joint.scale=(Vec3){1,1,1};
 spawn_caller=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(spawn_caller);fighter.gobj=spawn_caller;fighter.kind=Ft_Kind_Mario;fighter.x8_spawnNum=777;
 fighter.motion_id=349;fighter.cmd_vars[3]=1;fighter.coll_data.floor.normal=(Vec3){.6f,.8f,0};fighter.cur_pos=(Vec3){4,5,6};spawn_caller->hsd_obj=&creator_joint;
 GObj_InitUserData(spawn_caller,4,remove_fighter,&fighter);CHECK(nativeScriptFighter_Attach(spawn_caller));
}
static HSD_GObj* spawn(unsigned index)
{
 NativeScriptRuntimeContext c={.source=spawn_caller,.source_class=NativeCallbackSource_Fighter,.generation=777,.actor_id=777,.scene_epoch=nativeGObjPool.lifetime};
 CHECK(nativeScriptEffect_Spawn(&c,owner(index),false,0,false,&diagnostic)==NativeScriptVM_Ok);HSD_GObj* result=NULL;
 for(HSD_GObj* live=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_EFFECT_UNK0];live;live=live->next)if(!result||((EF_Effect*)live->user_data)->native_generation>((EF_Effect*)result->user_data)->native_generation)result=live;
 CHECK(result&&nativeScriptEffect_IsAttached(result));return result;
}
static void reject(size_t bytes)
{memcpy(owner_before,nativeScriptEffectActors,sizeof(owner_before));memcpy(index_before,nativeScriptEffectByObject,sizeof(index_before));CHECK(!nativeScriptEffect_Load(second,bytes));CHECK(!memcmp(owner_before,nativeScriptEffectActors,sizeof(owner_before))&&!memcmp(index_before,nativeScriptEffectByObject,sizeof(index_before)));}
static void part_origins(void)
{
 rig_available=true;rig_package.assets.ready=true;rig_package.assets.facts.joints=2;
 rig_package.assets.parts.parts_num=2;rig_tables[Ft_Kind_Mario]=&rig_package.assets.parts;ftPartsTable=rig_tables;
 HSD_CLASS_INFO(&hsdJObj)->head.obj_size=sizeof(HSD_JObj);
 creator_joint.object.parent.class_info=HSD_CLASS_INFO(&hsdJObj);
 rig_joint=(HSD_JObj){.object.parent.class_info=HSD_CLASS_INFO(&hsdJObj),.scale={1,1,1}};
 fighter.ft_data=&rig_package.assets.character.data;fighter.parts=rig_parts;
 rig_parts[0].joint=&creator_joint;rig_parts[1].joint=&rig_joint;spawn_caller->obj_kind=HSD_GObj_JObjKind;
 uint32_t function=UINT32_MAX;
 const NativeScriptFighterProgram* root=nativeScriptFighter_Program(Ft_Kind_Mario);
 for(unsigned n=0;n<root->bank->declarations.function_count;++n)
  if(nativeScriptSource_SpanEquals(&root->bank->tree,root->bank->declarations.functions[n].name,"SpawnAtPart"))function=n;
 CHECK(function!=UINT32_MAX);
 NativeScriptValue raw={.type=NativeScriptType_I32};
 for(int slot=-1;slot<=2;++slot) {
  raw.as.i64=slot;unsigned before=effect_models;
  CHECK(nativeScriptFighter_Helper(spawn_caller,777,function,&raw,1,NULL,&diagnostic)==NativeScriptVM_Ok);
  if(slot<0||slot>=2){CHECK(effect_models==before);++cases;continue;}
  HSD_GObj* effect=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_EFFECT_UNK0];CHECK(effect&&nativeScriptEffect_IsAttached(effect));
  EF_Effect* data=effect->user_data;HSD_JObj* expected=rig_parts[slot].joint;
  CHECK(data->attach_jobj==expected&&data->native_origin_joint==expected&&data->native_attachment_part==slot);
  HSD_GObj_80390CFC();CHECK(nativeScriptEffect_Status(effect)==NativeScriptVM_Ok);
  copy_effect_history();
 size_t bytes;CHECK(nativeScriptEffect_Save(packed,sizeof(packed),&bytes));
  memcpy(second,packed,bytes);
  ((NativeScriptEffectSnapshotRecord*)(second+sizeof(NativeScriptEffectSnapshotHeader)))->origin_identity.on_part=2;
  reject(bytes);
  rig_parts[slot].joint=NULL;CHECK(nativeScriptEffect_Status(effect)==NativeScriptVM_SourceRetired);
  memcpy(second,packed,bytes);reject(bytes);
  rig_parts[slot].joint=expected;CHECK(nativeScriptEffect_Load(packed,bytes));
  CHECK(nativeScriptEffect_Status(effect)==NativeScriptVM_Ok&&data->attach_jobj==expected);
  HSD_GObjFree(effect);CHECK(!nativeScriptEffect_HasActors(Ft_Kind_Mario));++cases;
 }
 rig_available=false;
}
static void deferred_origins(void)
{
 unsigned frees=effect_queue_frees;
 rig_available=true;rig_package.assets.ready=true;rig_package.assets.facts.joints=2;
 rig_package.assets.parts.parts_num=2;rig_tables[Ft_Kind_Mario]=&rig_package.assets.parts;ftPartsTable=rig_tables;
 creator_joint.object.parent.class_info=HSD_CLASS_INFO(&hsdJObj);
 rig_joint=(HSD_JObj){.object.parent.class_info=HSD_CLASS_INFO(&hsdJObj),.scale={1,1,1},.translate={30,40,50}};
 fighter.ft_data=&rig_package.assets.character.data;fighter.parts=rig_parts;
 rig_parts[0].joint=&creator_joint;rig_parts[1].joint=&rig_joint;spawn_caller->obj_kind=HSD_GObj_JObjKind;
 const NativeScriptFighterProgram* root=nativeScriptFighter_Program(Ft_Kind_Mario);
 uint32_t function=UINT32_MAX;
 for(unsigned n=0;n<root->bank->declarations.function_count;++n)
  if(nativeScriptSource_SpanEquals(&root->bank->tree,root->bank->declarations.functions[n].name,"SpawnDeferredAtPart"))function=n;
 CHECK(function!=UINT32_MAX);
 NativeScriptValue raw={.type=NativeScriptType_I32,.as.i64=1};
 // Source name lowering and immediate construction use the actual selected
 // bone sample, not Fighter.cur_pos or the declaration's numeric owner index.
 CHECK(nativeScriptFighter_Helper(spawn_caller,777,function,&raw,1,NULL,&diagnostic)==NativeScriptVM_Ok);
 HSD_GObj* effect=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_EFFECT_UNK0];CHECK(effect&&nativeScriptEffect_IsAttached(effect));
 CHECK(!memcmp(&((HSD_JObj*)effect->hsd_obj)->translate,&rig_joint.translate,sizeof(Vec3)));
 HSD_GObj_80390CFC();CHECK(((HSD_JObj*)effect->hsd_obj)->rotate.z==0.75f);
 HSD_GObjFree(effect);++cases;

 // Immediate dispatch must propagate a constructor failure to its caller;
 // merely logging it and returning success would let the script keep running.
 NativeScriptRuntimeContext context={.source=spawn_caller,.source_class=NativeCallbackSource_Fighter,
  .generation=777,.actor_id=777,.scene_epoch=nativeGObjPool.lifetime};
 effect_fail_model_at=effect_models+1;
 CHECK(nativeScriptEffect_Spawn(&context,owner(5),true,1,true,&diagnostic)==NativeScriptVM_Error);
 effect_fail_model_at=0;CHECK(!nativeScriptEffect_HasActors(Ft_Kind_Mario));++cases;

 HSD_GObjProc proc={.s_link=3};HSD_GObjProc* prior=HSD_GObj_CurrentInvokedProc;
 HSD_GObj_CurrentInvokedProc=&proc;
 unsigned before=effect_models;
 CHECK(nativeScriptFighter_Helper(spawn_caller,777,function,&raw,1,NULL,&diagnostic)==NativeScriptVM_Ok);
 HSD_GObj_CurrentInvokedProc=prior;
 CHECK(effect_models==before&&fighter.x60C);
 // The joint changes after enqueue, before the engine's real queue flush.
 rig_joint.translate=(Vec3){70,80,90};efAsync_QueueFlush(spawn_caller,&fighter.x60C);
 effect=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_EFFECT_UNK0];CHECK(effect&&nativeScriptEffect_IsAttached(effect)&&!fighter.x60C);
 CHECK(!memcmp(&((HSD_JObj*)effect->hsd_obj)->translate,&rig_joint.translate,sizeof(Vec3)));
 HSD_GObj_80390CFC();CHECK(nativeScriptEffect_Status(effect)==NativeScriptVM_Ok&&((HSD_JObj*)effect->hsd_obj)->rotate.z==0.75f);
 HSD_GObjFree(effect);++cases;
 // Each lifetime/layout/program/part change must drop queued work before a
 // constructor or its script can run. Clear has the same no-construction rule.
 for(unsigned change=0;change<9;++change) {
  before=effect_models;HSD_GObj_CurrentInvokedProc=&proc;
  CHECK(nativeScriptFighter_Helper(spawn_caller,777,function,&raw,1,NULL,&diagnostic)==NativeScriptVM_Ok);
  HSD_GObj_CurrentInvokedProc=prior;
  EF_QueuedEffect* queued=fighter.x60C;CHECK(queued&&queued->native_script_owner==owner(5));
  switch(change) {
  case 0:queued->native_script_epoch++;break;
  case 1:queued->native_creator_generation++;break;
  case 2:queued->native_script_program_identity++;break;
  case 3:queued->native_script_layout_identity++;break;
  case 4:queued->native_script_parameter_identity++;break;
  case 5:queued->native_script_root=&rig_joint;break;
  case 6:queued->native_sample_part=2;break;
  case 7:rig_parts[1].joint=NULL;break;
  case 8:break;
  }
  if(change==8)efAsync_QueueClear(&fighter.x60C);
  else efAsync_QueueFlush(spawn_caller,&fighter.x60C);
  rig_parts[1].joint=&rig_joint;
  CHECK(effect_models==before&&!fighter.x60C&&!nativeScriptEffect_HasActors(Ft_Kind_Mario));++cases;
 }
 HSD_GObj_CurrentInvokedProc=&proc;
 CHECK(nativeScriptFighter_Helper(spawn_caller,777,function,&raw,1,NULL,&diagnostic)==NativeScriptVM_Ok);
 HSD_GObj_CurrentInvokedProc=prior;
 before=effect_models;nativeScriptFighter_Detach(spawn_caller);
 efAsync_QueueFlush(spawn_caller,&fighter.x60C);
 CHECK(effect_models==before&&!fighter.x60C);++cases;
 CHECK(effect_queue_frees==frees+13);
 rig_available=false;
}
static void copy_effect_history(void)
{
 _Alignas(64) uint8_t memory[64];static uint8_t history_memory[8*1024*1024];
 NativeMatchArena arena;NativeMatchArenaFrame history;size_t bytes,after;
 CHECK(nativeMatchArena_Begin(&arena,memory,sizeof(memory),nativeScriptEffectEpoch));
 CHECK(nativeScriptEffect_AdmitHistory(&arena));
 CHECK(nativeMatchArena_BindFrame(&arena,&history,history_memory,sizeof(history_memory)));
 CHECK(nativeMatchArena_Save(&history));CHECK(nativeScriptEffect_Save(packed,sizeof(packed),&bytes));
 for(unsigned pass=0;pass<2;++pass){
  for(unsigned i=0;i<nativeScriptEffectInitializedCount;++i)if(nativeScriptEffectActors[i].active){
   NativeScriptEffectActor* actor=&nativeScriptEffectActors[i];actor->bytes[0]^=0xFF;
   actor->storage.identity^=0x1234;actor->storage.state=NULL;actor->creator_view.storage=NULL;
   nativeScriptEffectByObject[actor->members[0].slot]=UINT16_MAX;actor->members[0].generation^=0x1234;
  }
  CHECK(nativeMatchArena_Load(&history));CHECK(nativeScriptEffect_Save(second,sizeof(second),&after));
  CHECK(after==bytes&&!memcmp(packed,second,bytes));
 }
 ++cases;
}

int main(void)
{
 prepare();HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);scheduler_reset();caller();
 CHECK(nativeScriptFighter_Run(spawn_caller,NativeScriptFighter_Tick,NULL,0, NULL,NULL)==NativeScriptVM_Ok);HSD_GObj* effect=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_EFFECT_UNK0];CHECK(effect&&nativeScriptEffect_IsAttached(effect)&&((EF_Effect*)effect->user_data)->update==NULL&&!nativeScriptFighter_CanPublish(Ft_Kind_Mario));
 HSD_GObj_80390CFC();CHECK(nativeScriptEffect_Status(effect)==NativeScriptVM_Ok);HSD_JObj* root=effect->hsd_obj;CHECK(!(root->child->next->flags&JOBJ_HIDDEN)&&fabsf(root->rotate.z+atan2f(.6f,.8f))<1e-6f);++cases;
 for(unsigned branch=0;branch<4;++branch) {
     fighter.motion_id=branch&1?350:349;fighter.cmd_vars[3]=branch&2?0:1;fighter.coll_data.floor.normal=(Vec3){branch&1?-.6f:.6f,.8f,0};
     s32 native_flag=1;memcpy(&fighter.mv.co.common.x4.z,&native_flag,sizeof(native_flag));
     HSD_JObj reference_root=*root,first=*root->child,next=*root->child->next;reference_root.child=&first;first.next=&next;first.parent=next.parent=&reference_root;next.next=NULL;
     HSD_GObj reference_object={.hsd_obj=&reference_root};EF_Effect reference={.gobj=&reference_object,.parent_gobj=spawn_caller};effect_reference_tornado(&reference);
     HSD_GObj_80390CFC();CHECK(root->rotate.z==reference_root.rotate.z&&(root->child->next->flags&JOBJ_HIDDEN)==(next.flags&JOBJ_HIDDEN));
 }++cases;
 fighter.motion_id=350;fighter.cmd_vars[3]=0;HSD_GObj_80390CFC();CHECK(root->child->next->flags&JOBJ_HIDDEN);CHECK(root->rotate.z==0);++cases;
 HSD_JObj* child=root->child;root->child=NULL;HSD_GObj_80390CFC();CHECK(nativeScriptEffect_Status(effect)==NativeScriptVM_Ok);root->child=child;++cases;
 size_t bytes;CHECK(nativeScriptEffect_Save(packed,sizeof(packed),&bytes));CHECK(nativeScriptEffect_ValidateSnapshot(packed,bytes));
 HSD_GObj_80390CFC();CHECK(nativeScriptEffect_Load(packed,bytes));size_t copied;CHECK(nativeScriptEffect_Save(second,sizeof(second),&copied)&&copied==bytes&&!memcmp(packed,second,bytes));++cases;
 memcpy(second,packed,bytes);((NativeScriptEffectSnapshotRecord*)(second+sizeof(NativeScriptEffectSnapshotHeader)))->program_identity^=1;reject(bytes);++cases;
 memcpy(second,packed,bytes);((NativeScriptEffectSnapshotRecord*)(second+sizeof(NativeScriptEffectSnapshotHeader)))->members[0].generation++;reject(bytes);++cases;
 memcpy(second,packed,bytes);((NativeScriptEffectSnapshotRecord*)(second+sizeof(NativeScriptEffectSnapshotHeader)))->creator_layout++;reject(bytes);++cases;
 memcpy(second,packed,bytes);((NativeScriptEffectSnapshotRecord*)(second+sizeof(NativeScriptEffectSnapshotHeader)))->faulted=2;reject(bytes);++cases;
 memcpy(second,packed,bytes);NativeScriptEffectSnapshotRecord* malformed=(NativeScriptEffectSnapshotRecord*)(second+sizeof(NativeScriptEffectSnapshotHeader));
 for(unsigned field=0;field<bank.storage_layout.field_count;++field)if(fields[field].owner==owner(0)&&fields[field].type==NativeScriptType_Bool&&!fields[field].parameter){second[sizeof(NativeScriptEffectSnapshotHeader)+sizeof(*malformed)+fields[field].offset]=2;break;}reject(bytes);++cases;
 memcpy(second,packed,bytes);memcpy(second+bytes,packed+sizeof(NativeScriptEffectSnapshotHeader),bytes-sizeof(NativeScriptEffectSnapshotHeader));
 ((NativeScriptEffectSnapshotHeader*)second)->count=2;((NativeScriptEffectSnapshotHeader*)second)->bytes=(uint32_t)(2*bytes-sizeof(NativeScriptEffectSnapshotHeader));
 ((NativeScriptEffectSnapshotRecord*)(second+bytes))->slot++;reject(2*bytes-sizeof(NativeScriptEffectSnapshotHeader));++cases;
 NativeRollbackStateRegion peer_region;CHECK(nativeRollbackState_GetGlobalRegions(&peer_region,1)==1&&peer_region.size<=sizeof(oracle));memcpy(oracle,peer_region.memory,peer_region.size);
 size_t scene_bytes=nativeMatchArena_Used(&nativeMatchSceneArena);
 size_t object_bytes=nativeGObjPool.capacity*nativeGObjPool.stride,proc_bytes=nativeGObjProcPool.capacity*nativeGObjProcPool.stride;
 CHECK(object_bytes+proc_bytes<=sizeof(payload_oracle));memcpy(scene_oracle,nativeMatchSceneArena.memory,scene_bytes);
 memcpy(payload_oracle,nativeGObjPool.payload,object_bytes);memcpy(payload_oracle+object_bytes,nativeGObjProcPool.payload,proc_bytes);
 effect_peer_checkpoint(false);
 NativeMatchPoolReference original;CHECK(nativeMatchPool_Reference(&nativeGObjPool,effect,&original));HSD_GObjFree(effect);HSD_GObj* reused=spawn(0);CHECK(reused==effect&&!nativeScriptEffect_Load(packed,bytes));
 memcpy(peer_region.memory,oracle,peer_region.size);
 memcpy(nativeMatchSceneArena.memory,scene_oracle,scene_bytes);memcpy(nativeGObjPool.payload,payload_oracle,object_bytes);
 memcpy(nativeGObjProcPool.payload,payload_oracle+object_bytes,proc_bytes);effect_peer_checkpoint(true);CHECK(nativeMatchPool_Resolve(original)==effect);
 for(unsigned slot=0;slot<NATIVE_SCRIPT_EFFECT_ACTORS;++slot)if(nativeScriptEffectActors[slot].active)memset(nativeScriptEffectActors[slot].bytes,0,sizeof(nativeScriptEffectActors[slot].bytes));
 CHECK(nativeScriptEffect_Load(packed,bytes));CHECK(nativeScriptEffect_Save(second,sizeof(second),&copied)&&copied==bytes&&!memcmp(packed,second,bytes));root=effect->hsd_obj;HSD_GObj_80390CFC();CHECK(nativeScriptEffect_Status(effect)==NativeScriptVM_Ok);++cases;
 HSD_GObj* cancellation=spawn(1);NativeMatchPoolReference dead;CHECK(nativeMatchPool_Reference(&nativeGObjPool,cancellation,&dead));HSD_GObj_80390CFC();CHECK(!nativeMatchPool_Resolve(dead)&&!nativeScriptEffect_IsAttached(cancellation)&&nativeScriptRuntime_Idle());++cases;
 ((EF_Effect*)effect->user_data)->state_flags=EF_STATE_PAUSED;unsigned animations=effect_animations;HSD_GObj_80390CFC();CHECK(effect_animations==animations);((EF_Effect*)effect->user_data)->state_flags=EF_STATE_ACTIVE;++cases;
 effect_retire_on_anim=1;HSD_GObj_80390CFC();CHECK(!nativeScriptEffect_HasActors(Ft_Kind_Mario));++cases;caller();
 effect_retire_on_create=1;NativeScriptRuntimeContext c={.source=spawn_caller,.source_class=NativeCallbackSource_Fighter,.generation=777,.scene_epoch=nativeGObjPool.lifetime};
 CHECK(nativeScriptEffect_Spawn(&c,owner(0),false,0,false,&diagnostic)==NativeScriptVM_SourceRetired);CHECK(nativeScriptEffect_HasActors(Ft_Kind_Mario)&&!nativeScriptFighter_CanPublish(Ft_Kind_Mario));HSD_GObj_80390CFC();CHECK(!nativeScriptEffect_HasActors(Ft_Kind_Mario));++cases;caller();
 HSD_GObj* bare=spawn(2);CHECK(nativeScriptEffect_Save(packed,sizeof(packed),&bytes));HSD_GObj_80390CFC();CHECK(nativeScriptEffect_Status(bare)==NativeScriptVM_Ok);HSD_GObjFree(bare);++cases;
 HSD_GObj* pair=spawn(3);EF_Effect* pair_member=pair->user_data;CHECK(pair_member->native_member_count==2);HSD_GObj* pair_root=pair_member->native_members[0].object;CHECK(nativeScriptEffect_Spec(pair_root)->owner==owner(3));
 NativeMatchPoolReference pair_ref[2];for(unsigned member=0;member<2;++member)CHECK(nativeMatchPool_Reference(&nativeGObjPool,pair_member->native_members[member].object,&pair_ref[member]));
 HSD_GObj_80390CFC();CHECK(nativeScriptEffect_Status(pair_root)==NativeScriptVM_Ok);CHECK(nativeScriptEffect_Save(packed,sizeof(packed),&bytes)&&nativeScriptEffect_Load(packed,bytes));HSD_GObjFree(pair_root);
 CHECK(!nativeMatchPool_Resolve(pair_ref[0])&&!nativeMatchPool_Resolve(pair_ref[1])&&!nativeScriptEffect_HasActors(Ft_Kind_Mario));++cases;
 unsigned freed=effect_frees;effect_fail_model_at=effect_models+2;CHECK(nativeScriptEffect_Spawn(&c,owner(3),false,0,false,&diagnostic)==NativeScriptVM_Error&&effect_frees==freed+2&&!nativeScriptEffect_HasActors(Ft_Kind_Mario));effect_fail_model_at=0;++cases;
 for(unsigned cycle=0;cycle<1024;++cycle){effect=spawn(0);HSD_GObj_80390CFC();CHECK(nativeScriptEffect_Status(effect)==NativeScriptVM_Ok);HSD_GObjFree(effect);CHECK(!nativeScriptEffect_HasActors(Ft_Kind_Mario));}++cases;
 for(unsigned count=0;count<NATIVE_SCRIPT_EFFECT_ACTORS;++count)spawn(2);CHECK(nativeScriptEffect_Save(packed,sizeof(packed),&bytes));CHECK(((NativeScriptEffectSnapshotHeader*)packed)->count==NATIVE_SCRIPT_EFFECT_ACTORS);
 CHECK(nativeScriptEffect_Spawn(&c,owner(2),false,0,false,&diagnostic)==NativeScriptVM_Error);++cases;
 while(HSD_GObjPLinkHead[HSD_GOBJ_PLINK_EFFECT_UNK0])HSD_GObjFree(HSD_GObjPLinkHead[HSD_GOBJ_PLINK_EFFECT_UNK0]);
 CHECK(nativeScriptEffect_Save(second,sizeof(second),&copied)&&copied==sizeof(NativeScriptEffectSnapshotHeader));CHECK(!nativeScriptEffect_Load(packed,bytes));++cases;
 effect=spawn(4);root=effect->hsd_obj;HSD_GObj_80390CFC();
 CHECK(nativeScriptEffect_Status(effect)==NativeScriptVM_Ok&&root->rotate.x==0.25f&&root->rotate.y==-0.5f&&root->rotate.z==-0.25f);
 CHECK(nativeScriptEffect_Save(second,sizeof(second),&copied)&&nativeScriptEffect_Load(second,copied));
 root->rotate.x=99;root->rotate.y=98;root->rotate.z=97;HSD_GObj_80390CFC();
 CHECK(nativeScriptEffect_Status(effect)==NativeScriptVM_Ok&&root->rotate.x==0.25f&&root->rotate.y==-0.5f&&root->rotate.z==-0.25f);
 HSD_GObjFree(effect);CHECK(!nativeScriptEffect_HasActors(Ft_Kind_Mario));++cases;
 part_origins();
 deferred_origins();
 NativeRollbackStateRegion region;CHECK(nativeRollbackState_GetGlobalRegions(&region,1)==1);CHECK((uintptr_t)nativeScriptEffectActors>=(uintptr_t)region.memory&&(uintptr_t)nativeScriptEffectActors+sizeof(nativeScriptEffectActors)<=(uintptr_t)region.memory+region.size);
 CHECK(!((uintptr_t)nativeScriptRuntime_VM()>=(uintptr_t)region.memory&&(uintptr_t)nativeScriptRuntime_VM()<(uintptr_t)region.memory+region.size));
 printf("{\"status\":\"PASS connected effect owner\",\"cases\":%u,\"checks\":%u,\"native_model_frees\":%u,\"full_packed_bytes\":%zu,\"actor_reservation\":%zu}\n",cases,checks,effect_frees,bytes,sizeof(nativeScriptEffectActors));return 0;
}
