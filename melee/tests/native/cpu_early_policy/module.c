#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#ifndef EARLY_CPU_MODULE
#define EARLY_CPU_MODULE "early_cpu_action.inc"
#endif
#include EARLY_CPU_MODULE
__declspec(dllimport) void* __stdcall VirtualAlloc(void*,size_t,unsigned long,unsigned long);
__declspec(dllimport) int __stdcall VirtualProtect(void*,size_t,unsigned long,unsigned long*);
static void* pages;static Fighter* fp;static HSD_GObj* object;
static const NativeFighterDefinition* definition;static uint32_t generation;
static bool live;static unsigned action,checks,calls;
#define CHECK(x) do{++checks;assert(x);}while(0)
static unsigned offset(const char* name){for(unsigned i=0;i<definition->state_field_count;++i)if(!strcmp(name,definition->state_fields[i].name))return definition->state_fields[i].offset;assert(0);return 0;}
static void word(const char* name,int32_t value){memcpy(fp->native_module_state+offset(name),&value,4);}
static int32_t readword(const char* name){int32_t value;memcpy(&value,fp->native_module_state+offset(name),4);return value;}
static void flag(const char* name,bool value){fp->native_module_state[offset(name)]=value;}
static void protect(unsigned mode){unsigned long old;CHECK(VirtualProtect(pages,65536,mode,&old));}
static bool alive(HSD_GObj* source,uint32_t id,uint32_t type){CHECK(source==object&&id==generation&&type==NativeCallbackSource_Fighter);return live&&fp->x8_spawnNum==generation;}
static void landing(HSD_GObj* source){CHECK(source==object);++calls;if(action==1){live=false;protect(1);}else if(action==2)++fp->x8_spawnNum;}
static void reset(unsigned invalidation){protect(4);memset(pages,0,65536);object=pages;fp=(Fighter*)((char*)pages+4096);object->user_data=fp;fp->gobj=object;fp->x8_spawnNum=generation=0x80000007;live=true;action=invalidation;calls=0;word("mode",6);word("step",64);word("limit",127);flag("combat",true);flag("handled",true);flag("invalidate",invalidation!=0);float distance=15;memcpy(fp->native_module_state+offset("distance"),&distance,4);fp->cpu.x3C=40;}
int main(void)
{
 pages=VirtualAlloc(NULL,65536,0x3000,4);CHECK(pages);MotionState motion={0};NativeFighterDefinition base={.fighter_root="ftDataTest",.motion_state_first=341,.motion_state_count=1,.motion_states=&motion};NativeFighterHost host={.enter_landing_basic=landing,.callback_source_alive=alive};
 definition=FreakMelee_GetFighterModule(&base,&host);CHECK(definition&&definition->on_cpu_prepare&&definition->on_cpu_drop_command&&definition->on_cpu_stick_step&&definition->on_cpu_stick_limit);
 NativeFighterHost missing=host;missing.callback_source_alive=NULL;CHECK(!FreakMelee_GetFighterModule(&base,&missing));definition=FreakMelee_GetFighterModule(&base,&host);CHECK(definition);
 const uint32_t ids[]={0,1,0x1000001,0x80000001,0xffffffff};
 for(unsigned i=0;i<5;++i){reset(0);CHECK((uint32_t)definition->on_cpu_prepare(object,ids[i])==ids[i]);CHECK((uint32_t)readword("hint")==ids[i]&&readword("after")==1);CHECK(fp->cpu.xC==6&&fp->cpu.x3C==15&&fp->cpu.xF9_b2);CHECK(definition->on_cpu_drop_command(object));CHECK(definition->on_cpu_stick_step(object)==64&&definition->on_cpu_stick_limit(object)==127);CHECK(readword("after")==4);}
 const float distances[]={-1,INFINITY,-INFINITY,NAN,0,3.5f};
 for(unsigned i=0;i<6;++i){reset(0);memcpy(fp->native_module_state+offset("distance"),&distances[i],4);definition->on_cpu_prepare(object,0);CHECK(fp->cpu.x3C==(i<4?40:distances[i]));}
 for(unsigned mode=1;mode<=2;++mode)for(unsigned hook=0;hook<4;++hook){reset(mode);switch(hook){case 0:CHECK(definition->on_cpu_prepare(object,1)==0);break;case 1:CHECK(!definition->on_cpu_drop_command(object));break;case 2:CHECK(definition->on_cpu_stick_step(object)==0);break;case 3:CHECK(definition->on_cpu_stick_limit(object)==0);break;}protect(4);CHECK(calls==1&&readword("after")==0);}
 printf("PASS %u generated early CPU checks\n",checks);return 0;
}
