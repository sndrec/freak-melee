#include <melee/ft/native_script_compile.h>
#include <melee/ft/native_script_binding_signal_resolve.h>
#include <melee/ft/native_script_binding_signal.h>
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
static Buffers buffers[2];static NativeScriptCompileBank banks[2];static unsigned checks,calls;static uint32_t captured_semantic,captured_count;static NativeScriptValue captured[4];static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"signal catalog %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
bool nativeScriptBindings_SourceAlive(NativeScriptRuntimeContext* c){return c&&c->generation==17;}
bool nativeScriptBindingSignal_Invoke(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{(void)c;(void)out;(void)d;CHECK(count==2||count==4);captured_semantic=semantic;captured_count=count;for(unsigned i=0;i<count;++i)captured[i]=args[i];++calls;return false;}
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
static void run(const NativeScriptCompileBank* bank,const char* name,unsigned count)
{
    uint32_t f=named(bank,name);NativeScriptValue registers[256];NativeScriptVMFrame frames[16];NativeScriptVM vm={.registers=registers,.register_capacity=256,.frames=frames,.frame_capacity=16};NativeScriptRuntimeContext runtime={.source_class=bank->declarations.functions[f].owner?NativeCallbackSource_Item:NativeCallbackSource_Fighter,.generation=17};
    NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};calls=0;CHECK(nativeScriptVM_Run(&vm,&bank->code,&nativeScriptBindingHost,&runtime,f,NULL,0,&result)==NativeScriptVM_Error);CHECK(calls==1&&captured_count==count&&result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);
}
int main(void)
{
    const char* source="fighter:\n    behavior independent\narticle Bolt states 1:\n    state int count\n    on signal pulse:\n        state.count = state.count + int(signal.value)\n    func Self():\n        item.signal(\"pulse\", 1.25)\n    state 0 anim -1:\narticle Second states 1:\n    on signal pulse:\n        pass\n    state 0 anim -1:\nfunc Kick():\n    articles.signal(0x8000000000000000, Bolt, \"pulse\", 2.5)\nfunc Other():\n    articles.signal(-1, Second, \"pulse\", 0)\n";
    setup(0);setup(1);const NativeScriptCompileBank* accepted=NULL;NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.metadata=metadata};CHECK(nativeScriptCompile_Build(&banks[0],&request,&accepted,&diagnostic));CHECK(accepted==&banks[0]&&banks[0].valid);CHECK(banks[0].code.function_count==5);CHECK(banks[0].maximum_call_depth>=2);
    run(accepted,"Kick",4);CHECK((captured_semantic&NATIVE_SCRIPT_SIGNAL_FUNCTION_MASK)==0);CHECK((captured_semantic&NATIVE_SCRIPT_SIGNAL_OPERATION_MASK)==(NativeScriptSignal_Target<<20));CHECK(captured[0].type==NativeScriptType_U64&&captured[0].as.u64==UINT64_C(9223372036854775808));CHECK(captured[2].type==NativeScriptType_Text&&captured[2].as.text.length==5);CHECK(captured[3].type==NativeScriptType_F32&&captured[3].as.f32==2.5f);
    uint32_t first=captured_semantic;run(accepted,"Other",4);CHECK(captured_semantic!=first);CHECK(captured[0].type==NativeScriptType_I32&&captured[0].as.i64==-1);
    run(accepted,"Self",2);CHECK((captured_semantic&NATIVE_SCRIPT_SIGNAL_FUNCTION_MASK)==0);CHECK((captured_semantic&NATIVE_SCRIPT_SIGNAL_OPERATION_MASK)==(NativeScriptSignal_Self<<20));CHECK(captured[0].type==NativeScriptType_Text&&captured[1].type==NativeScriptType_F32&&captured[1].as.f32==1.25f);
    char bad[Bytes];const char* mark=strstr(source,"\"pulse\", 2.5");CHECK(mark);unsigned offset=(unsigned)(mark-source);snprintf(bad,sizeof(bad),"%.*s\"absent\", 2.5)%s",(int)offset,source,strchr(mark,')')+1);reject(bad,"not declared",&accepted);
    reject("fighter:\n    behavior independent\narticle Bolt states 1:\n    on signal pulse:\n        item.signal(\"pulse\", 0)\n    state 0 anim -1:\n","recursion",&accepted);
    reject("fighter:\n    behavior independent\narticle Bolt states 1:\n    on signal pulse:\n        unknown.operation()\n    state 0 anim -1:\nfunc Kick():\n    articles.signal(-1,Bolt,\"pulse\",0)\n",NULL,&accepted);
    reject("fighter:\n    behavior independent\narticle Bolt states 1:\n    on signal pulse:\n        pass\n    func Bad():\n        item.signal(\"pul\\x73e\",0)\n    state 0 anim -1:\n",NULL,&accepted);
    reject("fighter:\n    behavior independent\narticle Bolt states 1:\n    on signal pulse:\n        pass\n    state 0 anim -1:\nfunc Kick():\n    articles.signal(1.5,Bolt,\"pulse\",0)\n","integral",&accepted);
    reject("fighter:\n    behavior independent\narticle Bolt states 1:\n    on signal pulse:\n        pass\n    state 0 anim -1:\nfunc Kick():\n    articles.signal(-1,3,\"pulse\",0)\n","identifier",&accepted);
    reject("fighter:\n    behavior independent\nfunc Kick():\n    item.signal(\"pulse\",0)\n",NULL,&accepted);
    reject("fighter:\n    behavior independent\narticle Bolt states 1:\n    on signal pulse:\n        return 1\n    state 0 anim -1:\n",NULL,&accepted);
    reject("fighter:\n    behavior independent\narticle Bolt states 1:\n    on signal pulse:\n        pass\n    func Bad():\n        creator.articles.signal(-1,Bolt,\"pulse\",0)\n    state 0 anim -1:\n",NULL,&accepted);
    reject("fighter:\n    behavior independent\narticle Bolt states 1:\n    on signal pulse:\n        pass\n    func Bad():\n        item.signal(\"pulse\",0,1)\n    state 0 anim -1:\n",NULL,&accepted);
    const char* native_name="fighter:\n    behavior independent\narticle It_Kind_Zelda_DinFire states 1:\n    on signal burst:\n        pass\n    state 0 anim -1:\nfunc Builtin():\n    articles.signal(-1,It_Kind_Zelda_DinFire,\"burst\",0)\n";
    request=(NativeScriptCompileRequest){.source=native_name,.source_bytes=(uint32_t)strlen(native_name),.metadata=metadata};CHECK(nativeScriptCompile_Build(&banks[1],&request,&accepted,&diagnostic));run(accepted,"Builtin",4);NativeScriptConstant constant;CHECK(nativeScriptConstants_Lookup("It_Kind_Zelda_DinFire",(uint32_t)strlen("It_Kind_Zelda_DinFire"),&constant));CHECK(captured[1].type==NativeScriptType_I32&&(uint64_t)captured[1].as.i64==constant.bits);CHECK((captured_semantic&NATIVE_SCRIPT_SIGNAL_FUNCTION_MASK)==0);
    const char* factories="fighter:\n    behavior independent\narticle Bolt states 1:\n    on signal pulse:\n        pass\n    state 0 anim -1:\nfunc Position():\n    let int handle = articles.create_at_position(Bolt,1,2,3,1,false)\nfunc Velocity():\n    let int handle = articles.create_with_velocity(Bolt,1,2,3,4,5,6,-1,true,0x8000000000000000,0.25, true)\n";
    request=(NativeScriptCompileRequest){.source=factories,.source_bytes=(uint32_t)strlen(factories),.metadata=metadata};setup(0);CHECK(nativeScriptCompile_Build(&banks[0],&request,&accepted,&diagnostic));unsigned factory_calls=0;
    for(unsigned i=0;i<accepted->code.instruction_count;++i)if(accepted->code.instructions[i].opcode==NativeScriptBC_Invoke){uint32_t semantic=accepted->code.instructions[i].semantic;CHECK((semantic&NATIVE_SCRIPT_SPAWN_DOMAIN_MASK)==NATIVE_SCRIPT_SPAWN_DOMAIN);uint32_t base=nativeScriptBindingSpawn_Base(semantic);CHECK(base==NativeScriptBinding_ArticlesCreateAtPosition||base==NativeScriptBinding_ArticlesCreateWithVelocity);++factory_calls;}CHECK(factory_calls==2);
    printf("{\"status\":\"PASS indexed signal compile/graph/VM routing\",\"checks\":%u,\"native_signal_unavailable\":true,\"whole_assets_accepted\":false}\n",checks);return 0;
}

#include <melee/ft/native_script_binding_item_links.h>
bool nativeScriptBindingItemLinks_Invoke(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* a,uint32_t n,NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)a;(void)n;(void)v;(void)d;return false;}
