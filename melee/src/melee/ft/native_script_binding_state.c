#include "native_script_bindings.h"
#include "native_script_expression.h"
#include "native_callback_lifetime.h"
#include "native_fighter_lifetime.h"
#include "native_fighter_runtime.h"
#include "native_source_package.h"
#include "ft_081B.h"
#include "ft_0892.h"
#include "ftcommon.h"
#include "kinds/ftCommon/ftCo_Fall.h"
#include "kinds/ftCommon/ftCo_AppealS.h"
#include "kinds/ftCommon/ftCo_FallAerial.h"
#include "kinds/ftCommon/ftCo_FallSpecial.h"
#include "kinds/ftCommon/ftCo_Landing.h"
#include "kinds/ftCommon/ftCo_LandingAir.h"
#include "kinds/ftCommon/ftCo_DamageFall.h"
#include "kinds/ftCommon/ftCo_DownBound.h"
#include "kinds/ftCommon/ftCo_CliffJump.h"
#include "types.h"
#include <melee/mp/forward.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/controller.h>
#include <stdio.h>
#include <math.h>

static bool error(NativeScriptDiagnostic* diagnostic,const char* message)
{ if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",message);return false; }

static bool taunt_motion(const NativeScriptValue* value,const NativeScriptMoveLayout* layout,
    uint32_t* result,NativeScriptDiagnostic* diagnostic)
{
    uint64_t motion;
    if(!value || !nativeScriptExpression_IsIntegral(value->type))
        return error(diagnostic,"Taunt motion must be integral.");
    if(value->type==NativeScriptType_I32 || value->type==NativeScriptType_I64) {
        if(value->as.i64<0)return error(diagnostic,"Taunt motion is negative.");
        motion=(uint64_t)value->as.i64;
    } else motion=value->as.u64;
    if(motion>UINT16_MAX || !nativeScriptMoves_Contains(layout,(uint32_t)motion))
        return error(diagnostic,"Taunt motion is outside the selected source motion table.");
    *result=(uint32_t)motion;return true;
}

bool nativeScriptBindingState_Read(NativeScriptRuntimeContext* context,uint32_t semantic,
    NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    if(semantic==NativeScriptBinding_CollisionLanded && (!context || !result ||
       context->source_class!=NativeCallbackSource_Fighter || !nativeScriptBindings_SourceAlive(context)))
        return error(diagnostic,"Air contact requires its live Fighter source");
    Fighter* fp=((HSD_GObj*)context->source)->user_data;
    if(semantic==NativeScriptBinding_CollisionLanded) {
        NativeFighterLifetime lifetime;
        if(!result || !nativeFighterLifetime_Capture(fp,&lifetime) ||
           context->source!=lifetime.object || context->generation!=lifetime.generation ||
           context->scene_epoch!=lifetime.reference.lifetime ||
           (lifetime.source_owned && nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=
                &lifetime.program->bank->motion_layout))
            return error(diagnostic,"Air contact requires its exact live Fighter source");
        bool landed=ft_80081D0C(context->source)!=GA_Ground;
        if(context->source!=lifetime.object || context->generation!=lifetime.generation ||
           context->scene_epoch!=lifetime.reference.lifetime || !nativeFighterLifetime_Current(&lifetime) ||
           (lifetime.source_owned && nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=
                &lifetime.program->bank->motion_layout))
            return error(diagnostic,"Air contact source changed before publication");
        *result=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=landed};return true;
    }
    uint32_t held=fp->input.held_buttons[0],pressed=fp->input.pressed_buttons,flags=fp->coll_data.env_flags;
    switch(semantic) {
    case NativeScriptBinding_InputAttackHeld:result->as.u64=(held&HSD_PAD_A)!=0;break;
    case NativeScriptBinding_InputAttackPressed:result->as.u64=(pressed&HSD_PAD_A)!=0;break;
    case NativeScriptBinding_InputJumpHeld:result->as.u64=(held&HSD_PAD_XY)!=0;break;
    case NativeScriptBinding_InputShieldHeld:result->as.u64=(held&HSD_PAD_LR)!=0;break;
    case NativeScriptBinding_InputShieldPressed:result->as.u64=(pressed&HSD_PAD_LR)!=0;break;
    case NativeScriptBinding_InputUpHeld:result->as.u64=(held&HSD_PAD_DPADUP)!=0;break;
    case NativeScriptBinding_InputDownHeld:result->as.u64=(held&HSD_PAD_DPADDOWN)!=0;break;
    case NativeScriptBinding_InputZHeld:result->as.u64=(held&HSD_PAD_Z)!=0;break;
    case NativeScriptBinding_InputLHeld:result->as.u64=(held&HSD_PAD_L)!=0;break;
    case NativeScriptBinding_InputRHeld:result->as.u64=(held&HSD_PAD_R)!=0;break;
    case NativeScriptBinding_InputAttackOrSpecialPressed:result->as.u64=(pressed&HSD_PAD_AB)!=0;break;
    case NativeScriptBinding_InputPreviousX:result->as.f32=fp->input.lstick[1].x;break;
    case NativeScriptBinding_InputPreviousY:result->as.f32=fp->input.lstick[1].y;break;
    case NativeScriptBinding_InputHorizontalSmashAge:result->as.i64=fp->x673;break;
    case NativeScriptBinding_ControlsInterruptAllowed:result->as.u64=fp->allow_interrupt!=0;break;
    case NativeScriptBinding_CollisionFloorContact:result->as.u64=(flags&Collide_FloorMask)!=0;break;
    case NativeScriptBinding_CollisionCeilingContact:result->as.u64=(flags&Collide_CeilingMask)!=0;break;
    case NativeScriptBinding_CollisionLeftWall:result->as.u64=(flags&Collide_LeftWallMask)!=0;break;
    case NativeScriptBinding_CollisionRightWall:result->as.u64=(flags&Collide_RightWallMask)!=0;break;
    case NativeScriptBinding_CollisionWallContact:result->as.u64=(flags&(Collide_LeftWallMask|Collide_RightWallMask))!=0;break;
    case NativeScriptBinding_CollisionWallAhead:result->as.u64=(fp->facing_dir==1&&(flags&Collide_LeftWallMask))||(fp->facing_dir==-1&&(flags&Collide_RightWallMask));break;
    case NativeScriptBinding_CollisionWallHugAhead:result->as.u64=(fp->facing_dir==1&&(flags&Collide_LeftWallHug))||(fp->facing_dir==-1&&(flags&Collide_RightWallHug));break;
    case NativeScriptBinding_CollisionLeftGround:result->as.u64=!ft_80082708(context->source);break;
    case NativeScriptBinding_CollisionSpecialGroundLost:result->as.u64=!ft_800827A0(context->source);break;
    case NativeScriptBinding_FloorLine:result->as.i64=fp->coll_data.floor.index;break;
    case NativeScriptBinding_FloorNormalX:result->as.f32=fp->coll_data.floor.normal.x;break;
    case NativeScriptBinding_FloorNormalY:result->as.f32=fp->coll_data.floor.normal.y;break;
    case NativeScriptBinding_FloorNormalZ:result->as.f32=fp->coll_data.floor.normal.z;break;
    default:return error(diagnostic,"Unsupported state/control/collision property.");
    }
    return true;
}

bool nativeScriptBindingState_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* arguments,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    (void)count;
    if(!context || context->source_class!=NativeCallbackSource_Fighter ||
       !nativeScriptBindings_SourceAlive(context))return error(diagnostic,"State source is unavailable or retired.");
    HSD_GObj* object=context->source;Fighter* fp=object->user_data;
    const NativeSourcePackage* source=nativeFighterRuntime_SourcePackage(fp->kind);
    const NativeScriptMoveLayout* layout=source?nativeScriptBindings_SourceFighterLayout(context,diagnostic):NULL;
    if(source && !layout)return false;
    if(semantic==NativeScriptBinding_CollisionGroundBox||semantic==NativeScriptBinding_CollisionGroundBoxEdges||semantic==NativeScriptBinding_CollisionAirBox||semantic==NativeScriptBinding_CollisionAirBoxLedge) {
        NativeFighterLifetime saved;
        bool ledge=semantic==NativeScriptBinding_CollisionAirBoxLedge;
        unsigned start=ledge?1:0;
        if(!result || !arguments || count!=6+start)
            return error(diagnostic,"Collision box requires six Float coordinates and a result destination.");
        if(ledge && (arguments[0].type!=NativeScriptType_I32 || arguments[0].as.i64<INT32_MIN || arguments[0].as.i64>INT32_MAX))
            return error(diagnostic,"Collision-box ledge direction must be a native signed Int.");
        for(unsigned i=start;i<count;++i)
            if(arguments[i].type!=NativeScriptType_F32 || !isfinite(arguments[i].as.f32))
                return error(diagnostic,"Collision box coordinates must be finite Float values.");
        if(!nativeFighterLifetime_Capture(fp,&saved) || saved.object!=object ||
           saved.generation!=context->generation || saved.reference.lifetime!=context->scene_epoch)
            return error(diagnostic,"Collision box requires its exact live Fighter source.");
        const NativeScriptValue* coordinates=arguments+start;
        ftCollisionBox box={coordinates[0].as.f32,coordinates[1].as.f32,
            {coordinates[2].as.f32,coordinates[3].as.f32},{coordinates[4].as.f32,coordinates[5].as.f32}};
        bool contact;
        if(semantic==NativeScriptBinding_CollisionGroundBox)contact=ft_80082888(object,&box);
        else if(semantic==NativeScriptBinding_CollisionGroundBoxEdges)contact=ft_80082978(object,&box);
        else if(ledge)contact=ft_8008239C(object,(int)arguments[0].as.i64,&box);
        else contact=ft_800824A0(object,&box);
        if(context->source!=saved.object || context->generation!=saved.generation ||
           context->scene_epoch!=saved.reference.lifetime || !nativeFighterLifetime_Current(&saved) ||
           (source && nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=layout))
            return error(diagnostic,"Collision box source retired or faulted during its query.");
        *result=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=contact};return true;
    }
    if(!layout)layout=nativeScriptBindings_SourceFighterLayout(context,diagnostic);
    if(!layout)return false;
    if(semantic==NativeScriptBinding_StateDamageFall || semantic==NativeScriptBinding_StateCliffJumpPhase2 || semantic==NativeScriptBinding_StateFinish || semantic==NativeScriptBinding_StateDownBound) {
        bool jump=semantic==NativeScriptBinding_StateCliffJumpPhase2;
        if(!result || count!=(jump?1:0) ||
           (jump && (!arguments || arguments[0].type!=NativeScriptType_F32 || !isfinite(arguments[0].as.f32))))
            return error(diagnostic,"Finish and damage fall take no arguments; cliff jump phase 2 requires one finite Float");
        NativeFighterLifetime saved;
        if(!nativeFighterLifetime_Capture(fp,&saved) || !saved.source_owned ||
           saved.object!=context->source || saved.generation!=context->generation ||
           saved.reference.lifetime!=context->scene_epoch ||
           layout!=&saved.program->bank->motion_layout)
            return error(diagnostic,"Common state entry requires its exact owned Fighter program/storage");
        if(jump)ftCo_8009B390(object,arguments[0].as.f32);
        else if(semantic==NativeScriptBinding_StateFinish)ftCommon_8007D92C(object);
        else if(semantic==NativeScriptBinding_StateDownBound)ftCo_80097D40(object);
        else ftCo_80090780(object);
        if(context->source!=saved.object || context->generation!=saved.generation ||
           context->scene_epoch!=saved.reference.lifetime || !nativeFighterLifetime_Current(&saved) ||
           nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=&saved.program->bank->motion_layout)
            return error(diagnostic,"Common state source changed during native continuation");
        *result=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    if(semantic==NativeScriptBinding_StateTaunt) {
        uint32_t right,left;NativeFighterLifetime saved;
        if(!result || !arguments || count!=2)
            return error(diagnostic,"Taunt requires two motion arguments and a result destination.");
        if(!taunt_motion(&arguments[0],layout,&right,diagnostic) ||
           !taunt_motion(&arguments[1],layout,&left,diagnostic))return false;
        if(!source || (layout->common_count&&!source->assets.common_motions) ||
           (layout->count&&!source->assets.motions) || fp->x18!=(s32)layout->first ||
           fp->x1C_actionStateList!=source->assets.common_motions ||
           fp->x20_actionStateList!=source->assets.motions ||
           !fp->x24 || fp->x24!=source->assets.character.data.xC)
            return error(diagnostic,"Taunt source motion/animation tables differ from owned assets.");
        const MotionState* motion=left>=layout->first?&source->assets.motions[left-layout->first]:&source->assets.common_motions[left];
        if(motion->anim_id<0 || (uint32_t)motion->anim_id>=source->assets.facts.actions)
            return error(diagnostic,"Taunt left animation is outside the owned animation table.");
        if(!nativeFighterLifetime_Capture(fp,&saved))
            return error(diagnostic,"Taunt source identity is unavailable.");
        // The common primitive selects LEFT only for facing -1 and a nonzero
        // left animation x8; every other facing value selects RIGHT.
        ftCo_800DEAE8(object,(FtMotionId)right,(FtMotionId)left);
        if(!nativeFighterLifetime_Current(&saved) ||
           !nativeScriptBindings_SourceFighterLayout(context,diagnostic))
            return error(diagnostic,"Taunt source retired or changed during motion entry.");
        *result=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    // Neutral entry can run both entry and cleanup policies. Retain its actual
    // actor/program/storage identity across the complete native continuation.
    NativeFighterLifetime neutral_lifetime;
    bool neutral=semantic==NativeScriptBinding_StateWait || semantic==NativeScriptBinding_StateWaitBlend;
    if(neutral && !nativeFighterLifetime_Capture(fp,&neutral_lifetime))
        return error(diagnostic,"Neutral source identity is unavailable.");
    if((semantic==NativeScriptBinding_StateLanding || semantic==NativeScriptBinding_StateLandingAerial) &&
       (arguments[0].as.i64<0 || arguments[0].as.i64>UINT16_MAX ||
        !nativeScriptMoves_Contains(layout,(uint32_t)arguments[0].as.i64)))
        return error(diagnostic,"Landing target is outside the selected source motion table.");
    switch(semantic) {
    case NativeScriptBinding_StateWait:case NativeScriptBinding_StateWaitBlend:
        ft_8008A348(object,semantic==NativeScriptBinding_StateWait?0:arguments[0].as.f32);break;
    case NativeScriptBinding_StateFall:ftCo_Fall_Enter(object);break;
    case NativeScriptBinding_StateFallAerial:ftCo_FallAerial_Enter(object);break;
    case NativeScriptBinding_StateFreefall:ftCo_80096900(object,(int)arguments[0].as.i64,(int)arguments[1].as.i64,arguments[2].as.u64!=0,arguments[3].as.f32,arguments[4].as.f32);break;
    case NativeScriptBinding_StateFreefallBlend:ftCo_800969D8(object,(int)arguments[0].as.i64,(int)arguments[1].as.i64,(int)arguments[2].as.i64,arguments[3].as.f32,arguments[4].as.f32,arguments[5].as.f32);break;
    case NativeScriptBinding_StateLanding:ftCo_Landing_Enter(object,(FtMotionId)arguments[0].as.i64,arguments[1].as.u64!=0,(MotionFlags)arguments[2].as.u64,arguments[3].as.f32,arguments[4].as.f32);break;
    case NativeScriptBinding_StateBasicLanding:ftCo_Landing_Enter_Basic(object);break;
    case NativeScriptBinding_StateLandingSpecial:case NativeScriptBinding_StateLandingFallSpecial:ftCo_LandingFallSpecial_Enter(object,arguments[0].as.u64!=0,arguments[1].as.f32);break;
    case NativeScriptBinding_StateLandingAerial:ftCo_LandingAir_EnterWithMsidLag(object,(FtMotionId)arguments[0].as.i64,arguments[1].as.f32);break;
    default:return error(diagnostic,"Unsupported general state operation.");
    }
    if(!nativeScriptBindings_SourceAlive(context) || object->user_data!=fp ||
       (neutral && !nativeFighterLifetime_Current(&neutral_lifetime)) ||
       !nativeScriptBindings_SourceFighterLayout(context,diagnostic))
        return error(diagnostic,"State source retired or faulted during its engine continuation.");
    return true;
}
