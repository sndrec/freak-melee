#include "../../../tools/native_fighter_script.h"
#include "../../../tools/native_fighter_script_pose.h"
#include "../../../tools/native_fighter_script_functions.h"
#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static unsigned cases;
static bool generate(const char* source, const char* path, const NativeFighterScriptConstruction* construction)
{
    FILE* out=path?fopen(path,"w+"):tmpfile(); assert(out);
    NativeFighterScriptError error={0}; ++cases;
    bool ok=nativeFighterScript_GenerateC(source,strlen(source),"pose_probe",NULL,NULL, construction,out,&error);
    if (!ok && path) fprintf(stderr,"line %u: %s\n",error.line,error.message);
    if (!ok) assert(error.line && error.message[0]);
    fclose(out); return ok;
}
int main(int argc,char** argv)
{
    assert(argc==4);
    FILE* input=fopen(argv[1],"rb"); assert(input);
    fseek(input,0,SEEK_END); long size=ftell(input); rewind(input);
    char* source=calloc((size_t)size+128,1); assert(source);
    assert(fread(source,1,size,input)==(size_t)size); fclose(input);
    assert(generate(source,argv[2], &(const NativeFighterScriptConstruction){"ftDataSeak",341,1}));
    const char* original="action Probe:\n    on load:\n        PosePolicy()\n";
    char* action=strstr(source,original); assert(action);
    char* state=calloc((size_t)size+128,1); assert(state);
    strcpy(state,""); strncat(state,source,action-source);
    strcat(state,"move Probe:\n    on state_341_physics:\n        PosePolicy()\n");
    strcat(state,action+strlen(original)); assert(generate(state,argv[3], &(const NativeFighterScriptConstruction){"ftDataSeak",341,1}));
    free(state); free(source);
    const char* declarations[]={
        "pose_graph Bad slot 16 path \"0\"\n",
        "pose_graph Bad slot -1 path \"0\"\n",
        "pose_graph Bad slot 1 path \"256\"\n",
        "pose_graph Bad slot 1 path \"0000\"\n",
        "pose_graph Bad slot 1 path \"0/\"\n",
        "pose_graph Bad slot 1 path \"0//1\"\n",
        "pose_graph Bad slot 1 path \"0/0/0/0/0/0/0/0/0/0/0/0/0\"\n",
        "pose_graph Bad slot 1 path \"0\" extra\n",
        "pose_graph Bad slot 1 path \"0\"\npose_graph Bad slot 2 path \"0\"\n",
        "action Prior:\n    on load:\n        pass\npose_graph Bad slot 1 path \"0\"\n",
    };
    char text[4096];
    for(unsigned i=0;i<sizeof(declarations)/sizeof(*declarations);++i) {
        snprintf(text,sizeof(text),"%saction Probe:\n    on load:\n        pass\n",declarations[i]);
        assert(!generate(text,NULL, &(const NativeFighterScriptConstruction){"ftDataSeak",341,1}));
    }
    const char* invalid[]={
        "pose.evaluate(0)", "pose.evaluate(Pose.live, 1)",
        "pose.load_animation(Pose.live, 1.5)", "pose.load_animation(Pose.live)",
        "pose.request_frame(Pose.live, Unknown)", "pose.copy_costume(Pose.blend, 1.5)",
        "pose.request_frame(Pose.live, true)",
        "pose.blend_to_live(1, true, 0.5)",
        "pose.copy_graph(Pose.live, 1, Missing)", "pose.copy_graph(Pose.live, 1, \"GroundRest\")",
        "pose.blend_graph(Pose.live, 1, GroundRest, 1)", "pose.blend_to_live(1.5, 1, 0)",
        "pose.copy_to_live(1.5)", "pose.copy_to_live(1).x",
        "let float invalid = pose.evaluate(Pose.live)",
    };
    for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);++i) {
        snprintf(text,sizeof(text),"pose_graph GroundRest slot 4 path \"0\"\naction Probe:\n    on load:\n        %s\n",invalid[i]);
        assert(!generate(text,NULL, &(const NativeFighterScriptConstruction){"ftDataSeak",341,1}));
    }
    assert(!generate("article It_Kind_SeakNeedle states 1:\n    on created:\n            pose.evaluate(Pose.live)\n    state 0 anim -1:\n",NULL, &(const NativeFighterScriptConstruction){"ftDataSeak",341,1}));
    assert(!generate("model_effect Effect 1164:\n    on update:\n        pose.evaluate(Pose.live)\naction Probe:\n    on load:\n        pass\n",NULL, &(const NativeFighterScriptConstruction){"ftDataSeak",341,1}));
    strcpy(text,"");
    for(unsigned i=0;i<16;++i) {
        char line[96]; snprintf(line,sizeof(line),"pose_graph Graph%u slot %u path \"\"\n",i,i);
        strcat(text,line);
    }
    strcat(text,"action Probe:\n    on load:\n        pass\n"); assert(generate(text,NULL, &(const NativeFighterScriptConstruction){"ftDataSeak",341,1}));
    char* last=strstr(text,"action Probe:"); assert(last);
    strcpy(last,"pose_graph Beyond slot 0 path \"0\"\naction Probe:\n    on load:\n        pass\n");
    assert(!generate(text,NULL, &(const NativeFighterScriptConstruction){"ftDataSeak",341,1}));
    ScriptCodeBlock block={0}; char guarded[16]; memset(guarded,'x',sizeof(guarded));
    assert(!nativeScriptPose_Action(&block,"pose.evaluate(Pose.live)",guarded+1,3));
    assert(guarded[0]=='x' && guarded[4]=='x');
    printf("fighter pose graph compiler: %u programs\n",cases);
    return 0;
}
