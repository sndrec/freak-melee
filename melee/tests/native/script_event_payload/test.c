#include <melee/ft/native_script_receiver.h>
#include <melee/ft/native_script_event_payload.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

enum { TestNamespace=1, Reenter=2, Retire=3, Observe=4, ReenterSignal=5 };
static char source[65536];
static NativeScriptSourceNode syntax[1024];
static NativeScriptIRNode expressions[4096];
static uint32_t arguments[4096];
static NativeScriptFunctionDeclaration functions[128];
static NativeScriptParameterDeclaration parameters[128];
static NativeScriptValueDeclaration values[128];
static NativeScriptLocal locals[128];
static NativeScriptInstruction instructions[4096];
static NativeScriptFunctionCode code[128];
static NativeScriptValue registers[4096];
static NativeScriptVMFrame vm_frames[32];
static NativeScriptEventPayloadFrame payload_frames[16];
static unsigned checks;
#define CHECK(value) do {++checks;if(!(value)){fprintf(stderr,"line %u: %s\n",__LINE__,#value);exit(1);}}while(0)

typedef struct Runtime {
    NativeScriptEventPayloadHandle payload;
    bool reentry;
} Runtime;
typedef struct Lifetime { void* source;uint32_t generation;bool alive; } Lifetime;
static Lifetime life;
static NativeScriptVM vm;
static NativeScriptBytecodeProgram program;
static NativeScriptProgramIndex index;
static NativeScriptVMHost host;
static NativeScriptEventPayloadPool pool;
static uint32_t inner,signal_inner;
static float observed;
static unsigned writes,retirements;

static bool alive(NativeScriptRuntimeContext* context)
{
    return life.alive && context->source==life.source && context->generation==life.generation;
}

static NativeScriptValue i32(int32_t value)
{
    NativeScriptValue result={.type=NativeScriptType_I32};result.as.i64=value;return result;
}
static NativeScriptValue float_value(float value)
{
    NativeScriptValue result={.type=NativeScriptType_F32};result.as.f32=value;return result;
}

static bool name_is(const NativeScriptIRArena* arena,const NativeScriptIRNode* node,const char* text)
{
    return node->text_length==strlen(text) && !memcmp(arena->source+node->text_offset,text,node->text_length);
}

static bool resolve(void* context,const NativeScriptIRArena* arena,const NativeScriptIRNode* node,
    NativeScriptIRResolution* result,NativeScriptDiagnostic* diagnostic)
{
    int status=nativeScriptEventPayload_TryResolve(context,arena,node,result,diagnostic);
    if (status) return status>0;
    if (node->kind==NativeScriptIR_Name && name_is(arena,node,"test")) {
        *result=(NativeScriptIRResolution){NativeScriptType_Namespace,TestNamespace,0,0};return true;
    }
    if (node->kind==NativeScriptIR_Member && arena->nodes[node->a].semantic==TestNamespace) {
        uint32_t semantic=name_is(arena,node,"reenter")?Reenter:name_is(arena,node,"retire")?Retire:
            name_is(arena,node,"observe")?Observe:name_is(arena,node,"reenter_signal")?ReenterSignal:0;
        if (semantic) { *result=(NativeScriptIRResolution){NativeScriptType_Callable,semantic,NativeScriptFlag_Callable,0};return true; }
    }
    if (node->kind==NativeScriptIR_Call) {
        uint32_t semantic=arena->nodes[node->a].semantic;
        if (semantic>=Reenter && semantic<=ReenterSignal) {
            *result=(NativeScriptIRResolution){NativeScriptType_Void,semantic,NativeScriptFlag_MayInvalidateSource,0};return true;
        }
    }
    snprintf(diagnostic->message,sizeof(diagnostic->message),"Unknown test binding");return false;
}

static bool describe(void* unused,uint32_t semantic,NativeScriptCallLayout* layout,NativeScriptDiagnostic* diagnostic)
{
    (void)unused;(void)diagnostic;*layout=(NativeScriptCallLayout){0};
    if (semantic==Observe) {layout->argument_count=1;layout->argument_types[0]=NativeScriptType_Object;return true;}
    return semantic==Reenter || semantic==Retire || semantic==ReenterSignal;
}

static bool read_value(NativeScriptRuntimeContext* context,uint32_t semantic,const NativeScriptValue* selectors,
    uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    (void)selectors;
    Runtime* runtime=context->user;
    if ((semantic&UINT32_C(0xf0000000))==NATIVE_SCRIPT_EVENT_VALUE_SEMANTIC)
        return !count && nativeScriptEventPayload_Read(&pool,runtime->payload,context,alive,semantic,result,diagnostic);
    if ((semantic&UINT32_C(0xf0000000))==NATIVE_SCRIPT_STATE_SEMANTIC && !count) {*result=float_value(observed);return alive(context);}
    return false;
}

static bool write_value(NativeScriptRuntimeContext* context,uint32_t semantic,const NativeScriptValue* selectors,
    uint32_t count,const NativeScriptValue* value,NativeScriptDiagnostic* diagnostic)
{
    (void)selectors;(void)diagnostic;
    if (!alive(context) || count || (semantic&UINT32_C(0xf0000000))!=NATIVE_SCRIPT_STATE_SEMANTIC || value->type!=NativeScriptType_F32) return false;
    ++writes;observed=value->as.f32;return true;
}

static NativeScriptVMStatus entry(uint32_t function,NativeScriptRuntimeContext* context,
    const NativeScriptValue* arguments,uint32_t count,NativeScriptValue* result)
{
    NativeScriptDiagnostic diagnostic;NativeScriptEventResolution event;
    CHECK(nativeScriptEvents_Resolve(index.tree,&syntax[functions[function].source_node],&event,&diagnostic));
    Runtime runtime={0};NativeScriptRuntimeContext call=*context;call.user=&runtime;
    CHECK(nativeScriptEventPayload_Push(&pool,event.event_id,&call,alive,arguments,count, NULL,&runtime.payload,&diagnostic));
    uint32_t typed_count;const NativeScriptValue* typed=nativeScriptEventPayload_Arguments(&pool,runtime.payload,&call,alive,&typed_count,&diagnostic);
    CHECK(typed!=NULL);CHECK(typed_count==count);
    NativeScriptVMStatus status=nativeScriptVM_Run(&vm,&program,&host,&call,function,typed,typed_count,result);
    CHECK(nativeScriptEventPayload_Pop(&pool,runtime.payload,&diagnostic));return status;
}

static bool invoke(NativeScriptRuntimeContext* context,uint32_t semantic,const NativeScriptValue* selectors,
    uint32_t selector_count,NativeScriptValue* args,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    (void)selectors;(void)diagnostic;CHECK(selector_count==0);
    Runtime* runtime=context->user;
    if (semantic==Reenter || semantic==ReenterSignal) {
        uint32_t depth=pool.depth,vm_depth=vm.depth,top=vm.register_top;
        NativeScriptValue argument=semantic==Reenter ? i32(777) : float_value(1000),nested;
        CHECK(entry(semantic==Reenter ? inner : signal_inner,context,&argument,1,&nested)==NativeScriptVM_Ok);
        CHECK(pool.depth==depth && vm.depth==vm_depth && vm.register_top==top);
        if (semantic==Reenter) CHECK(nested.type==NativeScriptType_I32 && nested.as.i64==777);
        // A nested descriptor cannot borrow an outer property's same-type slot.
        NativeScriptEventResolution inner_event;CHECK(nativeScriptEvents_Resolve(index.tree,&syntax[functions[inner].source_node],&inner_event,diagnostic));
        if (semantic==Reenter) {
            NativeScriptValue sentinel=i32(-999);
            uint32_t foreign=NATIVE_SCRIPT_EVENT_VALUE_SEMANTIC|(inner_event.event_id*4);
            CHECK(!nativeScriptEventPayload_Read(&pool,runtime->payload,context,alive,foreign,&sentinel,diagnostic));CHECK(sentinel.as.i64==-999);
        }
        *result=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    if (semantic==Retire) {
        CHECK(count==0);++retirements;life.alive=false;
#ifdef _WIN32
        DWORD previous;CHECK(VirtualProtect(context->source,4096,PAGE_NOACCESS,&previous)!=0);
#endif
        *result=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    if (semantic==Observe) {
        CHECK(count==1 && args[0].type==NativeScriptType_Object);
        CHECK(args[0].as.object.identity==UINT32_C(0xf0001234));CHECK(args[0].as.object.generation==UINT32_C(0xbeef));CHECK(args[0].as.object.kind==NATIVE_SCRIPT_RECEIVER_RETAINED_KIND);
        *result=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    return false;
}

static uint32_t callback(const char* name,unsigned occurrence)
{
    for (uint32_t i=0;i<index.function_count;++i) if (functions[i].callback &&
        nativeScriptSource_SpanEquals(index.tree,functions[i].name,name)) {
        if (!occurrence--) return i;
    }
    CHECK(false);return UINT32_MAX;
}

static void resolver_failures(uint32_t callback_node)
{
    NativeScriptDiagnostic diagnostic;NativeScriptEventPayloadResolverContext context;
    CHECK(nativeScriptEventPayload_ResolverContext(index.tree,callback_node,&context,&diagnostic));
    const NativeScriptSourceNode* body=&syntax[syntax[callback_node].first_child];
    NativeScriptIRNode node={.kind=NativeScriptIR_Name,.source=body->source};
    const char* marker=strstr(source+body->source.offset,"event");CHECK(marker!=NULL);
    node.text_offset=(uint32_t)(marker-source);node.text_length=5;node.source.offset=node.text_offset;node.source.length=5;
    NativeScriptIRArena arena={source,index.tree->source_bytes,expressions,0,4096,arguments,0,4096};NativeScriptIRResolution result;
    CHECK(nativeScriptEventPayload_TryResolve(&context,&arena,&node,&result,&diagnostic)==1);
    CHECK(result.type==NativeScriptType_Namespace);
    const char* helper_value=strstr(source,"value + 1");CHECK(helper_value!=NULL);
    node.source.offset=(uint32_t)(helper_value-source);
    CHECK(nativeScriptEventPayload_TryResolve(&context,&arena,&node,&result,&diagnostic)==-1);
    CHECK(strstr(diagnostic.message,"source scope")!=NULL);
    arena.node_count=1;expressions[0]=(NativeScriptIRNode){.kind=NativeScriptIR_Name,.type=NativeScriptType_Namespace,.semantic=NATIVE_SCRIPT_STATE_NAMESPACE};
    node.kind=NativeScriptIR_Member;node.a=0;node.source=body->source;
    CHECK(nativeScriptEventPayload_TryResolve(&context,&arena,&node,&result,&diagnostic)==0);
}

static void bad_source(const char* text,const char* expected)
{
    NativeScriptSourceTree tree={text,(uint32_t)strlen(text),syntax,0,1024};NativeScriptDiagnostic diagnostic;
    CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
    NativeScriptIRArena arena={text,tree.source_bytes,expressions,0,4096,arguments,0,4096};
    NativeScriptProgramIndex rejected_index={.tree=&tree,.expressions=&arena,.functions=functions,.function_capacity=128,
        .parameters=parameters,.parameter_capacity=128,.values=values,.value_capacity=128,.event_signature=nativeScriptEvents_Signature};
    CHECK(nativeScriptProgram_Index(&rejected_index,&diagnostic));
    NativeScriptProgramCompiler compiler={.index=&rejected_index,.native_bindings=resolve,.locals=locals,.local_capacity=128};
    for (uint32_t i=0;i<rejected_index.function_count;++i) {
        NativeScriptEventPayloadResolverContext payload_context;
        if (functions[i].callback) {CHECK(nativeScriptEventPayload_ResolverContext(&tree,functions[i].source_node,&payload_context,&diagnostic));compiler.native_binding_context=&payload_context;}
        else compiler.native_binding_context=NULL;
        NativeScriptStatementCompiler body;
        if (!nativeScriptProgram_CompileBody(&compiler,i,&body,&diagnostic)) {if(!strstr(diagnostic.message,expected))fprintf(stderr,"Expected %s; got %s\n",expected,diagnostic.message);CHECK(strstr(diagnostic.message,expected)!=NULL);return;}
    }
    CHECK(false);
}

int main(int argc,char** argv)
{
    CHECK(argc==2);FILE* file=fopen(argv[1],"rb");CHECK(file!=NULL);
    uint32_t bytes=(uint32_t)fread(source,1,sizeof(source)-1,file);fclose(file);
    NativeScriptDiagnostic diagnostic;NativeScriptSourceTree tree={source,bytes,syntax,0,1024};CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
    NativeScriptIRArena arena={source,bytes,expressions,0,4096,arguments,0,4096};
    index=(NativeScriptProgramIndex){.tree=&tree,.expressions=&arena,.functions=functions,.function_capacity=128,
        .parameters=parameters,.parameter_capacity=128,.values=values,.value_capacity=128,.event_signature=nativeScriptEvents_Signature};
    CHECK(nativeScriptProgram_Index(&index,&diagnostic));
    NativeScriptProgramCompiler compiler={.index=&index,.native_bindings=resolve,.locals=locals,.local_capacity=128};
    NativeScriptBytecodeBuilder builder={.instructions=instructions,.instruction_capacity=4096,.functions=code,
        .function_count=index.function_count,.describe_call=describe};
    for (uint32_t i=0;i<index.function_count;++i) {
        NativeScriptEventPayloadResolverContext context;
        if (functions[i].callback) {CHECK(nativeScriptEventPayload_ResolverContext(&tree,functions[i].source_node,&context,&diagnostic));compiler.native_binding_context=&context;}
        else compiler.native_binding_context=NULL;
        NativeScriptStatementCompiler body;
        if (!nativeScriptProgram_CompileBody(&compiler,i,&body,&diagnostic)) {fprintf(stderr,"compile line%u %s\n",diagnostic.source.line,diagnostic.message);return 1;}
        CHECK(nativeScriptBytecode_Compile(&builder,&body,functions[i].source_node,i,(uint16_t)functions[i].parameter_count,&diagnostic));
    }
    program=(NativeScriptBytecodeProgram){source,bytes,instructions,builder.instruction_count,code,index.function_count,false}; { NativeScriptDiagnostic preparation={0}; if(!nativeScriptBytecode_Prepare(&program,&preparation)){fprintf(stderr,"bytecode preparation: %s\n",preparation.message);abort();} }
    vm=(NativeScriptVM){.registers=registers,.register_capacity=4096,.frames=vm_frames,.frame_capacity=32};
    pool=(NativeScriptEventPayloadPool){.frames=payload_frames,.capacity=16};host=(NativeScriptVMHost){alive,read_value,write_value,invoke};
#ifdef _WIN32
    void* actor=VirtualAlloc(NULL,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);CHECK(actor!=NULL);
#else
    int marker;void* actor=&marker;
#endif
    life=(Lifetime){actor,1,true};NativeScriptRuntimeContext context={.source=actor,.generation=1,.frame=100,.actor_id=17};
    inner=callback("action_metadata",0);signal_inner=callback("signal inner",0);
    NativeScriptValue result,args[3]={i32(10),{.type=NativeScriptType_Bool}};args[1].as.u64=1;
    CHECK(entry(callback("cpu_projectile_hit",0),&context,args,2,&result)==NativeScriptVM_Ok);CHECK(result.as.i64==22);CHECK(pool.depth==0 && vm.depth==0);
    args[0]=(NativeScriptValue){.type=NativeScriptType_U32};args[0].as.u64=UINT32_C(0xfedcba98);
    CHECK(entry(callback("cpu_prepare",0),&context,args,1,&result)==NativeScriptVM_Ok);CHECK((uint32_t)result.as.i64==UINT32_C(0xfedcba98));
    args[0]=(NativeScriptValue){.type=NativeScriptType_Object};args[0].as.object=(NativeScriptObjectValue){UINT32_C(0xf0001234),UINT32_C(0xbeef),NATIVE_SCRIPT_RECEIVER_RETAINED_KIND};
    NativeScriptEventResolution invalid_event;NativeScriptEventPayloadHandle invalid_handle;
    CHECK(nativeScriptEvents_Resolve(&tree,&syntax[functions[callback("capture_wait_enter",0)].source_node],&invalid_event,&diagnostic));
    // ABI-only fixture identities are controlled, but tags/classes use the
    // production retained-token contract and malformed tokens remain rejected.
    args[0].as.object.kind=0;
    CHECK(!nativeScriptEventPayload_Push(&pool,invalid_event.event_id,&context,alive,args,1, NULL,&invalid_handle,&diagnostic));CHECK(pool.depth==0);
    args[0].as.object.kind=NATIVE_SCRIPT_RECEIVER_RETAINED_KIND|1;
    CHECK(!nativeScriptEventPayload_Push(&pool,invalid_event.event_id,&context,alive,args,1, NULL,&invalid_handle,&diagnostic));CHECK(pool.depth==0);
    args[0].as.object.kind=NATIVE_SCRIPT_RECEIVER_RETAINED_KIND|3;
    CHECK(!nativeScriptEventPayload_Push(&pool,invalid_event.event_id,&context,alive,args,1, NULL,&invalid_handle,&diagnostic));CHECK(pool.depth==0);
    args[0].as.object.kind=NATIVE_SCRIPT_RECEIVER_RETAINED_KIND;
    CHECK(entry(callback("capture_wait_enter",0),&context,args,1,&result)==NativeScriptVM_Ok);
    context.source_class=1;args[0]=float_value(2);observed=0;
    CHECK(entry(callback("signal outer",0),&context,args,1,&result)==NativeScriptVM_Ok);CHECK(observed==1004 && writes==3);CHECK(pool.depth==0);
    context.source_class=0;
    NativeScriptEventResolution event;CHECK(nativeScriptEvents_Resolve(&tree,&syntax[functions[inner].source_node],&event,&diagnostic));
    Runtime outer_runtime;context.user=&outer_runtime;args[0]=i32(12);
    CHECK(nativeScriptEventPayload_Push(&pool,event.event_id,&context,alive,args,1, NULL,&outer_runtime.payload,&diagnostic));
    Runtime nested_runtime;CHECK(nativeScriptEventPayload_Push(&pool,event.event_id,&context,alive,args,1, NULL,&nested_runtime.payload,&diagnostic));
    uint32_t field=NATIVE_SCRIPT_EVENT_VALUE_SEMANTIC|(event.event_id*4);result=i32(-999);
    CHECK(!nativeScriptEventPayload_Read(&pool,outer_runtime.payload,&context,alive,field,&result,&diagnostic));CHECK(result.as.i64==-999);
    CHECK(!nativeScriptEventPayload_Pop(&pool,outer_runtime.payload,&diagnostic));
    CHECK(nativeScriptEventPayload_Pop(&pool,nested_runtime.payload,&diagnostic));
    CHECK(nativeScriptEventPayload_Read(&pool,outer_runtime.payload,&context,alive,field,&result,&diagnostic));CHECK(result.as.i64==12);
    CHECK(nativeScriptEventPayload_Pop(&pool,outer_runtime.payload,&diagnostic));
    CHECK(!nativeScriptEventPayload_Read(&pool,outer_runtime.payload,&context,alive,field,&result,&diagnostic));
    NativeScriptEventPayloadHandle reused;CHECK(nativeScriptEventPayload_Push(&pool,event.event_id,&context,alive,args,1, NULL,&reused,&diagnostic));
    CHECK(reused.serial!=outer_runtime.payload.serial);CHECK(!nativeScriptEventPayload_Read(&pool,outer_runtime.payload,&context,alive,field,&result,&diagnostic));
    NativeScriptRuntimeContext wrong=context;wrong.generation=2;CHECK(!nativeScriptEventPayload_Read(&pool,reused,&wrong,alive,field,&result,&diagnostic));
    wrong=context;wrong.actor_id=18;CHECK(!nativeScriptEventPayload_Read(&pool,reused,&wrong,alive,field,&result,&diagnostic));
    wrong=context;wrong.frame=101;CHECK(!nativeScriptEventPayload_Read(&pool,reused,&wrong,alive,field,&result,&diagnostic));
    CHECK(nativeScriptEventPayload_Pop(&pool,reused,&diagnostic));
    pool.capacity=1;CHECK(nativeScriptEventPayload_Push(&pool,event.event_id,&context,alive,args,1, NULL,&reused,&diagnostic));
    CHECK(!nativeScriptEventPayload_Push(&pool,event.event_id,&context,alive,args,1, NULL,&nested_runtime.payload,&diagnostic));CHECK(pool.depth==1);CHECK(nativeScriptEventPayload_Pop(&pool,reused,&diagnostic));pool.capacity=16;
    args[0]=float_value(12);CHECK(!nativeScriptEventPayload_Push(&pool,event.event_id,&context,alive,args,1, NULL,&reused,&diagnostic));CHECK(pool.depth==0);
    args[0]=i32(12);args[0].as.i64=INT64_C(0x100000000);CHECK(!nativeScriptEventPayload_Push(&pool,event.event_id,&context,alive,args,1, NULL,&reused,&diagnostic));CHECK(pool.depth==0);
    CHECK(!nativeScriptEventPayload_Push(&pool,event.event_id,&context,alive,args,0, NULL,&reused,&diagnostic));
    uint64_t serial=pool.serial;pool.serial=UINT64_MAX;args[0]=i32(12);
    CHECK(!nativeScriptEventPayload_Push(&pool,event.event_id,&context,alive,args,1, NULL,&reused,&diagnostic));CHECK(pool.depth==0);pool.serial=serial;
    uint32_t zero_event=UINT32_MAX;
    for (uint32_t id=0;id<nativeScriptEvents_Count();++id) {
        const NativeScriptEventDescriptor* descriptor=nativeScriptEvents_Descriptor(id);
        if (descriptor->context==NativeScriptEvent_Fighter && descriptor->signature_supported && !descriptor->argument_count) {zero_event=id;break;}
    }
    CHECK(zero_event!=UINT32_MAX);
    CHECK(nativeScriptEventPayload_Push(&pool,zero_event,&context,alive,NULL,0, NULL,&reused,&diagnostic));
    uint32_t empty_count=99;
    CHECK(nativeScriptEventPayload_Arguments(&pool,reused,&context,alive,&empty_count,&diagnostic)!=NULL);CHECK(empty_count==0);
    CHECK(nativeScriptEventPayload_Pop(&pool,reused,&diagnostic));
    resolver_failures(functions[callback("cpu_projectile_hit",0)].source_node);
    context.user=NULL;args[0]=i32(3);result=i32(-999);
    CHECK(entry(callback("cpu_projectile_defense",0),&context,args,1,&result)==NativeScriptVM_SourceRetired);CHECK(retirements==1 && result.as.i64==-999);CHECK(pool.depth==0 && vm.depth==0);
#ifdef _WIN32
    CHECK(VirtualFree(actor,0,MEM_RELEASE)!=0);
#endif
    uint32_t accepted_functions=index.function_count;
    bad_source("action A:\n    on action_metadata:\n        event.metadata = 1\n","not writable");
    bad_source("action A:\n    on action_metadata:\n        return event.prediction_frames\n","Unrecognized property");
    bad_source("action A:\n    on action_metadata:\n        return signal.value\n","namespace is unavailable");
    bad_source("action A:\n    on action_metadata:\n        return event.metadata()\n","unknown member/index/call");
    bad_source("action A:\n    on action_metadata:\n        return event.metadata[0]\n","unknown member/index/call");
    bad_source("func Helper() -> int:\n    return event.metadata\n","source scope");
    bad_source("action A:\n    on capture_wait_enter:\n        test.observe(other.position)\n","Unknown test binding");
    printf("{\"result\":\"pass\",\"checks\":%u,\"functions\":%u,\"instructions\":%u}\n",checks,accepted_functions,builder.instruction_count);
    return 0;
}
