#include "native_script_binding_common_properties.h"
#include "native_script_bindings.h"
#include "native_callback_lifetime.h"
#include "fighter.h"
#include "native_fighter_lifetime.h"
#include "ftdata.h"
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/class.h>
#include <stdio.h>

static bool error(NativeScriptDiagnostic* d,const char* message)
{if(d)snprintf(d->message,sizeof(d->message),"%s",message);return false;}

// Smash fields belong to the physical Fighter instance, not a stock-kind table.
static Fighter* smash_actor(NativeScriptRuntimeContext* context,NativeScriptDiagnostic* d)
{
    NativeFighterLifetime saved;
    if(!context||context->source_class!=NativeCallbackSource_Fighter||!context->scene_epoch||
       !nativeScriptBindings_SourceAlive(context)||
       !nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&saved)||
       saved.object!=context->source||saved.reference.lifetime!=context->scene_epoch||
       saved.generation!=context->generation||!nativeFighterLifetime_Current(&saved)){
        error(d,"Smash state requires its exact live Fighter instance");return NULL;
    }
    return saved.data;
}

bool nativeScriptBindingCommonProperties_Read(NativeScriptRuntimeContext* context,uint32_t semantic,
    NativeScriptValue* result,NativeScriptDiagnostic* d)
{
    if(semantic>=NativeScriptBinding_SmashChargeMode&&semantic<=NativeScriptBinding_SmashShakeTableCount){
        if(!result)return error(d,"Missing smash property output");
        Fighter* fighter=smash_actor(context,d);if(!fighter)return false;
        SmashAttr* state=&fighter->smash_attrs;
        NativeScriptValue value={0};
        switch(semantic){
        case NativeScriptBinding_SmashChargeMode:value.type=NativeScriptType_I32;value.as.i64=state->state;break;
        case NativeScriptBinding_SmashChargeFrames:value.type=NativeScriptType_F32;value.as.f32=state->x2118_frames;break;
        case NativeScriptBinding_SmashChargeLimit:value.type=NativeScriptType_F32;value.as.f32=state->x211C_holdFrame;break;
        case NativeScriptBinding_SmashSavedAnimationRate:value.type=NativeScriptType_F32;value.as.f32=state->x2124_frameSpeedMul;break;
        case NativeScriptBinding_SmashChargeColor:value.type=NativeScriptType_I32;value.as.i64=state->x2128;break;
        case NativeScriptBinding_SmashShakeIndex:value.type=NativeScriptType_I32;value.as.i64=state->x212C;break;
        case NativeScriptBinding_SmashShakePeriod:value.type=NativeScriptType_I32;value.as.i64=state->x212D;break;
        case NativeScriptBinding_SmashShakeTableCount:
            if(!Fighter_SmashChargeShakeTable)return error(d,"Native smash shake table is unavailable");
            value.type=NativeScriptType_I32;value.as.i64=Fighter_SmashChargeShakeTable->x4;break;
        default:return error(d,"Unknown smash state property");
        }
        *result=value;return true;
    }

    if(semantic==NativeScriptBinding_CaptureArmed||semantic==NativeScriptBinding_AnimationHipRootCompensation||semantic==NativeScriptBinding_DamageForceTopBlastKO){
        NativeFighterLifetime saved;
        if(!context||!result||context->source_class!=NativeCallbackSource_Fighter||!context->scene_epoch||
           !nativeScriptBindings_SourceAlive(context)||!nativeScriptBindings_SourceFighterLayout(context,d)||
           !nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&saved)||!saved.source_owned||
           saved.object!=context->source||saved.reference.lifetime!=context->scene_epoch||saved.generation!=context->generation||
           !nativeFighterLifetime_Current(&saved))return error(d,"Engine policy needs its exact live owned Fighter");
        bool flag=semantic==NativeScriptBinding_CaptureArmed?saved.data->x221E_b6:
            semantic==NativeScriptBinding_AnimationHipRootCompensation?saved.data->x2221_b2:saved.data->x2222_b3;
        *result=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=flag};return true;
    }
    if(semantic>=NativeScriptBinding_ShieldPushbackDamageScale&&
       semantic<=NativeScriptBinding_ShieldPowershieldDuration){
        NativeMatchPoolReference reference;NativeFighterLifetime saved;
        if(!context||!result||context->source_class!=NativeCallbackSource_Fighter||!context->scene_epoch||
           !nativeMatchPool_Reference(&nativeGObjPool,context->source,&reference)||reference.lifetime!=context->scene_epoch||
           ((HSD_GObj*)context->source)->classifier!=HSD_GOBJ_CLASS_FIGHTER||!((HSD_GObj*)context->source)->user_data||
           !nativeScriptBindings_SourceAlive(context)||!nativeScriptBindings_SourceFighterLayout(context,d)||
           !nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&saved)||!saved.source_owned||
           saved.object!=context->source||saved.reference.lifetime!=context->scene_epoch||saved.generation!=context->generation)
            return error(d,"Shield tuning data requires its exact owned source Fighter");
        const ftCommonData* common=p_ftCommonData;
        if(!common)return error(d,"Common shield tuning data is unavailable");
        NativeScriptValue value={.type=NativeScriptType_F32};
        switch(semantic){
        case NativeScriptBinding_ShieldPushbackDamageScale:value.as.f32=common->x28C;break;
        case NativeScriptBinding_ShieldPushbackBase:value.as.f32=common->x290;break;
        case NativeScriptBinding_ShieldPushbackVelocityScale:value.as.f32=common->x294;break;
        case NativeScriptBinding_ShieldPushbackLightMin:value.as.f32=common->x2E4;break;
        case NativeScriptBinding_ShieldPushbackLightMax:value.as.f32=common->x2E8;break;
        case NativeScriptBinding_ShieldStartHealth:value.as.f32=common->x260_startShieldHealth;break;
        case NativeScriptBinding_ShieldMinimumHoldDuration:value.as.f32=common->x268;break;
        case NativeScriptBinding_ShieldReflectDuration:value.as.f32=common->x2A4;break;
        case NativeScriptBinding_ShieldReflectRadius:value.as.f32=common->x2A8;break;
        case NativeScriptBinding_ShieldReflectDamageMultiplier:value.as.f32=common->x2AC;break;
        case NativeScriptBinding_ShieldReflectSpeedMultiplier:value.as.f32=common->x2B0;break;
        case NativeScriptBinding_ShieldPowershieldDuration:value.as.f32=common->x2B4;break;
        default:return error(d,"Unknown shield tuning property");
        }
        if(!nativeScriptBindings_SourceAlive(context)||!nativeScriptBindings_SourceFighterLayout(context,d)||
           p_ftCommonData!=common||!nativeFighterLifetime_Current(&saved))
            return error(d,"Shield tuning source, program, storage or common data retired");
        *result=value;return true;
    }
    if(semantic==NativeScriptBinding_ControlsHorizontalSmashThreshold||
       semantic==NativeScriptBinding_ControlsHorizontalSmashWindow||
       semantic==NativeScriptBinding_ControlsHorizontalSmashExtension){
        NativeMatchPoolReference reference;NativeFighterLifetime saved;
        if(!context||!result||context->source_class!=NativeCallbackSource_Fighter||!context->scene_epoch||
           !nativeMatchPool_Reference(&nativeGObjPool,context->source,&reference)||reference.lifetime!=context->scene_epoch||
           !nativeScriptBindings_SourceAlive(context)||!nativeScriptBindings_SourceFighterLayout(context,d)||
           !nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&saved)||!saved.source_owned||
           saved.object!=context->source||saved.reference.lifetime!=context->scene_epoch||saved.generation!=context->generation)
            return error(d,"Horizontal smash controls need their live selected source Fighter");
        const ftCommonData* common=p_ftCommonData;
        if(!common)return error(d,"Common horizontal smash data is unavailable");
        NativeScriptValue value={.type=NativeScriptType_F32};
        if(semantic==NativeScriptBinding_ControlsHorizontalSmashThreshold)value.as.f32=common->dash_smash_stick_threshold;
        // The legacy common_attribute ABI returns Float even though window
        // storage is native signed Int; retain that exact conversion.
        else if(semantic==NativeScriptBinding_ControlsHorizontalSmashWindow)value.as.f32=common->dash_smash_window;
        else value.as.f32=common->x44;
        if(p_ftCommonData!=common||!nativeFighterLifetime_Current(&saved)||
           !nativeScriptBindings_SourceFighterLayout(context,d))
            return error(d,"Horizontal smash source, program, storage or common data retired");
        *result=value;return true;
    }
    if(semantic==NativeScriptBinding_FighterActionDetachesItems){
        NativeMatchPoolReference reference;
        if(!context||!result||context->source_class!=NativeCallbackSource_Fighter||!context->scene_epoch||
           !nativeMatchPool_Reference(&nativeGObjPool,context->source,&reference)||reference.lifetime!=context->scene_epoch||
           ((HSD_GObj*)context->source)->classifier!=HSD_GOBJ_CLASS_FIGHTER||!((HSD_GObj*)context->source)->user_data)
            return error(d,"Item-detachment policy requires a physical Fighter in its scene");
        Fighter* fighter=((HSD_GObj*)context->source)->user_data;NativeFighterLifetime saved;
        if(!nativeFighterLifetime_Capture(fighter,&saved)||saved.object!=context->source||
           saved.reference.lifetime!=context->scene_epoch||saved.generation!=context->generation)
            return error(d,"Item-detachment policy requires the exact retained Fighter identity");
        NativeScriptValue value={.type=NativeScriptType_Bool,.as.u64=fighter->x2070.x2071_b6!=0};
        if(!nativeFighterLifetime_Current(&saved)||!nativeScriptBindings_SourceAlive(context))
            return error(d,"Item-detachment policy Fighter changed before publication");
        *result=value;return true;
    }
    if(!context||!result||context->source_class!=NativeCallbackSource_Fighter||
       !nativeScriptBindings_SourceAlive(context))return error(d,"Common property requires a live Fighter");
    Fighter* fighter=((HSD_GObj*)context->source)->user_data;
    NativeScriptValue value={.type=NativeScriptType_F32};
    switch(semantic) {
    case NativeScriptBinding_AirTerminalVelocity:value.as.f32=fighter->co_attrs.terminal_velocity;break;
    case NativeScriptBinding_AirDriftThreshold:
        if(!p_ftCommonData)return error(d,"Common air-drift threshold data is unavailable");
        value.as.f32=p_ftCommonData->x258;break;
    case NativeScriptBinding_ControlsStickSmoothing:
        if(!p_ftCommonData)return error(d,"Common stick-smoothing data is unavailable");
        value.as.f32=p_ftCommonData->x44C;break;
    case NativeScriptBinding_ControlsHorizontalStickDeadzone:
        if(!p_ftCommonData)return error(d,"Common horizontal stick deadzone is unavailable");
        value.as.f32=p_ftCommonData->horizontal_stick_deadzone;break;
    case NativeScriptBinding_ControlsAerialJumpReverseThreshold:
        if(!p_ftCommonData)return error(d,"Common aerial-jump reverse data is unavailable");
        value.as.f32=p_ftCommonData->x78;break;
    case NativeScriptBinding_AirJumpHorizontalMultiplier:
        value.as.f32=fighter->co_attrs.air_jump_h_multiplier;break;
    case NativeScriptBinding_ControlsTapJumpThreshold:
        if(!p_ftCommonData)return error(d,"Common tap-jump threshold data is unavailable");
        value.as.f32=p_ftCommonData->tap_jump_threshold;break;
    case NativeScriptBinding_ControlsAerialActionDownThreshold:
        if(!p_ftCommonData)return error(d,"Common aerial-action down data is unavailable");
        value.as.f32=p_ftCommonData->x88;break;
    case NativeScriptBinding_ModelInstanceScaleY:value.as.f32=fighter->x34_scale.y;break;
    case NativeScriptBinding_ModelBaseScale:value.as.f32=fighter->co_attrs.model_scaling;break;
    // Exact ftCommon_GetModelScale / ftLib_800869D4 field arithmetic; different
    // from the instance scale and from the live root JObj's rendered scale.
    case NativeScriptBinding_ModelScale:value.as.f32=fighter->x34_scale.y*fighter->co_attrs.model_scaling;break;
    // First scalar of the common effect-attribute block. Native PositionScalar
    // recipes consume this value directly, without multiplying model scale.
    case NativeScriptBinding_ModelEffectScale:value.as.f32=fighter->co_attrs.xBC.size;break;
    case NativeScriptBinding_ModelRootScaleY: {
        NativeScriptRuntimeContext saved=*context;HSD_GObj* object=saved.source;
        if(object->classifier!=HSD_GOBJ_CLASS_FIGHTER||fighter->gobj!=object)
            return error(d,"Rendered root scale requires the retained Fighter identity");
        HSD_JObj* root=object->hsd_obj;u8 object_kind=object->obj_kind;
        HSD_ClassInfo* root_class=root?HSD_CLASS_METHOD(root):NULL;
        if(root && (object_kind!=HSD_GObj_JObjKind||!root_class||
           root_class->head.obj_size<(int)sizeof(*root)||
           !hsdObjIsDescendantOf((HSD_Obj*)root,HSD_CLASS_INFO(&hsdJObj))))
            return error(d,"Rendered root scale requires an admitted native JObj class and extent");
        // Class admission can initialize class metadata. Validate the captured
        // Fighter/root again before accessing the admitted object, without
        // requiring a foreign receiver to share the caller's authored schema.
        if(!nativeScriptBindings_SourceAlive(&saved)||object->user_data!=fighter||
           fighter->gobj!=object||object->classifier!=HSD_GOBJ_CLASS_FIGHTER||
           object->hsd_obj!=root||object->obj_kind!=object_kind||
           (root && (HSD_CLASS_METHOD(root)!=root_class||root_class->head.obj_size<(int)sizeof(*root))))
            return error(d,"Rendered root scale identity changed during native class admission");
        value.as.f32=root?HSD_JObjGetScaleY(root):1.0f;
        if(!nativeScriptBindings_SourceAlive(&saved)||object->user_data!=fighter||
           fighter->gobj!=object||object->classifier!=HSD_GOBJ_CLASS_FIGHTER||
           object->hsd_obj!=root||object->obj_kind!=object_kind||
           (root && (HSD_CLASS_METHOD(root)!=root_class||root_class->head.obj_size<(int)sizeof(*root))))
            return error(d,"Rendered root scale retired before publication");
        break;
    }
    case NativeScriptBinding_CeilingNormalX:value.as.f32=fighter->coll_data.ceiling.normal.x;break;
    case NativeScriptBinding_CeilingNormalY:value.as.f32=fighter->coll_data.ceiling.normal.y;break;
    case NativeScriptBinding_CeilingNormalZ:value.as.f32=fighter->coll_data.ceiling.normal.z;break;
    case NativeScriptBinding_WallLeftNormalX:value.as.f32=fighter->coll_data.left_facing_wall.normal.x;break;
    case NativeScriptBinding_WallLeftNormalY:value.as.f32=fighter->coll_data.left_facing_wall.normal.y;break;
    case NativeScriptBinding_WallLeftNormalZ:value.as.f32=fighter->coll_data.left_facing_wall.normal.z;break;
    case NativeScriptBinding_WallRightNormalX:value.as.f32=fighter->coll_data.right_facing_wall.normal.x;break;
    case NativeScriptBinding_WallRightNormalY:value.as.f32=fighter->coll_data.right_facing_wall.normal.y;break;
    case NativeScriptBinding_WallRightNormalZ:value.as.f32=fighter->coll_data.right_facing_wall.normal.z;break;
    case NativeScriptBinding_ECBTopY:value.as.f32=fighter->coll_data.ecb.top.y;break;
    case NativeScriptBinding_ECBBottomY:value.as.f32=fighter->coll_data.ecb.bottom.y;break;
    case NativeScriptBinding_ECBTopX:value.as.f32=fighter->coll_data.ecb.top.x;break;
    case NativeScriptBinding_ECBBottomX:value.as.f32=fighter->coll_data.ecb.bottom.x;break;
    case NativeScriptBinding_ECBLeftX:value.as.f32=fighter->coll_data.ecb.left.x;break;
    case NativeScriptBinding_ECBLeftY:value.as.f32=fighter->coll_data.ecb.left.y;break;
    case NativeScriptBinding_ECBRightX:value.as.f32=fighter->coll_data.ecb.right.x;break;
    case NativeScriptBinding_ECBRightY:value.as.f32=fighter->coll_data.ecb.right.y;break;
    case NativeScriptBinding_ItemsBlockGroundPickup:value=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=fighter->x2223_b4};break;
    case NativeScriptBinding_ArticlesAttachedVisible:value=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=fighter->x221E_b4};break;
    case NativeScriptBinding_FighterCreationID:value=(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=(uint32_t)fighter->x8_spawnNum};break;
    // Exact ftGetFacingDirInt convention, including zero/negative zero/NaN.
    case NativeScriptBinding_FacingSign:value=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=fighter->facing_dir<0?-1:1};break;
    default:return error(d,"Unsupported common Fighter property");
    }
    if(!nativeScriptBindings_SourceAlive(context))return error(d,"Common property source retired before publication");
    *result=value;return true;
}

bool nativeScriptBindingCommonProperties_Write(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* value,NativeScriptDiagnostic* d)
{
    if(semantic>=NativeScriptBinding_SmashChargeMode&&semantic<=NativeScriptBinding_SmashShakePeriod){
        if(!value)return error(d,"Missing smash property value");
        Fighter* fighter=smash_actor(context,d);if(!fighter)return false;
        SmashAttr* state=&fighter->smash_attrs;
        bool floating=semantic>=NativeScriptBinding_SmashChargeFrames&&semantic<=NativeScriptBinding_SmashSavedAnimationRate;
        if(value->type!=(floating?NativeScriptType_F32:NativeScriptType_I32))return error(d,"Smash property requires its exact Float or Int type");
        if((semantic==NativeScriptBinding_SmashChargeMode&&(value->as.i64<0||value->as.i64>4))||
           ((semantic==NativeScriptBinding_SmashShakeIndex||semantic==NativeScriptBinding_SmashShakePeriod)&&(value->as.i64<0||value->as.i64>255)))
            return error(d,"Smash mode or shake byte is outside its native range");
        switch(semantic){
        case NativeScriptBinding_SmashChargeMode:state->state=(SmashState)value->as.i64;break;
        case NativeScriptBinding_SmashChargeFrames:state->x2118_frames=value->as.f32;break;
        case NativeScriptBinding_SmashChargeLimit:state->x211C_holdFrame=value->as.f32;break;
        case NativeScriptBinding_SmashSavedAnimationRate:state->x2124_frameSpeedMul=value->as.f32;break;
        case NativeScriptBinding_SmashChargeColor:state->x2128=(int32_t)value->as.i64;break;
        case NativeScriptBinding_SmashShakeIndex:state->x212C=(uint8_t)value->as.i64;break;
        case NativeScriptBinding_SmashShakePeriod:state->x212D=(uint8_t)value->as.i64;break;
        default:return error(d,"Unknown writable smash state property");
        }
        return true;
    }

    if(semantic==NativeScriptBinding_CaptureArmed||semantic==NativeScriptBinding_AnimationHipRootCompensation||semantic==NativeScriptBinding_DamageForceTopBlastKO){
        NativeFighterLifetime saved;
        if(!context||!value||value->type!=NativeScriptType_Bool||value->as.u64>1||
           context->source_class!=NativeCallbackSource_Fighter||!context->scene_epoch||
           !nativeScriptBindings_SourceAlive(context)||!nativeScriptBindings_SourceFighterLayout(context,d)||
           !nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&saved)||!saved.source_owned||
           saved.object!=context->source||saved.reference.lifetime!=context->scene_epoch||saved.generation!=context->generation||
           !nativeFighterLifetime_Current(&saved))return error(d,"Engine policy needs its exact owned Fighter and canonical Bool");
        if(semantic==NativeScriptBinding_CaptureArmed)saved.data->x221E_b6=value->as.u64!=0;
        else if(semantic==NativeScriptBinding_AnimationHipRootCompensation)saved.data->x2221_b2=value->as.u64!=0;
        else saved.data->x2222_b3=value->as.u64!=0;
        return true;
    }
    if(!context||!value||context->source_class!=NativeCallbackSource_Fighter||
       !nativeScriptBindings_SourceAlive(context)||
       (semantic!=NativeScriptBinding_ItemsBlockGroundPickup&&semantic!=NativeScriptBinding_ArticlesAttachedVisible&&semantic!=NativeScriptBinding_ControlsInterruptAllowed)||
       value->type!=NativeScriptType_Bool||value->as.u64>1)return error(d,"Fighter policy requires a live Fighter and Boolean value");
    Fighter* fighter=((HSD_GObj*)context->source)->user_data;
    if(semantic==NativeScriptBinding_ControlsInterruptAllowed)fighter->allow_interrupt=value->as.u64!=0;
    else if(semantic==NativeScriptBinding_ArticlesAttachedVisible)fighter->x221E_b4=value->as.u64!=0;
    else fighter->x2223_b4=value->as.u64!=0;
    return true;
}
