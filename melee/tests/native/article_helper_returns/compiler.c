#include "../../../tools/native_fighter_script.h"
#include "../../../tools/native_fighter_item_functions.h"
#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static unsigned cases;
static bool generate(const char* source, const char* path, const NativeFighterScriptConstruction* construction)
{
    FILE* out=path?fopen(path,"w+"):tmpfile(); assert(out);
    NativeFighterScriptError error={0}; ++cases;
    bool ok=nativeFighterScript_GenerateC(source,strlen(source),"article_returns",NULL,NULL, construction,out,&error);
    if (!ok && path) fprintf(stderr,"line %u: %s\n",error.line,error.message);
    if (!ok) assert(error.line && error.message[0]);
    fclose(out); return ok;
}
int main(int argc,char** argv)
{
    assert(argc==4);
    FILE* input=fopen(argv[1],"rb"); assert(input);
    fseek(input,0,SEEK_END); long size=ftell(input); rewind(input);
    char* source=calloc((size_t)size+64,1); assert(source);
    assert(fread(source,1,size,input)==(size_t)size); fclose(input);
    assert(generate(source,argv[2], &(const NativeFighterScriptConstruction){"ftDataNess",341,1}));
    char* action=strstr(source,"action Probe:\n    on load:\n        pass\n"); assert(action);
    const char* replacement="move Probe:\n    on state_341_physics:\n        pass\n";
    char* state=calloc((size_t)size+128,1); assert(state);
    strcpy(state,"");
    strncat(state,source,action-source); strcat(state,replacement);
    strcat(state,action+strlen("action Probe:\n    on load:\n        pass\n"));
    assert(generate(state,argv[3], &(const NativeFighterScriptConstruction){"ftDataNess",341,1})); free(state); free(source);
    const char* invalid[]={
        "    func Bad() -> int:\n        return 1.5\n",
        "    func Bad() -> float:\n        return\n",
        "    func Bad():\n        return 1\n",
        "    func Bad() -> string:\n        return 1\n",
        "    func Bad() -> int:\n        if true:\n            return 1\n",
        "    func Bad() -> int:\n        return Bad()\n",
        "    func Bad() -> int:\n        return Other()\n    func Other() -> int:\n        return Bad()\n",
        "    func Void():\n        pass\n    func Bad() -> int:\n        return Void()\n",
        "    func Value(int x) -> int:\n        return x\n    func Bad() -> int:\n        return Value(1.5)\n",
        "    func Value() -> int:\n        return 1\n    func Bad() -> int:\n        return Value(1)\n",
        "    func Value() -> int:\n        return 1\n    func Bad() -> int:\n        return Value().x\n",
        "    func Value() -> int:\n        return 1\n    on held_status:\n            return Value()\n",
        "    func Bad() -> int:\n        return creator.call(Missing)\n",
        "    func Bad() -> int:\n        return creator.call(Void)\n",
        "    func Bad() -> int:\n        return creator.call(Identity, 1.5)\n",
        "    func Bad() -> int:\n        return creator.call(Identity, 1).x\n",
    };
    char text[2048];
    for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);++i) {
        snprintf(text,sizeof(text),"func Void():\n    pass\nfunc Identity(int x) -> int:\n    return x\n"
            "article It_Kind_Ness_PKFire states 1:\n%s    state 0 anim -1:\n",invalid[i]);
        assert(!generate(text,NULL, &(const NativeFighterScriptConstruction){"ftDataNess",341,1}));
    }
    // Missing final values are rejected, while Float may deliberately widen Int.
    assert(generate("article It_Kind_Ness_PKFire states 1:\n"
        "    func Widen() -> float:\n        return 16777217\n"
        "    func Truth() -> bool:\n        return -1\n"
        "    func TruthFloat() -> bool:\n        return 0.25\n"
        "    on created:\n            let float value = Widen()\n    state 0 anim -1:\n",NULL, &(const NativeFighterScriptConstruction){"ftDataNess",341,1}));
    // Eight arguments are supported, but ordered expansion must still fit the
    // bounded emitted statement. Reject oversized expressions, never truncate.
    assert(!generate("func Eight(int a, int b, int c, int d, int e, int f, int g, int h) -> int:\n"
        "    return a\narticle It_Kind_Ness_PKFire states 1:\n"
        "    on created:\n            let int value = creator.call(Eight, int(item.creation_id), int(item.creation_id), int(item.creation_id), int(item.creation_id), int(item.creation_id), int(item.creation_id), int(item.creation_id), int(item.creation_id))\n"
        "    state 0 anim -1:\n",NULL, &(const NativeFighterScriptConstruction){"ftDataNess",341,1}));
    printf("article helper return compiler: %u programs\n",cases);
    return 0;
}
