#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <melee/ft/types.h>
#include <melee/ft/inlines.h>
#include <melee/ft/ftcmdscript.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/ft/kinds/ftPopo/forward.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/mp/mplib.h>
#include <melee/mp/mpisland.h>
#include <melee/mp/types.h>
#include <sysdolphin/baselib/gobj.h>
__declspec(dllimport) void* __stdcall VirtualAlloc(void*,size_t,unsigned long,unsigned long);
__declspec(dllimport) int __stdcall VirtualProtect(void*,size_t,unsigned long,unsigned long*);
static void* pages[2];static Fighter* actor[2];static HSD_GObj* objects[2];static u32 generations[2];static bool living[2],handled;static NativeFighterDefinition definitions[2];static unsigned selected,mutation,mutate_callback,callback_count,checks;static int trace[100],count;
#define CHECK(x) do{++checks;assert(x);}while(0)
static void protect(unsigned who,unsigned mode){unsigned long old;CHECK(VirtualProtect(pages[who],65536,mode,&old));}
static void record(int id){CHECK(count<100);trace[count++]=id;}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind){return &definitions[selected];}
bool nativeCallbackSource_Snapshot(HSD_GObj* source,uint32_t type,uint32_t* id){CHECK(type==NativeCallbackSource_Fighter);for(unsigned i=0;i<2;++i)if(source==objects[i]&&living[i]){*id=generations[i];return true;}return false;}
bool nativeCallbackSource_Alive(HSD_GObj* source,uint32_t id,uint32_t type){CHECK(type==NativeCallbackSource_Fighter);for(unsigned i=0;i<2;++i)if(source==objects[i])return living[i]&&actor[i]->x8_spawnNum==id;return false;}
static bool callback(HSD_GObj* source){CHECK(source==objects[0]);if(++callback_count==mutate_callback)switch(mutation){case 1:living[0]=false;protect(0,1);break;case 2:++actor[0]->x8_spawnNum;break;case 3:actor[0]->kind=Ft_Kind_Fox;break;case 4:actor[0]->ft_data=(void*)2;break;case 5:objects[0]->user_data=(void*)3;break;case 6:selected=1;break;case 7:living[1]=false;protect(1,1);break;case 8:++actor[1]->x8_spawnNum;break;case 9:actor[0]->victim_gobj=NULL;break;case 10:objects[1]->user_data=NULL;break;}return handled;}
#define ACTION(name,id) static void name(Fighter* f){record(id);}
ACTION(ftCo_800B04DC,2) ACTION(ftCo_800B101C,6) ACTION(ftCo_800B24B8,4) ACTION(ftCo_800AF290,7) ACTION(ftCo_800AECF0,8) ACTION(ftCo_800B00F8,9) ACTION(ftCo_800AFC40,10) ACTION(ftCo_800B126C,13) ACTION(ftCo_800B1478,14) ACTION(ftCo_800B17D0,17) ACTION(ftCo_800AF78C,18) ACTION(ftCo_800B1AB8,29) ACTION(ftCo_800B1DA0_noinline2,24) ACTION(ftCo_800B1EF0,27) ACTION(ftCo_800B21C8,28) ACTION(ftCo_800A0C8C,100) ACTION(ftCo_CpuClearTargetAndFinish,101) ACTION(ftCo_CpuSetNeutralStick,102)
static void ftCo_800AFE3C(Fighter* f,int value){record(30+value);}
static void ftCo_800A75DC(Fighter* f,Fighter* target){record(40);}
static void ftCo_CpuSetDestination(Fighter* f,float x,float y,float spacing){record(45);f->cpu.x54=(Vec2){x,y};}
static bool floor_found;
static bool ftCo_CpuPlanAttack(Fighter* f){record(41);return true;}
static void ftCo_CpuClearTargetModes(struct CpuFighter* d){d->xF9_b2=d->xF9_b3=d->xF9_b4=d->xF9_b5=d->xF9_b6=d->xF9_b7=false;}
static void ftCo_CpuInitEnemyTarget(Fighter* f,struct CpuFighter* d){record(42);d->x44=actor[1];}
static bool ftCo_800B2AFC_IsIgnoredFloor(int line){return false;}
bool mpCheckFloor(float ax,float ay,float bx,float by,float yoff,Vec3* point,int* line,u32* flags,Vec3* normal,int skipline,int skipjoint,int onlyjoint,bool(*callback_fn)(HSD_GObj*,int),HSD_GObj* source){record(43);*point=(Vec3){7,8,9};*line=1;return floor_found;}
static Fighter* ftCo_800A589C(Fighter* f){return actor[1];}
static void ftPp_SpecialLw_Enter(HSD_GObj* source){record(44);}
#include "hook_bodies.h"
static bool ftCo_IsGrabbing(Fighter* f){return true;}
float HSD_Randf(void){record(105);return .25f;}
void ftCo_800B463C(Fighter* f,cmd_t cmd){record(200+cmd);}
void ftCo_800B46B8(Fighter* f,cmd_t cmd,arg_t arg){record(300+cmd);record(arg);}
static void ftCo_800B4880(Fighter* f,int script){record(400+script);}
static void ftCo_CpuPressAWithLstickX(Fighter* f,int value){record(110);}
static void ftCo_CpuFlickLstickX(Fighter* f,int first,int second){record(111);}
mp_UnkStruct0* mpIsland_8005AC14(Vec3* position,float offset){record(112);return NULL;}
void mpIsland_8005ACE8(mp_UnkStruct0* island,Vec3* left,Vec3* right){CHECK(false);}
#include "capture_body.h"
static void reset(void){for(unsigned i=0;i<2;++i){protect(i,4);memset(pages[i],0,65536);objects[i]=pages[i];actor[i]=(Fighter*)((char*)pages[i]+4096);objects[i]->user_data=actor[i];actor[i]->gobj=objects[i];actor[i]->x8_spawnNum=generations[i]=0x80000001+i;actor[i]->kind=Ft_Kind_Mario;actor[i]->ft_data=(void*)1;living[i]=true;}actor[0]->victim_gobj=objects[1];actor[0]->cpu.x448=actor[0]->cpu.xFC;actor[0]->cpu.x444=actor[0]->cpu.xFC+5;actor[0]->cpu.xFC[0].lstick=(S8Vec2){-128,127};actor[0]->cpu.xFC[0].cstick=(S8Vec2){13,14};actor[0]->cpu.xFC[0].x0=0xffffffff;actor[0]->cpu.xFC[0].cur_pos=(Vec3){10,20,30};actor[0]->cpu.xFC[0].facing_dir=-1;selected=mutation=mutate_callback=callback_count=count=0;handled=false;memset(definitions,0,sizeof(definitions));}
int main(void)
{
 for(unsigned i=0;i<2;++i){pages[i]=VirtualAlloc(NULL,65536,0x3000,4);CHECK(pages[i]);}
 for(unsigned floor=0;floor<2;++floor)for(int mode=-1;mode<33;++mode)for(int behavior=-1;behavior<20;++behavior){reset();floor_found=floor;actor[0]->cpu.xC=mode;actor[0]->cpu.x18=behavior;Fighter initial=*actor[0];stock_ftCo_800B2AFC(actor[0]);struct CpuFighter expected=actor[0]->cpu;int expected_count=count,expected_trace[100];memcpy(expected_trace,trace,sizeof(trace));*actor[0]=initial;count=0;current_ftCo_800B2AFC(actor[0]);CHECK(!memcmp(&expected,&actor[0]->cpu,sizeof(expected))&&count==expected_count&&!memcmp(trace,expected_trace,count*sizeof(int)));*actor[0]=initial;count=callback_count=0;definitions[0].on_cpu_mode_update=callback;current_ftCo_800B2AFC(actor[0]);CHECK(callback_count==1&&!memcmp(&expected,&actor[0]->cpu,sizeof(expected))&&count==expected_count&&!memcmp(trace,expected_trace,count*sizeof(int)));}
 for(unsigned sync=0;sync<2;++sync)for(unsigned copy=0;copy<2;++copy)for(int motion=0;motion<370;++motion){reset();actor[0]->motion_id=motion;actor[0]->x2225_b3=sync;actor[0]->cpu.xFA_b7=copy;actor[0]->cur_pos=(Vec3){1,2,3};Fighter initial=*actor[0];stock_ftCo_800B0AF4(actor[0]);Fighter expected=*actor[0];int expected_count=count,expected_trace[100];memcpy(expected_trace,trace,sizeof(trace));*actor[0]=initial;count=0;current_ftCo_800B0AF4(actor[0]);CHECK(!memcmp(&expected,actor[0],sizeof(expected))&&count==expected_count&&!memcmp(trace,expected_trace,count*sizeof(int)));*actor[0]=initial;count=callback_count=0;definitions[0].on_cpu_output=callback;current_ftCo_800B0AF4(actor[0]);CHECK(callback_count==1&&!memcmp(&expected,actor[0],sizeof(expected))&&count==expected_count&&!memcmp(trace,expected_trace,count*sizeof(int)));}
 for(unsigned hook=0;hook<2;++hook)for(unsigned result=0;result<2;++result)for(unsigned mode=1;mode<=6;++mode){reset();mutation=mode;mutate_callback=1;handled=result;if(hook)definitions[0].on_cpu_output=callback;else definitions[0].on_cpu_mode_update=callback;actor[0]->cpu.xC=6;actor[0]->cpu.xFA_b7=true;if(hook)current_ftCo_800B0AF4(actor[0]);else current_ftCo_800B2AFC(actor[0]);protect(0,4);CHECK(callback_count==1&&count==0);}
 for(unsigned hook=1;hook<=2;++hook)for(unsigned result=0;result<2;++result)for(unsigned mode=1;mode<=10;++mode){reset();definitions[0].on_cpu_capture_decision=callback;definitions[0].on_cpu_pummel_special=callback;mutation=mode;mutate_callback=hook;handled=result;ftCo_800B683C(actor[0]);protect(0,4);protect(1,4);CHECK(callback_count==(hook==2&&result?1:hook));CHECK(count==0&&actor[0]->cpu.x94==0);}
 reset();definitions[0].on_cpu_capture_decision=callback;definitions[0].on_cpu_pummel_special=callback;ftCo_800B683C(actor[0]);CHECK(callback_count==2&&actor[0]->cpu.x94==1&&count==2&&trace[0]==105&&trace[1]==402);
 printf("PASS %u actual full mode/output/capture checks; stage/combat handlers and RNG controlled\n",checks);return 0;
}
