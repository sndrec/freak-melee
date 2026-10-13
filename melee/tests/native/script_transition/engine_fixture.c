#include "engine_fixture.h"
#include <melee/ft/native_script_bindings.h>
#include <melee/ft/ft_081B.h>
#include <melee/pl/plbonuslib.h>
#include <melee/sfx/crowdsfx.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftchangeparam.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/native_fighter_runtime.h>
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned script_state_changes,script_state_hooks;
ScriptTransitionChange script_transition_changes[32];
static bool retire_change;
void scriptStateFixture_Reset(void){script_state_changes=script_state_hooks=0;retire_change=false;}
void scriptStateFixture_RetireOnChange(bool retire){retire_change=retire;}
void Fighter_ChangeMotionState(Fighter_GObj* object,FtMotionId motion,MotionFlags flags,
    float frame,float rate,float blend,Fighter_GObj* source)
{
    (void)blend;(void)source;if(script_state_changes>=32)abort();
    script_transition_changes[script_state_changes++]=(ScriptTransitionChange){(uint32_t)motion,(uint32_t)flags,frame,rate};
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
// Taunt is intentionally unavailable here; covered by script_taunt_target_colors.
void ftCo_800DEAE8(Fighter_GObj* object,FtMotionId right,FtMotionId left)
{(void)object;(void)right;(void)left;abort();}
void ftPe_8011D598(Fighter_GObj* object){(void)object;abort();}
void ftMh_MS_389_80151018(Fighter_GObj* object){(void)object;abort();}
void ftCh_GrabUnk1_8015BC88(Fighter_GObj* object){(void)object;abort();}
void ftCommon_8007D60C(Fighter* fp){fp->ground_or_air=GA_Air;}
void ftCommon_UseAllJumps(Fighter* fp){fp->x1968_jumpsUsed=fp->co_attrs.max_jumps;}
int un_80322598(int number,float height){(void)number;(void)height;return 0;}
int itGetKind(HSD_GObj* object){(void)object;abort();}
bool ft_80081A00(HSD_GObj* object){(void)object;return false;}
static bool collision(CollData* data,float amount){data->cur_pos.x+=amount;return (data->env_flags&Collide_FloorMask)!=0;}
bool mpColl_8004B108(CollData* data){return collision(data,0.125f);}
bool mpColl_8004B2DC(CollData* data){return collision(data,0.25f);}
bool mpColl_800471F8(CollData* data){return collision(data,0.375f);}
bool mpColl_8004B21C(CollData* data,ftCollisionBox* box){return collision(data,box->top*0.03125f);}
bool mpColl_8004730C(CollData* data,ftCollisionBox* box){return collision(data,box->bottom*0.03125f);}

#include "script_state_actual.h"

// New shared-state branches are outside this transition routing fixture.
bool mpColl_8004B3F0(CollData* data,ftCollisionBox* box)
{(void)data;(void)box;abort();}
bool ft_8008239C(Fighter_GObj* object,int facing,ftCollisionBox* box)
{(void)object;(void)facing;(void)box;abort();}
void ftCo_8009B390(Fighter_GObj* object,float multiplier)
{(void)object;(void)multiplier;abort();}
void ftCommon_8007D92C(Fighter_GObj* object){(void)object;abort();}
void ftCo_80097D40(Fighter_GObj* object){(void)object;abort();}

HSD_GObjEvent ftData_OnLanding[Ft_Kind_RuntimeCapacity];
bool ft_80084A18(Fighter_GObj* object){(void)object;return true;}
bool un_803224DC(s32 id,float x,float kb){(void)id;(void)x;(void)kb;return false;}
void pl_8003FC44(int slot,int secondary){(void)slot;(void)secondary;abort();}
#include "script_transition_planes_actual.h"

// These unrelated binding families are absent from this fixture. Any accidental
// execution aborts instead of supplying a successful synthetic implementation.
bool nativeScriptBindingSetup_Invoke(NativeScriptRuntimeContext* c,uint32_t s,
    const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;abort();}
bool nativeScriptBindingCombat_Invoke(NativeScriptRuntimeContext* c,uint32_t s,
    const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;abort();}
bool nativeScriptBindingSetup_Read(NativeScriptRuntimeContext* c,uint32_t s,
    const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;abort();}
bool nativeScriptBindingSetup_Write(NativeScriptRuntimeContext* c,uint32_t s,
    const NativeScriptValue* v,uint32_t n,const NativeScriptValue* r,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;abort();}
bool nativeScriptBindingCombat_Read(NativeScriptRuntimeContext* c,uint32_t s,
    NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)v;(void)d;abort();}
bool nativeScriptBindingCombat_Write(NativeScriptRuntimeContext* c,uint32_t s,
    const NativeScriptValue* v,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)v;(void)d;abort();}
