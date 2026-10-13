#include "boundary.h"
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_script_receiver.h>
#include <melee/ft/fighter.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""

// Independent frozen expectations: 28 original slots, plus 25 new slots.
// The two original CPU slots make 27 CPU contracts overall.
typedef struct Expected { const char* name;uint8_t result,count,args[2]; } Expected;
#define V(name) {name,NativeScriptType_Void,0,{0}}
#define B(name) {name,NativeScriptType_Bool,0,{0}}
#define I(name) {name,NativeScriptType_I32,0,{0}}
#define FLAG(name) {name,NativeScriptType_Void,1,{NativeScriptType_Bool}}
static const Expected expected[]={
 V("load"),V("tick"),V("user_data_remove"),V("death"),V("late_update"),
 V("special_n"),V("special_s"),V("special_hi"),V("special_lw"),
 V("special_air_n"),V("special_air_s"),V("special_air_hi"),V("special_air_lw"),
 B("cpu_capture_decision"),V("cpu_ranged_candidates"),V("load_special_attrs"),V("landing_action"),
 FLAG("item_pickup_ext"),FLAG("item_drop_ext"),FLAG("item_pickup"),FLAG("item_drop"),
 V("item_visible"),V("item_invisible"),V("knockback_enter"),V("knockback_exit"),V("landing"),V("aerial_jump"),V("form_activated"),
 V("cpu_initialize"),B("cpu_mode_update"),B("cpu_output"),
 {"cpu_prepare",NativeScriptType_I32,1,{NativeScriptType_I32}},
 B("cpu_drop_command"),I("cpu_stick_step"),I("cpu_stick_limit"),B("cpu_pending_action"),
 {"cpu_spacing_update",NativeScriptType_Void,1,{NativeScriptType_F32}},
 B("cpu_attack_motion"),B("cpu_grab_motion"),B("cpu_attack_decision"),B("cpu_travel_decision"),
 B("cpu_falling_recovery_allowed"),B("cpu_falling_destination_reached"),B("cpu_available_jump"),
 {"cpu_projectile_defense",NativeScriptType_Bool,1,{NativeScriptType_I32}},
 B("cpu_pummel_special"),B("cpu_ranged_allowed"),
 {"cpu_special_candidate",NativeScriptType_Bool,2,{NativeScriptType_F32,NativeScriptType_F32}},
 {"cpu_projectile_hit",NativeScriptType_I32,2,{NativeScriptType_I32,NativeScriptType_Bool}},
 {"cpu_threat",NativeScriptType_Bool,1,{NativeScriptType_Object}},
 B("cpu_charge_decision"),V("cpu_charge_cancel"),V("cpu_recovery")
};
#undef V
#undef B
#undef I
#undef FLAG
enum{N=4096,F=128,V=128};
static char source_bytes[65536],source[32768];static NativeScriptSourceNode syntax[N];
static NativeScriptIRNode expressions[N*2];static uint32_t arguments[N*2];
static NativeScriptFunctionDeclaration declarations[F];static NativeScriptParameterDeclaration parameters[V];
static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[64];
static NativeScriptInstruction instructions[8192];static NativeScriptFunctionCode code[F];
static NativeScriptExecutionFunction functions[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[8];static NativeScriptParameterSlot slots[V];
static NativeScriptCompileBank bank;static NativeScriptFighterProgram program,rejected;
static NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=10,.ground={343,345,347,349},.air={344,346,348,350}};
static HSD_GObj* generation_target;
static HSD_GObj* advance_generation(void){Fighter* f=generation_target->user_data;++f->x8_spawnNum;nativeScriptFighter_NewLife(generation_target,f->x8_spawnNum);return generation_target;}
static Fighter fighters[16];static unsigned allocated,checks,cpu_events,old_events,negative_cases;
#define CHECK(v) do{++checks;if(!(v)){fprintf(stderr,"CPU ABI line%u %s\n",__LINE__,#v);exit(1);}}while(0)

static void append(const char* text){CHECK(strlen(source)+strlen(text)<sizeof(source));strcat(source,text);}
static void initialize_bank(void)
{
 bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=syntax,.capacity=N},.expressions={.nodes=expressions,.node_capacity=N*2,.arguments=arguments,.argument_capacity=N*2},.declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=64,.instructions=instructions,.instruction_capacity=8192,.function_code=code,.function_code_capacity=F,.execution_functions=functions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=8},.parameter_slots=slots,.parameter_slot_capacity=V};
}
static void prepare(bool full)
{
 initialize_bank();source[0]=0;
 append("fighter:\n    behavior independent\nstate int tag\nstate int calls\nstate int seen_int\nstate float seen_float\nstate float seen_second\nstate bool seen_flag\nstate bool present\nstate int other_tag\nstate int tail\n");
 if(full)for(unsigned hook=0;hook<ARRAY_SIZE(expected);++hook){
  char line[256];snprintf(line,sizeof(line),"%s Hook%u:\n    on %s:\n        state.tag = %u\n        state.calls = state.calls + 1\n",(hook>=5&&hook<=12)?"move":"action",hook,expected[hook].name,hook+1);append(line);
  if(expected[hook].count==1&&expected[hook].args[0]==NativeScriptType_Bool)append("        state.seen_flag = event.flag\n");
  if(hook==NativeScriptFighter_CpuPrepare)append("        state.seen_int = event.history_source\n        return event.history_source\n");
  else if(hook==NativeScriptFighter_CpuProjectileHit)append("        state.seen_int = event.prediction_frames\n        state.seen_flag = event.unblockable\n        return event.prediction_frames\n");
  else if(hook==NativeScriptFighter_CpuProjectileDefense)append("        state.seen_int = event.category\n        return true\n");
  else if(hook==NativeScriptFighter_CpuSpacingUpdate)append("        state.seen_float = event.value\n");
  else if(hook==NativeScriptFighter_CpuSpecialCandidate)append("        state.seen_float = event.angle\n        state.seen_second = event.distance\n        return true\n");
  else if(hook==NativeScriptFighter_CpuThreat)append("        state.present = other.exists\n        state.other_tag = 0\n        if other.exists:\n            state.other_tag = other.state.tag\n            position.x = 0\n            state.present = other.exists\n        return state.present\n");
  else if(hook==NativeScriptFighter_CpuRecovery)append("        position.x = 1\n        state.tail = state.tail + 1\n");
  else if(expected[hook].result==NativeScriptType_Bool)append("        return true\n");
  else if(expected[hook].result==NativeScriptType_I32){snprintf(line,sizeof(line),"        return %u\n",hook+100);append(line);}
 }
 NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout};
 const NativeScriptCompileBank* accepted=NULL;NativeScriptDiagnostic diagnostic={0};
 if(!nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic)||!nativeScriptFighter_PrepareProgram(Ft_Kind_Mario,accepted,&layout,NULL,&program,&diagnostic)){
  fprintf(stderr,"prepare %u:%u %s\n",diagnostic.source.line,diagnostic.source.column,diagnostic.message);exit(1);
 }
 CHECK(nativeScriptFighter_CanPublish(Ft_Kind_Mario));nativeScriptFighter_PublishProgram(Ft_Kind_Mario,&program);
}
static void remove_data(void* data){nativeScriptFighter_Detach(((Fighter*)data)->gobj);}
static HSD_GObj* create(void)
{
 CHECK(allocated<ARRAY_SIZE(fighters));HSD_GObj* object=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(object);
 Fighter* fighter=&fighters[allocated++];memset(fighter,0,sizeof(*fighter));fighter->gobj=object;fighter->kind=Ft_Kind_Mario;fighter->behavior_kind=Ft_Kind_Mario;fighter->x8_spawnNum=allocated;
 GObj_InitUserData(object,4,remove_data,fighter);CHECK(nativeScriptFighter_Attach(object));return object;
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
static NativeScriptValue sentinel(void){return(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0xFEED0123CAFEBEEF)};}
static NativeScriptValue object_value(HSD_GObj* object)
{
 NativeScriptValue value=sentinel();NativeScriptDiagnostic diagnostic={0};
 CHECK(nativeScriptReceiver_CaptureExternal(object,NativeCallbackSource_Fighter,nativeGObjPool.lifetime,&value,&diagnostic));return value;
}
static void valid_arguments(unsigned hook,HSD_GObj* other,NativeScriptValue* args)
{
 for(unsigned i=0;i<expected[hook].count;++i){args[i]=(NativeScriptValue){.type=expected[hook].args[i]};
  switch(args[i].type){case NativeScriptType_I32:args[i].as.i64=-1234567;break;case NativeScriptType_F32:args[i].as.f32=i?-27.25f:1.125f;break;
  case NativeScriptType_Bool:args[i].as.u64=1;break;case NativeScriptType_Object:args[i]=object_value(other);break;default:CHECK(false);}
 }
}
static void all_contracts(HSD_GObj* caller,HSD_GObj* other)
{
 CHECK(ARRAY_SIZE(expected)==53&&NativeScriptFighter_CpuRecovery+1==53);set_int(other,"tag",777);
 for(unsigned hook=0;hook<ARRAY_SIZE(expected);++hook){
  const Expected* e=&expected[hook];const NativeScriptFighterHookContract* actual=&nativeScriptFighterHookContracts[hook];
  CHECK(!strcmp(actual->name,e->name)&&actual->result_type==e->result&&actual->argument_count==e->count);
  for(unsigned i=0;i<e->count;++i)CHECK(actual->arguments[i]==e->args[i]);
  CHECK(nativeScriptFighter_HasHook(caller,hook));NativeScriptValue args[3]={0},result=sentinel();valid_arguments(hook,other,args);
  int before=state(caller,"calls",NULL).as.i64;
  CHECK(nativeScriptFighter_Run(caller,hook,args,e->count, NULL,&result)==NativeScriptVM_Ok);
  CHECK(state(caller,"tag",NULL).as.i64==hook+1&&state(caller,"calls",NULL).as.i64==before+1&&result.type==e->result);
  if(e->result==NativeScriptType_Bool)CHECK(result.as.u64==1);
  if(e->result==NativeScriptType_I32)CHECK(result.as.i64==(e->count?-1234567:(int)hook+100));
  if(hook==NativeScriptFighter_CpuSpacingUpdate)CHECK(state(caller,"seen_float",NULL).as.f32==1.125f);
  if(hook==NativeScriptFighter_CpuSpecialCandidate)CHECK(state(caller,"seen_float",NULL).as.f32==1.125f&&state(caller,"seen_second",NULL).as.f32==-27.25f);
  if(hook==NativeScriptFighter_CpuThreat)CHECK(state(caller,"present",NULL).as.u64&&state(caller,"other_tag",NULL).as.i64==777);
  if(!strncmp(e->name,"cpu_",4))++cpu_events;if(hook<28)++old_events;
  // Shape/type failures happen before source execution and preserve output.
  before=state(caller,"calls",NULL).as.i64;result=sentinel();NativeScriptValue unchanged=result;
  CHECK(nativeScriptFighter_Run(caller,hook,args,e->count+1, NULL,&result)==NativeScriptVM_Error);
  CHECK(!memcmp(&result,&unchanged,sizeof(result))&&state(caller,"calls",NULL).as.i64==before&&nativeScriptFighter_Status(caller)==NativeScriptVM_Ok);++negative_cases;
  if(e->count){
   CHECK(nativeScriptFighter_Run(caller,hook,NULL,e->count, NULL,&result)==NativeScriptVM_Error);++negative_cases;
   for(unsigned i=0;i<e->count;++i){NativeScriptValue bad[2]={args[0],args[1]};bad[i].type=NativeScriptType_U64;
    CHECK(nativeScriptFighter_Run(caller,hook,bad,e->count, NULL,&result)==NativeScriptVM_Error);
    CHECK(!memcmp(&result,&unchanged,sizeof(result))&&nativeScriptFighter_Status(caller)==NativeScriptVM_Ok);++negative_cases;
   }
  }
 }
 CHECK(cpu_events==27&&old_events==28);
}
static void preparation_rejections(void)
{
 static unsigned char previous[sizeof(rejected)];memset(&rejected,0xA5,sizeof(rejected));memcpy(previous,&rejected,sizeof(rejected));
 for(unsigned hook=0;hook<ARRAY_SIZE(expected);++hook){
  uint32_t function=program.hooks[hook];CHECK(function<bank.declarations.function_count);NativeScriptFunctionDeclaration saved=declarations[function];
  declarations[function].result_type=NativeScriptType_U64;NativeScriptDiagnostic diagnostic={0};
  CHECK(!nativeScriptFighter_PrepareProgram(Ft_Kind_Mario,&bank,&layout,NULL,&rejected,&diagnostic));
  CHECK(!memcmp(previous,&rejected,sizeof(rejected)));declarations[function]=saved;++negative_cases;
  declarations[function].parameter_count=saved.parameter_count+1;
  CHECK(!nativeScriptFighter_PrepareProgram(Ft_Kind_Mario,&bank,&layout,NULL,&rejected,&diagnostic));
  CHECK(!memcmp(previous,&rejected,sizeof(rejected)));declarations[function]=saved;++negative_cases;
  for(unsigned i=0;i<saved.parameter_count;++i){NativeScriptParameterDeclaration* parameter=&parameters[saved.parameter_first+i];NativeScriptIRType type=parameter->type;
   parameter->type=NativeScriptType_U64;CHECK(!nativeScriptFighter_PrepareProgram(Ft_Kind_Mario,&bank,&layout,NULL,&rejected,&diagnostic));
   CHECK(!memcmp(previous,&rejected,sizeof(rejected)));parameter->type=type;++negative_cases;
  }
 }
}
static void history_bits(HSD_GObj* caller)
{
 const uint32_t bits[]={0,1,INT32_MAX,UINT32_C(0x80000000),UINT32_MAX,UINT32_C(0x87654321)};
 for(unsigned i=0;i<ARRAY_SIZE(bits);++i){
  // Exact native ftCo_800A101C scheduler representation, including high bits.
  NativeScriptValue argument={.type=NativeScriptType_I32,.as.i64=(int32_t)bits[i]},result=sentinel();
  CHECK(nativeScriptFighter_Run(caller,NativeScriptFighter_CpuPrepare,&argument,1, NULL,&result)==NativeScriptVM_Ok);
  CHECK(result.type==NativeScriptType_I32&&(uint32_t)(int32_t)result.as.i64==bits[i]);
  CHECK((uint32_t)(int32_t)state(caller,"seen_int",NULL).as.i64==bits[i]);
 }
}
static void object_and_lifetime(HSD_GObj* caller,HSD_GObj* other)
{
 NativeScriptValue missing=object_value(NULL),result=sentinel();
 CHECK(nativeScriptFighter_Run(caller,NativeScriptFighter_CpuThreat,&missing,1, NULL,&result)==NativeScriptVM_Ok&&result.type==NativeScriptType_Bool&&!result.as.u64);
 CHECK(!state(caller,"present",NULL).as.u64&&!state(caller,"other_tag",NULL).as.i64);
 NativeScriptValue token=object_value(other);generation_target=other;scheduler_spawn_on_write=advance_generation;result=sentinel();
 CHECK(nativeScriptFighter_Run(caller,NativeScriptFighter_CpuThreat,&token,1, NULL,&result)==NativeScriptVM_Ok&&!result.as.u64);
 CHECK(nativeScriptFighter_Status(other)==NativeScriptVM_Ok);
 token=object_value(other);scheduler_retire_target=other;result=sentinel();
 CHECK(nativeScriptFighter_Run(caller,NativeScriptFighter_CpuThreat,&token,1, NULL,&result)==NativeScriptVM_Ok&&!result.as.u64&&nativeScriptFighter_Status(caller)==NativeScriptVM_Ok);
 HSD_GObj* replacement=create();CHECK(replacement==other);set_int(replacement,"tag",999);
 HSD_GObj* malformed=create();NativeScriptValue unchanged=sentinel();result=unchanged;
 CHECK(nativeScriptFighter_Run(malformed,NativeScriptFighter_CpuThreat,&token,1, NULL,&result)==NativeScriptVM_Error);
 CHECK(!memcmp(&result,&unchanged,sizeof(result))&&nativeScriptFighter_Status(malformed)==NativeScriptVM_Error);
 HSD_GObjFree(malformed);
 token=object_value(replacement);result=sentinel();
 CHECK(nativeScriptFighter_Run(caller,NativeScriptFighter_CpuThreat,&token,1, NULL,&result)==NativeScriptVM_Ok&&result.as.u64&&state(caller,"other_tag",NULL).as.i64==999);
 HSD_GObjFree(replacement);
 HSD_GObj* fault=create();scheduler_fail_write=true;result=unchanged;
 CHECK(nativeScriptFighter_Run(fault,NativeScriptFighter_CpuRecovery,NULL,0, NULL,&result)==NativeScriptVM_Error);scheduler_fail_write=false;
 CHECK(!memcmp(&result,&unchanged,sizeof(result))&&state(fault,"tail",NULL).as.i64==0&&nativeScriptFighter_Status(fault)==NativeScriptVM_Error);HSD_GObjFree(fault);
 HSD_GObj* retiring=create();NativeMatchPoolReference reference;CHECK(nativeMatchPool_Reference(&nativeGObjPool,retiring,&reference));scheduler_remove=true;result=unchanged;
 CHECK(nativeScriptFighter_Run(retiring,NativeScriptFighter_CpuRecovery,NULL,0, NULL,&result)==NativeScriptVM_SourceRetired);
 CHECK(!memcmp(&result,&unchanged,sizeof(result))&&!nativeMatchPool_Resolve(reference)&&nativeScriptRuntime_Idle());
}
static void absent(void)
{
 prepare(false);HSD_GObj* caller=create();
 for(unsigned hook=0;hook<ARRAY_SIZE(expected);++hook){NativeScriptValue args[2]={0},result=sentinel();valid_arguments(hook,NULL,args);
  CHECK(!nativeScriptFighter_HasHook(caller,hook));
  CHECK(nativeScriptFighter_Run(caller,hook,args,expected[hook].count, NULL,&result)==NativeScriptVM_Ok&&result.type==expected[hook].result);
  if(result.type==NativeScriptType_Bool||result.type==NativeScriptType_I32)CHECK(result.as.u64==0);
  CHECK(state(caller,"calls",NULL).as.i64==0);
  result=sentinel();NativeScriptValue unchanged=result;
  CHECK(nativeScriptFighter_Run(caller,hook,args,expected[hook].count+1, NULL,&result)==NativeScriptVM_Error);
  CHECK(!memcmp(&result,&unchanged,sizeof(result))&&nativeScriptFighter_Status(caller)==NativeScriptVM_Ok);++negative_cases;
 }
 CHECK(!nativeScriptFighter_HasHook(caller,UINT32_MAX));NativeScriptValue output=sentinel(),unchanged=output;
 CHECK(nativeScriptFighter_Run(caller,NativeScriptFighter_HookCount,NULL,0, NULL,&output)==NativeScriptVM_Error&&!memcmp(&output,&unchanged,sizeof(output)));
 HSD_GObjFree(caller);CHECK(!nativeScriptFighter_ReferencesProgram(&bank)&&nativeScriptRuntime_Idle());
}
int main(void)
{
 prepare(true);preparation_rejections();HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);
 HSD_GObj* caller=create(),*other=create();all_contracts(caller,other);history_bits(caller);object_and_lifetime(caller,other);HSD_GObjFree(caller);absent();
 printf("{\"status\":\"PASS connected CPU hook ABI\",\"checks\":%u,\"cpu_hooks\":%u,\"original_contracts\":%u,\"negative_cases\":%u}\n",checks,cpu_events,old_events,negative_cases);return 0;
}
