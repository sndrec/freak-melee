/* Complete original callbacks vs generated scalar slice. Shared animation,
 * motion and stage policy are controlled; falling/acceleration bodies are real. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <melee/ef/forward.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcliffcommon.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/inlines.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallSpecial.h>
#include <melee/ft/kinds/ftCommon/ftCo_Landing.h>
#include <melee/ft/kinds/ftCommon/inlines.h>
#include <melee/ft/kinds/ftPopo/ftpopo.h>
#include <melee/ft/kinds/ftPopo/ftpopospecials.h>
#include <melee/ft/kinds/ftPopo/ftpopospecialhi.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_module.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>

static unsigned checks,pairs,safety;
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"pair%u safety%u line%d %s\n",pairs,safety,__LINE__,#x);exit(1);}}while(0)
static Fighter fighter,expected;
static HSD_GObj source,*heads[32];
HSD_GObj** HSD_GObjPLinkHead=heads;
static ftIceClimberAttributes attributes;
static const NativeFighterDefinition *definition;
static bool remaining,ground,landed,cliff,authored;
static unsigned invalidation;
typedef struct Trace {int op,a,b;float x,y,z;}Trace;
static Trace trace[32],expected_trace[32];
static unsigned trace_count,expected_count;
enum {Change,GroundPhysics,GroundProbe,AirProbe,Cliff,ToAir,ToGround,Freefall,Landing};
static void event(int op,int a,int b,float x,float y,float z){CHECK(trace_count<32);trace[trace_count++]=(Trace){op,a,b,x,y,z};}
static void unexpected(void){CHECK(!"unexpected Host boundary");}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind){return kind==Ft_Kind_CustomFirst?definition:NULL;}
bool nativeModelEffect_Snapshot(HSD_GObj*g,uint32_t*n){(void)g;*n=0;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj*g,uint32_t n){(void)g;(void)n;return NULL;}
void __assert(char*f,u32 line,char*m){fprintf(stderr,"%s:%lu %s\n",f,line,m);exit(1);}
static void invalidate(unsigned point){if(invalidation==point){source.user_data=NULL;heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;}else if(invalidation==point+10)fighter.x8_spawnNum++;}
void Fighter_ChangeMotionState(Fighter_GObj*g,FtMotionId id,MotionFlags flags,float frame,float rate,float blend,Fighter_GObj*donor){
 CHECK(g==&source&&donor==NULL);event(Change,id,(int)flags,frame,rate,blend);fighter.motion_id=id;fighter.cur_anim_frame=frame;invalidate(1);
}
bool ftAnim_IsFramesRemaining(Fighter_GObj*g){CHECK(g==&source);return remaining;}
void ft_8008A2BC(Fighter_GObj*g){CHECK(g==&source);event(Change,14,0,0,0,0);fighter.motion_id=14;}
void ft_80084F3C(Fighter_GObj*g){CHECK(g==&source);event(GroundPhysics,0,0,0,0,0);fighter.xE4_ground_accel_1=-.125f;}
bool ft_800827A0(Fighter_GObj*g){CHECK(g==&source);event(GroundProbe,0,0,0,0,0);return ground;}
bool ft_CheckGroundAndLedge(Fighter_GObj*g,int direction){CHECK(g==&source);event(AirProbe,direction,0,0,0,0);return landed;}
bool ftCliffCommon_80081298(Fighter_GObj*g){CHECK(g==&source);event(Cliff,0,0,0,0,0);return cliff;}
void ftCommon_8007D60C(Fighter*f){CHECK(f==&fighter);event(ToAir,0,0,0,0,0);f->ground_or_air=GA_Air;f->x1968_jumpsUsed=f->co_attrs.max_jumps;invalidate(2);}
void ftCommon_8007D7FC(Fighter*f){CHECK(f==&fighter);event(ToGround,0,0,0,0,0);f->ground_or_air=GA_Ground;invalidate(3);}
void ftCo_80096900(Fighter_GObj*g,int a,int b,bool allowed,float mobility,float lag){CHECK(g==&source);event(Freefall,a,b,allowed,mobility,lag);fighter.motion_id=35;}
void ftCo_LandingFallSpecial_Enter(Fighter_GObj*g,bool allowed,float lag){CHECK(g==&source);event(Landing,allowed,0,lag,0,0);fighter.motion_id=43;}
#include "actual.inc"
#include "generated.inc"
#include "parameters.inc"
static unsigned field(const char*n){for(unsigned i=0;i<definition->state_field_count;++i)if(!strcmp(definition->state_fields[i].name,n))return definition->state_fields[i].offset;CHECK(false);return 0;}
static void fi(const char*n,int32_t v){memcpy(fighter.native_module_state+field(n),&v,4);}
static void fl(const char*n,float v){memcpy(fighter.native_module_state+field(n),&v,4);}
static int32_t ri(const char*n){int32_t v;memcpy(&v,fighter.native_module_state+field(n),4);return v;}
static float rf(const char*n){float v;memcpy(&v,fighter.native_module_state+field(n),4);return v;}
static void initialize(bool scripted,int motion){
 authored=scripted;invalidation=0;memset(&fighter,0,sizeof(fighter));memset(&source,0,sizeof(source));memset(heads,0,sizeof(heads));
 memset(trace,0,sizeof(trace));trace_count=0;source.user_data=&fighter;heads[HSD_GOBJ_PLINK_FIGHTER]=&source;
 fighter.kind=scripted?Ft_Kind_CustomFirst:Ft_Kind_Popo;fighter.x8_spawnNum=UINT32_C(0xFEDCBA98);fighter.dat_attrs=&attributes;
 fighter.motion_id=motion;fighter.cur_anim_frame=8.5f;fighter.frame_speed_mul=.75f;fighter.facing_dir=1;fighter.ground_or_air=motion>=352;
 fighter.co_attrs.gravity=.12f;fighter.co_attrs.terminal_velocity=2.5f;fighter.co_attrs.aerial_friction=.125f;fighter.co_attrs.max_jumps=2;
 fighter.self_vel=(Vec3){.75f,-.25f,.875f};fighter.x74_self_accel=(Vec3){.5f,.6f,.7f};
 fighter.cur_pos=(Vec3){1.25f,-3.5f,7.75f};fighter.cmd_vars[0]=3;fighter.cmd_vars[1]=4;fighter.cmd_vars[2]=2;fighter.cmd_vars[3]=6;
 remaining=true;ground=true;landed=false;cliff=false;
}
static void save(void){expected=fighter;expected_count=trace_count;memcpy(expected_trace,trace,sizeof(trace));}
static void same_float(float a,float b){CHECK(memcmp(&a,&b,4)==0);}
static void compare(void){
 ++pairs;CHECK(trace_count==expected_count);CHECK(!memcmp(trace,expected_trace,trace_count*sizeof(*trace)));
 CHECK(fighter.motion_id==expected.motion_id);CHECK(fighter.ground_or_air==expected.ground_or_air);CHECK(fighter.x1968_jumpsUsed==expected.x1968_jumpsUsed);
 same_float(fighter.cur_anim_frame,expected.cur_anim_frame);same_float(fighter.frame_speed_mul,expected.frame_speed_mul);
 for(unsigned i=0;i<4;++i)CHECK(fighter.cmd_vars[i]==expected.cmd_vars[i]);
 for(unsigned i=0;i<3;++i){same_float((&fighter.self_vel.x)[i],(&expected.self_vel.x)[i]);same_float((&fighter.x74_self_accel.x)[i],(&expected.x74_self_accel.x)[i]);same_float((&fighter.cur_pos.x)[i],(&expected.cur_pos.x)[i]);}
 same_float(fighter.xE4_ground_accel_1,expected.xE4_ground_accel_1);same_float(fighter.facing_dir,expected.facing_dir);
 CHECK(fighter.x8_spawnNum==expected.x8_spawnNum);
}
static void setup_squall(float accel,int pending,int timer,int elapsed){
 if(authored){fl("squall_input_accel",accel);fi("squall_pending_pumps",pending);fi("squall_pump_timer",timer);fi("squall_elapsed",elapsed);}
 else{fighter.mv.pp.specials.x1C=accel;fighter.mv.pp.specials.x10=pending;fighter.mv.pp.specials.x14=timer;fighter.mv.pp.specials.x18=elapsed;}
}
static void compare_squall(void){compare();same_float(rf("squall_input_accel"),expected.mv.pp.specials.x1C);CHECK(ri("squall_pending_pumps")==expected.mv.pp.specials.x10);CHECK(ri("squall_pump_timer")==expected.mv.pp.specials.x14);CHECK(ri("squall_elapsed")==expected.mv.pp.specials.x18);}
static void squall_cases(void){
 HSD_GObjEvent iasa[]={ftPp_SpecialS1_IASA,ftPp_SpecialS2_IASA,ftPp_SpecialAirS1_IASA,ftPp_SpecialAirS2_IASA};
 float stick[]={-1,-attributes.x40,-.01f,0,.01f,attributes.x40,1};
 for(unsigned state=0;state<4;++state)for(unsigned s=0;s<7;++s){
  for(unsigned mode=0;mode<2;++mode){initialize(mode,343+state);fighter.input.lstick[0].x=stick[s];setup_squall(-7,3,4,5);if(mode)definition->motion_states[2+state].input_cb(&source);else iasa[state](&source);if(!mode)save();else compare_squall();}
 }
 float accel[]={-2,-.125f,0,.125f,2},vel[]={-4,-attributes.x3C,-.125f,0,.125f,attributes.x3C,4};
 for(unsigned state=0;state<2;++state)for(unsigned a=0;a<5;++a)for(unsigned v=0;v<7;++v)
 for(unsigned pending=0;pending<2;++pending)for(unsigned timer=0;timer<3;++timer)for(unsigned duration=0;duration<2;++duration)for(unsigned buttons=0;buttons<4;++buttons){
  for(unsigned mode=0;mode<2;++mode){initialize(mode,345+state);fighter.self_vel.x=vel[v];fighter.cmd_vars[0]=buttons&1;fighter.cmd_vars[2]=1;fighter.input.pressed_buttons=buttons&2?HSD_PAD_B:0;
   setup_squall(accel[a],pending,attributes.x68+(int)timer-1,(int)attributes.x5C+(int)duration-2);
   if(mode)definition->motion_states[4+state].phys_cb(&source);else if(state)ftPp_SpecialAirS2_Phys(&source);else ftPp_SpecialAirS1_Phys(&source);
   if(!mode)save();else compare_squall();}
 }
}
static void belay_cases(void){
 HSD_GObjEvent original[4][4]={
  {ftPp_SpecialHiStart_1_Anim,ftPp_SpecialHiStart_1_IASA,ftPp_SpecialHiStart_1_Phys,ftPp_SpecialHiStart_1_Coll},
  {ftPp_SpecialHiThrow_1_Anim,ftPp_SpecialHiThrow_1_IASA,ftPp_SpecialHiThrow_1_Phys,ftPp_SpecialHiThrow_1_Coll},
  {ftPp_SpecialAirHiStart_1_Anim,ftPp_SpecialAirHiStart_1_IASA,ftPp_SpecialAirHiStart_1_Phys,ftPp_SpecialAirHiStart_1_Coll},
  {ftPp_SpecialAirHiThrow_1_Anim,ftPp_SpecialAirHiThrow_1_IASA,ftPp_SpecialAirHiThrow_1_Phys,ftPp_SpecialAirHiThrow_1_Coll}};
 int motions[]={350,351,355,356};
 for(unsigned state=0;state<4;++state)for(unsigned phase=0;phase<4;++phase)for(unsigned bits=0;bits<64;++bits){
  for(unsigned mode=0;mode<2;++mode){initialize(mode,motions[state]);remaining=bits&1;ground=bits&2;landed=bits&4;cliff=bits&8;fighter.cmd_vars[2]=bits&16?7:0;fighter.self_vel.y=bits&32?.5f:-.5f;fighter.facing_dir=bits&16?-1:1;
   if(mode){const MotionState*m=&definition->motion_states[motions[state]-341];HSD_GObjEvent cb=phase==0?m->anim_cb:phase==1?m->input_cb:phase==2?m->phys_cb:m->coll_cb;CHECK(cb);cb(&source);}else original[state][phase](&source);
   if(!mode)save();else compare();}
 }
 /* Authored safety is distinct from healthy original source semantics. */
 for(unsigned recycle=0;recycle<2;++recycle){
  initialize(true,350);remaining=false;invalidation=1+10*recycle;
  definition->motion_states[9].anim_cb(&source);++safety;
  CHECK(trace_count==1&&trace[0].op==Change);
 }

}
int main(void){
 NativeFighterHost host={0};
#include "hosts.inc"
 host.callback_source_alive=nativeCallbackSource_Alive;host.change_motion_state=Fighter_ChangeMotionState;host.animation_frames_remaining=ftAnim_IsFramesRemaining;
 host.finish_ground_action=ft_8008A2BC;host.ground_friction=ft_80084F3C;host.special_ground_collision=ft_800827A0;host.check_ground_and_ledge=ft_CheckGroundAndLedge;
 host.check_cliff_catch=ftCliffCommon_80081298;host.force_air_and_use_jumps=ftCommon_8007D60C;host.air_to_ground=ftCommon_8007D7FC;
 host.enter_freefall=ftCo_80096900;host.enter_landing_fall_special=ftCo_LandingFallSpecial_Enter;host.fall=ftCommon_Fall;host.fall_basic=ftCommon_FallBasic;host.air_decel_normal=ftCommon_CalcSelfAccel_DeaccelAir;host.abs_float=fabsf;
 CHECK(FreakMelee_SetFighterParameters(parameter_bits,sizeof(parameter_bits)/sizeof(*parameter_bits),parameter_schema));
 static MotionState motions[26];NativeFighterDefinition base={.fighter_root="ftDataPopo",.source_kind=Ft_Kind_Popo,.motion_state_first=341,.motion_state_count=26,.motion_states=motions};
 definition=FreakMelee_GetFighterModule(&base,&host);CHECK(definition);memset(&attributes,0,sizeof(attributes));
#include "attributes.inc"
 squall_cases();belay_cases();printf("PASS %u original/generated pairs, %u checks, %u authored safety cases\n",pairs,checks,safety);return 0;
}
