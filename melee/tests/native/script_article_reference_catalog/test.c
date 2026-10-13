#include <melee/ft/native_script_compile.h>
#include <melee/ft/native_script_binding_article_reference_resolve.h>
#include <melee/ft/native_script_binding_receiver.h>
#include <melee/ft/native_script_binding_article_reference.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_script_constants.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum{Bytes=16384,Nodes=512,Expr=2048,Functions=64,Values=64,Instructions=4096};
typedef struct Buffers{
    char source[Bytes];NativeScriptSourceNode nodes[Nodes];NativeScriptIRNode expressions[Expr];uint32_t arguments[Expr];
    NativeScriptFunctionDeclaration functions[Functions];NativeScriptParameterDeclaration parameters[128];NativeScriptValueDeclaration values[Values];NativeScriptLocal locals[64];
    NativeScriptInstruction instructions[Instructions];NativeScriptFunctionCode code[Functions];NativeScriptExecutionFunction execution[Functions];NativeScriptCompileFunctionScratch graph[Functions];
    NativeScriptStorageField fields[Values];NativeScriptStorageOwner owners[16];NativeScriptParameterSlot slots[Values];
}Buffers;
static Buffers buffers[2];static NativeScriptCompileBank banks[2];static unsigned checks,calls;static uint32_t captured_semantic,captured_count;static NativeScriptValue captured[6];static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"signal catalog %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
bool nativeScriptBindings_SourceAlive(NativeScriptRuntimeContext* c){return c&&c->generation==17;}
bool nativeScriptBindingArticleReference_Read(const NativeScriptReceiverScope* s,NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* v,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{(void)s;(void)c;(void)semantic;(void)v;(void)count;(void)out;(void)d;return false;}
bool nativeScriptBindingArticleReference_Invoke(const NativeScriptReceiverScope* s,NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* selectors,uint32_t selector_count,NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{(void)s;(void)c;(void)selectors;(void)out;(void)d;CHECK(selector_count==0);CHECK(count==2||count==5||count==6);captured_semantic=semantic;captured_count=count;for(unsigned i=0;i<count;++i)captured[i]=args[i];++calls;return false;}
static void setup(unsigned index)
{
    Buffers* b=&buffers[index];banks[index]=(NativeScriptCompileBank){.source_storage=b->source,.source_capacity=Bytes,.tree={.nodes=b->nodes,.capacity=Nodes},
        .expressions={.nodes=b->expressions,.node_capacity=Expr,.arguments=b->arguments,.argument_capacity=Expr},
        .declarations={.functions=b->functions,.function_capacity=Functions,.parameters=b->parameters,.parameter_capacity=128,.values=b->values,.value_capacity=Values},
        .locals=b->locals,.local_capacity=64,.instructions=b->instructions,.instruction_capacity=Instructions,.function_code=b->code,.function_code_capacity=Functions,
        .execution_functions=b->execution,.execution_function_capacity=Functions,.graph=b->graph,.graph_capacity=Functions,
        .storage_layout={.fields=b->fields,.field_capacity=Values,.owners=b->owners,.owner_capacity=16},.parameter_slots=b->slots,.parameter_slot_capacity=Values};
}
static bool metadata(void* unused,const NativeScriptSourceTree* tree,uint32_t at,NativeScriptDiagnostic* d)
{
    (void)unused;const NativeScriptSourceNode* node=&tree->nodes[at];
    if(node->kind==NativeScriptSource_Article)return nativeScriptSource_SpanEquals(tree,node->value,"states 1");
    if(node->kind==NativeScriptSource_ArticleState)return nativeScriptSource_SpanEquals(tree,node->value,"0 anim -1");
    if(node->kind==NativeScriptSource_Function&&node->parent<tree->count&&tree->nodes[node->parent].kind==NativeScriptSource_Article)return true;
    if(node->kind==NativeScriptSource_State&&node->parent<tree->count&&tree->nodes[node->parent].kind==NativeScriptSource_Article)return true;
    if(node->kind==NativeScriptSource_Parameter&&node->parent<tree->count&&tree->nodes[node->parent].kind==NativeScriptSource_Article)return true;
    if(node->kind==NativeScriptSource_Callback&&node->parent<tree->count&&tree->nodes[node->parent].kind==NativeScriptSource_Article){NativeScriptEventResolution event;return nativeScriptEvents_Resolve(tree,node,&event,d);}
    return nativeScriptCompile_MinimalMetadata(NULL,tree,at,d);
}
static uint32_t named(const NativeScriptCompileBank* b,const char* name)
{for(unsigned f=0;f<b->declarations.function_count;++f)if(nativeScriptSource_SpanEquals(&b->tree,b->declarations.functions[f].name,name))return f;CHECK(false);return 0;}
static uint64_t hash(const void* memory,size_t size)
{const unsigned char* p=memory;uint64_t value=UINT64_C(1469598103934665603);while(size--){value^=*p++;value*=UINT64_C(1099511628211);}return value;}
static void reject(const char* source,const char* message,const NativeScriptCompileBank** accepted)
{
    uint64_t before=hash(&buffers[0],sizeof(buffers[0]));NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.metadata=metadata};
    CHECK(!nativeScriptCompile_Build(&banks[1],&request,accepted,&diagnostic));CHECK(*accepted==&banks[0]&&banks[0].valid);CHECK(hash(&buffers[0],sizeof(buffers[0]))==before);
    if(message)CHECK(strstr(diagnostic.message,message));
}
// This test host routes receiver-domain instructions through the real dispatcher.
// It never supplies an engine object/token: native reference leaves reject.
static bool read_receiver(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* selectors,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{return nativeScriptBindingReceiver_Read(NULL,c,semantic,selectors,count,out,d);}
static bool invoke_receiver(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* selectors,uint32_t selector_count,NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{return nativeScriptBindingReceiver_Invoke(NULL,c,semantic,selectors,selector_count,args,count,out,d);}
static void run(const NativeScriptCompileBank* bank,const char* name,unsigned count)
{
    uint32_t f=named(bank,name);NativeScriptValue registers[256];NativeScriptVMFrame frames[16];NativeScriptVM vm={.registers=registers,.register_capacity=256,.frames=frames,.frame_capacity=16};NativeScriptRuntimeContext runtime={.source_class=bank->declarations.functions[f].owner?NativeCallbackSource_Item:NativeCallbackSource_Fighter,.generation=17};
    NativeScriptVMHost host={.source_alive=nativeScriptBindings_SourceAlive,.read=read_receiver,.invoke=invoke_receiver};NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};calls=0;
    CHECK(nativeScriptVM_Run(&vm,&bank->code,&host,&runtime,f,NULL,0,&result)==NativeScriptVM_Error);CHECK(calls==1&&captured_count==count&&result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);
}
static void negative(const char* body,const char* message,const NativeScriptCompileBank** accepted)
{
    char source[Bytes];snprintf(source,sizeof(source),"fighter:\n    behavior independent\narticle Bolt states 1:\n    state float amount\n    state float samples[2]\n    parameter float gain = 1\n    state 0 anim -1:\nfunc Bad():\n    %s\n",body);reject(source,message,accepted);
}
int main(void)
{
    const char* source="fighter:\n    behavior independent\narticle Bolt states 1:\n    state float amount\n    state bool enabled\n    state int number\n    state float samples[2]\n    parameter float gain = 1\n    on held_status:\n        return articles.ref(-1,Bolt).state.number\n    func ItemRead() -> bool:\n        return articles.ref(-1,Bolt).exists\n    state 0 anim -1:\narticle Other states 1:\n    state int amount\n    state 0 anim -1:\nfunc Exists() -> bool:\n    return articles.ref(0x8000000000000000,Bolt).exists\nfunc Owner() -> uint:\n    return articles.ref(-1,Bolt).current_owner\nfunc Scalar() -> float:\n    return articles.ref(-1,Bolt).state.amount\nfunc Integer() -> int:\n    return articles.ref(-1,Other).state.amount\nfunc Array() -> float:\n    return articles.ref(-1,Bolt).state.samples[0x8000000000000000]\nfunc Enabled() -> bool:\n    return articles.ref(-1,Bolt).state.enabled\nfunc Number() -> int:\n    return articles.ref(-1,Bolt).state.number\nfunc Bone() -> bool:\n    let float x = 1\n    let float y = 2\n    let float z = 3\n    return articles.read_bone_position(-1,Bolt,6,x,y,z)\nfunc Translation() -> bool:\n    let float x = 1\n    let float y = 2\n    let float z = 3\n    return articles.read_primary_translation(-1,Bolt,x,y,z)\naction Predicate:\n    on cpu_capture_decision:\n        return articles.ref(-1,Bolt).exists\n";
    setup(0);setup(1);const NativeScriptCompileBank* accepted=NULL;NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.metadata=metadata};CHECK(nativeScriptCompile_Build(&banks[0],&request,&accepted,&diagnostic));CHECK(accepted==&banks[0]&&banks[0].valid);CHECK(banks[0].code.function_count==12);
    unsigned roots=0,arrays=0,fields=0,translations=0;uint32_t root_owner=0;
    for(unsigned i=0;i<accepted->code.instruction_count;++i){const NativeScriptInstruction* op=&accepted->code.instructions[i];uint32_t category=op->semantic&NATIVE_SCRIPT_RECEIVER_CLASS_MASK;
        if(op->opcode==NativeScriptBC_Invoke&&category==NATIVE_SCRIPT_RECEIVER_ARTICLE_ROOT){++roots;CHECK(op->type==NativeScriptType_Object&&op->argument_count==2);if(!root_owner)root_owner=op->semantic&NATIVE_SCRIPT_RECEIVER_PAYLOAD_MASK;}
        if(op->opcode==NativeScriptBC_Read&&category==NATIVE_SCRIPT_RECEIVER_ARTICLE_STATE){++fields;CHECK(op->type==NativeScriptType_Bool||op->type==NativeScriptType_I32||op->type==NativeScriptType_F32);if(op->selector_count==2)++arrays;CHECK(!(op->flags&NativeScriptFlag_Writable));}
        if(op->opcode==NativeScriptBC_Invoke&&category==NATIVE_SCRIPT_RECEIVER_ARTICLE_TRANSLATION){++translations;CHECK(op->argument_count==5&&op->type==NativeScriptType_Bool);}
    }
    CHECK(roots==10&&fields==6&&arrays==1&&translations==1);
    run(accepted,"Exists",2);CHECK((captured_semantic&NATIVE_SCRIPT_RECEIVER_CLASS_MASK)==NATIVE_SCRIPT_RECEIVER_ARTICLE_ROOT);CHECK(captured[0].type==NativeScriptType_U64&&captured[0].as.u64==UINT64_C(9223372036854775808));
    run(accepted,"ItemRead",2);CHECK((captured_semantic&NATIVE_SCRIPT_RECEIVER_PAYLOAD_MASK)==root_owner);run(accepted,"Translation",5);CHECK((captured_semantic&NATIVE_SCRIPT_RECEIVER_CLASS_MASK)==NATIVE_SCRIPT_RECEIVER_ARTICLE_TRANSLATION);for(unsigned i=2;i<5;++i)CHECK(captured[i].type==NativeScriptType_F32);
    run(accepted,"Bone",6);CHECK((captured_semantic&NATIVE_SCRIPT_RECEIVER_CLASS_MASK)==NATIVE_SCRIPT_RECEIVER_ARTICLE_BONE_POSITION);CHECK(captured[2].type==NativeScriptType_I32&&captured[2].as.i64==6);
    NativeScriptBindingReceiverResolver resolver={.program=&banks[0].declarations,.source_class=NativeCallbackSource_Fighter};NativeScriptCallLayout layout;CHECK(nativeScriptBindingReceiver_DescribeCall(&resolver,NATIVE_SCRIPT_RECEIVER_ARTICLE_TRANSLATION|root_owner,&layout,&diagnostic));CHECK(layout.argument_count==5&&layout.output_mask==28&&layout.publication==NativeScriptOutputs_SuccessOnly);
    NativeScriptRuntimeContext runtime={.generation=17};NativeScriptValue value={.type=NativeScriptType_F32,.as.f32=1};CHECK(!nativeScriptBindingReceiver_ValidateTarget(NULL,&runtime,NATIVE_SCRIPT_RECEIVER_ARTICLE_STATE,NULL,0,&diagnostic));CHECK(!nativeScriptBindingReceiver_Write(NULL,&runtime,NATIVE_SCRIPT_RECEIVER_ARTICLE_STATE,NULL,0,&value,&diagnostic));
    negative("articles.ref(-1,Bolt).state.amount = 3",NULL,&accepted);negative("articles.ref(-1,Bolt).state.samples[0] = 3",NULL,&accepted);negative("let bool bad = articles.ref(-1,Bolt).state.samples == None",NULL,&accepted);negative("articles.ref(-1,Bolt).state.samples",NULL,&accepted);
    negative("let float bad = articles.ref(-1,Bolt).state.samples[0.5]",NULL,&accepted);negative("let float bad = articles.ref(-1,Bolt).state.gain","not declared",&accepted);negative("let int bad = articles.ref(-1,Bolt).motion",NULL,&accepted);negative("articles.ref(-1,Bolt).position.x = 1",NULL,&accepted);negative("articles.ref(-1,Bolt).surface_normal.y = 1",NULL,&accepted);negative("let float bad = articles.ref(-1,Bolt).position.w",NULL,&accepted);negative("let bool bad = articles.ref(1.5,Bolt).exists","integral",&accepted);negative("let bool bad = articles.ref(-1,3).exists","identifier",&accepted);
    negative("let float x = 0\n    let float y = 0\n    let float z = 0\n    articles.read_bone_position(-1,Bolt,1.5,x,y,z)","integral",&accepted);
    negative("let float x = 0\n    let float y = 0\n    let float z = 0\n    articles.read_primary_translation(-1,Bolt,1,y,z)",NULL,&accepted);
    reject("fighter:\n    behavior independent\narticle Bolt states 1:\n    state uint bits\n    state 0 anim -1:\nfunc Bad() -> uint:\n    return articles.ref(-1,Bolt).state.bits\n","supported",&accepted);
    reject("fighter:\n    behavior independent\narticle Bolt states 1:\n    func Bad() -> bool:\n        return creator.articles.ref(-1,Bolt).exists\n    state 0 anim -1:\n",NULL,&accepted);
    printf("{\"status\":\"PASS readonly Article reference compiler/receiver routing\",\"checks\":%u,\"native_reference_unavailable\":true,\"whole_assets_accepted\":false}\n",checks);return 0;
}
