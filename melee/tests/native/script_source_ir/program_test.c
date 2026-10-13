#include <melee/ft/native_script_program.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static NativeScriptSourceNode nodes[32768];
static NativeScriptIRNode expressions[8192];
static uint32_t arguments[8192];
static NativeScriptFunctionDeclaration functions[2048];
static NativeScriptParameterDeclaration parameters[4096];
static NativeScriptValueDeclaration values[2048];
static NativeScriptLocal locals[128];
static unsigned checks;
#define CHECK(x) do {++checks;if (!(x)) {fprintf(stderr,"%u: %s\n",__LINE__,#x);exit(1);}} while (0)

static bool index_source(const char* source,NativeScriptProgramIndex* index,NativeScriptDiagnostic* diagnostic)
{
    static NativeScriptSourceTree tree;
    static NativeScriptIRArena arena;
    tree=(NativeScriptSourceTree){source,(uint32_t)strlen(source),nodes,0,32768};
    arena=(NativeScriptIRArena){source,tree.source_bytes,expressions,0,8192,arguments,0,8192};
    *index=(NativeScriptProgramIndex){.tree=&tree,.expressions=&arena,
        .functions=functions,.function_capacity=2048,.parameters=parameters,.parameter_capacity=4096,
        .values=values,.value_capacity=2048};
    return nativeScriptSource_Parse(&tree,diagnostic) && nativeScriptProgram_Index(index,diagnostic);
}

static void fixture(void)
{
    const char* source=
        "parameter float gain = 0.1 + 0.2\n"
        "parameter bool enabled = true or 1 / 0 > 2\n"
        "attribute float original 12\n"
        "state bool active\n"
        "state int samples[4]\n"
        "func later(float amount) -> float:\n"
        "    return amount * attrs.gain\n"
        "func start() -> float:\n"
        "    state.samples[2] = 4\n"
        "    state.active = attrs.enabled\n"
        "    return later(2.0)\n"
        "article projectile states 1:\n"
        "    state float speed\n"
        "    parameter float gain = 7\n"
        "    func later(float amount) -> float:\n"
        "        return amount + item.params.gain\n"
        "    func start() -> float:\n"
        "        item.state.speed = later(2)\n"
        "        return item.state.speed\n"
        "    on created:\n"
        "        pass\n";
    NativeScriptProgramIndex index;NativeScriptDiagnostic diagnostic;
    CHECK(index_source(source,&index,&diagnostic));
    CHECK(index.function_count==5 && index.parameter_count==2 && index.value_count==7);
    CHECK(values[0].default_value.type==NativeScriptType_F32 && values[0].default_value.as.f32==0.3f);
    CHECK(values[1].default_value.type==NativeScriptType_Bool && values[1].default_value.as.u64==1);
    CHECK(values[2].imported && values[2].import_offset==12);
    CHECK(values[3].byte_offset==0 && values[3].default_value.type==NativeScriptType_Bool);
    CHECK(values[4].byte_offset==4 && values[4].count==4);
    CHECK(values[5].byte_offset==0 && values[5].owner!=0);
    CHECK(values[6].owner==values[5].owner && values[6].default_value.as.f32==7);
    NativeScriptProgramCompiler compiler={.index=&index,.locals=locals,.local_capacity=128};
    NativeScriptStatementCompiler body;
    for (uint32_t i=0;i<4;++i) CHECK(nativeScriptProgram_CompileBody(&compiler,i,&body,&diagnostic));
    CHECK(!nativeScriptProgram_CompileBody(&compiler,4,&body,&diagnostic));
    CHECK(strstr(diagnostic.message,"signature"));
    unsigned root_calls=0,article_calls=0;
    for (uint32_t i=0;i<index.expressions->node_count;++i) {
        NativeScriptIRNode* node=&expressions[i];
        if (node->kind==NativeScriptIR_Call) {
            root_calls+=node->semantic==NATIVE_SCRIPT_FUNCTION_SEMANTIC;
            article_calls+=node->semantic==(NATIVE_SCRIPT_FUNCTION_SEMANTIC|2);
        }
    }
    CHECK(root_calls==1 && article_calls==1);
    const char* invalid[]={
        "parameter float gain\n", "parameter bool gain = 1\n", "parameter int gain = 1.5\n",
        "parameter float gain = unknown\n", "parameter float gain = 1 / 0\n",
        "parameter float gain = 1\nparameter float gain = 2\n",
        "attribute float gain -1\n", "attribute float gain 0.5\n", "attribute float gain 4294967296\n",
        "state int slots[0]\n", "state int slots[-1]\n", "state int slots[65536]\n",
        "func f(int a, float a):\n    pass\n",
        "func f(int a,):\n    pass\n", "func f(int a) -> unknown:\n    pass\n",
        "func f():\n    pass\nfunc f():\n    pass\n"
    };
    for (unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);++i) {
        CHECK(!index_source(invalid[i],&index,&diagnostic));
        CHECK(index.function_count==0 && index.parameter_count==0 && index.value_count==0);
        CHECK(diagnostic.message[0] && diagnostic.source.line);
    }
    CHECK(index_source(source,&index,&diagnostic));
    index.function_capacity=1;
    CHECK(!nativeScriptProgram_Index(&index,&diagnostic));
    CHECK(index.function_count==0 && index.parameter_count==0 && index.value_count==0);
    CHECK(index_source("state int single[1]\nfunc f() -> int:\n    state.single[0] = 3\n    return state.single[0]\n",&index,&diagnostic));
    CHECK(values[0].array && values[0].count==1);
    compiler.index=&index;
    CHECK(nativeScriptProgram_CompileBody(&compiler,0,&body,&diagnostic));
    CHECK(index_source("state int only_fighter\narticle projectile states 1:\n    func bad() -> int:\n        return item.state.only_fighter\n",&index,&diagnostic));
    CHECK(!nativeScriptProgram_CompileBody(&compiler,0,&body,&diagnostic));
    CHECK(index_source("parameter int only_fighter = 2\narticle projectile states 1:\n    func bad() -> int:\n        return item.params.only_fighter\n",&index,&diagnostic));
    CHECK(!nativeScriptProgram_CompileBody(&compiler,0,&body,&diagnostic));
    CHECK(index_source("parameter int shared = 2\narticle projectile states 1:\n    parameter float shared = 3\n    func Read() -> float:\n        return program.attrs.shared + item.params.shared\n",&index,&diagnostic));
    CHECK(nativeScriptProgram_CompileBody(&compiler,0,&body,&diagnostic));
    unsigned shared_reads=0,local_reads=0;
    for(uint32_t i=0;i<index.expressions->node_count;++i) {
        shared_reads+=expressions[i].semantic==NATIVE_SCRIPT_PROGRAM_PARAMETER_SEMANTIC&&expressions[i].type==NativeScriptType_I32;
        local_reads+=expressions[i].semantic==(NATIVE_SCRIPT_PARAMETER_SEMANTIC|1)&&expressions[i].type==NativeScriptType_F32;
    }
    CHECK(shared_reads==1&&local_reads==1);
    const char* invalid_program_access[]={
        "parameter int shared = 2\nfunc Bad():\n    program.attrs.shared = 3\n",
        "state int secret\nfunc Bad() -> int:\n    return program.attrs.secret\n",
        "state int secret\nfunc Bad() -> int:\n    return program.state.secret\n",
        "article projectile states 1:\n    parameter int local = 2\n    func Bad() -> int:\n        return program.attrs.local\n",
        "parameter int shared = 2\nfunc Bad() -> int:\n    return program.attrs.shared[0]\n"
    };
    for(unsigned i=0;i<sizeof(invalid_program_access)/sizeof(*invalid_program_access);++i) {
        CHECK(index_source(invalid_program_access[i],&index,&diagnostic));
        CHECK(!nativeScriptProgram_CompileBody(&compiler,0,&body,&diagnostic));
    }
    CHECK(index_source("func bad() -> int:\n    return item.state.none\n",&index,&diagnostic));
    CHECK(!nativeScriptProgram_CompileBody(&compiler,0,&body,&diagnostic));
}

static void exports(void)
{
    NativeScriptProgramIndex index;NativeScriptDiagnostic diagnostic;
    const char* source="export func costarring() -> int:\n    return 17\nexport func liquid() -> bool:\n    return true\n";
    CHECK(index_source(source,&index,&diagnostic));CHECK(index.export_count==2&&index.export_name_count==0);
    CHECK(index.exports[0].key==UINT32_C(0x5e4daa9d)&&index.exports[1].key==index.exports[0].key);
    CHECK(index.exports[0].function==0&&index.exports[1].function==1);
    uint32_t first=UINT32_MAX,again=UINT32_MAX;
    CHECK(nativeScriptProgram_ExportName(&index,index.exports[0].name,&first,&diagnostic));
    CHECK(nativeScriptProgram_ExportName(&index,index.exports[0].name,&again,&diagnostic));
    CHECK(first==0&&again==first&&index.export_name_count==1);
    CHECK(nativeScriptProgram_ExportName(&index,index.exports[1].name,&again,&diagnostic));
    CHECK(again==1&&index.export_names[0].key==index.export_names[1].key);
    NativeScriptSourceSpan invalid=index.exports[0].name;invalid.offset=index.tree->source_bytes;invalid.length=1;
    again=UINT32_MAX;CHECK(!nativeScriptProgram_ExportName(&index,invalid,&again,&diagnostic));CHECK(again==UINT32_MAX&&index.export_name_count==2);
    const char* bad[]={"export func Bad(int value):\n    pass\n","export func Bad() -> uint:\n    return 1\n",
        "article Probe states 1:\n    export func Bad():\n        pass\n"};
    for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i){CHECK(!index_source(bad[i],&index,&diagnostic));
        CHECK(index.export_count==0&&index.export_name_count==0&&index.function_count==0);}
    char long_name[160];memcpy(long_name,"export func ",12);memset(long_name+12,'a',128);
    memcpy(long_name+140,"():\n    pass\n",14);
    CHECK(!index_source(long_name,&index,&diagnostic));
    CHECK(index.export_count==0&&index.function_count==0&&strstr(diagnostic.message,"127"));
}

int main(int argc,char** argv)
{
    fixture();exports();unsigned declarations=0;
    for (int i=1;i<argc;++i) {
        FILE* file=fopen(argv[i],"rb");CHECK(file);
        CHECK(fseek(file,0,SEEK_END)==0);long bytes=ftell(file);CHECK(bytes>=0);
        rewind(file);char* source=malloc((size_t)bytes+1);CHECK(source);
        CHECK(fread(source,1,(size_t)bytes,file)==(size_t)bytes);fclose(file);source[bytes]=0;
        NativeScriptProgramIndex index;NativeScriptDiagnostic diagnostic;
        if (!index_source(source,&index,&diagnostic)) {
            fprintf(stderr,"%s:%u:%u: %s\n",argv[i],diagnostic.source.line,diagnostic.source.column,diagnostic.message);return 1;
        }
        ++checks;declarations+=index.function_count+index.value_count;
        for (uint32_t v=0;v<index.value_count;++v) {
            CHECK(values[v].default_value.type==values[v].type);
            if (!values[v].parameter) CHECK(values[v].byte_offset% (values[v].type==NativeScriptType_Bool?1:4)==0);
        }
        free(source);
    }
    printf("{\"checks\":%u,\"corpus_files\":%d,\"indexed_declarations\":%u}\n",checks,argc-1,declarations);
    return 0;
}
