// Controlled shared engine boundaries. Fighter decisions use actual originals
// and actual generated callbacks; geometry/allocation/paper are not simulated.
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0881.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/kinds/ftCommon/ftCo_Attack1.h>
#include <melee/ft/kinds/ftCommon/ftCo_Attack100.h>
#include <melee/ft/kinds/ftCommon/ftCo_AttackAir.h>
#include <melee/ft/kinds/ftCommon/ftCo_AttackHi3.h>
#include <melee/ft/kinds/ftCommon/ftCo_AttackHi4.h>
#include <melee/ft/kinds/ftCommon/ftCo_AttackLw3.h>
#include <melee/ft/kinds/ftCommon/ftCo_AttackLw4.h>
#include <melee/ft/kinds/ftCommon/ftCo_AttackS3.h>
#include <melee/ft/kinds/ftCommon/ftCo_AttackS4.h>
#include <melee/ft/kinds/ftCommon/ftCo_Dash.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/kinds/ftCommon/ftCo_Jump.h>
#include <melee/ft/kinds/ftCommon/ftCo_Landing.h>
#include <melee/ft/kinds/ftCommon/ftCo_LandingAir.h>
#include <melee/ft/kinds/ftCommon/ftCo_Squat.h>
#include <melee/ft/kinds/ftCommon/ftCo_SquatWait.h>
#include <melee/ft/kinds/ftCommon/ftCo_Turn.h>
#include <melee/ft/kinds/ftCommon/ftCo_Wait.h>
#include <melee/ft/kinds/ftCommon/ftCo_Walk.h>
#include <melee/ft/kinds/ftCommon/ftpickupitem.h>
static bool finished, collision_trigger, pickup, spawn_success;
static int interrupt_stop, aerial_selected;
static HSD_JObj fighter_joints[2][53], children[2];
static FighterBone fighter_parts[2][53];
static unsigned world_reads[2];
static unsigned bone_count(Fighter*f){check(f==fighters+side,"owned part bounds");return 53;}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ (void)kind; return NULL; }
bool ftAnim_IsFramesRemaining(Fighter_GObj*g)
{check(g==owners+side,"frame query source");event('F',0,0,0);return !finished;}
void Fighter_ChangeMotionState(Fighter_GObj*g,FtMotionId motion,MotionFlags flags,
                              float start,float rate,float blend,Fighter_GObj* donor)
{check(g==owners+side&&blend==0&&donor==NULL,"motion tuple");event('M',motion,flags,start);fighters[side].motion_id=motion;fighters[side].cur_anim_frame=start;fighters[side].frame_speed_mul=rate;}
void ftAnim_8006EBA4(Fighter_GObj*g){check(g==owners+side,"animation source");event('A',0,0,0);}
void ft_8008A2BC(Fighter_GObj*g){check(g==owners+side,"wait source");event('W',0,0,0);fighters[side].motion_id=0;}
void ftCo_Fall_Enter(Fighter_GObj*g){check(g==owners+side,"fall source");event('f',0,0,0);fighters[side].motion_id=29;}
void ftCo_800D638C(Fighter_GObj*g){check(g==owners+side,"crouch source");event('c',0,0,0);fighters[side].motion_id=40;}
void ft_80084FA8(Fighter_GObj*g){check(g==owners+side,"ground motion source");event('G',0,0,0);}
void ft_80084F3C(Fighter_GObj*g){check(g==owners+side,"friction source");event('g',0,0,0);}
void ft_80084DB0(Fighter_GObj*g){check(g==owners+side,"air physics source");event('a',0,0,0);}
void ft_80084104(Fighter_GObj*g){check(g==owners+side,"ground collision source");event('C',0,0,0);if(collision_trigger)fighters[side].ground_or_air=GA_Air;}
void ft_80084280(Fighter_GObj*g){check(g==owners+side,"landing collision source");event('C',1,0,0);if(collision_trigger){fighters[side].ground_or_air=GA_Air;fighters[side].motion_id=29;}}
void ft_80082C74(Fighter_GObj*g,HSD_GObjEvent land){check(g==owners+side&&land!=NULL,"air landing callback");event('C',2,0,0);if(collision_trigger)land(g);}
bool ftpickupitem_80094790(Fighter_GObj*g){check(g==owners+side,"pickup source");event('p',0,pickup,0);return pickup;}
void ft_800892A0(Fighter_GObj*g){check(g==owners+side,"refresh source");event('H',0,0,0);}
void ft_80089824(Fighter_GObj*g){check(g==owners+side,"record source");event('H',1,0,0);}
void ftCommon_8007D7FC(Fighter*f){check(f==fighters+side,"ground conversion");event('L',0,0,0);f->ground_or_air=GA_Ground;}
float ftAnim_8006F484(Fighter_GObj*g){check(g==owners+side,"animation length");event('l',0,0,0);return 24.25f;}
void ftAnim_SetAnimRate(Fighter_GObj*g,float rate){check(g==owners+side,"landing rate");event('r',-1,0,rate);fighters[side].frame_speed_mul=rate;}
void ftCo_Landing_Enter_Basic(Fighter_GObj*g){check(g==owners+side,"basic landing");event('L',1,0,0);fighters[side].ground_or_air=GA_Ground;fighters[side].motion_id=42;}
void ftCo_Wait_IASA(Fighter_GObj*g){check(g==owners+side,"forward IASA");event('I',12,0,0);}
void ftCo_Attack11_IASA(Fighter_GObj*g){check(g==owners+side,"jab IASA");event('I',13,0,0);}
void ftCo_Attack100Loop_IASA(Fighter_GObj*g){check(g==owners+side,"rapid IASA");event('I',14,0,0);}
void ftCo_AttackAirN_IASA(Fighter_GObj*g){check(g==owners+side,"neutral IASA");event('I',15,0,0);}
void ftCo_AttackAirB_IASA(Fighter_GObj*g){check(g==owners+side,"back IASA");event('I',16,0,0);}
void ftCo_AttackAirHi_IASA(Fighter_GObj*g){check(g==owners+side,"up IASA");event('I',17,0,0);}
static bool input_check(Fighter_GObj*g,int operation){check(g==owners+side,"interrupt source");event('I',operation,0,0);return interrupt_stop==operation;}
#define INPUT(NAME,INDEX) bool NAME(Fighter_GObj*g){return input_check(g,INDEX);}
INPUT(ftCo_AttackS4_CheckInput,0) INPUT(ftCo_AttackHi4_CheckInput,1)
INPUT(ftCo_AttackLw4_CheckInput,2) INPUT(ftCo_AttackS3_CheckInput,3)
INPUT(ftCo_AttackHi3_CheckInput,4) INPUT(ftCo_AttackLw3_CheckInput,5)
INPUT(ftCo_Attack1_CheckInput,6) INPUT(ftCo_Jump_CheckInput,7)
INPUT(ftCo_Dash_CheckInput,8) INPUT(ftCo_Squat_CheckInput,9)
INPUT(ftCo_Turn_CheckInput,10) INPUT(ftCo_Walk_CheckInput,11)
#undef INPUT
FtMotionId ftCo_AttackAir_GetMsidFromCStick(Fighter*f)
{check(f==fighters+side,"air selection");return aerial_selected;}
void ftCo_AttackAir_EnterFromMsid(Fighter_GObj*g,FtMotionId motion)
{check(g==owners+side,"shared air entry");event('E',motion,0,0);fighters[side].motion_id=motion;}
void lb_8000B1CC(HSD_JObj*j,Vec3*offset,Vec3*position)
{
 check(offset==NULL||(!offset->x&&!offset->y&&!offset->z),"world origin offset");
 int part=-1;if(j==&children[side])part=100;else for(unsigned i=0;i<53;i++)if(j==&fighter_joints[side][i])part=i;
 check(part>=0,"actual requested world joint");*position=(Vec3){(float)part+2.25f,-(float)part*.5f-3,7.75f};
 ++world_reads[side];event('J',part,0,position->x);event('j',0,0,position->y);event('j',1,0,position->z);
}
void lb_800119DC(Vec3*p,int frames,float strength,float decay,float frequency)
{event('w',frames,0,p->x);event('w',0,0,p->y);event('w',1,0,p->z);event('w',2,0,strength);event('w',3,0,decay);event('w',4,0,frequency);}
static bool world_position(Item_GObj*g,unsigned model,const char*path,Vec3*position)
{check(g==objects[side]+1&&model==0&&!strcmp(path,"0"),"Manhole primary first child");lb_8000B1CC(HSD_JObjGetChild(g->hsd_obj),NULL,position);return true;}
static HSD_GObj* spawn_prop(unsigned k,Fighter_GObj*g,Vec3*p,int part,float facing)
{
 check(g==owners+side&&facing==fighters[side].facing_dir,"prop owner facing");
 check(part==(k<3?32:k==3?1:k==4?17:21),"prop raw part");
 event('B',k,part,p->x);event('b',0,0,p->y);event('b',1,0,p->z);
 return spawn_success?objects[side]+k:NULL;
}
HSD_GObj* itGamewatchGreenhouse_Spawn(Fighter_GObj*g,Vec3*p,Fighter_Part part,float face){return spawn_prop(0,g,p,part,face);}
HSD_GObj* it_802C65E4(Fighter_GObj*g,Vec3*p,Fighter_Part part,float face){return spawn_prop(1,g,p,part,face);}
HSD_GObj* itGamewatchFire_Spawn(Fighter_GObj*g,Vec3*p,Fighter_Part part,float face){return spawn_prop(2,g,p,part,face);}
HSD_GObj* it_802C6C38(Fighter_GObj*g,Vec3*p,enum_t part,float face){return spawn_prop(3,g,p,part,face);}
HSD_GObj* it_802C6F40(Fighter_GObj*g,Vec3*p,Fighter_Part part,float face){return spawn_prop(4,g,p,part,face);}
HSD_GObj* it_802C720C(Fighter_GObj*g,Vec3*p,Fighter_Part part,float face){return spawn_prop(5,g,p,part,face);}
static int32_t create(Fighter_GObj*g,ItemKind kind,Vec3*p,Vec3*v,float facing,bool origin,uint32_t group,float value,bool collision)
{
 check(!v->x&&!v->y&&!v->z&&!origin&&!group&&value==0&&collision,"ordinary prop creation inputs");
 for(unsigned k=0;k<6;k++)if(kind==kinds[k]){HSD_GObj*result=spawn_prop(k,g,p,k<3?32:k==3?1:k==4?17:21,facing);return result?(int32_t)items[side][k].x1C:-1;}
 abort();
}
static bool drop(Fighter_GObj*g,uint32_t item_id,bool normal)
{check(g==owners+side&&item_id==(uint32_t)items[side][9].x1C&&normal,"retained drop");event('D',9,normal,0);return true;}
void ftCommon_8007E6DC(Fighter_GObj*g,Fighter_GObj*item,s32 normal)
{check(item==objects[side]+9&&normal==1,"original drop arguments");drop(g,(uint32_t)items[side][9].x1C,true);}
