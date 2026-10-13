#include <melee/ft/native_script_vm.h>
#include <melee/ft/native_script_expression.h>
#include <melee/ft/native_script_program.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

enum { Namespace=1, Retire=2, Point=3, RetiringPoint=4, NextIndex=5, Value=6, Echo=7,
       StateValue=8, Array=9, Resource=10, Joint=11, JointX=12, Reenter=13 };
static char source[65536];
static NativeScriptSourceNode syntax[2048];
static NativeScriptIRNode nodes[8192];
static uint32_t arguments[8192];
static NativeScriptInstruction instructions[8192];
static NativeScriptFunctionCode functions[32];
static NativeScriptLocal locals[256];
static NativeScriptFunctionDeclaration declarations[32];
static NativeScriptParameterDeclaration parameter_declarations[64];
static NativeScriptValueDeclaration value_declarations[64];
static NativeScriptValue registers[4096];
static NativeScriptVMFrame frames[32];
static unsigned checks;

#define CHECK(expression) do { ++checks; if (!(expression)) { fprintf(stderr,"line %u failed: %s\n",__LINE__,#expression);exit(1); } } while (0)

static bool preparation_alive(NativeScriptRuntimeContext* context)
{ ++*(unsigned*)context->user; return true; }

static void preparation_tests(void)
{
    NativeScriptInstruction baseline[4]={
        {.opcode=NativeScriptBC_Constant,.destination=0,.type=NativeScriptType_I32,.immediate=5},
        {.opcode=NativeScriptBC_Move,.destination=1,.a=0},
        {.opcode=NativeScriptBC_Return,.a=1,.type=NativeScriptType_I32},
        {.opcode=NativeScriptBC_Return,.type=NativeScriptType_Void}};
    NativeScriptFunctionCode signatures[2]={
        {.instruction_count=3,.register_count=2,.local_count=1,.parameter_count=1,
            .parameter_types={NativeScriptType_I32},.result_type=NativeScriptType_I32},
        {.first_instruction=3,.instruction_count=1,.register_count=1,.result_type=NativeScriptType_Void}};
    NativeScriptValue values[64],argument={.type=NativeScriptType_I32,.as.i64=17};
    NativeScriptVMFrame stack[4];
    NativeScriptVM machine={.registers=values,.register_capacity=64,.frames=stack,.frame_capacity=4};
    unsigned calls=0;
    NativeScriptRuntimeContext context={.user=&calls};
    NativeScriptVMHost vm_interface={.source_alive=preparation_alive};
    for(unsigned test=0;test<30;++test) {
        NativeScriptInstruction ops[4];memcpy(ops,baseline,sizeof(ops));
        NativeScriptFunctionCode functions[2];memcpy(functions,signatures,sizeof(functions));
        NativeScriptBytecodeProgram candidate={"",0,ops,4,functions,2,false};
        switch(test) {
        case 0:ops[0].destination=2;break;
        case 1:ops[1].a=2;break;
        case 2:ops[1].opcode=NativeScriptBC_Binary;ops[1].b=2;break;
        case 3:ops[0].type=NativeScriptType_Text;ops[0].immediate=1;break;
        case 4:ops[0].type=NativeScriptType_PoseGraph;ops[0].immediate=16;break;
        case 5:ops[1].opcode=NativeScriptBC_Write;ops[1].b=2;break;
        case 6:ops[1].opcode=NativeScriptBC_Read;ops[1].a=2;ops[1].selector_count=1;break;
        case 7:ops[1].opcode=NativeScriptBC_Invoke;ops[1].argument_count=33;break;
        case 8:ops[1].opcode=NativeScriptBC_Invoke;ops[1].b=UINT16_MAX;ops[1].argument_count=1;break;
        case 9:ops[1].opcode=NativeScriptBC_Call;ops[1].semantic=2;break;
        case 10:ops[1].opcode=NativeScriptBC_Call;ops[1].semantic=1;ops[1].type=NativeScriptType_I32;break;
        case 11:ops[1].opcode=NativeScriptBC_Call;ops[1].semantic=1;ops[1].type=NativeScriptType_Void;ops[1].argument_count=1;break;
        case 12:ops[1].opcode=NativeScriptBC_Jump;ops[1].target=3;break;
        case 13:ops[1].opcode=NativeScriptBC_JumpFalse;ops[1].a=2;break;
        case 14:ops[2].type=NativeScriptType_F32;break;
        case 15:ops[2].a=2;break;
        case 16:ops[1].opcode=UINT8_MAX;break;
        case 17:functions[0].instruction_count=0;break;
        case 18:functions[0].first_instruction=UINT32_MAX;break;
        case 19:functions[0].local_count=3;break;
        case 20:functions[0].parameter_count=2;break;
        case 21:functions[0].parameter_count=functions[0].local_count=functions[0].register_count=33;break;
        case 22:ops[2].opcode=NativeScriptBC_Move;ops[2].destination=1;break;
        case 23:functions[1].first_instruction=4;break;
        case 24:candidate.instructions=NULL;break;
        case 25:candidate.functions=NULL;break;
        case 26:candidate.source=NULL;candidate.source_bytes=1;break;
        case 27:functions[1].instruction_count=UINT32_MAX;break;
        case 28:ops[2].opcode=NativeScriptBC_JumpTrue;ops[2].target=0;break;
        case 29:ops[1].opcode=NativeScriptBC_ValidateBinding;ops[1].a=2;ops[1].selector_count=1;break;
        }
        NativeScriptDiagnostic diagnostic={0};
        CHECK(!nativeScriptBytecode_Prepare(&candidate,&diagnostic));
        CHECK(!candidate.prepared && diagnostic.message[0]);
        NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
        CHECK(nativeScriptVM_Run(&machine,&candidate,&vm_interface,&context,0,&argument,1,&result)==NativeScriptVM_Error);
        CHECK(!calls && result.type==NativeScriptType_U64 && result.as.u64==UINT64_MAX);
        CHECK(!machine.depth && !machine.register_top);
    }
    NativeScriptBytecodeProgram candidate={"",0,baseline,4,signatures,2,false};
    NativeScriptDiagnostic diagnostic={0};NativeScriptValue result;
    CHECK(nativeScriptBytecode_Prepare(&candidate,&diagnostic));
    CHECK(nativeScriptVM_Run(&machine,&candidate,&vm_interface,&context,0,&argument,1,&result)==NativeScriptVM_Ok);
    CHECK(result.type==NativeScriptType_I32 && result.as.i64==5 && calls==1);
}

typedef struct FunctionName { char name[64];uint32_t source,parameters;unsigned result; } FunctionName;
static FunctionName names[32];
static unsigned function_count;
static unsigned value_count;
static NativeScriptVM* active_vm;
static const NativeScriptBytecodeProgram* active_program;
static const NativeScriptVMHost* active_interface;
static uint32_t boost_function;

typedef struct TestHost {
    bool alive;
    float stored,array[4];
    unsigned invokes,writes,retiring_calls,sequence_count;
    unsigned sequence[16];
    bool wrong_output_type;
    bool recycle;
    uint32_t generation;
} TestHost;

static bool name_is(const NativeScriptIRArena* arena,const NativeScriptIRNode* node,const char* name)
{
    return node->text_length==strlen(name) && !memcmp(arena->source+node->text_offset,name,node->text_length);
}

static bool resolve(void* unused,const NativeScriptIRArena* arena,const NativeScriptIRNode* node,
    NativeScriptIRResolution* result,NativeScriptDiagnostic* diagnostic)
{
    (void)unused;
    *result=(NativeScriptIRResolution){NativeScriptType_Namespace,Namespace,0,0};
    unsigned flags=NativeScriptFlag_Readable|NativeScriptFlag_Addressable|NativeScriptFlag_Writable;
    if (node->kind==NativeScriptIR_Name) {
        if (name_is(arena,node,"native") || name_is(arena,node,"state")) return true;
        if (name_is(arena,node,"resource")) { result->type=NativeScriptType_Callable;result->semantic=Resource;return true; }
    } else if (node->kind==NativeScriptIR_Member) {
        if (name_is(arena,node,"retire")) { result->type=NativeScriptType_Callable;result->semantic=Retire;return true; }
        if (name_is(arena,node,"point")) { result->type=NativeScriptType_Callable;result->semantic=Point;return true; }
        if (name_is(arena,node,"retiring_point")) { result->type=NativeScriptType_Callable;result->semantic=RetiringPoint;return true; }
        if (name_is(arena,node,"next_index")) { result->type=NativeScriptType_Callable;result->semantic=NextIndex;return true; }
        if (name_is(arena,node,"echo")) { result->type=NativeScriptType_Callable;result->semantic=Echo;return true; }
        if (name_is(arena,node,"reenter")) { result->type=NativeScriptType_Callable;result->semantic=Reenter;return true; }
        if (name_is(arena,node,"value")) {
            if (name_is(arena,&arena->nodes[node->a],"state")) {
                *result=(NativeScriptIRResolution){NativeScriptType_F32,StateValue,(uint16_t)flags,0};return true;
            }
            result->type=NativeScriptType_Callable;result->semantic=Value;return true;
        }
        if (name_is(arena,node,"values")) { result->semantic=Array;return true; }
        if (name_is(arena,node,"joint")) { result->type=NativeScriptType_Callable;result->semantic=Joint;return true; }
        if (name_is(arena,node,"x")) { *result=(NativeScriptIRResolution){NativeScriptType_F32,JointX,(uint16_t)flags,0};return true; }
    } else if (node->kind==NativeScriptIR_Index) {
        if (arena->nodes[node->a].semantic==Array) {
            *result=(NativeScriptIRResolution){NativeScriptType_F32,Array,(uint16_t)flags,0};return true;
        }
    } else if (node->kind==NativeScriptIR_Call) {
        uint32_t semantic=arena->nodes[node->a].semantic;
        result->semantic=semantic;result->flags=NativeScriptFlag_MayInvalidateSource;
        if (semantic==Resource || semantic==Joint) { result->type=NativeScriptType_Namespace;result->flags=0;return true; }
        if (semantic==Retire && node->argument_count==0) { result->type=NativeScriptType_Bool;return true; }
        if (semantic==Point && node->argument_count==3) { result->type=NativeScriptType_Bool;return true; }
        if (semantic==RetiringPoint && node->argument_count==2) { result->type=NativeScriptType_Bool;return true; }
        if (semantic==NextIndex && node->argument_count==0) { result->type=NativeScriptType_I32;return true; }
        if (semantic==Value && node->argument_count==0) { result->type=NativeScriptType_F32;return true; }
        if (semantic==Echo && node->argument_count==1) { result->type=NativeScriptType_F32;return true; }
        if (semantic==Reenter && node->argument_count==0) { result->type=NativeScriptType_F32;return true; }
    }
    snprintf(diagnostic->message,sizeof(diagnostic->message),"Unknown test binding");return false;
}

static bool describe(void* unused,uint32_t semantic,NativeScriptCallLayout* layout,NativeScriptDiagnostic* diagnostic)
{
    (void)unused;(void)diagnostic;
    *layout=(NativeScriptCallLayout){0};
    if (semantic==Retire || semantic==NextIndex || semantic==Value || semantic==Reenter) return true;
    if (semantic==Point) {
        layout->argument_count=3;layout->argument_types[0]=NativeScriptType_I32;
        layout->argument_types[1]=layout->argument_types[2]=NativeScriptType_F32;
        layout->output_mask=6;layout->publication=NativeScriptOutputs_SuccessOnly;return true;
    }
    if (semantic==RetiringPoint) {
        layout->argument_count=2;layout->argument_types[0]=layout->argument_types[1]=NativeScriptType_F32;
        layout->output_mask=3;layout->publication=NativeScriptOutputs_AlwaysAfterGuard;return true;
    }
    if (semantic==Echo) { layout->argument_count=1;layout->argument_types[0]=NativeScriptType_F32;return true; }
    return false;
}

static bool alive(NativeScriptRuntimeContext* context)
{
    TestHost* host=context->user;
    return host->alive && host->generation==context->generation;
}

static NativeScriptValue f32(float value)
{
    NativeScriptValue result={.type=NativeScriptType_F32};result.as.f32=value;return result;
}

static bool read(NativeScriptRuntimeContext* context,uint32_t semantic,const NativeScriptValue* selectors,
    uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    TestHost* host=context->user;(void)diagnostic;
    if ((semantic&UINT32_C(0xf0000000))==NATIVE_SCRIPT_PARAMETER_SEMANTIC && !count) {
        uint32_t index=semantic&UINT32_C(0x0fffffff);
        if (index>=value_count || value_declarations[index].imported || !value_declarations[index].parameter) return false;
        *result=value_declarations[index].default_value;return true;
    }
    if ((semantic&UINT32_C(0xf0000000))==NATIVE_SCRIPT_STATE_SEMANTIC && !count) { *result=f32(host->stored);return true; }
    if (semantic==StateValue && !count) { *result=f32(host->stored);return true; }
    if (semantic==Array && count==1 && selectors[0].as.i64>=0 && selectors[0].as.i64<4) {
        *result=f32(host->array[selectors[0].as.i64]);return true;
    }
    if (semantic==JointX && count==2 && selectors[0].type==NativeScriptType_I32 &&
        selectors[0].as.i64==3 && selectors[1].type==NativeScriptType_Text) {
        CHECK(selectors[1].as.text.length==1);
        CHECK(source[selectors[1].as.text.offset]=='0');
        *result=f32(4);return true;
    }
    return false;
}

static bool write(NativeScriptRuntimeContext* context,uint32_t semantic,const NativeScriptValue* selectors,
    uint32_t count,const NativeScriptValue* value,NativeScriptDiagnostic* diagnostic)
{
    TestHost* host=context->user;(void)diagnostic;
    CHECK(value->type==NativeScriptType_F32);
    if (context->source) *(volatile float*)context->source=value->as.f32;
    ++host->writes;
    if ((semantic&UINT32_C(0xf0000000))==NATIVE_SCRIPT_STATE_SEMANTIC && !count) { host->stored=value->as.f32;return true; }
    if (semantic==StateValue && !count) { host->stored=value->as.f32;return true; }
    if (semantic==Array && count==1 && selectors[0].as.i64>=0 && selectors[0].as.i64<4) {
        host->array[selectors[0].as.i64]=value->as.f32;
        host->sequence[host->sequence_count++]=3;return true;
    }
    return false;
}

static bool invoke(NativeScriptRuntimeContext* context,uint32_t semantic,const NativeScriptValue* selectors,
    uint32_t selector_count,NativeScriptValue* values,uint32_t count,
    NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    TestHost* host=context->user;(void)selectors;(void)diagnostic;
    CHECK(selector_count==0);++host->invokes;
    if (semantic==Retire || semantic==RetiringPoint) {
        ++host->retiring_calls;
        if (host->recycle) ++host->generation; else host->alive=false;
#ifdef _WIN32
        if (context->source) {
            DWORD old_protection;
            CHECK(VirtualProtect(context->source,4096,PAGE_NOACCESS,&old_protection)!=0);
        }
#endif
        if (semantic==RetiringPoint) {
            CHECK(count==2);values[0]=f32(1);values[1]=f32(2);
        }
        *result=(NativeScriptValue){.type=NativeScriptType_Bool};result->as.u64=1;return true;
    }
    if (semantic==Point) {
        CHECK(count==3);CHECK(values[0].type==NativeScriptType_I32);
        values[1]=f32(5);values[2]=f32(6);
        if (host->wrong_output_type) values[2].type=NativeScriptType_F64;
        *result=(NativeScriptValue){.type=NativeScriptType_Bool};result->as.u64=values[0].as.i64!=0;return true;
    }
    if (semantic==NextIndex) {
        CHECK(count==0);host->sequence[host->sequence_count++]=1;
        *result=(NativeScriptValue){.type=NativeScriptType_I32};result->as.i64=2;return true;
    }
    if (semantic==Value) {
        CHECK(count==0);host->sequence[host->sequence_count++]=2;*result=f32(7);return true;
    }
    if (semantic==Echo) { CHECK(count==1);CHECK(values[0].type==NativeScriptType_F32);*result=values[0];return true; }
    if (semantic==Reenter) {
        CHECK(count==0);uint32_t depth=active_vm->depth,top=active_vm->register_top;
        NativeScriptValue argument=f32(4);
        CHECK(nativeScriptVM_Run(active_vm,active_program,active_interface,context,boost_function,&argument,1,result)==NativeScriptVM_Ok);
        CHECK(active_vm->depth==depth && active_vm->register_top==top);return true;
    }
    return false;
}

static uint32_t function_id(const char* name)
{
    for (unsigned i=0;i<function_count;++i) if (!strcmp(names[i].name,name)) return i;
    CHECK(false);return UINT32_MAX;
}

static void numeric_tests(void)
{
    NativeScriptDiagnostic diagnostic={0};NativeScriptValue result;
    NativeScriptValue a={.type=NativeScriptType_I32},b={.type=NativeScriptType_I32};
    a.as.i64=INT32_MAX;b.as.i64=1;
    CHECK(nativeScriptValue_Binary(NativeScriptOp_Add,&a,&b,&result,&diagnostic));CHECK(result.as.i64==INT32_MIN);
    a.as.i64=INT32_MIN;b.as.i64=-1;
    CHECK(nativeScriptValue_Binary(NativeScriptOp_Divide,&a,&b,&result,&diagnostic));CHECK(result.as.i64==INT32_MIN);
    CHECK(nativeScriptValue_Binary(NativeScriptOp_Remainder,&a,&b,&result,&diagnostic));CHECK(result.as.i64==0);
    b.as.i64=0;CHECK(!nativeScriptValue_Binary(NativeScriptOp_Divide,&a,&b,&result,&diagnostic));
    a.as.i64=-8;b.as.i64=2;CHECK(nativeScriptValue_Binary(NativeScriptOp_ShiftRight,&a,&b,&result,&diagnostic));CHECK(result.as.i64==-2);
    b.as.i64=32;CHECK(!nativeScriptValue_Binary(NativeScriptOp_ShiftLeft,&a,&b,&result,&diagnostic));
    a.as.i64=-1;CHECK(nativeScriptValue_Convert(&a,NativeScriptType_U32,&result,&diagnostic));CHECK(result.as.u64==UINT32_MAX);
    CHECK(nativeScriptValue_Convert(&result,NativeScriptType_I32,&result,&diagnostic));CHECK(result.as.i64==-1);
    a=(NativeScriptValue){.type=NativeScriptType_U64};a.as.u64=UINT64_MAX;
    CHECK(nativeScriptValue_Convert(&a,NativeScriptType_I64,&result,&diagnostic));CHECK(result.as.i64==-1);
    a=(NativeScriptValue){.type=NativeScriptType_F64};a.as.f64=16777217.0;
    CHECK(nativeScriptValue_Convert(&a,NativeScriptType_F32,&result,&diagnostic));CHECK(result.as.f32==16777216.0f);
    a.as.f64=NAN;CHECK(!nativeScriptValue_Convert(&a,NativeScriptType_I32,&result,&diagnostic));
    CHECK(nativeScriptValue_Convert(&a,NativeScriptType_Bool,&result,&diagnostic));CHECK(result.as.u64==1);
    a.as.f64=2147483648.0;CHECK(!nativeScriptValue_Convert(&a,NativeScriptType_I32,&result,&diagnostic));
    a.as.f64=-2147483648.0;CHECK(nativeScriptValue_Convert(&a,NativeScriptType_I32,&result,&diagnostic));CHECK(result.as.i64==INT32_MIN);
    a.as.f64=18446744073709551616.0;CHECK(!nativeScriptValue_Convert(&a,NativeScriptType_U64,&result,&diagnostic));
    a=f32(16777216.0f);b=f32(1);CHECK(nativeScriptValue_Binary(NativeScriptOp_Add,&a,&b,&result,&diagnostic));CHECK(result.as.f32==16777216.0f);
    a=(NativeScriptValue){.type=NativeScriptType_F64};b=a;a.as.f64=16777216.0;b.as.f64=1;
    CHECK(nativeScriptValue_Binary(NativeScriptOp_Add,&a,&b,&result,&diagnostic));CHECK(result.as.f64==16777217.0);
}

static void fallthrough_test(const char* text)
{
    NativeScriptSourceNode source_nodes[128];NativeScriptIRNode expression_nodes[128];uint32_t call_arguments[128];
    NativeScriptLocal source_locals[16];NativeScriptFunctionDeclaration functions_index[8];
    NativeScriptParameterDeclaration params[8];NativeScriptValueDeclaration values[8];
    NativeScriptInstruction code[128];NativeScriptFunctionCode function_code[8];
    NativeScriptDiagnostic diagnostic;
    NativeScriptSourceTree tree={text,(uint32_t)strlen(text),source_nodes,0,128};
    NativeScriptIRArena arena={text,(uint32_t)strlen(text),expression_nodes,0,128,call_arguments,0,128};
    NativeScriptProgramIndex index={.tree=&tree,.expressions=&arena,
        .functions=functions_index,.function_capacity=8,.parameters=params,.parameter_capacity=8,
        .values=values,.value_capacity=8};
    CHECK(nativeScriptSource_Parse(&tree,&diagnostic));CHECK(nativeScriptProgram_Index(&index,&diagnostic));
    NativeScriptProgramCompiler compiler={.index=&index,.native_bindings=resolve,.locals=source_locals,.local_capacity=16};
    NativeScriptStatementCompiler body;
    CHECK(nativeScriptProgram_CompileBody(&compiler,0,&body,&diagnostic));
    NativeScriptBytecodeBuilder builder={code,0,128,function_code,1,describe,NULL};
    CHECK(!nativeScriptBytecode_Compile(&builder,&body,functions_index[0].source_node,0,
        (uint16_t)functions_index[0].parameter_count,&diagnostic));
    CHECK(builder.instruction_count==0);CHECK(strstr(diagnostic.message,"path")!=NULL);CHECK(diagnostic.source.line==1);
}

int main(int argc,char** argv)
{
    CHECK(argc==2);FILE* file=fopen(argv[1],"rb");CHECK(file!=NULL);
    unsigned bytes=(unsigned)fread(source,1,sizeof(source)-1,file);fclose(file);
    NativeScriptDiagnostic diagnostic;
    NativeScriptSourceTree tree={source,bytes,syntax,0,2048};
    CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
    NativeScriptIRArena arena={source,bytes,nodes,0,8192,arguments,0,8192};
    NativeScriptProgramIndex index={.tree=&tree,.expressions=&arena,
        .functions=declarations,.function_capacity=32,
        .parameters=parameter_declarations,.parameter_capacity=64,
        .values=value_declarations,.value_capacity=64};
    CHECK(nativeScriptProgram_Index(&index,&diagnostic));
    function_count=index.function_count;
    value_count=index.value_count;
    for (unsigned i=0;i<function_count;++i) {
        FunctionName* name=&names[i];const NativeScriptFunctionDeclaration* declaration=&declarations[i];
        memcpy(name->name,source+declaration->name.offset,declaration->name.length);
        name->source=declaration->source_node;name->result=declaration->result_type;
        name->parameters=declaration->parameter_count;
    }
    NativeScriptBytecodeBuilder builder={instructions,0,8192,functions,function_count,describe,NULL};
    NativeScriptProgramCompiler program_compiler={.index=&index,.native_bindings=resolve,
        .locals=locals,.local_capacity=256};
    for (unsigned i=0;i<function_count;++i) {
        NativeScriptStatementCompiler compiler;
        if (!nativeScriptProgram_CompileBody(&program_compiler,i,&compiler,&diagnostic)) {
            fprintf(stderr,"compile %s line %u: %s\n",names[i].name,diagnostic.source.line,diagnostic.message);return 1;
        }
        if (!nativeScriptBytecode_Compile(&builder,&compiler,names[i].source,i,(uint16_t)names[i].parameters,&diagnostic)) {
            fprintf(stderr,"lower %s line %u: %s\n",names[i].name,diagnostic.source.line,diagnostic.message);return 1;
        }
    }
    NativeScriptBytecodeProgram program={source,bytes,instructions,builder.instruction_count,functions,function_count,false}; { NativeScriptDiagnostic preparation={0}; if(!nativeScriptBytecode_Prepare(&program,&preparation)){fprintf(stderr,"bytecode preparation: %s\n",preparation.message);abort();} }
    NativeScriptVM vm={.registers=registers,.register_capacity=4096,.frames=frames,.frame_capacity=32};
    NativeScriptVMHost host_interface={alive,read,write,invoke};
    active_vm=&vm;active_program=&program;active_interface=&host_interface;boost_function=function_id("Boost");
    TestHost host={.alive=true,.stored=-1,.generation=1};
    NativeScriptRuntimeContext context={.user=&host,.generation=1,.frame=123,.actor_id=17};
    NativeScriptValue result=f32(-999);
    NativeScriptVMStatus status=nativeScriptVM_Run(&vm,&program,&host_interface,&context,function_id("Evaluate"),NULL,0,&result);
    if (status!=NativeScriptVM_Ok) { fprintf(stderr,"execute line %u: %s\n",vm.diagnostic.source.line,vm.diagnostic.message);return 1; }
    CHECK(result.type==NativeScriptType_F32);CHECK(result.as.f32==111.25f);CHECK(host.stored==111.25f);
    CHECK(host.array[2]==7);CHECK(host.retiring_calls==0);CHECK(host.sequence_count==3);
    CHECK(host.sequence[0]==1 && host.sequence[1]==2 && host.sequence[2]==3);
    CHECK(vm.depth==0 && vm.register_top==0);
    CHECK(nativeScriptVM_Run(&vm,&program,&host_interface,&context,function_id("CallerRounding"),NULL,0,&result)==NativeScriptVM_Ok);
    CHECK(result.as.f32==16777216.0f);
    uint32_t numeric_seed=UINT32_C(0xb419735d);
    for (unsigned sample=0;sample<4096;++sample) {
        numeric_seed=numeric_seed*UINT32_C(1664525)+UINT32_C(1013904223);
        float left=(float)((int32_t)(numeric_seed&UINT32_C(0x07ffffff))-INT32_C(0x04000000))/17.0f;
        numeric_seed=numeric_seed*UINT32_C(1664525)+UINT32_C(1013904223);
        float right=(float)((int32_t)(numeric_seed&UINT32_C(0x07ffffff))-INT32_C(0x04000000))/31.0f;
        NativeScriptValue inputs[2]={f32(left),f32(right)};
        float expected=(float)((double)(left+right)*0.5+0.25);
        CHECK(nativeScriptVM_Run(&vm,&program,&host_interface,&context,function_id("NumericProbe"),inputs,2,&result)==NativeScriptVM_Ok);
        CHECK(result.type==NativeScriptType_F32 && result.as.f32==expected);
    }
    NativeScriptValue mode={.type=NativeScriptType_I32};mode.as.i64=-1;
    CHECK(nativeScriptVM_Run(&vm,&program,&host_interface,&context,function_id("BranchReturn"),&mode,1,&result)==NativeScriptVM_Ok);CHECK(result.as.i64==1);
    mode.as.i64=0;
    CHECK(nativeScriptVM_Run(&vm,&program,&host_interface,&context,function_id("BranchReturn"),&mode,1,&result)==NativeScriptVM_Ok);CHECK(result.as.i64==2);
    mode.as.i64=1;
    CHECK(nativeScriptVM_Run(&vm,&program,&host_interface,&context,function_id("BranchReturn"),&mode,1,&result)==NativeScriptVM_Ok);CHECK(result.as.i64==3);
    CHECK(nativeScriptVM_Run(&vm,&program,&host_interface,&context,function_id("BranchThenReturn"),&mode,1,&result)==NativeScriptVM_Ok);CHECK(result.as.i64==2);
    CHECK(nativeScriptVM_Run(&vm,&program,&host_interface,&context,function_id("Reentrant"),NULL,0,&result)==NativeScriptVM_Ok);
    CHECK(result.as.f32==5.25f);CHECK(vm.depth==0 && vm.register_top==0);
    CHECK(nativeScriptVM_Run(&vm,&program,&host_interface,&context,function_id("ReturnNested"),NULL,0,&result)==NativeScriptVM_Ok);CHECK(result.as.i64==6);
    host.wrong_output_type=true;host.writes=0;
    CHECK(nativeScriptVM_Run(&vm,&program,&host_interface,&context,function_id("Evaluate"),NULL,0,&result)==NativeScriptVM_Error);
    CHECK(strstr(vm.diagnostic.message,"output type")!=NULL);CHECK(host.writes==0);host.wrong_output_type=false;
    host=(TestHost){.alive=true,.stored=-1,.generation=1};result=f32(-999);
    CHECK(nativeScriptVM_Run(&vm,&program,&host_interface,&context,function_id("RetireHelper"),NULL,0,&result)==NativeScriptVM_SourceRetired);
    CHECK(host.writes==0 && host.invokes==1 && host.retiring_calls==1);
    CHECK(result.as.f32==-999);CHECK(vm.depth==0 && vm.register_top==0);
    CHECK(vm.error_actor==17 && vm.error_frame==123 && vm.error_function==function_id("Retire"));CHECK(vm.diagnostic.source.line!=0);
    host=(TestHost){.alive=true,.stored=-1,.generation=1,.recycle=true};
    CHECK(nativeScriptVM_Run(&vm,&program,&host_interface,&context,function_id("Retire"),NULL,0,&result)==NativeScriptVM_SourceRetired);
    CHECK(host.alive && host.generation==2 && host.writes==0);
#ifdef _WIN32
    void* protected_actor=VirtualAlloc(NULL,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);CHECK(protected_actor!=NULL);
    context.source=protected_actor;host=(TestHost){.alive=true,.stored=-1,.generation=1};result=f32(-999);
    CHECK(nativeScriptVM_Run(&vm,&program,&host_interface,&context,function_id("RetireHelper"),NULL,0,&result)==NativeScriptVM_SourceRetired);
    CHECK(host.writes==0 && result.as.f32==-999 && vm.depth==0);
    CHECK(VirtualFree(protected_actor,0,MEM_RELEASE)!=0);context.source=NULL;
#endif
    host.alive=true;
    host.generation=1;
    CHECK(nativeScriptVM_Run(&vm,&program,&host_interface,&context,function_id("BadDivide"),NULL,0,&result)==NativeScriptVM_Error);
    CHECK(strstr(vm.diagnostic.message,"division by zero")!=NULL);
    vm.register_capacity=1;
    CHECK(nativeScriptVM_Run(&vm,&program,&host_interface,&context,function_id("Evaluate"),NULL,0,&result)==NativeScriptVM_Error);
    CHECK(strstr(vm.diagnostic.message,"pool exhausted")!=NULL);vm.register_capacity=4096;
    vm.frame_capacity=1;
    CHECK(nativeScriptVM_Run(&vm,&program,&host_interface,&context,function_id("CallerRounding"),NULL,0,&result)==NativeScriptVM_Error);
    CHECK(vm.depth==0 && vm.register_top==0);vm.frame_capacity=32;
    numeric_tests();
    preparation_tests();
    fallthrough_test("func Missing(int value) -> int:\n    if value: return 1\n");
    fallthrough_test("func Missing(int value) -> int:\n    while value:\n        return 1\n");
    fallthrough_test("func Missing(int value) -> int:\n    if value: return 1\n    else: pass\n");
    printf("{\"result\":\"pass\",\"checks\":%u,\"functions\":%u,\"instructions\":%u}\n",checks,function_count,builder.instruction_count);
    return 0;
}
