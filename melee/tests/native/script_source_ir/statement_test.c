#include <melee/ft/native_script_statement.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

static NativeScriptSourceNode source_nodes[256];
static NativeScriptIRNode expression_nodes[1024];
static uint32_t arguments[128];
static NativeScriptLocal locals[32];
static unsigned checks;
#define CHECK(v) do {++checks;assert(v);} while(0)

static bool text(const NativeScriptIRArena* a,const NativeScriptIRNode* n,const char* value)
{
    return strlen(value)==n->text_length && memcmp(a->source+n->text_offset,value,n->text_length)==0;
}
static bool binding(void* context,const NativeScriptIRArena* arena,const NativeScriptIRNode* node,
    NativeScriptIRResolution* out,NativeScriptDiagnostic* diagnostic)
{
    (void)context;
    if (node->kind==NativeScriptIR_Name && (text(arena,node,"value") || text(arena,node,"immutable"))) {
        *out=(NativeScriptIRResolution){NativeScriptType_F32,1,NativeScriptFlag_Readable,0};return true;
    }
    if (node->kind==NativeScriptIR_Name && text(arena,node,"velocity")) {
        *out=(NativeScriptIRResolution){NativeScriptType_Namespace,2,0,0};return true;
    }
    if (node->kind==NativeScriptIR_Member && arena->nodes[node->a].semantic==2 && text(arena,node,"x")) {
        *out=(NativeScriptIRResolution){NativeScriptType_F32,3,NativeScriptFlag_Readable|NativeScriptFlag_Writable|NativeScriptFlag_Addressable,0};return true;
    }
    diagnostic->source=node->source;
    snprintf(diagnostic->message,sizeof(diagnostic->message),"Unknown test binding");return false;
}

static bool compile(const char* source,NativeScriptDiagnostic* diagnostic,
                     NativeScriptStatementCompiler* compiler,NativeScriptSourceTree* tree,NativeScriptIRArena* arena)
{
    *tree=(NativeScriptSourceTree){source,(uint32_t)strlen(source),source_nodes,0,256};
    CHECK(nativeScriptSource_Parse(tree,diagnostic));
    *arena=(NativeScriptIRArena){source,tree->source_bytes,expression_nodes,0,1024,arguments,0,128};
    *compiler=(NativeScriptStatementCompiler){.tree=tree,.expressions=arena,.bindings=binding,
        .locals=locals,.local_capacity=32,.return_type=NativeScriptType_F32};
    return nativeScriptStatement_Compile(compiler,1,diagnostic);
}

int main(void)
{
    const char* source="func compute(float value) -> float:\n"
        "    let float speed = value * 0.95\n"
        "    let int count = 3\n"
        "    while count > 0:\n"
        "        let float speed = 1\n"
        "        velocity.x = speed\n"
        "        count = count - 1\n"
        "        if count == 1: continue\n"
        "    if speed < 0: speed = 0\n"
        "    return speed\n";
    NativeScriptDiagnostic diagnostic;
    NativeScriptStatementCompiler compiler;
    NativeScriptSourceTree tree;
    NativeScriptIRArena arena;
    CHECK(compile(source,&diagnostic,&compiler,&tree,&arena));
    CHECK(compiler.local_count==3 && arena.node_count>0);
    unsigned assignments=0;
    for (uint32_t i=1;i<tree.count;++i) {
        if (source_nodes[i].kind==NativeScriptSource_Assign) {
            ++assignments;
            CHECK(source_nodes[i].expression!=UINT32_MAX && source_nodes[i].target_expression!=UINT32_MAX);
        }
    }
    CHECK(assignments==3);
    const char* invalid[]={
        "func f():\n    let float scoped = 0\n    if true:\n        let float inner = 1\n    return inner\n",
        "func f():\n    let bool flag = 1\n    return value\n",
        "func f():\n    let int count = 0.5\n    return value\n",
        "func f():\n    immutable = 3\n    return value\n",
        "func f():\n    break\n    return value\n",
        "func f():\n    let int duplicate = 0\n    let int duplicate = 1\n    return value\n",
        "func f():\n    return\n",
        "func f():\n    value + 3\n    return value\n"
    };
    for (unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);++i) {
        CHECK(!compile(invalid[i],&diagnostic,&compiler,&tree,&arena));
        CHECK(arena.node_count==0 && arena.argument_count==0 && compiler.local_count==0);
        CHECK(diagnostic.message[0]);
        for (uint32_t n=2;n<tree.count;++n)
            CHECK(source_nodes[n].expression==UINT32_MAX && source_nodes[n].target_expression==UINT32_MAX);
    }
    printf("{\"checks\":%u,\"typed_statement_ir\":true}\n",checks);
    return 0;
}
