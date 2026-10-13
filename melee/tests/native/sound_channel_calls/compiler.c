#include "../../../tools/native_fighter_script.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned programs;
static char* generate(const char* source, bool expected, const NativeFighterScriptConstruction* construction)
{
    FILE* file=tmpfile(); assert(file); NativeFighterScriptError error={0}; ++programs;
    bool ok=nativeFighterScript_GenerateC(source,strlen(source),"sound_channels",NULL,NULL, construction,file,&error);
    if(ok!=expected)fprintf(stderr,"Unexpected %s at%u: %s\n%s",ok?"accept":"reject",error.line,error.message,source);
    assert(ok==expected); if(!ok){assert(error.line);fclose(file);return NULL;}
    fseek(file,0,SEEK_END);long size=ftell(file);rewind(file);char* code=malloc(size+1);assert(code);
    assert(fread(code,1,size,file)==(size_t)size);code[size]=0;fclose(file);return code;
}

static const char fields[]="state int channel\nstate int sound_id\nstate int volume\nstate int pan\nstate int order\nstate int after\n";
static const char helpers[]=
    "func Channel() -> int:\n    state.order = state.order * 10 + 1\n    state.basic_landing()\n    return state.channel\n"
    "func Id() -> int:\n    state.order = state.order * 10 + 2\n    state.basic_landing()\n    return state.sound_id\n"
    "func Volume() -> int:\n    state.order = state.order * 10 + 3\n    state.basic_landing()\n    return state.volume\n"
    "func Pan() -> int:\n    state.order = state.order * 10 + 4\n    state.basic_landing()\n    return state.pan\n";

int main(int argc,char** argv)
{
    assert(argc==1||argc==3);
    const char* bad[]={
        "sound.channel()","sound.channel(0,1,2)","sound.channel(0,1,2,3,4)",
        "sound.channel(true,1,2,3)","sound.channel(0.5,1,2,3)",
        "sound.channel(0,false,2,3)","sound.channel(0,1,true,3)","sound.channel(0,1,2,false)",
        "sound.channel(0,1.0,2,3)","sound.channel(0,1,2.0,3)","sound.channel(0,1,2,3.0)",
        "let int result = sound.channel(0,1,2,3)","sound.channel(0,1,2,3) + 1","sound.channel",
    };
    for(unsigned route=0;route<2;++route){
        const char* entry=route?"move Sounds:\n    on state_341_physics:\n":"action Sounds:\n    on load:\n";
        char source[8192];
        for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);++i){snprintf(source,sizeof(source),"%s        %s\n",entry,bad[i]);free(generate(source,false, route ? &(const NativeFighterScriptConstruction){"ftDataTest",341,1} : NULL));}
        snprintf(source,sizeof(source),"%s%s%s%s        sound.channel(Channel(), Id(), Volume(), Pan())\n        state.after = 99\n",route?"":"",fields,helpers,route?"move Sounds:\n    on state_341_physics:\n":"action Sounds:\n    on load:\n");
        char* code=generate(source,true, route ? &(const NativeFighterScriptConstruction){"ftDataTest",341,1} : NULL);
        assert(strstr(code,"engine->sound_channels[sound_channel](fighter,sound_id"));
        assert(strstr(code,"host->sound_channels == 0"));
        for(unsigned i=0;i<7;++i){char required[80];snprintf(required,sizeof(required),"host->sound_channels[%u] == 0",i);assert(strstr(code,required));}
        assert(strstr(code,"host->callback_source_alive == 0"));
        assert(strstr(code,"engine->callback_source_alive(gobj, callback_generation, NativeCallbackSource_Fighter)"));
        if(argc==3){FILE* out=fopen(argv[route+1],"wb");assert(out);assert(fwrite(code,1,strlen(code),out)==strlen(code));fclose(out);}free(code);
        const char* good[]={"sound.channel(0, -1, 127, 64)","sound.channel(int(6), int(16777217), int(300), int(-1))","sound.channel(int(input.attack_held), 1, 2, 3)"};
        for(unsigned i=0;i<sizeof(good)/sizeof(good[0]);++i){snprintf(source,sizeof(source),"%s        %s\n",entry,good[i]);free(generate(source,true, route ? &(const NativeFighterScriptConstruction){"ftDataTest",341,1} : NULL));}
    }
    free(generate("action Sounds:\n    on load:\n        pass\narticle It_Kind_Ness_PKFire states 1:\n    on created:\n            sound.channel(0,1,2,3)\n    state 0 anim -1:\n",false, NULL));
    free(generate("model_effect SoundModel 0x47c:\n    on update:\n        sound.channel(0,1,2,3)\naction Sounds:\n    on load:\n        pass\n",false, NULL));
    printf("PASS %u sound-channel compiler programs, both routes, typed/context/arity negatives\n",programs);return 0;
}
