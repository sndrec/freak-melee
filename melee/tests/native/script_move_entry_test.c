#include <melee/ft/native_script_moves.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static NativeScriptSourceNode nodes[128];
static NativeScriptSourceTree tree;
static NativeScriptDiagnostic diagnostic;
static unsigned checks, cases;
static NativeScriptMoveLayout layout = {
    .common_count=341, .first=341, .count=20,
    .ground={341,343,356,358}, .air={342,344,357,359}
};
#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr,"move entry %u: %s (%s)\n",__LINE__,#x,diagnostic.message); \
    exit(1); } } while (0)

static void parse(const char* source)
{
    memset(&diagnostic,0,sizeof(diagnostic));
    tree=(NativeScriptSourceTree){.source=source,.source_bytes=(uint32_t)strlen(source),
        .nodes=nodes,.capacity=128};
    CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
}

static void policy(const char* source, bool accepted)
{
    parse(source);
    unsigned policies=0;
    for(uint32_t i=1;i<tree.count;++i) {
        if(nodes[i].kind!=NativeScriptSource_Metadata ||
           !nativeScriptSource_SpanEquals(&tree,nodes[i].name,"entry"))continue;
        bool result=nativeScriptMoves_Metadata(&layout,&tree,i,&diagnostic);
        CHECK(result==accepted);
        if(!accepted)CHECK(diagnostic.message[0]);
        ++policies;
    }
    CHECK(policies>0);
    ++cases;
}

int main(void)
{
    CHECK(nativeScriptMoves_Validate(&layout,&diagnostic));
    policy("move Spin:\n    entry explicit\n    on special_hi:\n        pass\n"
           "    on ground_animation:\n        pass\n",true);
    policy("move Air:\n    entry explicit\n    on special_air_n:\n        pass\n",true);
    policy("entry explicit\n",false);
    policy("fighter:\n    entry explicit\n",false);
    policy("action Test:\n    entry explicit\n",false);
    policy("article Test states 1:\n    entry explicit\n",false);
    policy("move Test:\n    entry implicit\n    on special_n:\n        pass\n",false);
    policy("move Test:\n    entry explicit extra\n    on special_n:\n        pass\n",false);
    policy("move Test:\n    entry explicit:\n        arbitrary value\n"
           "    on special_n:\n        pass\n",false);
    policy("move Test:\n    entry explicit\n    entry explicit\n"
           "    on special_n:\n        pass\n",false);
    policy("move Test:\n    on special_n:\n        pass\n    entry explicit\n",false);
    policy("move Test:\n    entry explicit\n    on ground_animation:\n        pass\n",false);
    policy("move Test:\n    entry explicit\n    on special_n:\n        pass\n"
           "    on special_hi:\n        pass\n",false);

    // Policy acceptance retains the existing numeric motion/phase ownership.
    parse("move Spin:\n    entry explicit\n    on special_hi:\n        pass\n"
          "    on ground_animation:\n        pass\n    on air_collision:\n        pass\n");
    unsigned targets=0;
    for(uint32_t i=1;i<tree.count;++i) {
        if(nodes[i].kind!=NativeScriptSource_Callback)continue;
        CHECK(nativeScriptMoves_Metadata(&layout,&tree,i,&diagnostic));
        if(nativeScriptSource_SpanEquals(&tree,nodes[i].name,"special_hi"))continue;
        NativeScriptMotionTarget target={0};
        CHECK(nativeScriptMoves_Target(&layout,&tree,&nodes[i],&target,&diagnostic));
        CHECK(target.count==1);
        bool ground=nativeScriptSource_SpanEquals(&tree,nodes[i].name,"ground_animation");
        CHECK(target.motions[0]==(ground?356u:357u));
        CHECK(target.phase==(ground?NativeScriptMotion_Animation:NativeScriptMotion_Collision));
        ++targets;
    }
    CHECK(targets==2); ++cases;
    printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u}\n",checks,cases);
    return 0;
}
