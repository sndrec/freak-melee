#include <melee/ft/native_script_source.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
static NativeScriptSourceNode nodes[2048];
#define CHECK(value) do {++checks; assert(value);} while (0)

static void fixture(void)
{
    const char* source=
        "parameter float gravity = 0.08\n"
        "state int charge\n"
        "export func boost(float value) -> float:\n"
        "    let float result = value * 0.95 # source numeric types remain explicit\n"
        "    if result > 0:\n"
        "        while result > 1:\n"
        "            result = result - 1\n"
        "    elif result == 0:\n"
        "        pass\n"
        "    else:\n"
        "        result = 0\n"
        "    return result\n"
        "article It_Kind_Example states 2:\n"
        "    state bool active[40]\n"
        "    resources capacity 40\n"
        "    on created:\n"
        "            item.state.active[0] = true\n"
        "            item.resource(0).joint(\"0/#:part\").request_animation(0)\n"
        "    state 0 anim -1:\n"
        "        on physics:\n"
        "            pass\n"
        "    state 1 anim -1 use 0:\n";
    NativeScriptSourceTree tree={source,(uint32_t)strlen(source),nodes,0,2048};
    NativeScriptDiagnostic diagnostic;
    CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
    unsigned functions=0, callbacks=0, assignments=0, branches=0;
    for (uint32_t i=1;i<tree.count;++i) {
        NativeScriptSourceNode* n=&nodes[i];
        CHECK(n->parent<i && n->source.offset+n->source.length<=tree.source_bytes);
        CHECK(n->expression==UINT32_MAX && n->target_expression==UINT32_MAX);
        if (n->kind==NativeScriptSource_Function) {
            ++functions;CHECK(n->exported && nativeScriptSource_SpanEquals(&tree,n->name,"boost"));
        }
        callbacks+=n->kind==NativeScriptSource_Callback;
        assignments+=n->kind==NativeScriptSource_Assign;
        branches+=n->kind==NativeScriptSource_If || n->kind==NativeScriptSource_ElseIf || n->kind==NativeScriptSource_Else;
        if (n->kind==NativeScriptSource_Expression) {
            CHECK(nativeScriptSource_SpanEquals(&tree,n->value,
                "item.resource(0).joint(\"0/#:part\").request_animation(0)"));
        }
        if (n->next!=UINT32_MAX) CHECK(nodes[n->next].parent==n->parent && n->next>i);
    }
    CHECK(functions==1 && callbacks==2 && assignments==3 && branches==3);
    const char* invalid[]={
        "func f():\n    if true:\n        pass\n      pass\n",
        "func f():\n\tpass\n",
        "func f():\n    pass\n        pass\n",
        "func f():\n    else:\n        pass\n",
        "func f():\n    if true:\n    pass\n",
        "func f():\n",
        "func f():\n    let bool x =\n",
        "func f():\n    let unknown x = 1\n",
        "func f():\n    x = y = 2\n",
        "func f():\n    foo(\"unfinished)\n",
        "func f():\n    foo([1))\n",
        "func f():\n    returnValue:\n        pass\n"
    };
    for (unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);++i) {
        tree.source=invalid[i];tree.source_bytes=(uint32_t)strlen(invalid[i]);
        CHECK(!nativeScriptSource_Parse(&tree,&diagnostic) && !tree.count);
        CHECK(diagnostic.message[0] && diagnostic.source.line);
    }
    tree.source=source;tree.source_bytes=(uint32_t)strlen(source);tree.capacity=2;
    CHECK(!nativeScriptSource_Parse(&tree,&diagnostic) && !tree.count);
    CHECK(strstr(diagnostic.message,"capacity"));
}

static void corpus(const char* path)
{
    FILE* file=fopen(path,"rb");CHECK(file!=NULL);
    CHECK(fseek(file,0,SEEK_END)==0);long bytes=ftell(file);CHECK(bytes>=0);
    CHECK(fseek(file,0,SEEK_SET)==0);
    char* source=malloc((size_t)bytes+1);CHECK(source!=NULL);
    CHECK(fread(source,1,(size_t)bytes,file)==(size_t)bytes);fclose(file);source[bytes]=0;
    unsigned lines=1;
    for (long i=0;i<bytes;++i) lines+=source[i]=='\n';
    NativeScriptSourceNode* storage=calloc(lines*2+1,sizeof(*storage));CHECK(storage!=NULL);
    NativeScriptSourceTree tree={source,(uint32_t)bytes,storage,0,lines*2+1};
    NativeScriptDiagnostic diagnostic;
    if (!nativeScriptSource_Parse(&tree,&diagnostic)) {
        fprintf(stderr,"%s:%u:%u: %s\n",path,diagnostic.source.line,diagnostic.source.column,diagnostic.message);
        exit(1);
    }
    unsigned executable_nodes=0;
    for (uint32_t i=1;i<tree.count;++i) {
        CHECK(storage[i].parent<i);
        executable_nodes+=storage[i].kind>=NativeScriptSource_If;
    }
    printf("{\"file\":\"%s\",\"source_nodes\":%u,\"executable_nodes\":%u}\n",path,tree.count,executable_nodes);
    free(storage);free(source);
}

int main(int argc,char** argv)
{
    fixture();
    for (int i=1;i<argc;++i) corpus(argv[i]);
    printf("{\"checks\":%u,\"syntax_only\":true}\n",checks);
    return 0;
}
