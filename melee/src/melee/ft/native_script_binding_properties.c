#include "native_script_bindings.h"
#include <melee/gr/stage.h>
#include "native_script_binding_colors.h"
#include "native_script_binding_fighter_items.h"
#include "native_script_binding_fighter_hitboxes.h"
#include "native_script_binding_hitlag.h"
#include "native_script_binding_held_items.h"
#include "native_script_binding_absorber.h"
#include "native_script_binding_ground_controls.h"
#include "native_script_binding_item_observations.h"
#include "native_script_binding_presentation.h"
#include "native_script_binding_visuals.h"
#include "native_script_binding_item.h"
#include "native_script_binding_spawn.h"
#include "native_script_binding_effect.h"
#include "native_script_binding_actions.h"
#include "native_script_binding_jump.h"
#include "native_script_binding_held_state.h"
#include "native_script_binding_cpu_control.h"
#include "native_script_binding_article_combat.h"
#include "native_script_binding_item_model.h"
#include "native_script_binding_item_resources.h"
#include "native_script_binding_item_model_resolve.h"
#include "native_script_binding_article_motion.h"
#include "native_script_binding_common_properties.h"
#include "native_script_binding_guard.h"
#include "native_script_binding_fighter_observations.h"
#include "native_script_binding_capture.h"
#include "native_script_expression.h"
#include "native_callback_lifetime.h"
#include "native_source_package.h"
#include "ftdata.h"
#include "types.h"
#include <melee/it/types.h>
#include <melee/it/native_item_lifetime.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/controller.h>

#include <stdio.h>

static bool error(NativeScriptDiagnostic* diagnostic,const char* message)
{
    if (diagnostic) {
        diagnostic->source=(NativeScriptSourceSpan){0};
        snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",message);
    }
    return false;
}

bool nativeScriptBindings_SourceAlive(NativeScriptRuntimeContext* context)
{
    if (!context || !nativeCallbackSource_Alive(context->source,context->generation,context->source_class))return false;
    HSD_GObj* object=context->source;
    if (context->source_class==NativeCallbackSource_Fighter)
        return ((Fighter*)object->user_data)->gobj==object;
    if (context->source_class==NativeCallbackSource_Item)
        return ((Item*)object->user_data)->entity==object;
    return context->source_class==NativeCallbackSource_ModelEffect;
}

const NativeScriptMoveLayout* nativeScriptBindings_SourceFighterLayout(
    NativeScriptRuntimeContext* context,NativeScriptDiagnostic* diagnostic)
{
    if(!context || context->source_class!=NativeCallbackSource_Fighter ||
       !nativeScriptBindings_SourceAlive(context)) {
        error(diagnostic,"Owned fighter source is unavailable or retired.");return NULL;
    }
    HSD_GObj* object=context->source;Fighter* fighter=object->user_data;
    const NativeSourcePackage* source=nativeFighterRuntime_SourcePackage(fighter->kind);
    const NativeScriptFighterProgram* selected=nativeScriptFighter_Program(fighter->kind);
    NativeScriptReceiverView view;
    if(!source || source->address!=source || !source->ready || !source->assets.ready ||
       source->target!=fighter->kind || fighter->kind<Ft_Kind_CustomFirst ||
       fighter->kind>=Ft_Kind_RuntimeCapacity || fighter->behavior_kind!=fighter->kind ||
       fighter->ft_data!=&source->assets.character.data ||
       gFtDataList[fighter->kind]!=&source->assets.character.data ||
       ftData_CharacterStateTables[fighter->kind]!=source->assets.motions ||
       !selected || selected->bank!=&source->compiler.bank ||
       selected->identity!=source->program.identity || !selected->bank->valid ||
       !selected->bank->has_motion_layout ||
       nativeScriptFighter_Status(object)!=NativeScriptVM_Ok ||
       !nativeScriptFighter_View(object,context->generation,&view) ||
       view.program!=&selected->bank->declarations || !view.storage ||
       !view.storage->live || view.storage->owner!=0 ||
       view.storage->generation!=context->generation ||
       view.storage->layout!=&selected->bank->storage_layout ||
       view.storage->parameters!=&selected->bank->storage_parameters) {
        error(diagnostic,"Fighter operation requires its live selected source program and owned data.");return NULL;
    }
    const NativeScriptMoveLayout* layout=&selected->bank->motion_layout;
    const NativeScriptMoveLayout* facts=&source->assets.facts.moves;
    if(layout->common_count!=facts->common_count || layout->first!=facts->first ||
       layout->count!=facts->count) {
        error(diagnostic,"Selected source motion layout differs from its imported table.");return NULL;
    }
    for(unsigned i=0;i<NativeScriptMove_Count;++i)
        if(layout->ground[i]!=facts->ground[i] || layout->air[i]!=facts->air[i] ||
           layout->transition_flags[i]!=facts->transition_flags[i]) {
            error(diagnostic,"Selected named motions differ from owned source facts.");return NULL;
        }
    return layout;
}

bool nativeScriptBindings_Context(void* actor,uint32_t source_class,uint32_t generation,
    NativeScriptRuntimeContext* result,NativeScriptDiagnostic* diagnostic)
{
    if (!result)return error(diagnostic,"Missing native source context output.");
    NativeScriptRuntimeContext value={.source=actor,.source_class=source_class,
        .generation=generation,.actor_id=generation};
    if (!nativeScriptBindings_SourceAlive(&value))return error(diagnostic,"Native script source is absent, retired or has a different generation.");
    *result=value;return true;
}

static const NativeScriptBindingDescriptor* property(NativeScriptRuntimeContext* context,
    uint32_t semantic,uint32_t selector_count,bool write,NativeScriptDiagnostic* diagnostic)
{
    const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(semantic);
    unsigned mask=context&&context->source_class==NativeCallbackSource_Fighter?NativeScriptBindings_Fighter:
        context&&context->source_class==NativeCallbackSource_Item?NativeScriptBindings_Article:
        context&&context->source_class==NativeCallbackSource_ModelEffect?NativeScriptBindings_Effect:0;
    if (!descriptor || (descriptor->kind!=NativeScriptBinding_Property && descriptor->kind!=NativeScriptBinding_Constant) || !(descriptor->contexts&mask) || selector_count) {
        error(diagnostic,"Native property or source context is unsupported.");return NULL;
    }
    if (!(descriptor->flags&(write?NativeScriptFlag_Writable:NativeScriptFlag_Readable))) {
        error(diagnostic,write?"Native property is read-only.":"Native property is not readable.");return NULL;
    }
    if (!nativeScriptBindings_SourceAlive(context)) {
        error(diagnostic,"Native property source has retired.");return NULL;
    }
    if(semantic>=NativeScriptBinding_ItemSlideVelocityX&&semantic<=NativeScriptBinding_ItemSlideVelocityZ) {
        NativeItemLifetime source;
        if(!nativeItemLifetime_Capture(context->source,&source) || !source.source_owned || source.removing ||
           source.reference.lifetime!=context->scene_epoch || source.generation!=context->generation) {
            error(diagnostic,"Slide velocity requires its live owned Item in the current scene");return NULL;
        }
    }
    return descriptor;
}

static float* vector_component(NativeScriptRuntimeContext* context,uint32_t semantic)
{
    HSD_GObj* object=context->source;
    bool fighter=context->source_class==NativeCallbackSource_Fighter;
    Fighter* fp=fighter?object->user_data:NULL;
    Item* item=fighter?NULL:object->user_data;
    Vec3* vector;unsigned axis;
    if (semantic>=NativeScriptBinding_PositionX&&semantic<=NativeScriptBinding_PositionZ) {
        vector=fighter?&fp->cur_pos:&item->pos;axis=semantic-NativeScriptBinding_PositionX;
    } else if (semantic>=NativeScriptBinding_PositionDeltaX&&semantic<=NativeScriptBinding_PositionDeltaZ) {
        vector=&fp->pos_delta;axis=semantic-NativeScriptBinding_PositionDeltaX;
    } else if (semantic>=NativeScriptBinding_VelocityX&&semantic<=NativeScriptBinding_VelocityZ) {
        vector=fighter?&fp->self_vel:&item->x40_vel;axis=semantic-NativeScriptBinding_VelocityX;
    } else if (semantic>=NativeScriptBinding_ItemSlideVelocityX&&semantic<=NativeScriptBinding_ItemSlideVelocityZ) {
        vector=&item->x7C;axis=semantic-NativeScriptBinding_ItemSlideVelocityX;
    } else if (semantic>=NativeScriptBinding_AccelerationX&&semantic<=NativeScriptBinding_AccelerationZ) {
        vector=&fp->x74_self_accel;axis=semantic-NativeScriptBinding_AccelerationX;
    } else return NULL;
    return axis==0?&vector->x:axis==1?&vector->y:&vector->z;
}

bool nativeScriptBindings_Read(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* selectors,uint32_t selector_count,
    NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    if((semantic>=NativeScriptBinding_HitboxCurrentX&&semantic<=NativeScriptBinding_HitboxCurrentZ)||
       (semantic>=NativeScriptBinding_HitboxPreviousX&&semantic<=NativeScriptBinding_HitboxPreviousZ))
        return nativeScriptBindingFighterHitboxes_Read(context,semantic,selectors,selector_count,result,diagnostic);
    if((semantic&NATIVE_SCRIPT_ITEM_RESOURCES_DOMAIN_MASK)==NATIVE_SCRIPT_ITEM_RESOURCES_DOMAIN)
        return nativeScriptBindingItemResources_Read(context,semantic,selectors,selector_count,result,diagnostic);
    if((semantic&NATIVE_SCRIPT_EFFECT_DOMAIN_MASK)==NATIVE_SCRIPT_EFFECT_DOMAIN)return nativeScriptBindingEffect_Read(context,semantic,selectors,selector_count,result,diagnostic);
    if((semantic&NATIVE_SCRIPT_ITEM_MODEL_DOMAIN_MASK)==NATIVE_SCRIPT_ITEM_MODEL_DOMAIN)
        return nativeScriptBindingItemModel_Read(context,semantic,selectors,selector_count,result,diagnostic);
    if(context && context->source_class==NativeCallbackSource_Item &&
       semantic>=NativeScriptBinding_CollisionFloorContact && semantic<=NativeScriptBinding_CollisionRightWall)
        return nativeScriptBindingArticleCombat_Read(context,semantic,selectors,selector_count,result,diagnostic);
    if(semantic==NativeScriptBinding_ItemAttachmentActive||semantic==NativeScriptBinding_ItemCurrentOwner||
       semantic==NativeScriptBinding_ItemContactFighter||semantic==NativeScriptBinding_ItemReflector||semantic==NativeScriptBinding_ItemReflectionOwnerTransferDisabled)
        return nativeScriptBindingArticleMotion_Read(context,semantic,selectors,selector_count,result,diagnostic);
    (void)selectors;
    if (!result)return error(diagnostic,"Missing native property output.");
    if (semantic==NativeScriptBinding_Command) {
        if (!context||context->source_class!=NativeCallbackSource_Fighter||!nativeScriptBindings_SourceAlive(context))return error(diagnostic,"Command source is absent or has retired.");
        return nativeScriptBindingSetup_Read(context,semantic,selectors,selector_count,result,diagnostic);
    }
    const NativeScriptBindingDescriptor* descriptor=property(context,semantic,selector_count,false,diagnostic);
    if (!descriptor)return false;
    NativeScriptValue value={.type=descriptor->result_type};
    float* coordinate=descriptor->kind==NativeScriptBinding_Property?vector_component(context,semantic):NULL;
    if (coordinate) {
        // property() already admitted this live source. Address selection and
        // this scalar field load perform no callbacks or engine reentry.
        value.as.f32=*coordinate;
        *result=value;
        return true;
    }
    else {
        HSD_GObj* object=context->source;
        Fighter* fp=context->source_class==NativeCallbackSource_Fighter?object->user_data:NULL;
        switch (semantic) {
        case NativeScriptBinding_StageInternalKind:value.as.i64=(int32_t)Stage_80225194();break;
        case NativeScriptBinding_CeilingNormalZ:
        case NativeScriptBinding_WallLeftNormalZ:
        case NativeScriptBinding_WallRightNormalZ:
            if(context->source_class==NativeCallbackSource_Item) {
                if(!nativeScriptBindingItemObservations_Read(context,semantic,&value,diagnostic))return false;
            } else if(!nativeScriptBindingCommonProperties_Read(context,semantic,&value,diagnostic))return false;
            break;
        case NativeScriptBinding_FloorIsPlatform:
            if(!nativeScriptBindingFighterObservations_Read(context,semantic,&value,diagnostic))return false;
            break;
        case NativeScriptBinding_SmashChargeMode:
        case NativeScriptBinding_SmashChargeFrames:
        case NativeScriptBinding_SmashChargeLimit:
        case NativeScriptBinding_SmashSavedAnimationRate:
        case NativeScriptBinding_SmashChargeColor:
        case NativeScriptBinding_SmashShakeIndex:
        case NativeScriptBinding_SmashShakePeriod:
        case NativeScriptBinding_SmashShakeTableCount:
            if(!nativeScriptBindingCommonProperties_Read(context,semantic,&value,diagnostic))return false;
            break;
        case NativeScriptBinding_HitlagActive:case NativeScriptBinding_HitlagLinkedId:
            if(!nativeScriptBindingHitlag_Read(context,semantic,&value,diagnostic))return false;
            break;
        case NativeScriptBinding_VisualsHeldItemVisible:
            if(!nativeScriptBindingVisuals_Read(context,semantic,&value,diagnostic))return false;
            break;
        case NativeScriptBinding_ControlsHorizontalSmashThreshold:
        case NativeScriptBinding_ControlsHorizontalSmashWindow:
        case NativeScriptBinding_ControlsHorizontalSmashExtension:
        case NativeScriptBinding_ShieldStartHealth:
        case NativeScriptBinding_ShieldMinimumHoldDuration:
        case NativeScriptBinding_ShieldReflectDuration:
        case NativeScriptBinding_ShieldReflectRadius:
        case NativeScriptBinding_ShieldReflectDamageMultiplier:
        case NativeScriptBinding_ShieldReflectSpeedMultiplier:
        case NativeScriptBinding_ShieldPowershieldDuration:
        case NativeScriptBinding_ShieldPushbackDamageScale:
        case NativeScriptBinding_AirDriftThreshold:
        case NativeScriptBinding_ShieldPushbackBase:
        case NativeScriptBinding_ShieldPushbackVelocityScale:
        case NativeScriptBinding_ShieldPushbackLightMin:
        case NativeScriptBinding_ShieldPushbackLightMax:
            if(!nativeScriptBindingCommonProperties_Read(context,semantic,&value,diagnostic))return false;
            break;
        case NativeScriptBinding_Pi:value.as.f64=3.14159265358979323846;break;
        case NativeScriptBinding_Tau:value.as.f64=6.28318530717958647692;break;
        case NativeScriptBinding_PiHalf:value.as.f64=3.14159265358979323846/2.0;break;
        case NativeScriptBinding_InputX:value.as.f32=fp->input.lstick[0].x;break;
        case NativeScriptBinding_InputY:value.as.f32=fp->input.lstick[0].y;break;
        case NativeScriptBinding_InputSpecialHeld:value.as.u64=(fp->input.held_buttons[0]&HSD_PAD_B)!=0;break;
        case NativeScriptBinding_InputSpecialPressed:value.as.u64=(fp->input.pressed_buttons&HSD_PAD_B)!=0;break;
        case NativeScriptBinding_Frame:value.as.f32=fp->cur_anim_frame;break;
        case NativeScriptBinding_FrameInteger: {
            NativeScriptValue frame={.type=NativeScriptType_F32,.as.f32=fp->cur_anim_frame};
            if (!nativeScriptValue_Convert(&frame,NativeScriptType_I32,&value,diagnostic))return false;
            break;
        }
        case NativeScriptBinding_FacingDirection:value.as.f32=fp->facing_dir;break;
        case NativeScriptBinding_FacingAnimationDirection:case NativeScriptBinding_WalkType:
        case NativeScriptBinding_FighterPlayerSlot:case NativeScriptBinding_InputDownPressAge:
        case NativeScriptBinding_ControlsThrowHorizontalThreshold:case NativeScriptBinding_ControlsThrowUpThreshold:case NativeScriptBinding_ControlsThrowDownThreshold:
            if(!nativeScriptBindingFighterObservations_Read(context,semantic,&value,diagnostic))return false;
            break;
        case NativeScriptBinding_ArticleFacing:value.as.f32=((Item*)object->user_data)->facing_dir;break;
        case NativeScriptBinding_Grounded:value.as.u64=fp->ground_or_air==GA_Ground;break;
        case NativeScriptBinding_Airborne:value.as.u64=fp->ground_or_air==GA_Air;break;
        case NativeScriptBinding_CaptureGroundLink:case NativeScriptBinding_CaptureHasVictim:case NativeScriptBinding_CaptureHasItem:case NativeScriptBinding_CaptureVictimVisible:case NativeScriptBinding_CaptureVictimDamage:case NativeScriptBinding_CaptureVictimFrame:
        case NativeScriptBinding_CaptureVictimAnimationFamily:
        case NativeScriptBinding_CaptureVictimFreezeState:
        case NativeScriptBinding_CaptureTimerDamageMultiplier:case NativeScriptBinding_CaptureTimerDecay:
        case NativeScriptBinding_CaptureMashReduction:case NativeScriptBinding_CaptureMashPauseFrames:
        case NativeScriptBinding_CaptureMashAnimationRate:case NativeScriptBinding_CaptureWiggleScale:
        case NativeScriptBinding_CaptureWiggleLimit:
        case NativeScriptBinding_CaptureEnclosureModelSlot:
        case NativeScriptBinding_CaptureEnclosureLaunchX:
        case NativeScriptBinding_CaptureEnclosureLaunchY:
        case NativeScriptBinding_CaptureEnclosureLaunchZ:
        case NativeScriptBinding_CaptureEnclosureDamageBehavior:
        case NativeScriptBinding_CaptureEnclosureShrinkFraction:
        case NativeScriptBinding_CaptureEnclosureShrinkFrames:
        case NativeScriptBinding_CaptureEnclosureLifetime:
        case NativeScriptBinding_CaptureEnclosureDamageTimeScale:
        case NativeScriptBinding_CaptureEnclosureEscapeX:
        case NativeScriptBinding_CaptureEnclosureEscapeY:
        case NativeScriptBinding_CaptureEnclosureEscapeZ:
        case NativeScriptBinding_CaptureEnclosureEscapeInvincibilityFrames:
            if(!nativeScriptBindingCapture_Read(context,semantic,&value,diagnostic))return false;
            break;
        case NativeScriptBinding_EffectsActive:value.as.u64=fp->x2219_b0!=0;break;
        case NativeScriptBinding_UsedTether:value.as.u64=fp->used_tether!=0;break;
        case NativeScriptBinding_Fastfall:value.as.u64=fp->fall_fast!=0;break;
        case NativeScriptBinding_InputAnalogShieldAge:value.as.i64=fp->trigger_analog_timer;break;
        case NativeScriptBinding_InputHorizontalTiltAge:value.as.i64=fp->x670_timer_lstick_tilt_x;break;
        case NativeScriptBinding_ItemCreationValue:
            if(!nativeScriptItem_GetCreationValue(context->source,&value.as.f32))
                return error(diagnostic,"Item creation value requires its live source actor");
            break;
        default:
            if(semantic>=NativeScriptBinding_CollisionPositionX&&semantic<=NativeScriptBinding_CollisionPositionZ) {
                if(context->source_class==NativeCallbackSource_Fighter) {
                    if(!nativeScriptBindingFighterObservations_Read(context,semantic,&value,diagnostic))return false;
                } else if(!nativeScriptBindingItemObservations_Read(context,semantic,&value,diagnostic))return false;
            } else if(semantic>=NativeScriptBinding_ItemOutlineColor&&semantic<=NativeScriptBinding_ItemLayered) {
                if(!nativeScriptBindingColors_Read(context,semantic,&value,diagnostic))return false;
            } else if(semantic==NativeScriptBinding_ItemsHeldId||semantic==NativeScriptBinding_ItemsHeldKind||
                      semantic==NativeScriptBinding_ItemsHeldMotion||semantic==NativeScriptBinding_ItemsHeldFrame) {
                if(!nativeScriptBindingHeldItems_Read(context,semantic,&value,diagnostic))return false;
            } else if(semantic>=NativeScriptBinding_AbsorberActive&&semantic<=NativeScriptBinding_AbsorberDirection) {
                if(!nativeScriptBindingAbsorber_Read(context,semantic,&value,diagnostic))return false;
            } else if(semantic==NativeScriptBinding_ReflectorActive||semantic==NativeScriptBinding_ReflectorKeepOwner||semantic==NativeScriptBinding_ReflectorHitDirection||
                      (semantic>=NativeScriptBinding_GuardActive&&semantic<=NativeScriptBinding_ShieldInitialSize)) {
                if(!nativeScriptBindingGuard_Read(context,semantic,&value,diagnostic))return false;
            } else if(semantic==NativeScriptBinding_FloorFriction) {
                if(context->source_class==NativeCallbackSource_Item) {
                    if(!nativeScriptBindingItemObservations_Read(context,semantic,&value,diagnostic))return false;
                } else if(!nativeScriptBindingFighterObservations_Read(context,semantic,&value,diagnostic))return false;
            } else if(semantic==NativeScriptBinding_MovementSpecialLandingLag||semantic==NativeScriptBinding_EngineDebugLevel||
                      semantic==NativeScriptBinding_ActionCategory||semantic==NativeScriptBinding_ActionCurrentMetadata||semantic==NativeScriptBinding_FighterIncapacitated||
                      (semantic>=NativeScriptBinding_GroundWalkAccelerationMultiplier&&semantic<=NativeScriptBinding_AirJumpHorizontalMax)||
                      semantic==NativeScriptBinding_MotionAllowsControllerSync||semantic==NativeScriptBinding_SpawnHorizontalOffset||semantic==NativeScriptBinding_GroundMidWalkPoint) {
                if(!nativeScriptBindingFighterObservations_Read(context,semantic,&value,diagnostic))return false;
            } else if(semantic==NativeScriptBinding_StatsLandingAirNeutralLag||semantic==NativeScriptBinding_StatsLandingAirBackLag) {
                if(!nativeScriptBindingGroundControls_Read(context,semantic,selectors,selector_count,&value,diagnostic))return false;
            } else if(semantic==NativeScriptBinding_ItemAnimationFinished ||
                      (semantic>=NativeScriptBinding_ItemLaunchAngle&&semantic<=NativeScriptBinding_ItemReflectionFacing) ||
                      (semantic>=NativeScriptBinding_ItemSurfaceNormalX&&semantic<=NativeScriptBinding_ItemSurfaceNormalZ) ||
                      (context->source_class==NativeCallbackSource_Item &&
                       ((semantic>=NativeScriptBinding_FloorNormalX && semantic<=NativeScriptBinding_FloorNormalZ)||
                        (semantic>=NativeScriptBinding_CeilingNormalX && semantic<=NativeScriptBinding_CeilingNormalY)||
                        (semantic>=NativeScriptBinding_WallLeftNormalX && semantic<=NativeScriptBinding_WallRightNormalY)))) {
                if(!nativeScriptBindingItemObservations_Read(context,semantic,&value,diagnostic))return false;
            } else if(semantic==NativeScriptBinding_ItemsAuxiliaryId||semantic==NativeScriptBinding_ItemsHeldUseCategory||semantic==NativeScriptBinding_ItemsHoldKind||semantic==NativeScriptBinding_ItemsReopenBlocked||semantic==NativeScriptBinding_ItemsRejectHealingPickup||semantic==NativeScriptBinding_ItemsHeavy) {
                if(!nativeScriptBindingFighterItems_Read(context,semantic,&value,diagnostic))return false;
            } else if(semantic==NativeScriptBinding_ModelBaseScale||semantic==NativeScriptBinding_ModelRootScaleY||semantic==NativeScriptBinding_ControlsHorizontalStickDeadzone||
                      semantic==NativeScriptBinding_FighterActionDetachesItems) {
                if(!nativeScriptBindingCommonProperties_Read(context,semantic,&value,diagnostic))return false;
            } else if(semantic==NativeScriptBinding_CostumeIndex ||
                      (semantic>=NativeScriptBinding_VisualsOutlineColor&&semantic<=NativeScriptBinding_ModelInstanceScaleZ)) {
                if(!nativeScriptBindingColors_Read(context,semantic,&value,diagnostic))return false;
            } else if((semantic>=NativeScriptBinding_CPUPendingAction&&semantic<=NativeScriptBinding_CPUDestinationY)||
                      (semantic>=NativeScriptBinding_CPUApproachSpacing&&semantic<=NativeScriptBinding_CPURangedGateEnabled)||
                      (semantic>=NativeScriptBinding_CPUOutputLeftX&&semantic<=NativeScriptBinding_CPUOutputButtons)) {
                if(!nativeScriptBindingCPUControl_Read(context,semantic,&value,diagnostic))return false;
            } else if(semantic==NativeScriptBinding_CaptureArmed||semantic==NativeScriptBinding_AnimationHipRootCompensation||semantic==NativeScriptBinding_DamageForceTopBlastKO||
                      (semantic>=NativeScriptBinding_AirTerminalVelocity&&semantic<=NativeScriptBinding_FacingSign) ||
                      (semantic>=NativeScriptBinding_ControlsAerialJumpReverseThreshold&&semantic<=NativeScriptBinding_ControlsAerialActionDownThreshold)) {
                if(!nativeScriptBindingCommonProperties_Read(context,semantic,&value,diagnostic))return false;
            } else if(semantic==NativeScriptBinding_DamageReverseKnockdownPose) {
                if(!nativeScriptBindingCombat_Read(context,semantic,&value,diagnostic))return false;
            } else if(semantic==NativeScriptBinding_MotionFallAnimation || semantic==NativeScriptBinding_JumpsUsed || semantic==NativeScriptBinding_JumpsMax ||
               semantic==NativeScriptBinding_CollisionBasicLandingVelocityThreshold) {
                if(!nativeScriptBindingJump_Read(context,semantic,&value,diagnostic))return false;
            } else if(semantic==NativeScriptBinding_BonesRootRotationY) {
                if(!nativeScriptBindingPresentation_Read(context,semantic,&value,diagnostic))return false;
            } else if(semantic>=NativeScriptBinding_ECBUp&&semantic<=NativeScriptBinding_ECBBack) {
                if(!nativeScriptBindingItem_Read(context,semantic,&value,diagnostic))return false;
            } else if(semantic>=NativeScriptBinding_CPU) {
                if(!nativeScriptBindingActions_Read(context,semantic,&value,diagnostic))return false;
            } else if(semantic>=NativeScriptBinding_Articles) {
                if(!nativeScriptBindingSpawn_Read(context,semantic,&value,diagnostic))return false;
            } else if(semantic>=NativeScriptBinding_Item) {
                if(!nativeScriptBindingItem_Read(context,semantic,&value,diagnostic))return false;
            } else if(semantic>=NativeScriptBinding_Sound) {
                if(!nativeScriptBindingPresentation_Read(context,semantic,&value,diagnostic))return false;
            } else if (semantic>=NativeScriptBinding_Damage) {
                if (!nativeScriptBindingCombat_Read(context,semantic,&value,diagnostic))return false;
            } else if (semantic>=NativeScriptBinding_Fighter) {
                if (!nativeScriptBindingSetup_Read(context,semantic,NULL,0,&value,diagnostic))return false;
            } else if (semantic>=NativeScriptBinding_State) {
                if (!nativeScriptBindingState_Read(context,semantic,&value,diagnostic))return false;
            } else if (!nativeScriptBindingKinetics_Read(context,semantic,&value,diagnostic))return false;
            break;
        }
    }
    // All current operations are pure field observations; retaining the guard
    // also keeps output publication explicit when this family is extended.
    if (!nativeScriptBindings_SourceAlive(context))return error(diagnostic,"Native property source retired before publication.");
    *result=value;return true;
}

bool nativeScriptBindings_Write(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* selectors,uint32_t selector_count,
    const NativeScriptValue* input,NativeScriptDiagnostic* diagnostic)
{
    if((semantic>=NativeScriptBinding_HitboxCurrentX&&semantic<=NativeScriptBinding_HitboxCurrentZ)||
       (semantic>=NativeScriptBinding_HitboxPreviousX&&semantic<=NativeScriptBinding_HitboxPreviousZ))
        return nativeScriptBindingFighterHitboxes_Write(context,semantic,selectors,selector_count,input,diagnostic);
    if((semantic&NATIVE_SCRIPT_ITEM_RESOURCES_DOMAIN_MASK)==NATIVE_SCRIPT_ITEM_RESOURCES_DOMAIN)
        return nativeScriptBindingItemResources_Write(context,semantic,selectors,selector_count,input,diagnostic);
    if((semantic&NATIVE_SCRIPT_EFFECT_DOMAIN_MASK)==NATIVE_SCRIPT_EFFECT_DOMAIN)return nativeScriptBindingEffect_Write(context,semantic,selectors,selector_count,input,diagnostic);
    if((semantic&NATIVE_SCRIPT_ITEM_MODEL_DOMAIN_MASK)==NATIVE_SCRIPT_ITEM_MODEL_DOMAIN)
        return nativeScriptBindingItemModel_Write(context,semantic,selectors,selector_count,input,diagnostic);
    (void)selectors;
    if (semantic==NativeScriptBinding_Command) {
        if (!context||context->source_class!=NativeCallbackSource_Fighter||!nativeScriptBindings_SourceAlive(context))return error(diagnostic,"Command source is absent or has retired.");
        return nativeScriptBindingSetup_Write(context,semantic,selectors,selector_count,input,diagnostic);
    }
    const NativeScriptBindingDescriptor* descriptor=property(context,semantic,selector_count,true,diagnostic);
    if (!descriptor)return false;
    if (!input || !nativeScriptExpression_Assignable(descriptor->result_type,input->type))
        return error(diagnostic,"Native property value type is not assignable.");
    if(semantic==NativeScriptBinding_MotionFallAnimation&&
       ((input->type==NativeScriptType_I32||input->type==NativeScriptType_I64)?
        input->as.i64<INT32_MIN||input->as.i64>INT32_MAX:
        (input->type==NativeScriptType_U32||input->type==NativeScriptType_U64)&&input->as.u64>INT32_MAX))
        return error(diagnostic,"Fall animation selector exceeds its native signed Int.");
    if(semantic==NativeScriptBinding_InputHorizontalSmashAge||
       semantic==NativeScriptBinding_InputAnalogShieldAge||semantic==NativeScriptBinding_InputHorizontalTiltAge){
        // Validate before Int32 conversion: large Int64/UInt64 values must not
        // wrap into an apparently valid timer. Native input saturates at 0xFE.
        bool valid=false;
        switch(input->type){
        case NativeScriptType_I32:case NativeScriptType_I64:valid=input->as.i64>=0&&input->as.i64<=0xFE;break;
        case NativeScriptType_U32:case NativeScriptType_U64:valid=input->as.u64<=0xFE;break;
        case NativeScriptType_Bool:valid=input->as.u64<=1;break;
        default:break;
        }
        if(!valid)return error(diagnostic,"Input age requires an integer from 0 through 254");
    }
    NativeScriptValue value;
    if (!nativeScriptValue_Convert(input,descriptor->result_type,&value,diagnostic))return false;
    float* coordinate=vector_component(context,semantic);
    if (coordinate)*coordinate=value.as.f32;
    else if(semantic>=NativeScriptBinding_SmashChargeMode && semantic<=NativeScriptBinding_SmashShakePeriod) {
        if(!nativeScriptBindingCommonProperties_Write(context,semantic,&value,diagnostic))return false;
    } else {
        HSD_GObj* object=context->source;
        Fighter* fp=context->source_class==NativeCallbackSource_Fighter?object->user_data:NULL;
        bool flag=value.as.u64!=0;
        switch (semantic) {
        case NativeScriptBinding_CaptureArmed:
        case NativeScriptBinding_AnimationHipRootCompensation:
        case NativeScriptBinding_DamageForceTopBlastKO:
            if(!nativeScriptBindingCommonProperties_Write(context,semantic,&value,diagnostic))return false;
            break;
        case NativeScriptBinding_MotionFallAnimation:
            if(!nativeScriptBindingJump_Write(context,semantic,&value,diagnostic))return false;
            break;
        case NativeScriptBinding_DamageReverseKnockdownPose:
            if(!nativeScriptBindingCombat_Write(context,semantic,&value,diagnostic))return false;
            break;
        case NativeScriptBinding_MotionID:
            if(!nativeScriptBindingHeldState_Write(context,semantic,&value,diagnostic))return false;
            break;
        case NativeScriptBinding_FacingDirection:fp->facing_dir=value.as.f32;break;
        case NativeScriptBinding_ArticleFacing:((Item*)object->user_data)->facing_dir=value.as.f32;break;
        case NativeScriptBinding_Airborne:fp->ground_or_air=flag?GA_Air:GA_Ground;break;
        case NativeScriptBinding_CaptureGroundLink:
        case NativeScriptBinding_CaptureVictimVisible:
        case NativeScriptBinding_CaptureTimerDamageMultiplier:case NativeScriptBinding_CaptureTimerDecay:
        case NativeScriptBinding_CaptureMashReduction:case NativeScriptBinding_CaptureMashPauseFrames:
        case NativeScriptBinding_CaptureMashAnimationRate:case NativeScriptBinding_CaptureWiggleScale:
        case NativeScriptBinding_CaptureWiggleLimit:
        case NativeScriptBinding_CaptureEnclosureModelSlot:
        case NativeScriptBinding_CaptureEnclosureLaunchX:
        case NativeScriptBinding_CaptureEnclosureLaunchY:
        case NativeScriptBinding_CaptureEnclosureLaunchZ:
        case NativeScriptBinding_CaptureEnclosureDamageBehavior:
        case NativeScriptBinding_CaptureEnclosureShrinkFraction:
        case NativeScriptBinding_CaptureEnclosureShrinkFrames:
        case NativeScriptBinding_CaptureEnclosureLifetime:
        case NativeScriptBinding_CaptureEnclosureDamageTimeScale:
        case NativeScriptBinding_CaptureEnclosureEscapeX:
        case NativeScriptBinding_CaptureEnclosureEscapeY:
        case NativeScriptBinding_CaptureEnclosureEscapeZ:
        case NativeScriptBinding_CaptureEnclosureEscapeInvincibilityFrames:
            return nativeScriptBindingCapture_Write(context,semantic,&value,diagnostic);
        case NativeScriptBinding_EffectsActive:fp->x2219_b0=flag;break;
        case NativeScriptBinding_UsedTether:fp->used_tether=flag;break;
        case NativeScriptBinding_Fastfall:fp->fall_fast=flag;break;
        case NativeScriptBinding_InputHorizontalSmashAge:fp->x673=(u8)value.as.i64;break;
        case NativeScriptBinding_InputAnalogShieldAge:fp->trigger_analog_timer=(u8)value.as.i64;break;
        case NativeScriptBinding_InputHorizontalTiltAge:fp->x670_timer_lstick_tilt_x=(u8)value.as.i64;break;
        default:
            if(semantic==NativeScriptBinding_SpawnHorizontalOffset) {
                if(!nativeScriptBindingFighterObservations_Write(context,semantic,&value,diagnostic))return false;
            } else if((semantic>=NativeScriptBinding_VisualsOutlineColor&&semantic<=NativeScriptBinding_ModelInstanceScaleZ) ||
               (semantic>=NativeScriptBinding_ItemOutlineColor&&semantic<=NativeScriptBinding_ItemLayered)) {
                if(!nativeScriptBindingColors_Write(context,semantic,&value,diagnostic))return false;
            } else if(semantic>=NativeScriptBinding_AbsorberActive&&semantic<=NativeScriptBinding_AbsorberSkipPositionUpdate) {
                if(!nativeScriptBindingAbsorber_Write(context,semantic,&value,diagnostic))return false;
            } else if(semantic==NativeScriptBinding_ReflectorKeepOwner||
                      (semantic>=NativeScriptBinding_GuardActive&&semantic<=NativeScriptBinding_GuardArticleHitlag)||
                      (semantic>=NativeScriptBinding_GuardElapsed&&semantic<=NativeScriptBinding_ShieldHealth)) {
                if(!nativeScriptBindingGuard_Write(context,semantic,&value,diagnostic))return false;
            } else if(semantic>=NativeScriptBinding_ItemSurfaceNormalX&&semantic<=NativeScriptBinding_ItemSurfaceNormalZ) {
                if(!nativeScriptBindingItemObservations_Write(context,semantic,&value,diagnostic))return false;
            } else if(semantic==NativeScriptBinding_ItemsReopenBlocked||semantic==NativeScriptBinding_ItemsRejectHealingPickup) {
                if(!nativeScriptBindingFighterItems_Write(context,semantic,&value,diagnostic))return false;
            } else if(semantic==NativeScriptBinding_CPUPendingAction||semantic==NativeScriptBinding_CPUBehavior||semantic==NativeScriptBinding_CPULevel||
                      semantic==NativeScriptBinding_CPUTravelFrames||semantic==NativeScriptBinding_CPUSelectedCommand||
                      (semantic>=NativeScriptBinding_CPUMode&&semantic<=NativeScriptBinding_CPURangedGateEnabled)||
                      (semantic>=NativeScriptBinding_CPUOutputLeftX&&semantic<=NativeScriptBinding_CPUOutputButtons)) {
                if(!nativeScriptBindingCPUControl_Write(context,semantic,&value,diagnostic))return false;
            } else if(semantic==NativeScriptBinding_ItemsBlockGroundPickup || semantic==NativeScriptBinding_ArticlesAttachedVisible || semantic==NativeScriptBinding_ControlsInterruptAllowed) {
                if(!nativeScriptBindingCommonProperties_Write(context,semantic,&value,diagnostic))return false;
            } else if(semantic==NativeScriptBinding_JumpsUsed) {
                if(!nativeScriptBindingJump_Write(context,semantic,&value,diagnostic))return false;
            } else if(semantic>=NativeScriptBinding_ECBUp&&semantic<=NativeScriptBinding_ECBBack) {
                if(!nativeScriptBindingItem_Write(context,semantic,&value,diagnostic))return false;
            } else if(semantic>=NativeScriptBinding_Item) {
                if(!nativeScriptBindingItem_Write(context,semantic,&value,diagnostic))return false;
            } else if (semantic>=NativeScriptBinding_Damage) {
                if (!nativeScriptBindingCombat_Write(context,semantic,&value,diagnostic))return false;
            } else if (!nativeScriptBindingKinetics_Write(context,semantic,&value,diagnostic))return false;
            break;
        }
    }
    return true;
}
