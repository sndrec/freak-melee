#include "engine_fixture.h"
#include <melee/ft/fighter.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftchangeparam.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/kinds/ftCommon/types.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/kinds/ftCommon/ftCo_AppealS.h>
#include <melee/ft/kinds/ftCommon/ftCo_Landing.h>
#include <melee/ft/kinds/ftCommon/ftCo_HammerLanding.h>
#include <melee/ft/kinds/ftCommon/ftCo_DownSpot.h>
#include <melee/ft/kinds/ftCommon/ftCo_ItemParasolOpen.h>
#include <melee/it/native_held_article.h>
#include <melee/ft/kinds/ftPeach/ftpeachspecialhi.h>
#include <melee/mp/mpcoll.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned script_state_changes,script_state_hooks;
unsigned script_collision_calls,script_collision_mode,script_collision_retire;
ftCollisionBox script_collision_box;
bool script_collision_owned;
bool nativeScriptFighter_IsAttached(Fighter_GObj* object)
{
    return script_collision_owned&&object&&object->classifier==HSD_GOBJ_CLASS_FIGHTER&&
        object->user_data&&nativeFighterRuntime_SourcePackage(((Fighter*)object->user_data)->kind)!=NULL;
}
static bool retire_change;
void scriptStateFixture_Reset(void){script_state_changes=script_state_hooks=script_collision_calls=script_collision_mode=script_collision_retire=0;script_collision_owned=false;retire_change=false;memset(&script_collision_box,0,sizeof(script_collision_box));}
void scriptStateFixture_RetireOnChange(bool retire){retire_change=retire;}
void Fighter_ChangeMotionState(Fighter_GObj* object,FtMotionId motion,MotionFlags flags,
    float frame,float rate,float blend,Fighter_GObj* source)
{
    (void)flags;(void)blend;(void)source;++script_state_changes;
    Fighter* fp=object->user_data;fp->motion_id=motion;fp->cur_anim_frame=frame;fp->frame_speed_mul=rate;
    if(retire_change) {
        HSD_GObj** cursor=&HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];
        while(*cursor&&*cursor!=object)cursor=&(*cursor)->next;
        if(*cursor==object)*cursor=object->next;
        object->user_data=(void*)1;
    }
}
FighterKind ftLib_GetKind(HSD_GObj* object){return ((Fighter*)object->user_data)->kind;}
bool ftCo_800C5240(Fighter_GObj* object){(void)object;return false;}
void ftCo_800C4ED8(Fighter_GObj* object){(void)object;abort();}
void ftCo_800C5D34(Fighter_GObj* object){(void)object;abort();}
void ftCo_80090780(Fighter_GObj* object){(void)object;abort();}
void ftCo_DownSpot_Enter(Fighter_GObj* object){(void)object;abort();}
void ftCo_HammerLanding_Enter(Fighter_GObj* object){(void)object;abort();}
// This older collision/state fixture does not exercise taunt entry. Its native
// direction selection and lifetime checks have a separate connected fixture.
void ftCo_800DEAE8(Fighter_GObj* object,FtMotionId right,FtMotionId left)
{(void)object;(void)right;(void)left;abort();}
void ftPe_8011D598(Fighter_GObj* object){(void)object;abort();}
void ftMh_MS_389_80151018(Fighter_GObj* object){(void)object;abort();}
void ftCh_GrabUnk1_8015BC88(Fighter_GObj* object){(void)object;abort();}
void ftCommon_8007D7FC(Fighter* fp){fp->ground_or_air=GA_Ground;fp->fall_fast=false;}
void ftCommon_8007D5D4(Fighter* fp){fp->ground_or_air=GA_Air;fp->gr_vel=0;}
void ftCommon_8007D60C(Fighter* fp){fp->ground_or_air=GA_Air;}
void ftCommon_UseAllJumps(Fighter* fp){fp->x1968_jumpsUsed=fp->co_attrs.max_jumps;}
// Held-item visual policy has a separate connected fixture. Never fake success.
void ftCommon_8007F5CC(HSD_GObj* object,bool visible){(void)object;(void)visible;abort();}
void un_80322598(int number,float height){(void)number;(void)height;}
int itGetKind(HSD_GObj* object){(void)object;abort();}
bool ft_80081A00(HSD_GObj* object){(void)object;return false;}
static bool collision(CollData* data,float amount){data->cur_pos.x+=amount;return (data->env_flags&Collide_FloorMask)!=0;}
bool mpColl_8004B108(CollData* data){return collision(data,0.125f);}
bool mpColl_8004B2DC(CollData* data){return collision(data,0.25f);}
bool mpColl_800471F8(CollData* data){return collision(data,0.375f);}
static bool box_collision(CollData* data,ftCollisionBox* box,unsigned mode)
{
    ++script_collision_calls;script_collision_mode=mode;script_collision_box=*box;
    bool contact=collision(data,(float)mode*0.125f);
    Fighter* fp=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER]->user_data;
    if(script_collision_retire==1)++fp->x8_spawnNum;
    if(script_collision_retire==2)fp->gobj->classifier=HSD_GOBJ_CLASS_STAGE;
    if(script_collision_retire==3)fp->gobj->user_data=(void*)1;
    if(script_collision_retire==4)++((NativeScriptFighterProgram*)nativeScriptFighter_Program(fp->kind))->identity;
    if(script_collision_retire==5)++((NativeScriptCompileBank*)nativeScriptFighter_Program(fp->kind)->bank)->storage_layout.identity;
    if(script_collision_retire==6)++((NativeScriptCompileBank*)nativeScriptFighter_Program(fp->kind)->bank)->storage_parameters.identity;
    if(script_collision_retire==7)++nativeGObjPool.lifetime;
    return contact;
}
bool mpColl_8004B21C(CollData* data,ftCollisionBox* box){return box_collision(data,box,1);}
bool mpColl_8004B3F0(CollData* data,ftCollisionBox* box){return box_collision(data,box,2);}
bool mpColl_8004730C(CollData* data,ftCollisionBox* box){return box_collision(data,box,3);}

#include "script_state_actual.h"
// Phase-two ledge entry belongs to the isolated tether cohort. This older
// common-state fixture must never substitute a successful engine call.
void ftCo_8009B390(Fighter_GObj* object,float multiplier)
{ (void)object; (void)multiplier; abort(); }
bool ft_8008239C(Fighter_GObj* object,int facing,ftCollisionBox* box)
{(void)object;(void)facing;(void)box;abort();}
void ftCommon_8007D92C(Fighter_GObj* object){(void)object;abort();}
void ftCo_80097D40(Fighter_GObj* object){(void)object;abort();}
