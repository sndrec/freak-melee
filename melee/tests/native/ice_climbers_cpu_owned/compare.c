#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <melee/ft/types.h>
#include <melee/ft/inlines.h>
#include <melee/it/types.h>
#include <melee/it/inlines.h>
#include <melee/ft/ftcmdscript.h>
#include <melee/ft/native_cpu_history.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/ft/kinds/ftPopo/forward.h>
#include <melee/mp/mplib.h>
#include <melee/mp/mpisland.h>
#include <melee/mp/types.h>
#include <melee/gr/types.h>
#if OWNED_ROUTE
#include "owned_1.inc"
#include "parameters_1.inc"
#else
#include "owned_0.inc"
#include "parameters_0.inc"
#endif
__declspec(dllimport) void* __stdcall VirtualAlloc(void*,size_t,unsigned long,unsigned long);
__declspec(dllimport) int __stdcall VirtualProtect(void*,size_t,unsigned long,unsigned long*);
static void* pages[2];
static Fighter* f[2]; static Fighter target[2]; static HSD_GObj object[2], target_object[2];
static uint32_t source_id[2]; static bool source_live[2], target_live;
static unsigned side, checks, pairs, retire_at, calls, retire_mode, safety;
static const NativeFighterDefinition* definition;
static int trace[2][128], trace_count[2];
static bool floor_found, blocked, excluded; static int island_mode;
static mp_UnkStruct0 island[2]; static HSD_GObj* heads[32];
HSD_GObj** HSD_GObjPLinkHead = heads; StageInfo stage_info;
static float random_value, max_error; static int selected_script[2];
static unsigned query_calls, eligibility_pairs, threshold_pairs;
static uint32_t ftCo_803C6594[40];
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"FAIL line %d side%u pair%u\n",__LINE__,side,pairs); abort(); } } while(0)
static void protect(unsigned which,unsigned mode){unsigned long prior;CHECK(VirtualProtect(pages[which],65536,mode,&prior));}
static void event(int id){CHECK(trace_count[side]<128);trace[side][trace_count[side]++]=id;}
static void boundary(void){if(side==1 && ++calls==retire_at){if(retire_mode==1){source_live[1]=false;protect(1,1);}else ++f[1]->x8_spawnNum;}}
bool nativeCallbackSource_Snapshot(HSD_GObj* g,uint32_t type,uint32_t* id){CHECK(type==NativeCallbackSource_Fighter);for(unsigned i=0;i<2;++i)if(g==&object[i]&&source_live[i]){*id=f[i]->x8_spawnNum;return true;}if(g==&target_object[side]&&target_live){*id=target[side].x8_spawnNum;return true;}return false;}
bool nativeCallbackSource_Alive(HSD_GObj* g,uint32_t id,uint32_t type){CHECK(type==NativeCallbackSource_Fighter);for(unsigned i=0;i<2;++i)if(g==&object[i])return source_live[i]&&(uint32_t)f[i]->x8_spawnNum==id;return target_live&&g==&target_object[side]&&(uint32_t)target[side].x8_spawnNum==id;}
HSD_GObj* ftLib_8008741C(u32 id){return target_live&&id==(u32)target[side].x8_spawnNum?&target_object[side]:NULL;}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind){return side?definition:NULL;}
#include "target_bodies.h"
static unsigned field(const char* name){for(unsigned i=0;i<definition->state_field_count;++i)if(!strcmp(name,definition->state_fields[i].name))return definition->state_fields[i].offset;CHECK(false);return 0;}
static void flag(const char* name,bool value){f[1]->native_module_state[field(name)]=value;}
static bool getflag(const char* name){return f[1]->native_module_state[field(name)]!=0;}
static void word(const char* name,int value){memcpy(f[1]->native_module_state+field(name),&value,4);}
static int getword(const char* name){int value;memcpy(&value,f[1]->native_module_state+field(name),4);return value;}
static Fighter* ftCo_800A589C(Fighter* fp){return target_live&&!target[side].x221F_b3?&target[side]:NULL;}
static Fighter* ftCo_800A4BEC(Fighter* fp){event(1);return &target[side];}
static void ftCo_CpuUpdateCommonItemTarget(Fighter* fp){event(2);}
static void ftCo_CpuUpdateSpecialItemTarget(Fighter* fp){event(3);}
static bool ftCo_800ADE48(Fighter* fp){event(4);return true;}
static void ftCo_800AEFB8(Fighter* fp){event(5);}
static void ftCo_800A8DE4(Fighter* fp){event(6);}
static void ftCo_800A1CC4(Fighter* fp,int policy){event(7);}
static void ftCo_800A1F3C(Fighter*,float,float,float);
static bool ftCo_CpuShouldAct(Fighter*);
static bool ftCo_CpuDataShouldAct(struct CpuFighter*);
static void ftCo_800B0918(Fighter*,Fighter*);
static void fn_80122D2C(HSD_GObj* g){CHECK(false);}
void Fighter_ChangeMotionState(HSD_GObj* g,FtMotionId motion,MotionFlags flags,float start,float speed,float blend,HSD_GObj* other){CHECK(flags==0&&start==0&&speed==1&&blend==0&&!other);event(8);((Fighter*)g->user_data)->motion_id=motion;boundary();}
void ftAnim_8006EBA4(HSD_GObj* g){event(9);boundary();}
#include "blizzard_entry.h"
bool mpCheckFloor(float ax,float ay,float bx,float by,float offset,Vec3* point,int* line,u32* flags,Vec3* normal,int skipline,int skipjoint,int onlyjoint,bool(*filter)(HSD_GObj*,int),HSD_GObj* g){CHECK(ax==bx&&offset==0&&skipline==-1&&skipjoint==-1&&onlyjoint==-1&&!filter&&!g);event(10);*point=(Vec3){ax+1,3,0};*line=2;*flags=0;*normal=(Vec3){0,1,0};return floor_found;}
mp_UnkStruct0* mpIsland_8005AB54(int line){return island_mode==0?NULL:&island[island_mode==1?0:(line==1?0:1)];}
static bool ftCo_800A2718(mp_UnkStruct0* value){return value&&blocked;}
bool grBigBlue_801EF844(int line){return excluded;}
bool grInishie1_801FCAAC(int line){return false;}
bool grCorneria_801E2D90(int line){return false;}
bool grVenom_80206D10(int line){return false;}
static bool ftCo_800A75DC_CheckFloor(float a,float b,float c,float d,Vec3* pos,int* line,u32* flags,Vec3* normal){bool found=mpCheckFloor(a,b,c,d,0,pos,line,flags,normal,-1,-1,-1,NULL,NULL);return found&&!excluded;}
#include "stock_bodies.h"
#include "common_bodies.h"
static bool record_input(HSD_GObj* g,uint32_t id){bool result=nativeCpuHistory_Record(g,id);boundary();return result;}
static bool read_pose(HSD_GObj* g,Vec3* pos,float* facing){bool result=nativeCpuHistory_ReadPose(g,pos,facing);boundary();return result;}
static bool apply_input(HSD_GObj* g){bool result=nativeCpuHistory_Apply(g);boundary();return result;}
static bool query_is_fighter(uint32_t id){++query_calls;return nativeFighterTarget_IsFighter(id);}
static void retire_query(HSD_GObj* g){CHECK(g==&object[1]);boundary();}
static uint32_t member(HSD_GObj* g,unsigned index){CHECK(g==&object[1]&&index==0);return target_live?(uint32_t)target[1].x8_spawnNum:0;}
static void prepare(Fighter* fp){ftCo_CpuPrepareCombatPlan(fp);boundary();}
static bool prepare_destination(Fighter* fp){bool value=ftCo_CpuPrepareDestinationUpdate(fp);boundary();return value;}
static void recover(Fighter* fp){ftCo_800A8DE4(fp);boundary();}
static bool attack(Fighter* fp){bool result=ftCo_800ADE48(fp);boundary();return result;}
static void no_target(Fighter* fp){ftCo_800AEFB8(fp);boundary();}
static void destination(Fighter* fp,float x,float y,float spacing){ftCo_800A1F3C(fp,x,y,spacing);boundary();}
static bool island_bounds(int line,Vec3* left,Vec3* right){mp_UnkStruct0* i=mpIsland_8005AB54(line);if(!i)return false;*left=i->x8;*right=i->x14;boundary();return true;}
static bool same_island(int a,int b){mp_UnkStruct0* first=mpIsland_8005AB54(a);return first&&first==mpIsland_8005AB54(b);}
static bool excluded_line(int line){return excluded;}
static bool blocked_line(int line){return ftCo_800A2718(mpIsland_8005AB54(line));}
static ItemKind held_kind(HSD_GObj* g){return ((Item*)g->user_data)->kind;}
float HSD_Randf(void){event(11);return random_value;}
void ftCo_800B463C(Fighter* fp,cmd_t command){event(100+command);boundary();}
void ftCo_800B46B8(Fighter* fp,cmd_t command,arg_t arg){event(200+command);event(arg);boundary();}
static void ftCo_800B4880(Fighter* fp,int script){event(300+script);selected_script[side]=script;boundary();}
static void ftCo_CpuClearTargetAndFinish(Fighter* f){event(12);}
static void ftCo_CpuSetNeutralStick(Fighter* f){ftCo_800B46B8(f,CpuCmd_SetLstickX,0);ftCo_800B46B8(f,CpuCmd_SetLstickY,0);}
static bool ftCo_IsGrabbing(Fighter* fp){return true;}
static void ftCo_CpuPressAWithLstickX(Fighter* fp,int arg){event(13);event(arg);}
static void ftCo_CpuFlickLstickX(Fighter* fp,int a,int b){event(14);event(a);event(b);}
static void ftCo_800A0C8C(Fighter* fp){event(15);}
mp_UnkStruct0* mpIsland_8005AC14(Vec3* p,float offset){return NULL;}
void mpIsland_8005ACE8(mp_UnkStruct0* i,Vec3* a,Vec3* b){CHECK(false);}
#include "capture_body.h"
static void unused(void){fprintf(stderr,"Unexpected unused Host call\n");abort();}
static void unused_sound(Fighter* fp,int id,u8 volume,u8 pan){unused();}
static const NativeFighterSoundChannel unused_channels[7]={unused_sound,unused_sound,unused_sound,unused_sound,unused_sound,unused_sound,unused_sound};
static void choose(unsigned which){side=which;heads[HSD_GOBJ_PLINK_FIGHTER]=target_live?&target_object[which]:NULL;}
static void reset(unsigned motion,unsigned target_motion,unsigned bits,unsigned geometry,unsigned distance)
{
 for(unsigned i=0;i<2;++i){protect(i,4);memset(pages[i],0,65536);f[i]=pages[i];object[i]=(HSD_GObj){.user_data=f[i]};f[i]->gobj=&object[i];f[i]->x8_spawnNum=source_id[i]=0x80000011+i;source_live[i]=true;
  f[i]->kind=i?Ft_Kind_Mario:Ft_Kind_Nana;f[i]->cpu.xC=6;f[i]->motion_id=motion;f[i]->ground_or_air=bits&1;f[i]->x2225_b3=(bits>>1)&1;f[i]->cpu.xFA_b7=(bits>>2)&1;f[i]->cpu.xFB_b0=(bits>>3)&1;
  f[i]->cpu.x18=(bits>>4)%6;f[i]->cpu.x1C=2;f[i]->cpu.x20=3;f[i]->cpu.x60=geometry&1;f[i]->cpu.x56C=1;f[i]->co_attrs.mid_walk_point=1.5f;f[i]->cur_pos=(Vec3){distance==0?-25:distance==1?0:25,0,7};f[i]->pos_delta=(Vec3){0,(bits&16)?-1:0,4};f[i]->coll_data.floor.index=1;
  f[i]->cpu.x448=f[i]->cpu.xFC;f[i]->cpu.x444=f[i]->cpu.xFC+5;f[i]->cpu.lstick=(S8Vec2){17,18};f[i]->cpu.cstick=(S8Vec2){19,20};f[i]->cpu.buttons=0xf00d;f[i]->cpu.ltrigger=21;f[i]->cpu.rtrigger=22;
  for(unsigned k=0;k<30;++k){f[i]->cpu.xFC[k].cur_pos=(Vec3){10.25f,12.5f,13};f[i]->cpu.xFC[k].facing_dir=-1;f[i]->cpu.xFC[k].lstick=(S8Vec2){37,38};f[i]->cpu.xFC[k].cstick=(S8Vec2){39,40};f[i]->cpu.xFC[k].x4=41;f[i]->cpu.xFC[k].x5=42;f[i]->cpu.xFC[k].x0=0x80000001;f[i]->cpu.xFC[k].xA=0xa5;f[i]->cpu.xFC[k].xB=0x5a;}
  target[i]=(Fighter){0};target[i].gobj=&target_object[i];target_object[i]=(HSD_GObj){.user_data=&target[i]};target[i].x8_spawnNum=0x8000abcd;target[i].motion_id=target_motion;target[i].ground_or_air=(bits>>4)&1;target[i].cur_pos=(Vec3){2,4,6};target[i].pos_delta=(Vec3){0.5f,0,3};target[i].facing_dir=-1;target[i].dmg.x1830_percent=180;target[i].cpu.x564=3;target[i].coll_data.floor.index=2;target[i].input.lstick[0]=(Vec2){.5f,.25f};target[i].input.cstick[0]=(Vec2){.125f,.75f};target[i].input.triggers[0]=.75f;target[i].input.held_buttons[0]=0x80000001;
  trace_count[i]=0;selected_script[i]=0;}
 target_live=true;retire_at=calls=retire_mode=0;floor_found=geometry&2;blocked=geometry&4;excluded=geometry&8;island_mode=(geometry>>4)%3;island[0].x8=(Vec3){-20,5,0};island[0].x14=(Vec3){20,5,0};island[1].x8=(Vec3){-10,6,0};island[1].x14=(Vec3){10,6,0};
 flag("copy_controls",f[0]->cpu.xFA_b7);flag("recovery_lock",f[0]->cpu.xFB_b0);word("history_target",target[1].x8_spawnNum);random_value=.25f;
}
static void near(float a,float b){float error=fabsf(a-b);if(error>max_error)max_error=error;CHECK(error<=0.00002f);}
static void compare(bool mode)
{
 ++pairs;CHECK(trace_count[0]==trace_count[1]);CHECK(!memcmp(trace[0],trace[1],trace_count[0]*sizeof(int)));
 CHECK(f[0]->cpu.level==f[1]->cpu.level&&f[0]->cpu.x18==f[1]->cpu.x18&&f[0]->cpu.x60==f[1]->cpu.x60&&f[0]->cpu.xFA_b2==f[1]->cpu.xFA_b2);
 CHECK(f[0]->cpu.xF8_b0==f[1]->cpu.xF8_b0&&f[0]->cpu.xF9_b2==f[1]->cpu.xF9_b2&&f[0]->cpu.xF9_b4==f[1]->cpu.xF9_b4);
 CHECK(f[0]->cpu.xFA_b7==getflag("copy_controls")&&f[0]->cpu.xFB_b0==getflag("recovery_lock"));
 CHECK(!memcmp(&f[0]->cpu.lstick,&f[1]->cpu.lstick,sizeof(S8Vec2))&&!memcmp(&f[0]->cpu.cstick,&f[1]->cpu.cstick,sizeof(S8Vec2))&&f[0]->cpu.buttons==f[1]->cpu.buttons&&f[0]->cpu.ltrigger==f[1]->cpu.ltrigger&&f[0]->cpu.rtrigger==f[1]->cpu.rtrigger);
 near(f[0]->cpu.x54.x,f[1]->cpu.x54.x);near(f[0]->cpu.x54.y,f[1]->cpu.x54.y);near(f[0]->cpu.x38,f[1]->cpu.x38);
 near(f[0]->cur_pos.x,f[1]->cur_pos.x);near(f[0]->cur_pos.y,f[1]->cur_pos.y);near(f[0]->cur_pos.z,f[1]->cur_pos.z);CHECK(f[0]->facing_dir==f[1]->facing_dir&&f[0]->motion_id==f[1]->motion_id);
 CHECK(f[0]->cmd_vars[0]==f[1]->cmd_vars[0]&&f[0]->cmd_vars[3]==f[1]->cmd_vars[3]&&f[0]->throw_flags==f[1]->throw_flags);
 if(mode){CHECK(f[0]->cpu.x444-f[0]->cpu.xFC==f[1]->cpu.x444-f[1]->cpu.xFC&&f[0]->cpu.x448-f[0]->cpu.xFC==f[1]->cpu.x448-f[1]->cpu.xFC);CHECK(!memcmp(f[0]->cpu.xFC,f[1]->cpu.xFC,sizeof(f[0]->cpu.xFC)));}
 if(f[0]->motion_id==ftPp_MS_SpecialLw&&f[0]->accessory4_cb){CHECK(f[1]->accessory4_cb==script_fn_blizzard_cue&&getword("blizzard_countdown")==f[0]->mv.pp.speciallw.x0&&getflag("blizzard_emitting")==f[0]->mv.pp.speciallw.x4_b0);}
}
int main(void)
{
 for(unsigned i=0;i<2;++i){pages[i]=VirtualAlloc(NULL,65536,0x3000,4);CHECK(pages[i]);}
 MotionState states[26]={0};NativeFighterDefinition base={.fighter_root="ftDataTest",.motion_state_first=341,.motion_state_count=26,.motion_states=states};NativeFighterHost host={0};
#if OWNED_ROUTE
#include "host_1.inc"
#else
#include "host_0.inc"
#endif
 host.sound_channels=unused_channels;host.abs_float=fabsf;host.callback_source_alive=nativeCallbackSource_Alive;host.family_member_id=member;host.target_is_fighter=query_is_fighter;host.target_position=nativeFighterTarget_Position;host.target_position_delta=nativeFighterTarget_PositionDelta;host.target_airborne=nativeFighterTarget_Airborne;host.target_motion=nativeFighterTarget_Motion;host.target_damage=nativeFighterTarget_Damage;host.target_facing=nativeFighterTarget_Facing;host.target_sleeping=nativeFighterTarget_Sleeping;host.target_floor_line=nativeFighterTarget_FloorLine;host.target_cpu_body_half_width=nativeFighterTarget_CpuBodyHalfWidth;host.held_item_kind=held_kind;host.enter_landing_basic=retire_query;
 host.cpu_record_input=record_input;host.cpu_read_history_pose=read_pose;host.cpu_apply_recorded_input=apply_input;host.cpu_prepare_combat_plan=prepare;host.cpu_prepare_destination_update=prepare_destination;host.cpu_update_recovery_destination=recover;host.cpu_plan_attack=attack;host.cpu_plan_without_target=no_target;host.cpu_set_destination=destination;host.stage_read_floor=mpCheckFloor;host.stage_line_island_bounds=island_bounds;host.stage_same_line_island=same_island;host.stage_cpu_floor_excluded=excluded_line;host.stage_cpu_island_blocked=blocked_line;host.change_motion_state=Fighter_ChangeMotionState;host.start_animation=ftAnim_8006EBA4;host.random_float=HSD_Randf;host.cpu_command=ftCo_800B463C;host.cpu_command_arg=ftCo_800B46B8;host.cpu_run_script=ftCo_800B4880;
 CHECK(FreakMelee_SetFighterParameters(tuning,sizeof(tuning)/4,tuning_schema));definition=FreakMelee_GetFighterModule(&base,&host);CHECK(definition&&definition->on_cpu_mode_update&&definition->on_cpu_output);
 const int motions[]={ftCo_MS_Wait,ftCo_MS_Squat,ftCo_MS_SquatWait,ftCo_MS_Landing,ftCo_MS_LandingFallSpecial,ftCo_MS_LandingAirN,ftCo_MS_LandingAirF,ftCo_MS_LandingAirLw,ftCo_MS_CliffCatch,ftCo_MS_CliffWait,ftCo_MS_FireFlowerShoot,ftCo_MS_FireFlowerShootAir,ftCo_MS_Run,357,358,361,366,367};
 for(unsigned m=0;m<sizeof(motions)/sizeof(*motions);++m)for(unsigned tm=0;tm<370;tm+=3)for(unsigned bits=0;bits<32;++bits)for(unsigned geometry=0;geometry<48;geometry+=7){reset(motions[m],tm,bits,geometry,(tm/3)%3);choose(0);ftCo_800B101C(f[0]);choose(1);CHECK(definition->on_cpu_mode_update(&object[1]));compare(true);}
 for(unsigned motion=0;motion<370;++motion)for(unsigned tm=0;tm<2;++tm)for(unsigned bits=0;bits<8;++bits){reset(motion,tm?357:14,bits,0,1);choose(0);ftCo_800B0AF4(f[0]);choose(1);CHECK(definition->on_cpu_output(&object[1]));compare(false);}
 const ItemKind held[]={It_Kind_Box,It_Kind_Taru,It_Kind_Kusudama,It_Kind_TaruCann,It_Kind_Heart};
 for(unsigned motion=0;motion<370;++motion)for(unsigned tm=0;tm<370;++tm)for(unsigned bits=0;bits<4;++bits){reset(motion,tm,bits,0,1);choose(0);bool expected=ftCo_800B0CA8(f[0],&target[0]);choose(1);CHECK(expected==script_fn_follower_copy_allowed(&object[1],target[1].x8_spawnNum));++eligibility_pairs;}
 for(unsigned h=0;h<5;++h)for(unsigned motion=0;motion<370;++motion)for(unsigned sync=0;sync<2;++sync){reset(motion,14,sync?2:0,0,1);Item item={.kind=held[h]};HSD_GObj g={.user_data=&item};f[0]->item_gobj=f[1]->item_gobj=&g;choose(0);bool expected=ftCo_800B0CA8(f[0],&target[0]);choose(1);CHECK(expected==script_fn_follower_copy_allowed(&object[1],target[1].x8_spawnNum));++eligibility_pairs;}
 const float speeds[]={-1.5001f,-1.5f,-1.4999f,0,1.4999f,1.5f,1.5001f};
 const float distances[]={24.9999f,25,25.0001f,-24.9999f,-25,-25.0001f};
 for(unsigned v=0;v<7;++v)for(unsigned d=0;d<6;++d)for(unsigned lock=0;lock<2;++lock)for(unsigned behavior=0;behavior<6;++behavior){reset(14,14,lock?8:0,0,1);for(unsigned s=0;s<2;++s){f[s]->cur_pos=(Vec3){distances[d],0,3};target[s].cur_pos=(Vec3){0,0,6};f[s]->pos_delta.x=speeds[v];target[s].pos_delta.x=0;f[s]->cpu.x18=behavior;}choose(0);bool expected=ftCo_800B0E98(f[0],&target[0]);choose(1);CHECK(expected==script_fn_follower_can_start_copy(&object[1],target[1].x8_spawnNum));CHECK(f[0]->cpu.x60==f[1]->cpu.x60&&f[0]->cpu.xFA_b2==f[1]->cpu.xFA_b2);++threshold_pairs;}
 for(unsigned level=0;level<10;++level)for(unsigned grab=0;grab<3;++grab)for(unsigned rng=0;rng<20;++rng){reset(14,14,0,0,0);random_value=rng*.05f;f[0]->kind=Ft_Kind_Popo;f[0]->victim_gobj=&target_object[0];f[1]->victim_gobj=&target_object[1];f[0]->cpu.level=f[1]->cpu.level=level;f[0]->cpu.x94=f[1]->cpu.x94=grab;choose(0);stock_capture(f[0]);choose(1);ftCo_800B683C(f[1]);++pairs;CHECK(trace_count[0]==trace_count[1]&&!memcmp(trace[0],trace[1],trace_count[0]*4)&&f[0]->cpu.x94==f[1]->cpu.x94&&selected_script[0]==selected_script[1]);}
 // Missing, sleeping and invalid history cases are authored deterministic safety,
 // not execution of old invalid pointers. Normal mode6 never falls into stock.
 for(unsigned bad=0;bad<4;++bad){reset(14,14,4,0,1);choose(1);if(bad==0)target_live=false;else if(bad==1)target[1].x221F_b3=true;else if(bad==2)f[1]->cpu.x444=NULL;else f[1]->cpu.x448=NULL;CHECK(definition->on_cpu_mode_update(&object[1]));CHECK(getflag("copy_controls"));CHECK(f[1]->cpu.level==0);++safety;}
 for(unsigned mode=1;mode<=2;++mode)for(unsigned at=1;at<=6;++at){reset(14,14,0,2,1);choose(1);retire_at=at;retire_mode=mode;definition->on_cpu_mode_update(&object[1]);protect(1,4);if(calls==at){CHECK(f[1]->ground_or_air==GA_Ground);++safety;}else CHECK(calls<at);}
 for(unsigned retired=0;retired<3;++retired){reset(14,14,0,0,1);choose(1);query_calls=0;flag("query_retire",retired!=0);retire_at=retired?1:0;retire_mode=retired;script_fn_query_fixture_run(&object[1]);protect(1,4);CHECK(getword("query_evaluations")==1&&getword("query_after")==(!retired));CHECK(query_calls==(!retired)&&getflag("query_result")==(!retired));++safety;}
 printf("PASS %u whole-phase pairs, %u eligibility pairs, %u threshold pairs, %u checks, %u authored safety cases; max_float_error %.9g\n",pairs,eligibility_pairs,threshold_pairs,checks,safety,max_error);return 0;
}
