#include "attack_boundary.h"
#include "boundary.h"
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftwaitanim.h>
#include <melee/ft/kinds/ftCommon/ftpickupitem.h>
#include <melee/ft/ft_0DF1.h>
#include <melee/ft/ft_0881.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/fighter.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#pragma clang section data = ""
unsigned attack_trace[64],attack_count,attack_stop,attack_failure,attack_stock;
bool attack_pickup,attack_held;int attack_last_motion;
static ftCommonData common;
bool normal_contact,normal_interrupted;
ftCommonData* p_ftCommonData=&common;
static struct Fighter_WaitAnimData motion={.x8=1};
void normal_point(unsigned stage,HSD_GObj* object)
{
 if(attack_count==64)abort();attack_trace[attack_count++]=stage;
 if(attack_stop!=stage)return;attack_stop=0;
 switch(attack_failure){
 case 1:HSD_GObjFree(object);break;
 case 2:{scheduler_fail_write=true;NativeScriptValue result;
  if(nativeScriptFighter_Run(object,NativeScriptFighter_Tick,NULL,0, NULL,&result)!=NativeScriptVM_Error)abort();
  scheduler_fail_write=false;break;}
 case 3:{Fighter* fighter=object->user_data;++fighter->x8_spawnNum;nativeScriptFighter_NewLife(object,fighter->x8_spawnNum);break;}
 case 4:object->user_data=NULL;break;
 case 5:{NativeScriptFighterProgram* program=(NativeScriptFighterProgram*)nativeScriptFighter_Program(((Fighter*)object->user_data)->kind);
  if(!program)abort();program->identity^=1;break;}
 default:abort();
 }
}
void attack_reset(void)
{
 memset(attack_trace,0,sizeof(attack_trace));attack_count=attack_stop=attack_failure=0;attack_pickup=attack_held=false;attack_last_motion=-1;normal_contact=normal_interrupted=false;
 common.xDC=common.xE0=0.2f;common.x20_radians=0.5f;common.xB8_radians=0.5f;common.xBC_radians=0.2f;common.xC0_radians=-0.2f;common.xC4_radians=-0.5f;common.x5F0=99;
}
void attack_record_motion(HSD_GObj* object,int id,MotionFlags flags,float start,float speed,float blend)
{(void)flags;(void)start;(void)speed;(void)blend;attack_last_motion=id;normal_point(Attack_Motion,object);}
void normal_snapshot_inside(void);
void attack_record_source(HSD_GObj* object,float value){if(value==889){normal_snapshot_inside();return;}normal_point((unsigned)value,object);}
bool ftpickupitem_80094790(Fighter_GObj* object){normal_point(Attack_Pickup,object);return attack_pickup;}
void ftAnim_8006EBA4(Fighter_GObj* object){normal_point(Attack_Animation,object);}
bool ftCo_8008A698(Fighter* fighter){(void)fighter;return attack_held;}
void ftCo_8008A6D8(Fighter_GObj* object,s32 id){(void)id;normal_point(Attack_HeldSetup,object);}
void ftCommon_8007EFC0(Fighter* fighter,u32 value){(void)value;normal_point(Attack_CommonFlags,fighter->gobj);}
struct Fighter_WaitAnimData* ftData_80085FD4(Fighter* fighter,FtMotionId id){(void)fighter;(void)id;return &motion;}
bool ftCo_800DF478(Fighter* fighter){(void)fighter;return false;}
float ftCo_GetLStickAngle(Fighter* fighter){return atan2f(fighter->input.lstick[0].y,fighter->input.lstick[0].x);}
float ftCo_GetCStickAngle(Fighter* fighter){return atan2f(fighter->input.cstick[0].y,fighter->input.cstick[0].x);}
void ft_800892A0(HSD_GObj* object){normal_point(Normal_Recount,object);}
void ft_80089824(HSD_GObj* object){normal_point(Normal_HitReset,object);}
#define STOCK(name) void name(HSD_GObj* object){(void)object;++attack_stock;}
STOCK(ftGw_Attack11_Enter) STOCK(ftGw_Attack100Start_Enter) STOCK(ftGw_AttackLw3_Enter)
STOCK(ftGw_AttackAirN_DecideAction) STOCK(ftLk_AttackAir_Enter)
STOCK(ftNs_AttackS4_Enter) STOCK(ftPe_AttackS4_Enter) STOCK(ftGw_AttackS4_Enter)
STOCK(ftLk_AttackAir_800EB3BC) STOCK(ftCl_Init_8014919C)

void efLib_PauseAll(HSD_GObj* object){(void)object;abort();}
void efLib_ResumeAll(HSD_GObj* object){(void)object;abort();}

bool mpColl_800471F8(CollData* data){Fighter* f=(Fighter*)((char*)data-(size_t)&((Fighter*)0)->coll_data);data->cur_pos.x+=7;normal_point(Normal_Geometry,f->gobj);return normal_contact;}
bool ft_80081A00(Fighter_GObj* object){normal_point(Normal_Response,object);return normal_interrupted;}

#include <melee/ft/ftchangeparam.h>
#include <melee/ft/ft_0CDD.h>
#include <melee/ft/kinds/ftCommon/ftCo_HammerJump.h>
#include <melee/ft/kinds/ftNess/ftnessattackhi4.h>
#include <melee/ft/kinds/ftNess/ftnessattacklw4.h>
bool ground_item_throw;unsigned ground_item_use;
bool ftCo_80094E54(Fighter* f){(void)f;return ground_item_throw;}
int itGetUseKind(Item_GObj* object){(void)object;return ground_item_use;}
bool it_8026B594(Item_GObj* object){(void)object;return false;}
bool ftCo_800DF1C8(Fighter* f){return fabsf(f->input.cstick[1].x)<p_ftCommonData->dash_smash_stick_threshold && fabsf(f->input.cstick[0].x)>=p_ftCommonData->dash_smash_stick_threshold;}
bool ftCo_800DF2D8(Fighter* f){return f->input.cstick[1].y<p_ftCommonData->xCC && f->input.cstick[0].y>=p_ftCommonData->xCC;}
bool ftCo_800DF3A8(Fighter* f){return f->input.cstick[1].y>p_ftCommonData->xD4 && f->input.cstick[0].y<=p_ftCommonData->xD4;}
bool ftCo_800DF21C(Fighter* f){return ftCo_800DF1C8(f);}
bool ftCo_800DF30C(Fighter* f){return ftCo_800DF2D8(f);}
bool ftCo_800DF3DC(Fighter* f){return ftCo_800DF3A8(f);}
void ftCo_800957F4(Fighter_GObj* object,FtMotionId motion){attack_last_motion=motion;normal_point(Attack_HeldSetup,object);}
void ftCo_Attack_800CCF58(Fighter_GObj* object,int n){(void)n;normal_point(Attack_HeldSetup,object);}
void ftCo_Attack_800CDD14(Fighter_GObj* object){normal_point(Attack_HeldSetup,object);}
bool ftCo_800C5A50(Fighter_GObj* object){(void)object;abort();}
void ftCommon_800804A0(Fighter* f,float value){f->gr_vel+=value;}
void ft_PlaySFX(Fighter* f,enum_t id,u8 volume,u8 pan){(void)f;(void)id;(void)volume;(void)pan;abort();}
float ftCo_CalcYScaledKnockback(float value,float scale,float power){return value*powf(scale,power);}
struct Fighter_804D6524_t* Fighter_804D6524;
void ftNs_AttackHi4_Enter(Fighter_GObj* object){(void)object;++attack_stock;}
void ftNs_AttackLw4_Enter(Fighter_GObj* object){(void)object;++attack_stock;}
void normal_snapshot_inside(void){abort();}

// Shared special checks call these native table slots only for foreign stock
// actors. Attached source actors must execute their actual interpreted hooks.
static void stock_special(HSD_GObj* o){(void)o;++attack_stock;abort();}
HSD_GObjEvent ftData_SpecialS[Ft_Kind_RuntimeCapacity];
HSD_GObjEvent ftData_SpecialHi[Ft_Kind_RuntimeCapacity];
HSD_GObjEvent ftData_SpecialN[Ft_Kind_RuntimeCapacity];
HSD_GObjEvent ftData_SpecialLw[Ft_Kind_RuntimeCapacity];
float ground_special_friction;unsigned ground_special_friction_reads;
void ground_special_reset(void){
 ground_special_friction=.5f;ground_special_friction_reads=0;
 ftData_SpecialS[Ft_Kind_CustomFirst]=stock_special;
 ftData_SpecialHi[Ft_Kind_CustomFirst]=stock_special;
 ftData_SpecialN[Ft_Kind_CustomFirst]=stock_special;
 ftData_SpecialLw[Ft_Kind_CustomFirst]=stock_special;
}
float mpLib_800569EC(u32 flags){if(flags!=7)abort();++ground_special_friction_reads;return ground_special_friction;}
// Callback installation is outside this input cohort.
void ftCommon_8007F76C(HSD_GObj* o){(void)o;abort();}

// Guard source packages select the common entries (their native Definition is
// NULL). These leaves record entry/motion selection; they do not implement
// shield geometry, effects, animation, sounds or render asset initialization.
void ftCo_800924C0(Fighter_GObj* object){Fighter_ChangeMotionState(object,ftCo_MS_GuardOn,Ft_MF_SkipAnim,0,1,0,NULL);}
void ftCo_80093A50(Fighter_GObj* object){Fighter_ChangeMotionState(object,ftCo_MS_GuardReflect,Ft_MF_SkipAnim,0,1,0,NULL);}
void ftYs_Init_8012BECC(Fighter_GObj* object){(void)object;abort();}
void ftYs_Shield_8012C914(Fighter_GObj* object){(void)object;abort();}
void ftParts_80074B0C(Fighter_GObj* object,int part,int state){(void)object;(void)part;(void)state;abort();}

// Catch setup and item-throw delivery are controlled selected engine entries.
void ftCo_800D8C54(Fighter_GObj* object,FtMotionId motion){Fighter_ChangeMotionState(object,motion,0,0,1,0,NULL);}
void ftCo_80095A30(Fighter_GObj* object){normal_point(220,object);}
void ftCo_800DEAE8(Fighter_GObj* object,FtMotionId right,FtMotionId left){Fighter_ChangeMotionState(object,((Fighter*)object->user_data)->facing_dir<0?left:right,0,0,1,0,NULL);}
unsigned ground_taunt_bonus;
void pl_80040120(int player,int sub){(void)player;(void)sub;++ground_taunt_bonus;}
// Native stock/debug taunt branches are unreachable for an attached source.
void ftCl_Init_80149318(Fighter_GObj* object){(void)object;abort();}
void ftDr_Init_80149910(Fighter_GObj* object){(void)object;abort();}
void ftCo_800DEBD0(Fighter_GObj* object){(void)object;abort();}
void lb_8000B1CC(HSD_JObj* joint,Vec3* offset,Vec3* result){(void)joint;(void)offset;(void)result;abort();}
void lb_800119DC(Vec3* pos,int id,float a,float b,float c){(void)pos;(void)id;(void)a;(void)b;(void)c;abort();}

// Heavy throw entry runs the actual common selector and initializer. Actual
// held-item placement/release and motion callbacks remain separate fixtures.
float ground_heavy_multiplier=1;
float itGetDamageMultiplier(Item_GObj* object){if(!object)abort();return ground_heavy_multiplier;}
void ftCo_80095EFC(Fighter_GObj* object){normal_point(221,object);}
void ftCo_800974C4(Fighter_GObj* object){(void)object;abort();}
void ftCo_800961D0(Fighter_GObj* object){(void)object;abort();}
void ftCo_80096498(Fighter_GObj* object){(void)object;abort();}
