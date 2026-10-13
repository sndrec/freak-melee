#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <placeholder.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftcpuattack.h>
#include <melee/ft/ftcmdscript.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftcommon_assets.h>
#include <melee/ft/kinds/ftCommon/ftCo_0A01.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/gr/stage.h>
#include <melee/gr/ground.h>
#include <melee/gr/types.h>
#include <melee/mp/types.h>
#include <melee/mp/mplib.h>
#include <melee/mp/mpisland.h>
#ifdef EARLY_CPU_MODULE
#include EARLY_CPU_MODULE
#endif
static unsigned checks,cases,trace_count,hook_calls,b9704_calls,late_calls;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"%d case%u: %s\n",__LINE__,cases,#x);exit(1);}}while(0)
static int trace[256][3];static Fighter partner,replacement;static HSD_GObj partner_object;
static NativeFighterDefinition definition,other_definition;static const NativeFighterDefinition* active;
static ftData original_data,other_data;static bool live,floor_hit,retire_history_at_floor;static unsigned retirement,retire_hook;
static int32_t step_result,limit_result;static bool drop_result;static uint32_t hint,history_result;
static void* pages;static HSD_GObj* object;static Fighter* fp;static uint32_t generation;
StageInfo stage_info;
ftCommonData* p_ftCommonData;
static ftCommonData common;
static void protect(unsigned protection){DWORD old;CHECK(VirtualProtect(pages,65536,protection,&old));}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind){(void)kind;return active;}
uint32_t nativeFighterRuntime_CpuHistorySource(Fighter_GObj* source){CHECK(source==object);return hint;}
bool nativeCallbackSource_Snapshot(HSD_GObj* source,uint32_t type,uint32_t* id){CHECK(source==object&&type==NativeCallbackSource_Fighter);if(!live)return false;*id=((Fighter*)source->user_data)->x8_spawnNum;return true;}
bool nativeCallbackSource_Alive(HSD_GObj* source,uint32_t id,uint32_t type){CHECK(source==object&&type==NativeCallbackSource_Fighter);return live&&((Fighter*)source->user_data)->x8_spawnNum==id;}
Fighter_GObj* ftLib_8008741C(u32 id){return id&&id==partner.x8_spawnNum?&partner_object:NULL;}
static void invalidate(void){
 if(++hook_calls!=retire_hook)return;
 switch(retirement){case 1:live=false;protect(PAGE_NOACCESS);break;case 2:++fp->x8_spawnNum;break;case 3:fp->kind=Ft_Kind_Mars;break;case 4:active=&other_definition;break;case 5:replacement=*fp;object->user_data=&replacement;break;case 6:fp->ft_data=&other_data;break;}
}
static int32_t prepare(Fighter_GObj* source,uint32_t input){CHECK(source==object&&input==hint);CHECK(fp->cpu.xC==3&&fp->cpu.x3C==40&&fp->cpu.level==4&&fp->cpu.x14==7&&!fp->cpu.xF9_b2);fp->cpu.xC=6;fp->cpu.x3C=15;fp->cpu.xF9_b2=true;invalidate();return (int32_t)history_result;}
static int32_t step(Fighter_GObj* source){CHECK(source==object);invalidate();return step_result;}
static int32_t limit(Fighter_GObj* source){CHECK(source==object);invalidate();return limit_result;}
static bool drop(Fighter_GObj* source){CHECK(source==object);invalidate();return drop_result;}
static void late(Fighter_GObj* source){CHECK(source==object);++late_calls;}
void ftCo_800B46B8(Fighter* source,cmd_t command,arg_t value){(void)source;CHECK(trace_count<256);trace[trace_count][0]=command;trace[trace_count][1]=value;trace[trace_count++][2]=0;}
void ftCo_800B463C(Fighter* source,cmd_t command){ftCo_800B46B8(source,command,0);}
void ftCo_800B4778(Fighter* source,cmd_t command,arg_t a,arg_t b){ftCo_800B46B8(source,command,a);trace[trace_count-1][2]=b;}
void ftCo_800B9704(Fighter* source){(void)source;++b9704_calls;}
float HSD_Randf(void){return 0.25f;}
static Fighter* ftCo_800A589C(Fighter* source){(void)source;return &partner;}
bool ftCo_800A0FB0(Vec3* pos,int* line,u32* flags,Vec3* normal,int a,int b,int c,float x,float y,float z,float w,float q){(void)a;(void)b;(void)c;(void)x;(void)y;(void)z;(void)w;(void)q;*pos=(Vec3){-11,12,0};*line=-1;*flags=0;*normal=(Vec3){0,1,0};if(retire_history_at_floor)++partner.x8_spawnNum;return floor_hit;}
bool ftCo_800A28D0(Fighter* source,float value){(void)source;(void)value;return false;}
bool ftCo_800A1CA8(Fighter* source){(void)source;return false;}
bool ftCo_800A2BD4(Fighter* source){(void)source;return false;}
static bool inlineC0(Fighter* source){(void)source;return false;}
static void ftCo_800A0148(Fighter* source){ftCo_800B463C(source,55);}
static void ftCo_800A0384(Fighter* source){ftCo_800B463C(source,56);}
u32 mpLineGetFlags(int line){(void)line;return 0;}
mp_UnkStruct0* mpIsland_8005AB54(int index){(void)index;return NULL;}
void mpIsland_8005ACE8(mp_UnkStruct0* island,Vec3* a,Vec3* b){(void)island;*a=(Vec3){-100,0,0};*b=(Vec3){100,0,0};}
#include "bodies.h"
static void reset(FighterKind kind,int level){
 ++cases;protect(PAGE_READWRITE);memset(pages,0xA5,65536);object=pages;fp=(Fighter*)((char*)pages+4096);object->user_data=fp;fp->gobj=object;fp->kind=kind;fp->ft_data=&original_data;fp->x8_spawnNum=generation=0x80000001;fp->cur_pos=(Vec3){3,4,5};fp->facing_dir=1;fp->co_attrs.gravity=0.08f;fp->co_attrs.jump_v_initial_velocity=2;fp->co_attrs.air_jump_v_multiplier=1.5f;fp->cpu.level=level;fp->motion_id=ftCo_MS_Wait;fp->ground_or_air=GA_Ground;fp->coll_data.floor.index=-1;fp->cpu.xF8_b0=0;fp->cpu.x54=(Vec2){100,4};fp->cpu.x38=5;fp->cpu.x3C=40;
 memset(&definition,0,sizeof(definition));active=NULL;live=true;trace_count=hook_calls=b9704_calls=late_calls=0;retirement=retire_hook=0;hint=history_result=partner.x8_spawnNum;floor_hit=false;retire_history_at_floor=false;step_result=64;limit_result=127;drop_result=false;
}
static void compare_cpu(const struct CpuFighter* stock,const struct CpuFighter* current){struct CpuFighter a=*stock,b=*current;a.write_pos=b.write_pos=NULL;a.x444=b.x444=NULL;a.x448=b.x448=NULL;CHECK(!memcmp(&a,&b,sizeof(a)));}
#ifdef EARLY_CPU_MODULE
static const NativeFighterDefinition* generated_policy;
static unsigned generated_offset(const char* name){for(unsigned i=0;i<generated_policy->state_field_count;++i)if(!strcmp(generated_policy->state_fields[i].name,name))return generated_policy->state_fields[i].offset;CHECK(false);return 0;}
static void generated_int(const char* name,int32_t value){memcpy(fp->native_module_state+generated_offset(name),&value,4);}
static void generated_flag(const char* name,bool value){fp->native_module_state[generated_offset(name)]=value;}
static void generated_float(const char* name,float value){memcpy(fp->native_module_state+generated_offset(name),&value,4);}
static int32_t generated_read(const char* name){int32_t value;memcpy(&value,fp->native_module_state+generated_offset(name),4);return value;}
static void generated_landing(HSD_GObj* source){CHECK(source==object);invalidate();}
static void configure_generated(void){active=generated_policy;memset(fp->native_module_state,0,sizeof(fp->native_module_state));generated_int("mode",6);generated_float("distance",15);generated_flag("combat",true);generated_int("step",64);generated_int("limit",127);}
static void generated_cases(void)
{
 MotionState motion={0};NativeFighterDefinition base={.fighter_root="ftDataTest",.motion_state_first=341,.motion_state_count=1,.motion_states=&motion};NativeFighterHost host={.callback_source_alive=nativeCallbackSource_Alive,.enter_landing_basic=generated_landing};generated_policy=FreakMelee_GetFighterModule(&base,&host);CHECK(generated_policy);
 // Actual generated early policy reproduces healthy original Nana initialization,
 // including the already-supported late spacing policy, all thirty records and RNG.
 for(unsigned floor=0;floor<2;++floor)for(int level=0;level<=9;++level){reset(Ft_Kind_Nana,level);floor_hit=floor;stock_ftCo_800A101C(fp,3,level,7);struct CpuFighter saved=fp->cpu;reset(Ft_Kind_Mario,level);floor_hit=floor;configure_generated();current_ftCo_800A101C(fp,3,level,7);compare_cpu(&saved,&fp->cpu);CHECK((uint32_t)generated_read("hint")==partner.x8_spawnNum&&generated_read("after")==1);}
 const int values[]={-1,0,64,127,128};
 for(unsigned a=0;a<5;++a)for(unsigned b=0;b<5;++b){reset(Ft_Kind_Mario,4);configure_generated();generated_int("step",values[a]);generated_int("limit",values[b]);int step=-11,limit=-22;current_ftCo_800AA320(fp,&step,&limit);CHECK(step==(values[a]>=0&&values[a]<=127?values[a]:2));CHECK(limit==(values[b]>=0&&values[b]<=127?values[b]:87));CHECK(generated_read("after")==2);}
 for(unsigned handled=0;handled<2;++handled){reset(Ft_Kind_Mario,4);configure_generated();generated_flag("handled",handled);current_ftCo_800A08F0(fp);CHECK(trace_count==(handled?2:9)&&generated_read("after")==1);}
 for(unsigned r=1;r<=6;++r){
  reset(Ft_Kind_Mario,4);configure_generated();generated_flag("invalidate",true);retirement=r;retire_hook=1;current_ftCo_800A101C(fp,3,4,7);protect(PAGE_READWRITE);CHECK(b9704_calls==0&&hook_calls==1);
  reset(Ft_Kind_Mario,4);configure_generated();generated_flag("invalidate",true);retirement=r;retire_hook=1;int step=-11,limit=-22;current_ftCo_800AA320(fp,&step,&limit);protect(PAGE_READWRITE);CHECK(step==-11&&limit==-22&&hook_calls==1);
 }
}
#endif
int main(void){
 pages=VirtualAlloc(NULL,65536,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);CHECK(pages);partner.x8_spawnNum=0x80000007;partner.cur_pos=(Vec3){31,32,33};partner.facing_dir=-1;partner_object.user_data=&partner;common.horizontal_stick_deadzone=0.3f;p_ftCommonData=&common;
 // Healthy no-hook full-body equivalence, including retained history bytes.
 const FighterKind kinds[]={Ft_Kind_Mario,Ft_Kind_Nana,Ft_Kind_Donkey,Ft_Kind_Koopa,Ft_Kind_GKoops};
 for(unsigned k=0;k<5;++k)for(int level=-1;level<=10;++level)for(unsigned floor=0;floor<2;++floor)for(int mode=0;mode<30;++mode){reset(kinds[k],level);floor_hit=floor;stock_ftCo_800A101C(fp,mode,level,7);struct CpuFighter saved=fp->cpu;reset(kinds[k],level);floor_hit=floor;current_ftCo_800A101C(fp,mode,level,7);compare_cpu(&saved,&fp->cpu);CHECK(b9704_calls==1);}
 for(unsigned k=0;k<2;++k)for(int level=-1;level<=10;++level){reset(kinds[k],level);int a=-11,b=-22;stock_ftCo_800AA320(fp,&a,&b);int c=-11,d=-22;current_ftCo_800AA320(fp,&c,&d);CHECK(a==c&&b==d);for(unsigned squat=0;squat<2;++squat){fp->motion_id=squat?ftCo_MS_Squat:ftCo_MS_Wait;trace_count=0;stock_ftCo_800A08F0(fp);int saved[256][3];unsigned n=trace_count;memcpy(saved,trace,sizeof(saved));trace_count=0;current_ftCo_800A08F0(fp);CHECK(n==trace_count&&!memcmp(saved,trace,n*sizeof(trace[0])));}}
 // Five whole travel callers: healthy native defaults match exact command order.
 void(*stock_callers[])(Fighter*)={stock_ftCo_800AA42C,stock_ftCo_800AA844,stock_ftCo_800AABC8,stock_ftCo_800AACD0,stock_ftCo_800AD54C};
 void(*current_callers[])(Fighter*)={current_ftCo_800AA42C,current_ftCo_800AA844,current_ftCo_800AABC8,current_ftCo_800AACD0,current_ftCo_800AD54C};
 const int motions[]={ftCo_MS_Wait,ftCo_MS_Dash,ftCo_MS_Ottotto};
 for(unsigned caller=0;caller<5;++caller)for(unsigned k=0;k<2;++k)for(int level=0;level<10;++level)for(unsigned air=0;air<2;++air)for(unsigned motion=0;motion<3;++motion){reset(kinds[k],level);fp->ground_or_air=air?GA_Air:GA_Ground;fp->motion_id=motions[motion];stock_callers[caller](fp);int commands[256][3];unsigned count=trace_count;memcpy(commands,trace,sizeof(commands));reset(kinds[k],level);fp->ground_or_air=air?GA_Air:GA_Ground;fp->motion_id=motions[motion];current_callers[caller](fp);CHECK(count==trace_count&&!memcmp(commands,trace,count*sizeof(trace[0])));}
 // Hook ordering/default staging and retained bytes; high-bit source survives signed return.
 for(unsigned valid=0;valid<2;++valid){reset(Ft_Kind_Mario,4);active=&definition;definition.on_cpu_prepare=prepare;definition.on_cpu_initialize=late;history_result=valid?partner.x8_spawnNum:0xffffffff;current_ftCo_800A101C(fp,3,4,7);CHECK(hook_calls==1&&late_calls==1&&b9704_calls==1&&fp->cpu.xC==6&&fp->cpu.x3C==15&&!fp->cpu.xF9_b2);CHECK(fp->cpu.x444==&fp->cpu.xFC[5]&&fp->cpu.x448==fp->cpu.xFC);for(unsigned i=0;i<30;++i){CHECK(fp->cpu.xFC[i].cstick.x==(signed char)0xA5&&fp->cpu.xFC[i].cstick.y==(signed char)0xA5&&fp->cpu.xFC[i].xA==0xA5&&fp->cpu.xFC[i].xB==0xA5);CHECK(fp->cpu.xFC[i].cur_pos.x==(valid?31:0)&&fp->cpu.xFC[i].facing_dir==(valid?-1:1));}}
 // A history source retired after prepare but before copy resolves to neutral.
 reset(Ft_Kind_Mario,4);active=&definition;definition.on_cpu_prepare=prepare;retire_history_at_floor=true;uint32_t previous_partner=partner.x8_spawnNum;current_ftCo_800A101C(fp,3,4,7);for(unsigned i=0;i<30;++i)CHECK(fp->cpu.xFC[i].cur_pos.x==0&&fp->cpu.xFC[i].facing_dir==1);partner.x8_spawnNum=previous_partner;
 const int values[]={INT32_MIN,-1,0,1,64,127,128,INT32_MAX};
 for(unsigned a=0;a<8;++a)for(unsigned b=0;b<8;++b)for(int level=-1;level<=10;++level){reset(Ft_Kind_Mario,level);active=&definition;definition.on_cpu_stick_step=step;definition.on_cpu_stick_limit=limit;step_result=values[a];limit_result=values[b];int old_step=-11,old_limit=-22;stock_ftCo_800AA320(fp,&old_step,&old_limit);int actual_step=-11,actual_limit=-22;current_ftCo_800AA320(fp,&actual_step,&actual_limit);CHECK(hook_calls==2);CHECK(actual_step==(step_result>=0&&step_result<=127?step_result:old_step));CHECK(actual_limit==(limit_result>=0&&limit_result<=127?limit_result:old_limit));}
 for(unsigned r=1;r<=6;++r){
  reset(Ft_Kind_Mario,4);active=&definition;definition.on_cpu_prepare=prepare;retirement=r;retire_hook=1;current_ftCo_800A101C(fp,3,4,7);protect(PAGE_READWRITE);CHECK(hook_calls==1&&b9704_calls==0);
  for(unsigned result=0;result<2;++result){reset(Ft_Kind_Mario,4);active=&definition;definition.on_cpu_drop_command=drop;retirement=r;retire_hook=1;drop_result=result;current_ftCo_800A08F0(fp);protect(PAGE_READWRITE);CHECK(hook_calls==1&&trace_count==2);}
  for(unsigned when=1;when<=2;++when){reset(Ft_Kind_Mario,4);active=&definition;definition.on_cpu_stick_step=step;definition.on_cpu_stick_limit=limit;retirement=r;retire_hook=when;int a=-11,b=-22;current_ftCo_800AA320(fp,&a,&b);protect(PAGE_READWRITE);CHECK(hook_calls==when&&a==-11&&b==-22);}
  void(*callers[])(Fighter*)={current_ftCo_800AA42C,current_ftCo_800AA844,current_ftCo_800AABC8,current_ftCo_800AACD0,current_ftCo_800AD54C};
  for(unsigned i=0;i<5;++i){reset(Ft_Kind_Mario,4);active=&definition;definition.on_cpu_stick_step=step;retirement=r;retire_hook=1;callers[i](fp);protect(PAGE_READWRITE);CHECK(hook_calls==1&&trace_count==0);}
 }
 // Squat priority never reaches drop hook; false/true result ordinary continuation.
 for(unsigned handled=0;handled<2;++handled){reset(Ft_Kind_Mario,4);active=&definition;definition.on_cpu_drop_command=drop;drop_result=handled;current_ftCo_800A08F0(fp);CHECK(hook_calls==1&&trace_count==(handled?2:9));reset(Ft_Kind_Mario,4);active=&definition;definition.on_cpu_drop_command=drop;fp->motion_id=ftCo_MS_Squat;current_ftCo_800A08F0(fp);CHECK(hook_calls==0&&trace_count==4);}
 #ifdef EARLY_CPU_MODULE
 generated_cases();
 #endif
 printf("PASS %u early CPU source cases / %u checks\n",cases,checks);return 0;
}
