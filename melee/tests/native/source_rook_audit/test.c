#include <melee/ft/native_script_compile.h>
#include <melee/ft/native_script_item_program.h>
#include <melee/ft/native_script_constants.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "actual_compile_context.h"

enum {N=2048,F=128,V=256,I=16384};
static char source[65536];static NativeScriptSourceNode syntax[N];
static NativeScriptIRNode expressions[I];static uint32_t arguments[I];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration parameters[V];
static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[V];
static NativeScriptInstruction instructions[I];static NativeScriptFunctionCode code[F];
static NativeScriptCompileBank bank;
static bool failed_body[F];
static void json(const char* text)
{putchar('"');for(;*text;++text){if(*text=='"'||*text=='\\')putchar('\\');if(*text=='\n')fputs("\\n",stdout);else if(*text=='\r')fputs("\\r",stdout);else putchar(*text);}putchar('"');}
static void diagnostic(const NativeScriptDiagnostic* d)
{printf("{\"line\":%u,\"column\":%u,\"message\":",d->source.line,d->source.column);json(d->message);putchar('}');}
int main(int argc,char** argv)
{
    if(argc!=2)return 1;FILE* file=fopen(argv[1],"rb");if(!file)return 1;
    size_t bytes=fread(source,1,sizeof(source)-1,file);bool complete=feof(file);fclose(file);if(!complete)return 1;source[bytes]=0;
    bank=(NativeScriptCompileBank){.tree={.source=source,.source_bytes=(uint32_t)bytes,.nodes=syntax,.capacity=N},
        .expressions={.source=source,.source_bytes=(uint32_t)bytes,.nodes=expressions,.node_capacity=I,.arguments=arguments,.argument_capacity=I},
        .declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V,
            .event_signature=nativeScriptEvents_Signature},.locals=locals,.local_capacity=V,
        .instructions=instructions,.instruction_capacity=I,.function_code=code,.function_code_capacity=F,
        .has_motion_layout=true,.motion_layout={.common_count=341,.first=341,.count=12}};
    bank.declarations.tree=&bank.tree;bank.declarations.expressions=&bank.expressions;
    NativeScriptDiagnostic d={0};bool parsed=nativeScriptSource_Parse(&bank.tree,&d);
    printf("{\"parsed\":%s,\"parse_error\":",parsed?"true":"false");diagnostic(&d);
    if(!parsed){puts(",\"indexed\":false}");return 0;}
    bool indexed=nativeScriptProgram_Index(&bank.declarations,&d);
    printf(",\"nodes\":%u,\"indexed\":%s,\"index_error\":",bank.tree.count,indexed?"true":"false");diagnostic(&d);
    if(!indexed){puts("}");return 0;}
    NativeScriptItemMetadataContext metadata={.motions=&bank.motion_layout};
    printf(",\"metadata_without_assets\":[");bool first=true;
    for(uint32_t at=0;at<bank.tree.count;++at){
        const NativeScriptSourceNode* n=&bank.tree.nodes[at];
        if(n->kind>=NativeScriptSource_If&&n->kind<=NativeScriptSource_Pass)continue;
        d=(NativeScriptDiagnostic){0};bool ok=nativeScriptItemMetadata_Validate(&metadata,&bank.tree,at,&d);
        if(!first)putchar(',');first=false;
        printf("{\"node\":%u,\"kind\":%u,\"line\":%u,\"accepted\":%s,\"diagnostic\":",at,n->kind,n->source.line,ok?"true":"false");diagnostic(&d);putchar('}');
    }
    printf("],\"bodies\":[");unsigned passed=0,failed=0;
    NativeScriptProgramCompiler compiler={.index=&bank.declarations,.native_bindings=resolve,.locals=locals,.local_capacity=V};
    NativeScriptBytecodeBuilder builder={.instructions=instructions,.instruction_capacity=I,.functions=code,
        .function_count=bank.declarations.function_count,.describe_call=describe};
    for(uint32_t f=0;f<bank.declarations.function_count;++f){
        if(f)putchar(',');CompileContext context;NativeScriptStatementCompiler body={0};d=(NativeScriptDiagnostic){0};
        uint32_t first_expression=bank.expressions.node_count;
        bool ok=function_context(&bank,f,&context,&d);compiler.native_binding_context=&context;builder.binding_context=&context;
        if(ok)ok=nativeScriptProgram_CompileBody(&compiler,f,&body,&d);
        if(ok)ok=nativeScriptMoveLowering_Apply(&bank.motion_layout,&bank.declarations,f,&bank.expressions,first_expression,&d);
        if(ok)ok=nativeScriptBytecode_Compile(&builder,&body,functions[f].source_node,f,(uint16_t)functions[f].parameter_count,&d);
        char name[128]={0};memcpy(name,source+functions[f].name.offset,functions[f].name.length<127?functions[f].name.length:127);
        printf("{\"function\":%u,\"owner\":%u,\"line\":%u,\"name\":",f,functions[f].owner,functions[f].name.line);json(name);
        printf(",\"compiled\":%s,\"diagnostic\":",ok?"true":"false");diagnostic(&d);
        printf(",\"locals\":[");
        if(ok)for(uint32_t i=0;i<body.local_count;++i){
            if(i)putchar(',');char local[128]={0};NativeScriptSourceSpan span=body.locals[i].name;
            if(span.length>=sizeof(local))return 1;memcpy(local,source+span.offset,span.length);
            printf("{\"name\":");json(local);printf(",\"type\":");json(nativeScriptExpression_TypeName(body.locals[i].type));putchar('}');
        }
        printf("]}");
        failed_body[f]=!ok;if(ok)++passed;else ++failed;
    }
    printf("],\"calls_in_failed_bodies\":[");first=true;
    for(uint32_t at=0;at<bank.tree.count;++at){
        const NativeScriptSourceNode* statement=&bank.tree.nodes[at];
        if(statement->kind<NativeScriptSource_If||statement->kind>NativeScriptSource_Pass)continue;
        uint32_t ancestor=at;
        while(ancestor&&bank.tree.nodes[ancestor].kind!=NativeScriptSource_Callback)ancestor=bank.tree.nodes[ancestor].parent;
        uint32_t f=0;while(f<bank.declarations.function_count&&functions[f].source_node!=ancestor)++f;
        if(f==bank.declarations.function_count||!failed_body[f])continue;
        NativeScriptExpressionToken tokens[128];uint32_t count=0;d=(NativeScriptDiagnostic){0};
        if(!nativeScriptExpression_Tokenize(source,(uint32_t)bytes,statement->source,tokens,128,&count,&d))return 1;
        CompileContext context;if(!function_context(&bank,f,&context,&d))return 1;
        compiler.active_function=f;compiler.native_binding_context=&context;
        for(uint32_t begin=0;begin<count;++begin){
            if(tokens[begin].kind!=NativeScriptToken_Name||(begin&&tokens[begin-1].kind==NativeScriptToken_Dot))continue;
            uint32_t open=begin+1;
            while(open+1<count&&tokens[open].kind==NativeScriptToken_Dot&&tokens[open+1].kind==NativeScriptToken_Name)open+=2;
            if(open==count||tokens[open].kind!=NativeScriptToken_OpenParen)continue;
            uint32_t end=open+1,depth=1;
            for(;end<count&&depth;++end){if(tokens[end].kind==NativeScriptToken_OpenParen)++depth;else if(tokens[end].kind==NativeScriptToken_CloseParen)--depth;}
            if(depth)return 1;
            NativeScriptSourceSpan span=tokens[begin].source;
            span.length=tokens[end-1].source.offset+tokens[end-1].source.length-span.offset;
            NativeScriptIRArena arena=bank.expressions;arena.node_count=arena.argument_count=0;uint32_t expression;
            d=(NativeScriptDiagnostic){0};bool ok=nativeScriptExpression_Parse(&arena,span,nativeScriptProgram_Resolve,&compiler,&expression,&d);
            char text[1024]={0};if(span.length>=sizeof(text))return 1;memcpy(text,source+span.offset,span.length);
            if(!first)putchar(',');first=false;
            printf("{\"function\":%u,\"line\":%u,\"expression\":",f,span.line);json(text);
            printf(",\"accepted\":%s,\"diagnostic\":",ok?"true":"false");diagnostic(&d);putchar('}');
        }
    }
    printf("],\"summary\":{\"functions\":%u,\"compiled\":%u,\"failed\":%u,\"owned_state_fields\":%u},\"whole_package_accepted\":false}\n",
        bank.declarations.function_count,passed,failed,bank.declarations.value_count);return 0;
}
