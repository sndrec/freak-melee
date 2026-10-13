#include "air_boundary.h"
#include "boundary.h"
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/ftcommon.h>
#include <melee/mp/mpcoll.h>
#include <melee/mp/mplib.h>
#include <melee/ft/ftwalljump.h>
#include <melee/ft/ftcliffcommon.h>
#include <melee/ft/kinds/ftCommon/ftCo_StopCeil.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
// Explicitly unselected operations retained by the shared setup/air dispatch.
void ftCommon_8007F76C(Fighter_GObj* o){(void)o;abort();}
void ftCommon_UnlockECB(Fighter* f){(void)f;abort();}
void ft_80084E1C(Fighter_GObj* o,float threshold,float acceleration,float speed)
{(void)o;(void)threshold;(void)acceleration;(void)speed;abort();}
unsigned air_trace[32],air_count,air_stop,air_failure;
bool air_contact,air_environment,air_wall,air_cliff;
int air_line;u32 air_flags;
static ftCommonData common;
ftCommonData* p_ftCommonData=&common;
void air_reset(void){memset(air_trace,0,sizeof(air_trace));air_count=air_stop=air_failure=0;air_contact=air_environment=air_wall=air_cliff=false;air_line=0;air_flags=0;common.x25C=.25f;}
void air_point(unsigned stage,HSD_GObj* object)
{
 if(air_count==32)abort();air_trace[air_count++]=stage;
 if(stage!=air_stop)return;air_stop=0;
 switch(air_failure){
 case 1:HSD_GObjFree(object);break;
 case 2:{scheduler_fail_write=true;NativeScriptValue result;if(nativeScriptFighter_Run(object,NativeScriptFighter_Tick,NULL,0, NULL,&result)!=NativeScriptVM_Error)abort();scheduler_fail_write=false;break;}
 case 3:object->user_data=NULL;break;
 case 4:++((Fighter*)object->user_data)->x8_spawnNum;break;
 case 5:object->classifier=HSD_GOBJ_CLASS_ITEM;break;
 case 6:HSD_GObjFree(object);{HSD_GObj* replacement=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);if(replacement!=object)abort();}break;
 case 7:((Fighter*)object->user_data)->ft_data=NULL;break;
 default:abort();
 }
}
static Fighter* from_coll(CollData* data){return (Fighter*)((char*)data- offsetof(Fighter,coll_data));}
void mpCollSetFacingDir(CollData* data,int direction){data->facing_dir=direction;air_point(Air_Facing,from_coll(data)->gobj);}
static bool map(CollData* data,bool (*predicate)(Fighter_GObj*,int),Fighter_GObj* object,unsigned stage)
{data->cur_pos.x+=7;data->cur_pos.y-=3;bool allowed=predicate(object,air_line);air_point(stage,object);return air_contact&&allowed;}
bool mpColl_80047AC8(CollData* data,bool (*predicate)(Fighter_GObj*,int),Fighter_GObj* object){return map(data,predicate,object,Air_MapA);}
bool mpColl_80047E14(CollData* data,bool (*predicate)(Fighter_GObj*,int),Fighter_GObj* object){return map(data,predicate,object,Air_MapE);}
u32 mpLineGetFlags(int line){(void)line;return air_flags;}
bool ft_80081A00(Fighter_GObj* object){air_point(Air_Environment,object);return air_environment;}
bool ftWallJump_8008169C(HSD_GObj* object){air_point(Air_Wall,object);return air_wall;}
bool ftCliffCommon_80081298(Fighter_GObj* object){air_point(Air_Cliff,object);return air_cliff;}
bool ftCo_8009EF68(Fighter_GObj* object){air_point(Air_Aerial,object);return false;}
