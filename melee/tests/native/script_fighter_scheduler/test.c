#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "boundary.h"
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_fighter_snapshot.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_script_actor_access.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftcpuattack.h>
#include <melee/ft/ftchangeparam.h>
#include <melee/ft/kinds/ftCommon/ftpickupitem.h>
#include <melee/ft/kinds/ftCommon/ftCo_Landing.h>
#include <melee/ft/kinds/ftCommon/ftCo_Wait.h>
#include <melee/ft/kinds/ftCommon/ftCo_Attack1.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/fighter.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <melee/gm/native_match_snapshot_pool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum{N=2048,F=128,V=128};
static char source_bytes[65536];static NativeScriptSourceNode syntax[N];static NativeScriptIRNode expressions[N*2];static uint32_t arguments[N*2];
static NativeScriptFunctionDeclaration declarations[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[64];
static NativeScriptInstruction instructions[8192];static NativeScriptFunctionCode code[F];static NativeScriptExecutionFunction functions[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[8];static NativeScriptParameterSlot slots[V];
static NativeScriptCompileBank bank;static NativeScriptFighterProgram program;static NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=10,.ground={343,345,347,349},.air={344,346,348,350}};
static Fighter fighters[32];static Item item;static HSD_JObj joints[32];static ftData data;static ftCo_DatAttrs attrs;static struct itPickup pickup;static Vec2 extra;
static unsigned count,checks;static uint8_t snapshot[65536],changed[65536];
static HSD_GObj* replacement;
#define CHECK(v)do{++checks;if(!(v)){fprintf(stderr,"fighter scheduler check %u %s\n",__LINE__,#v);exit(1);}}while(0)
static void userdata_remove(void* p){nativeScriptFighter_Detach(((Fighter*)p)->gobj);}
static void item_remove(void* p){(void)p;}
static void prepare(void)
{
 bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=syntax,.capacity=N},.expressions={.nodes=expressions,.node_capacity=N*2,.arguments=arguments,.argument_capacity=N*2},.declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=64,.instructions=instructions,.instruction_capacity=8192,.function_code=code,.function_code_capacity=F,.execution_functions=functions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=8},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* common_source="fighter:\n    behavior independent\nstate int trace\nstate bool handled\nstate int pulses\nstate bool landing_option\nstate bool landing_option_after\nfunc ResetRoot():\n    state.trace = 0\n    position.x = 0\nfunc Trace(int code):\n    state.trace = state.trace * 10 + code\n    position.x = state.trace\naction Events:\n    on wait_input:\n        Trace(8)\n        return state.handled\n    on action_status:\n        Trace(9)\n        return state.handled\n    on load:\n        Trace(1)\n    on cpu_capture_decision:\n        Trace(2)\n        return state.handled\n    on cpu_ranged_candidates:\n        Trace(3)\n    on load_special_attrs:\n        Trace(4)\n    on landing_action:\n        state.landing_option = event.interrupt_allowed\n        Trace(5)\n        state.landing_option_after = event.interrupt_allowed\n    on item_pickup_ext:\n        if event.flag: Trace(6)\n        else: Trace(7)\n    on item_drop_ext:\n        if event.flag: Trace(8)\n        else: Trace(9)\n    on item_pickup:\n        if event.flag: Trace(2)\n        else: Trace(3)\n    on item_drop:\n        if event.flag: Trace(4)\n        else: Trace(5)\n    on item_visible:\n        Trace(6)\n    on item_invisible:\n        Trace(7)\n    on knockback_enter:\n        Trace(8)\n    on knockback_exit:\n        Trace(9)\n    on jab_start:\n        Trace(4)\n        return Transition.skip_attack_count\n    on auxiliary_item_attach:\n        Trace(2)\n    on auxiliary_item_visibility_reset:\n        Trace(3)\nmove Fireball:\n    on special_n:\n        pass\n    on neutral_special_cue:\n        Trace(2)\nfunc Pulse():\n    state.pulses = state.pulses + 1\nfunc ClearSelf():\n    state.pulses = state.pulses + 1\n    callbacks.accessory(None)\nfunc RebindSelf():\n    state.pulses = state.pulses + 1\n    callbacks.accessory(Pulse)\nfunc ArmPhases():\n    callbacks.animation(Pulse)\n    callbacks.interrupt(Pulse)\n    callbacks.physics(Pulse)\n    callbacks.collision(Pulse)\n    callbacks.accessory(Pulse)\nfunc ArmCueClear():\n    callbacks.accessory_cue(None)\nfunc ArmEveryFrame():\n    callbacks.accessory(Pulse)\nfunc ArmCue():\n    callbacks.accessory_cue(Pulse)\nfunc ArmClear():\n    callbacks.accessory(ClearSelf)\nfunc ArmRebind():\n    callbacks.accessory(RebindSelf)\nfunc RetiringPulse():\n    state.pulses = state.pulses + 1\n    position.x = 1\n    state.pulses = state.pulses + 100\nfunc WithArgument(int value):\n    pass\nfunc BoolResult() -> bool:\n    return true\n";
 static char source[16384];
 snprintf(source,sizeof(source),"%s\nfunc ArmContacts():\n    callbacks.track_reflect_hit()\n    callbacks.track_reflect()\n    callbacks.track_hurtbox_detect()\n    callbacks.track_shield_hit()\nfunc ArmContactHelpers():\n    callbacks.reflect_hit(Pulse)\n    callbacks.reflect(Pulse)\n    callbacks.hurtbox_detect(Pulse)\n    callbacks.shield_hit(Pulse)\nfunc QueryPulse():\n    state.handled = callbacks.matches(Callback.reflect_hit, Pulse)\nfunc QueryNone():\n    state.handled = callbacks.matches(Callback.reflect_hit, None)\nfunc ClearContacts():\n    callbacks.reflect_hit(None)\n    callbacks.reflect(None)\n    callbacks.hurtbox_detect(None)\n    callbacks.shield_hit(None)\naction Contacts:\n    on reflect_hit:\n        state.pulses = state.pulses + 1\n    on reflect:\n        state.pulses = state.pulses + 2\n    on hurtbox_detect:\n        state.pulses = state.pulses + 4\n    on shield_hit:\n        state.pulses = state.pulses + 8\n",common_source);
 NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout};const NativeScriptCompileBank* accepted=NULL;NativeScriptDiagnostic d={0};
 if(!nativeScriptCompile_Build(&bank,&request,&accepted,&d)||!nativeScriptFighter_PrepareProgram(Ft_Kind_Mario,accepted,&layout,NULL,&program,&d)){fprintf(stderr,"prepare %u:%u %s\n",d.source.line,d.source.column,d.message);exit(1);}
 CHECK(nativeScriptFighter_CanPublish(Ft_Kind_Mario));nativeScriptFighter_PublishProgram(Ft_Kind_Mario,&program);
 data.x0=&attrs;data.x40=&pickup;data.x50=&extra;
}
static HSD_GObj* create(void)
{
 CHECK(count<32);HSD_GObj* object=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);Fighter* fighter=&fighters[count];memset(fighter,0,sizeof(*fighter));fighter->gobj=object;fighter->kind=Ft_Kind_Mario;fighter->behavior_kind=Ft_Kind_Mario;fighter->ft_data=&data;fighter->x8_spawnNum=++count;fighter->x34_scale=(Vec3){1,1,1};fighter->ground_or_air=GA_Ground;fighter->x221E_b3=true;object->hsd_obj=&joints[count-1];GObj_InitUserData(object,4,userdata_remove,fighter);CHECK(nativeScriptFighter_Attach(object));return object;
}
static NativeScriptValue state(HSD_GObj* object,const char* name,const NativeScriptValue* input)
{
 NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(object,((Fighter*)object->user_data)->x8_spawnNum,&view));NativeScriptDiagnostic d;
 for(unsigned i=0;i<view.program->value_count;++i)if(!view.program->values[i].owner&&nativeScriptSource_SpanEquals(view.program->tree,view.program->values[i].name,name)){NativeScriptValue out;if(input)CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,input,&d));CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&out,&d));return out;}CHECK(false);return(NativeScriptValue){0};
}
static HSD_GObj* create_replacement(void){CHECK(nativeScriptFighter_ReferencesProgram(&bank));replacement=create();return replacement;}
static void reset_trace(HSD_GObj* object){NativeScriptValue value={.type=NativeScriptType_I32,.as.i64=1};state(object,"trace",&value);}
static uint32_t function_id(const char* name)
{
 for(uint32_t i=0;i<bank.declarations.function_count;++i)
  if(nativeScriptSource_SpanEquals(&bank.tree,declarations[i].name,name))return i;
 CHECK(false);return UINT32_MAX;
}
static void invoke_helper(HSD_GObj* object,const char* name)
{
 NativeScriptValue result={0};NativeScriptDiagnostic diagnostic={0};
 CHECK(nativeScriptFighter_Helper(object,((Fighter*)object->user_data)->x8_spawnNum,
     function_id(name),NULL,0,&result,&diagnostic)==NativeScriptVM_Ok);
 CHECK(result.type==NativeScriptType_Void&&nativeScriptFighter_Status(object)==NativeScriptVM_Ok);
}
static void helper_status(HSD_GObj* object)
{
 uint32_t generation=((Fighter*)object->user_data)->x8_spawnNum;
 NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=123};NativeScriptDiagnostic diagnostic={0};
 CHECK(nativeScriptFighter_Helper(object,generation,function_id("BoolResult"),NULL,0,&out,&diagnostic)==NativeScriptVM_Ok);
 CHECK(out.type==NativeScriptType_Bool&&out.as.u64==1);
 uint32_t invalid[]={UINT32_MAX,program.hooks[NativeScriptFighter_Load],function_id("WithArgument")};
 for(unsigned i=0;i<3;++i){out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=123};memset(&diagnostic,0,sizeof(diagnostic));
  CHECK(nativeScriptFighter_Helper(object,generation,invalid[i],NULL,0,&out,&diagnostic)==NativeScriptVM_Error);
  CHECK(out.type==NativeScriptType_U64&&out.as.u64==123&&diagnostic.message[0]);
  CHECK(nativeScriptFighter_Status(object)==NativeScriptVM_Ok&&nativeScriptRuntime_Idle());
 }
 out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=123};
 CHECK(nativeScriptFighter_Helper(object,generation+1,function_id("Pulse"),NULL,0,&out,&diagnostic)==NativeScriptVM_Error);
 CHECK(out.type==NativeScriptType_U64&&out.as.u64==123&&nativeScriptFighter_Status(object)==NativeScriptVM_Ok);
 CHECK(nativeScriptFighter_Helper(NULL,generation,function_id("Pulse"),NULL,0,&out,&diagnostic)==NativeScriptVM_Error);
 CHECK(out.type==NativeScriptType_U64&&out.as.u64==123&&diagnostic.message[0]);
 static NativeScriptProgramIndex foreign;
 CHECK(!nativeScriptActors_Helper(NULL,object,generation,&foreign,function_id("Pulse"),NULL,0,&out,&diagnostic));
 CHECK(out.type==NativeScriptType_U64&&out.as.u64==123);
 CHECK(nativeScriptActors_Helper(NULL,object,generation,&bank.declarations,function_id("BoolResult"),NULL,0,&out,&diagnostic));
 CHECK(out.type==NativeScriptType_Bool&&out.as.u64==1);
 out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=123};
 size_t bytes;CHECK(nativeScriptFighter_Save(snapshot,sizeof(snapshot),&bytes));
 scheduler_fail_write=true;
 CHECK(nativeScriptFighter_Helper(object,generation,function_id("RetiringPulse"),NULL,0,&out,&diagnostic)==NativeScriptVM_Error);
 scheduler_fail_write=false;
 CHECK(out.type==NativeScriptType_U64&&out.as.u64==123&&diagnostic.message[0]);
 CHECK(nativeScriptFighter_Status(object)==NativeScriptVM_Error&&nativeScriptRuntime_Idle());
 CHECK(nativeScriptFighter_Helper(object,generation,function_id("Pulse"),NULL,0,&out,&diagnostic)==NativeScriptVM_Error);
 CHECK(!strcmp(diagnostic.message,"Fighter helper recipient is faulted")&&
     !diagnostic.source.line&&!diagnostic.source.column&&out.type==NativeScriptType_U64&&out.as.u64==123);
 CHECK(nativeScriptFighter_Load(snapshot,bytes)&&nativeScriptFighter_Status(object)==NativeScriptVM_Ok);
}
static void set_pulses(HSD_GObj* object,int32_t pulses)
{NativeScriptValue value={.type=NativeScriptType_I32,.as.i64=pulses};state(object,"pulses",&value);}
static unsigned native_accessory_calls;
static void native_accessory(HSD_GObj* object){(void)object;++native_accessory_calls;}
static NativeScriptFighterSnapshotRecord* saved_actor(uint8_t* bytes,HSD_GObj* object)
{
 NativeScriptFighterSnapshotHeader* header=(void*)bytes;
 uint8_t* cursor=bytes+sizeof(*header);
 for(uint32_t i=0;i<header->count;++i){
  NativeScriptFighterSnapshotRecord* record=(void*)cursor;
  if(nativeMatchPool_Resolve(record->object)==object)return record;
  cursor+=sizeof(*record)+record->state_bytes;
 }
 CHECK(false);return NULL;
}
static void motion_helpers(HSD_GObj* object)
{
 Fighter* fighter=object->user_data;uint32_t pulse=function_id("Pulse");size_t bytes;
 set_pulses(object,0);
 // Every phase registers and executes authored helpers through catalog/Setup/VM.
 invoke_helper(object,"ArmPhases");
 for(uint32_t phase=0;phase<NativeScriptMotion_Count;++phase){
  CHECK(nativeScriptFighter_MotionBound(object,phase));
  CHECK(nativeScriptFighter_RunMotion(object,phase));
  CHECK(state(object,"pulses",NULL).as.i64==phase+1);
 }
 CHECK(nativeScriptFighter_Save(snapshot,sizeof(snapshot),&bytes));
 for(uint32_t phase=0;phase<NativeScriptMotion_Count;++phase)
  CHECK(nativeScriptFighter_BindMotionPhase(object,phase,UINT32_MAX,false));
 CHECK(nativeScriptFighter_Load(snapshot,bytes));
 for(uint32_t phase=0;phase<NativeScriptMotion_Count;++phase){
  CHECK(nativeScriptFighter_MotionBound(object,phase));
  CHECK(nativeScriptFighter_RunMotion(object,phase));
 }
 CHECK(state(object,"pulses",NULL).as.i64==2*NativeScriptMotion_Count);
 // Invalid registration is atomic and never faults a live actor.
 uint32_t invalid_ids[]={function_id("WithArgument"),function_id("BoolResult"),
     program.hooks[NativeScriptFighter_Load],program.hooks[NativeScriptFighter_CpuCaptureDecision],
     program.motions[0].functions[NativeScriptMotion_Accessory],bank.declarations.function_count,UINT32_MAX-1};
 for(unsigned i=0;i<sizeof(invalid_ids)/sizeof(*invalid_ids);++i){
  if(invalid_ids[i]==UINT32_MAX)continue;
  CHECK(!nativeScriptFighter_BindMotionPhase(object,NativeScriptMotion_Accessory,invalid_ids[i],false));
  CHECK(nativeScriptFighter_MotionBound(object,NativeScriptMotion_Accessory));
  CHECK(nativeScriptFighter_Status(object)==NativeScriptVM_Ok);
 }
 for(uint32_t phase=0;phase<NativeScriptMotion_Accessory;++phase)
  CHECK(!nativeScriptFighter_BindMotionPhase(object,phase,pulse,true));
 CHECK(!nativeScriptFighter_BindMotionPhase(object,NativeScriptMotion_Count,pulse,false));
 // Actual catalog + Setup registers helpers from source; the actual accessory
 // scheduler consumes cue mode, while every-frame mode leaves cue bits alone.
 for(unsigned cue=0;cue<2;++cue){
  invoke_helper(object,cue?"ArmCue":"ArmEveryFrame");set_pulses(object,0);
  unsigned expected=0;
  for(unsigned frame=0;frame<256;++frame){
   bool bit=((frame*73u+19u)&15u)<5u;fighter->throw_flags_b0=bit;
   Fighter_8006C80C(object);expected+=!cue||bit;
   CHECK(state(object,"pulses",NULL).as.i64==expected);
   CHECK(fighter->throw_flags_b0==(cue?false:bit));
  }
 }
 invoke_helper(object,"ArmClear");set_pulses(object,0);
 Fighter_8006C80C(object);CHECK(state(object,"pulses",NULL).as.i64==1);
 CHECK(!nativeScriptFighter_MotionBound(object,NativeScriptMotion_Accessory));
 Fighter_8006C80C(object);CHECK(state(object,"pulses",NULL).as.i64==1);
 invoke_helper(object,"ArmRebind");set_pulses(object,0);
 Fighter_8006C80C(object);Fighter_8006C80C(object);
 CHECK(state(object,"pulses",NULL).as.i64==2);
 // Native common-engine assignment supersedes the source helper permanently.
 fighter->accessory4_cb=native_accessory;unsigned native_before=native_accessory_calls;
 Fighter_8006C80C(object);CHECK(native_accessory_calls==native_before+1);
 CHECK(!nativeScriptFighter_MotionBound(object,NativeScriptMotion_Accessory));
 fighter->accessory4_cb=NULL;Fighter_8006C80C(object);
 CHECK(state(object,"pulses",NULL).as.i64==2);
 invoke_helper(object,"ArmCueClear");
 CHECK(!nativeScriptFighter_MotionBound(object,NativeScriptMotion_Accessory));
 CHECK(nativeScriptFighter_Save(snapshot,sizeof(snapshot),&bytes));
 CHECK(saved_actor(snapshot,object)->accessory_cue==0);
 // Reselection restores the immutable prepared callback and cue policy.
 invoke_helper(object,"ArmEveryFrame");nativeScriptFighter_SelectMotion(object,343);
 reset_trace(object);fighter->throw_flags_b0=false;Fighter_8006C80C(object);
 CHECK(state(object,"trace",NULL).as.i64==1);
 fighter->throw_flags_b0=true;Fighter_8006C80C(object);
 CHECK(state(object,"trace",NULL).as.i64==12&&!fighter->throw_flags_b0);
 invoke_helper(object,"ArmCue");set_pulses(object,37);
 CHECK(nativeScriptFighter_Save(snapshot,sizeof(snapshot),&bytes));
 // Helper snapshots allow both accessory policies, but reject malformed policy,
 // wrong ABI, source callbacks and cue policy on any other phase.
 memcpy(changed,snapshot,bytes);NativeScriptFighterSnapshotRecord* record=saved_actor(changed,object);
 record->accessory_cue=0;CHECK(nativeScriptFighter_ValidateSnapshot(changed,bytes));
 record->accessory_cue=2;CHECK(!nativeScriptFighter_ValidateSnapshot(changed,bytes));
 record->accessory_cue=1;record->motions[NativeScriptMotion_Accessory]=UINT32_MAX;
 CHECK(!nativeScriptFighter_ValidateSnapshot(changed,bytes));
 for(unsigned i=0;i<4;++i){memcpy(changed,snapshot,bytes);record=saved_actor(changed,object);
  record->motions[NativeScriptMotion_Accessory]=invalid_ids[i];
  CHECK(!nativeScriptFighter_ValidateSnapshot(changed,bytes));
  CHECK(!nativeScriptFighter_Load(changed,bytes));
  CHECK(state(object,"pulses",NULL).as.i64==37);}
 invoke_helper(object,"ArmClear");set_pulses(object,91);
 CHECK(nativeScriptFighter_Load(snapshot,bytes));
 CHECK(state(object,"pulses",NULL).as.i64==37);
 fighter->throw_flags_b0=false;Fighter_8006C80C(object);
 CHECK(state(object,"pulses",NULL).as.i64==37);
 fighter->throw_flags_b0=true;Fighter_8006C80C(object);
 CHECK(state(object,"pulses",NULL).as.i64==38);
 // NewLife rebinds generation and preserves authored state and motion helpers.
 uint32_t previous=fighter->x8_spawnNum;NativeScriptReceiverView view;
 fighter->x8_spawnNum+=1000;nativeScriptFighter_NewLife(object,fighter->x8_spawnNum);
 CHECK(!nativeScriptFighter_View(object,previous,&view));
 CHECK(nativeScriptFighter_Status(object)==NativeScriptVM_Ok);
 CHECK(state(object,"pulses",NULL).as.i64==38);
 fighter->throw_flags_b0=true;Fighter_8006C80C(object);
 CHECK(state(object,"pulses",NULL).as.i64==39);
}
static void motion_resurrection(HSD_GObj* object)
{
 // Aggregate restore: actual pool metadata/payload + native Fighter peer + PLink
 // roots, then packed actor ownership/state. No renderer or proc graph exists.
 Fighter saved=*(Fighter*)object->user_data;HSD_GObj* heads[33],*tails[33];
 memcpy(heads,HSD_GObjPLinkHead,sizeof(heads));memcpy(tails,plinklow_gobjs,sizeof(tails));
 NativeMatchPoolReference reference;CHECK(nativeMatchPool_Reference(&nativeGObjPool,object,&reference));
 size_t actor_bytes,pool_bytes=nativeMatchSnapshotPool_Bytes(&nativeGObjPool),written;
 void* pool=malloc(pool_bytes);CHECK(pool);
 CHECK(nativeMatchSnapshotPool_Save(&nativeGObjPool,pool,pool_bytes,&written)&&written==pool_bytes);
 CHECK(nativeScriptFighter_Save(snapshot,sizeof(snapshot),&actor_bytes));
 HSD_GObjFree(object);CHECK(!nativeMatchPool_Resolve(reference));
 CHECK(nativeScriptFighter_Status(object)==NativeScriptVM_SourceRetired);
 HSD_GObj* reused=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);
 CHECK(reused==object&&!nativeMatchPool_Resolve(reference));HSD_GObjFree(reused);
 CHECK(nativeMatchSnapshotPool_Load(&nativeGObjPool,pool,pool_bytes));free(pool);
 *(Fighter*)object->user_data=saved;
 memcpy(HSD_GObjPLinkHead,heads,sizeof(heads));memcpy(plinklow_gobjs,tails,sizeof(tails));
 CHECK(nativeMatchPool_Resolve(reference)==object);
 CHECK(nativeScriptFighter_Load(snapshot,actor_bytes));
 CHECK(nativeScriptFighter_Status(object)==NativeScriptVM_Ok);
 CHECK(state(object,"pulses",NULL).as.i64==39);
 ((Fighter*)object->user_data)->throw_flags_b0=true;Fighter_8006C80C(object);
 CHECK(state(object,"pulses",NULL).as.i64==40);
}
static void native_contact(HSD_GObj* object){(void)object;}
static void contact_callbacks(HSD_GObj* object)
{
 Fighter* fighter=object->user_data;
 HSD_GObjEvent* native[]={&fighter->reflect_hit_cb,&fighter->x21F8,&fighter->hurtbox_detect_cb,&fighter->shield_hit_cb};
 set_pulses(object,0);invoke_helper(object,"ArmContacts");
 size_t bytes;CHECK(nativeScriptFighter_Save(snapshot,sizeof(snapshot),&bytes));
 for(unsigned i=0;i<4;++i){
  unsigned slot=NativeScriptFighterCallback_ReflectHit+i;
  CHECK(!*native[i]&&nativeScriptFighter_CallbackBound(object,slot));
  CHECK(nativeScriptFighter_RunCallback(object,slot));
 }
 CHECK(state(object,"pulses",NULL).as.i64==15);
 nativeScriptFighter_ResetMotionCallbacks(object);
 for(unsigned i=0;i<4;++i)CHECK(!nativeScriptFighter_CallbackBound(object,NativeScriptFighterCallback_ReflectHit+i));
 CHECK(nativeScriptFighter_Load(snapshot,bytes));
 for(unsigned i=0;i<4;++i)CHECK(nativeScriptFighter_CallbackBound(object,NativeScriptFighterCallback_ReflectHit+i));
 CHECK(state(object,"pulses",NULL).as.i64==0);
 memcpy(changed,snapshot,bytes);NativeScriptFighterSnapshotRecord* record=(void*)(changed+sizeof(NativeScriptFighterSnapshotHeader));
 record->callbacks[NativeScriptFighterCallback_ReflectHit]=program.callbacks[NativeScriptFighterCallback_Reflect];
 CHECK(!nativeScriptFighter_ValidateSnapshot(changed,bytes));
 invoke_helper(object,"ArmContactHelpers");
 for(unsigned i=0;i<4;++i){
  unsigned slot=NativeScriptFighterCallback_ReflectHit+i;
  CHECK(!nativeScriptFighter_BindCallback(object,slot,function_id("WithArgument")));
  CHECK(!nativeScriptFighter_BindCallback(object,slot,function_id("BoolResult")));
  CHECK(nativeScriptFighter_CallbackBound(object,slot));
  CHECK(nativeScriptFighter_RunCallback(object,slot));
  *native[i]=native_contact;CHECK(!nativeScriptFighter_CallbackBound(object,slot));
  CHECK(*native[i]==native_contact);
 }
 CHECK(state(object,"pulses",NULL).as.i64==4);
 invoke_helper(object,"ClearContacts");
 for(unsigned i=0;i<4;++i)CHECK(!*native[i]&&!nativeScriptFighter_CallbackBound(object,NativeScriptFighterCallback_ReflectHit+i));
 invoke_helper(object,"ArmContacts");nativeScriptFighter_NewLife(object,fighter->x8_spawnNum);
 for(unsigned i=0;i<4;++i)CHECK(nativeScriptFighter_CallbackBound(object,NativeScriptFighterCallback_ReflectHit+i));
 nativeScriptFighter_ResetMotionCallbacks(object);++checks;
}
static void callback_identity(HSD_GObj* object)
{
 Fighter* fighter=object->user_data;bool result=false;
 unsigned slot=NativeScriptFighterCallback_ReflectHit,pulse=function_id("Pulse");
 CHECK(nativeScriptFighter_CallbackMatches(object,slot,UINT32_MAX,&result)&&result);
 invoke_helper(object,"QueryNone");CHECK(state(object,"handled",NULL).as.u64);
 CHECK(nativeScriptFighter_BindCallback(object,slot,pulse));
 CHECK(nativeScriptFighter_CallbackMatches(object,slot,pulse,&result)&&result);
 invoke_helper(object,"QueryPulse");CHECK(state(object,"handled",NULL).as.u64);
 invoke_helper(object,"QueryNone");CHECK(!state(object,"handled",NULL).as.u64);
 fighter->reflect_hit_cb=native_contact;
 size_t before,after;CHECK(nativeScriptFighter_Save(snapshot,sizeof(snapshot),&before));
 CHECK(nativeScriptFighter_CallbackMatches(object,slot,pulse,&result)&&!result);
 CHECK(nativeScriptFighter_CallbackMatches(object,slot,UINT32_MAX,&result)&&!result);
 CHECK(nativeScriptFighter_Save(changed,sizeof(changed),&after)&&before==after&&!memcmp(snapshot,changed,before));
 CHECK(!nativeScriptFighter_CallbackMatches(object,slot,function_id("WithArgument"),&result));
 CHECK(!nativeScriptFighter_CallbackMatches(object,slot,function_id("BoolResult"),&result));
 CHECK(!nativeScriptFighter_CallbackMatches(object,NativeScriptFighterCallback_Count,pulse,&result));
 CHECK(!nativeScriptFighter_CallbackBound(object,slot));fighter->reflect_hit_cb=NULL;
 CHECK(nativeScriptFighter_CallbackMatches(object,slot,UINT32_MAX,&result)&&result);
 CHECK(nativeScriptFighter_BindCallback(object,slot,pulse));
 CHECK(nativeScriptFighter_Save(snapshot,sizeof(snapshot),&before));
 CHECK(nativeScriptFighter_BindCallback(object,slot,UINT32_MAX));
 CHECK(nativeScriptFighter_Load(snapshot,before));
 CHECK(nativeScriptFighter_CallbackMatches(object,slot,pulse,&result)&&result);
 ++fighter->x8_spawnNum;result=true;CHECK(!nativeScriptFighter_CallbackMatches(object,slot,pulse,&result)&&result);--fighter->x8_spawnNum;
 invoke_helper(object,"ClearContacts");
}
static void motion_lifetime(void)
{
 // Real scheduler tails stop when the reached source helper faults or removes
 // its actor. Its following source statement must not publish the +100 write.
 HSD_GObj* fault=create();invoke_helper(fault,"ArmEveryFrame");
 CHECK(nativeScriptFighter_BindMotionPhase(fault,NativeScriptMotion_Accessory,function_id("RetiringPulse"),false));
 unsigned before=fighter_accessory_tail;scheduler_fail_write=true;
 Fighter_8006C80C(fault);scheduler_fail_write=false;
 CHECK(nativeScriptFighter_Status(fault)==NativeScriptVM_Error&&fighter_accessory_tail==before);
 CHECK(state(fault,"pulses",NULL).as.i64==1);HSD_GObjFree(fault);
 HSD_GObj* retired=create();NativeMatchPoolReference reference;
 CHECK(nativeMatchPool_Reference(&nativeGObjPool,retired,&reference));
 CHECK(nativeScriptFighter_BindMotionPhase(retired,NativeScriptMotion_Accessory,function_id("RetiringPulse"),false));
 before=fighter_accessory_tail;scheduler_remove=true;Fighter_8006C80C(retired);
 CHECK(!nativeMatchPool_Resolve(reference)&&fighter_accessory_tail==before);
 CHECK(nativeScriptFighter_Status(retired)==NativeScriptVM_SourceRetired&&nativeScriptRuntime_Idle());
}
#include "input_cases.h"
#include "auxiliary_cases.h"

int main(void)
{
 prepare();HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);fighter_scheduler_legacy();HSD_GObj* object=create();Fighter* fighter=object->user_data;
 input_hook_cases(object);
 CHECK(state(object,"trace",NULL).as.i64==1);NativeScriptValue argument={.type=NativeScriptType_Bool,.as.u64=1},result;
 CHECK(nativeScriptFighter_Run(object,NativeScriptFighter_CpuCaptureDecision,NULL,0, NULL,&result)==NativeScriptVM_Ok&&result.type==NativeScriptType_Bool&&!result.as.u64);
 for(unsigned which=0;which<4;++which){reset_trace(object);CHECK(nativeScriptFighter_Run(object,NativeScriptFighter_ItemPickupExt+which,&argument,1, NULL,&result)==NativeScriptVM_Ok&&result.type==NativeScriptType_Void);}
 CHECK(nativeScriptFighter_Run(object,NativeScriptFighter_ItemPickup,NULL,0, NULL,&result)==NativeScriptVM_Error&&nativeScriptFighter_Status(object)==NativeScriptVM_Ok);
 result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=123};CHECK(nativeScriptFighter_Run(object,NativeScriptFighter_SpecialAirN,NULL,0, NULL,&result)==NativeScriptVM_Ok&&result.type==NativeScriptType_Void);NativeScriptValue invalid={.type=NativeScriptType_I32,.as.i64=1};result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=123};CHECK(nativeScriptFighter_Run(object,NativeScriptFighter_ItemPickup,&invalid,1, NULL,&result)==NativeScriptVM_Error&&result.type==NativeScriptType_U64&&result.as.u64==123&&nativeScriptFighter_Status(object)==NativeScriptVM_Ok);
 reset_trace(object);ftCommon_8007E7E4(object,2);CHECK(state(object,"trace",NULL).as.i64==12);ftCommon_8007E79C(object,0);CHECK(state(object,"trace",NULL).as.i64==125);
 HSD_GObj* held=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);item.entity=held;item.kind=It_Kind_Foods;item.x1C=51;GObj_InitUserData(held,6,item_remove,&item);fighter->item_gobj=held;
 reset_trace(object);ftpickupitem_80094818(object,true);CHECK(state(object,"trace",NULL).as.i64==16);ftCommon_8007E6DC(object,held,0);CHECK(state(object,"trace",NULL).as.i64==169&&fighter->item_gobj==NULL);
 auxiliary_item_cases(object,held);
 fighter->item_gobj=held;reset_trace(object);ftCommon_8007F5CC(object,false);CHECK(state(object,"trace",NULL).as.i64==17&&!fighter->x221E_b3);ftCommon_8007F5CC(object,true);CHECK(state(object,"trace",NULL).as.i64==176&&fighter->x221E_b3);ftCommon_8007F824(object);ftCommon_8007F86C(object);CHECK(state(object,"trace",NULL).as.i64==17689);
 reset_trace(object);ftCo_800D105C(object);CHECK(state(object,"trace",NULL).as.i64==14);ftCo_Landing_Enter(object,ftCo_MS_Landing,true,0,0,1);CHECK(state(object,"trace",NULL).as.i64==145&&fighter->mv.co.landing.allow_interrupt);
 CHECK(state(object,"landing_option",NULL).as.u64&&state(object,"landing_option_after",NULL).as.u64);
 scheduler_overwrite_landing=true;reset_trace(object);ftCo_Landing_Enter(object,ftCo_MS_Landing,true,0,0,1);
 CHECK(state(object,"trace",NULL).as.i64==15&&!fighter->mv.co.landing.allow_interrupt);
 CHECK(state(object,"landing_option",NULL).as.u64&&state(object,"landing_option_after",NULL).as.u64);
 scheduler_overwrite_landing=false;reset_trace(object);ftCo_Landing_Enter(object,ftCo_MS_Landing,false,0,0,1);
 CHECK(state(object,"trace",NULL).as.i64==15&&!state(object,"landing_option",NULL).as.u64&&!state(object,"landing_option_after",NULL).as.u64);
 NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=345},wrong_option={.type=NativeScriptType_I32,.as.i64=1};
 CHECK(nativeScriptFighter_Run(object,NativeScriptFighter_LandingAction,NULL,0, NULL,&sentinel)==NativeScriptVM_Error&&sentinel.type==NativeScriptType_U64&&sentinel.as.u64==345);
 CHECK(nativeScriptFighter_Run(object,NativeScriptFighter_LandingAction,&wrong_option,1, NULL,&sentinel)==NativeScriptVM_Error&&sentinel.type==NativeScriptType_U64&&sentinel.as.u64==345);
 CHECK(nativeScriptRuntime_Idle());
 reset_trace(object);ftCo_800B77E8(fighter);CHECK(state(object,"trace",NULL).as.i64==13);
 HSD_GObj* victim=create();fighter->victim_gobj=victim;reset_trace(object);unsigned generic=fighter_cpu_tail;ftCo_800B683C(fighter);CHECK(state(object,"trace",NULL).as.i64==12&&fighter_cpu_tail==generic+1);NativeScriptValue handled={.type=NativeScriptType_Bool,.as.u64=1};state(object,"handled",&handled);generic=fighter_cpu_tail;ftCo_800B683C(fighter);CHECK(state(object,"trace",NULL).as.i64==122&&fighter_cpu_tail==generic);CHECK(fighter_stock_calls==0);
 nativeScriptFighter_SelectMotion(object,343);reset_trace(object);Fighter_8006C80C(object);CHECK(state(object,"trace",NULL).as.i64==1);fighter->throw_flags_b0=true;Fighter_8006C80C(object);CHECK(state(object,"trace",NULL).as.i64==12&&!fighter->throw_flags_b0);Fighter_8006C80C(object);CHECK(state(object,"trace",NULL).as.i64==12);
 size_t bytes;CHECK(nativeScriptFighter_Save(snapshot,sizeof(snapshot),&bytes));memcpy(changed,snapshot,bytes);NativeScriptFighterSnapshotRecord* record=(void*)(changed+sizeof(NativeScriptFighterSnapshotHeader));record->accessory_cue=0;CHECK(!nativeScriptFighter_ValidateSnapshot(changed,bytes));CHECK(nativeScriptFighter_BindMotionPhase(object,NativeScriptMotion_Accessory,UINT32_MAX,false));fighter->throw_flags_b0=true;Fighter_8006C80C(object);CHECK(state(object,"trace",NULL).as.i64==12);CHECK(nativeScriptFighter_Load(snapshot,bytes));Fighter_8006C80C(object);CHECK(state(object,"trace",NULL).as.i64==122);
 unsigned helper=UINT32_MAX,cpu=UINT32_MAX;for(unsigned i=0;i<bank.declarations.function_count;++i){if(nativeScriptSource_SpanEquals(&bank.tree,declarations[i].name,"ResetRoot"))helper=i;if(declarations[i].callback&&bank.execution_functions[i].event_id==functions[nativeScriptFighter_Program(Ft_Kind_Mario)->hooks[NativeScriptFighter_CpuCaptureDecision]].event_id)cpu=i;}
 CHECK(helper!=UINT32_MAX&&cpu!=UINT32_MAX);CHECK(!nativeScriptFighter_BindCallback(object,NativeScriptFighterCallback_Damage,cpu));CHECK(nativeScriptFighter_BindCallback(object,NativeScriptFighterCallback_Damage,helper));CHECK(nativeScriptFighter_BindCallback(object,NativeScriptFighterCallback_Death2,helper));CHECK(nativeScriptFighter_Save(snapshot,sizeof(snapshot),&bytes));CHECK(nativeScriptFighter_RunCallback(object,NativeScriptFighterCallback_Damage)&&state(object,"trace",NULL).as.i64==0);CHECK(nativeScriptFighter_Load(snapshot,bytes));CHECK(nativeScriptFighter_CallbackBound(object,NativeScriptFighterCallback_Damage)&&nativeScriptFighter_CallbackBound(object,NativeScriptFighterCallback_Death2));CHECK(nativeScriptFighter_RunCallback(object,NativeScriptFighterCallback_Death2)&&state(object,"trace",NULL).as.i64==0);memcpy(changed,snapshot,bytes);record=(void*)(changed+sizeof(NativeScriptFighterSnapshotHeader));record->callbacks[NativeScriptFighterCallback_Damage]=cpu;CHECK(!nativeScriptFighter_ValidateSnapshot(changed,bytes));
 helper_status(object);motion_helpers(object);motion_resurrection(object);contact_callbacks(object);callback_identity(object);motion_lifetime();
 HSD_GObj* fault=create();Fighter* fault_data=fault->user_data;fault_data->item_gobj=held;scheduler_fail_write=true;unsigned bookkeeping=fighter_bookkeeping;ftCommon_8007E6DC(fault,held,true);scheduler_fail_write=false;CHECK(nativeScriptFighter_Status(fault)==NativeScriptVM_Error&&fault_data->item_gobj==held&&fighter_bookkeeping==bookkeeping);result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=123};CHECK(nativeScriptFighter_Run(fault,NativeScriptFighter_CpuCaptureDecision,NULL,0, NULL,&result)==NativeScriptVM_Error&&result.type==NativeScriptType_U64&&result.as.u64==123);
 HSD_GObj* retired=create();NativeMatchPoolReference retired_reference;CHECK(nativeMatchPool_Reference(&nativeGObjPool,retired,&retired_reference));scheduler_remove=true;CHECK(nativeScriptFighter_Helper(retired,((Fighter*)retired->user_data)->x8_spawnNum,function_id("RetiringPulse"),NULL,0,&result,&(NativeScriptDiagnostic){0})==NativeScriptVM_SourceRetired&&nativeMatchPool_Resolve(retired_reference)==NULL&&result.type==NativeScriptType_U64&&result.as.u64==123);
 retired=create();CHECK(nativeMatchPool_Reference(&nativeGObjPool,retired,&retired_reference));scheduler_remove=true;scheduler_spawn_on_write=create_replacement;CHECK(nativeScriptFighter_Helper(retired,((Fighter*)retired->user_data)->x8_spawnNum,function_id("RetiringPulse"),NULL,0,&result,&(NativeScriptDiagnostic){0})==NativeScriptVM_SourceRetired&&nativeMatchPool_Resolve(retired_reference)==NULL&&replacement&&nativeScriptFighter_Status(replacement)==NativeScriptVM_Ok&&state(replacement,"trace",NULL).as.i64==1);HSD_GObjFree(replacement);
 NativeScriptValue no={.type=NativeScriptType_Bool,.as.u64=0};state(object,"handled",&no);unsigned before_target=fighter_cpu_tail;scheduler_retire_target=victim;ftCo_800B683C(fighter);CHECK(fighter_cpu_tail==before_target&&nativeScriptFighter_Status(object)==NativeScriptVM_Ok);victim=NULL;
 static NativeScriptCompileBank unrelated;CHECK(nativeScriptFighter_ReferencesProgram(&bank)&&!nativeScriptFighter_ReferencesProgram(&unrelated));
 HSD_GObjFree(fault);HSD_GObjFree(held);HSD_GObjFree(object);CHECK(nativeScriptRuntime_Idle()&&!nativeScriptFighter_ReferencesProgram(&bank));absent_input_hooks();printf("{\"status\":\"PASS actual Fighter event scheduler\",\"checks\":%u}\n",checks);return 0;
}
