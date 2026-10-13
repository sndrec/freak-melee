#include <melee/ft/native_script_event_payload.h>
#include <melee/ft/native_fighter_trail.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks,cases;
static NativeScriptDiagnostic diagnostic;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"record %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
static NativeScriptEventPayloadFrame frames[2];
static NativeScriptEventPayloadPool pool={frames,2,0,0};
static NativeScriptEventPayloadHandle handle;
static NativeScriptRuntimeContext context={.source=&pool,.generation=7,.actor_id=11,.frame=13,.scene_epoch=UINT64_C(0x100000003)};
static bool live=true;
static unsigned alive_calls,retire_on_call;
static bool alive(NativeScriptRuntimeContext* c)
{if(retire_on_call&&++alive_calls==retire_on_call)live=false;return live&&c->source==&pool&&c->generation==7;}
static uint32_t event(const char* name)
{for(uint32_t i=0;i<nativeScriptEvents_Count();++i)if(!strcmp(nativeScriptEvents_Descriptor(i)->name,name))return i;CHECK(false);return UINT32_MAX;}
static uint32_t field(uint32_t id,unsigned index)
{return NATIVE_SCRIPT_EVENT_RECORD_SEMANTIC|(id*NATIVE_SCRIPT_EVENT_RECORD_FIELDS+index);}
static NativeScriptValue integer(int64_t value)
{return (NativeScriptValue){.type=NativeScriptType_I32,.as.i64=value};}
static void reset(void)
{memset(frames,0x5A,sizeof(frames));pool.depth=0;pool.serial=0;live=true;alive_calls=retire_on_call=0;context.source_class=0;context.generation=7;context.scene_epoch=UINT64_C(0x100000003);}
static NativeFighterTrail seed(void)
{
    NativeFighterTrail value={.bone=-1,.axis=0,.inner_distance=-0.0f,.outer_distance=2.25f,
        .inner_fade=.25f,.outer_fade=.75f,.start_alpha=0,.end_alpha=255,
        .inner_red=-17,.inner_green=19,.inner_blue=23,.outer_red=29,.outer_green=31,.outer_blue=37};
    uint32_t nan=UINT32_C(0x7fc12345);memcpy(&value.inner_fade,&nan,4);return value;
}
static void kernel(void)
{
    uint32_t trail=event("trail"),tick=event("tick");
    const NativeScriptEventDescriptor* descriptor=nativeScriptEvents_Descriptor(trail);
    CHECK(descriptor->signature_supported&&descriptor->argument_count==0&&descriptor->result_type==NativeScriptType_Void);
    CHECK(descriptor->body_policy==NativeScriptEvent_ReadOnlyBody&&descriptor->record->field_count==14);
    CHECK(descriptor->record->native_bytes==sizeof(NativeFighterTrail));
    NativeFighterTrail original=seed();uint8_t unaligned[sizeof(original)+1];memcpy(unaligned+1,&original,sizeof(original));
    NativeScriptEventRecordIO io={unaligned+1,sizeof(original)};reset();
    CHECK(nativeScriptEventPayload_Push(&pool,trail,&context,alive,NULL,0,&io,&handle,&diagnostic));
    NativeScriptValue value={0};
    for(unsigned i=0;i<14;++i){CHECK(nativeScriptEventPayload_Read(&pool,handle,&context,alive,field(trail,i),&value,&diagnostic));
        CHECK(value.type==descriptor->record->fields[i].type);}
    uint8_t output[sizeof(original)+2];memset(output,0xA5,sizeof(output));
    CHECK(nativeScriptEventPayload_ExportRecord(&pool,handle,&context,alive,output+1,sizeof(original),&diagnostic));
    CHECK(!memcmp(output+1,&original,sizeof(original))&&output[0]==0xA5&&output[sizeof(output)-1]==0xA5);++cases;
    value=integer(104);CHECK(nativeScriptEventPayload_Write(&pool,handle,&context,alive,field(trail,0),&value,&diagnostic));
    CHECK(!memcmp(unaligned+1,&original,sizeof(original)));
    NativeFighterTrail expected=original;expected.bone=104;
    CHECK(nativeScriptEventPayload_ExportRecord(&pool,handle,&context,alive,output+1,sizeof(original),&diagnostic));
    CHECK(!memcmp(output+1,&expected,sizeof(expected)));++cases;
    NativeScriptEventPayloadHandle outer=handle,inner;
    CHECK(nativeScriptEventPayload_Push(&pool,trail,&context,alive,NULL,0,&io,&inner,&diagnostic));
    CHECK(!nativeScriptEventPayload_Write(&pool,outer,&context,alive,field(trail,0),&value,&diagnostic));
    memset(output,0xA5,sizeof(output));CHECK(!nativeScriptEventPayload_ExportRecord(&pool,outer,&context,alive,output+1,sizeof(original),&diagnostic));
    CHECK(output[1]==0xA5&&!nativeScriptEventPayload_Pop(&pool,outer,&diagnostic));
    NativeScriptEventPayloadHandle absent={99,99};CHECK(!nativeScriptEventPayload_Push(&pool,trail,&context,alive,NULL,0,&io,&absent,&diagnostic));
    CHECK(absent.serial==99&&absent.slot==99&&pool.depth==2);
    CHECK(nativeScriptEventPayload_Pop(&pool,inner,&diagnostic));
    CHECK(nativeScriptEventPayload_Read(&pool,outer,&context,alive,field(trail,0),&value,&diagnostic)&&value.as.i64==104);++cases;
    value=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=1};
    CHECK(!nativeScriptEventPayload_Write(&pool,outer,&context,alive,field(trail,0),&value,&diagnostic));
    value=integer(INT64_C(0x100000001));CHECK(!nativeScriptEventPayload_Write(&pool,outer,&context,alive,field(trail,0),&value,&diagnostic));
    value=integer(9);CHECK(!nativeScriptEventPayload_Write(&pool,outer,&context,alive,field(tick,0),&value,&diagnostic));
    CHECK(!nativeScriptEventPayload_Write(&pool,outer,&context,alive,NATIVE_SCRIPT_EVENT_VALUE_SEMANTIC,&value,&diagnostic));
    CHECK(!nativeScriptEventPayload_Write(&pool,outer,&context,alive,field(trail,15),&value,&diagnostic));++cases;
    frames[outer.slot].fields[13]=integer(INT64_C(0x100000000));
    memset(output,0xA5,sizeof(output));CHECK(!nativeScriptEventPayload_ExportRecord(&pool,outer,&context,alive,output+1,sizeof(original),&diagnostic));
    for(unsigned i=0;i<sizeof(output);++i)CHECK(output[i]==0xA5);
    frames[outer.slot].fields[13]=integer(original.outer_blue);++cases;
    NativeScriptValue scalar={.type=NativeScriptType_F32,.as.f32=-0.0f};
    CHECK(nativeScriptEventPayload_Write(&pool,outer,&context,alive,field(trail,3),&scalar,&diagnostic));
    CHECK(nativeScriptEventPayload_ExportRecord(&pool,outer,&context,alive,output+1,sizeof(original),&diagnostic));
    CHECK(!memcmp(output+1+offsetof(NativeFighterTrail,outer_distance),&scalar.as.f32,4));++cases;
    memset(output,0xA5,sizeof(output));retire_on_call=2;
    CHECK(!nativeScriptEventPayload_ExportRecord(&pool,outer,&context,alive,output+1,sizeof(original),&diagnostic));
    for(unsigned i=0;i<sizeof(output);++i)CHECK(output[i]==0xA5);
    live=true;alive_calls=retire_on_call=0;++cases;
    context.scene_epoch++;CHECK(!nativeScriptEventPayload_ExportRecord(&pool,outer,&context,alive,output+1,sizeof(original),&diagnostic));context.scene_epoch--;
    live=false;CHECK(!nativeScriptEventPayload_Write(&pool,outer,&context,alive,field(trail,0),&value,&diagnostic));live=true;
    CHECK(nativeScriptEventPayload_Pop(&pool,outer,&diagnostic));
    value=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
    CHECK(!nativeScriptEventPayload_Read(&pool,outer,&context,alive,field(trail,0),&value,&diagnostic)&&value.as.u64==99);++cases;
    reset();NativeScriptEventPayloadFrame prior=frames[0];absent=(NativeScriptEventPayloadHandle){99,99};
    CHECK(!nativeScriptEventPayload_Push(&pool,trail,&context,alive,NULL,0,NULL,&absent,&diagnostic));
    io.bytes--;CHECK(!nativeScriptEventPayload_Push(&pool,trail,&context,alive,NULL,0,&io,&absent,&diagnostic));io.bytes++;
    CHECK(!nativeScriptEventPayload_Push(&pool,tick,&context,alive,NULL,0,&io,&absent,&diagnostic));
    CHECK(pool.depth==0&&pool.serial==0&&absent.serial==99&&!memcmp(&prior,&frames[0],sizeof(prior)));++cases;
    uint32_t history=event("cpu_prepare");NativeScriptValue high={.type=NativeScriptType_U32,.as.u64=UINT32_C(0xf1234567)};
    CHECK(nativeScriptEventPayload_Push(&pool,history,&context,alive,&high,1,NULL,&handle,&diagnostic));
    CHECK(nativeScriptEventPayload_Read(&pool,handle,&context,alive,NATIVE_SCRIPT_EVENT_VALUE_SEMANTIC|(history*4),&value,&diagnostic));
    CHECK(value.type==NativeScriptType_I32&&(uint32_t)value.as.i64==UINT32_C(0xf1234567));
    CHECK(nativeScriptEventPayload_Pop(&pool,handle,&diagnostic));++cases;
}

static NativeScriptSourceNode syntax[128];static NativeScriptIRNode expressions[256];static uint32_t arguments[256];
static NativeScriptFunctionDeclaration functions[8];static NativeScriptParameterDeclaration parameters[16];
static NativeScriptValueDeclaration values[16];static NativeScriptLocal locals[32];
static NativeScriptInstruction instructions[256];static NativeScriptFunctionCode code[8];
static bool resolve(void* c,const NativeScriptIRArena* a,const NativeScriptIRNode* n,NativeScriptIRResolution* r,NativeScriptDiagnostic* d)
{
    int status=nativeScriptEventPayload_TryResolve(c,a,n,r,d);if(status)return status>0;
    if(n->kind==NativeScriptIR_Name&&n->text_length==4&&!memcmp(a->source+n->text_offset,"test",4)) {
        *r=(NativeScriptIRResolution){.type=NativeScriptType_Namespace,.semantic=1};return true;
    }
    if(n->kind==NativeScriptIR_Member&&a->nodes[n->a].semantic==1) {
        *r=(NativeScriptIRResolution){.type=NativeScriptType_Callable,.semantic=2,.flags=NativeScriptFlag_Callable};return true;
    }
    if(n->kind==NativeScriptIR_Call&&a->nodes[n->a].semantic==2) {
        *r=(NativeScriptIRResolution){.type=NativeScriptType_Void,.semantic=2,.flags=NativeScriptFlag_MayInvalidateSource};return true;
    }
    return false;
}
static bool read_record(NativeScriptRuntimeContext* c,uint32_t id,const NativeScriptValue* selectors,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{(void)selectors;return !count&&nativeScriptEventPayload_Read(&pool,handle,c,alive,id,out,d);}
static bool write_record(NativeScriptRuntimeContext* c,uint32_t id,const NativeScriptValue* selectors,uint32_t count,const NativeScriptValue* value,NativeScriptDiagnostic* d)
{(void)selectors;return !count&&nativeScriptEventPayload_Write(&pool,handle,c,alive,id,value,d);}
static bool compile(const char* source,bool execute)
{
    NativeScriptSourceTree tree={source,(uint32_t)strlen(source),syntax,0,128};
    NativeScriptIRArena arena={source,tree.source_bytes,expressions,0,256,arguments,0,256};
    NativeScriptProgramIndex index={.tree=&tree,.expressions=&arena,.functions=functions,.function_capacity=8,
        .parameters=parameters,.parameter_capacity=16,.values=values,.value_capacity=16,.event_signature=nativeScriptEvents_Signature};
    if(!nativeScriptSource_Parse(&tree,&diagnostic)||!nativeScriptProgram_Index(&index,&diagnostic))return false;
    NativeScriptProgramCompiler compiler={.index=&index,.native_bindings=resolve,.locals=locals,.local_capacity=32};
    NativeScriptBytecodeBuilder builder={.instructions=instructions,.instruction_capacity=256,.functions=code,.function_count=index.function_count};
    for(unsigned i=0;i<index.function_count;++i){NativeScriptEventPayloadResolverContext payload={0};
        if(functions[i].callback&&!nativeScriptEventPayload_ResolverContext(&tree,functions[i].source_node,&payload,&diagnostic))return false;
        compiler.native_binding_context=functions[i].callback?&payload:NULL;NativeScriptStatementCompiler body;
        if(!nativeScriptProgram_CompileBody(&compiler,i,&body,&diagnostic))return false;
        if(execute&&!nativeScriptBytecode_Compile(&builder,&body,functions[i].source_node,i,functions[i].parameter_count,&diagnostic))return false;
    }
    if(!execute)return true;
    NativeScriptBytecodeProgram program={source,tree.source_bytes,instructions,builder.instruction_count,code,index.function_count,false}; { NativeScriptDiagnostic preparation={0}; if(!nativeScriptBytecode_Prepare(&program,&preparation)){fprintf(stderr,"bytecode preparation: %s\n",preparation.message);abort();} }
    NativeScriptValue registers[64],result={0};NativeScriptVMFrame vm_frames[4];
    NativeScriptVM vm={.registers=registers,.register_capacity=64,.frames=vm_frames,.frame_capacity=4};
    NativeScriptVMHost host={.source_alive=alive,.read=read_record,.write=write_record};
    NativeFighterTrail input=seed(),output;NativeScriptEventRecordIO io={&input,sizeof(input)};reset();
    CHECK(nativeScriptEventPayload_Push(&pool,event("trail"),&context,alive,NULL,0,&io,&handle,&diagnostic));
    CHECK(nativeScriptVM_Run(&vm,&program,&host,&context,0,NULL,0,&result)==NativeScriptVM_Ok);
    CHECK(nativeScriptEventPayload_ExportRecord(&pool,handle,&context,alive,&output,sizeof(output),&diagnostic));
    CHECK(output.axis==1&&output.outer_red==input.outer_red+1&&output.bone==input.bone);
    CHECK(nativeScriptEventPayload_Pop(&pool,handle,&diagnostic));return true;
}
static void compiler(void)
{
    CHECK(compile("action Ribbon:\n    on trail:\n        let int tone = trail.outer_red\n        trail.outer_red = tone + 1\n        if trail.axis == 0: trail.axis = 1\n",true));++cases;
    CHECK(!compile("action Ribbon:\n    state int stored\n    on trail:\n        state.stored = 1\n",false));CHECK(strstr(diagnostic.message,"Read-only event cannot write"));++cases;
    CHECK(!compile("action Ribbon:\n    on tick:\n        trail.axis = 1\n",false));++cases;
    CHECK(!compile("func Helper():\n    let int value = trail.axis\n",false));++cases;
    CHECK(!compile("action Ribbon:\n    on trail:\n        trail.unknown = 1\n",false));++cases;
    CHECK(!compile("action Ribbon:\n    on trail:\n        test.mutate()\n",false));CHECK(strstr(diagnostic.message,"source-checked pure call"));++cases;
}
int main(void)
{kernel();compiler();printf("{\"status\":\"PASS event record\",\"checks\":%u,\"cases\":%u}\n",checks,cases);return 0;}
