#include "../../../tools/native_fighter_script.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned programs;
static const char* routes[] = {
    "action Policy:\n    on tick:\n        pass\n",
    "move Policy:\n    on state_341_physics:\n        pass\n"
};
static char* generate(const char* text, bool good, const NativeFighterScriptConstruction* construction)
{
    FILE* file=tmpfile();assert(file);NativeFighterScriptError error={0};++programs;
    bool ok=nativeFighterScript_GenerateC(text,strlen(text),"sound_policy",NULL,NULL, construction,file,&error);
    if(ok!=good)fprintf(stderr,"Unexpected %s line%u %s\n%s",ok?"accept":"reject",error.line,error.message,text);
    assert(ok==good);if(!ok){assert(error.line);fclose(file);return NULL;}
    fseek(file,0,SEEK_END);long n=ftell(file);rewind(file);char* code=malloc(n+1);assert(code);
    assert(fread(code,1,n,file)==(size_t)n);code[n]=0;fclose(file);return code;
}
int main(int argc,char** argv)
{
    assert(argc==1||argc==5);
    for(unsigned route=0;route<2;++route){
        for(unsigned mask=0;mask<128;++mask){char source[512],expected[96];
            snprintf(source,sizeof(source),"sound_mute_channels %u\n%s",mask,routes[route]);
            char* code=generate(source,true, route ? &(const NativeFighterScriptConstruction){"ftDataTest",341,1} : NULL);snprintf(expected,sizeof(expected),"generated.sound_mute_channels = %u;",mask);assert(strstr(code,expected));free(code);}
        const struct{const char* literal;unsigned value;} literals[]={{"0x00",0},{"0x7f",127},{"0X44",68},{"010",10},{"068",68},{"127 # filter",127}};
        for(unsigned i=0;i<sizeof(literals)/sizeof(*literals);++i){char source[512],expected[96];
            snprintf(source,sizeof(source),"# context\nsound_mute_channels %s\n%s",literals[i].literal,routes[route]);char* code=generate(source,true, route ? &(const NativeFighterScriptConstruction){"ftDataTest",341,1} : NULL);
            snprintf(expected,sizeof(expected),"generated.sound_mute_channels = %u;",literals[i].value);assert(strstr(code,expected));free(code);}
        char* inherited=generate(routes[route],true, route ? &(const NativeFighterScriptConstruction){"ftDataTest",341,1} : NULL);assert(!strstr(inherited,"generated.sound_mute_channels ="));free(inherited);
        char independent[1024];snprintf(independent,sizeof(independent),"fighter:\n    behavior independent\nsound_mute_channels 68\n%s    on load:\n        pass\n    on load_special_attrs:\n        pass\n",routes[route]);
        char* own=generate(independent,true, route ? &(const NativeFighterScriptConstruction){"ftDataTest",341,1} : NULL);assert(strstr(own,"generated.source_kind = Ft_Kind_None"));assert(strstr(own,"generated.sound_mute_channels = 68;"));free(own);
        const char* bad[]={"128","0x80","-1","+1","true","false","1.0","1 + 2","mask","","0x","0xGG","999999999999999999999","1 extra","1,2","1()"};
        for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i){char source[512];snprintf(source,sizeof(source),"sound_mute_channels %s\n%s",bad[i],routes[route]);generate(source,false, route ? &(const NativeFighterScriptConstruction){"ftDataTest",341,1} : NULL);}
        char source[512];snprintf(source,sizeof(source),"sound_mute_channels 0\nsound_mute_channels 127\n%s",routes[route]);generate(source,false, route ? &(const NativeFighterScriptConstruction){"ftDataTest",341,1} : NULL);
        snprintf(source,sizeof(source),"    sound_mute_channels 0\n%s",routes[route]);generate(source,false, route ? &(const NativeFighterScriptConstruction){"ftDataTest",341,1} : NULL);
        snprintf(source,sizeof(source),"%ssound_mute_channels 0\n",routes[route]);generate(source,false, route ? &(const NativeFighterScriptConstruction){"ftDataTest",341,1} : NULL);
    }
    if(argc==5)for(unsigned i=0;i<4;++i){char source[512];unsigned route=i&1;
        snprintf(source,sizeof(source),"%s%s",i<2?"sound_mute_channels 127\n":"",routes[route]);char* code=generate(source,true, NULL);
        FILE* file=fopen(argv[i+1],"wb");assert(file);assert(fwrite(code,1,strlen(code),file)==strlen(code));fclose(file);free(code);}
    printf("PASS %u sound-policy compiler programs, both routes, all128 masks, decimal/hex/comments/inheritance and strict negatives\n",programs);
    return 0;
}
