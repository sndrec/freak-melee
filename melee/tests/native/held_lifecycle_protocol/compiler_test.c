#include "../../../tools/native_fighter_script.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do { ++checks; assert(x); } while (0)
static char* read_file(FILE* f) { CHECK(!fseek(f,0,SEEK_END));long n=ftell(f);CHECK(n>=0);rewind(f);char* p=malloc(n+1);CHECK(p);CHECK(fread(p,1,n,f)==n);p[n]=0;return p; }
static bool generate(const char* source,FILE* out, const NativeFighterScriptConstruction* construction) {
    NativeFighterScriptError error={0};
    bool ok=nativeFighterScript_GenerateC(source,strlen(source),"held_lifecycle",NULL,NULL, construction,out,&error);
    if (!ok) fprintf(stderr,"%u: %s\n",error.line,error.message);
    return ok;
}
int main(int argc,char**argv) {
    CHECK(argc==4); FILE* in=fopen(argv[1],"rb");CHECK(in);char* source=read_file(in);fclose(in);
    for(unsigned form=0;form<2;++form) {
        char* text=malloc(strlen(source)+64);CHECK(text);
        if(form) { strcpy(text,"");strcat(text,source);char* action=strstr(text,"action Lifecycle:");CHECK(action);memmove(action+4,action+6,strlen(action+6)+1);memcpy(action,"move",4); }
        else strcpy(text,source);
        FILE* out=fopen(argv[form+2],"w+b");CHECK(out);CHECK(generate(text,out, form ? &(const NativeFighterScriptConstruction){"ftDataMario",341,1} : NULL));char* code=read_file(out);
        CHECK(strstr(code,"float blend"));CHECK(strstr(code,"return previous_motion;"));CHECK(strstr(code,"ftCo_MS_FallAerial"));
        CHECK(strstr(code,"fighter->mv.co.landing.allow_interrupt"));
        CHECK(strstr(code,"engine->notify_held_neutral(gobj, NULL)"));
        CHECK(strstr(code,"host->notify_held_neutral == 0"));CHECK(strstr(code,"host->callback_source_alive == 0"));
        CHECK(strstr(code,"fighter->x2221_b5"));
        CHECK(!strstr(code,"ftPe_")&&!strstr(code,"it_802BDB94"));
        free(code);free(text);fclose(out);
    }
    const char* invalid[]={
        "action X:\n    on load:\n        let bool bad = items.reopen_blocked()\n",
        "func bad():\n    let float value = event.blend\naction X:\n    on neutral_enter:\n        bad()\n",
        "func bad():\n    let int value = event.previous_motion\naction X:\n    on air_resume_motion:\n        return 1\n",
        "action X:\n    on load:\n        let bool bad = event.interrupt_allowed\n",
        "func bad() -> bool:\n    return event.interrupt_allowed\naction X:\n    on landing_action:\n        let bool value = bad()\n",
        "action X:\n    on landing_action:\n        event.interrupt_allowed = true\n",
        "action X:\n    on landing_action:\n        let bool bad = event.interrupt_allowed()\n",
        "action X:\n    on load:\n        let float bad = event.blend\n",
        "action X:\n    on neutral_enter:\n        let int bad = event.previous_motion\n",
        "action X:\n    on air_resume_motion:\n        return true\n",
        "action X:\n    on air_resume_motion:\n        return 1.5\n",
        "action X:\n    on air_resume_motion:\n        let float bad = event.blend\n",
        "action X:\n    on load:\n        items.reopen_blocked = unknown\n",
        "action X:\n    on load:\n        let bool bad = items.notify_held_neutral(1)\n",
        "article It_Kind_Peach_Parasol states 1:\n    state 0 anim -1:\n    on holder_neutral:\n            let bool bad = event.interrupt_allowed\n",
        "article It_Kind_Peach_Parasol states 1:\n    state 0 anim -1:\n    on holder_neutral:\n            let bool bad = items.notify_held_neutral()\n",
        "article It_Kind_Peach_Parasol states 1:\n    state 0 anim -1:\n    on holder_neutral:\n            return true\n",
    };
    for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);++i) {FILE*out=tmpfile();CHECK(out);if(generate(invalid[i],out, NULL)){fprintf(stderr,"Unexpected acceptance %u: %s",i,invalid[i]);abort();}++checks;fclose(out);}
    free(source);printf("Lifecycle compiler PASS %u checks\n",checks);return 0;
}
