#include "native_script_binding_common_movement.h"
#include "native_script_bindings.h"
#include "native_callback_lifetime.h"
#include "native_source_package.h"
#include "fighter.h"
#include "ftcommon.h"
#include "ftwalkcommon.h"
#include "ft_084E.h"
#include "ft_0892.h"
#include "types.h"
#include "kinds/ftCommon/ftCo_Pass.h"
#include "kinds/ftCommon/ftCo_Turn.h"
#include "kinds/ftCommon/ftCo_Jump.h"
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>
#include <math.h>

typedef struct MovementSource {
    NativeScriptRuntimeContext* context;
    NativeMatchPoolReference reference;
    Fighter* fighter;
    const NativeSourcePackage* package;
    const NativeScriptFighterProgram* program;
    NativeScriptReceiverView view;
    const NativeScriptStorageParameters* parameters;
    uint64_t program_identity,layout_identity,parameter_identity;
    uint32_t generation;
} MovementSource;

static bool error(NativeScriptDiagnostic* diagnostic,const char* text)
{if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",text);return false;}

static bool current(const MovementSource* source,NativeScriptDiagnostic* diagnostic)
{
    NativeScriptRuntimeContext* c=source->context;NativeScriptReceiverView view;
    if(c->source_class!=NativeCallbackSource_Fighter || c->generation!=source->generation ||
       nativeMatchPool_Resolve(source->reference)!=c->source || source->reference.lifetime!=c->scene_epoch ||
       !nativeScriptBindings_SourceAlive(c) || ((HSD_GObj*)c->source)->user_data!=source->fighter ||
       nativeFighterRuntime_SourcePackage(source->fighter->kind)!=source->package ||
       nativeScriptFighter_Program(source->fighter->kind)!=source->program ||
       source->program->identity!=source->program_identity ||
       nativeScriptBindings_SourceFighterLayout(c,diagnostic)!=&source->program->bank->motion_layout ||
       !nativeScriptFighter_View(c->source,c->generation,&view) || view.program!=source->view.program ||
       view.storage!=source->view.storage || !view.storage || !view.storage->live ||
       view.storage->layout->identity!=source->layout_identity ||
       view.storage->parameters!=source->parameters || !source->parameters ||
       source->parameters->identity!=source->parameter_identity)
        return error(diagnostic,"Common movement source, program or storage retired before continuation.");
    return true;
}

static bool capture(NativeScriptRuntimeContext* context,MovementSource* source,NativeScriptDiagnostic* diagnostic)
{
    if(!context || context->source_class!=NativeCallbackSource_Fighter || !context->scene_epoch ||
       !nativeMatchPool_Reference(&nativeGObjPool,context->source,&source->reference) ||
       source->reference.lifetime!=context->scene_epoch ||
       !nativeScriptBindings_SourceFighterLayout(context,diagnostic))
        return error(diagnostic,"Common movement requires its live bound source Fighter.");
    source->context=context;source->generation=context->generation;
    source->fighter=((HSD_GObj*)context->source)->user_data;
    source->package=nativeFighterRuntime_SourcePackage(source->fighter->kind);
    source->program=nativeScriptFighter_Program(source->fighter->kind);
    if(!source->package || !source->program ||
       !nativeScriptFighter_View(context->source,context->generation,&source->view) ||
       !source->view.storage || !source->view.storage->layout || !source->view.storage->parameters)
        return error(diagnostic,"Common movement owned storage view is absent.");
    source->parameters=source->view.storage->parameters;
    source->program_identity=source->program->identity;
    source->layout_identity=source->view.storage->layout->identity;
    source->parameter_identity=source->parameters->identity;
    return current(source,diagnostic);
}

bool nativeScriptBindingCommonMovement_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* arguments,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    if(!result || count!=(semantic==NativeScriptBinding_WalkEnter?10:semantic==NativeScriptBinding_StatePlatformDrop?3:0))
        return error(diagnostic,"Walk entry takes ten arguments; platform drop takes three; other common movement operations take none.");
    if(semantic!=NativeScriptBinding_FacingFromInputThreshold &&
       semantic!=NativeScriptBinding_PhysicsForceAirAndUseJumps &&
       semantic!=NativeScriptBinding_PhysicsGroundToAir &&
       semantic!=NativeScriptBinding_PhysicsSwitchToGround && semantic!=NativeScriptBinding_CollisionSkipSoftFloor &&
       semantic!=NativeScriptBinding_WalkEnter && semantic!=NativeScriptBinding_WalkUpdateAnimation &&
       semantic!=NativeScriptBinding_PhysicsWalk && semantic!=NativeScriptBinding_PhysicsGroundAnimationFacingMotion &&
       semantic!=NativeScriptBinding_ControlsWalkRequested && semantic!=NativeScriptBinding_ControlsWalkStopRequested &&
       semantic!=NativeScriptBinding_ControlsTurnRequested && semantic!=NativeScriptBinding_ControlsPlatformDropRequested &&
       semantic!=NativeScriptBinding_ControlsJumpInput && semantic!=NativeScriptBinding_StatePlatformDrop)
        return error(diagnostic,"Unknown common movement operation.");
    MovementSource source;if(!capture(context,&source,diagnostic))return false;
    if(semantic==NativeScriptBinding_WalkEnter){
        if(!arguments || arguments[0].type!=NativeScriptType_I32 || arguments[0].as.i64<0 || arguments[0].as.i64>UINT16_MAX-2 ||
           arguments[1].type!=NativeScriptType_U32 || arguments[1].as.u64>UINT32_MAX)
            return error(diagnostic,"Walk entry requires an Int base motion and native-width UInt flags");
        for(unsigned i=2;i<10;++i)
            if(arguments[i].type!=NativeScriptType_F32 || !isfinite(arguments[i].as.f32))
                return error(diagnostic,"Walk frame lengths, reference speeds and acceleration must be finite Floats");
        for(unsigned i=6;i<9;++i)if(arguments[i].as.f32==0)
            return error(diagnostic,"Walk reference speeds must be nonzero for animation-rate division");
        if(!p_ftCommonData || !isfinite(p_ftCommonData->walk_fast_stick_threshold) ||
           !isfinite(p_ftCommonData->walk_middle_animation_stick_threshold) ||
           !isfinite(source.fighter->gr_vel) || !isfinite(source.fighter->co_attrs.walk_max_vel))
            return error(diagnostic,"Walk entry requires finite native selection thresholds and velocity");
    }else if(semantic==NativeScriptBinding_StatePlatformDrop){
        if(!arguments || arguments[0].type!=NativeScriptType_I32 || arguments[0].as.i64<0 || arguments[0].as.i64>UINT16_MAX ||
           arguments[1].type!=NativeScriptType_U32 || arguments[1].as.u64>UINT32_MAX ||
           arguments[2].type!=NativeScriptType_F32 || !isfinite(arguments[2].as.f32) || !p_ftCommonData)
            return error(diagnostic,"Platform drop requires an admitted motion Int, native-width flags UInt, finite frame Float and common data");
    }else if(semantic==NativeScriptBinding_WalkUpdateAnimation){
        int64_t walk=(int64_t)source.fighter->motion_id-source.fighter->mv.co.walk.msid;
        if(walk<0 || walk>2)return error(diagnostic,"Walk animation requires a current slow, middle or fast walk motion");
        float speed=walk==0?source.fighter->mv.co.walk.slow_anim_rate:
            walk==1?source.fighter->mv.co.walk.middle_anim_rate:source.fighter->mv.co.walk.fast_anim_rate;
        if(!isfinite(speed) || speed==0 || !isfinite(source.fighter->gr_vel) ||
           !isfinite(source.fighter->mv.co.walk.x0) || !isfinite(source.fighter->facing_dir))
            return error(diagnostic,"Walk animation requires finite velocity/facing and a nonzero reference speed");
    }else if((semantic==NativeScriptBinding_PhysicsWalk || semantic==NativeScriptBinding_ControlsWalkRequested ||
              semantic==NativeScriptBinding_ControlsWalkStopRequested || semantic==NativeScriptBinding_ControlsTurnRequested ||
              semantic==NativeScriptBinding_ControlsPlatformDropRequested || semantic==NativeScriptBinding_ControlsJumpInput) && !p_ftCommonData)
        return error(diagnostic,"Common movement/input data is unavailable");
    if(semantic==NativeScriptBinding_WalkEnter || semantic==NativeScriptBinding_StatePlatformDrop){
        const NativeScriptMoveLayout* layout=&source.program->bank->motion_layout;
        if(source.fighter->x18!=(s32)layout->first || !source.package->assets.common_motions ||
           source.fighter->x1C_actionStateList!=source.package->assets.common_motions ||
           source.fighter->x20_actionStateList!=source.package->assets.motions)
            return error(diagnostic,"Movement entry motion tables differ from the owned fighter assets");
        unsigned motions=semantic==NativeScriptBinding_WalkEnter?3:1;
        for(unsigned i=0;i<motions;++i)if(!nativeScriptMoves_Contains(layout,(uint32_t)arguments[0].as.i64+i))
            return error(diagnostic,"Movement entry requires admitted consecutive motions");
    }
    NativeScriptValue value={.type=NativeScriptType_Void};
    switch(semantic) {
    case NativeScriptBinding_StatePlatformDrop:
        ftCo_8009A184(context->source,(FtMotionId)arguments[0].as.i64,(MotionFlags)arguments[1].as.u64,arguments[2].as.f32);break;
    case NativeScriptBinding_ControlsWalkRequested:value.type=NativeScriptType_Bool;value.as.u64=ftWalkCommon_800DFC70(context->source);break;
    case NativeScriptBinding_ControlsWalkStopRequested:value.type=NativeScriptType_Bool;value.as.u64=ft_8008A1FC(context->source);break;
    case NativeScriptBinding_ControlsTurnRequested:value.type=NativeScriptType_Bool;value.as.u64=ftCo_800C97A8(context->source);break;
    case NativeScriptBinding_ControlsPlatformDropRequested:value.type=NativeScriptType_Bool;value.as.u64=ftCo_80099F1C(context->source);break;
    case NativeScriptBinding_ControlsJumpInput:value.type=NativeScriptType_I32;value.as.i64=ftCo_Jump_GetInput(context->source);break;
    case NativeScriptBinding_WalkEnter:
        ftWalkCommon_800DFCA4(context->source,(FtMotionId)arguments[0].as.i64,(MotionFlags)arguments[1].as.u64,
            arguments[2].as.f32,arguments[3].as.f32,arguments[4].as.f32,arguments[5].as.f32,
            arguments[6].as.f32,arguments[7].as.f32,arguments[8].as.f32,arguments[9].as.f32);break;
    case NativeScriptBinding_WalkUpdateAnimation:ftWalkCommon_800DFDDC(context->source);break;
    case NativeScriptBinding_PhysicsWalk:ftWalkCommon_800E0060(context->source);break;
    case NativeScriptBinding_PhysicsGroundAnimationFacingMotion:ft_800850B4(context->source);break;
    case NativeScriptBinding_FacingFromInputThreshold:
        if(!p_ftCommonData)return error(diagnostic,"Common facing threshold data is unavailable.");
        ftCommon_8007DA24(source.fighter);break;
    case NativeScriptBinding_PhysicsForceAirAndUseJumps:
        ftCommon_8007D60C(source.fighter);break;
    case NativeScriptBinding_PhysicsGroundToAir:
        ftCommon_8007D5D4(source.fighter);break;
    case NativeScriptBinding_PhysicsSwitchToGround:
        ftCommon_8007D7FC(source.fighter);break;
    case NativeScriptBinding_CollisionSkipSoftFloor:
        value.type=NativeScriptType_Bool;value.as.u64=ftCo_8009A134(context->source);break;
    default:return false;
    }
    if(!current(&source,diagnostic))return false;
    *result=value;return true;
}
