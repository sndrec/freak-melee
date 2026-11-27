#include "native_script_bindings.h"
#include "native_callback_lifetime.h"
#include "native_fighter_runtime.h"
#include "native_source_package.h"
#include "ftcommon.h"
#include "ft_081B.h"
#include "ft_084E.h"
#include "ftaction.h"
#include "ftanim.h"
#include "fighter.h"
#include "kinds/ftCommon/ftCo_Attack100.h"
#include <melee/lb/lbanim.h>
#include <melee/lb/lbvector.h>
#include <sysdolphin/baselib/gobj.h>

#include <stdio.h>

static bool error(NativeScriptDiagnostic* diagnostic,const char* message)
{
    if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",message);
    return false;
}

bool nativeScriptBindingKinetics_Read(NativeScriptRuntimeContext* context,uint32_t semantic,
    NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    Fighter* fighter=((HSD_GObj*)context->source)->user_data;
    switch(semantic) {
    case NativeScriptBinding_AnimationFinished:result->as.u64=!ftAnim_IsFramesRemaining(context->source);break;
    case NativeScriptBinding_AnimationRate:result->as.f32=fighter->frame_speed_mul;break;
    case NativeScriptBinding_AnimationID:result->as.i64=fighter->x5F4_arr[0].idx;break;
    case NativeScriptBinding_AnimationLength:result->as.f32=ftAnim_8006F484(context->source);break;
    case NativeScriptBinding_AnimationDeltaX:result->as.f32=fighter->x6A4_transNOffset.x;break;
    case NativeScriptBinding_AnimationDeltaY:result->as.f32=fighter->x6A4_transNOffset.y;break;
    case NativeScriptBinding_AnimationDeltaZ:result->as.f32=fighter->x6A4_transNOffset.z;break;
    case NativeScriptBinding_MotionID:result->as.i64=fighter->motion_id;break;
    case NativeScriptBinding_MotionLaunchAngle:result->as.f32=fighter->lstick_angle;break;
    case NativeScriptBinding_GroundVelocity:result->as.f32=fighter->gr_vel;break;
    case NativeScriptBinding_GroundAcceleration:result->as.f32=fighter->xE4_ground_accel_1;break;
    case NativeScriptBinding_GroundFriction:result->as.f32=fighter->co_attrs.ground_friction;break;
    case NativeScriptBinding_AirDriftMax:result->as.f32=fighter->co_attrs.air_drift_max;break;
    case NativeScriptBinding_AirDriftStickMultiplier:result->as.f32=fighter->co_attrs.air_drift_stick_mul;break;
    default:return error(diagnostic,"Unsupported kinetic property.");
    }
    return true;
}

bool nativeScriptBindingKinetics_Write(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* value,NativeScriptDiagnostic* diagnostic)
{
    Fighter* fighter=((HSD_GObj*)context->source)->user_data;
    switch(semantic) {
    case NativeScriptBinding_AnimationDeltaX:fighter->x6A4_transNOffset.x=value->as.f32;break;
    case NativeScriptBinding_AnimationDeltaY:fighter->x6A4_transNOffset.y=value->as.f32;break;
    case NativeScriptBinding_AnimationDeltaZ:fighter->x6A4_transNOffset.z=value->as.f32;break;
    case NativeScriptBinding_MotionLaunchAngle:fighter->lstick_angle=value->as.f32;break;
    case NativeScriptBinding_GroundVelocity:fighter->gr_vel=value->as.f32;break;
    case NativeScriptBinding_GroundAcceleration:fighter->xE4_ground_accel_1=value->as.f32;break;
    default:return error(diagnostic,"Unsupported kinetic property write.");
    }
    return true;
}

static bool compound(float* destination,const NativeScriptValue* argument,
    NativeScriptIROperation operation,NativeScriptDiagnostic* diagnostic)
{
    NativeScriptValue converted;
    NativeScriptIRType type=argument->type==NativeScriptType_F64?NativeScriptType_F64:NativeScriptType_F32;
    if(!nativeScriptValue_Convert(argument,type,&converted,diagnostic))return false;
    // These native field operations preserve the original C floating arithmetic,
    // including IEEE zero-divisor behavior. General VM arithmetic is separate.
    if(type==NativeScriptType_F64) {
        double left=*destination,right=converted.as.f64;
        *destination=(float)(operation==NativeScriptOp_Add?left+right:
            operation==NativeScriptOp_Multiply?left*right:left/right);
    } else {
        float left=*destination,right=converted.as.f32;
        *destination=operation==NativeScriptOp_Add?left+right:
            operation==NativeScriptOp_Multiply?left*right:left/right;
    }
    return true;
}

bool nativeScriptBindingKinetics_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* arguments,uint32_t count,NativeScriptValue* result,
    NativeScriptDiagnostic* diagnostic)
{
    if(semantic==NativeScriptBinding_AnimationClipLength) {
        if(!context || !arguments || !result || count!=1 ||
           context->source_class!=NativeCallbackSource_Fighter ||
           !nativeScriptBindings_SourceAlive(context) ||
           !nativeScriptBindings_SourceFighterLayout(context,diagnostic) ||
           !nativeScriptExpression_IsIntegral(arguments[0].type))
            return error(diagnostic,"Clip length requires its owned source Fighter and an integral index.");
        uint64_t index;
        if(arguments[0].type==NativeScriptType_I32 || arguments[0].type==NativeScriptType_I64) {
            if(arguments[0].as.i64<0)return error(diagnostic,"Animation clip index is negative.");
            index=(uint64_t)arguments[0].as.i64;
        } else index=arguments[0].as.u64;
        const Fighter* fighter=((const HSD_GObj*)context->source)->user_data;
        const NativeSourcePackage* source=nativeFighterRuntime_SourcePackage(fighter->kind);
        bool gameplay=source && fighter->x24==source->assets.character.data.xC &&
            fighter->x58C==source->assets.facts.actions;
        bool demo=source && fighter->x24==source->assets.character.data.x14 &&
            fighter->x58C==source->assets.facts.demo_count;
        if(!fighter->x24 || (!gameplay && !demo) || index>UINT16_MAX)
            return error(diagnostic,"Animation clip index or table is outside the selected owned assets.");
        // ftData_80085E50 reads the actor's active table, including demo actors
        // running the same Load callback. Out-of-table IDs yield NULL, whose
        // lbAnim_8001E8F8 length is zero; never substitute the gameplay bank.
        const FigaTree* tree=index<(uint32_t)fighter->x58C?fighter->x24[index].x14:NULL;
        *result=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=tree?tree->frames:0};
        return true;
    }
    (void)count;
    HSD_GObj* object=context->source;
    Fighter* fighter=object->user_data;
    bool guarded=(semantic==NativeScriptBinding_AnimationStart ||
        semantic==NativeScriptBinding_PhysicsAnimationVelocity) &&
        nativeFighterRuntime_SourcePackage(fighter->kind)!=NULL;
    if(guarded && !nativeScriptBindings_SourceFighterLayout(context,diagnostic))return false;
    switch(semantic) {
    case NativeScriptBinding_AnimationStart: {
        // Original ftAnim_8006EBA4 order, with continuation barriers between
        // stages that can execute subaction/animation callbacks.
        ftAnim_8006E9B4(object);
        if(!nativeScriptBindings_SourceAlive(context) || (guarded && !nativeScriptBindings_SourceFighterLayout(context,diagnostic)))return error(diagnostic,"Animation source retired during pose update.");
        const u32 motion_sequence = fighter->native_motion_sequence;
        ftAction_80073240(object);
        if(!nativeScriptBindings_SourceAlive(context) || (guarded && !nativeScriptBindings_SourceFighterLayout(context,diagnostic)))return error(diagnostic,"Animation source retired during subactions.");
        if (fighter->native_motion_sequence != motion_sequence) break;
        ftAnim_800707B0(object);
        if(!nativeScriptBindings_SourceAlive(context) || (guarded && !nativeScriptBindings_SourceFighterLayout(context,diagnostic)))return error(diagnostic,"Animation source retired during material update.");
        ftCo_800DB500(object);break;
    }
    case NativeScriptBinding_AnimationSetRate:ftAnim_SetAnimRate(object,arguments[0].as.f32);break;
    case NativeScriptBinding_AnimationSetTimer:fighter->cur_anim_frame=arguments[0].as.f32;break;
    case NativeScriptBinding_AnimationClearMotionSamples:
        fighter->x6D8=(Vec3){0};fighter->x6C0=(Vec3){0};fighter->lstick_angle=0;
        fighter->x6A4_transNOffset=(Vec3){0};fighter->x68C_transNPos=(Vec3){0};break;
    case NativeScriptBinding_MotionSetVerticalSpeed:fighter->self_vel.y=arguments[0].as.f32;break;
    case NativeScriptBinding_MotionAddVerticalSpeed:return compound(&fighter->self_vel.y,&arguments[0],NativeScriptOp_Add,diagnostic);
    case NativeScriptBinding_MotionScaleHorizontalSpeed:return compound(&fighter->self_vel.x,&arguments[0],NativeScriptOp_Multiply,diagnostic);
    case NativeScriptBinding_MotionDivideHorizontalSpeed:return compound(&fighter->self_vel.x,&arguments[0],NativeScriptOp_Divide,diagnostic);
    case NativeScriptBinding_MotionScaleVelocity:
        fighter->self_vel.x*=arguments[0].as.f32;fighter->self_vel.y*=arguments[0].as.f32;fighter->self_vel.z*=arguments[0].as.f32;break;
    case NativeScriptBinding_MotionScalePlanarVelocity:
        fighter->self_vel.x*=arguments[0].as.f32;fighter->self_vel.y*=arguments[0].as.f32;break;
    case NativeScriptBinding_PhysicsClearKinetics:ftCommon_8007E2FC(object);break;
    case NativeScriptBinding_PhysicsGroundFriction:ft_80084F3C(object);break;
    case NativeScriptBinding_PhysicsAirDrift:ft_80084DB0(object);break;
    case NativeScriptBinding_PhysicsGroundMotion:ft_80084FA8(object);break;
    case NativeScriptBinding_PhysicsAirMotion:ft_80085154(object);break;
    case NativeScriptBinding_PhysicsAirGravityAndFriction:ft_80084EEC(object);break;
    case NativeScriptBinding_PhysicsClampAirDrift:ftCommon_ClampAirDrift(fighter);break;
    case NativeScriptBinding_PhysicsQuickAirDecel:result->as.u64=ftCommon_CalcSelfAccel_DeaccelQuickAir(fighter);break;
    case NativeScriptBinding_PhysicsFallBasic:ftCommon_FallBasic(fighter);break;
    case NativeScriptBinding_PhysicsGroundToSelf:ftCommon_SetSelfMovementFromGroundedMovement(object);break;
    case NativeScriptBinding_PhysicsAscend:ftCommon_Ascend(fighter,arguments[0].as.f32,arguments[1].as.f32);break;
    case NativeScriptBinding_PhysicsGroundAccel:ftCommon_CalcGroundAccel_AccelToLStickX(fighter,0,arguments[0].as.f32,arguments[1].as.f32);break;
    case NativeScriptBinding_PhysicsAirAccel:ftCommon_CalcSelfAccel_DriftSimple_NoFriction(fighter,0,arguments[0].as.f32,arguments[1].as.f32);break;
    case NativeScriptBinding_PhysicsClampHorizontalSpeed:ftCommon_ClampSelfVelX(fighter,arguments[0].as.f32);break;
    case NativeScriptBinding_PhysicsClampGroundSpeed:ftCommon_ClampGroundVel(fighter,arguments[0].as.f32);break;
    case NativeScriptBinding_PhysicsClampFallSpeed:ftCommon_ClampFallSpeed(fighter,arguments[0].as.f32);break;
    case NativeScriptBinding_PhysicsAirDecel:ftCommon_CalcSelfAccel_Deaccel(fighter,arguments[0].as.f32);break;
    case NativeScriptBinding_PhysicsRotateVelocityUnitAxis: {
        Vec3 axis={arguments[0].as.f32,arguments[1].as.f32,arguments[2].as.f32};
        lbVector_RotateAboutUnitAxis(&fighter->self_vel,&axis,arguments[3].as.f32);break;
    }
    case NativeScriptBinding_PhysicsMirrorVelocity: {
        Vec3 normal={arguments[0].as.f32,arguments[1].as.f32,arguments[2].as.f32};
        lbVector_Mirror(&fighter->self_vel,&normal);break;
    }
    case NativeScriptBinding_PhysicsFall:ftCommon_Fall(fighter,arguments[0].as.f32,fighter->co_attrs.terminal_velocity);break;
    case NativeScriptBinding_PhysicsFallWithTerminal:ftCommon_Fall(fighter,arguments[0].as.f32,arguments[1].as.f32);break;
    case NativeScriptBinding_PhysicsAirDecelNormal:ftCommon_CalcSelfAccel_DeaccelAir(fighter);break;
    case NativeScriptBinding_PhysicsGroundDecelerate:ftCommon_CalcGroundAccel_Deaccel(fighter,arguments[0].as.f32);break;
    case NativeScriptBinding_PhysicsAirAccelCommon:ftCommon_CalcSelfAccel_DriftSimple_NoFriction(fighter,p_ftCommonData->x258,arguments[0].as.f32,arguments[1].as.f32);break;
    case NativeScriptBinding_PhysicsAirAccelFriction:ftCommon_CalcSelfAccel_DriftSimple(fighter,arguments[0].as.f32,arguments[1].as.f32,arguments[2].as.f32);break;
    case NativeScriptBinding_PhysicsAirAccelNormal:ftCommon_CalcSelfAccel_Drift(fighter);break;
    case NativeScriptBinding_PhysicsAirAccelFrom:ftCommon_CalcSelfAccel_DriftFrom(fighter,arguments[0].as.f32);break;
    case NativeScriptBinding_PhysicsQuickDecel:result->as.u64=ftCommon_CalcSelfAccel_DeaccelQuick(fighter,arguments[0].as.f32);break;
    case NativeScriptBinding_PhysicsAnimationVelocity:
        ft_800851C0(object);
        if(!nativeScriptBindings_SourceAlive(context) || (guarded && !nativeScriptBindings_SourceFighterLayout(context,diagnostic)))return error(diagnostic,"Animation-velocity source retired between axes.");
        ft_80085134(object);break;
    case NativeScriptBinding_PhysicsAnimationVelocityPlain:ft_80085134(object);break;
    case NativeScriptBinding_PhysicsGroundAnimationVelocity:ft_80085088(object);break;
    case NativeScriptBinding_PhysicsFastfall:ftCommon_FallFast(fighter);break;
    case NativeScriptBinding_PhysicsCheckFastfall:result->as.u64=ftCommon_CheckFallFast(fighter);break;
    default:return error(diagnostic,"Unsupported fighter kinetic operation.");
    }
    if(guarded && !nativeScriptBindings_SourceFighterLayout(context,diagnostic))return false;
    return true;
}
