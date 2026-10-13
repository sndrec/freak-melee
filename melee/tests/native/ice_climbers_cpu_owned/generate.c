#include "native_fighter_script.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned validation_checks;
static void validate(void)
{
    const char* cases[] = {
        "let bool found = targets.is_fighter(int(-2147483647))",
        "let float radius = ground.mid_walk_point",
        "let bool found = targets.is_fighter(true)",
        "let bool found = targets.is_fighter(1.5)",
        "ground.mid_walk_point = 1.5",
        "let int radius = ground.mid_walk_point",
    };
    for (unsigned route=0;route<2;++route) for(unsigned i=0;i<6;++i) {
        char source[500];
        snprintf(source,sizeof(source),"%s%s\n    on load:\n        %s\n%s",
                 route?"":"",route?"move Check:":"action Check:",cases[i],
                 route?"    on state_341_physics:\n        pass\n":"");
        FILE* out=tmpfile(); NativeFighterScriptError error={0};
        bool result=nativeFighterScript_GenerateC(source,strlen(source),"check",NULL,NULL, route ? &(const NativeFighterScriptConstruction){"ftDataTest",341,1} : NULL,out,&error);
        fclose(out); ++validation_checks;
        if(result!=(i<2)){fprintf(stderr,"Overlay contract case%u route%u: %u %s\n",i,route,error.line,error.message);exit(3);}
    }
    const char* invalid_contexts[]={
        "model_effect E 0x47c:\n    on update:\n        let float value = ground.mid_walk_point\naction Check:\n    on load:\n        pass\n",
        "model_effect E 0x47c:\n    on update:\n        let bool value = targets.is_fighter(int(1))\naction Check:\n    on load:\n        pass\n",
        "article It_Kind_Ness_PKFire states 1:\n    state 0 anim -1:\n        on physics:\n            let float value = ground.mid_walk_point\naction Check:\n    on load:\n        pass\n",
    };
    for(unsigned i=0;i<3;++i){FILE* out=tmpfile();NativeFighterScriptError error={0};bool ok=nativeFighterScript_GenerateC(invalid_contexts[i],strlen(invalid_contexts[i]),"check",NULL,NULL,NULL,out,&error);fclose(out);++validation_checks;if(ok)exit(3);}
}
int main(int argc, char** argv)
{
    if (argc != 4) return 2;
    validate();
    FILE* input = fopen(argv[1], "rb");
    if (!input) return 2;
    fseek(input, 0, SEEK_END); long size = ftell(input); rewind(input);
    char* source = malloc((size_t)size + 1);
    if (!source || fread(source, 1, size, input) != (size_t)size) return 2;
    source[size] = 0; fclose(input);
    FILE* tuning = fopen(argv[2], "rb");
    if (!tuning) return 2;
    fseek(tuning, 0, SEEK_END); long tuning_size = ftell(tuning); rewind(tuning);
    char* tuning_text = malloc((size_t)tuning_size + 1);
    if (!tuning_text || fread(tuning_text, 1, tuning_size, tuning) != (size_t)tuning_size) return 2;
    fclose(tuning); NativeFighterParameters parameters; unsigned line;
    if (!nativeFighterParameters_Parse(tuning_text, tuning_size, &parameters, &line)) return 2;
    free(tuning_text);
    FILE* output = fopen(argv[3], "wb");
    if (!output) return 2;
    NativeFighterScriptError error = { 0 };
    bool ok = nativeFighterScript_GenerateC(source, size, "owned_follower",
                                            NULL, &parameters, &(const NativeFighterScriptConstruction){"ftDataNana",341,26}, output, &error);
    fclose(output); free(source);
    if (!ok) fprintf(stderr, "source line %u: %s\n", error.line, error.message);
    else printf("PASS %u overlay grammar/context/type checks\n",validation_checks);
    return ok ? 0 : 1;
}
