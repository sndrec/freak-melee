// Eight complete frozen production bodies vs the actual full generated DLL.
// Motion transition, animation metadata, statistics and other fighters are sinks.
#include <windows.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_visuals.h>
#include <melee/ft/kinds/ftPeach/forward.h>
#include <melee/ft/kinds/ftPeach/ftpeach.h>
#include <melee/ft/kinds/ftCommon/ftCo_AppealS.h>
#include <melee/ft/kinds/ftCLink/ftclink.h>
#include <melee/ft/kinds/ftDrMario/ftdrmario.h>
#include <melee/ft/kinds/ftZelda/ftzelda.h>
#include <melee/ft/kinds/ftKirby/ftkirby.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/fighter.h>
#include <melee/ft/inlines.h>
#include <melee/ft/ftparts.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lb_00F9.h>
#include <melee/pl/plbonuslib.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks, pairs, guarded_safety, generated_safety;
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"pair%u line%d %s\n",pairs,__LINE__,#x);exit(1); } } while(0)
static void unexpected(void) { CHECK(false); }
static void stock_slot(HSD_GObj* g) { (void)g;unexpected(); }
#include "base.h"
#include "parameters.h"
static HMODULE library;
static const NativeFighterDefinition* definition;
static bool authored, live=true;
static HSD_GObj source, *heads[32];
HSD_GObj** HSD_GObjPLinkHead=heads;
static Fighter* fighter;
static MotionState common[341];
static struct Fighter_WaitAnimData animation;
DbLKind DbLevel;
static unsigned mutation, event_count;
typedef struct TauntTraceEvent { int operation, a, b, previous_group; } TauntTraceEvent;
static TauntTraceEvent events[8];
static void record(int operation,int a,int b)
{ CHECK(event_count<8);events[event_count++]=(TauntTraceEvent){operation,a,b,fighter->x5F4_arr[2].prev}; }
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ return authored&&kind==Ft_Kind_Peach?definition:NULL; }
struct Fighter_WaitAnimData* ftData_80085FD4(Fighter* f,FtMotionId id)
{ CHECK(f==fighter&&id==201);record(1,id,animation.x8);return &animation; }
void Fighter_ChangeMotionState(Fighter_GObj* g,FtMotionId motion,MotionFlags flags,
                               float frame,float rate,float blend,Fighter_GObj* other)
{
 CHECK(g==&source&&flags==0&&frame==0&&rate==1&&blend==0&&!other);
 record(2,motion,0);fighter->motion_id=motion;
 if(mutation==1) {
  heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;live=false;
  DWORD old;CHECK(VirtualProtect(fighter,sizeof(*fighter),PAGE_NOACCESS,&old));
 } else if(mutation==2) {
  fighter->x8_spawnNum++;
 } else if(mutation==3) {
  fighter->kind=Ft_Kind_Falco;
 } else if(mutation==4) {
  authored=false;
 }
}
void pl_80040120(int player,int flag) { CHECK(live);record(3,player,flag); }
void ftCl_Init_80149318(HSD_GObj* g) { (void)g;unexpected(); }
void ftDr_Init_80149910(HSD_GObj* g) { (void)g;unexpected(); }
void ftZd_Init_801395C8(HSD_GObj* g) { (void)g;unexpected(); }
void ftKb_SpecialN_800F5D04(HSD_GObj* g,bool flag) { (void)g;(void)flag;unexpected(); }
void lb_8000B1CC(HSD_JObj* j,Vec3* a,Vec3* b) { (void)j;(void)a;(void)b;unexpected(); }
void lb_800119DC(Vec3* p,int n,float a,float b,float c) { (void)p;(void)n;(void)a;(void)b;(void)c;unexpected(); }
#include "taunt_bodies.h"
static void reset(int level,int previous,int current,float facing,int animated_left,unsigned buttons)
{
 if(!live) { DWORD old;CHECK(VirtualProtect(fighter,sizeof(*fighter),PAGE_READWRITE,&old)); }
 memset(fighter,0,sizeof *fighter);memset(&source,0,sizeof source);live=true;
 source.user_data=fighter;fighter->gobj=&source;fighter->kind=Ft_Kind_Peach;fighter->behavior_kind=Ft_Kind_Peach;
 fighter->x8_spawnNum=0x81234567;heads[HSD_GOBJ_PLINK_FIGHTER]=&source;
 fighter->x18=341;fighter->x1C_actionStateList=common;fighter->x20_actionStateList=common;
 fighter->facing_dir=facing;fighter->allow_interrupt=true;fighter->motion_id=ftCo_MS_Wait;
 fighter->x5AC.model_num=7;fighter->x5F4_arr[2].prev=previous;fighter->x5F4_arr[2].idx=current;
 fighter->player_id=2;fighter->x221F_b4=1;fighter->input.pressed_buttons=buttons;
 animation.x8=animated_left;DbLevel=(DbLKind)level;mutation=event_count=0;
}
typedef struct Projection { int motion,previous,current;bool allowed,dirty; } Projection;
static Projection projection(void)
{ return (Projection){fighter->motion_id,fighter->x5F4_arr[2].prev,fighter->x5F4_arr[2].idx,fighter->allow_interrupt,fighter->x221D_b2}; }
static void healthy(void)
{
 const int levels[]={-1,0,1,2,3,4,5,INT_MAX};
 const int groups[]={-1,0,1,4,-3};const int metadata[]={0,1,0x100};
 const unsigned buttons[]={0,HSD_PAD_DPADUP,HSD_PAD_A,HSD_PAD_B,HSD_PAD_DPADUP|HSD_PAD_A,HSD_PAD_DPADUP|HSD_PAD_B,HSD_PAD_DPADLEFT,0xffff};
 for(unsigned l=0;l<8;l++)for(unsigned p=0;p<5;p++)for(unsigned c=0;c<2;c++)for(int face=-1;face<=1;face+=2)
 for(unsigned m=0;m<3;m++)for(unsigned input=0;input<8;input++)for(unsigned route=0;route<2;route++) {
  int current=c?7:-1;reset(levels[l],groups[p],current,face,metadata[m],buttons[input]);authored=false;
  bool before=true;if(route)before=ftCo_800DE9D8(&source);else ftCo_800DEA28(&source);
  Projection native=projection();TauntTraceEvent expected[8];memcpy(expected,events,sizeof events);unsigned count=event_count;
  reset(levels[l],groups[p],current,face,metadata[m],buttons[input]);authored=true;fighter->behavior_kind=Ft_Kind_None;
  bool after=true;if(route)after=ftCo_800DE9D8(&source);else ftCo_800DEA28(&source);
  Projection script=projection();CHECK(before==after);
  CHECK(native.motion==script.motion&&native.previous==script.previous&&native.current==script.current);
  CHECK(native.allowed==script.allowed&&native.dirty==script.dirty);
  CHECK(event_count==count&&memcmp(events,expected,count*sizeof(TauntTraceEvent))==0);
  if(!route||(buttons[input]&HSD_PAD_DPADUP)) {
   CHECK(script.motion==((face==-1&&metadata[m])?265:264));
   CHECK(!script.allowed&&script.current==current);
   CHECK(script.previous==(levels[l]>=3?(groups[p]==-1?0:-1):groups[p]));
  } else { CHECK(event_count==0&&script.motion==ftCo_MS_Wait&&script.allowed); }
  ++pairs;
 }
}
static void safety(void)
{
 for(unsigned route=0;route<2;route++)for(unsigned mode=1;mode<=4;mode++) {
  reset(4,-1,7,-1,1,HSD_PAD_DPADUP);authored=true;mutation=mode;
  if(route==0) { definition->on_taunt(&source);++generated_safety; }
#ifdef HAVE_GUARDED_ENTRY
  else { guarded_taunt_entry(&source);++guarded_safety;CHECK(event_count==2); }
#else
  else continue;
#endif
  CHECK(event_count>=1);CHECK(events[event_count-1].operation==2);
  if(mode==1)CHECK(!live);else if(mode==2)CHECK(fighter->x8_spawnNum==0x81234568);
  else if(mode==3)CHECK(fighter->kind==Ft_Kind_Falco);else CHECK(!authored);
 }
}
int main(int argc,char** argv)
{
 _Static_assert(ftCo_MS_AppealSR==264&&ftCo_MS_AppealSL==265,"literal common taunt IDs");
 CHECK(argc==2);fighter=VirtualAlloc(NULL,sizeof *fighter,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);CHECK(fighter);
 library=LoadLibraryA(argv[1]);CHECK(library);NativeFighterHost host={0};
#include "required_host.h"
 host.common_motion_states=common;host.debug_level=&DbLevel;
 host.previous_model_group=nativeFighterVisuals_PreviousGroup;host.set_visibility_group=ftParts_80074A4C;
 host.enter_taunt=ftCo_800DEAE8;host.callback_source_alive=nativeCallbackSource_Alive;
 common[144]=common[146]=(MotionState){.anim_cb=stock_slot,.input_cb=stock_slot,.phys_cb=stock_slot,.coll_cb=stock_slot};
 common[265].anim_id=201;
 typedef int(*SetParameters)(const uint32_t*,unsigned,uint64_t);
 SetParameters set=(SetParameters)GetProcAddress(library,"FreakMelee_SetFighterParameters");CHECK(set);
 CHECK(set(parameter_bits,sizeof parameter_bits/sizeof parameter_bits[0],PARAMETER_SCHEMA));
 typedef const NativeFighterDefinition*(*Entry)(const NativeFighterDefinition*,const NativeFighterHost*);
 Entry open=(Entry)GetProcAddress(library,"FreakMelee_GetFighterModule");CHECK(open);
 definition=open(&ftPeachClone_Definition,&host);CHECK(definition&&definition->on_taunt);
 healthy();safety();if(!live) { DWORD old;CHECK(VirtualProtect(fighter,sizeof *fighter,PAGE_READWRITE,&old)); }
 CHECK(VirtualFree(fighter,0,MEM_RELEASE));FreeLibrary(library);
 printf("{\"checks\":%u,\"healthy_pairs\":%u,\"generated_safety\":%u,\"guarded_source_safety\":%u,\"failures\":0}\n",checks,pairs,generated_safety,guarded_safety);return 0;
}
