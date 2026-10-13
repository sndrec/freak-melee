#include <melee/ft/native_script_compile.h>
#include <melee/ft/native_script_binding_receiver.h>
#include <melee/ft/native_script_binding_spawn.h>
#include <melee/ft/native_script_item_program.h>
#include <melee/ft/native_script_constants.h>
#include <melee/ft/native_script_move_lowering.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/it/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "actual_compile_context.h"

enum { N=2048,F=128,V=256,I=16384 };
static char source[65536];static NativeScriptSourceNode syntax[N];static NativeScriptIRNode expressions[I];static uint32_t arguments[I];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[128];
static NativeScriptInstruction instructions[I];static NativeScriptFunctionCode code[F];static NativeScriptExecutionFunction execution[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[32];static NativeScriptParameterSlot slots[V];
static NativeScriptParameterSupply supplies[V];
static NativeScriptMoveLayout motions={.common_count=341,.first=341,.count=10,.ground={343,345,347,349},.air={344,346,348,350}};
static NativeScriptCompileBank bank;
static ItemAttr attributes;static HSD_Joint joint;static ItemModelDesc model={.x0_joint=&joint};static ItemStateDesc anim_desc[2];static ItemStateArray anims={anim_desc,2};
static Article article={.x0_common_attr=&attributes,.x10_modelDesc=&model,.xC_itemStates=&anims};
static NativeScriptItemAsset assets[]={ {.declaration="It_Kind_Mario_Cape",.kind=It_Kind_Mario_Cape,.article=&article}, {.declaration="It_Kind_Mario_Fire",.kind=It_Kind_Mario_Fire,.article=&article} };
static NativeScriptItemMetadataContext metadata_context={.motions=&motions,.assets=assets,.asset_count=2};
static bool metadata(void* unused,const NativeScriptSourceTree* tree,uint32_t at,NativeScriptDiagnostic* d)
{
    (void)unused;const NativeScriptSourceNode* n=&tree->nodes[at];
    // This initial compile probe validates exactly the existing effect header
    // and its audited event ownership; actual effect asset publication is a
    // separate production contract, never inferred from this controlled model.
    if(n->kind==NativeScriptSource_Effect)return n->parent==0 && n->name.length && nativeScriptSource_SpanEquals(tree,n->value,"0x47c");
    if(n->kind==NativeScriptSource_Callback && tree->nodes[n->parent].kind==NativeScriptSource_Effect){NativeScriptEventResolution e;return nativeScriptEvents_Resolve(tree,n,&e,d);}
    return nativeScriptItemMetadata_Validate(&metadata_context,tree,at,d);
}
static bool imports(void* unused,const NativeScriptProgramIndex* index,const NativeScriptParameterSupply** out,uint32_t* count,NativeScriptDiagnostic* d)
{
    (void)unused;(void)d;*count=0;
    for(uint32_t i=0;i<index->value_count;++i)if(index->values[i].imported) {
        supplies[*count]=(NativeScriptParameterSupply){.semantic=NATIVE_SCRIPT_PARAMETER_SEMANTIC|i,.owner=index->values[i].owner,.value={.type=index->values[i].type}};++*count;
    }
    *out=supplies;return true;
}
static void json(const char* text)
{putchar('"');for(;*text;++text){if(*text=='"'||*text=='\\')putchar('\\');if(*text=='\n')fputs("\\n",stdout);else putchar(*text);}putchar('"');}
int main(int argc,char** argv)
{
    if(argc!=2)return 1;FILE* file=fopen(argv[1],"rb");if(!file)return 1;size_t bytes=fread(source,1,sizeof(source)-1,file);fclose(file);source[bytes]=0;
    static char candidate_source[65536];
    bank=(NativeScriptCompileBank){.source_storage=candidate_source,.source_capacity=sizeof(candidate_source),.tree={.nodes=syntax,.capacity=N},.expressions={.nodes=expressions,.node_capacity=I,.arguments=arguments,.argument_capacity=I},
        .declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=128,
        .instructions=instructions,.instruction_capacity=I,.function_code=code,.function_code_capacity=F,.execution_functions=execution,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,
        .storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=32},.parameter_slots=slots,.parameter_slot_capacity=V};
    NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)bytes,.metadata=metadata,.parameter_provider=imports,.motion_layout=&motions};
    NativeScriptDiagnostic d={0};const NativeScriptCompileBank* accepted=NULL;bool complete=nativeScriptCompile_Build(&bank,&request,&accepted,&d);
    printf("{\"whole_compile\":%s,\"first_error\":",complete?"true":"false");json(d.message);
    // Failed body compilation preserves declaration/index data. Census every
    // real body through the mechanically extracted production context/resolver.
    unsigned passed=0,failed=0;printf(",\"bodies\":[");
    NativeScriptProgramCompiler compiler={.index=&bank.declarations,.native_bindings=resolve,.locals=locals,.local_capacity=128};
    NativeScriptBytecodeBuilder builder={.instructions=instructions,.instruction_capacity=I,.functions=code,.function_count=bank.declarations.function_count,.describe_call=describe};
    bank.expressions.node_count=bank.expressions.argument_count=0;
    for(uint32_t i=0;i<bank.declarations.function_count;++i) {
        if(i)putchar(',');CompileContext context;NativeScriptStatementCompiler body;d=(NativeScriptDiagnostic){0};
        uint32_t first=bank.expressions.node_count;bool ok=function_context(&bank,i,&context,&d);compiler.native_binding_context=&context;builder.binding_context=&context;
        if(ok)ok=nativeScriptProgram_CompileBody(&compiler,i,&body,&d);
        if(ok)ok=nativeScriptMoveLowering_Apply(&motions,&bank.declarations,i,&bank.expressions,first,&d);
        if(ok)ok=nativeScriptBytecode_Compile(&builder,&body,functions[i].source_node,i,(uint16_t)functions[i].parameter_count,&d);
        printf("{\"function\":%u,\"line\":%u,\"compiled\":%s,\"error_line\":%u,\"error\":",i,functions[i].name.line,ok?"true":"false",d.source.line);json(d.message);putchar('}');
        if(ok)++passed;else ++failed;
    }
    printf("],\"summary\":{\"functions\":%u,\"compiled\":%u,\"unresolved\":%u,\"whole_compile\":%s}}\n",bank.declarations.function_count,passed,failed,complete?"true":"false");return 0;
}
