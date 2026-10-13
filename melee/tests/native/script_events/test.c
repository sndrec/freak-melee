#include <melee/ft/native_script_events.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { SourceBytes=1048576, SourceNodes=65536, ExpressionNodes=16384,
       Functions=2048, Parameters=8192 };
static char source[SourceBytes];
static NativeScriptSourceNode source_nodes[SourceNodes];
static NativeScriptIRNode expression_nodes[ExpressionNodes];
static uint32_t expression_arguments[ExpressionNodes];
static NativeScriptFunctionDeclaration declarations[Functions];
static NativeScriptParameterDeclaration parameters[Parameters];
static NativeScriptValueDeclaration values[2048];
static NativeScriptLocal locals[128];
static NativeScriptInstruction instructions[16384];
static NativeScriptFunctionCode code[Functions];
static NativeScriptValue registers[1024];
static NativeScriptVMFrame frames[16];
static unsigned checks;
#define CHECK(value) do {++checks;if (!(value)) {fprintf(stderr,"line %u: %s\n",__LINE__,#value);exit(1);}} while(0)

typedef struct PayloadScope { const NativeScriptEventDescriptor* event; } PayloadScope;

static void json_text(const char* text,uint32_t length)
{
    putchar('"');
    for (uint32_t i=0;i<length;++i) {
        unsigned char c=(unsigned char)text[i];
        if (c=='"' || c=='\\') printf("\\%c",c);
        else if (c<32) printf("\\u%04x",c);
        else putchar(c);
    }
    putchar('"');
}

static bool token_is(const NativeScriptIRArena* arena,const NativeScriptIRNode* node,const char* text)
{
    return node->text_length==strlen(text) && !memcmp(arena->source+node->text_offset,text,node->text_length);
}

static bool payload(void* opaque,const NativeScriptIRArena* arena,const NativeScriptIRNode* node,
    NativeScriptIRResolution* result,NativeScriptDiagnostic* diagnostic)
{
    PayloadScope* scope=opaque;
    if (node->kind==NativeScriptIR_Name && (token_is(arena,node,"event") || token_is(arena,node,"signal"))) {
        *result=(NativeScriptIRResolution){NativeScriptType_Namespace,1,0,0};return true;
    }
    if (node->kind==NativeScriptIR_Member && scope->event) {
        const NativeScriptIRNode* base=&arena->nodes[node->a];
        for (uint32_t i=0;i<scope->event->argument_count;++i) {
            const char* property=scope->event->arguments[i].property;
            const char* dot=strchr(property,'.');
            if (dot && base->text_length==(uint32_t)(dot-property) &&
                !memcmp(arena->source+base->text_offset,property,(size_t)(dot-property)) && token_is(arena,node,dot+1)) {
                *result=(NativeScriptIRResolution){(NativeScriptIRType)scope->event->arguments[i].type,
                    NATIVE_SCRIPT_LOCAL_SEMANTIC|i,NativeScriptFlag_Readable,0};return true;
            }
        }
    }
    snprintf(diagnostic->message,sizeof(diagnostic->message),"Unknown/read-only test event payload");return false;
}

static bool alive(NativeScriptRuntimeContext* context) { (void)context;return true; }
static NativeScriptValue integer(int32_t value)
{
    NativeScriptValue result={.type=NativeScriptType_I32};result.as.i64=value;return result;
}

static NativeScriptSourceTree parse_file(const char* path)
{
    FILE* input=fopen(path,"rb");CHECK(input!=NULL);
    uint32_t bytes=(uint32_t)fread(source,1,sizeof(source),input);CHECK(feof(input));fclose(input);
    NativeScriptSourceTree tree={source,bytes,source_nodes,0,SourceNodes};NativeScriptDiagnostic diagnostic;
    if (!nativeScriptSource_Parse(&tree,&diagnostic)) {
        fprintf(stderr,"parse %s line %u: %s\n",path,diagnostic.source.line,diagnostic.message);exit(1);
    }
    return tree;
}

static void rejected(const char* text,const char* message)
{
    NativeScriptSourceTree tree={text,(uint32_t)strlen(text),source_nodes,0,SourceNodes};
    NativeScriptDiagnostic diagnostic;CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
    for (uint32_t i=1;i<tree.count;++i) if (source_nodes[i].kind==NativeScriptSource_Callback) {
        NativeScriptEventResolution resolution;
        CHECK(!nativeScriptEvents_Resolve(&tree,&source_nodes[i],&resolution,&diagnostic));
        CHECK(strstr(diagnostic.message,message)!=NULL);return;
    }
    CHECK(false);
}

static void rejected_body(const char* text,const char* message)
{
    NativeScriptSourceTree tree={text,(uint32_t)strlen(text),source_nodes,0,SourceNodes};
    NativeScriptDiagnostic diagnostic;CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
    NativeScriptIRArena arena={text,tree.source_bytes,expression_nodes,0,ExpressionNodes,expression_arguments,0,ExpressionNodes};
    NativeScriptProgramIndex index={.tree=&tree,.expressions=&arena,.functions=declarations,.function_capacity=Functions,
        .parameters=parameters,.parameter_capacity=Parameters,.values=values,.value_capacity=2048,
        .event_signature=nativeScriptEvents_Signature};
    CHECK(nativeScriptProgram_Index(&index,&diagnostic));
    PayloadScope scope={0};NativeScriptProgramCompiler compiler={.index=&index,.native_bindings=payload,
        .native_binding_context=&scope,.locals=locals,.local_capacity=128};
    for (uint32_t i=0;i<index.function_count;++i) if (declarations[i].callback) {
        NativeScriptEventResolution resolution;CHECK(nativeScriptEvents_Resolve(&tree,&source_nodes[declarations[i].source_node],&resolution,&diagnostic));
        scope.event=resolution.descriptor;NativeScriptStatementCompiler body;
        CHECK(!nativeScriptProgram_CompileBody(&compiler,i,&body,&diagnostic));
        CHECK(strstr(diagnostic.message,message)!=NULL);return;
    }
    CHECK(false);
}

static void unit(const char* path)
{
    NativeScriptSourceTree tree=parse_file(path);NativeScriptDiagnostic diagnostic;
    NativeScriptIRArena arena={source,tree.source_bytes,expression_nodes,0,ExpressionNodes,expression_arguments,0,ExpressionNodes};
    NativeScriptProgramIndex index={.tree=&tree,.expressions=&arena,.functions=declarations,.function_capacity=Functions,
        .parameters=parameters,.parameter_capacity=Parameters,.values=values,.value_capacity=2048,
        .event_signature=nativeScriptEvents_Signature};
    CHECK(nativeScriptProgram_Index(&index,&diagnostic));
    NativeScriptBytecodeBuilder builder={.instructions=instructions,.instruction_capacity=16384,
        .functions=code,.function_count=index.function_count};
    PayloadScope scope={0};NativeScriptProgramCompiler compiler={.index=&index,.native_bindings=payload,
        .native_binding_context=&scope,.locals=locals,.local_capacity=128};
    for (uint32_t i=0;i<index.function_count;++i) {
        const NativeScriptFunctionDeclaration* declaration=&declarations[i];
        NativeScriptEventResolution resolution;
        if (declaration->callback) {
            CHECK(nativeScriptEvents_Resolve(&tree,&source_nodes[declaration->source_node],&resolution,&diagnostic));
            scope.event=resolution.descriptor;
        } else scope.event=NULL;
        NativeScriptStatementCompiler body;CHECK(nativeScriptProgram_CompileBody(&compiler,i,&body,&diagnostic));
        CHECK(nativeScriptBytecode_Compile(&builder,&body,declaration->source_node,i,(uint16_t)declaration->parameter_count,&diagnostic));
        for (uint32_t p=0;p<declaration->parameter_count;++p) CHECK(parameters[declaration->parameter_first+p].name.length==0);
    }
    NativeScriptBytecodeProgram program={source,tree.source_bytes,instructions,builder.instruction_count,code,index.function_count,false}; { NativeScriptDiagnostic preparation={0}; if(!nativeScriptBytecode_Prepare(&program,&preparation)){fprintf(stderr,"bytecode preparation: %s\n",preparation.message);abort();} }
    NativeScriptVM vm={.registers=registers,.register_capacity=1024,.frames=frames,.frame_capacity=16};
    NativeScriptVMHost host={.source_alive=alive};NativeScriptRuntimeContext context={0};
    uint32_t void_count=0,bool_count=0,integer_count=0,float_count=0,callback_count=0;
    for (uint32_t i=0;i<index.function_count;++i) {
        if (!declarations[i].callback) {
            NativeScriptValue result;
            CHECK(nativeScriptVM_Run(&vm,&program,&host,&context,i,NULL,0,&result)==NativeScriptVM_Ok);
            CHECK(result.type==NativeScriptType_Bool);
            CHECK(result.as.u64==(nativeScriptSource_SpanEquals(&tree,declarations[i].name,"HelperFalse") ? 0 : 1));
            continue;
        }
        ++callback_count;
        NativeScriptEventResolution resolution;CHECK(nativeScriptEvents_Resolve(&tree,&source_nodes[declarations[i].source_node],&resolution,&diagnostic));
        const NativeScriptEventDescriptor* event=resolution.descriptor;
        NativeScriptValue args[4],result;
        for (uint32_t a=0;a<event->argument_count;++a) {
            args[a]=integer(a==0?100:200);args[a].type=event->arguments[a].type;
            if (args[a].type==NativeScriptType_F32) args[a].as.f32=2.0f;
            if (args[a].type==NativeScriptType_Bool) args[a].as.u64=1;
            if (args[a].type==NativeScriptType_Object) args[a].as.object=(NativeScriptObjectValue){3,4,1};
        }
        CHECK(nativeScriptVM_Run(&vm,&program,&host,&context,i,args,event->argument_count,&result)==NativeScriptVM_Ok);
        CHECK(result.type==event->result_type);
        if (result.type==NativeScriptType_Void) ++void_count;
        else if (result.type==NativeScriptType_Bool) {
            ++bool_count;
            CHECK(result.as.u64==(!strcmp(event->name,"collision") || !strcmp(event->name,"shield_alpha") ? 1 : 0));
        } else if (result.type==NativeScriptType_F32) { ++float_count;CHECK(result.as.f32==0); }
        else if (result.type==NativeScriptType_I32) {
            ++integer_count;
            int32_t expected=event->fallthrough==NativeScriptEvent_ReturnArgument ? (event->fallthrough_argument==0?100:200) :
                event->fallthrough==NativeScriptEvent_ReturnMinusOne ? -1 : !strcmp(event->name,"cpu_prepare") ? 100 : 0;
            CHECK(result.as.i64==expected);
        }
        if (!strcmp(event->name,"held_motion")) {
            args[0]=integer(-10);CHECK(nativeScriptVM_Run(&vm,&program,&host,&context,i,args,event->argument_count,&result)==NativeScriptVM_Ok);CHECK(result.as.i64==7);
        }
        if (!strcmp(event->name,"action_metadata")) {
            args[0]=integer(1);CHECK(nativeScriptVM_Run(&vm,&program,&host,&context,i,args,event->argument_count,&result)==NativeScriptVM_Ok);CHECK(result.as.i64==9);
        }
        if (!strcmp(event->name,"cpu_prepare")) {
            args[0]=(NativeScriptValue){.type=NativeScriptType_U32};args[0].as.u64=UINT32_C(0xfedcba98);
            CHECK(nativeScriptVM_Run(&vm,&program,&host,&context,i,args,event->argument_count,&result)==NativeScriptVM_Ok);CHECK((uint32_t)result.as.i64==UINT32_C(0xfedcba98));
        }
        if (!strcmp(event->name,"held_status")) CHECK((event->body_policy&NativeScriptEvent_ReadOnlyBody)!=0);
        if (event->kind==NativeScriptEvent_ArticleSignal) CHECK(resolution.signal_name.length==6);
        if (event->kind==NativeScriptEvent_FighterMotion) CHECK(resolution.motion==350);
    }
    uint32_t callbacks=callback_count,emitted=builder.instruction_count;
    rejected("action Bad:\n    on input foo:\n        pass\n","unsupported declarative");
    rejected("action Bad:\n    on setup 3:\n        pass\n","unsupported declarative");
    rejected("model_effect Bad 1:\n    on unknown_event:\n        pass\n","Unknown event");
    rejected("article It_Kind_Mario_Fire states 1:\n    on animation:\n        pass\n","Unknown event");
    rejected("move Bad:\n    on animation:\n        pass\n","Unknown event");
    rejected("action Bad:\n    on special_n:\n        pass\n","Special entry");
    rejected("move Bad:\n    on grabbed:\n        pass\n","Capture callback");
    rejected("action Bad:\n    on trail unexpected:\n        pass\n","Unknown event");
    rejected("article It_Kind_Mario_Fire states 1:\n    on signal bad 3:\n        pass\n","unsupported declarative");
    rejected("article It_Kind_Mario_Fire states 1:\n    state 2 anim -1:\n        on physics:\n            pass\n","in-range");
    rejected("article It_Kind_Mario_Fire states 1:\n    state 0 unexpected 0:\n        on physics:\n            pass\n","anim A");
    rejected_body("action Policy:\n    on wait_input:\n        return\n","return value");
    rejected_body("article It_Kind_Mario_Fire states 1:\n    state int counter\n    on held_status:\n        state.counter = 1\n","Read-only");
    rejected_body("article It_Kind_Mario_Fire states 1:\n    on held_status:\n        while true:\n            pass\n","Read-only");
    rejected_body("func Helper() -> int:\n    return 1\narticle It_Kind_Mario_Fire states 1:\n    on held_status:\n        return Helper()\n","Read-only");
    rejected_body("action Policy:\n    on cpu_mode_update:\n        return 1\n","exact return");
    rejected_body("action Policy:\n    on cpu_prepare:\n        return true\n","exact return");
    rejected_body("article It_Kind_Mario_Fire states 1:\n    on held_status:\n        return false\n","exact return");
    rejected_body("article It_Kind_Mario_Fire states 1:\n    on held_request:\n        return 0.5\n","exact return");
    printf("{\"result\":\"pass\",\"checks\":%u,\"callbacks\":%u,\"instructions\":%u,\"void\":%u,\"bool\":%u,\"int\":%u,\"float\":%u}\n",
        checks,callbacks,emitted,void_count,bool_count,integer_count,float_count);
}

static void census(const char* path)
{
    NativeScriptSourceTree tree=parse_file(path);bool comma=false;uint32_t resolved=0,unresolved=0;
    printf("{\"path\":");json_text(path,(uint32_t)strlen(path));printf(",\"callbacks\":[");
    for (uint32_t i=1;i<tree.count;++i) if (source_nodes[i].kind==NativeScriptSource_Callback) {
        const NativeScriptSourceNode* node=&source_nodes[i];NativeScriptDiagnostic diagnostic={0};NativeScriptEventResolution resolution;
        bool valid=nativeScriptEvents_Resolve(&tree,node,&resolution,&diagnostic);
        if (comma) putchar(',');comma=true;
        printf("{\"line\":%u,\"name\":",node->source.line);json_text(source+node->name.offset,node->name.length);
        printf(",\"resolved\":%s",valid?"true":"false");
        if (valid) ++resolved;else ++unresolved;
        if (resolution.descriptor) {
            const NativeScriptEventDescriptor* descriptor=resolution.descriptor;
            printf(",\"event_id\":%u,\"context\":%u,\"kind\":%u,\"result_type\":",resolution.event_id,descriptor->context,descriptor->kind);
            const char* type=nativeScriptExpression_TypeName((NativeScriptIRType)descriptor->result_type);json_text(type,(uint32_t)strlen(type));
            printf(",\"native_slot\":");json_text(descriptor->native_slot,(uint32_t)strlen(descriptor->native_slot));
            printf(",\"native_signature\":");json_text(descriptor->native_signature,(uint32_t)strlen(descriptor->native_signature));
            printf(",\"source_evidence\":");json_text(descriptor->source_evidence,(uint32_t)strlen(descriptor->source_evidence));
            printf(",\"arguments\":[");
            for (unsigned a=0;a<descriptor->argument_count;++a) {
                if (a) putchar(',');printf("{\"property\":");json_text(descriptor->arguments[a].property,(uint32_t)strlen(descriptor->arguments[a].property));
                printf(",\"type\":");type=nativeScriptExpression_TypeName((NativeScriptIRType)descriptor->arguments[a].type);json_text(type,(uint32_t)strlen(type));putchar('}');
            }
            printf("],\"body_policy\":%u,\"fallthrough\":%u,\"fallthrough_argument\":%u",descriptor->body_policy,descriptor->fallthrough,descriptor->fallthrough_argument);
        }
        if (!valid) { printf(",\"diagnostic\":");json_text(diagnostic.message,(uint32_t)strlen(diagnostic.message)); }
        putchar('}');
    }
    printf("],\"resolved\":%u,\"unresolved\":%u}",resolved,unresolved);
}

int main(int argc,char** argv)
{
    CHECK(argc>=3);
    if (!strcmp(argv[1],"unit")) unit(argv[2]);
    else if (!strcmp(argv[1],"census")) {
        putchar('[');for (int i=2;i<argc;++i) {if (i!=2)putchar(',');census(argv[i]);}puts("]");
    } else CHECK(false);
    return 0;
}
