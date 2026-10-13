#include <melee/ft/native_script_compile.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { SourceBytes=65536, Nodes=2048, ExpressionNodes=4096, Functions=128,
       Parameters=256, ValueFields=256, Instructions=8192 };
typedef struct Buffers {
    char source[SourceBytes];NativeScriptSourceNode nodes[Nodes];
    NativeScriptIRNode expressions[ExpressionNodes];uint32_t arguments[ExpressionNodes];
    NativeScriptFunctionDeclaration functions[Functions];NativeScriptParameterDeclaration parameters[Parameters];
    NativeScriptValueDeclaration values[ValueFields];NativeScriptLocal locals[128];
    NativeScriptInstruction instructions[Instructions];NativeScriptFunctionCode code[Functions];
    NativeScriptExecutionFunction executions[Functions];NativeScriptCompileFunctionScratch graph[Functions];
    NativeScriptStorageField fields[ValueFields];NativeScriptStorageOwner owners[32];NativeScriptParameterSlot slots[ValueFields];
} Buffers;
static Buffers buffers[2];static NativeScriptCompileBank banks[2];
static char input[SourceBytes];static unsigned checks;
#define CHECK(value)do{++checks;if(!(value)){fprintf(stderr,"line %u: %s\n",__LINE__,#value);exit(1);}}while(0)

static void setup(unsigned at)
{
    Buffers* b=&buffers[at];
    banks[at]=(NativeScriptCompileBank){.source_storage=b->source,.source_capacity=SourceBytes,
        .tree={.nodes=b->nodes,.capacity=Nodes},
        .expressions={.nodes=b->expressions,.node_capacity=ExpressionNodes,.arguments=b->arguments,.argument_capacity=ExpressionNodes},
        .declarations={.functions=b->functions,.function_capacity=Functions,.parameters=b->parameters,.parameter_capacity=Parameters,
            .values=b->values,.value_capacity=ValueFields},.locals=b->locals,.local_capacity=128,
        .instructions=b->instructions,.instruction_capacity=Instructions,.function_code=b->code,.function_code_capacity=Functions,
        .execution_functions=b->executions,.execution_function_capacity=Functions,.graph=b->graph,.graph_capacity=Functions,
        .storage_layout={.fields=b->fields,.field_capacity=ValueFields,.owners=b->owners,.owner_capacity=32},
        .parameter_slots=b->slots,.parameter_slot_capacity=ValueFields};
}

static uint64_t hash(const void* memory,size_t bytes)
{
    const uint8_t* data=memory;uint64_t value=UINT64_C(1469598103934665603);
    for(size_t i=0;i<bytes;++i){value^=data[i];value*=UINT64_C(1099511628211);}return value;
}

static void reject(const char* text,const char* diagnostic_text,const NativeScriptCompileBank** accepted)
{
    NativeScriptDiagnostic diagnostic;uint64_t old=hash(&buffers[0],sizeof(buffers[0]));
    NativeScriptCompileRequest request={.source=text,.source_bytes=(uint32_t)strlen(text)};
    CHECK(!nativeScriptCompile_Build(&banks[1],&request,accepted,&diagnostic));
    CHECK(*accepted==&banks[0] && banks[0].valid);CHECK(hash(&buffers[0],sizeof(buffers[0]))==old);
    if(!strstr(diagnostic.message,diagnostic_text))fprintf(stderr,"wanted %s; got %s\n",diagnostic_text,diagnostic.message);
    CHECK(strstr(diagnostic.message,diagnostic_text)!=NULL);
}

typedef struct Runtime {NativeScriptStorageInstance* state;float velocity,position;} Runtime;
static bool alive(NativeScriptRuntimeContext* context)
{return ((Runtime*)context->user)->state->live;}
static bool read(NativeScriptRuntimeContext* context,uint32_t semantic,const NativeScriptValue* selectors,
    uint32_t count,NativeScriptValue* value,NativeScriptDiagnostic* diagnostic)
{
    Runtime* runtime=context->user;
    if(semantic&UINT32_C(0xf0000000))return nativeScriptStorage_Read(runtime->state,semantic,selectors,count,value,diagnostic);
    if(semantic==NativeScriptBinding_VelocityX&&!count){*value=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=runtime->velocity};return true;}
    return false;
}
static bool write(NativeScriptRuntimeContext* context,uint32_t semantic,const NativeScriptValue* selectors,
    uint32_t count,const NativeScriptValue* value,NativeScriptDiagnostic* diagnostic)
{
    Runtime* runtime=context->user;
    if(semantic&UINT32_C(0xf0000000))return nativeScriptStorage_Write(runtime->state,semantic,selectors,count,value,diagnostic);
    if(semantic==NativeScriptBinding_PositionY&&!count&&value->type==NativeScriptType_F32){runtime->position=value->as.f32;return true;}
    return false;
}

static bool extended(void* count,const NativeScriptSourceTree* tree,uint32_t at,NativeScriptDiagnostic* diagnostic)
{
    ++*(unsigned*)count;
    const NativeScriptSourceNode* node=&tree->nodes[at];
    if(node->kind==NativeScriptSource_Article)
        return nativeScriptSource_SpanEquals(tree,node->value,"states 1");
    if(node->kind==NativeScriptSource_Attribute&&node->parent==0)return true;
    if(node->parent<tree->count&&tree->nodes[node->parent].kind==NativeScriptSource_Article&&
        (node->kind==NativeScriptSource_Function||node->kind==NativeScriptSource_State||node->kind==NativeScriptSource_Parameter))return true;
    if(node->kind==NativeScriptSource_Callback&&node->parent<tree->count&&tree->nodes[node->parent].kind==NativeScriptSource_Article){
        NativeScriptEventResolution event;return nativeScriptEvents_Resolve(tree,node,&event,diagnostic);
    }
    return nativeScriptCompile_MinimalMetadata(NULL,tree,at,diagnostic);
}

typedef struct Provider {
    NativeScriptParameterSupply supply;unsigned calls;bool reject;
} Provider;
static bool provide(void* opaque,const NativeScriptProgramIndex* index,
    const NativeScriptParameterSupply** values,uint32_t* count,NativeScriptDiagnostic* diagnostic)
{
    Provider* provider=opaque;++provider->calls;
    CHECK(index->value_count==1 && index->values[0].imported);
    CHECK(index->values[0].owner==0 && index->values[0].type==NativeScriptType_F32);
    if(provider->reject){snprintf(diagnostic->message,sizeof(diagnostic->message),"Rejected candidate DAT import");return false;}
    provider->supply=(NativeScriptParameterSupply){.semantic=NATIVE_SCRIPT_PARAMETER_SEMANTIC,
        .value={.type=NativeScriptType_F32,.as.f32=3.25f}};
    *values=&provider->supply;*count=1;return true;
}

int main(int argc,char** argv)
{
    CHECK(argc==2);FILE* file=fopen(argv[1],"rb");CHECK(file!=NULL);
    uint32_t bytes=(uint32_t)fread(input,1,sizeof(input)-1,file);fclose(file);setup(0);setup(1);
    const NativeScriptCompileBank* accepted=NULL;NativeScriptDiagnostic diagnostic;
    NativeScriptCompileRequest request={.source=input,.source_bytes=bytes};
    if(!nativeScriptCompile_Build(&banks[0],&request,&accepted,&diagnostic)){fprintf(stderr,"build line%u: %s\n",diagnostic.source.line,diagnostic.message);return 1;}
    CHECK(accepted==&banks[0] && accepted->valid);CHECK(accepted->tree.source==buffers[0].source);
    CHECK(accepted->declarations.tree==&banks[0].tree);CHECK(accepted->storage_layout.program==&banks[0].declarations);
    CHECK(accepted->maximum_call_depth==2);CHECK(accepted->maximum_call_registers>accepted->function_code[3].register_count);
    uint8_t storage_bytes[128];NativeScriptStorageInstance state;
    CHECK(nativeScriptStorage_Create(&accepted->storage_layout,&accepted->storage_parameters,0,1,1,
        storage_bytes,sizeof(storage_bytes),&state,&diagnostic));
    NativeScriptValue registers[1024];NativeScriptVMFrame frames[16];
    NativeScriptVM vm={.registers=registers,.register_capacity=1024,.frames=frames,.frame_capacity=16};
    Runtime runtime={&state,0.5f,0};NativeScriptRuntimeContext context={.user=&runtime};NativeScriptVMHost host={.source_alive=alive,.read=read,.write=write};
    NativeScriptValue result;
    for(unsigned tick=0;tick<16;++tick){
        CHECK(nativeScriptVM_Run(&vm,&accepted->code,&host,&context,3,NULL,0,&result)==NativeScriptVM_Ok);
        CHECK(runtime.position==0.75f*(tick+1));
    }
    uint64_t old=hash(&buffers[0],sizeof(buffers[0]));
    CHECK(!nativeScriptCompile_Build(&banks[0],&request,&accepted,&diagnostic));CHECK(hash(&buffers[0],sizeof(buffers[0]))==old);
    NativeScriptSourceNode* saved=banks[1].tree.nodes;banks[1].tree.nodes=banks[0].tree.nodes;
    CHECK(!nativeScriptCompile_Build(&banks[1],&request,&accepted,&diagnostic));CHECK(hash(&buffers[0],sizeof(buffers[0]))==old);banks[1].tree.nodes=saved;
    reject("fighter:\n    behavior independent\naction A:\n    on tick:\n        velocity.x()\n","Unsupported",&accepted);
    reject("fighter:\n    behavior independent\nunknown_metadata 3\naction A:\n    on tick:\n        pass\n","Unsupported declaration",&accepted);
    reject("fighter:\n    behavior independent\narticle It_Kind_Mario_Fire states 1:\n    on created:\n        pass\n","Unsupported declaration",&accepted);
    reject("fighter:\n    behavior independent\nfunc Recurse() -> int:\n    return Recurse()\naction A:\n    on tick:\n        pass\n","recursion",&accepted);
    reject("fighter:\n    behavior independent\nfunc Bad() -> int:\n    pass\naction A:\n    on tick:\n        pass\n","without returning",&accepted);
    reject("fighter:\n    behavior independent\naction A:\n    on tick:\n        state.unresolved = 1\n","unsupported",&accepted);
    reject("action A:\n    on tick:\n        pass\n","behavior independent",&accepted);
    reject("fighter:\n    behavior independent\naction A:\n    on tick:\n        pass\naction B:\n    on tick:\n        pass\n","Duplicate callback",&accepted);
    uint32_t capacity=banks[1].instruction_capacity;banks[1].instruction_capacity=1;
    reject(input,"instruction capacity",&accepted);banks[1].instruction_capacity=capacity;
    capacity=banks[1].local_capacity;banks[1].local_capacity=0;
    reject(input,"Parameter-local capacity",&accepted);banks[1].local_capacity=capacity;
    unsigned validated=0;
    const char* imported="fighter:\n    behavior independent\nattribute float supplied 0\naction A:\n    on tick:\n        velocity.x = attrs.supplied\n";
    request=(NativeScriptCompileRequest){.source=imported,.source_bytes=(uint32_t)strlen(imported),.metadata=extended,.metadata_context=&validated};
    CHECK(!nativeScriptCompile_Build(&banks[1],&request,&accepted,&diagnostic));CHECK(accepted==&banks[0]);CHECK(strstr(diagnostic.message,"not been supplied")!=NULL);
    NativeScriptParameterSupply supply={.semantic=NATIVE_SCRIPT_PARAMETER_SEMANTIC,.value={.type=NativeScriptType_F32,.as.f32=2}};
    request.parameter_supply=&supply;request.parameter_supply_count=1;
    CHECK(nativeScriptCompile_Build(&banks[1],&request,&accepted,&diagnostic));CHECK(accepted->storage_parameters.slots[0].resolved);
    // Return to the first accepted bank before the extended article example.
    accepted=&banks[0];
    const char* entity="fighter:\n    behavior independent\nparameter float power = 2\narticle It_Kind_Mario_Fire states 1:\n    parameter float power = 1\n    state float total\n    on created:\n        item.state.total = item.params.power + program.attrs.power\n";
    request=(NativeScriptCompileRequest){.source=entity,.source_bytes=(uint32_t)strlen(entity),.metadata=extended,.metadata_context=&validated};
    CHECK(nativeScriptCompile_Build(&banks[1],&request,&accepted,&diagnostic));CHECK(accepted==&banks[1] && validated>0);
    CHECK(accepted->execution_functions[0].source_class==1 && accepted->execution_functions[0].owner!=0);
    // An explicitly expanded metadata schema cannot reinterpret root-owned
    // helper code as article-owned code behind the script author's back.
    const char* crossing="fighter:\n    behavior independent\nfunc Root() -> int:\n    return 1\narticle It_Kind_Mario_Fire states 1:\n    on created:\n        let int result = Root()\n";
    request=(NativeScriptCompileRequest){.source=crossing,.source_bytes=(uint32_t)strlen(crossing),.metadata=extended,.metadata_context=&validated};
    uint64_t second=hash(&buffers[1],sizeof(buffers[1]));CHECK(!nativeScriptCompile_Build(&banks[0],&request,&accepted,&diagnostic));
    CHECK(accepted==&banks[1] && hash(&buffers[1],sizeof(buffers[1]))==second);CHECK(strstr(diagnostic.message,"Cross-owner")!=NULL);
    Provider provider={.reject=true};
    request=(NativeScriptCompileRequest){.source=imported,.source_bytes=(uint32_t)strlen(imported),
        .metadata=extended,.metadata_context=&validated,.parameter_provider=provide,.parameter_context=&provider};
    CHECK(!nativeScriptCompile_Build(&banks[0],&request,&accepted,&diagnostic));
    CHECK(provider.calls==1 && accepted==&banks[1] && hash(&buffers[1],sizeof(buffers[1]))==second);
    CHECK(strstr(diagnostic.message,"Rejected candidate DAT import")!=NULL);
    provider.reject=false;
    CHECK(nativeScriptCompile_Build(&banks[0],&request,&accepted,&diagnostic));
    CHECK(provider.calls==2 && accepted==&banks[0]);
    CHECK(accepted->storage_parameters.slots[0].resolved);
    CHECK(accepted->storage_parameters.slots[0].value.as.f32==3.25f);
    provider.supply.value.as.f32=99; // Parameter storage owns its copy.
    CHECK(accepted->storage_parameters.slots[0].value.as.f32==3.25f);
    request.parameter_supply=&provider.supply;request.parameter_supply_count=1;
    uint64_t final=hash(&buffers[0],sizeof(buffers[0]));
    CHECK(!nativeScriptCompile_Build(&banks[1],&request,&accepted,&diagnostic));
    CHECK(provider.calls==2 && accepted==&banks[0] && hash(&buffers[0],sizeof(buffers[0]))==final);
    CHECK(strstr(diagnostic.message,"mutually exclusive")!=NULL);
    const char* exports="fighter:\n    behavior independent\nexport func answer(int value) -> int:\n    return value\nfunc Call() -> int:\n    let bool delivered = targets.call(7, \"answer\", 17)\n    let bool result = targets.call_bool(7, \"answer\", 17)\n    let float number = targets.call_float(7, \"answer\", 17)\n    return targets.call_int(7, \"answer\", 17)\n";
    request=(NativeScriptCompileRequest){.source=exports,.source_bytes=(uint32_t)strlen(exports)};
    CHECK(nativeScriptCompile_Build(&banks[1],&request,&accepted,&diagnostic));CHECK(accepted==&banks[1]);
    CHECK(accepted->declarations.export_count==1&&accepted->declarations.export_name_count==1);
    unsigned calls=0;
    for(unsigned i=0;i<accepted->code.instruction_count;++i){const NativeScriptInstruction* instruction=&accepted->code.instructions[i];
        if((instruction->semantic&NATIVE_SCRIPT_TARGET_EXPORT_DOMAIN_MASK)==NATIVE_SCRIPT_TARGET_EXPORT_DOMAIN){
            CHECK(instruction->opcode==NativeScriptBC_Invoke);CHECK((instruction->semantic&NATIVE_SCRIPT_TARGET_EXPORT_INDEX_MASK)==0);
            CHECK(((instruction->semantic&NATIVE_SCRIPT_TARGET_EXPORT_ARGUMENT_MASK)>>NATIVE_SCRIPT_TARGET_EXPORT_ARGUMENT_SHIFT)==3);++calls;}}
    CHECK(calls==4);
    second=hash(&buffers[1],sizeof(buffers[1]));
    const char* bad_exports[]={"fighter:\n    behavior independent\nfunc Bad():\n    targets.call(7, \"answer\", None)\n",
        "fighter:\n    behavior independent\nfunc Bad():\n    targets.call(1.5, \"answer\")\n",
        "fighter:\n    behavior independent\nfunc Bad():\n    targets.call(7, \"bad.name\")\n"};
    for(unsigned i=0;i<sizeof(bad_exports)/sizeof(*bad_exports);++i){
        request=(NativeScriptCompileRequest){.source=bad_exports[i],.source_bytes=(uint32_t)strlen(bad_exports[i])};
        CHECK(!nativeScriptCompile_Build(&banks[0],&request,&accepted,&diagnostic));
        CHECK(accepted==&banks[1]&&hash(&buffers[1],sizeof(buffers[1]))==second);}
    printf("{\"result\":\"pass\",\"checks\":%u,\"ticks\":16,\"functions\":5,\"transactional_publication\":true}\n",checks);
    return 0;
}
