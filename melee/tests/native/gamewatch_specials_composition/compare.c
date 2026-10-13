// Unchanged frozen stock bodies versus actual generated callbacks. Shared
// physics/animation/collision are controlled call-order boundaries, not an engine
// simulation. Creation, paper rendering and absent absorber hit query are open.
#include "behavior.generated.h"
#include "parameters.h"
#include <melee/ft/fighter.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/kinds/ftGameWatch/types.h>
#include <melee/ft/kinds/ftGameWatch/forward.h>
#include <melee/it/item.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/random.h>
#include <stdio.h>
#include <stdlib.h>
static Fighter fighters[2]; static HSD_GObj owners[2],*heads[32];
HSD_GObj** HSD_GObjPLinkHead=heads;
static FighterBone parts[2][53];static HSD_JObj roots[2];
static ftGameWatchAttributes tuning;static struct ftCommonData common;
struct ftCommonData* p_ftCommonData=&common;
static NativeFighterDefinition base;static MotionState motions[26];
static const NativeFighterDefinition* definition;
static unsigned family,checks,failures,pairs,phase_pairs,roll_pairs,entry_pairs;
static unsigned seeds[2];static bool remaining,ground,landed,ledge,cliff;
static const char* current;
static unsigned phase_seen[26], judge_seen;
typedef struct SpecialEvent { char op;int a,b;float x,y; } SpecialEvent;
static SpecialEvent events[2][128];static unsigned count[2];
static void check(bool v,const char* reason){++checks;if(!v&&failures++<16)fprintf(stderr,"%s %s pair%u\n",current?current:"setup",reason,pairs);}
static void event(char op,int a,int b,float x,float y){check(count[family]<128,"event bound");events[family][count[family]++]=(SpecialEvent){op,a,b,x,y};}
static void set_state(const char* name,int value){for(unsigned i=0;i<definition->state_field_count;i++)if(!strcmp(name,definition->state_fields[i].name)){memcpy(fighters[1].native_module_state+script_state_base+definition->state_fields[i].offset,&value,definition->state_fields[i].type==NativeFighterState_Bool?1:4);return;}abort();}
static int get_state(const char* name){for(unsigned i=0;i<definition->state_field_count;i++)if(!strcmp(name,definition->state_fields[i].name)){int value=0;memcpy(&value,fighters[1].native_module_state+script_state_base+definition->state_fields[i].offset,definition->state_fields[i].type==NativeFighterState_Bool?1:4);return value;}abort();}
static float pf(const char* name){for(unsigned i=0;i<FIXTURE_PARAMETER_COUNT;i++)if(!strcmp(name,fixture_parameter_names[i]))return script_parameters[i].f;abort();}
static void pi(const char* name,int v){for(unsigned i=0;i<FIXTURE_PARAMETER_COUNT;i++)if(!strcmp(name,fixture_parameter_names[i])){script_parameters[i].i=v;return;}abort();}
void Fighter_ChangeMotionState(Fighter_GObj* g,FtMotionId motion,MotionFlags flags,float frame,float rate,float blend,Fighter_GObj* donor){check(g==&owners[family]&&blend==0&&!donor,"motion args");fighters[family].motion_id=motion;fighters[family].cur_anim_frame=frame;event('M',motion,flags,frame,rate);}
void ftAnim_8006EBA4(Fighter_GObj* g){(void)g;event('A',0,0,0,0);}
bool ftAnim_IsFramesRemaining(Fighter_GObj* g){(void)g;event('Q',0,0,0,0);return remaining;}
void ft_8008A2BC(Fighter_GObj* g){(void)g;fighters[family].motion_id=ftCo_MS_Wait;event('W',0,0,0,0);}
void ftCo_Fall_Enter(Fighter_GObj* g){(void)g;fighters[family].motion_id=ftCo_MS_Fall;event('F',0,0,0,0);}
void ft_80084F3C(Fighter_GObj* g){(void)g;event('P',0,0,0,0);}
void ft_80084EEC(Fighter_GObj* g){(void)g;event('P',1,0,0,0);}
void ft_80085154(Fighter_GObj* g){(void)g;event('P',2,0,0,0);}
void ftCommon_Fall(Fighter* f,float gravity,float terminal){(void)f;event('p',0,0,gravity,terminal);}
void ftCommon_FallBasic(Fighter* f){(void)f;event('p',1,0,0,0);}
void ftCommon_CalcSelfAccel_Deaccel(Fighter* f,float value){(void)f;event('d',0,0,value,0);}
void ftCommon_8007D5D4(Fighter* f){f->ground_or_air=GA_Air;event('T',1,0,0,0);}
void ftCommon_8007D7FC(Fighter* f){f->ground_or_air=GA_Ground;event('T',0,0,0,0);}
void ftCommon_8007D60C(Fighter* f){f->ground_or_air=GA_Air;f->x1968_jumpsUsed=f->co_attrs.max_jumps;event('J',0,0,0,0);}
bool ft_800827A0(Fighter_GObj* g){(void)g;event('G',0,0,0,0);return ground;}
bool ft_80082708(Fighter_GObj* g){(void)g;event('G',1,0,0,0);return ground;}
GroundOrAir ft_80081D0C(Fighter_GObj* g){(void)g;event('L',0,0,0,0);return landed?GA_Air:GA_Ground;}
bool ft_CheckGroundAndLedge(Fighter_GObj* g,int dir){(void)g;event('E',dir,0,0,0);return ledge;}
bool ftCliffCommon_80081298(Fighter_GObj* g){(void)g;event('C',0,0,0,0);return cliff;}
void ftCliffCommon_80081370(Fighter_GObj* g){(void)g;event('c',0,0,0,0);}
void ftCommon_UpdateFacing(Fighter* f){if(f->input.lstick[0].x!=0)f->facing_dir=f->input.lstick[0].x>0?1:-1;event('D',0,0,0,0);}
void ftPartSetRotY(Fighter* f,int part,float rot){(void)f;event('Y',part,0,rot,0);}
static void root_rotation(Fighter* f,Fighter_Part part,float rot){ftPartSetRotY(f,part,rot);}
Fighter_Part ftParts_GetBoneIndex(Fighter* f,Fighter_Part p){(void)f;return p;}
static unsigned bone_count(Fighter* f){(void)f;return 53;}
void ft_80088510(Fighter* f,int id,u8 volume,u8 pan){(void)f;event('S',id,volume,pan,0);}
void ftCo_80096900(Fighter_GObj* g,int a,int b,bool allow,float mobility,float lag){(void)g;event('B',a,b,mobility,lag);check(allow,"freefall allow");}
static void freefall(Fighter_GObj* g,int a,int b,int allow,float mobility,float lag,float blend){check(blend==0,"freefall zero blend");ftCo_80096900(g,a,b,allow,mobility,lag);}
void ftCo_LandingFallSpecial_Enter(Fighter_GObj* g,bool interrupt,float lag){(void)g;event('Z',interrupt,0,lag,0);}
void ftParts_80074B0C(Fighter_GObj* g,int group,int value){(void)g;event('V',group,value,0,0);}
void ftColl_CreateAbsorbHit(Fighter_GObj* g,AbsorbDesc* desc){(void)g;event('H',desc->x0_bone_id,0,desc->x10_size,desc->x4_offset.x);check(desc->x4_offset.y==tuning.x80_GAMEWATCH_PANIC_ABSORPTION.x4_offset.y&&desc->x4_offset.z==tuning.x80_GAMEWATCH_PANIC_ABSORPTION.x4_offset.z,"absorber offsets");fighters[family].x2218_b6=1;}
void ftColl_8007ABD0(HitCapsule* h,u32 damage,Fighter_GObj* g){(void)g;event('h',(int)(h-fighters[family].x914),damage,0,0);h->damage=damage;}
bool ftCo_800BFFD0(Fighter* f,int id,bool arg){(void)f;event('b',id,arg,0,0);return true;}
s32 HSD_Randi(s32 bound){check(bound>0,"defined RNG total");event('R',bound,0,0,0);seeds[family]=seeds[family]*1664525u+1013904223u;return (seeds[family]>>8)%(unsigned)bound;}
static s32 random_int(s32 bound){return HSD_Randi(bound);}
static bool signal(Fighter_GObj* g,s32 id,uint32_t hash,float payload){(void)g;(void)id;(void)hash;(void)payload;return false;}
static bool remove_owned(Fighter_GObj* g,s32 id){(void)g;(void)id;event('r',0,0,0,0);return true;}
// Original setup functions are retained only to compare callback presence;
// neither allocator nor these constructor stubs is within the accepted matrix.
void lb_8000B1CC(HSD_JObj* j,Vec3* offset,Vec3* p){(void)j;(void)offset;*p=(Vec3){0};}
float ftCommon_GetModelScale(Fighter* f){(void)f;return 1;}
HSD_GObj* it_802C7774(float f,HSD_GObj* p,Vec3* v,s32 part,u32 value){(void)f;(void)p;(void)v;(void)part;(void)value;abort();}
HSD_GObj* it_8028FAF4(Item_GObj* p,Vec3* v){(void)p;(void)v;abort();}
Item_GObj* it_802C8038(Item_GObj* p,Vec3* v,s32 a,s32 b,float dir,float offset){(void)p;(void)v;(void)a;(void)b;(void)dir;(void)offset;abort();}
HSD_GObj* it_802C7D60(Item_GObj* p,Vec3* v,Fighter_Part part,float dir){(void)p;(void)v;(void)part;(void)dir;abort();}
void it_802C7A84(Item_GObj* g){(void)g;abort();}
void it_802C8158(Item_GObj* g){(void)g;abort();}
void it_802C7E94(Item_GObj* g){(void)g;abort();}
void ftGw_Init_OnDamage(Fighter_GObj* g){(void)g;abort();}
#define ARTICLE_BOUNDARIES
#include "article_compare.h"
#include "original.h"
#undef ARTICLE_BOUNDARIES
static void select(unsigned f){family=f;heads[HSD_GOBJ_PLINK_FIGHTER]=&owners[f];}
static void reset(unsigned motion,unsigned flags,float frame,float input)
{
 memset(fighters,0,sizeof(fighters));memset(owners,0,sizeof(owners));memset(count,0,sizeof(count));
 for(unsigned i=0;i<2;i++){owners[i].user_data=&fighters[i];fighters[i].gobj=&owners[i];fighters[i].kind=Ft_Kind_CustomFirst;fighters[i].x8_spawnNum=(int32_t)0xFEDCBA98u;fighters[i].parts=parts[i];parts[i][0].joint=&roots[i];fighters[i].dat_attrs=&tuning;fighters[i].facing_dir=(flags&1)?-1:1;fighters[i].motion_id=motion;fighters[i].cur_anim_frame=frame;fighters[i].self_vel=(Vec3){3.25f,(flags&2)?-1.5f:2.5f,9};fighters[i].x74_self_accel.y=4;fighters[i].cmd_vars[0]=(flags>>2)%4;fighters[i].cmd_vars[1]=7;fighters[i].ground_or_air=motion>=364?GA_Air:GA_Ground;fighters[i].input.lstick[0].x=input;fighters[i].input.held_buttons[0]=(flags&16)?HSD_PAD_B:0;fighters[i].co_attrs.max_jumps=2;fighters[i].absorb_hit.skip_update_pos=1;fighters[i].x2218_b6=1;fighters[i].x5AC.model_num=11;seeds[i]=71+flags;
 for(unsigned h=0;h<4;h++)fighters[i].x914[h].state=(flags&(1<<h))?HitCapsule_Enabled:HitCapsule_Disabled;
 }
 int latest=(motion-355)%9,previous=(latest+2)%9;fighters[0].u.gw.x222C_judgeVar1=latest;fighters[0].u.gw.x2230_judgeVar2=previous;set_state("judge_latest",latest);set_state("judge_previous",previous);
 fighters[0].u.gw.x2234=flags&8?1:0;set_state("judge_impulse_used",flags&8?1:0);
 fighters[0].u.gw.x2238_panicCharge=flags%5;fighters[0].u.gw.x223C_panicDamage=(flags%3)*13;set_state("panic_charge",flags%5);set_state("panic_damage",(flags%3)*13);
 fighters[0].mv.gw.SpecialLw.isRelease=flags&8?1:0;fighters[0].mv.gw.SpecialLw.turnFrames=(int)(flags%5)-1;set_state("panic_release",flags&8?1:0);set_state("panic_turn",(int)(flags%5)-1);
 set_state("judge_handle",-1);set_state("rescue_handle",-1);set_state("panic_handle",-1);
 remaining=flags&32;ground=flags&16;landed=flags&8;ledge=flags&4;cliff=flags&2;
}
static void equal(void)
{
 ++pairs;check(count[0]==count[1],"event count");for(unsigned i=0;i<count[0]&&i<count[1];i++){SpecialEvent*a=&events[0][i],*b=&events[1][i];check(a->op==b->op&&a->a==b->a&&a->b==b->b&&a->x==b->x&&a->y==b->y,"ordered boundary");}
 check(fighters[0].motion_id==fighters[1].motion_id&&fighters[0].ground_or_air==fighters[1].ground_or_air&&fighters[0].cur_anim_frame==fighters[1].cur_anim_frame,"transition");check(!memcmp(fighters[0].cmd_vars,fighters[1].cmd_vars,sizeof(fighters[0].cmd_vars)),"commands");check(!memcmp(&fighters[0].self_vel,&fighters[1].self_vel,sizeof(Vec3)),"velocity");check(fighters[0].x74_self_accel.y==fighters[1].x74_self_accel.y,"acceleration");check(fighters[0].facing_dir==fighters[1].facing_dir&&fighters[0].lstick_angle==fighters[1].lstick_angle,"turn+launchangle");check(fighters[0].x1968_jumpsUsed==fighters[1].x1968_jumpsUsed,"jumps");
 check(fighters[0].u.gw.x222C_judgeVar1==get_state("judge_latest")&&fighters[0].u.gw.x2230_judgeVar2==get_state("judge_previous"),"Judge history");check(fighters[0].u.gw.x2234==get_state("judge_impulse_used"),"Judge impulse gate");check(fighters[0].u.gw.x2238_panicCharge==get_state("panic_charge")&&fighters[0].u.gw.x223C_panicDamage==get_state("panic_damage"),"Panic charge+damage");check(fighters[0].mv.gw.SpecialLw.isRelease==get_state("panic_release")&&fighters[0].mv.gw.SpecialLw.turnFrames==get_state("panic_turn"),"Panic release+turn");
 check(fighters[0].x2218_b6==fighters[1].x2218_b6&&fighters[0].absorb_hit.skip_update_pos==fighters[1].absorb_hit.skip_update_pos,"absorber flags");check(seeds[0]==seeds[1],"RNG bound/count/advance");check((fighters[0].accessory4_cb!=NULL)==(fighters[1].accessory4_cb!=NULL),"accessory presence only");for(unsigned h=0;h<4;h++)check(fighters[0].x914[h].damage==fighters[1].x914[h].damage,"hit damage");
}
static HSD_GObjEvent original(unsigned motion,unsigned phase)
{
 static HSD_GObjEvent table[8][4]={
 {ftGw_SpecialS_Anim,ftGw_SpecialS_IASA,ftGw_SpecialS_Phys,ftGw_SpecialS_Coll},
 {ftGw_SpecialAirS_Anim,ftGw_SpecialAirS_IASA,ftGw_SpecialAirS_Phys,ftGw_SpecialAirS_Coll},
 {ftGw_SpecialHi_Anim,ftGw_SpecialHi_IASA,ftGw_SpecialHi_Phys,ftGw_SpecialHi_Coll},
 {ftGw_SpecialAirHi_Anim,ftGw_SpecialAirHi_IASA,ftGw_SpecialAirHi_Phys,ftGw_SpecialAirHi_Coll},
 {ftGw_SpecialLw_Anim,ftGw_SpecialLw_IASA,ftGw_SpecialLw_Phys,ftGw_SpecialLw_Coll},
 {ftGw_SpecialLwCatch_Anim,ftGw_SpecialLwCatch_IASA,ftGw_SpecialLwCatch_Phys,ftGw_SpecialLwCatch_Coll},
 {ftGw_SpecialLwShoot_Anim,ftGw_SpecialLwShoot_IASA,ftGw_SpecialLwShoot_Phys,ftGw_SpecialLwShoot_Coll},
 {ftGw_SpecialAirLw_Anim,ftGw_SpecialAirLw_IASA,ftGw_SpecialAirLw_Phys,ftGw_SpecialAirLw_Coll}};
 if(motion<=363)return table[0][phase];if(motion<=372)return table[1][phase];if(motion<=374)return table[motion-371][phase];if(motion<=378)return table[motion-371][phase];
 static HSD_GObjEvent tail[2][4]={{ftGw_SpecialAirLwCatch_Anim,ftGw_SpecialAirLwCatch_IASA,ftGw_SpecialAirLwCatch_Phys,ftGw_SpecialAirLwCatch_Coll},{ftGw_SpecialAirLwShoot_Anim,ftGw_SpecialAirLwShoot_IASA,ftGw_SpecialAirLwShoot_Phys,ftGw_SpecialAirLwShoot_Coll}};
 return tail[motion-379][phase];
}
static void matrix(void)
{
 float frames[]={0,4,4.5f,38,39};float sticks[]={-1,-.3f,-.2f,0,.2f,.3f,1};
 for(unsigned motion=355;motion<=380;motion++)for(unsigned phase=0;phase<4;phase++)for(unsigned flags=0;flags<64;flags++)for(unsigned frame=0;frame<5;frame++){
  reset(motion,flags,frames[frame],sticks[flags%7]);current="phase";
  select(0);original(motion,phase)(&owners[0]);select(1);const MotionState*m=&definition->motion_states[motion-355];HSD_GObjEvent fn[]={m->anim_cb,m->input_cb,m->phys_cb,m->coll_cb};fn[phase](&owners[1]);equal();phase_pairs++;phase_seen[motion-355]|=1u<<phase;
 }
 for(int latest=0;latest<9;latest++)for(int previous=0;previous<9;previous++)for(unsigned pattern=0;pattern<4;pattern++)for(unsigned seed=0;seed<32;seed++){
  reset(355,0,0,0);current="roll";fighters[0].u.gw.x222C_judgeVar1=latest;fighters[0].u.gw.x2230_judgeVar2=previous;set_state("judge_latest",latest);set_state("judge_previous",previous);seeds[0]=seeds[1]=seed;
  for(unsigned i=0;i<9;i++){int weight=pattern==0?1:pattern==1?(int)i+1:pattern==2?(i%3==0?0:3):((i+seed)%2?0:2);tuning.x34_GAMEWATCH_JUDGE_ROLL[i]=weight;char n[40];snprintf(n,sizeof(n),"judge_weight_%u",i);pi(n,weight);}
  select(0);int a=ftGw_SpecialS_GetRandomInt(&owners[0]);select(1);int b=script_fn_JudgeRoll(&owners[1]);check(a==b,"selected index");check(b>=0&&b<9,"valid Judge result");judge_seen|=1u<<(unsigned)b;equal();roll_pairs++;
 }
 for(unsigned entry=0;entry<6;entry++)for(unsigned flags=0;flags<64;flags++){
  reset(355,flags,9.5f,.8f);current="entry";select(0);HSD_GObjEvent fns[]={ftGw_SpecialS_Enter,ftGw_SpecialAirS_Enter,ftGw_SpecialHi_Enter,ftGw_SpecialAirHi_Enter,ftGw_SpecialLw_Enter,ftGw_SpecialAirLw_Enter};fns[entry](&owners[0]);select(1);HSD_GObjEvent scripted[]={authored_ground_s,authored_air_s,authored_ground_hi,authored_air_hi,authored_ground_lw,authored_air_lw};scripted[entry](&owners[1]);equal();entry_pairs++;
 }
}
#include "article_compare.h"
int main(void)
{
 NativeFighterHost host={.callback_source_alive=nativeCallbackSource_Alive,.random_int=random_int,.change_motion_state=Fighter_ChangeMotionState,.start_animation=ftAnim_8006EBA4,.animation_frames_remaining=ftAnim_IsFramesRemaining,.finish_ground_action=ft_8008A2BC,.enter_fall=ftCo_Fall_Enter,.ground_friction=ft_80084F3C,.air_gravity_and_friction=ft_80084EEC,.air_motion=ft_80085154,.fall=ftCommon_Fall,.fall_basic=ftCommon_FallBasic,.air_decel=ftCommon_CalcSelfAccel_Deaccel,.ground_to_air=ftCommon_8007D5D4,.air_to_ground=ftCommon_8007D7FC,.force_air_and_use_jumps=ftCommon_8007D60C,.special_ground_collision=ft_800827A0,.ground_collision=ft_80082708,.air_collision=ft_80081D0C,.check_ground_and_ledge=ft_CheckGroundAndLedge,.check_cliff_catch=ftCliffCommon_80081298,.enter_cliff_catch=ftCliffCommon_80081370,.update_facing=ftCommon_UpdateFacing,.set_root_rotation_y=root_rotation,.resolve_bone=ftParts_GetBoneIndex,.bone_count=bone_count,.play_sound=ft_80088510,.enter_freefall_blend=freefall,.enter_landing_fall_special=ftCo_LandingFallSpecial_Enter,.signal_owned_article=signal,.remove_owned_article=remove_owned,.set_visibility_group=ftParts_80074B0C,.create_absorber=ftColl_CreateAbsorbHit,.set_hitbox_damage=ftColl_8007ABD0,.abs_float=fabsf,.item_creator=item_creator,.item_current_owner=current_owner,.target_motion=target_motion,.item_joint_value=joint_value,.item_set_joint_value=joint_set,.item_interact=interaction,.item_change_state_flags=Item_80268E5C,.item_remove=Item_8026A8EC,.item_pause=it_8026B724,.item_resume=it_8026B73C};
 base.source_kind=Ft_Kind_GameWatch;base.joint_count=53;base.motion_state_first=355;base.motion_state_count=26;base.motion_states=motions;base.special_ground_state[1]=355;base.special_air_state[1]=364;base.special_ground_state[2]=373;base.special_air_state[2]=374;base.special_ground_state[3]=375;base.special_air_state[3]=378;
 definition=FreakMelee_GetFighterModule(&base,&host);check(definition!=NULL,"generated module accepted");if(!definition)return 2;check(FreakMelee_SetFighterParameters(fixture_parameter_bits,FIXTURE_PARAMETER_COUNT,FIXTURE_PARAMETER_SCHEMA),"parameters");
 tuning.x20_GAMEWATCH_JUDGE_MOMENTUM_PRESERVE=pf("judge_preserve");tuning.x24_GAMEWATCH_JUDGE_MOMENTUM_MUL=pf("judge_decel");tuning.x28_GAMEWATCH_JUDGE_VEL_Y=pf("judge_impulse");tuning.x2C_GAMEWATCH_JUDGE_FRICTION1=pf("judge_gravity");tuning.x30_GAMEWATCH_JUDGE_FRICTION2=pf("judge_terminal");tuning.x58_GAMEWATCH_RESCUE_STICK_RANGE=pf("rescue_stick");tuning.x5C_GAMEWATCH_RESCUE_ANGLE_UNK=pf("rescue_angle");tuning.x60_GAMEWATCH_RESCUE_LANDING=pf("rescue_landing");tuning.x64_GAMEWATCH_PANIC_MOMENTUM_PRESERVE=pf("panic_preserve");tuning.x68_GAMEWATCH_PANIC_MOMENTUM_MUL=pf("panic_decel");tuning.x6C_GAMEWATCH_PANIC_FALL_ACCEL=pf("panic_gravity");tuning.x70_GAMEWATCH_PANIC_VEL_Y_MAX=pf("panic_terminal");tuning.x74_GAMEWATCH_PANIC_DAMAGE_ADD=pf("panic_damage_add");tuning.x78_GAMEWATCH_PANIC_DAMAGE_MUL=pf("panic_damage_mul");tuning.x7C_GAMEWATCH_PANIC_TURN_FRAMES=pf("panic_turn_frames");tuning.x80_GAMEWATCH_PANIC_ABSORPTION=(AbsorbDesc){0,{1,2,0},4};common.horizontal_stick_deadzone=pf("panic_stick_deadzone");
 matrix();article_matrix();for(unsigned i=0;i<26;i++)check(phase_seen[i]==15,"all four phases for every numerical motion");check(judge_seen==511,"all nine weighted variants reached");printf("Specials source: %u checks, %u failures, %u pairs; phases%u rolls%u entries%u articles%u\n",checks,failures,pairs,phase_pairs,roll_pairs,entry_pairs,article_pairs);return failures?1:0;
}
