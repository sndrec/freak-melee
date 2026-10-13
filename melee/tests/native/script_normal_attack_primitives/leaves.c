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
