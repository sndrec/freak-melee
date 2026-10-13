#include "boundary.h"
#include "attack_boundary.h"
#include <melee/ft/native_script_binding_normal_attacks.h>
#include <melee/ft/native_script_fighter_snapshot.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_script_receiver.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/kinds/ftCommon/ftCo_Attack1.h>
#include <melee/ft/kinds/ftCommon/ftCo_AttackAir.h>
#include <melee/ft/kinds/ftCommon/ftCo_SquatWait.h>
#include <melee/ft/fighter.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum{N=4096,F=128,V=128};
static char source_bytes[65536],source[32768];static NativeScriptSourceNode syntax[N];
static NativeScriptIRNode expressions[N*2];static uint32_t arguments[N*2];
static NativeScriptFunctionDeclaration declarations[F];static NativeScriptParameterDeclaration parameters[V];
static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[64];
static NativeScriptInstruction instructions[8192];static NativeScriptFunctionCode code[F];
static NativeScriptExecutionFunction functions[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[8];static NativeScriptParameterSlot slots[V];
static NativeSourcePackage package;
#define bank package.compiler.bank
#define prepared package.program
static MotionState native_motions[410];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind){return kind==Ft_Kind_CustomFirst?&package:NULL;}
static unsigned comparisons,primitive_calls;
static NativeScriptMoveLayout layout={.common_count=341,.first=400,.count=10,.ground={402,404,406,408},.air={403,405,407,409}};
static Fighter fighters[128];static unsigned allocated,checks,cases;
#define CHECK(v) do{++checks;if(!(v)){fprintf(stderr,"Attack scheduler line%u %s\n",__LINE__,#v);exit(1);}}while(0)
static void append(const char* text){CHECK(strlen(source)+strlen(text)<sizeof(source));strcat(source,text);}
static void initialize_bank(void)
{
 bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=syntax,.capacity=N},.expressions={.nodes=expressions,.node_capacity=N*2,.arguments=arguments,.argument_capacity=N*2},.declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=64,.instructions=instructions,.instruction_capacity=8192,.function_code=code,.function_code_capacity=F,.execution_functions=functions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=8},.parameter_slots=slots,.parameter_slot_capacity=V};
}
static void remove_data(void* data){nativeScriptFighter_Detach(((Fighter*)data)->gobj);}
static void set_int(HSD_GObj*,const char*,int);
static HSD_GObj* create(void)
{
 CHECK(allocated<ARRAY_SIZE(fighters));HSD_GObj* object=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(object);
 Fighter* fighter=&fighters[allocated++];memset(fighter,0,sizeof(*fighter));fighter->gobj=object;fighter->ft_data=&package.assets.character.data;fighter->kind=Ft_Kind_CustomFirst;fighter->behavior_kind=Ft_Kind_CustomFirst;fighter->x8_spawnNum=allocated;fighter->motion_id=-777;fighter->facing_dir=1;fighter->x34_scale=(Vec3){1,1,1};fighter->co_attrs.jab_2_input_window=13;fighter->co_attrs.jab_3_input_window=17;
 GObj_InitUserData(object,4,remove_data,fighter);CHECK(nativeScriptFighter_Attach(object));set_int(object,"motion",402);return object;
}
static NativeScriptValue state(HSD_GObj* object,const char* name,const NativeScriptValue* input)
{
 NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(object,((Fighter*)object->user_data)->x8_spawnNum,&view));NativeScriptDiagnostic diagnostic={0};
 for(unsigned i=0;i<view.program->value_count;++i)if(!view.program->values[i].owner&&nativeScriptSource_SpanEquals(view.program->tree,view.program->values[i].name,name)){
  NativeScriptValue output;if(input)CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,input,&diagnostic));
  CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&output,&diagnostic));return output;
 }CHECK(false);return(NativeScriptValue){0};
}
static void set_int(HSD_GObj* object,const char* name,int value){NativeScriptValue input={.type=NativeScriptType_I32,.as.i64=value};state(object,name,&input);}


static uint32_t code_id(const char* name)
{
 for(uint32_t i=0;i<bank.declarations.function_count;++i)
  if(nativeScriptSource_SpanEquals(&bank.tree,declarations[i].name,name))return i;
 CHECK(false);return UINT32_MAX;
}
static bool invoke(HSD_GObj* object,const char* name)
{
 NativeScriptValue output;NativeScriptDiagnostic diagnostic={0};++primitive_calls;
 return nativeScriptFighter_Helper(object,((Fighter*)object->user_data)->x8_spawnNum,code_id(name),NULL,0,&output,&diagnostic)==NativeScriptVM_Ok;
}
static void prepare(void)
{
 initialize_bank();source[0]=0;
 append("fighter:\n    behavior independent\nstate int motion\nstate int finishes\nstate int tail\nstate bool result\nfunc Finish():\n    state.finishes = state.finishes + 1\n    position.x = 8\nfunc FaultFinish():\n    state.finishes = state.finishes + 1\n    position.x = 888\nfunc SnapshotFinish():\n    position.x = 889\nfunc BadBool() -> bool:\n    return true\nfunc WithArg(int value):\n    pass\nfunc Jab():\n    state.result = attack.enter_jab(state.motion)\n    state.tail = state.tail + 1\nfunc Rapid():\n    attack.enter_rapid_jab(state.motion)\n    callbacks.accessory(Finish)\n    state.tail = state.tail + 1\nfunc Pickup():\n    state.result = items.try_ground_pickup()\n    state.tail = state.tail + 1\nfunc Update():\n    attack.update_rapid_jab(Finish)\n    state.tail = state.tail + 1\nfunc UpdateFault():\n    attack.update_rapid_jab(FaultFinish)\n    state.tail = state.tail + 1\nfunc UpdateSnapshot():\n    attack.update_rapid_jab(SnapshotFinish)\nfunc Landing():\n    collision.air_aerial_landing_callback(Finish)\n    state.tail = state.tail + 1\nfunc LandingFault():\n    collision.air_aerial_landing_callback(FaultFinish)\n    state.tail = state.tail + 1\nfunc NestedFinish():\n    collision.air_aerial_landing_callback(Finish)\n    state.finishes = state.finishes + 10\nfunc NestedUpdate():\n    attack.update_rapid_jab(NestedFinish)\naction FaultDriver:\n    on tick:\n        position.x = 999\nmove Normal:\n    on jab_enter:\n        if attack.enter_jab(state.motion): callbacks.accessory(Finish)\n    on rapid_jab_enter:\n        attack.enter_rapid_jab(state.motion)\n        callbacks.accessory(Finish)\n");
 NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout};
 const NativeScriptCompileBank* accepted=NULL;NativeScriptDiagnostic diagnostic={0};
 if(!nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic)||!nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,accepted,&layout,NULL,&prepared,&diagnostic)){
  fprintf(stderr,"prepare %u:%u %s\n",diagnostic.source.line,diagnostic.source.column,diagnostic.message);exit(1);
 }
 package.address=&package;package.target=Ft_Kind_CustomFirst;package.ready=true;package.assets.ready=true;package.assets.facts.moves=layout;package.assets.motions=native_motions;
 gFtDataList[Ft_Kind_CustomFirst]=&package.assets.character.data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=native_motions;
 CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&prepared);
}
static void trace(const unsigned* values,unsigned length)
{CHECK(attack_count==length);for(unsigned i=0;i<length;++i)CHECK(attack_trace[i]==values[i]);}
#define TRACE(...) do{const unsigned expected[]={__VA_ARGS__};trace(expected,ARRAY_SIZE(expected));}while(0)
void frozen_rapid_update(HSD_GObj*,HSD_GObjEvent);
static void native_finish(HSD_GObj* object){normal_point(Normal_Finish,object);}
static void reset_native(Fighter* f,int frame,bool ready,bool again,bool cue)
{
 f->cur_anim_frame=frame;f->frame_speed_mul=1;f->mv.co.attack100.x0=ready;f->mv.co.attack100.x4=again;f->throw_flags_b3=cue;
}
static void source_primitives(void)
{
 HSD_GObj* object=create();Fighter* f=object->user_data;
 for(unsigned picked=0;picked<2;++picked){
  attack_reset();attack_pickup=picked;f->motion_id=-777;set_int(object,"tail",0);CHECK(invoke(object,"Jab"));++cases;
  CHECK(state(object,"result",NULL).as.u64==!picked&&state(object,"tail",NULL).as.i64==1);
  if(picked)TRACE(Attack_Pickup);else{TRACE(Attack_Pickup,Attack_Motion,Attack_Animation);CHECK(f->motion_id==402);}
  attack_reset();attack_pickup=picked;set_int(object,"tail",0);CHECK(invoke(object,"Rapid"));++cases;
  CHECK(state(object,"tail",NULL).as.i64==1&&nativeScriptFighter_MotionBound(object,NativeScriptMotion_Accessory));
  if(picked)TRACE(Attack_Pickup);else TRACE(Attack_Pickup,Attack_Motion,Attack_Animation);
  attack_reset();attack_pickup=picked;CHECK(invoke(object,"Pickup"));TRACE(Attack_Pickup);CHECK(state(object,"result",NULL).as.u64==picked);++cases;
 }
 // Actual scheduler -> source entry -> catalog -> common helper -> accessory.
 attack_reset();attack_pickup=true;fn_800D6AC4(object);TRACE(Attack_Pickup);CHECK(nativeScriptFighter_MotionBound(object,NativeScriptMotion_Accessory));++cases;
 attack_reset();attack_pickup=true;CHECK(nativeScriptFighter_BindMotionPhase(object,NativeScriptMotion_Accessory,UINT32_MAX,false));Attack1_decideAttack11(object);TRACE(Attack_Pickup);CHECK(!nativeScriptFighter_MotionBound(object,NativeScriptMotion_Accessory));++cases;
 for(unsigned contact=0;contact<2;++contact)for(unsigned interrupted=0;interrupted<2;++interrupted){
  attack_reset();normal_contact=contact;normal_interrupted=interrupted;set_int(object,"finishes",0);CHECK(invoke(object,"Landing"));++cases;
  if(contact&&!interrupted)TRACE(Normal_Geometry,Normal_Response,Normal_Finish);else TRACE(Normal_Geometry,Normal_Response);
  CHECK(state(object,"finishes",NULL).as.i64==(contact&&!interrupted));
 }
 attack_reset();normal_contact=true;reset_native(f,2,true,false,true);set_int(object,"finishes",0);CHECK(invoke(object,"NestedUpdate"));TRACE(Normal_Geometry,Normal_Response,Normal_Finish);CHECK(state(object,"finishes",NULL).as.i64==11);++cases;
 HSD_GObjFree(object);
}
static void rapid_comparison(void)
{
 HSD_GObj* object=create();Fighter* f=object->user_data;
 for(int frame=-1;frame<=2;++frame)for(unsigned ready=0;ready<2;++ready)for(unsigned again=0;again<2;++again)for(unsigned cue=0;cue<2;++cue)for(unsigned picked=0;picked<2;++picked){
  reset_native(f,frame,ready,again,cue);attack_reset();attack_pickup=picked;frozen_rapid_update(object,native_finish);
  unsigned events[64],length=attack_count;memcpy(events,attack_trace,sizeof(events));bool x0=f->mv.co.attack100.x0,x4=f->mv.co.attack100.x4,b3=f->throw_flags_b3;
  reset_native(f,frame,ready,again,cue);attack_reset();attack_pickup=picked;set_int(object,"finishes",0);CHECK(invoke(object,"Update"));
  trace(events,length);CHECK(f->mv.co.attack100.x0==x0&&f->mv.co.attack100.x4==x4&&f->throw_flags_b3==b3);
  bool finished=false;for(unsigned i=0;i<length;++i)finished|=events[i]==Normal_Finish;
  CHECK(state(object,"finishes",NULL).as.i64==finished);++comparisons;
 }
 HSD_GObjFree(object);
}
static NativeScriptRuntimeContext context(HSD_GObj* object)
{NativeScriptRuntimeContext c;NativeScriptDiagnostic d={0};CHECK(nativeScriptBindings_Context(object,NativeCallbackSource_Fighter,((Fighter*)object->user_data)->x8_spawnNum,&c,&d));c.scene_epoch=nativeGObjPool.lifetime;return c;}
static void rejected_inputs(void)
{
 HSD_GObj* object=create();NativeScriptRuntimeContext c=context(object);NativeScriptDiagnostic d={0};
 NativeScriptValue output={.type=NativeScriptType_U64,.as.u64=UINT64_C(0xDEADFACE12345678)},before=output;
 const int motions[]={-1,341,399,410,INT32_MAX,INT32_MIN};
 for(unsigned i=0;i<ARRAY_SIZE(motions);++i){NativeScriptValue arg={.type=NativeScriptType_I32,.as.i64=motions[i]};attack_reset();CHECK(!nativeScriptBindingNormalAttacks_Invoke(&c,NativeScriptBinding_AttackEnterJab,&arg,1,&output,&d));CHECK(!attack_count&&!memcmp(&output,&before,sizeof(output)));++cases;}
 uint32_t invalid[]={code_id("BadBool"),code_id("WithArg"),prepared.hooks[NativeScriptFighter_JabEnter],bank.declarations.function_count,UINT32_MAX};
 for(unsigned i=0;i<ARRAY_SIZE(invalid);++i){NativeScriptValue arg={.type=NativeScriptType_Callable,.as.u64=invalid[i]};attack_reset();CHECK(!nativeScriptBindingNormalAttacks_Invoke(&c,NativeScriptBinding_AttackUpdateRapidJab,&arg,1,&output,&d));CHECK(!attack_count&&!memcmp(&output,&before,sizeof(output)));++cases;}
 NativeScriptValue none={.type=NativeScriptType_Null};CHECK(!nativeScriptBindingNormalAttacks_Invoke(&c,NativeScriptBinding_CollisionAirAerialLandingCallback,&none,1,&output,&d));CHECK(!memcmp(&output,&before,sizeof(output)));++cases;
 NativeScriptValue valid={.type=NativeScriptType_I32,.as.i64=402};NativeScriptRuntimeContext bad=c;++bad.generation;CHECK(!nativeScriptBindingNormalAttacks_Invoke(&bad,NativeScriptBinding_AttackEnterJab,&valid,1,&output,&d));
 bad=c;++bad.scene_epoch;CHECK(!nativeScriptBindingNormalAttacks_Invoke(&bad,NativeScriptBinding_AttackEnterJab,&valid,1,&output,&d));CHECK(!memcmp(&output,&before,sizeof(output)));++cases;
 package.assets.facts.moves.first++;CHECK(!nativeScriptBindingNormalAttacks_Invoke(&c,NativeScriptBinding_AttackEnterJab,&valid,1,&output,&d));package.assets.facts.moves.first--;CHECK(!memcmp(&output,&before,sizeof(output)));++cases;
 HSD_GObjFree(object);
}
static void withheld_outputs(void)
{
 const unsigned operations[]={NativeScriptBinding_AttackEnterJab,NativeScriptBinding_ItemsTryGroundPickup,NativeScriptBinding_CollisionAirAerialLandingCallback};
 const unsigned stages[]={Attack_Motion,Attack_Pickup,Normal_Geometry};
 for(unsigned i=0;i<3;++i){
  HSD_GObj* object=create();NativeScriptRuntimeContext c=context(object);NativeScriptDiagnostic d={0};
  NativeScriptValue argument={.type=i==2?NativeScriptType_Callable:NativeScriptType_I32,.as.u64=i==2?code_id("Finish"):402};
  NativeScriptValue output={.type=NativeScriptType_U64,.as.u64=UINT64_C(0xABCD1234FEDC9876)},before=output;
  attack_reset();normal_contact=true;attack_stop=stages[i];attack_failure=2;
  CHECK(!nativeScriptBindingNormalAttacks_Invoke(&c,operations[i],&argument,i==1?0:1,&output,&d));
  CHECK(!memcmp(&output,&before,sizeof(output))&&nativeScriptFighter_Status(object)==NativeScriptVM_Error);
  if(i==2)CHECK(((Fighter*)object->user_data)->cur_pos.x==0); // no stale post-geometry write
  HSD_GObjFree(object);++cases;
 }
}
static void interruptions(void)
{
 const char* names[]={"Jab","Rapid","Pickup","Update","Landing"};
 for(unsigned op=0;op<5;++op)for(unsigned stage=1;stage<=Normal_Response;++stage){
  bool reached=(op<2&&(stage==Attack_Pickup||stage==Attack_Motion||stage==Attack_Animation))||(op==2&&stage==Attack_Pickup)||(op==3&&(stage==Normal_Recount||stage==Normal_HitReset||stage==Normal_Finish))||(op==4&&(stage==Normal_Geometry||stage==Normal_Response||stage==Normal_Finish));
  if(!reached)continue;
  for(unsigned failure=1;failure<=2;++failure){HSD_GObj* object=create();Fighter* f=object->user_data;reset_native(f,0,true,false,true);NativeMatchPoolReference reference;CHECK(nativeMatchPool_Reference(&nativeGObjPool,object,&reference));
   attack_reset();normal_contact=true;attack_stop=stage;attack_failure=failure;set_int(object,"tail",0);CHECK(!invoke(object,names[op]));++cases;
   CHECK(attack_trace[attack_count-1]==stage);
   if(failure==1)CHECK(!nativeMatchPool_Resolve(reference));else{CHECK(nativeScriptFighter_Status(object)==NativeScriptVM_Error&&state(object,"tail",NULL).as.i64==0);HSD_GObjFree(object);}
   CHECK(nativeScriptRuntime_Idle());
  }
 }
}
static uint8_t snapshot[65536];static unsigned snapshot_during_rejected;
void normal_snapshot_inside(void)
{size_t size=999;if(!nativeScriptFighter_Save(snapshot,sizeof(snapshot),&size))++snapshot_during_rejected;else abort();}
static void snapshots(void)
{
 HSD_GObj* object=create();Fighter* f=object->user_data;size_t bytes;reset_native(f,0,true,false,true);set_int(object,"finishes",3);
 CHECK(nativeScriptFighter_Save(snapshot,sizeof(snapshot),&bytes));Fighter peer=*f;
 attack_reset();CHECK(invoke(object,"Update"));CHECK(state(object,"finishes",NULL).as.i64==4);
 *f=peer;CHECK(nativeScriptFighter_Load(snapshot,bytes));CHECK(state(object,"finishes",NULL).as.i64==3);
 attack_reset();CHECK(invoke(object,"Update"));CHECK(state(object,"finishes",NULL).as.i64==4);++cases;
 attack_reset();reset_native(f,2,true,false,true);CHECK(invoke(object,"UpdateSnapshot"));CHECK(snapshot_during_rejected==1&&nativeScriptRuntime_Idle());
 CHECK(nativeScriptFighter_Save(snapshot,sizeof(snapshot),&bytes));HSD_GObjFree(object);++cases;
}
int main(void)
{
 prepare();HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);
 source_primitives();rapid_comparison();rejected_inputs();withheld_outputs();interruptions();snapshots();CHECK(nativeScriptRuntime_Idle()&&!nativeScriptFighter_ReferencesProgram(&bank));
 printf("{\"status\":\"PASS connected normal attack primitives\",\"checks\":%u,\"cases\":%u,\"native_pairs\":%u,\"source_invocations\":%u}\n",checks,cases,comparisons,primitive_calls);return 0;
}
