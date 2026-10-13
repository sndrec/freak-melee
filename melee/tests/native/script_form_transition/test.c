#include "fixture.h"
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_script_binding_transition.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { N=256,I=1024,F=8,P=32,V=4 };
static char source_storage[4096];static NativeScriptSourceNode nodes[N];
static NativeScriptIRNode ir[I];static uint32_t ir_args[I];
static NativeScriptFunctionDeclaration declarations[F];static NativeScriptParameterDeclaration parameters[P];
static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[16];
static NativeScriptInstruction instructions[I];static NativeScriptFunctionCode code[F];
static NativeScriptExecutionFunction entries[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[V];static NativeScriptParameterSlot slots[V];
static uint8_t bytes[32];static NativeScriptExecution execution;static NativeScriptDiagnostic diagnostic;
static unsigned checks,pairs,vm_runs;static uint32_t apply,begin;
#define CHECK(v)do{++checks;if(!(v)){fprintf(stderr,"transition line %u: %s (%s)\n",__LINE__,#v,diagnostic.message);exit(1);}}while(0)
static NativeScriptMoveLayout layout(void)
{return(NativeScriptMoveLayout){.common_count=341,.first=341,.count=8,.ground={341,343,345,347},.air={342,344,346,348}};}
static uint32_t id(const char* name)
{
 for(uint32_t i=0;i<package.compiler.bank.declarations.function_count;++i)
  if(nativeScriptSource_SpanEquals(&package.compiler.bank.tree,declarations[i].name,name))return i;
 CHECK(false);return UINT32_MAX;
}
static void prepare(void)
{
 NativeScriptCompileBank* bank=&package.compiler.bank;
 *bank=(NativeScriptCompileBank){.source_storage=source_storage,.source_capacity=sizeof(source_storage),
 .tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=I,.arguments=ir_args,.argument_capacity=I},
 .declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=P,.values=values,.value_capacity=V},
 .locals=locals,.local_capacity=16,.instructions=instructions,.instruction_capacity=I,.function_code=code,.function_code_capacity=F,
 .execution_functions=entries,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,
 .storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=V},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* source="fighter:\n    behavior independent\nstate int completed\nfunc Apply(int motion, uint flags, float frame, float rate):\n    state.enter_at_speed(motion, flags, frame, rate)\n    state.completed = state.completed + 1\nfunc Begin(int motion):\n    state.begin(motion)\n    state.completed = state.completed + 1\n";
 NativeScriptMoveLayout motions=layout();NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&motions};
 const NativeScriptCompileBank* accepted=NULL;
 CHECK(nativeScriptCompile_Build(bank,&request,&accepted,&diagnostic)&&accepted==bank);
 CHECK(bank->valid&&bank->declarations.function_count==2);apply=id("Apply");begin=id("Begin");
 const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(NativeScriptBinding_StateEnterAtSpeed);
 CHECK(descriptor&&descriptor->argument_count==4&&descriptor->result_type==NativeScriptType_Void);
 CHECK(descriptor->arguments[0].numeric_any&&descriptor->arguments[1].type==NativeScriptType_U32&&
       descriptor->arguments[2].type==NativeScriptType_F32&&descriptor->arguments[3].type==NativeScriptType_F32);
}
static void setup(void)
{
 package.compiler.bank.valid=true;package.compiler.bank.has_motion_layout=true;package.compiler.bank.motion_layout=layout();
 fixture_select();
 CHECK(nativeScriptStorage_Create(&package.compiler.bank.storage_layout,&package.compiler.bank.storage_parameters,
     0,17,17,bytes,sizeof(bytes),&storage,&diagnostic));
 execution=(NativeScriptExecution){.declarations=&package.compiler.bank.declarations,.code=&package.compiler.bank.code,
     .storage=&storage,.vm=nativeScriptRuntime_VM(),.payloads=nativeScriptRuntime_Payloads(),
     .functions=entries,.function_count=2};
 CHECK(nativeScriptExecution_Bind(&execution,&diagnostic));
}
static NativeScriptRuntimeContext context(void)
{return(NativeScriptRuntimeContext){.source=&object,.source_class=NativeCallbackSource_Fighter,.generation=17,.actor_id=17,.frame=123};}
static NativeScriptValue sentinel(void)
{return(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0xabcdef9876543210)};}
static void unchanged(const NativeScriptValue* output)
{CHECK(output->type==NativeScriptType_U64&&output->as.u64==UINT64_C(0xabcdef9876543210));}
static uint32_t bits(float value){uint32_t word;memcpy(&word,&value,4);return word;}
static float float_bits(uint32_t value){float number;memcpy(&number,&value,4);return number;}
static void args(NativeScriptValue output[4],uint32_t motion,uint32_t flags,uint32_t frame,uint32_t rate)
{
 output[0]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=motion};
 output[1]=(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=flags};
 output[2]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=float_bits(frame)};
 output[3]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=float_bits(rate)};
}
static bool direct(NativeScriptValue inputs[4],NativeScriptValue* result)
{
 NativeScriptRuntimeContext caller=context();
 return nativeScriptBindings_Invoke(&caller,NativeScriptBinding_StateEnterAtSpeed,NULL,0,inputs,4,result,&diagnostic);
}
static NativeScriptVMStatus run(uint32_t function,const NativeScriptValue* inputs,uint32_t count,NativeScriptValue* result)
{
 ++vm_runs;return nativeScriptExecution_Run(&execution,&object,NativeCallbackSource_Fighter,17,123,0,function,inputs,count, NULL,result);
}
static int64_t completed(void)
{
 NativeScriptValue value;CHECK(nativeScriptStorage_Read(&storage,NATIVE_SCRIPT_STATE_SEMANTIC,NULL,0,&value,&diagnostic));
 CHECK(value.type==NativeScriptType_I32);return value.as.i64;
}
static int32_t completed_bytes(void)
{int32_t value;memcpy(&value,bytes+fields[0].offset,4);return value;}
static void arguments_match(uint32_t motion,uint32_t flags,uint32_t frame,uint32_t rate)
{
 CHECK(change_count==1&&change.object==&object&&change.target==NULL);
 CHECK((uint32_t)change.motion==motion&&(uint32_t)change.flags==flags);
 CHECK(bits(change.frame)==frame&&bits(change.rate)==rate&&bits(change.blend)==0);
 CHECK(order_count==1&&order[0]==1&&start_count==0);
 CHECK(fighter.motion_id==(FtMotionId)motion&&bits(fighter.cur_anim_frame)==frame&&bits(fighter.frame_speed_mul)==rate);
}
static void numeric_cases(void)
{
 const uint32_t floats[]={0,UINT32_C(0x80000000),UINT32_C(0x3f800000),UINT32_C(0xbf800000),
    UINT32_C(0x3f000000),UINT32_C(0xc0600000),UINT32_C(0x41140000),UINT32_C(0x3f800001),
    UINT32_C(0x00000001),UINT32_C(0x80000001),UINT32_C(0x00800000)};
 const uint32_t flags[]={0,1,UINT32_C(0x80000000),UINT32_C(0x80004000),UINT32_C(0xa5a55a5a),UINT32_MAX};
 const uint32_t motions[]={0,340,341,344,348};
 for(unsigned m=0;m<5;++m)for(unsigned f=0;f<6;++f)
 for(unsigned frame=0;frame<11;++frame)for(unsigned rate=0;rate<11;++rate){
  NativeScriptValue inputs[4],result=sentinel();args(inputs,motions[m],flags[f],floats[frame],floats[rate]);
  setup();CHECK(direct(inputs,&result)&&result.type==NativeScriptType_Void);
  arguments_match(motions[m],flags[f],floats[frame],floats[rate]);CHECK(completed()==0);
  setup();result=sentinel();CHECK(run(apply,inputs,4,&result)==NativeScriptVM_Ok&&result.type==NativeScriptType_Void);
  arguments_match(motions[m],flags[f],floats[frame],floats[rate]);CHECK(completed()==1);
  CHECK(nativeScriptRuntime_Idle());++pairs;
 }
 // Integral motion widths and signed flag marshalling preserve exact bits.
 const NativeScriptValue integral[]={
 {.type=NativeScriptType_Bool,.as.u64=1},{.type=NativeScriptType_I32,.as.i64=341},
 {.type=NativeScriptType_U32,.as.u64=341},{.type=NativeScriptType_I64,.as.i64=341},{.type=NativeScriptType_U64,.as.u64=341}};
 for(unsigned i=0;i<5;++i){NativeScriptValue inputs[4],result;args(inputs,341,0,0,UINT32_C(0x3f800000));
  inputs[0]=integral[i];inputs[1]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=INT32_MIN};setup();
  CHECK(direct(inputs,&result));arguments_match(i?341:1,UINT32_C(0x80000000),0,UINT32_C(0x3f800000));}
 // F64 and integer operands marshal to the explicit F32 native formals.
 NativeScriptValue inputs[4],result;args(inputs,341,0,0,0);
 inputs[2]=(NativeScriptValue){.type=NativeScriptType_F64,.as.f64=1.0000000596046448};
 inputs[3]=(NativeScriptValue){.type=NativeScriptType_I64,.as.i64=-3};setup();CHECK(direct(inputs,&result));
 arguments_match(341,0,bits((float)inputs[2].as.f64),bits(-3.0f));
}
static void lifetime_cases(void)
{
 for(unsigned mode=ChangePhysicalRemove;mode<=ChangeDataReplace;++mode){
  NativeScriptValue inputs[4],result=sentinel();args(inputs,341,UINT32_MAX,UINT32_C(0x80000000),UINT32_C(0xbf800000));
  setup();change_mode=mode;CHECK(!direct(inputs,&result));unchanged(&result);
  CHECK(change_count==1&&start_count==0&&order_count==1&&completed_bytes()==0&&diagnostic.message[0]);
  setup();change_mode=mode;result=sentinel();NativeScriptVMStatus expected=
    mode<=ChangeLogicalRetire?NativeScriptVM_SourceRetired:NativeScriptVM_Error;
  CHECK(run(apply,inputs,4,&result)==expected);unchanged(&result);
  CHECK(change_count==1&&start_count==0&&order_count==1&&nativeScriptRuntime_Idle());
  if(mode!=ChangeLogicalRetire)CHECK(completed()==0);
 }
 // Begin still installs motion before animation startup; enter_at_speed never does.
 setup();NativeScriptValue motion={.type=NativeScriptType_I32,.as.i64=341},result;
 CHECK(run(begin,&motion,1,&result)==NativeScriptVM_Ok&&result.type==NativeScriptType_Void);
 CHECK(change_count==1&&start_count==1&&order_count==2&&order[0]==1&&order[1]==2);
 CHECK(bits(change.frame)==0&&bits(change.rate)==UINT32_C(0x3f800000)&&completed()==1);
 for(unsigned mode=ChangePhysicalRemove;mode<=ChangeDataReplace;++mode){
  setup();change_mode=mode;result=sentinel();
  CHECK(run(begin,&motion,1,&result)!=NativeScriptVM_Ok);unchanged(&result);
  CHECK(change_count==1&&start_count==0&&order_count==1&&nativeScriptRuntime_Idle());
 }
}
static void rejection_cases(void)
{
 const NativeScriptValue invalid[]={
 {.type=NativeScriptType_I32,.as.i64=-1},{.type=NativeScriptType_I64,.as.i64=INT64_MIN},
 {.type=NativeScriptType_U32,.as.u64=349},{.type=NativeScriptType_U32,.as.u64=UINT16_MAX},
 {.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000155)},
 {.type=NativeScriptType_F32,.as.f32=341},{.type=NativeScriptType_F64,.as.f64=341},
 {.type=NativeScriptType_Null}};
 for(unsigned i=0;i<8;++i){setup();NativeScriptValue inputs[4],result=sentinel();args(inputs,341,0,0,0);inputs[0]=invalid[i];
  CHECK(!direct(inputs,&result)&&change_count==0&&start_count==0);unchanged(&result);}
 // Actual ownership validator denies inconsistent selected package/bank/table/view
 // relationships before native mutation. These are controlled publication errors.
 for(unsigned failure=0;failure<23;++failure){setup();
  switch(failure){
  case 0:package.address=NULL;break;case 1:package.ready=false;break;case 2:package.assets.ready=false;break;
  case 3:package.target=Ft_Kind_CustomFirst+1;break;case 4:fighter.kind=Ft_Kind_Mario;break;
  case 5:fighter.behavior_kind=Ft_Kind_Mario;break;case 6:fighter.ft_data=NULL;break;
  case 7:gFtDataList[fighter.kind]=NULL;break;case 8:ftData_CharacterStateTables[fighter.kind]=NULL;break;
  case 9:selected.bank=NULL;break;case 10:++selected.identity;break;case 11:package.compiler.bank.valid=false;break;
  case 12:package.compiler.bank.has_motion_layout=false;break;case 13:actor_status=NativeScriptVM_Error;break;
  case 14:storage.live=false;break;case 15:storage.owner=1;break;case 16:++storage.generation;break;
  case 17:storage.layout=NULL;break;case 18:view_program=NULL;break;case 19:++package.assets.facts.moves.first;break;
  case 20:++package.assets.facts.moves.ground[0];break;case 21:++fighter.x8_spawnNum;break;
  case 22:package.assets.facts.moves.transition_flags[0]=1;break;
  }
  NativeScriptValue inputs[4],result=sentinel();args(inputs,341,0,0,0);
  CHECK(!direct(inputs,&result)&&change_count==0&&start_count==0);unchanged(&result);
 }
 setup();NativeScriptValue inputs[4],result=sentinel();args(inputs,341,0,0,0);
 NativeScriptRuntimeContext caller=context();caller.source_class=NativeCallbackSource_Item;
 CHECK(!nativeScriptBindings_Invoke(&caller,NativeScriptBinding_StateEnterAtSpeed,NULL,0,inputs,4,&result,&diagnostic));unchanged(&result);
 caller=context();CHECK(!nativeScriptBindings_Invoke(&caller,NativeScriptBinding_StateEnterAtSpeed,NULL,0,inputs,3,&result,&diagnostic));unchanged(&result);
 setup();CHECK(run(apply,inputs,4,&result)==NativeScriptVM_Ok&&completed()==1);
}
int main(void)
{
 prepare();numeric_cases();lifetime_cases();rejection_cases();
 printf("{\"status\":\"PASS source motion rate\",\"checks\":%u,\"bit_argument_pairs\":%u,\"vm_runs\":%u}\n",checks,pairs,vm_runs);return 0;
}
