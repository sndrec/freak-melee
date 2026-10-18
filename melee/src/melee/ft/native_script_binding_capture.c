#include "native_script_binding_capture.h"
#include "native_fighter_lifetime.h"
#include "native_fighter_capture.h"
#include "native_fighter_capture_rig.h"
#include "native_source_package.h"
#include "fighter.h"
#include "ftcommon.h"
#include "native_capture.h"
#include "ftparts.h"
#include "ftcoll.h"
#include "kinds/ftCommon/ftCo_Attack100.h"
#include "kinds/ftCommon/ftCo_CaptureDamageKoopa.h"
#include "kinds/ftCommon/ftCo_CaptureKoopa.h"
#include "kinds/ftCommon/ftCo_Shouldered.h"
#include "kinds/ftCommon/ftCo_Thrown.h"
#include "kinds/ftCommon/ftCo_ThrownKoopa.h"
#include "kinds/ftCommon/ftCo_Throw.h"
#include "kinds/ftCommon/ftCo_DamageFall.h"
#include "kinds/ftCommon/ftCo_Fall.h"
#include <melee/it/native_capture_item.h>
#include <math.h>
#include <stdio.h>

static bool error(NativeScriptDiagnostic* diagnostic,const char* message)
{if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",message);return false;}

static bool retain_source(NativeScriptRuntimeContext* context,NativeFighterLifetime* saved,
    NativeScriptDiagnostic* diagnostic)
{
    NativeMatchPoolReference physical;
    if(!context || context->source_class!=NativeCallbackSource_Fighter || !context->scene_epoch ||
       !nativeMatchPool_Reference(&nativeGObjPool,context->source,&physical) ||
       physical.lifetime!=context->scene_epoch ||
       ((HSD_GObj*)context->source)->classifier!=HSD_GOBJ_CLASS_FIGHTER ||
       !nativeScriptBindings_SourceAlive(context))
        return error(diagnostic,"Capture source is unavailable or retired");
    if(!nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,saved) ||
       !saved->source_owned || saved->object!=context->source || saved->generation!=context->generation ||
       saved->reference.lifetime!=context->scene_epoch ||
       nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=&saved->program->bank->motion_layout ||
       !nativeFighterLifetime_Current(saved))
        return error(diagnostic,"Capture operation requires its exact owned Fighter program/storage");
    return true;
}

bool nativeScriptBindingCapture_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* arguments,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    if(!result)return error(diagnostic,"Missing capture result");
    if(semantic==NativeScriptBinding_CaptureEncloseCaught){
        if(!arguments || count!=1 || arguments[0].type!=NativeScriptType_F32 ||
           !isfinite(arguments[0].as.f32) || arguments[0].as.f32<0)
            return error(diagnostic,"Caught enclosure requires one finite nonnegative radius");
    }else if(semantic==NativeScriptBinding_CaptureConstrainCaught){
        if(count)return error(diagnostic,"Caught constraint takes no arguments");
    }else if(semantic==NativeScriptBinding_CaptureSetVictimCaptureMotion || semantic==NativeScriptBinding_CaptureInitializeHold ||
             semantic==NativeScriptBinding_CaptureSetVictimHoldMotion || semantic==NativeScriptBinding_CaptureSetVictimThrowMotion ||
             semantic==NativeScriptBinding_CaptureSetVictimAttachedThrow){
        uint32_t expected=semantic==NativeScriptBinding_CaptureSetVictimAttachedThrow?4:
            semantic==NativeScriptBinding_CaptureSetVictimThrowMotion?2:1;
        if(!arguments || count!=expected || arguments[0].type!=NativeScriptType_I32 ||
           arguments[0].as.i64<0 || arguments[0].as.i64>UINT16_MAX)
            return error(diagnostic,"Victim motion requires its declared arguments and an Int motion ID");
        if(expected==2 && (arguments[1].type!=NativeScriptType_F32 || !isfinite(arguments[1].as.f32)))
            return error(diagnostic,"Thrown victim playback rate requires a finite Float");
        if(expected==4 && (arguments[1].type!=NativeScriptType_U32 || arguments[1].as.u64>UINT32_MAX ||
           arguments[2].type!=NativeScriptType_F32 || !isfinite(arguments[2].as.f32) || arguments[3].type!=NativeScriptType_Bool))
            return error(diagnostic,"Attached throw requires native-width flags, a finite Float frame and a Bool initial-update option");
    }else if(semantic==NativeScriptBinding_CaptureReleaseDamage){
        if(!arguments || count!=2 || arguments[0].type!=NativeScriptType_Bool || arguments[1].type!=NativeScriptType_Bool)
            return error(diagnostic,"Damage release requires two Bools: retain collision credit and force upward launch");
    }else if(semantic==NativeScriptBinding_CaptureSyncVictimFacing || semantic==NativeScriptBinding_ThrowsUpdate || semantic==NativeScriptBinding_PhysicsThrowMotion || semantic==NativeScriptBinding_CaptureReleaseDamageFall || semantic==NativeScriptBinding_CaptureDropVictim){
        if(count)return error(diagnostic,"Throw update, movement and victim-facing sync take no arguments");
    }else if(semantic==NativeScriptBinding_CaptureSetVictimBodyStatus){
        if(!arguments || count!=1 || arguments[0].type!=NativeScriptType_I32 ||
           arguments[0].as.i64<0 || arguments[0].as.i64>2)
            return error(diagnostic,"Victim body status requires one Int: 0 normal, 1 invincible, 2 intangible");
    }else if(semantic==NativeScriptBinding_CaptureAttachItem || semantic==NativeScriptBinding_CaptureReleaseItem){
        uint32_t expected=semantic==NativeScriptBinding_CaptureAttachItem?1:2;
        if(!arguments || count!=expected || arguments[0].type!=NativeScriptType_I32 ||
           arguments[0].as.i64<0 || arguments[0].as.i64>INT32_MAX ||
           (expected==2 && arguments[1].type!=NativeScriptType_Bool))
            return error(diagnostic,"Captured item operation requires a bone ID Int; release also requires a Bool");
    }else if(semantic==NativeScriptBinding_CaptureMask || semantic==NativeScriptBinding_CaptureArm){
        if(!arguments || count!=1 || arguments[0].type!=NativeScriptType_I32 ||
           arguments[0].as.i64<0 || arguments[0].as.i64>INT16_MAX)
            return error(diagnostic,"Capture arm/exclusion mask requires one Int in 0..32767");
    }else if(semantic==NativeScriptBinding_CaptureBegin || semantic==NativeScriptBinding_CaptureBeginAttached){
        const uint32_t expected=semantic==NativeScriptBinding_CaptureBeginAttached?1:5;
        if(!arguments || count!=expected || arguments[0].type!=NativeScriptType_I32 ||
           arguments[0].as.i64<0 || arguments[0].as.i64>INT16_MAX)
            return error(diagnostic,"Capture entry requires an Int motion ID and its declared options");
        for(unsigned i=1;i<expected;++i)if(arguments[i].type!=NativeScriptType_Bool)
            return error(diagnostic,"Capture entry options require Bools");
    }else if(semantic<NativeScriptBinding_CaptureClearMovement ||
             semantic>NativeScriptBinding_CaptureReleaseThrow || count)
        return error(diagnostic,"Unsupported capture operation or arguments");
    NativeFighterLifetime saved;
    if(!retain_source(context,&saved,diagnostic))return false;
    if(semantic==NativeScriptBinding_CaptureReleaseDamage || semantic==NativeScriptBinding_CaptureReleaseDamageFall || semantic==NativeScriptBinding_CaptureDropVictim){
        NativeFighterCapturePair pair;
        if(!nativeFighterCapture_Retain(saved.object,&pair) || (pair.victim.object && !nativeFighterCapture_Attached(&pair)))
            return error(diagnostic,"Damage release requires its live captured relation");
        if(pair.victim.object){
            // The common damage routine intentionally unlinks both actors.
            // Keep the original identities across its callbacks and launch.
            if(semantic==NativeScriptBinding_CaptureDropVictim)ftCo_800DC920(saved.object,pair.victim.object);
            else ftCo_800DE2A8(saved.object,pair.victim.object);
            if(!nativeFighterCapture_Current(&pair,false))
                return error(diagnostic,"Damage release changed a retained actor before launch");
            if(semantic==NativeScriptBinding_CaptureDropVictim)ftCo_Fall_Enter(pair.victim.object);
            else if(semantic==NativeScriptBinding_CaptureReleaseDamageFall)ftCo_80090780(pair.victim.object);
            else ftCo_800DE7C0(pair.victim.object,arguments[0].as.u64?saved.object:NULL,arguments[1].as.u64!=0);
            if(!nativeFighterCapture_Current(&pair,false))
                return error(diagnostic,"Damage release changed a retained actor during launch");
        }
    }else if(semantic==NativeScriptBinding_ThrowsUpdate){
        NativeFighterCapturePair pair;
        if(!nativeFighterCapture_Retain(saved.object,&pair) || (pair.victim.object && !nativeFighterCapture_Attached(&pair)))
            return error(diagnostic,"Throw update requires its live captured relation");
        ftCo_800DD724(saved.object);
        if(!nativeFighterCapture_Current(&pair,false) ||
           (saved.data->victim_gobj && !nativeFighterCapture_Attached(&pair)))
            return error(diagnostic,"Throw update changed a retained actor or replaced its captured relation");
    }else if(semantic==NativeScriptBinding_PhysicsThrowMotion)ftCo_ThrowF_Phys(saved.object);
    else if(semantic==NativeScriptBinding_CaptureSyncVictimFacing){
        NativeFighterCapturePair pair;
        if(!nativeFighterCapture_Retain(saved.object,&pair) || (pair.victim.object && !nativeFighterCapture_Attached(&pair)))
            return error(diagnostic,"Victim facing sync requires its live captured relation");
        if(pair.victim.object)pair.victim.data->facing_dir=saved.data->facing_dir;
    }else if(semantic==NativeScriptBinding_CaptureEncloseCaught || semantic==NativeScriptBinding_CaptureConstrainCaught){
        NativeFighterCapturePair pair;
        if(!nativeFighterCapture_Retain(saved.object,&pair))
            return error(diagnostic,"Caught policy references a retired or inconsistent victim");
        if(pair.victim.object){
            HSD_JObj* joint;
            if(semantic==NativeScriptBinding_CaptureEncloseCaught){
                if(!nativeFighterCapture_Joint(&pair.victim,FtPart_XRotN,&joint))
                    return error(diagnostic,"Caught enclosure requires the victim's admitted rotation joint");
                float scale=ftCommon_GetModelScale(pair.victim.data);
                if(!isfinite(scale) || scale<=0 || !isfinite(arguments[0].as.f32/scale))
                    return error(diagnostic,"Caught enclosure requires a finite positive victim model scale");
                nativeCapture_EncloseCaught(saved.object,arguments[0].as.f32);
            }else{
                if(!nativeFighterCapture_Joint(&pair.holder,FtPart_TransN2,&joint) ||
                   (!pair.victim.data->x2226_b2 && !nativeFighterCapture_Joint(&pair.victim,FtPart_XRotN,&joint)))
                    return error(diagnostic,"Caught constraint requires admitted holder and victim joints");
                ftCo_800DB368(pair.holder.data,pair.victim.data);
            }
            if(!nativeFighterCapture_Current(&pair,true))
                return error(diagnostic,"Caught policy changed a retained actor lifetime");
        }
    }else if(semantic==NativeScriptBinding_CaptureSetVictimCaptureMotion || semantic==NativeScriptBinding_CaptureInitializeHold ||
             semantic==NativeScriptBinding_CaptureSetVictimHoldMotion || semantic==NativeScriptBinding_CaptureSetVictimThrowMotion ||
             semantic==NativeScriptBinding_CaptureSetVictimAttachedThrow){
        NativeFighterCapturePair pair;
        if(!nativeFighterCapture_Retain(saved.object,&pair))
            return error(diagnostic,"Victim motion references a retired or inconsistent captured pair");
        if(pair.victim.object){
            Fighter* victim=pair.victim.data;
            FtMotionId motion=(FtMotionId)arguments[0].as.i64;
            if(!nativeFighterCapture_AdmitMotion(&pair.victim,motion) || !nativeFighterCapture_Attached(&pair))
                return error(diagnostic,"Victim motion requires an admitted motion and reciprocal holder link");
            if(semantic==NativeScriptBinding_CaptureInitializeHold){
                if(!p_ftCommonData || !isfinite(victim->dmg.x1830_percent) ||
                   !isfinite(p_ftCommonData->x4A0) || !isfinite(p_ftCommonData->x4A4))
                    return error(diagnostic,"Hold initialization requires finite native mash timer data");
                float timer=victim->dmg.x1830_percent*p_ftCommonData->x4A0+p_ftCommonData->x4A4;
                if(!isfinite(timer) || timer<0 || (double)timer>INT32_MAX)
                    return error(diagnostic,"Hold mash timer is outside the engine's nonnegative Int domain");
                ftCo_8009C640(pair.victim.object,motion);
            }else if(semantic==NativeScriptBinding_CaptureSetVictimHoldMotion)ftCo_8009C5A4(pair.victim.object,motion);
            else if(semantic==NativeScriptBinding_CaptureSetVictimThrowMotion)ftCo_800DE3FC(pair.victim.object,motion,arguments[1].as.f32);
            else if(semantic==NativeScriptBinding_CaptureSetVictimAttachedThrow)
                ftCo_EnterAttachedThrow(pair.victim.object,motion,(MotionFlags)arguments[1].as.u64,arguments[2].as.f32,arguments[3].as.u64!=0);
            else ftCo_EnterAttachedCaptureMotion(pair.victim.object,motion);
            if(!nativeFighterCapture_Attached(&pair))
                return error(diagnostic,"Victim motion changed a retained actor lifetime");
        }
    }else if(semantic==NativeScriptBinding_CaptureSetVictimBodyStatus){
        NativeFighterCapturePair pair;
        if(!nativeFighterCapture_Retain(saved.object,&pair))
            return error(diagnostic,"Victim body status references a retired or inconsistent victim");
        if(pair.victim.object){
            ftColl_8007B62C(pair.victim.object,(enum_t)arguments[0].as.i64);
            if(!nativeFighterCapture_Current(&pair,true))
                return error(diagnostic,"Victim body status changed a retained actor lifetime");
        }
    }else if(semantic==NativeScriptBinding_CaptureAttachItem || semantic==NativeScriptBinding_CaptureReleaseItem){
        NativeCaptureItemPair pair;
        if(!nativeCaptureItem_Retain(saved.object,&pair))
            return error(diagnostic,"Captured item is retired or inconsistent");
        const NativeSourcePackage* package=nativeFighterRuntime_SourcePackage(saved.kind);
        int selected=package?nativeFighterRig_Resolve(&package->assets.rig,arguments[0].as.i64):(int)arguments[0].as.i64;
        if(selected<0 || selected>=MAX_FT_PARTS)return error(diagnostic,"Captured item bone ID is absent");
        unsigned bone=(unsigned)selected;
        if(semantic==NativeScriptBinding_CaptureAttachItem){
            if(!pair.item.object || !nativeCaptureItem_Attach(pair.item.object,saved.object,bone))
                return error(diagnostic,"Captured item attachment requires a live selected item and valid bones");
        }else if(pair.item.object && !nativeCaptureItem_Release(pair.item.object,bone,arguments[1].as.u64!=0))
            return error(diagnostic,"Captured item release changed a retained actor or rig lifetime");
    }else if(semantic==NativeScriptBinding_CaptureArm) {
        if(!nativeScriptFighter_HasHook(saved.object,NativeScriptFighter_GrabCatch) ||
           !nativeScriptFighter_HasHook(saved.object,NativeScriptFighter_Grabbed))
            return error(diagnostic,"Arming capture requires owned grab_catch and grabbed events");
        // Null machine-code slots select the prepared owner hooks at the actual
        // collision scheduler. Ordinary engine grabs install native callbacks
        // and supersede these hooks without another mutable registration layer.
        ftCommon_8007E2D0(saved.data,(s16)arguments[0].as.i64,NULL,NULL,NULL);
    } else if(semantic==NativeScriptBinding_CaptureMask)
        ftCommon_8007E2F4(saved.data,(s16)arguments[0].as.i64);
    else if(semantic==NativeScriptBinding_CaptureBeginAttached){
        NativeFighterCapturePair pair;
        if(!nativeFighterCapture_Retain(saved.object,&pair) || !pair.victim.object ||
           !nativeFighterCapture_AdmitMotion(&pair.victim,(FtMotionId)arguments[0].as.i64))
            return error(diagnostic,"Attached capture entry requires a live selected victim and an admitted motion");
        ftCo_InitializeAttachedCapture(pair.victim.object,saved.object,(FtMotionId)arguments[0].as.i64);
        if(!nativeFighterCapture_Attached(&pair))
            return error(diagnostic,"Attached capture entry changed a retained actor or reciprocal link");
    }else if(semantic==NativeScriptBinding_CaptureBegin){
        NativeFighterCapturePair pair;
        if(!nativeFighterCapture_Retain(saved.object,&pair) || !pair.victim.object ||
           !nativeFighterCapture_Begin(pair.victim.object,saved.object,(FtMotionId)arguments[0].as.i64,
              arguments[1].as.u64!=0,arguments[2].as.u64!=0,arguments[3].as.u64!=0,arguments[4].as.u64!=0))
            return error(diagnostic,"Capture entry requires a live selected victim and an admitted motion");
    }else if(semantic==NativeScriptBinding_CaptureClearMovement)ftCommon_8007E2FC(saved.object);
    else if(semantic==NativeScriptBinding_CaptureAttachToVictim){
        if(!nativeFighterCapture_AttachToVictim(saved.object))
            return error(diagnostic,"Capture attachment requires a live selected victim");
    }else if(semantic==NativeScriptBinding_CaptureFollowVictim){
        NativeFighterCapturePair pair;
        if(!nativeFighterCapture_Retain(saved.object,&pair) || !pair.victim.object)
            return error(diagnostic,"Capture tracking requires a live selected victim");
        saved.data->accessory4_cb=nativeFighterCapture_FollowVictim;
    }else if(semantic==NativeScriptBinding_CaptureReleaseIntoEnclosure){
        if(!nativeCapture_ReleaseIntoEnclosure(saved.object))
            return error(diagnostic,"Enclosure release requires a live captured pair, imported model and victim rig");
    }else if(semantic==NativeScriptBinding_CaptureReleaseThrow){
        if(!nativeFighterCapture_ReleaseThrow(saved.object))
            return error(diagnostic,"Capture throw release changed a retained actor lifetime");
    }
    if(!nativeFighterLifetime_Current(&saved))
        return error(diagnostic,"Capture source changed before publication");
    *result=(NativeScriptValue){.type=NativeScriptType_Void};return true;
}

static float* setting(Fighter* fighter,uint32_t semantic)
{
    switch(semantic){
    case NativeScriptBinding_CaptureTimerDamageMultiplier:return &fighter->capture_settings.timer_damage_multiplier;
    case NativeScriptBinding_CaptureTimerDecay:return &fighter->capture_settings.timer_decay;
    case NativeScriptBinding_CaptureMashReduction:return &fighter->capture_settings.mash_reduction;
    case NativeScriptBinding_CaptureMashPauseFrames:return &fighter->capture_settings.mash_pause_frames;
    case NativeScriptBinding_CaptureMashAnimationRate:return &fighter->capture_settings.mash_animation_rate;
    case NativeScriptBinding_CaptureWiggleScale:return &fighter->capture_settings.wiggle_scale;
    case NativeScriptBinding_CaptureWiggleLimit:return &fighter->capture_settings.wiggle_limit;
    case NativeScriptBinding_CaptureEnclosureLaunchX:return &fighter->capture_enclosure_settings.launch_velocity.x;
    case NativeScriptBinding_CaptureEnclosureLaunchY:return &fighter->capture_enclosure_settings.launch_velocity.y;
    case NativeScriptBinding_CaptureEnclosureLaunchZ:return &fighter->capture_enclosure_settings.launch_velocity.z;
    case NativeScriptBinding_CaptureEnclosureDamageBehavior:return &fighter->capture_enclosure_settings.damage_behavior;
    case NativeScriptBinding_CaptureEnclosureShrinkFraction:return &fighter->capture_enclosure_settings.shrink_fraction;
    case NativeScriptBinding_CaptureEnclosureShrinkFrames:return &fighter->capture_enclosure_settings.shrink_frames;
    case NativeScriptBinding_CaptureEnclosureLifetime:return &fighter->capture_enclosure_settings.lifetime;
    case NativeScriptBinding_CaptureEnclosureDamageTimeScale:return &fighter->capture_enclosure_settings.damage_time_scale;
    case NativeScriptBinding_CaptureEnclosureEscapeX:return &fighter->capture_enclosure_settings.escape_velocity.x;
    case NativeScriptBinding_CaptureEnclosureEscapeY:return &fighter->capture_enclosure_settings.escape_velocity.y;
    case NativeScriptBinding_CaptureEnclosureEscapeZ:return &fighter->capture_enclosure_settings.escape_velocity.z;
    default:return NULL;
    }
}

static int* integer_setting(Fighter* fighter,uint32_t semantic)
{
    switch(semantic){
    case NativeScriptBinding_CaptureEnclosureModelSlot:return &fighter->capture_enclosure_settings.model_slot;
    case NativeScriptBinding_CaptureEnclosureEscapeInvincibilityFrames:return &fighter->capture_enclosure_settings.escape_invincibility_frames;
    default:return NULL;
    }
}

bool nativeScriptBindingCapture_Read(NativeScriptRuntimeContext* context,uint32_t semantic,
    NativeScriptValue* output,NativeScriptDiagnostic* diagnostic)
{
    NativeFighterLifetime saved;
    if(!output || !retain_source(context,&saved,diagnostic))return false;
    NativeScriptValue value={.type=NativeScriptType_Bool};
    float* parameter=setting(saved.data,semantic);
    int* integer=integer_setting(saved.data,semantic);
    if(integer){value.type=NativeScriptType_I32;value.as.i64=*integer;}
    else if(parameter){value.type=NativeScriptType_F32;value.as.f32=*parameter;}
    else if(semantic==NativeScriptBinding_CaptureGroundLink)value.as.u64=saved.data->x221B_b7;
    else if(semantic==NativeScriptBinding_CaptureHasItem){
        NativeCaptureItemPair pair;
        if(!nativeCaptureItem_Retain(saved.object,&pair))
            return error(diagnostic,"Capture observation references a retired or inconsistent item");
        value.as.u64=pair.item.object!=NULL;
    }
    else if(semantic==NativeScriptBinding_CaptureHasVictim || semantic==NativeScriptBinding_CaptureVictimDamage ||
            semantic==NativeScriptBinding_CaptureVictimVisible || semantic==NativeScriptBinding_CaptureVictimFrame ||
            semantic==NativeScriptBinding_CaptureVictimAnimationFamily || semantic==NativeScriptBinding_CaptureVictimFreezeState){
        NativeFighterCapturePair pair;
        if(!nativeFighterCapture_Retain(saved.object,&pair))
            return error(diagnostic,"Capture observation references a retired or inconsistent victim");
        if(semantic==NativeScriptBinding_CaptureVictimAnimationFamily){
            value.type=NativeScriptType_I32;
            value.as.i64=pair.victim.object?nativeFighterRuntime_AnimationFamily(pair.victim.data):Ft_Kind_None;
            if(!nativeFighterCapture_Current(&pair,true))
                return error(diagnostic,"Victim animation family changed its retained capture relation");
        }else if(semantic==NativeScriptBinding_CaptureVictimFreezeState)value.as.u64=pair.victim.object&&pair.victim.data->x2222_b6;
        else if(semantic==NativeScriptBinding_CaptureVictimFrame){value.type=NativeScriptType_F32;value.as.f32=pair.victim.object?pair.victim.data->cur_anim_frame:0;}
        else if(semantic==NativeScriptBinding_CaptureHasVictim)value.as.u64=pair.victim.object!=NULL;
        else if(semantic==NativeScriptBinding_CaptureVictimVisible)value.as.u64=pair.victim.object&&!pair.victim.data->invisible;
        else{value.type=NativeScriptType_F32;value.as.f32=pair.victim.object?pair.victim.data->dmg.x1830_percent:0;}
    }else return error(diagnostic,"Unsupported capture observation");
    if(!nativeFighterLifetime_Current(&saved))return error(diagnostic,"Capture observation source retired");
    *output=value;return true;
}

bool nativeScriptBindingCapture_Write(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* value,NativeScriptDiagnostic* diagnostic)
{
    bool ground_link=semantic==NativeScriptBinding_CaptureGroundLink;
    bool victim_visible=semantic==NativeScriptBinding_CaptureVictimVisible;
    bool integer_parameter=semantic==NativeScriptBinding_CaptureEnclosureModelSlot ||
        semantic==NativeScriptBinding_CaptureEnclosureEscapeInvincibilityFrames;
    NativeScriptIRType type=(ground_link||victim_visible)?NativeScriptType_Bool:integer_parameter?NativeScriptType_I32:NativeScriptType_F32;
    if(!value || value->type!=type || (integer_parameter &&
       (value->as.i64<INT32_MIN || value->as.i64>INT32_MAX)))
        return error(diagnostic,"Capture settings require their declared Bool, Float or native-width Int type");
    NativeFighterLifetime saved;
    if(!retain_source(context,&saved,diagnostic))return false;
    float* parameter=setting(saved.data,semantic);
    int* integer=integer_setting(saved.data,semantic);
    if(victim_visible){
        NativeFighterCapturePair pair;
        if(!nativeFighterCapture_Retain(saved.object,&pair))
            return error(diagnostic,"Capture visibility references a retired or inconsistent victim");
        if(pair.victim.object)pair.victim.data->invisible=value->as.u64==0;
    }else if(integer)*integer=(int)value->as.i64;
    else if(parameter)*parameter=value->as.f32;
    else if(ground_link)saved.data->x221B_b7=value->as.u64!=0;
    else return error(diagnostic,"Unsupported capture assignment");
    return nativeFighterLifetime_Current(&saved) || error(diagnostic,"Capture assignment source retired");
}
