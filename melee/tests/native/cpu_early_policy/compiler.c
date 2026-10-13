#include "../../../tools/native_fighter_script.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned programs;
static char* generate(const char* source,bool expected, const NativeFighterScriptConstruction* construction)
{
    FILE* out=tmpfile();assert(out);NativeFighterScriptError error={0};++programs;
    bool ok=nativeFighterScript_GenerateC(source,strlen(source),"early_cpu",NULL,NULL, construction,out,&error);
    if(ok!=expected)fprintf(stderr,"Unexpected %s at %u: %s\n%s",ok?"accept":"reject",error.line,error.message,source);
    assert(ok==expected);if(!ok){assert(error.line);fclose(out);return NULL;}
    fseek(out,0,SEEK_END);long size=ftell(out);rewind(out);char* code=malloc(size+1);assert(code);
    assert(fread(code,1,size,out)==(size_t)size);code[size]=0;fclose(out);return code;
}
static const char fields[]=
 "state int mode\nstate float distance\nstate bool combat\nstate bool handled\nstate int step\nstate int limit\nstate int hint\nstate bool invalidate\nstate int after\n"
;
static const char policy[]=
 "on cpu_prepare:\n    state.hint = event.history_source\n    cpu.mode = state.mode\n    cpu.follow_distance = state.distance\n    cpu.combat_candidates_enabled = state.combat\n    if state.invalidate:\n        state.basic_landing()\n    state.after = state.after + 1\n    return event.history_source\n"
 "on cpu_initialize:\n    cpu.set_approach_spacing(1)\n"
 "on cpu_drop_command:\n    if state.invalidate:\n        state.basic_landing()\n    state.after = state.after + 1\n    return state.handled\n"
 "on cpu_stick_step:\n    if state.invalidate:\n        state.basic_landing()\n    state.after = state.after + 1\n    return state.step\n"
 "on cpu_stick_limit:\n    if state.invalidate:\n        state.basic_landing()\n    state.after = state.after + 1\n    return state.limit\n";
static void source_program(char* output,size_t capacity,unsigned route,const char* variables,const char* handlers)
{
 int n=snprintf(output,capacity,"%s%s%s",route?"":"",variables,route?"move Early:\n":"action Early:\n");assert(n>=0&&(size_t)n<capacity);
 size_t used=n;
 while(*handlers){const char* end=strchr(handlers,'\n');size_t length=end?(size_t)(end-handlers)+1:strlen(handlers);n=snprintf(output+used,capacity-used,"%s%.*s","    ",(int)length,handlers);assert(n>=0&&(size_t)n<capacity-used);used+=n;handlers+=length;}
 n=snprintf(output+used,capacity-used,"%s",route?"    on state_341_physics:\n        pass\n":"    on load:\n        pass\n");assert(n>=0&&(size_t)n<capacity-used);
}
int main(int argc,char** argv)
{
 assert(argc==1||argc==3);char source[8192];
 for(unsigned route=0;route<2;++route){
  source_program(source,sizeof(source),route,fields,policy);
  char* code=generate(source,true, NULL);assert(strstr(code,"uint32_t history_source"));assert(strstr(code,"on_cpu_prepare = authored_"));assert(strstr(code,"host->callback_source_alive == 0"));
  if(argc==3){FILE* out=fopen(argv[route+1],"wb");assert(out);assert(fwrite(code,1,strlen(code),out)==strlen(code));fclose(out);}free(code);
  const char* bad[]={
   "on cpu_prepare:\n    return true\n", "on cpu_prepare:\n    return 1.25\n",
   "on cpu_stick_step:\n    return false\n", "on cpu_stick_limit:\n    return 0.5\n",
   "on cpu_drop_command:\n    return int(1)\n", "on cpu_drop_command:\n    return 1.5\n",
   "on cpu_initialize:\n    let int value = event.history_source\n",
   "on cpu_prepare:\n    event.history_source = int(4)\n    return int(0)\n",
   "on cpu_prepare:\n    cpu.mode = true\n    return int(0)\n",
   "on cpu_prepare:\n    cpu.mode = 2.5\n    return int(0)\n",
   "on cpu_prepare:\n    cpu.follow_distance = false\n    return int(0)\n",
   "on cpu_prepare:\n    cpu.combat_candidates_enabled = int(1)\n    return int(0)\n",
  };
  for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i){source_program(source,sizeof(source),route,"",bad[i]);free(generate(source,false, NULL));}
  const char* good[]={"on cpu_prepare:\n    cpu.mode = int(-2147483647)\n    cpu.follow_distance = int(15)\n    cpu.combat_candidates_enabled = true\n    return int(0)\n", "on cpu_stick_step:\n    return int(0)\non cpu_stick_limit:\n    return int(127)\n"};
  for(unsigned i=0;i<2;++i){source_program(source,sizeof(source),route,"",good[i]);free(generate(source,true, NULL));}
 }
 free(generate("article It_Kind_Ness_PKFire states 1:\n    on created:\n        cpu.mode = int(6)\n    state 0 anim -1:\naction Early:\n    on load:\n        pass\n",false, NULL));
 free(generate("article It_Kind_Ness_PKFire states 1:\n    on created:\n        let int hint = event.history_source\n    state 0 anim -1:\naction Early:\n    on load:\n        pass\n",false, NULL));
 free(generate("model_effect EarlyEffect 0x47c:\n    on update:\n        cpu.mode = int(6)\naction Early:\n    on load:\n        pass\n",false, NULL));
 printf("PASS %u early CPU compiler programs, both routes\n",programs);return 0;
}
