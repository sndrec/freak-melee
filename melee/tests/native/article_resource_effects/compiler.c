#include "../../../tools/native_fighter_script.h"
#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static unsigned programs;
static const char* declaration =
    "article It_Kind_Samus_GBeam states 1:\n"
    "    resources capacity 2\n    state int finished\n"
    "    func OperandInt(int value) -> int:\n        item.advance_animation()\n        return value\n"
    "    func OperandFloat(float value) -> float:\n        item.advance_animation()\n        return value\n"
    "    state 0 anim -1:\n        on physics:\n";
static void generate(unsigned route, const char* body, bool expected, const char* path)
{
    char source[8192];
    int size=snprintf(source,sizeof(source),"%s%s%s",route?
        "move Probe:\n    on state_341_physics:\n        pass\n":
        "action Probe:\n    on load:\n        pass\n",declaration,body);
    assert(size>0 && size<sizeof(source));
    FILE* file=path?fopen(path,"wb"):tmpfile(); assert(file);
    NativeFighterScriptError error={0}; ++programs;
    bool ok=nativeFighterScript_GenerateC(source,size,"resource_effects",NULL,NULL, route ? &(const NativeFighterScriptConstruction){"ftDataTest",341,1} : NULL,file,&error);
    if(ok!=expected) fprintf(stderr,"Unexpected %s line %u: %s\n%s\n",ok?"accept":"reject",error.line,error.message,source);
    assert(ok==expected); if(!ok)assert(error.line); fclose(file);
}
int main(int argc,char** argv)
{
    assert(argc==3);
    const char* valid=
        "            item.resource(OperandInt(0)).spawn_effect_at_position(OperandInt(1011), OperandFloat(1), OperandFloat(2), OperandFloat(3))\n"
        "            item.resource(OperandInt(1)).spawn_effect_facing_at_position(OperandInt(1009), OperandFloat(4), OperandFloat(5), OperandFloat(6), OperandFloat(-1))\n"
        "            item.state.finished = 7\n";
    const char* bad[]={
        "item.resource(true).spawn_effect_at_position(1011, 1, 2, 3)",
        "item.resource(0.25).spawn_effect_at_position(1011, 1, 2, 3)",
        "item.resource(0).spawn_effect_at_position(1011.5, 1, 2, 3)",
        "item.resource(0).spawn_effect_at_position(true, 1, 2, 3)",
        "item.resource(0).spawn_effect_at_position(1011, false, 2, 3)",
        "item.resource(0).spawn_effect_at_position(1011, 1, 2)",
        "item.resource(0).spawn_effect_at_position(1011, 1, 2, 3, 4)",
        "item.resource(0).spawn_effect_facing_at_position(1009, 1, 2, 3, true)",
        "item.resource(0).spawn_effect_facing_at_position(1009, 1, 2, 3)",
        "item.resource(0).spawn_effect_at_position(1011, 1, 2, 3) trailing",
        "let bool wrong = item.resource(0).spawn_effect_at_position(1011, 1, 2, 3)",
        "item.resource(0).joint(\"\").spawn_effect_at_position(1011, 1, 2, 3)",
    };
    for(unsigned route=0;route<2;++route){
        generate(route,valid,true,argv[route+1]);
        for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i){char line[1024];snprintf(line,sizeof(line),"            %s\n",bad[i]);generate(route,line,false,NULL);}
    }
    const char* context_bad[]={
        "action Probe:\n    on load:\n        item.resource(0).spawn_effect_at_position(1011, 1, 2, 3)\n",
        "model_effect Probe 0x47C:\n    on update:\n        item.resource(0).spawn_effect_at_position(1011, 1, 2, 3)\naction Probe:\n    on load:\n        pass\n",
        "article It_Kind_Samus_GBeam states 1:\n    state 0 anim -1:\n        on physics:\n            item.resource(0).spawn_effect_at_position(1011, 1, 2, 3)\n",
    };
    for(unsigned i=0;i<sizeof(context_bad)/sizeof(*context_bad);++i) {
        FILE* out=tmpfile();assert(out);NativeFighterScriptError error={0};++programs;
        assert(!nativeFighterScript_GenerateC(context_bad[i],strlen(context_bad[i]),"resource_effects",NULL,NULL, NULL,out,&error));
        assert(error.line);fclose(out);
    }
    printf("{\"compiler_programs\":%u}\n",programs);
}
