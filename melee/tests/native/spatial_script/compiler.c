#include "../../../tools/native_fighter_script.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool generate(const char* text, FILE* output)
{
    NativeFighterScriptError error={0};
    bool result=nativeFighterScript_GenerateC(text, strlen(text), "spatial", NULL, NULL, NULL, output, &error);
    if (!result) fprintf(stderr,"Line %u: %s\n",error.line,error.message);
    return result;
}

int main(int argc,char** argv)
{
    assert(argc==3);
    FILE* input=fopen(argv[1],"rb"); assert(input);
    char text[8192]; size_t size=fread(text,1,sizeof(text)-1,input); assert(feof(input));
    text[size]=0; fclose(input);
    FILE* output=fopen(argv[2],"w+b"); assert(output && generate(text,output));
    rewind(output); char code[65536]; size=fread(code,1,sizeof(code)-1,output); assert(feof(output));code[size]=0;fclose(output);
    assert(strstr(code,"host->spawn_wind == 0") && strstr(code,"host->random_int == 0"));
    assert(strstr(code,"host->item_joint_world_position == 0"));
    const char* bad[]={
        "effects.wind_at_position(1,2,3,1.5,5,6,7)",
        "effects.wind_at_position(true,2,3,4,5,6,7)",
        "item.joint_world_position(1.5,\"0\",x,y,z)",
        "item.joint_world_position(0,\"256\",x,y,z)",
        "item.joint_world_position(0,\"0/\",x,y,z)",
        "item.joint_world_position(0,\"0\",x,x,z)",
        "item.joint_world_position(0,\"0\",x,y,undeclared)",
        "item.joint_world_position(0,\"0\",x,y,1)",
        "item.joint_world_position(0,\"0\",x,y,item.state.count)",
    };
    for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);++i){
        snprintf(text,sizeof(text),"article It_Kind_GameWatch_Manhole states 1:\n    state int count\n    state 0 anim -1:\n        on physics:\n            let float x = 0\n            let float y = 0\n            let float z = 0\n            %s\n",bad[i]);
        output=tmpfile();assert(output && !generate(text,output));fclose(output);
    }
    output=tmpfile();assert(output && !generate("action X:\n    on load:\n        item.joint_world_position(0,\"0\",x,y,z)\n",output));fclose(output);
    puts("PASS spatial compiler validation and generated Host requirements");
}
