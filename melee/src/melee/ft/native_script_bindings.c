#include "native_script_bindings.h"
#include "native_script_binding_normal_attacks.h"
#include "native_script_binding_fighter_items.h"
#include "native_script_binding_absorber.h"
#include "native_script_binding_guard.h"
#include "native_script_binding_capture.h"
#include "native_script_binding_air_inputs.h"
#include "native_script_binding_air_physics.h"
#include "native_script_binding_fighter_observations.h"
#include "native_script_binding_contact.h"
#include "native_script_binding_held_state.h"
#include "native_script_binding_colors.h"
#include "native_script_expression.h"
#include "native_script_math.h"
#include "native_callback_lifetime.h"
#include "native_script_program.h"
#include "native_script_binding_transition.h"
#include "native_script_binding_presentation.h"
#include "native_script_constants.h"
#include "native_script_binding_item.h"
#include "native_script_binding_item_lifecycle.h"
#include "native_script_binding_spawn.h"
#include "native_script_binding_effect.h"
#include "native_script_binding_stage.h"
#include "native_script_binding_family.h"
#include "native_script_binding_actions.h"
#include "native_script_binding_ledge.h"
#include "native_script_binding_jump.h"
#include "native_script_binding_forms.h"
#include "native_script_binding_pose.h"
#include "native_script_binding_common_movement.h"
#include "native_script_binding_visuals.h"
#include "native_script_binding_article_feedback.h"
#include "native_script_binding_signal_resolve.h"
#include "native_script_binding_signal.h"
#include "native_script_binding_article_reference_resolve.h"
#include "native_script_binding_vectors.h"
#include "native_script_binding_article_lifecycle.h"
#include "native_script_binding_cpu_control.h"
#include "native_script_binding_targets.h"
#include "native_script_binding_item_model.h"
#include "native_script_binding_item_resources.h"
#include "native_script_binding_item_animation.h"
#include "native_script_binding_item_model_resolve.h"
#include "native_script_binding_article_combat.h"
#include "native_script_binding_article_motion.h"
#include "native_script_binding_world_feedback.h"
#include "native_script_binding_fighter_hitboxes.h"
#include "native_script_binding_hitbox_create.h"
#include "native_script_binding_hitlag.h"
#include "native_script_binding_ground_controls.h"
#include "native_script_binding_item_observations.h"
#include "native_script_binding_item_links.h"
#include "native_script_binding_item_collision.h"
#include "native_script_binding_item_terrain.h"
#include "native_script_binding_audio.h"
#include "native_script_binding_model_graph.h"
#include "native_script_binding_attachments.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define F NativeScriptBindings_Fighter
#define A NativeScriptBindings_Article
#define ALL NativeScriptBindings_All
#define R NativeScriptFlag_Readable
#define W (NativeScriptFlag_Readable | NativeScriptFlag_Addressable | NativeScriptFlag_Writable)
#define PURE_CALL (NativeScriptFlag_Readable | NativeScriptFlag_Callable | NativeScriptFlag_Pure)
#define NS(id, text, owner, context) [id] = { .name=text, .parent=owner, .kind=NativeScriptBinding_Namespace, .contexts=context, .result_type=NativeScriptType_Namespace }
#define PROP(id, text, owner, context, type, access) [id] = { .name=text, .parent=owner, .flags=access, .kind=NativeScriptBinding_Property, .contexts=context, .result_type=type }
#define ARG(value_type) {.type=value_type, .mode=NativeScriptArgument_Input}
#define MATH1(id, text, context, result, input) [id] = { .name=text, .flags=PURE_CALL, .kind=NativeScriptBinding_Intrinsic, .contexts=context, .result_type=result, .argument_count=1, .arguments={ARG(input)} }
#define MATH2(id, text, context, result, input) [id] = { .name=text, .flags=PURE_CALL, .kind=NativeScriptBinding_Intrinsic, .contexts=context, .result_type=result, .argument_count=2, .arguments={ARG(input),ARG(input)} }
#define CONVERT(id, text, result) [id] = { .name=text, .flags=PURE_CALL, .kind=NativeScriptBinding_Intrinsic, .contexts=ALL, .result_type=result, .argument_count=1, .arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}} }
#define CONSTANT(id, text) [id] = { .name=text, .flags=R, .kind=NativeScriptBinding_Constant, .contexts=ALL, .result_type=NativeScriptType_F64 }
#define KINETIC_CALL (NativeScriptFlag_Readable | NativeScriptFlag_Callable)
#define K0(id, text, owner, result, effects) [id] = { .name=text,.parent=owner,.flags=KINETIC_CALL|effects,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=result }
#define K1(id, text, owner, result, input, effects) [id] = { .name=text,.parent=owner,.flags=KINETIC_CALL|effects,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=result,.argument_count=1,.arguments={ARG(input)} }
#define K2(id, text, owner) [id] = { .name=text,.parent=owner,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)} }
#define KN(id, text) [id] = { .name=text,.parent=NativeScriptBinding_Motion,.flags=KINETIC_CALL,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}} }
#define STATE(id, text, argc, ...) [id]={.name=text,.parent=NATIVE_SCRIPT_STATE_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=argc,.arguments={__VA_ARGS__}}
#define CODE_CALLBACK(id, text) [id]={.name=text,.parent=NativeScriptBinding_Callbacks,.flags=KINETIC_CALL,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={{.type=NativeScriptType_Callable,.mode=NativeScriptArgument_CodeRef,.numeric_any=false}}}

static const NativeScriptBindingDescriptor catalog[NativeScriptBinding_Count] = {
    [NativeScriptBinding_ModelGraphs]={.name="model_graphs",.kind=NativeScriptBinding_Namespace,.contexts=F,.result_type=NativeScriptType_Namespace,.flags=NativeScriptFlag_SourceOnly},
    [NativeScriptBinding_ModelGraphsInstall]={.name="install",.parent=NativeScriptBinding_ModelGraphs,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.result_type=NativeScriptType_Bool,.argument_count=2,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_Shield]={.name="shield",.kind=NativeScriptBinding_Namespace,.contexts=F,.result_type=NativeScriptType_Namespace,.flags=NativeScriptFlag_SourceOnly},
    PROP(NativeScriptBinding_ShieldPushbackDamageScale,"pushback_damage_scale",NativeScriptBinding_Shield,F,NativeScriptType_F32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ShieldPushbackBase,"pushback_base",NativeScriptBinding_Shield,F,NativeScriptType_F32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ShieldPushbackVelocityScale,"pushback_velocity_scale",NativeScriptBinding_Shield,F,NativeScriptType_F32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ShieldPushbackLightMin,"pushback_light_min",NativeScriptBinding_Shield,F,NativeScriptType_F32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ShieldPushbackLightMax,"pushback_light_max",NativeScriptBinding_Shield,F,NativeScriptType_F32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ShieldStartHealth,"start_health",NativeScriptBinding_Shield,F,NativeScriptType_F32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ShieldMinimumHoldDuration,"minimum_hold_duration",NativeScriptBinding_Shield,F,NativeScriptType_F32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ShieldReflectDuration,"reflect_duration",NativeScriptBinding_Shield,F,NativeScriptType_F32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ShieldReflectRadius,"reflect_radius",NativeScriptBinding_Shield,F,NativeScriptType_F32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ShieldReflectDamageMultiplier,"reflect_damage_multiplier",NativeScriptBinding_Shield,F,NativeScriptType_F32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ShieldReflectSpeedMultiplier,"reflect_speed_multiplier",NativeScriptBinding_Shield,F,NativeScriptType_F32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ShieldPowershieldDuration,"powershield_duration",NativeScriptBinding_Shield,F,NativeScriptType_F32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_GuardElapsed,"elapsed",NativeScriptBinding_Guard,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_GuardReleaseRequested,"release_requested",NativeScriptBinding_Guard,F,NativeScriptType_Bool,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_GuardMinimumHoldFrames,"minimum_hold_frames",NativeScriptBinding_Guard,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_GuardReflectFrames,"reflect_frames",NativeScriptBinding_Guard,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_GuardPowershieldFrames,"powershield_frames",NativeScriptBinding_Guard,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_GuardInputLockFrames,"input_lock_frames",NativeScriptBinding_Guard,F,NativeScriptType_I32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_GuardInterruptMode,"interrupt_mode",NativeScriptBinding_Guard,F,NativeScriptType_I32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_GuardDelayedActionFrames,"delayed_action_frames",NativeScriptBinding_Guard,F,NativeScriptType_I32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_GuardVolumeEnabled,"volume_enabled",NativeScriptBinding_Guard,F,NativeScriptType_Bool,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_GuardNewPowershield,"new_powershield",NativeScriptBinding_Guard,F,NativeScriptType_Bool,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_GuardReflectWindow,"reflect_window",NativeScriptBinding_Guard,F,NativeScriptType_Bool,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_GuardPowershieldWindow,"powershield_window",NativeScriptBinding_Guard,F,NativeScriptType_Bool,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ShieldHealth,"health",NativeScriptBinding_Shield,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ShieldLightAmount,"light_amount",NativeScriptBinding_Shield,F,NativeScriptType_F32,R|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ShieldBone,"bone",NativeScriptBinding_Shield,F,NativeScriptType_U32,R|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ShieldInitialSize,"initial_size",NativeScriptBinding_Shield,F,NativeScriptType_F32,R|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ItemsHeavy,"heavy",NativeScriptBinding_Items,F,NativeScriptType_Bool,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    NS(NativeScriptBinding_Family,"family",0,F),
    [NativeScriptBinding_FamilyMemberId]={.name="member_id",.parent=NativeScriptBinding_Family,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_Pure,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_I32,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    K1(NativeScriptBinding_FamilyRetireCompanionOnSleep,"retire_companion_on_sleep",NativeScriptBinding_Family,NativeScriptType_Void,NativeScriptType_Bool,NativeScriptFlag_SourceOnly),
    NS(NativeScriptBinding_Position,"position",0,F|A),
    NS(NativeScriptBinding_PositionDelta,"delta",NativeScriptBinding_Position,F),
    NS(NativeScriptBinding_Velocity,"velocity",0,F|A),
    NS(NativeScriptBinding_Acceleration,"acceleration",0,F),
    NS(NativeScriptBinding_Input,"input",0,F),
    NS(NativeScriptBinding_Facing,"facing",0,F),
    NS(NativeScriptBinding_Effects,"effects",0,F|A),
    NS(NativeScriptBinding_Movement,"movement",0,F),
    NS(NativeScriptBinding_Guard,"guard",0,F),
    [NativeScriptBinding_GuardCreate]={.name="create",.parent=NativeScriptBinding_Guard,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=6,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),{.type=NativeScriptType_Callable,.mode=NativeScriptArgument_CodeRef,.numeric_any=false}}},
    K0(NativeScriptBinding_GuardUpdatePosition,"update_position",NativeScriptBinding_Guard,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_GuardScaleJoint,"scale_joint",NativeScriptBinding_Guard,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_GuardInstallShield,"install_shield",NativeScriptBinding_Guard,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_GuardInstallReflector,"install_reflector",NativeScriptBinding_Guard,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_GuardConsumeReleaseInput,"consume_release_input",NativeScriptBinding_Guard,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_GuardDrain,"drain",NativeScriptBinding_Guard,NativeScriptType_Bool,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_GuardTickPowershield,"tick_powershield",NativeScriptBinding_Guard,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_GuardTrackSDI,"track_sdi",NativeScriptBinding_Guard,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_GuardEnterHold,"enter_hold",NativeScriptBinding_Guard,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_GuardEnterRelease,"enter_release",NativeScriptBinding_Guard,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_ControlsTryPowershield,"try_powershield",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_ControlsTryGuardItemThrow,"try_guard_item_throw",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_ControlsTrySpotdodge,"try_spotdodge",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_ControlsTryGuardSpotdodge,"try_guard_spotdodge",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_ControlsTryRoll,"try_roll",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_ControlsTryGuardPlatformDrop,"try_guard_platform_drop",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_ControlsGuardReflectActions,"guard_reflect_actions",NativeScriptBinding_Controls,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_PhysicsGuardMotion,"guard_motion",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_PhysicsGuardHitMotion,"guard_hit_motion",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_CollisionGuardStandard,"guard_standard",NativeScriptBinding_Collision,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_CollisionGuardHit,"guard_hit",NativeScriptBinding_Collision,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    PROP(NativeScriptBinding_GuardActive,"active",NativeScriptBinding_Guard,F,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_GuardReactive,"reactive",NativeScriptBinding_Guard,F,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_GuardSkipContactDetails,"skip_contact_details",NativeScriptBinding_Guard,F,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_GuardFrontOnly,"front_only",NativeScriptBinding_Guard,F,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_GuardFilteredAttacksOnly,"filtered_attacks_only",NativeScriptBinding_Guard,F,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_GuardMeleeHitlag,"melee_hitlag",NativeScriptBinding_Guard,F,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_GuardArticleHitlag,"article_hitlag",NativeScriptBinding_Guard,F,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_GuardHitDamage,"hit_damage",NativeScriptBinding_Guard,F,NativeScriptType_I32,R),
    PROP(NativeScriptBinding_GuardHitFacing,"hit_facing",NativeScriptBinding_Guard,F,NativeScriptType_F32,R),
    K0(NativeScriptBinding_ControlsAerialAttackMotion,"aerial_attack_motion",NativeScriptBinding_Controls,NativeScriptType_I32,0),
    K0(NativeScriptBinding_InputCStickAttackEdge,"cstick_attack_edge",NativeScriptBinding_Input,NativeScriptType_Bool,0),
    K0(NativeScriptBinding_StateAirGun,"air_gun",NATIVE_SCRIPT_STATE_NAMESPACE,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_ControlsTryAirItemThrow,"try_air_item_throw",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_ControlsTryAirSpecial,"try_air_special",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_PhysicsAnimationVerticalVelocityAdjusted,"animation_vertical_velocity_adjusted",NativeScriptBinding_Physics,NativeScriptType_Void,0),
    K0(NativeScriptBinding_PhysicsAnimationVerticalVelocity,"animation_vertical_velocity",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptFlag_SourceOnly),
    [NativeScriptBinding_CollisionAirLandingCallback]={.name="air_landing_callback",.parent=NativeScriptBinding_Collision,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={{.type=NativeScriptType_Callable,.mode=NativeScriptArgument_CodeRef,.numeric_any=false}}},
    [NativeScriptBinding_EffectsSpawnAsyncAttached]={.name="spawn_async_attached",.parent=NativeScriptBinding_Effects,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_U32)}},
    [NativeScriptBinding_EffectsSpawnAsyncWithScalar]={.name="spawn_async_with_scalar",.parent=NativeScriptBinding_Effects,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_U32),ARG(NativeScriptType_F32)}},
    PROP(NativeScriptBinding_PositionX,"x",NativeScriptBinding_Position,F|A,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_PositionY,"y",NativeScriptBinding_Position,F|A,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_PositionZ,"z",NativeScriptBinding_Position,F|A,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_PositionDeltaX,"x",NativeScriptBinding_PositionDelta,F,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_PositionDeltaY,"y",NativeScriptBinding_PositionDelta,F,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_PositionDeltaZ,"z",NativeScriptBinding_PositionDelta,F,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_VelocityX,"x",NativeScriptBinding_Velocity,F|A,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_VelocityY,"y",NativeScriptBinding_Velocity,F|A,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_VelocityZ,"z",NativeScriptBinding_Velocity,F|A,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_AccelerationX,"x",NativeScriptBinding_Acceleration,F,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_AccelerationY,"y",NativeScriptBinding_Acceleration,F,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_AccelerationZ,"z",NativeScriptBinding_Acceleration,F,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_InputX,"x",NativeScriptBinding_Input,F,NativeScriptType_F32,R),
    PROP(NativeScriptBinding_InputY,"y",NativeScriptBinding_Input,F,NativeScriptType_F32,R),
    PROP(NativeScriptBinding_InputSpecialHeld,"special_held",NativeScriptBinding_Input,F,NativeScriptType_Bool,R),
    PROP(NativeScriptBinding_InputSpecialPressed,"special_pressed",NativeScriptBinding_Input,F,NativeScriptType_Bool,R),
    PROP(NativeScriptBinding_Frame,"frame",0,F,NativeScriptType_F32,R),
    PROP(NativeScriptBinding_FrameInteger,"integer",NativeScriptBinding_Frame,F,NativeScriptType_I32,R),
    PROP(NativeScriptBinding_FacingDirection,"direction",NativeScriptBinding_Facing,F,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_FacingAnimationDirection,"animation_direction",NativeScriptBinding_Facing,F,NativeScriptType_F32,R|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ArticleFacing,"facing",0,A,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_Grounded,"grounded",0,F,NativeScriptType_Bool,R),
    PROP(NativeScriptBinding_Airborne,"airborne",0,F,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_EffectsActive,"active",NativeScriptBinding_Effects,F,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_UsedTether,"used_tether",NativeScriptBinding_Movement,F,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_Fastfall,"fastfall",NativeScriptBinding_Movement,F,NativeScriptType_Bool,W),
    MATH1(NativeScriptBinding_Abs,"abs",ALL,NativeScriptType_F32,NativeScriptType_F32),
    MATH1(NativeScriptBinding_Sqrt,"sqrt",ALL,NativeScriptType_F32,NativeScriptType_F32),
    MATH1(NativeScriptBinding_Sin,"sin",ALL,NativeScriptType_F32,NativeScriptType_F32),
    MATH1(NativeScriptBinding_Cos,"cos",ALL,NativeScriptType_F32,NativeScriptType_F32),
    MATH2(NativeScriptBinding_Min,"min",ALL,NativeScriptType_F32,NativeScriptType_F32),
    MATH2(NativeScriptBinding_Max,"max",ALL,NativeScriptType_F32,NativeScriptType_F32),
    MATH2(NativeScriptBinding_Atan2,"atan2",F|NativeScriptBindings_Effect,NativeScriptType_F64,NativeScriptType_F64),
    MATH2(NativeScriptBinding_ArticleAtan2,"atan2",A,NativeScriptType_F32,NativeScriptType_F32),
    MATH2(NativeScriptBinding_Atan2Float,"atan2_float",ALL,NativeScriptType_F32,NativeScriptType_F32),
    MATH1(NativeScriptBinding_Signbit,"signbit",ALL,NativeScriptType_I32,NativeScriptType_F32),
    MATH1(NativeScriptBinding_WrapAngle,"wrap_angle",ALL,NativeScriptType_F32,NativeScriptType_F32),
    MATH1(NativeScriptBinding_WrapSignedAngle,"wrap_signed_angle",ALL,NativeScriptType_F32,NativeScriptType_F32),
    CONVERT(NativeScriptBinding_Int,"int",NativeScriptType_I32),
    CONVERT(NativeScriptBinding_UInt,"uint",NativeScriptType_U32),
    CONVERT(NativeScriptBinding_Float,"float",NativeScriptType_F32),
    CONVERT(NativeScriptBinding_Double,"double",NativeScriptType_F64),
    CONSTANT(NativeScriptBinding_Pi,"pi"),
    CONSTANT(NativeScriptBinding_Tau,"tau"),
    CONSTANT(NativeScriptBinding_PiHalf,"pi_half"),
    MATH1(NativeScriptBinding_RadiansFloat,"radians",ALL,NativeScriptType_F32,NativeScriptType_F32),
    MATH1(NativeScriptBinding_RadiansDouble,"radians",ALL,NativeScriptType_F64,NativeScriptType_F64),
    NS(NativeScriptBinding_Animation,"animation",0,F),
    NS(NativeScriptBinding_AnimationDelta,"delta",NativeScriptBinding_Animation,F),
    NS(NativeScriptBinding_Motion,"motion",0,F),
    NS(NativeScriptBinding_Physics,"physics",0,F),
    NS(NativeScriptBinding_Ground,"ground",0,F),
    NS(NativeScriptBinding_Air,"air",0,F),
    PROP(NativeScriptBinding_AnimationFinished,"finished",NativeScriptBinding_Animation,F,NativeScriptType_Bool,R|NativeScriptFlag_MayInvalidateSource),
    PROP(NativeScriptBinding_AnimationRate,"rate",NativeScriptBinding_Animation,F,NativeScriptType_F32,R),
    PROP(NativeScriptBinding_AnimationID,"id",NativeScriptBinding_Animation,F,NativeScriptType_I32,R),
    PROP(NativeScriptBinding_AnimationLength,"length",NativeScriptBinding_Animation,F,NativeScriptType_F32,R|NativeScriptFlag_MayInvalidateSource),
    PROP(NativeScriptBinding_AnimationDeltaX,"x",NativeScriptBinding_AnimationDelta,F,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_AnimationDeltaY,"y",NativeScriptBinding_AnimationDelta,F,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_AnimationDeltaZ,"z",NativeScriptBinding_AnimationDelta,F,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_MotionID,"id",NativeScriptBinding_Motion,F,NativeScriptType_I32,W),
    PROP(NativeScriptBinding_MotionLaunchAngle,"launch_angle",NativeScriptBinding_Motion,F,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_GroundVelocity,"velocity",NativeScriptBinding_Ground,F,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_GroundAcceleration,"acceleration",NativeScriptBinding_Ground,F,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_GroundFriction,"friction",NativeScriptBinding_Ground,F,NativeScriptType_F32,R),
    PROP(NativeScriptBinding_AirDriftMax,"drift_max",NativeScriptBinding_Air,F,NativeScriptType_F32,R),
    PROP(NativeScriptBinding_AirDriftStickMultiplier,"drift_stick_mul",NativeScriptBinding_Air,F,NativeScriptType_F32,R),
    PROP(NativeScriptBinding_AirDriftThreshold,"drift_threshold",NativeScriptBinding_Air,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    K0(NativeScriptBinding_AnimationStart,"start",NativeScriptBinding_Animation,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K1(NativeScriptBinding_AnimationSetRate,"set_rate",NativeScriptBinding_Animation,NativeScriptType_Void,NativeScriptType_F32,NativeScriptFlag_MayInvalidateSource),
    K1(NativeScriptBinding_AnimationSetTimer,"set_timer",NativeScriptBinding_Animation,NativeScriptType_Void,NativeScriptType_F32,0),
    K0(NativeScriptBinding_AnimationClearMotionSamples,"clear_motion_samples",NativeScriptBinding_Animation,NativeScriptType_Void,0),
    K1(NativeScriptBinding_MotionSetVerticalSpeed,"set_vertical_speed",NativeScriptBinding_Motion,NativeScriptType_Void,NativeScriptType_F32,0),
    KN(NativeScriptBinding_MotionAddVerticalSpeed,"add_vertical_speed"),
    KN(NativeScriptBinding_MotionScaleHorizontalSpeed,"scale_horizontal_speed"),
    KN(NativeScriptBinding_MotionDivideHorizontalSpeed,"divide_horizontal_speed"),
    K1(NativeScriptBinding_MotionScaleVelocity,"scale_velocity",NativeScriptBinding_Motion,NativeScriptType_Void,NativeScriptType_F32,0),
    K1(NativeScriptBinding_MotionScalePlanarVelocity,"scale_planar_velocity",NativeScriptBinding_Motion,NativeScriptType_Void,NativeScriptType_F32,0),
    K0(NativeScriptBinding_PhysicsClearKinetics,"clear_kinetics",NativeScriptBinding_Physics,NativeScriptType_Void,0),
    K0(NativeScriptBinding_PhysicsGroundFriction,"ground_friction",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_PhysicsAirDrift,"air_drift",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K1(NativeScriptBinding_PhysicsAirDriftScaled,"air_drift",0,NativeScriptType_Void,NativeScriptType_F32,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_PhysicsGroundMotion,"ground_motion",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_PhysicsAirMotion,"air_motion",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_PhysicsAirGravityAndFriction,"air_gravity_and_friction",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_PhysicsClampAirDrift,"clamp_air_drift",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_PhysicsQuickAirDecel,"quick_air_decel",NativeScriptBinding_Physics,NativeScriptType_Bool,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_PhysicsFallBasic,"fall_basic",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_PhysicsGroundToSelf,"ground_to_self",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K2(NativeScriptBinding_PhysicsAscend,"ascend",NativeScriptBinding_Physics),
    K2(NativeScriptBinding_PhysicsGroundAccel,"ground_accel",NativeScriptBinding_Physics),
    K2(NativeScriptBinding_PhysicsAirAccel,"air_accel",NativeScriptBinding_Physics),
    K1(NativeScriptBinding_PhysicsClampHorizontalSpeed,"clamp_horizontal_speed",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptType_F32,NativeScriptFlag_MayInvalidateSource),
    K1(NativeScriptBinding_PhysicsClampGroundSpeed,"clamp_ground_speed",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptType_F32,NativeScriptFlag_MayInvalidateSource),
    K1(NativeScriptBinding_PhysicsClampFallSpeed,"clamp_fall_speed",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptType_F32,NativeScriptFlag_MayInvalidateSource),
    K1(NativeScriptBinding_PhysicsAirDecel,"air_decel",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptType_F32,NativeScriptFlag_MayInvalidateSource),
    K1(NativeScriptBinding_PhysicsFall,"fall",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptType_F32,NativeScriptFlag_MayInvalidateSource),
    K2(NativeScriptBinding_PhysicsFallWithTerminal,"fall",NativeScriptBinding_Physics),
    K0(NativeScriptBinding_PhysicsAirDecelNormal,"air_decel_normal",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K1(NativeScriptBinding_PhysicsGroundDecelerate,"ground_decelerate",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptType_F32,NativeScriptFlag_MayInvalidateSource),
    K2(NativeScriptBinding_PhysicsAirAccelCommon,"air_accel_common",NativeScriptBinding_Physics),
    [NativeScriptBinding_PhysicsAirAccelFriction]={.name="air_accel_friction",.parent=NativeScriptBinding_Physics,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    K0(NativeScriptBinding_PhysicsAirAccelNormal,"air_accel_normal",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K1(NativeScriptBinding_PhysicsAirAccelFrom,"air_accel_from",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptType_F32,NativeScriptFlag_MayInvalidateSource),
    K1(NativeScriptBinding_PhysicsQuickDecel,"quick_decel",NativeScriptBinding_Physics,NativeScriptType_Bool,NativeScriptType_F32,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_PhysicsAnimationVelocity,"animation_velocity",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_PhysicsAnimationVelocityPlain,"animation_velocity_plain",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_PhysicsGroundAnimationVelocity,"ground_animation_velocity",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_PhysicsFastfall,"fastfall",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_PhysicsCheckFastfall,"check_fastfall",NativeScriptBinding_Physics,NativeScriptType_Bool,NativeScriptFlag_MayInvalidateSource),
    NS(NativeScriptBinding_State,"state",0,F),
    NS(NativeScriptBinding_Collision,"collision",0,F|A),
    NS(NativeScriptBinding_Floor,"floor",0,F|A),
    NS(NativeScriptBinding_FloorNormal,"normal",NativeScriptBinding_Floor,F|A),
    NS(NativeScriptBinding_Controls,"controls",0,F),
    PROP(NativeScriptBinding_InputAttackHeld,"attack_held",NativeScriptBinding_Input,F,NativeScriptType_Bool,R),
    PROP(NativeScriptBinding_InputAttackPressed,"attack_pressed",NativeScriptBinding_Input,F,NativeScriptType_Bool,R),
    PROP(NativeScriptBinding_InputJumpHeld,"jump_held",NativeScriptBinding_Input,F,NativeScriptType_Bool,R),
    PROP(NativeScriptBinding_InputShieldHeld,"shield_held",NativeScriptBinding_Input,F,NativeScriptType_Bool,R),
    PROP(NativeScriptBinding_InputShieldPressed,"shield_pressed",NativeScriptBinding_Input,F,NativeScriptType_Bool,R),
    PROP(NativeScriptBinding_InputUpHeld,"up_held",NativeScriptBinding_Input,F,NativeScriptType_Bool,R),
    PROP(NativeScriptBinding_InputDownHeld,"down_held",NativeScriptBinding_Input,F,NativeScriptType_Bool,R),
    PROP(NativeScriptBinding_InputZHeld,"z_held",NativeScriptBinding_Input,F,NativeScriptType_Bool,R),
    PROP(NativeScriptBinding_InputLHeld,"l_held",NativeScriptBinding_Input,F,NativeScriptType_Bool,R),
    PROP(NativeScriptBinding_InputRHeld,"r_held",NativeScriptBinding_Input,F,NativeScriptType_Bool,R),
    PROP(NativeScriptBinding_InputAttackOrSpecialPressed,"attack_or_special_pressed",NativeScriptBinding_Input,F,NativeScriptType_Bool,R),
    PROP(NativeScriptBinding_InputPreviousX,"previous_x",NativeScriptBinding_Input,F,NativeScriptType_F32,R),
    PROP(NativeScriptBinding_InputPreviousY,"previous_y",NativeScriptBinding_Input,F,NativeScriptType_F32,R),
    PROP(NativeScriptBinding_InputHorizontalSmashAge,"horizontal_smash_age",NativeScriptBinding_Input,F,NativeScriptType_I32,W),
    PROP(NativeScriptBinding_InputAnalogShieldAge,"analog_shield_age",NativeScriptBinding_Input,F,NativeScriptType_I32,W),
    PROP(NativeScriptBinding_InputHorizontalTiltAge,"horizontal_tilt_age",NativeScriptBinding_Input,F,NativeScriptType_I32,W),
    PROP(NativeScriptBinding_ControlsInterruptAllowed,"interrupt_allowed",NativeScriptBinding_Controls,F,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_CollisionFloorContact,"floor_contact",NativeScriptBinding_Collision,F|A,NativeScriptType_Bool,R),
    PROP(NativeScriptBinding_CollisionCeilingContact,"ceiling_contact",NativeScriptBinding_Collision,F|A,NativeScriptType_Bool,R),
    PROP(NativeScriptBinding_CollisionLeftWall,"left_wall",NativeScriptBinding_Collision,F|A,NativeScriptType_Bool,R),
    PROP(NativeScriptBinding_CollisionRightWall,"right_wall",NativeScriptBinding_Collision,F|A,NativeScriptType_Bool,R),
    PROP(NativeScriptBinding_CollisionWallContact,"wall_contact",NativeScriptBinding_Collision,F,NativeScriptType_Bool,R),
    PROP(NativeScriptBinding_CollisionWallAhead,"wall_ahead",NativeScriptBinding_Collision,F,NativeScriptType_Bool,R),
    PROP(NativeScriptBinding_CollisionWallHugAhead,"wall_hug_ahead",NativeScriptBinding_Collision,F,NativeScriptType_Bool,R),
    PROP(NativeScriptBinding_CollisionLeftGround,"left_ground",NativeScriptBinding_Collision,F,NativeScriptType_Bool,R|NativeScriptFlag_MayInvalidateSource),
    PROP(NativeScriptBinding_CollisionSpecialGroundLost,"special_ground_lost",NativeScriptBinding_Collision,F,NativeScriptType_Bool,R|NativeScriptFlag_MayInvalidateSource),
    PROP(NativeScriptBinding_CollisionLanded,"landed",NativeScriptBinding_Collision,F,NativeScriptType_Bool,R|NativeScriptFlag_MayInvalidateSource),
    PROP(NativeScriptBinding_FloorLine,"line",NativeScriptBinding_Floor,F|A,NativeScriptType_I32,R),
    PROP(NativeScriptBinding_FloorNormalX,"x",NativeScriptBinding_FloorNormal,F|A,NativeScriptType_F32,R),
    PROP(NativeScriptBinding_FloorNormalY,"y",NativeScriptBinding_FloorNormal,F|A,NativeScriptType_F32,R),
    PROP(NativeScriptBinding_FloorNormalZ,"z",NativeScriptBinding_FloorNormal,F|A,NativeScriptType_F32,R),
    PROP(NativeScriptBinding_FloorFriction,"friction",NativeScriptBinding_Floor,F|A,NativeScriptType_F32,R|NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_StateWait,"wait",NATIVE_SCRIPT_STATE_NAMESPACE,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    STATE(NativeScriptBinding_StateWaitBlend,"wait_blend",1,ARG(NativeScriptType_F32)),
    K0(NativeScriptBinding_StateFall,"fall",NATIVE_SCRIPT_STATE_NAMESPACE,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_StateFallAerial,"fall_aerial",NATIVE_SCRIPT_STATE_NAMESPACE,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    STATE(NativeScriptBinding_StateFreefall,"freefall",5,ARG(NativeScriptType_I32),ARG(NativeScriptType_I32),ARG(NativeScriptType_Bool),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)),
    STATE(NativeScriptBinding_StateFreefallBlend,"freefall_blend",6,ARG(NativeScriptType_I32),ARG(NativeScriptType_I32),ARG(NativeScriptType_I32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)),
    STATE(NativeScriptBinding_StateLanding,"landing",5,ARG(NativeScriptType_I32),ARG(NativeScriptType_Bool),ARG(NativeScriptType_U32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)),
    K0(NativeScriptBinding_StateBasicLanding,"basic_landing",NATIVE_SCRIPT_STATE_NAMESPACE,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    STATE(NativeScriptBinding_StateLandingSpecial,"landing_special",2,ARG(NativeScriptType_Bool),ARG(NativeScriptType_F32)),
    STATE(NativeScriptBinding_StateLandingFallSpecial,"landing_fall_special",2,ARG(NativeScriptType_Bool),ARG(NativeScriptType_F32)),
    STATE(NativeScriptBinding_StateLandingAerial,"landing_aerial",2,ARG(NativeScriptType_I32),ARG(NativeScriptType_F32)),
    [NativeScriptBinding_CollisionGroundBox]={.name="ground_box",.parent=NativeScriptBinding_Collision,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=6,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_CollisionGroundBoxEdges]={.name="ground_box_edges",.parent=NativeScriptBinding_Collision,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=6,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_CollisionAirBox]={.name="air_box",.parent=NativeScriptBinding_Collision,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=6,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    NS(NativeScriptBinding_Fighter,"fighter",0,F),
    [NativeScriptBinding_Command]={.name="command",.flags=PURE_CALL|W,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_I32,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    K0(NativeScriptBinding_FighterEnableWalljump,"enable_walljump",NativeScriptBinding_Fighter,NativeScriptType_Void,0),
    K0(NativeScriptBinding_FighterEnableMultiJump,"enable_multi_jump",NativeScriptBinding_Fighter,NativeScriptType_Void,0),
    K0(NativeScriptBinding_FighterCopyExtAttributes,"copy_ext_attributes",NativeScriptBinding_Fighter,NativeScriptType_Void,0),
    K0(NativeScriptBinding_FighterReloadExtAttributes,"reload_ext_attributes",NativeScriptBinding_Fighter,NativeScriptType_Void,0),
    [NativeScriptBinding_FighterItemPickup]={.name="item_pickup",.parent=NativeScriptBinding_Fighter,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={ARG(NativeScriptType_Bool),ARG(NativeScriptType_I32),ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_FighterItemDrop]={.name="item_drop",.parent=NativeScriptBinding_Fighter,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={ARG(NativeScriptType_Bool),ARG(NativeScriptType_I32),ARG(NativeScriptType_I32)}},
    K1(NativeScriptBinding_FighterItemVisible,"item_visible",NativeScriptBinding_Fighter,NativeScriptType_Void,NativeScriptType_I32,NativeScriptFlag_MayInvalidateSource),
    K1(NativeScriptBinding_FighterItemInvisible,"item_invisible",NativeScriptBinding_Fighter,NativeScriptType_Void,NativeScriptType_I32,NativeScriptFlag_MayInvalidateSource),
    K1(NativeScriptBinding_FighterKnockbackEnter,"knockback_enter",NativeScriptBinding_Fighter,NativeScriptType_Void,NativeScriptType_I32,NativeScriptFlag_MayInvalidateSource),
    K1(NativeScriptBinding_FighterKnockbackExit,"knockback_exit",NativeScriptBinding_Fighter,NativeScriptType_Void,NativeScriptType_I32,NativeScriptFlag_MayInvalidateSource),
    PROP(NativeScriptBinding_AirGravity,"gravity",NativeScriptBinding_Air,F,NativeScriptType_F32,R),
    PROP(NativeScriptBinding_AirFriction,"friction",NativeScriptBinding_Air,F,NativeScriptType_F32,R),
    NS(NativeScriptBinding_Callbacks,"callbacks",0,F),
    NS(NativeScriptBinding_Cue,"cue",0,F),
    NS(NativeScriptBinding_CueReverse,"reverse",NativeScriptBinding_Cue,F),
    NS(NativeScriptBinding_CueArticle,"article",NativeScriptBinding_Cue,F),
    NS(NativeScriptBinding_CueThrow,"throw",NativeScriptBinding_Cue,F),
    K0(NativeScriptBinding_CueReverseConsume,"consume",NativeScriptBinding_CueReverse,NativeScriptType_Bool,0),
    K0(NativeScriptBinding_CueArticleConsume,"consume",NativeScriptBinding_CueArticle,NativeScriptType_Bool,0),
    K0(NativeScriptBinding_CueThrowConsume,"consume",NativeScriptBinding_CueThrow,NativeScriptType_Bool,0),
    K0(NativeScriptBinding_CueThrowReset,"reset",NativeScriptBinding_CueThrow,NativeScriptType_Void,0),
    K1(NativeScriptBinding_CueConsume,"consume",NativeScriptBinding_Cue,NativeScriptType_Bool,NativeScriptType_I32,NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_CollisionUnlockECB,"unlock_ecb",NativeScriptBinding_Collision,NativeScriptType_Void,NativeScriptFlag_SourceOnly),
    CODE_CALLBACK(NativeScriptBinding_CallbackDamage,"damage"),
    CODE_CALLBACK(NativeScriptBinding_CallbackDeath1,"death1"),
    CODE_CALLBACK(NativeScriptBinding_CallbackDeath2,"death2"),
    CODE_CALLBACK(NativeScriptBinding_CallbackDeath3,"death3"),
    CODE_CALLBACK(NativeScriptBinding_CallbackPreHitlag,"pre_hitlag"),
    CODE_CALLBACK(NativeScriptBinding_CallbackPostHitlag,"post_hitlag"),
    CODE_CALLBACK(NativeScriptBinding_CallbackDealDamage,"deal_damage"),
    CODE_CALLBACK(NativeScriptBinding_CallbackStateChanged,"state_changed"),
    CODE_CALLBACK(NativeScriptBinding_CallbackReflectHit,"reflect_hit"),
    CODE_CALLBACK(NativeScriptBinding_CallbackReflect,"reflect"),
    CODE_CALLBACK(NativeScriptBinding_CallbackHurtboxDetect,"hurtbox_detect"),
    CODE_CALLBACK(NativeScriptBinding_CallbackShieldHit,"shield_hit"),
    CODE_CALLBACK(NativeScriptBinding_CallbackSecondaryDamage,"secondary_damage"),
    K0(NativeScriptBinding_CallbackTrackReflectHit,"track_reflect_hit",NativeScriptBinding_Callbacks,NativeScriptType_Void,0),
    K0(NativeScriptBinding_CallbackTrackReflect,"track_reflect",NativeScriptBinding_Callbacks,NativeScriptType_Void,0),
    K0(NativeScriptBinding_CallbackTrackHurtboxDetect,"track_hurtbox_detect",NativeScriptBinding_Callbacks,NativeScriptType_Void,0),
    K0(NativeScriptBinding_CallbackTrackShieldHit,"track_shield_hit",NativeScriptBinding_Callbacks,NativeScriptType_Void,0),
    K0(NativeScriptBinding_CallbackTrackDamage,"track_damage",NativeScriptBinding_Callbacks,NativeScriptType_Void,0),
    K0(NativeScriptBinding_CallbackTrackDeath2,"track_death2",NativeScriptBinding_Callbacks,NativeScriptType_Void,0),
    K0(NativeScriptBinding_CallbackTrackDeath3,"track_death3",NativeScriptBinding_Callbacks,NativeScriptType_Void,0),
    K0(NativeScriptBinding_CallbackTrackHitlag,"track_hitlag",NativeScriptBinding_Callbacks,NativeScriptType_Void,0),
    K0(NativeScriptBinding_CallbackTrackDealDamage,"track_deal_damage",NativeScriptBinding_Callbacks,NativeScriptType_Void,0),
    K0(NativeScriptBinding_CallbackTrackStateChange,"track_state_change",NativeScriptBinding_Callbacks,NativeScriptType_Void,0),
    K0(NativeScriptBinding_CallbackClearDamageDeath,"clear_damage_death",NativeScriptBinding_Callbacks,NativeScriptType_Void,0),
    NS(NativeScriptBinding_Damage,"damage",0,F),
    PROP(NativeScriptBinding_FloorIsPlatform,"is_platform",NativeScriptBinding_Floor,F,NativeScriptType_Bool,R|NativeScriptFlag_SourceOnly),
    [NativeScriptBinding_PhysicsMirrorVelocity]={.name="mirror_velocity",.parent=NativeScriptBinding_Physics,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_PhysicsRotateVelocityUnitAxis]={.name="rotate_velocity_unit_axis",.parent=NativeScriptBinding_Physics,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=4,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    STATE(NativeScriptBinding_StateDownBound,"down_bound",0),
    NS(NativeScriptBinding_Smash,"smash",0,F),
    PROP(NativeScriptBinding_SmashChargeMode,"charge_mode",NativeScriptBinding_Smash,F,NativeScriptType_I32,R|W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_SmashChargeFrames,"charge_frames",NativeScriptBinding_Smash,F,NativeScriptType_F32,R|W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_SmashChargeLimit,"charge_limit",NativeScriptBinding_Smash,F,NativeScriptType_F32,R|W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_SmashSavedAnimationRate,"saved_animation_rate",NativeScriptBinding_Smash,F,NativeScriptType_F32,R|W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_SmashChargeColor,"charge_color",NativeScriptBinding_Smash,F,NativeScriptType_I32,R|W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_SmashShakeIndex,"shake_index",NativeScriptBinding_Smash,F,NativeScriptType_I32,R|W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_SmashShakePeriod,"shake_period",NativeScriptBinding_Smash,F,NativeScriptType_I32,R|W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_SmashShakeTableCount,"shake_table_count",NativeScriptBinding_Smash,F,NativeScriptType_I32,R|NativeScriptFlag_SourceOnly),
    NS(NativeScriptBinding_Random,"random",0,ALL),
    NS(NativeScriptBinding_Hurtboxes,"hurtboxes",0,F),
    NS(NativeScriptBinding_Hurt,"Hurt",0,F),
    PROP(NativeScriptBinding_DamageSongImmune,"song_immune",NativeScriptBinding_Damage,F,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_DamageArmor,"armor",NativeScriptBinding_Damage,F,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_DamageJumpArmor,"jump_armor",NativeScriptBinding_Damage,F,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_DamageInHitstun,"in_hitstun",NativeScriptBinding_Damage,F,NativeScriptType_Bool,R),
    PROP(NativeScriptBinding_DamageReactionSuppressed,"reaction_suppressed",NativeScriptBinding_Damage,F,NativeScriptType_Bool,R|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_DamageKnockbackLevel,"knockback_level",NativeScriptBinding_Damage,F,NativeScriptType_I32,R|NativeScriptFlag_SourceOnly),
    [NativeScriptBinding_DamageEnter]={.name="enter",.parent=NativeScriptBinding_Damage,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_F32)}},
    K1(NativeScriptBinding_DamageDefaultReaction,"default_reaction",NativeScriptBinding_Damage,NativeScriptType_Void,NativeScriptType_F32,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_DamageUpdateTimer,"update_timer",NativeScriptBinding_Damage,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_PhysicsDamageMotion,"damage_motion",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_ArticlesNewCollisionGroup,"new_collision_group",NativeScriptBinding_Articles,NativeScriptType_I32,NativeScriptFlag_SourceOnly),
    K1(NativeScriptBinding_FighterSelfDamage,"self_damage",NativeScriptBinding_Fighter,NativeScriptType_Void,NativeScriptType_F32,NativeScriptFlag_MayInvalidateSource),
    K1(NativeScriptBinding_DamageHeal,"heal",NativeScriptBinding_Damage,NativeScriptType_Void,NativeScriptType_F32,NativeScriptFlag_MayInvalidateSource),
    [NativeScriptBinding_RandomFloat]={.name="float",.parent=NativeScriptBinding_Random,.flags=KINETIC_CALL,.kind=NativeScriptBinding_Intrinsic,.contexts=ALL,.result_type=NativeScriptType_F32},
    [NativeScriptBinding_RandomInt]={.name="int",.parent=NativeScriptBinding_Random,.flags=KINETIC_CALL,.kind=NativeScriptBinding_Intrinsic,.contexts=ALL,.result_type=NativeScriptType_I32,.argument_count=1,.arguments={ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_RandomIntExcluding]={.name="int_excluding",.parent=NativeScriptBinding_Random,.flags=KINETIC_CALL,.kind=NativeScriptBinding_Intrinsic,.contexts=ALL,.result_type=NativeScriptType_I32,.argument_count=3,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_I32),ARG(NativeScriptType_I32)}},
    PROP(NativeScriptBinding_HurtNormal,"normal",NativeScriptBinding_Hurt,F,NativeScriptType_I32,R),
    PROP(NativeScriptBinding_HurtInvincible,"invincible",NativeScriptBinding_Hurt,F,NativeScriptType_I32,R),
    PROP(NativeScriptBinding_HurtIntangible,"intangible",NativeScriptBinding_Hurt,F,NativeScriptType_I32,R),
    PROP(NativeScriptBinding_HurtboxesStatus,"status",NativeScriptBinding_Hurtboxes,F,NativeScriptType_I32,R),
    K1(NativeScriptBinding_HurtboxesSetAll,"set_all",NativeScriptBinding_Hurtboxes,NativeScriptType_Void,NativeScriptType_I32,0),
    [NativeScriptBinding_HurtboxesSetBone]={.name="set_bone",.parent=NativeScriptBinding_Hurtboxes,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_I32)}},
    K1(NativeScriptBinding_HurtboxesSetBodyStatus,"set_body_status",NativeScriptBinding_Hurtboxes,NativeScriptType_Void,NativeScriptType_I32,NativeScriptFlag_MayInvalidateSource),
    [NativeScriptBinding_HurtboxesDefine]={.name="define",.parent=NativeScriptBinding_Hurtboxes,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=11,.arguments={ARG(NativeScriptType_I32),{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_I32),ARG(NativeScriptType_Bool),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    K0(NativeScriptBinding_EffectsClearAll,"clear_all",NativeScriptBinding_Effects,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_EffectsDestroyAll,"destroy_all",NativeScriptBinding_Effects,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_EffectsPause,"pause",NativeScriptBinding_Effects,NativeScriptType_Void,0),
    K0(NativeScriptBinding_EffectsResume,"resume",NativeScriptBinding_Effects,NativeScriptType_Void,0),
    K0(NativeScriptBinding_EffectsPauseDuringHitlag,"pause_during_hitlag",NativeScriptBinding_Effects,NativeScriptType_Void,0),
    [NativeScriptBinding_EffectsSpawnBetween]={.name="spawn_between",.parent=NativeScriptBinding_Effects,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_I32),ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_EffectsSpawnAsyncPositionOnPart]={.name="spawn_async_position_on_part",.parent=NativeScriptBinding_Effects,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_EffectsSpawnAsyncPositionScalarOnPart]={.name="spawn_async_position_scalar_on_part",.parent=NativeScriptBinding_Effects,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_I32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_Stage]={.name="stage",.kind=NativeScriptBinding_Namespace,.contexts=F|A,.result_type=NativeScriptType_Namespace,.flags=NativeScriptFlag_SourceOnly},
    PROP(NativeScriptBinding_StageInternalKind,"internal_kind",NativeScriptBinding_Stage,F|A,NativeScriptType_I32,R|NativeScriptFlag_Pure),
    [NativeScriptBinding_StageSweepBox]={.name="sweep_box",.parent=NativeScriptBinding_Stage,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_I32,.argument_count=13,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_StageSegmentBlocked]={.name="segment_blocked",.parent=NativeScriptBinding_Stage,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Bool,.argument_count=4,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    STATE(NativeScriptBinding_StateEnter,"enter",1,{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}),
    STATE(NativeScriptBinding_StateEnterFrame,"enter_frame",2,{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_F32)),
    STATE(NativeScriptBinding_StateEnterSpeed,"enter_speed",3,{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)),
    STATE(NativeScriptBinding_StateBegin,"begin",1,{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}),
    STATE(NativeScriptBinding_StateEnterFlags,"enter_flags",2,{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_U32)),
    STATE(NativeScriptBinding_StateEnterAt,"enter_at",3,{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_U32),ARG(NativeScriptType_F32)),
    STATE(NativeScriptBinding_StateToAir,"to_air",0),
    STATE(NativeScriptBinding_StateToGround,"to_ground",0),
    STATE(NativeScriptBinding_StateSwitchToAir,"switch_to_air",2,{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_U32)),
    STATE(NativeScriptBinding_StateSwitchToGround,"switch_to_ground",2,{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_U32)),
    NS(NativeScriptBinding_Sound,"sound",0,F),NS(NativeScriptBinding_Bones,"bones",0,F),
    NS(NativeScriptBinding_BonesRoot,"root",NativeScriptBinding_Bones,F),
    PROP(NativeScriptBinding_BonesCount,"count",NativeScriptBinding_Bones,F,NativeScriptType_I32,R),
    PROP(NativeScriptBinding_BonesRootRotationX,"rotation_x",NativeScriptBinding_BonesRoot,F,NativeScriptType_F32,R),
    PROP(NativeScriptBinding_BonesRootRotationY,"rotation_y",NativeScriptBinding_BonesRoot,F,NativeScriptType_F32,R),
    PROP(NativeScriptBinding_BonesRootScaleX,"scale_x",NativeScriptBinding_BonesRoot,F,NativeScriptType_F32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_BonesRootScaleY,"scale_y",NativeScriptBinding_BonesRoot,F,NativeScriptType_F32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_BonesRootScaleZ,"scale_z",NativeScriptBinding_BonesRoot,F,NativeScriptType_F32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    [NativeScriptBinding_BonesRootSetScale]={.name="set_scale",.parent=NativeScriptBinding_BonesRoot,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    NS(NativeScriptBinding_Attack,"attack",0,F),
    K1(NativeScriptBinding_AttackEnterJab,"enter_jab",NativeScriptBinding_Attack,NativeScriptType_Bool,NativeScriptType_I32,NativeScriptFlag_MayInvalidateSource),
    K1(NativeScriptBinding_AttackEnterRapidJab,"enter_rapid_jab",NativeScriptBinding_Attack,NativeScriptType_Void,NativeScriptType_I32,NativeScriptFlag_MayInvalidateSource),
    [NativeScriptBinding_AttackUpdateRapidJab]={.name="update_rapid_jab",.parent=NativeScriptBinding_Attack,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={{.type=NativeScriptType_Callable,.mode=NativeScriptArgument_CodeRef,.numeric_any=false}}},
    K0(NativeScriptBinding_ItemsTryGroundPickup,"try_ground_pickup",NativeScriptBinding_Items,NativeScriptType_Bool,NativeScriptFlag_MayInvalidateSource),
    [NativeScriptBinding_CollisionAirAerialLandingCallback]={.name="air_aerial_landing_callback",.parent=NativeScriptBinding_Collision,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={{.type=NativeScriptType_Callable,.mode=NativeScriptArgument_CodeRef,.numeric_any=false}}},
    [NativeScriptBinding_CollisionAirAerialLandingWithLedges]={.name="air_aerial_landing_callback",.parent=0,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={{.type=NativeScriptType_Callable,.mode=NativeScriptArgument_CodeRef,.numeric_any=false},ARG(NativeScriptType_Bool)}},
    K1(NativeScriptBinding_BonesIndex,"index",NativeScriptBinding_Bones,NativeScriptType_I32,NativeScriptType_I32,NativeScriptFlag_Pure),
    [NativeScriptBinding_SoundPlay]={.name="play",.parent=NativeScriptBinding_Sound,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_I32),ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_SoundLoop]={.name="loop",.parent=NativeScriptBinding_Sound,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_I32),ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_SoundChannel]={.name="channel",.parent=NativeScriptBinding_Sound,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=4,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_I32),ARG(NativeScriptType_I32),ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_SoundClip]={.name="clip",.parent=NativeScriptBinding_Sound,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=4,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_I32),ARG(NativeScriptType_I32),ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_BonesRootRotateX]={.name="rotate_x",.parent=NativeScriptBinding_BonesRoot,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_BonesRootRotateXWithJoint]={.name="rotate_x_joint",.parent=NativeScriptBinding_BonesRoot,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_BonesRotateX]={.name="rotate_x",.parent=NativeScriptBinding_Bones,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_BonesRootRotateY]={.name="rotate_y",.parent=NativeScriptBinding_BonesRoot,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_BonesRootRotateYWithJoint]={.name="rotate_y_joint",.parent=NativeScriptBinding_BonesRoot,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_BonesRotateY]={.name="rotate_y",.parent=NativeScriptBinding_Bones,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_BonesRootRotateZ]={.name="rotate_z",.parent=NativeScriptBinding_BonesRoot,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_BonesRootRotateZWithJoint]={.name="rotate_z_joint",.parent=NativeScriptBinding_BonesRoot,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_BonesRotateZ]={.name="rotate_z",.parent=NativeScriptBinding_Bones,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_BonesRootResetZ]={.name="reset_z",.parent=NativeScriptBinding_BonesRoot,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=0,.arguments={}},
    [NativeScriptBinding_BonesSetScale]={.name="set_scale",.parent=NativeScriptBinding_Bones,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=4,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_BonesSetTranslation]={.name="set_translation",.parent=NativeScriptBinding_Bones,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=4,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_BonesSetDirectAnimationCopy]={.name="set_direct_animation_copy",.parent=NativeScriptBinding_Bones,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_Bool)}},
    [NativeScriptBinding_BonesWorldPosition]={.name="world_position",.parent=NativeScriptBinding_Bones,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=4,.arguments={ARG(NativeScriptType_I32),{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false}}, .publication=NativeScriptOutputs_AlwaysAfterGuard},
    [NativeScriptBinding_BonesWorldPositionWithOffset]={.name="world_position_offset",.parent=NativeScriptBinding_Bones,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=7,.arguments={ARG(NativeScriptType_I32),{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}, .publication=NativeScriptOutputs_AlwaysAfterGuard},
    NS(NativeScriptBinding_Item,"item",0,A),
    PROP(NativeScriptBinding_ItemLife,"life",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_ItemHalfLife,"half_life",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_ItemAnimationRate,"animation_rate",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_ItemSpinSpeed,"spin_speed",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_ItemKind,"kind",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_I32,R),
    PROP(NativeScriptBinding_ItemCreationID,"creation_id",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_U32,R),
    PROP(NativeScriptBinding_ItemMotionState,"motion_state",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_I32,R),
    PROP(NativeScriptBinding_ItemAge,"age",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_I32,R),
    PROP(NativeScriptBinding_ItemAccumulatedDamage,"accumulated_damage",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_I32,R),
    PROP(NativeScriptBinding_ItemDamageReceived,"damage_received",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_I32,R),
    PROP(NativeScriptBinding_ItemDamageBehavior,"damage_behavior",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ItemCommand0,"command0",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_U32,W),
    PROP(NativeScriptBinding_ItemCommand1,"command1",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_U32,W),
    PROP(NativeScriptBinding_ItemCommand2,"command2",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_U32,W),
    PROP(NativeScriptBinding_ItemCommand3,"command3",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_U32,W),
    PROP(NativeScriptBinding_ItemCollisionMode,"collision_mode",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_I32,W),
    PROP(NativeScriptBinding_ItemCollisionGroup,"collision_group",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_U32,W),
    PROP(NativeScriptBinding_ItemExitCategory,"exit_category",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_I32,W),
    PROP(NativeScriptBinding_ItemAnimationFrame,"animation_frame",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_F32,R),
    PROP(NativeScriptBinding_ItemBaseSpinSpeed,"base_spin_speed",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_F32,R),
    PROP(NativeScriptBinding_ItemContactFighter,"contact_fighter",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_U32,R|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ItemReflector,"reflector",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_U32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ItemReflectionOwnerTransferDisabled,"reflection_owner_transfer_disabled",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_Bool,R|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ItemReflectionMultiplier,"reflection_multiplier",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_F32,R),
    PROP(NativeScriptBinding_ItemScale,"scale",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_F32,R),
    PROP(NativeScriptBinding_ItemThrowScale,"throw_scale",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_F32,R),
    PROP(NativeScriptBinding_ItemDamageDirection,"damage_direction",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_F32,R),
    PROP(NativeScriptBinding_ItemDropSpeed,"drop_speed",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_F32,R),
    PROP(NativeScriptBinding_ItemBaseScale,"base_scale",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_F32,R),
    PROP(NativeScriptBinding_ItemSurfaceRestitution,"surface_restitution",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_F32,R),
    PROP(NativeScriptBinding_ItemPhysicsDisabled,"physics_disabled",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_ItemCustomFloorMotion,"custom_floor_motion",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_ItemRemoveAtBounds,"remove_at_bounds",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_ItemSuppressDestructionEffects,"suppress_destruction_effects",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_ItemWorldInteraction,"world_interaction",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_ItemOwnerBound,"owner_bound",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_ItemReverseSpin,"reverse_spin",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_ItemThrowSpeedDamage,"throw_speed_damage",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_ItemDamageHitlag,"damage_hitlag",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_ItemContactHitlag,"contact_hitlag",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_ItemHarmOwner,"harm_owner",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_ItemHarmTeammates,"harm_teammates",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_ItemHarmSiblingItems,"harm_sibling_items",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_ItemCanBeHitByOwner,"can_be_hit_by_owner",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_ItemCanBeHitByTeammates,"can_be_hit_by_teammates",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_ItemCanBeHitBySiblingItems,"can_be_hit_by_sibling_items",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_ItemSubactionSoundsMuted,"subaction_sounds_muted",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_ItemStopOnSurface,"stop_on_surface",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_ItemDynamicECB,"dynamic_ecb",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_ItemGrounded,"grounded",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_ItemAirborne,"airborne",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_ItemGroundContact,"ground_contact",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_Bool,R),
    [NativeScriptBinding_ItemSweepFloor]={.name="sweep_floor",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_ItemWallContactFlags]={.name="wall_contact_flags",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_I32},
    [NativeScriptBinding_ItemCeilingContactFlags]={.name="ceiling_contact_flags",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_I32},
    [NativeScriptBinding_ItemReflectFromWall]={.name="reflect_from_wall",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Bool},
    [NativeScriptBinding_ItemAlignToFloor]={.name="align_to_floor",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_ItemAlignFacingToVelocity]={.name="align_facing_to_velocity",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_ItemGravity]={.name="gravity",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=0,.arguments={}},
    [NativeScriptBinding_ItemGravityWithSpin]={.name="gravity_with_spin",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={ARG(NativeScriptType_Bool)}},
    [NativeScriptBinding_ItemSetLifetime]={.name="set_lifetime",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ItemExpireTick]={.name="expire_tick",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Bool,.argument_count=0,.arguments={}},
    [NativeScriptBinding_ItemReflect]={.name="reflect",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Bool,.argument_count=0,.arguments={}},
    [NativeScriptBinding_ItemShieldBounce]={.name="shield_bounce",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Bool,.argument_count=0,.arguments={}},
    [NativeScriptBinding_ItemBounceStage]={.name="bounce_stage",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Bool,.argument_count=0,.arguments={}},
    [NativeScriptBinding_ItemBounceVictim]={.name="bounce",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_ItemRespondStageMask]={.name="respond_stage_mask",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_ItemAttachmentOwnerIs]={.name="attachment_owner_is",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=PURE_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={ARG(NativeScriptType_Object)}},
    [NativeScriptBinding_ItemReleaseCamera]={.name="release_camera",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_ItemHitsStage]={.name="hits_stage",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Bool},
    [NativeScriptBinding_ItemLandOnStage]={.name="land_on_stage",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_ItemStopLoopSound]={.name="stop_loop_sound",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_ItemStageCollision]={.name="stage_collision",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=0,.arguments={}},
    [NativeScriptBinding_ItemPause]={.name="pause",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=0,.arguments={}},
    [NativeScriptBinding_ItemResume]={.name="resume",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=0,.arguments={}},
    [NativeScriptBinding_ItemUseManualLifetime]={.name="use_manual_lifetime",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=0,.arguments={}},
    [NativeScriptBinding_ItemMirrorVelocity]={.name="mirror_velocity",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=0,.arguments={}},
    [NativeScriptBinding_ItemMirrorVelocityWithNormal]={.name="mirror_velocity_normal",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ItemPlaySound]={.name="play_sound",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_I32),ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_ItemLoopSound]={.name="loop_sound",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_I32),ARG(NativeScriptType_I32)}},


    NS(NativeScriptBinding_Articles,"articles",NativeScriptBinding_Invalid,F|A),
    NS(NativeScriptBinding_Items,"items",NativeScriptBinding_Invalid,F),
    [NativeScriptBinding_SpawnAuthoredProjectile]={.name="spawn_authored_projectile",.parent=NativeScriptBinding_Articles,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_U32),ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_ArticlesLaunchAtPosition]={.name="launch_at_position",.parent=NativeScriptBinding_Articles,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=10,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_I32),ARG(NativeScriptType_I32),{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ArticlesLaunchProjectile]={.name="launch_projectile",.parent=NativeScriptBinding_Articles,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=8,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_U32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_SpawnBoneAttached]={.name="spawn_bone_attached",.parent=NativeScriptBinding_Articles,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_I32,.argument_count=2,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_U32)}},
    [NativeScriptBinding_SpawnAttachedPresentation]={.name="spawn_attached",.parent=NativeScriptBinding_Articles,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_I32,.argument_count=7,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_U32),ARG(NativeScriptType_I32),ARG(NativeScriptType_I32),ARG(NativeScriptType_U32),ARG(NativeScriptType_U32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ArticlesOwned]={.name="owned",.parent=NativeScriptBinding_Articles,.flags=KINETIC_CALL,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_ArticlesPauseOwned]={.name="pause_owned",.parent=NativeScriptBinding_Articles,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_ArticlesResumeOwned]={.name="resume_owned",.parent=NativeScriptBinding_Articles,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_ArticlesRemoveOwned]={.name="remove_owned",.parent=NativeScriptBinding_Articles,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    PROP(NativeScriptBinding_ItemsHeld,"held",NativeScriptBinding_Items,F,NativeScriptType_Bool,R),
    [NativeScriptBinding_ItemsPull]={.name="pull",.parent=NativeScriptBinding_Items,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={ARG(NativeScriptType_U32)}},
    [NativeScriptBinding_ItemAttachmentActive]={.name="attachment_active",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.kind=NativeScriptBinding_Property,.contexts=A,.result_type=NativeScriptType_Bool,.flags=R|NativeScriptFlag_SourceOnly},
    [NativeScriptBinding_ItemChangeState]={.name="change_state",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_ItemChangeStateFlags]={.name="change_state_flags",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_U32)}},
    [NativeScriptBinding_ItemActivateAttached]={.name="activate_attached",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_ItemInteract]={.name="interact",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={ARG(NativeScriptType_Object)}},
    [NativeScriptBinding_ItemSpawnFood]={.name="spawn_food",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},


    NS(NativeScriptBinding_CPU,"cpu",0,F),
    PROP(NativeScriptBinding_CPUGrabAttempts,"grab_attempts",NativeScriptBinding_CPU,F,NativeScriptType_I32,R),
    PROP(NativeScriptBinding_CPUBodyHalfWidth,"body_half_width",NativeScriptBinding_CPU,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_CPULevel,"level",NativeScriptBinding_CPU,F,NativeScriptType_I32,W),
    K1(NativeScriptBinding_CPURunScript,"run_script",NativeScriptBinding_CPU,NativeScriptType_Void,NativeScriptType_I32,NativeScriptFlag_MayInvalidateSource),
    K1(NativeScriptBinding_CPURangedUnsafe,"ranged_unsafe",NativeScriptBinding_CPU,NativeScriptType_Bool,NativeScriptType_F32,NativeScriptFlag_MayInvalidateSource),
    K1(NativeScriptBinding_CPUAddCandidate,"add_candidate",NativeScriptBinding_CPU,NativeScriptType_Void,NativeScriptType_I32,0),
    K0(NativeScriptBinding_ControlsGroundActions,"ground_actions",NativeScriptBinding_Controls,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_ControlsAirActions,"air_actions",NativeScriptBinding_Controls,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_ControlsTauntActions,"taunt_actions",NativeScriptBinding_Controls,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_StateCatchLedge,"catch_ledge",NATIVE_SCRIPT_STATE_NAMESPACE,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_PhysicsGroundToAir,"ground_to_air",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_CollisionGroundWithNudge,"ground_with_nudge",NativeScriptBinding_Collision,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly),
    [NativeScriptBinding_ItemTrackAccessory]={.name="track_accessory",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_ItemClearAccessory]={.name="clear_accessory",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_ItemResetSpin]={.name="reset_spin",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_ItemScaleByAttribute]={.name="scale_by_attribute",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ItemSetInstanceScale]={.name="set_instance_scale",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ItemInheritOwnerScale]={.name="inherit_owner_scale",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_ItemRefreshPhysics]={.name="refresh_physics",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_ItemRemove]={.name="remove",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_ItemHideModel]={.name="hide_model",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_ItemShowModel]={.name="show_model",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_ItemSetModelAnimationRate]={.name="set_model_animation_rate",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ItemBeginExpiry]={.name="begin_expiry",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_ItemSetAppearance]={.name="set_appearance",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ItemsSpawnFood]={.name="spawn_food",.parent=NativeScriptBinding_Items,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=3,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_AnimationClipLength]={.name="clip_length",.parent=NativeScriptBinding_Animation,.flags=PURE_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_F32,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    K0(NativeScriptBinding_FacingFromInput,"from_input",NativeScriptBinding_Facing,NativeScriptType_Void,0),
    K0(NativeScriptBinding_CollisionGroundStandard,"ground_standard",NativeScriptBinding_Collision,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_CollisionAirStandard,"air_standard",NativeScriptBinding_Collision,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    [NativeScriptBinding_CollisionAirSpecialLanding]={.name="air_special_landing",.parent=NativeScriptBinding_Collision,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_Bool),ARG(NativeScriptType_F32)}},
    NS(NativeScriptBinding_Visuals,"visuals",0,F),
    K1(NativeScriptBinding_VisualsHideGroup,"hide_group",NativeScriptBinding_Visuals,NativeScriptType_Void,NativeScriptType_I32,0),
    NS(NativeScriptBinding_Reflector,"reflector",0,F),
    PROP(NativeScriptBinding_ReflectorActive,"active",NativeScriptBinding_Reflector,F,NativeScriptType_Bool,R|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ReflectorHitDirection,"hit_direction",NativeScriptBinding_Reflector,F,NativeScriptType_F32,R|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ReflectorKeepOwner,"keep_owner",NativeScriptBinding_Reflector,F,NativeScriptType_Bool,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ReflectorMaxDamage,"max_damage",NativeScriptBinding_Reflector,F,NativeScriptType_I32,R|W|NativeScriptFlag_SourceOnly),
    [NativeScriptBinding_ReflectorCreate]={.name="create",.parent=NativeScriptBinding_Reflector,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=9,.arguments={ARG(NativeScriptType_U32),ARG(NativeScriptType_I32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_Bool)}},
    K0(NativeScriptBinding_ReflectorDisable,"disable",NativeScriptBinding_Reflector,NativeScriptType_Void,0),
    K0(NativeScriptBinding_ReflectorRetain,"retain",NativeScriptBinding_Reflector,NativeScriptType_Void,0),
    K0(NativeScriptBinding_ReflectorUpdate,"update",NativeScriptBinding_Reflector,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_ArticlesStopCue,"stop_cue",NativeScriptBinding_Articles,NativeScriptType_Void,0),
    [NativeScriptBinding_EffectsSpawn]={.name="spawn",.parent=NativeScriptBinding_Effects,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_U32)}},
    [NativeScriptBinding_EffectsWind]={.name="wind",.parent=NativeScriptBinding_Effects,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=6,.arguments={ARG(NativeScriptType_U32),ARG(NativeScriptType_F32),ARG(NativeScriptType_I32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ItemSpawnEffectOnBone]={.name="spawn_effect_on_bone",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_U32)}},
    [NativeScriptBinding_ItemSpawnEffectOnBoneMode]={.name="spawn_effect_on_bone_mode",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_U32),ARG(NativeScriptType_U32)}},
    K1(NativeScriptBinding_CollisionCheckGroundLedge,"check_ground_ledge",NativeScriptBinding_Collision,NativeScriptType_Bool,NativeScriptType_I32,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_CollisionCliffCatch,"cliff_catch",NativeScriptBinding_Collision,NativeScriptType_Bool,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_ControlsTryAerialJump,"try_aerial_jump",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_MayInvalidateSource),
    NS(NativeScriptBinding_Jump,"jump",0,F),
    [NativeScriptBinding_JumpBegin]={.name="begin",.parent=NativeScriptBinding_Jump,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=7,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_Bool),ARG(NativeScriptType_Bool),ARG(NativeScriptType_Bool)}},
    K1(NativeScriptBinding_StateHeldOpen,"held_open",NATIVE_SCRIPT_STATE_NAMESPACE,NativeScriptType_Void,NativeScriptType_F32,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    NS(NativeScriptBinding_Accessory,"accessory",0,F),
    K1(NativeScriptBinding_AccessoryCreate,"create",NativeScriptBinding_Accessory,NativeScriptType_Bool,NativeScriptType_I32,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    [NativeScriptBinding_AccessoryAddAnimation]={.name="add_animation",.parent=NativeScriptBinding_Accessory,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_AccessorySetScale]={.name="set_scale",.parent=NativeScriptBinding_Accessory,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_AccessorySetGeometryRate]={.name="set_geometry_animation_rate",.parent=NativeScriptBinding_Accessory,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_Bool)}},
    K1(NativeScriptBinding_AccessoryRequestFrame,"request_frame",NativeScriptBinding_Accessory,NativeScriptType_Void,NativeScriptType_F32,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    [NativeScriptBinding_AccessoryEvaluate]={.name="evaluate",.parent=NativeScriptBinding_Accessory,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void},
    K1(NativeScriptBinding_AccessoryFollowBone,"follow_bone",NativeScriptBinding_Accessory,NativeScriptType_Void,NativeScriptType_I32,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    [NativeScriptBinding_AccessoryClear]={.name="clear",.parent=NativeScriptBinding_Accessory,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void},
    NS(NativeScriptBinding_Attachments,"attachments",0,F),
    [NativeScriptBinding_AttachmentsHasRoot]={.name="has_root",.parent=NativeScriptBinding_Attachments,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_Pure,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={{.type=NativeScriptType_Text,.mode=NativeScriptArgument_Literal}}},
    [NativeScriptBinding_AttachmentsLoad]={.name="load",.parent=NativeScriptBinding_Attachments,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=5,.arguments={ARG(NativeScriptType_I32),{.type=NativeScriptType_Text,.mode=NativeScriptArgument_Literal},ARG(NativeScriptType_I32),ARG(NativeScriptType_I32),ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_AttachmentsAddDynamics]={.name="add_dynamics",.parent=NativeScriptBinding_Attachments,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_I32),ARG(NativeScriptType_I32)}},
    K0(NativeScriptBinding_AttachmentsClearAll,"clear_all",NativeScriptBinding_Attachments,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    NS(NativeScriptBinding_Jumps,"jumps",0,F),
    [NativeScriptBinding_JumpsConfigureMulti]={.name="configure_multi",.parent=NativeScriptBinding_Jumps,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=13,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_I32),ARG(NativeScriptType_I32),ARG(NativeScriptType_I32)}},
    K1(NativeScriptBinding_JumpsMultiVerticalSpeed,"multi_vertical_speed",NativeScriptBinding_Jumps,NativeScriptType_F32,NativeScriptType_I32,NativeScriptFlag_SourceOnly|NativeScriptFlag_Pure),
    [NativeScriptBinding_PhysicsAirDriftWithLimits]={.name="air_drift_with_limits",.parent=NativeScriptBinding_Physics,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_CollisionAirBoxLedge]={.name="air_box_ledge",.parent=NativeScriptBinding_Collision,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=7,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    PROP(NativeScriptBinding_JumpsUsed,"used",NativeScriptBinding_Jumps,F,NativeScriptType_I32,W),
    PROP(NativeScriptBinding_JumpsMax,"max",NativeScriptBinding_Jumps,F,NativeScriptType_I32,R),
    K1(NativeScriptBinding_BonesRootAddRotationY,"add_rotation_y",NativeScriptBinding_BonesRoot,NativeScriptType_Void,NativeScriptType_F32,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_ControlsAerialJumpActions,"aerial_jump_actions",NativeScriptBinding_Controls,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_JumpUpdateAnimation,"update_animation",NativeScriptBinding_Jump,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    PROP(NativeScriptBinding_MotionFallAnimation,"fall_animation",NativeScriptBinding_Motion,F,NativeScriptType_I32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_DamageReverseKnockdownPose,"reverse_knockdown_pose",NativeScriptBinding_Damage,F,NativeScriptType_Bool,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CollisionBasicLandingVelocityThreshold,"basic_landing_velocity_threshold",NativeScriptBinding_Collision,F,NativeScriptType_F32,R),
    K0(NativeScriptBinding_CollisionTryWallJump,"try_wall_jump",NativeScriptBinding_Collision,NativeScriptType_Bool,NativeScriptFlag_MayInvalidateSource),
    NS(NativeScriptBinding_Forms,"forms",0,F),
    K0(NativeScriptBinding_FormsActivatePartner,"activate_partner",NativeScriptBinding_Forms,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    CODE_CALLBACK(NativeScriptBinding_CallbackAnimation,"animation"),
    CODE_CALLBACK(NativeScriptBinding_CallbackInterrupt,"interrupt"),
    CODE_CALLBACK(NativeScriptBinding_CallbackPhysics,"physics"),
    CODE_CALLBACK(NativeScriptBinding_CallbackCollision,"collision"),
    CODE_CALLBACK(NativeScriptBinding_CallbackAccessory,"accessory"),
    CODE_CALLBACK(NativeScriptBinding_CallbackAccessoryCue,"accessory_cue"),
    [NativeScriptBinding_CallbackAccessoryBefore]={.name="accessory_before",.parent=NativeScriptBinding_Callbacks,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={{.type=NativeScriptType_Callable,.mode=NativeScriptArgument_CodeRef,.numeric_any=false}}},
    [NativeScriptBinding_CallbackAccessoryHitlag]={.name="accessory_hitlag",.parent=NativeScriptBinding_Callbacks,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={{.type=NativeScriptType_Callable,.mode=NativeScriptArgument_CodeRef,.numeric_any=false}}},
    [NativeScriptBinding_EffectsSpawnOnPart]={.name="spawn_on_part",.parent=NativeScriptBinding_Effects,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_EffectsWindAtPosition]={.name="wind_at_position",.parent=NativeScriptBinding_Effects,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Void,.argument_count=7,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_I32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    STATE(NativeScriptBinding_StateEnterAtSpeed,"enter_at_speed",4,{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_U32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)),
    [NativeScriptBinding_EffectsSpawnWithScalar]={.name="spawn_with_scalar",.parent=NativeScriptBinding_Effects,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_U32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_EffectsSpawnOnJointWithScalar]={.name="spawn_on_joint_with_scalar",.parent=NativeScriptBinding_Effects,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_U32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_Pose]={.name="pose",.flags=NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Namespace,.contexts=F,.result_type=NativeScriptType_Namespace},
    [NativeScriptBinding_PoseLoadAnimation]={.name="load_animation",.parent=NativeScriptBinding_Pose,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_PoseDestination),{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_PoseRequestFrame]={.name="request_frame",.parent=NativeScriptBinding_Pose,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_PoseDestination),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_PoseEvaluate]={.name="evaluate",.parent=NativeScriptBinding_Pose,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={ARG(NativeScriptType_PoseDestination)}},
    [NativeScriptBinding_PoseCopyCostume]={.name="copy_costume",.parent=NativeScriptBinding_Pose,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_PoseDestination),{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_PoseCopyGraph]={.name="copy_graph",.parent=NativeScriptBinding_Pose,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_PoseDestination),ARG(NativeScriptType_PoseGraph)}},
    [NativeScriptBinding_PoseBlendGraph]={.name="blend_graph",.parent=NativeScriptBinding_Pose,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=4,.arguments={ARG(NativeScriptType_PoseDestination),ARG(NativeScriptType_PoseGraph),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_PoseBlendToLive]={.name="blend_to_live",.parent=NativeScriptBinding_Pose,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_PoseCopyToLive]={.name="copy_to_live",.parent=NativeScriptBinding_Pose,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},

    K0(NativeScriptBinding_FacingFromInputThreshold,"from_input_threshold",NativeScriptBinding_Facing,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_PhysicsForceAirAndUseJumps,"force_air_and_use_jumps",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_PhysicsSwitchToGround,"switch_to_ground",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_CollisionSkipSoftFloor,"skip_soft_floor",NativeScriptBinding_Collision,NativeScriptType_Bool,NativeScriptFlag_MayInvalidateSource),
    K1(NativeScriptBinding_VisualsSetVisible,"set_visible",NativeScriptBinding_Visuals,NativeScriptType_Void,NativeScriptType_Bool,NativeScriptFlag_MayInvalidateSource),
    [NativeScriptBinding_VisualsSetGroup]={.name="set_group",.parent=NativeScriptBinding_Visuals,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_VisualsSetCurrentGroup]={.name="set_current_group",.parent=NativeScriptBinding_Visuals,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_VisualsCurrentGroup]={.name="current_group",.parent=NativeScriptBinding_Visuals,.flags=KINETIC_CALL|NativeScriptFlag_Pure,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_I32,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_VisualsPreviousGroup]={.name="previous_group",.parent=NativeScriptBinding_Visuals,.flags=KINETIC_CALL|NativeScriptFlag_Pure,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_I32,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    NS(NativeScriptBinding_Color,"color",0,ALL),
    [NativeScriptBinding_ColorStart]={.name="start",.parent=NativeScriptBinding_Color,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_Bool)}},
    K0(NativeScriptBinding_ColorClearAttack,"clear_attack",NativeScriptBinding_Color,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    PROP(NativeScriptBinding_CaptureArmed,"armed",NativeScriptBinding_Capture,F,NativeScriptType_Bool,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_AnimationHipRootCompensation,"hip_root_compensation",NativeScriptBinding_Animation,F,NativeScriptType_Bool,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_DamageForceTopBlastKO,"force_top_blast_ko",NativeScriptBinding_Damage,F,NativeScriptType_Bool,W|NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_DamageQuietKO,"quiet_ko",NativeScriptBinding_Damage,NativeScriptType_Void,NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_AirTerminalVelocity,"terminal_velocity",NativeScriptBinding_Air,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_ControlsStickSmoothing,"stick_smoothing",NativeScriptBinding_Controls,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_ControlsAerialJumpReverseThreshold,"aerial_jump_reverse_threshold",NativeScriptBinding_Controls,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_AirJumpHorizontalMultiplier,"jump_horizontal_multiplier",NativeScriptBinding_Air,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_ControlsTapJumpThreshold,"tap_jump_threshold",NativeScriptBinding_Controls,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_ControlsAerialActionDownThreshold,"aerial_action_down_threshold",NativeScriptBinding_Controls,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    NS(NativeScriptBinding_Model,"model",0,F),
    PROP(NativeScriptBinding_ModelInstanceScaleY,"instance_scale_y",NativeScriptBinding_Model,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_ModelScale,"scale",NativeScriptBinding_Model,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_ModelEffectScale,"effect_scale",NativeScriptBinding_Model,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    [NativeScriptBinding_VisualsSetGroupMaterialFrame]={.name="set_group_material_frame",.parent=NativeScriptBinding_Visuals,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=4,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_VisualsInitializeGroupMaterialAnimation]={.name="initialize_group_material_animation",.parent=NativeScriptBinding_Visuals,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_F32,.argument_count=4,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_F32)}},
    PROP(NativeScriptBinding_ModelRootScaleY,"root_scale_y",NativeScriptBinding_Model,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_ModelBaseScale,"base_scale",NativeScriptBinding_Model,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_ModelInstanceScaleZ,"instance_scale_z",NativeScriptBinding_Model,F,NativeScriptType_F32,W),
    NS(NativeScriptBinding_Costume,"costume",0,F),
    PROP(NativeScriptBinding_CostumeIndex,"index",NativeScriptBinding_Costume,F,NativeScriptType_I32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_VisualsOutlineColor,"outline_color",NativeScriptBinding_Visuals,F,NativeScriptType_I32,W),
    PROP(NativeScriptBinding_VisualsPreserveSecondaryMaterial,"preserve_secondary_material",NativeScriptBinding_Visuals,F,NativeScriptType_Bool,W),
    [NativeScriptBinding_ColorRGBA]={.name="rgba",.parent=NativeScriptBinding_Color,.flags=PURE_CALL,.kind=NativeScriptBinding_Intrinsic,.contexts=ALL,.result_type=NativeScriptType_I32,.argument_count=4,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_VisualsSetDiffuseColor]={.name="set_diffuse_color",.parent=NativeScriptBinding_Visuals,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_VisualsInstallOutlineVisibility]={.name="install_outline_visibility",.parent=NativeScriptBinding_Visuals,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    PROP(NativeScriptBinding_ItemOutlineColor,"outline_color",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_I32,W),
    PROP(NativeScriptBinding_ItemControllerTint,"controller_tint",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_I32,W),
    PROP(NativeScriptBinding_ItemControllerTintEnabled,"controller_tint_enabled",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_ItemMaterialOpacity,"material_opacity",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_I32,W),
    PROP(NativeScriptBinding_ItemLayered,"layered",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_Bool,W),
    [NativeScriptBinding_ItemSetDiffuseColor]={.name="set_diffuse_color",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    PROP(NativeScriptBinding_ItemsHeldId,"held_id",NativeScriptBinding_Items,F,NativeScriptType_I32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_ItemsAuxiliaryId,"auxiliary_id",NativeScriptBinding_Items,F,NativeScriptType_I32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_ItemsHeldKind,"held_kind",NativeScriptBinding_Items,F,NativeScriptType_I32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_ItemsHeldUseCategory,"held_use_category",NativeScriptBinding_Items,F,NativeScriptType_I32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_ItemsHoldKind,"hold_kind",NativeScriptBinding_Items,F,NativeScriptType_I32,R|NativeScriptFlag_Pure),
    [NativeScriptBinding_ItemsSetVisible]={.name="set_visible",.parent=NativeScriptBinding_Items,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_Bool)}},
    [NativeScriptBinding_ItemsSetPaused]={.name="set_paused",.parent=NativeScriptBinding_Items,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_Bool)}},
    [NativeScriptBinding_ItemsNotifyDrop]={.name="notify_drop",.parent=NativeScriptBinding_Items,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_Bool)}},
    K1(NativeScriptBinding_ItemsNotifyPickup,"notify_pickup",NativeScriptBinding_Items,NativeScriptType_Void,NativeScriptType_Bool,NativeScriptFlag_MayInvalidateSource),
    [NativeScriptBinding_ItemsNotifyHeldNeutral]={.name="notify_held_neutral",.parent=NativeScriptBinding_Items,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool},
    PROP(NativeScriptBinding_ItemsReopenBlocked,"reopen_blocked",NativeScriptBinding_Items,F,NativeScriptType_Bool,W),
    K1(NativeScriptBinding_ItemsThrowHeld,"throw_held",NativeScriptBinding_Items,NativeScriptType_Void,NativeScriptType_I32,NativeScriptFlag_MayInvalidateSource),
    [NativeScriptBinding_ItemsReleaseHeld]={.name="release_held",.parent=NativeScriptBinding_Items,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=4,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ItemsCreateEngine]={.name="create_engine",.parent=NativeScriptBinding_Items,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_I32,.argument_count=10,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_StateTaunt]={.name="taunt",.parent=NATIVE_SCRIPT_STATE_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_TargetsControllerColorPresent]={.name="controller_color_present",.parent=NativeScriptBinding_Targets,.flags=PURE_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={{.mode=NativeScriptArgument_Input,.object_or_integral=true}}},
    [NativeScriptBinding_TargetsControllerColor]={.name="controller_color",.parent=NativeScriptBinding_Targets,.flags=PURE_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_I32,.argument_count=1,.arguments={{.mode=NativeScriptArgument_Input,.object_or_integral=true}}},
    NS(NativeScriptBinding_Ceiling,"ceiling",0,F|A),
    NS(NativeScriptBinding_CeilingNormal,"normal",NativeScriptBinding_Ceiling,F|A),
    PROP(NativeScriptBinding_CeilingNormalX,"x",NativeScriptBinding_CeilingNormal,F|A,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_CeilingNormalY,"y",NativeScriptBinding_CeilingNormal,F|A,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_CeilingNormalZ,"z",NativeScriptBinding_CeilingNormal,F|A,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    NS(NativeScriptBinding_Wall,"wall",0,F|A),
    NS(NativeScriptBinding_WallLeft,"left",NativeScriptBinding_Wall,F|A),
    NS(NativeScriptBinding_WallRight,"right",NativeScriptBinding_Wall,F|A),
    NS(NativeScriptBinding_WallLeftNormal,"normal",NativeScriptBinding_WallLeft,F|A),
    NS(NativeScriptBinding_WallRightNormal,"normal",NativeScriptBinding_WallRight,F|A),
    PROP(NativeScriptBinding_WallLeftNormalX,"x",NativeScriptBinding_WallLeftNormal,F|A,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_WallLeftNormalY,"y",NativeScriptBinding_WallLeftNormal,F|A,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_WallLeftNormalZ,"z",NativeScriptBinding_WallLeftNormal,F|A,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_WallRightNormalX,"x",NativeScriptBinding_WallRightNormal,F|A,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_WallRightNormalY,"y",NativeScriptBinding_WallRightNormal,F|A,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_WallRightNormalZ,"z",NativeScriptBinding_WallRightNormal,F|A,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    NS(NativeScriptBinding_ECB,"ecb",0,F|A),
    NS(NativeScriptBinding_ECBTop,"top",NativeScriptBinding_ECB,F),
    NS(NativeScriptBinding_ECBBottom,"bottom",NativeScriptBinding_ECB,F),
    PROP(NativeScriptBinding_ECBTopY,"y",NativeScriptBinding_ECBTop,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_ECBBottomY,"y",NativeScriptBinding_ECBBottom,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    NS(NativeScriptBinding_ECBLeft,"left",NativeScriptBinding_ECB,F),
    NS(NativeScriptBinding_ECBRight,"right",NativeScriptBinding_ECB,F),
    PROP(NativeScriptBinding_ECBTopX,"x",NativeScriptBinding_ECBTop,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_ECBBottomX,"x",NativeScriptBinding_ECBBottom,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_ECBLeftX,"x",NativeScriptBinding_ECBLeft,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_ECBLeftY,"y",NativeScriptBinding_ECBLeft,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_ECBRightX,"x",NativeScriptBinding_ECBRight,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_ECBRightY,"y",NativeScriptBinding_ECBRight,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_ItemsBlockGroundPickup,"block_ground_pickup",NativeScriptBinding_Items,F,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_ArticlesAttachedVisible,"attached_visible",NativeScriptBinding_Articles,F,NativeScriptType_Bool,W|NativeScriptFlag_SourceOnly),
    K1(NativeScriptBinding_ItemsSetHeldVisible,"set_held_visible",NativeScriptBinding_Items,NativeScriptType_Void,NativeScriptType_Bool,NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ItemsRejectHealingPickup,"reject_healing_pickup",NativeScriptBinding_Items,F,NativeScriptType_Bool,W),
    NS(NativeScriptBinding_Capture,"capture",0,F),
    K1(NativeScriptBinding_CaptureMask,"mask",NativeScriptBinding_Capture,NativeScriptType_Void,NativeScriptType_I32,NativeScriptFlag_SourceOnly),
    K1(NativeScriptBinding_CaptureArm,"arm",NativeScriptBinding_Capture,NativeScriptType_Void,NativeScriptType_I32,NativeScriptFlag_SourceOnly),
    [NativeScriptBinding_CaptureBegin]={.name="begin",.parent=NativeScriptBinding_Capture,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=5,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_Bool),ARG(NativeScriptType_Bool),ARG(NativeScriptType_Bool),ARG(NativeScriptType_Bool)}},
    K0(NativeScriptBinding_CaptureClearMovement,"clear_movement",NativeScriptBinding_Capture,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_CaptureAttachToVictim,"attach_to_victim",NativeScriptBinding_Capture,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_CaptureFollowVictim,"follow_victim",NativeScriptBinding_Capture,NativeScriptType_Void,NativeScriptFlag_SourceOnly),
    K1(NativeScriptBinding_CaptureAttachItem,"attach_item",NativeScriptBinding_Capture,NativeScriptType_Void,NativeScriptType_I32,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    [NativeScriptBinding_CaptureReleaseItem]={.name="release_item",.parent=NativeScriptBinding_Capture,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_Bool)}},
    K0(NativeScriptBinding_CaptureReleaseIntoEnclosure,"release_into_enclosure",NativeScriptBinding_Capture,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_CaptureReleaseThrow,"release_throw",NativeScriptBinding_Capture,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K1(NativeScriptBinding_CaptureEncloseCaught,"enclose_caught",NativeScriptBinding_Capture,NativeScriptType_Void,NativeScriptType_F32,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_CaptureConstrainCaught,"constrain_caught",NativeScriptBinding_Capture,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K1(NativeScriptBinding_CaptureSetVictimBodyStatus,"set_victim_body_status",NativeScriptBinding_Capture,NativeScriptType_Void,NativeScriptType_I32,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K1(NativeScriptBinding_CaptureSetVictimCaptureMotion,"set_victim_capture_motion",NativeScriptBinding_Capture,NativeScriptType_Void,NativeScriptType_I32,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K1(NativeScriptBinding_CaptureInitializeHold,"initialize_hold",NativeScriptBinding_Capture,NativeScriptType_Void,NativeScriptType_I32,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K1(NativeScriptBinding_CaptureSetVictimHoldMotion,"set_victim_hold_motion",NativeScriptBinding_Capture,NativeScriptType_Void,NativeScriptType_I32,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    [NativeScriptBinding_CaptureSetVictimThrowMotion]={.name="set_victim_throw_motion",.parent=NativeScriptBinding_Capture,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_CaptureSetVictimAttachedThrow]={.name="set_victim_attached_throw",.parent=NativeScriptBinding_Capture,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=4,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_U32),ARG(NativeScriptType_F32),ARG(NativeScriptType_Bool)}},
    K0(NativeScriptBinding_CaptureSyncVictimFacing,"sync_victim_facing",NativeScriptBinding_Capture,NativeScriptType_Void,NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CaptureVictimFrame,"victim_frame",NativeScriptBinding_Capture,F,NativeScriptType_F32,R|NativeScriptFlag_SourceOnly),
    NS(NativeScriptBinding_Throws,"throws",0,F),
    K0(NativeScriptBinding_ThrowsUpdate,"update",NativeScriptBinding_Throws,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_PhysicsThrowMotion,"throw_motion",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    [NativeScriptBinding_CaptureReleaseDamage]={.name="release_damage",.parent=NativeScriptBinding_Capture,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_Bool),ARG(NativeScriptType_Bool)}},
    K0(NativeScriptBinding_CaptureReleaseDamageFall,"release_damage_fall",NativeScriptBinding_Capture,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_CaptureDropVictim,"drop_victim",NativeScriptBinding_Capture,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    [NativeScriptBinding_CaptureBeginAttached]={.name="begin_attached",.parent=NativeScriptBinding_Capture,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={ARG(NativeScriptType_I32)}},
    PROP(NativeScriptBinding_CaptureVictimFreezeState,"victim_freeze_state",NativeScriptBinding_Capture,F,NativeScriptType_Bool,R|NativeScriptFlag_SourceOnly),
    NS(NativeScriptBinding_Walk,"walk",0,F),
    [NativeScriptBinding_WalkEnter]={.name="enter",.parent=NativeScriptBinding_Walk,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=10,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_U32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    PROP(NativeScriptBinding_WalkType,"type",NativeScriptBinding_Walk,F,NativeScriptType_I32,R|NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_WalkUpdateAnimation,"update_animation",NativeScriptBinding_Walk,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_PhysicsWalk,"walk",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_PhysicsGroundAnimationFacingMotion,"ground_animation_facing_motion",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    PROP(NativeScriptBinding_ControlsThrowHorizontalThreshold,"throw_horizontal_threshold",NativeScriptBinding_Controls,F,NativeScriptType_F32,R|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ControlsThrowUpThreshold,"throw_up_threshold",NativeScriptBinding_Controls,F,NativeScriptType_F32,R|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ControlsThrowDownThreshold,"throw_down_threshold",NativeScriptBinding_Controls,F,NativeScriptType_F32,R|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CaptureVictimAnimationFamily,"victim_animation_family",NativeScriptBinding_Capture,F,NativeScriptType_I32,R|NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_ControlsWalkRequested,"walk_requested",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_ControlsWalkStopRequested,"walk_stop_requested",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_ControlsTurnRequested,"turn_requested",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_ControlsPlatformDropRequested,"platform_drop_requested",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_ControlsJumpInput,"jump_input",NativeScriptBinding_Controls,NativeScriptType_I32,NativeScriptFlag_SourceOnly),
    [NativeScriptBinding_JumpPrepare]={.name="prepare",.parent=NativeScriptBinding_Jump,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_Bool)}},
    K0(NativeScriptBinding_JumpCheckShortHop,"check_short_hop",NativeScriptBinding_Jump,NativeScriptType_Void,NativeScriptFlag_SourceOnly),
    [NativeScriptBinding_JumpLaunch]={.name="launch",.parent=NativeScriptBinding_Jump,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_Bool),ARG(NativeScriptType_F32)}},
    K0(NativeScriptBinding_PhysicsJumpMotion,"jump_motion",NativeScriptBinding_Physics,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    [NativeScriptBinding_StatePlatformDrop]={.name="platform_drop",.parent=NATIVE_SCRIPT_STATE_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_U32),ARG(NativeScriptType_F32)}},
    K0(NativeScriptBinding_StateFinish,"finish",NATIVE_SCRIPT_STATE_NAMESPACE,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    PROP(NativeScriptBinding_CaptureGroundLink,"ground_link",NativeScriptBinding_Capture,F,NativeScriptType_Bool,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CaptureHasVictim,"has_victim",NativeScriptBinding_Capture,F,NativeScriptType_Bool,R|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CaptureHasItem,"has_item",NativeScriptBinding_Capture,F,NativeScriptType_Bool,R|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CaptureVictimVisible,"victim_visible",NativeScriptBinding_Capture,F,NativeScriptType_Bool,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CaptureVictimDamage,"victim_damage",NativeScriptBinding_Capture,F,NativeScriptType_F32,R|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CaptureTimerDamageMultiplier,"timer_damage_multiplier",NativeScriptBinding_Capture,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CaptureTimerDecay,"timer_decay",NativeScriptBinding_Capture,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CaptureMashReduction,"mash_reduction",NativeScriptBinding_Capture,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CaptureMashPauseFrames,"mash_pause_frames",NativeScriptBinding_Capture,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CaptureMashAnimationRate,"mash_animation_rate",NativeScriptBinding_Capture,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CaptureWiggleScale,"wiggle_scale",NativeScriptBinding_Capture,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CaptureWiggleLimit,"wiggle_limit",NativeScriptBinding_Capture,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CaptureEnclosureModelSlot,"enclosure_model_slot",NativeScriptBinding_Capture,F,NativeScriptType_I32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CaptureEnclosureLaunchX,"enclosure_launch_x",NativeScriptBinding_Capture,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CaptureEnclosureLaunchY,"enclosure_launch_y",NativeScriptBinding_Capture,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CaptureEnclosureLaunchZ,"enclosure_launch_z",NativeScriptBinding_Capture,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CaptureEnclosureDamageBehavior,"enclosure_damage_behavior",NativeScriptBinding_Capture,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CaptureEnclosureShrinkFraction,"enclosure_shrink_fraction",NativeScriptBinding_Capture,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CaptureEnclosureShrinkFrames,"enclosure_shrink_frames",NativeScriptBinding_Capture,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CaptureEnclosureLifetime,"enclosure_lifetime",NativeScriptBinding_Capture,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CaptureEnclosureDamageTimeScale,"enclosure_damage_time_scale",NativeScriptBinding_Capture,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CaptureEnclosureEscapeX,"enclosure_escape_x",NativeScriptBinding_Capture,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CaptureEnclosureEscapeY,"enclosure_escape_y",NativeScriptBinding_Capture,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CaptureEnclosureEscapeZ,"enclosure_escape_z",NativeScriptBinding_Capture,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CaptureEnclosureEscapeInvincibilityFrames,"enclosure_escape_invincibility_frames",NativeScriptBinding_Capture,F,NativeScriptType_I32,W|NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_StateDamageFall,"damage_fall",NATIVE_SCRIPT_STATE_NAMESPACE,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K1(NativeScriptBinding_StateCliffJumpPhase2,"cliff_jump_phase2",NATIVE_SCRIPT_STATE_NAMESPACE,NativeScriptType_Void,NativeScriptType_F32,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    [NativeScriptBinding_CollisionProbeLedge]={.name="probe_ledge",.parent=NativeScriptBinding_Collision,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=2,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    K0(NativeScriptBinding_CollisionLedgeAvailable,"ledge_available",NativeScriptBinding_Collision,NativeScriptType_Bool,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    [NativeScriptBinding_ItemSetCollisionFacing]={.name="set_collision_facing",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_ItemResetDefaultEcb]={.name="reset_default_ecb",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_ItemSlideOnGround]={.name="slide_on_ground",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_ItemClearAnimationCommands]={.name="clear_animation_commands",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    NS(NativeScriptBinding_CollisionPosition,"position",NativeScriptBinding_Collision,F|A),
    PROP(NativeScriptBinding_CollisionPositionX,"x",NativeScriptBinding_CollisionPosition,F|A,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_CollisionPositionY,"y",NativeScriptBinding_CollisionPosition,F|A,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_CollisionPositionZ,"z",NativeScriptBinding_CollisionPosition,F|A,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    [NativeScriptBinding_TargetsModelScale]={.name="model_scale",.parent=NativeScriptBinding_Targets,.flags=PURE_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_F32,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_TargetsGuardActive]={.name="guard_active",.parent=NativeScriptBinding_Targets,.flags=PURE_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_TargetsShield]={.name="shield",.parent=NativeScriptBinding_Targets,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Bool,.argument_count=5,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false}},.publication=NativeScriptOutputs_SuccessOnly},
    [NativeScriptBinding_TargetsReadCameraTargetPosition]={.name="read_camera_target_position",.parent=NativeScriptBinding_Targets,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Bool,.argument_count=4,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false}},.publication=NativeScriptOutputs_SuccessOnly},
    [NativeScriptBinding_ItemSetAnimationDescriptor]={.name="set_animation_descriptor",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_ItemAddAnimationDescriptor]={.name="add_animation_descriptor",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_ItemReleaseAttachment]={.name="release_attachment",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ItemReleaseAttachmentPreserveHeld]={.name="",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=4,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_Bool)}},
    PROP(NativeScriptBinding_FighterActionDetachesItems,"action_detaches_items",NativeScriptBinding_Fighter,F,NativeScriptType_Bool,R),
    PROP(NativeScriptBinding_FighterCreationID,"creation_id",NativeScriptBinding_Fighter,F,NativeScriptType_U32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_FacingSign,"sign",NativeScriptBinding_Facing,F,NativeScriptType_I32,R|NativeScriptFlag_Pure),
    [NativeScriptBinding_EffectsSpawnAtPosition]={.name="spawn_at_position",.parent=NativeScriptBinding_Effects,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=4,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    PROP(NativeScriptBinding_ControlsHorizontalStickDeadzone,"horizontal_stick_deadzone",NativeScriptBinding_Controls,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    [NativeScriptBinding_HitboxState]={.name="state",.parent=NativeScriptBinding_Hitbox,.flags=PURE_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_I32,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_HitboxDamage]={.name="damage",.parent=NativeScriptBinding_Hitbox,.flags=PURE_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_F32,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_HitboxGroup]={.name="group",.parent=NativeScriptBinding_Hitbox,.flags=PURE_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_U32,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_HitboxSetGroup]={.name="set_group",.parent=NativeScriptBinding_Hitbox,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_HitboxCurrent]={.name="current",.parent=NativeScriptBinding_Hitbox,.flags=PURE_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Namespace,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    PROP(NativeScriptBinding_HitboxCurrentX,"x",NativeScriptBinding_HitboxCurrent,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_HitboxCurrentY,"y",NativeScriptBinding_HitboxCurrent,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_HitboxCurrentZ,"z",NativeScriptBinding_HitboxCurrent,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    [NativeScriptBinding_HitboxPrevious]={.name="previous",.parent=NativeScriptBinding_Hitbox,.flags=PURE_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Namespace,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    PROP(NativeScriptBinding_HitboxPreviousX,"x",NativeScriptBinding_HitboxPrevious,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_HitboxPreviousY,"y",NativeScriptBinding_HitboxPrevious,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_HitboxPreviousZ,"z",NativeScriptBinding_HitboxPrevious,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    [NativeScriptBinding_HitboxSetWorldPosition]={.name="set_world_position",.parent=NativeScriptBinding_Hitbox,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=4,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_HitboxSetEnabled]={.name="set_enabled",.parent=NativeScriptBinding_Hitbox,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_Bool)}},
    [NativeScriptBinding_HitboxClearVictims]={.name="clear_victims",.parent=NativeScriptBinding_Hitbox,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    K0(NativeScriptBinding_HitboxClearAll,"clear_all",NativeScriptBinding_Hitbox,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    [NativeScriptBinding_HitboxCreate]={.name="create",.parent=NativeScriptBinding_Hitbox,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_HitboxDefinition)}},
    [NativeScriptBinding_HitboxLaunchTarget] = {
        .name = "set_launch_target", .parent = NativeScriptBinding_Hitbox,
        .flags = KINETIC_CALL | NativeScriptFlag_SourceOnly,
        .kind = NativeScriptBinding_Intrinsic, .contexts = F,
        .result_type = NativeScriptType_Void, .argument_count = 4,
        .arguments = {{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},
                      ARG(NativeScriptType_F32), ARG(NativeScriptType_F32),
                      ARG(NativeScriptType_F32)},
    },
    [NativeScriptBinding_HitboxLaunchFollow] = {
        .name = "set_launch_follow", .parent = NativeScriptBinding_Hitbox,
        .flags = KINETIC_CALL | NativeScriptFlag_SourceOnly,
        .kind = NativeScriptBinding_Intrinsic, .contexts = F,
        .result_type = NativeScriptType_Void, .argument_count = 5,
        .arguments = {{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},
                      ARG(NativeScriptType_F32), ARG(NativeScriptType_F32),
                      ARG(NativeScriptType_F32), ARG(NativeScriptType_F32)},
    },
    [NativeScriptBinding_Hitlag]={.name="hitlag",.kind=NativeScriptBinding_Namespace,.contexts=F,.result_type=NativeScriptType_Namespace},
    PROP(NativeScriptBinding_HitlagActive,"active",NativeScriptBinding_Hitlag,F,NativeScriptType_Bool,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_HitlagLinkedId,"linked_id",NativeScriptBinding_Hitlag,F,NativeScriptType_I32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_HitlagRequestRelease,"request_release",NativeScriptBinding_Hitlag,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_HitlagUnlink,"unlink",NativeScriptBinding_Hitlag,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    [NativeScriptBinding_HitlagLink]={.name="link",.parent=NativeScriptBinding_Hitlag,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    K0(NativeScriptBinding_ControlsTrySideSmash,"try_side_smash",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_ControlsTryUpSmash,"try_up_smash",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_ControlsTryDownSmash,"try_down_smash",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_ControlsTrySideTilt,"try_side_tilt",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_ControlsTryUpTilt,"try_up_tilt",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_ControlsTryDownTilt,"try_down_tilt",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_ControlsTryJab,"try_jab",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_ControlsTryGroundJump,"try_ground_jump",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_ControlsTryDash,"try_dash",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_ControlsTryCrouch,"try_crouch",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_ControlsTryTurn,"try_turn",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_ControlsTryWalk,"try_walk",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly),
    STATE(NativeScriptBinding_StateCrouchWait,"crouch_wait",0),
    NS(NativeScriptBinding_Stats,"stats",0,F),
    K0(NativeScriptBinding_StatsCountAsThrownItem,"count_as_thrown_item",NativeScriptBinding_Stats,NativeScriptType_Void,NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_StatsRefreshAttack,"refresh_attack",NativeScriptBinding_Stats,NativeScriptType_Void,NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_StatsRecordAttack,"record_attack",NativeScriptBinding_Stats,NativeScriptType_Void,NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_StatsNotifyTaunt,"notify_taunt",NativeScriptBinding_Stats,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    [NativeScriptBinding_StatsActionCount]={.name="action_count",.parent=NativeScriptBinding_Stats,.flags=PURE_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_U32,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    PROP(NativeScriptBinding_FighterPlayerSlot,"player_slot",NativeScriptBinding_Fighter,F,NativeScriptType_I32,R),
    PROP(NativeScriptBinding_InputDownPressAge,"down_press_age",NativeScriptBinding_Input,F,NativeScriptType_I32,R),
    [NativeScriptBinding_StageEvent]={.name="event",.parent=NativeScriptBinding_Stage,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Bool,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_F32)}},
    PROP(NativeScriptBinding_MovementSpecialLandingLag,"special_landing_lag",NativeScriptBinding_Movement,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    NS(NativeScriptBinding_Engine,"engine",0,F),
    PROP(NativeScriptBinding_EngineDebugLevel,"debug_level",NativeScriptBinding_Engine,F,NativeScriptType_I32,R|NativeScriptFlag_Pure),
    K0(NativeScriptBinding_CollisionContactReboundAllowed,"contact_rebound_allowed",NativeScriptBinding_Collision,NativeScriptType_Bool,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    PROP(NativeScriptBinding_StatsLandingAirNeutralLag,"landing_air_neutral_lag",NativeScriptBinding_Stats,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_StatsLandingAirBackLag,"landing_air_back_lag",NativeScriptBinding_Stats,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    [NativeScriptBinding_ItemDisableSurfaceResponse]={.name="disable_surface_response",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_ItemSweepStageFlags]={.name="sweep_stage_flags",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_I32},
    PROP(NativeScriptBinding_ItemAnimationFinished,"animation_finished",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_Bool,R|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ItemsHeldMotion,"held_motion",NativeScriptBinding_Items,F,NativeScriptType_I32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_ItemsHeldFrame,"held_frame",NativeScriptBinding_Items,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    NS(NativeScriptBinding_Absorber,"absorber",0,F),
    [NativeScriptBinding_AbsorberCreate]={.name="create",.parent=NativeScriptBinding_Absorber,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=5,.arguments={{.type=NativeScriptType_I32,.mode=NativeScriptArgument_Input},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Input},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Input},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Input},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Input}}},
    PROP(NativeScriptBinding_AbsorberActive,"active",NativeScriptBinding_Absorber,F,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_AbsorberHit,"hit",NativeScriptBinding_Absorber,F,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_AbsorberSkipPositionUpdate,"skip_position_update",NativeScriptBinding_Absorber,F,NativeScriptType_Bool,W),
    PROP(NativeScriptBinding_AbsorberDamage,"damage",NativeScriptBinding_Absorber,F,NativeScriptType_I32,R),
    PROP(NativeScriptBinding_AbsorberHits,"hits",NativeScriptBinding_Absorber,F,NativeScriptType_I32,R),
    PROP(NativeScriptBinding_AbsorberDirection,"direction",NativeScriptBinding_Absorber,F,NativeScriptType_F32,R),
    [NativeScriptBinding_ItemSpawnEffectAtPosition]={.name="spawn_effect_at_position",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=4,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Input},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Input},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Input}}},
    [NativeScriptBinding_ItemSpawnRootEffect]={.name="spawn_root_effect",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_ItemSpawnRootEffectWithOffset]={.name="spawn_root_effect_with_offset",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=4,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ItemDestroyEffects]={.name="destroy_effects",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_CallbackMatches]={.name="matches",.parent=NativeScriptBinding_Callbacks,.flags=PURE_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_Callable,.mode=NativeScriptArgument_CodeRef,.numeric_any=false}}},
    [NativeScriptBinding_ArticlesSignal]={.name="signal",.parent=NativeScriptBinding_Articles,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Bool,.argument_count=4,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_I32),{.type=NativeScriptType_Text,.mode=NativeScriptArgument_Literal,.numeric_any=false},ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ArticlesSignalOwned]={.name="signal_owned",.parent=NativeScriptBinding_Articles,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=4,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_I32),{.type=NativeScriptType_Text,.mode=NativeScriptArgument_Literal,.numeric_any=false},ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ArticlesLinkOwned]={.name="link_owned",.parent=NativeScriptBinding_Articles,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=3,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_I32),{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_ItemLinkId]={.name="link_id",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_I32,.argument_count=1,.arguments={ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_ItemLinkReciprocal]={.name="link_reciprocal",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Bool,.argument_count=2,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_ItemUnlink]={.name="unlink",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_ItemCopyLinkCollision]={.name="copy_link_collision",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_ItemSignal]={.name="signal",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Bool,.argument_count=2,.arguments={{.type=NativeScriptType_Text,.mode=NativeScriptArgument_Literal,.numeric_any=false},ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ArticlesCreateAtPosition]={.name="create_at_position",.parent=NativeScriptBinding_Articles,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_I32,.argument_count=6,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_Bool)}},
    [NativeScriptBinding_ArticlesCreateUnownedAtPosition]={.name="create_unowned_at_position",.parent=NativeScriptBinding_Articles,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_I32,.argument_count=8,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ArticlesCreateHeldAtCenter]={.name="create_held_at_center",.parent=NativeScriptBinding_Articles,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_I32,.argument_count=6,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ArticlesCreateWithVelocity]={.name="create_with_velocity",.parent=NativeScriptBinding_Articles,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_I32,.argument_count=12,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_Bool),{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_F32),ARG(NativeScriptType_Bool)}},
    [NativeScriptBinding_ArticlesRef]={.name="ref",.parent=NativeScriptBinding_Articles,.flags=PURE_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_ReceiverCapture,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Object,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_ArticlesReadPrimaryTranslation]={.name="read_primary_translation",.parent=NativeScriptBinding_Articles,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Bool,.argument_count=5,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_I32),{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false}},.publication=NativeScriptOutputs_SuccessOnly},
    [NativeScriptBinding_ArticlesReadBonePosition]={.name="read_bone_position",.parent=NativeScriptBinding_Articles,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Bool,.argument_count=6,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_I32),ARG(NativeScriptType_I32),{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false}},.publication=NativeScriptOutputs_SuccessOnly},
    NS(NativeScriptBinding_Vectors,"vectors",0,F|A),
    [NativeScriptBinding_VectorsRotate]={.name="rotate",.parent=NativeScriptBinding_Vectors,.flags=KINETIC_CALL,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Void,.argument_count=8,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_I32),ARG(NativeScriptType_F32),{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output}},.publication=NativeScriptOutputs_AlwaysAfterGuard},
    [NativeScriptBinding_VectorsNormalize]={.name="normalize",.parent=NativeScriptBinding_Vectors,.flags=KINETIC_CALL,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={{.type=NativeScriptType_F32,.mode=NativeScriptArgument_InputOutput},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_InputOutput},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_InputOutput}},.publication=NativeScriptOutputs_AlwaysAfterGuard},
    [NativeScriptBinding_VectorsNormalizeXY]={.name="normalize_xy",.parent=NativeScriptBinding_Vectors,.flags=KINETIC_CALL,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={{.type=NativeScriptType_F32,.mode=NativeScriptArgument_InputOutput},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_InputOutput},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_InputOutput}},.publication=NativeScriptOutputs_AlwaysAfterGuard},
    PROP(NativeScriptBinding_ECBUp,"up",NativeScriptBinding_ECB,A,NativeScriptType_F32,R|W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ECBDown,"down",NativeScriptBinding_ECB,A,NativeScriptType_F32,R|W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ECBFront,"front",NativeScriptBinding_ECB,A,NativeScriptType_F32,R|W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ECBBack,"back",NativeScriptBinding_ECB,A,NativeScriptType_F32,R|W|NativeScriptFlag_SourceOnly),
    [NativeScriptBinding_VectorsAngleXY]={.name="angle_xy",.parent=NativeScriptBinding_Vectors,.flags=PURE_CALL,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_F32,.argument_count=4,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_VectorsAngle]={.name="angle",.parent=NativeScriptBinding_Vectors,.flags=PURE_CALL,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_F32,.argument_count=6,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_VectorsCrossNormalizedX]={.name="cross_normalized_x",.parent=NativeScriptBinding_Vectors,.flags=PURE_CALL,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_F32,.argument_count=6,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_VectorsCrossNormalizedY]={.name="cross_normalized_y",.parent=NativeScriptBinding_Vectors,.flags=PURE_CALL,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_F32,.argument_count=6,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_VectorsCrossNormalizedZ]={.name="cross_normalized_z",.parent=NativeScriptBinding_Vectors,.flags=PURE_CALL,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_F32,.argument_count=6,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ItemClearCurrentOwner]={.name="clear_current_owner",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_ItemClearAttachmentOwner]={.name="clear_attachment_owner",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_ItemDetachOwner]={.name="detach_owner",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_ItemAttachToOwner]={.name="attach_to_owner",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_ItemAttachToOwnerAux]={.name="attach_to_owner",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_Bool)}},
    [NativeScriptBinding_ArticlesBindAuxiliary]={.name="bind_auxiliary",.parent=NativeScriptBinding_Articles,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    K0(NativeScriptBinding_ItemsClearHeld,"clear_held",NativeScriptBinding_Items,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_ItemsClearAuxiliary,"clear_auxiliary",NativeScriptBinding_Items,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource),
    [NativeScriptBinding_ItemsBindHeld]={.name="bind_held",.parent=NativeScriptBinding_Items,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_ItemsBindAuxiliary]={.name="bind_auxiliary",.parent=NativeScriptBinding_Items,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_ItemsBindExistingHeld]={.name="bind_existing_held",.parent=NativeScriptBinding_Items,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_ItemsBindExistingAuxiliary]={.name="bind_existing_auxiliary",.parent=NativeScriptBinding_Items,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_ArticlesBindHeld]={.name="bind_held",.parent=NativeScriptBinding_Articles,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_ItemsAttach]={.name="attach",.parent=NativeScriptBinding_Items,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=3,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_I32),ARG(NativeScriptType_Bool)}},
    [NativeScriptBinding_ItemsInheritOwnerScale]={.name="inherit_owner_scale",.parent=NativeScriptBinding_Items,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_Bool)}},
    PROP(NativeScriptBinding_CPUPendingAction,"pending_action",NativeScriptBinding_CPU,F,NativeScriptType_Bool,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CPUBehavior,"behavior",NativeScriptBinding_CPU,F,NativeScriptType_I32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CPUPreviousBehavior,"previous_behavior",NativeScriptBinding_CPU,F,NativeScriptType_I32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CPUTravelFrames,"travel_frames",NativeScriptBinding_CPU,F,NativeScriptType_I32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CPUSelectedCommand,"selected_command",NativeScriptBinding_CPU,F,NativeScriptType_I32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CPUElapsedFrames,"elapsed_frames",NativeScriptBinding_CPU,F,NativeScriptType_I32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CPUFloorInBounds,"floor_in_bounds",NativeScriptBinding_CPU,F,NativeScriptType_Bool,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CPUTargetID,"target_id",NativeScriptBinding_CPU,F,NativeScriptType_U32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    [NativeScriptBinding_CPUDestination]={.name="destination",.parent=NativeScriptBinding_CPU,.kind=NativeScriptBinding_Namespace,.contexts=F,.result_type=NativeScriptType_Namespace,.flags=NativeScriptFlag_SourceOnly},
    PROP(NativeScriptBinding_CPUDestinationX,"x",NativeScriptBinding_CPUDestination,F,NativeScriptType_F32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CPUDestinationY,"y",NativeScriptBinding_CPUDestination,F,NativeScriptType_F32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_CPUClearTarget,"clear_target",NativeScriptBinding_CPU,NativeScriptType_Void,NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly),
    [NativeScriptBinding_CPUQueue]={.name="queue",.parent=NativeScriptBinding_CPU,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_CPUQueueArg]={.name="queue",.parent=NativeScriptBinding_CPU,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_I32),{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_CPUQueuePair]={.name="queue",.parent=NativeScriptBinding_CPU,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={ARG(NativeScriptType_I32),{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_CPUNearestEnemyAt]={.name="nearest_enemy_at",.parent=NativeScriptBinding_CPU,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_U32,.argument_count=3,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_Targets]={.name="targets",.kind=NativeScriptBinding_Namespace,.contexts=F|A,.result_type=NativeScriptType_Namespace,.flags=NativeScriptFlag_SourceOnly},
    [NativeScriptBinding_TargetsReadPosition]={.name="read_position",.parent=NativeScriptBinding_Targets,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Bool,.argument_count=4,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false}},.publication=NativeScriptOutputs_SuccessOnly},
    [NativeScriptBinding_TargetsReadPositionDelta]={.name="read_position_delta",.parent=NativeScriptBinding_Targets,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Bool,.argument_count=4,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false}},.publication=NativeScriptOutputs_SuccessOnly},
    [NativeScriptBinding_TargetsReadBonePosition]={.name="read_bone_position",.parent=NativeScriptBinding_Targets,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Bool,.argument_count=5,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false}},.publication=NativeScriptOutputs_SuccessOnly},
    [NativeScriptBinding_TargetsMotion]={.name="motion",.parent=NativeScriptBinding_Targets,.flags=PURE_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_I32,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    PROP(NativeScriptBinding_VisualsHeldItemVisible,"held_item_visible",NativeScriptBinding_Visuals,F,NativeScriptType_Bool,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_ControlsTrySideSpecial,"try_side_special",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_ControlsTryUpSpecial,"try_up_special",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_ControlsTryNeutralSpecial,"try_neutral_special",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_ControlsTryDownSpecial,"try_down_special",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    NS(NativeScriptBinding_Action,"action",0,F),
    PROP(NativeScriptBinding_ActionCategory,"category",NativeScriptBinding_Action,F,NativeScriptType_I32,R|NativeScriptFlag_Pure),
    [NativeScriptBinding_ActionMetadata]={.name="metadata",.parent=NativeScriptBinding_Action,.flags=PURE_CALL,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_I32,.argument_count=4,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_I32),ARG(NativeScriptType_I32),ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_ActionMoveId]={.name="move_id",.parent=NativeScriptBinding_Action,.flags=PURE_CALL,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_I32,.argument_count=1,.arguments={ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_VisualsApplyPartAnimation]={.name="apply_part_animation",.parent=NativeScriptBinding_Visuals,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_I32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_VisualsSetPartAnimation]={.name="set_part_animation",.parent=NativeScriptBinding_Visuals,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_VisualsShowPartAnimation]={.name="show_part_animation",.parent=NativeScriptBinding_Visuals,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_VisualsHidePartAnimation]={.name="hide_part_animation",.parent=NativeScriptBinding_Visuals,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={ARG(NativeScriptType_I32)}},
    PROP(NativeScriptBinding_FighterIncapacitated,"incapacitated",NativeScriptBinding_Fighter,F,NativeScriptType_Bool,R|NativeScriptFlag_Pure),
    NS(NativeScriptBinding_Audio,"audio",0,F|A),
    [NativeScriptBinding_AudioPlayGlobal]={.name="play_global",.parent=NativeScriptBinding_Audio,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_I32,.argument_count=3,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_I32),ARG(NativeScriptType_I32)}},
    K0(NativeScriptBinding_ControlsTryGuard,"try_guard",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    [NativeScriptBinding_CollisionCopyFrom]={.name="copy_from",.parent=NativeScriptBinding_Collision,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={ARG(NativeScriptType_I32)}},
    K0(NativeScriptBinding_CollisionAirCatchHit,"air_catch_hit",NativeScriptBinding_Collision,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    PROP(NativeScriptBinding_MotionAllowsControllerSync,"allows_controller_sync",NativeScriptBinding_Motion,F,NativeScriptType_Bool,R|NativeScriptFlag_Pure),
    NS(NativeScriptBinding_Spawn,"spawn",0,F),
    PROP(NativeScriptBinding_SpawnHorizontalOffset,"horizontal_offset",NativeScriptBinding_Spawn,F,NativeScriptType_F32,W),
    K0(NativeScriptBinding_ControlsTryCatch,"try_catch",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_ControlsTryHeavyItemThrow,"try_heavy_item_throw",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_ControlsTryTaunt,"try_taunt",NativeScriptBinding_Controls,NativeScriptType_Bool,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_CPUPrepareDestinationUpdate,"prepare_destination_update",NativeScriptBinding_CPU,NativeScriptType_Bool,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    [NativeScriptBinding_EffectsSpawnWithScalarAtPosition]={.name="spawn_with_scalar_at_position",.parent=NativeScriptBinding_Effects,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=5,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_EffectsSpawnWithTwoScalarsAtPosition]={.name="spawn_with_two_scalars_at_position",.parent=NativeScriptBinding_Effects,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=6,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ItemFollowGround]={.name="follow_ground",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_ItemBounceToGround]={.name="bounce_to_ground",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_ItemBounceToGroundWithContactReset]={.name="",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={ARG(NativeScriptType_Bool)}},
    PROP(NativeScriptBinding_GroundMidWalkPoint,"mid_walk_point",NativeScriptBinding_Ground,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    [NativeScriptBinding_TargetsDamage]={.name="damage",.parent=NativeScriptBinding_Targets,.flags=PURE_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_F32,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_TargetsFacing]={.name="facing",.parent=NativeScriptBinding_Targets,.flags=PURE_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_F32,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_TargetsCpuBodyHalfWidth]={.name="cpu_body_half_width",.parent=NativeScriptBinding_Targets,.flags=PURE_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_F32,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_TargetsSleeping]={.name="sleeping",.parent=NativeScriptBinding_Targets,.flags=PURE_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_TargetsFloorLine]={.name="floor_line",.parent=NativeScriptBinding_Targets,.flags=PURE_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_I32,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_StageReadFloor]={.name="read_floor",.parent=NativeScriptBinding_Stage,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Bool,.argument_count=7,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_I32,.mode=NativeScriptArgument_Output,.numeric_any=false}},.publication=NativeScriptOutputs_SuccessOnly},
    [NativeScriptBinding_StageReadLineIslandBounds]={.name="read_line_island_bounds",.parent=NativeScriptBinding_Stage,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Bool,.argument_count=5,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false}},.publication=NativeScriptOutputs_SuccessOnly},
    [NativeScriptBinding_StageReadLineIslandBounds3D]={.name="",.parent=NativeScriptBinding_Stage,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Bool,.argument_count=7,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false}},.publication=NativeScriptOutputs_SuccessOnly},
    [NativeScriptBinding_StageSameLineIsland]={.name="same_line_island",.parent=NativeScriptBinding_Stage,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Bool,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_StageCpuFloorExcluded]={.name="cpu_floor_excluded",.parent=NativeScriptBinding_Stage,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_StageCpuIslandBlocked]={.name="cpu_island_blocked",.parent=NativeScriptBinding_Stage,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_StageCompareFloorIslands]={.name="compare_floor_islands",.parent=NativeScriptBinding_Stage,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Void,.argument_count=12,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_Bool),{.type=NativeScriptType_Bool,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_Bool,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false}},.publication=NativeScriptOutputs_AlwaysAfterGuard},
    K1(NativeScriptBinding_CPURecordInput,"record_input",NativeScriptBinding_CPU,NativeScriptType_Bool,NativeScriptType_I32,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    [NativeScriptBinding_StageFollowFloor]={.name="follow_floor",.parent=NativeScriptBinding_Stage,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Void,.argument_count=10,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),{.type=NativeScriptType_Bool,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false}},.publication=NativeScriptOutputs_AlwaysAfterGuard},
    [NativeScriptBinding_StageIslandBounds]={.name="island_bounds",.parent=NativeScriptBinding_Stage,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Void,.argument_count=9,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),{.type=NativeScriptType_Bool,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false}},.publication=NativeScriptOutputs_AlwaysAfterGuard},
    K0(NativeScriptBinding_CPUApplyRecordedInput,"apply_recorded_input",NativeScriptBinding_CPU,NativeScriptType_Bool,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    [NativeScriptBinding_CPUReadHistoryPose]={.name="read_history_pose",.parent=NativeScriptBinding_CPU,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Bool,.argument_count=4,.arguments={{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false}},.publication=NativeScriptOutputs_SuccessOnly},
    K0(NativeScriptBinding_CPUPrepareCombatPlan,"prepare_combat_plan",NativeScriptBinding_CPU,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_CPUUpdateRecoveryDestination,"update_recovery_destination",NativeScriptBinding_CPU,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_CPUPlanAttack,"plan_attack",NativeScriptBinding_CPU,NativeScriptType_Bool,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    K0(NativeScriptBinding_CPUPlanWithoutTarget,"plan_without_target",NativeScriptBinding_CPU,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    NS(NativeScriptBinding_CPUOutput,"output",NativeScriptBinding_CPU,F),
    PROP(NativeScriptBinding_CPUOutputLeftX,"left_x",NativeScriptBinding_CPUOutput,F,NativeScriptType_I32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CPUOutputLeftY,"left_y",NativeScriptBinding_CPUOutput,F,NativeScriptType_I32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CPUOutputLeftTrigger,"left_trigger",NativeScriptBinding_CPUOutput,F,NativeScriptType_I32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CPUOutputRightTrigger,"right_trigger",NativeScriptBinding_CPUOutput,F,NativeScriptType_I32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CPUOutputButtons,"buttons",NativeScriptBinding_CPUOutput,F,NativeScriptType_I32,W|NativeScriptFlag_SourceOnly),
    [NativeScriptBinding_CPUSetDestination]={.name="set_destination",.parent=NativeScriptBinding_CPU,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    K1(NativeScriptBinding_CPUSetApproachSpacing,"set_approach_spacing",NativeScriptBinding_CPU,NativeScriptType_Void,NativeScriptType_F32,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    PROP(NativeScriptBinding_CPUApproachSpacing,"approach_spacing",NativeScriptBinding_CPU,F,NativeScriptType_F32,R|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CPUMode,"mode",NativeScriptBinding_CPU,F,NativeScriptType_I32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CPUFollowDistance,"follow_distance",NativeScriptBinding_CPU,F,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CPUCombatCandidatesEnabled,"combat_candidates_enabled",NativeScriptBinding_CPU,F,NativeScriptType_Bool,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_CPURangedGateEnabled,"ranged_gate_enabled",NativeScriptBinding_CPU,F,NativeScriptType_Bool,W|NativeScriptFlag_SourceOnly),
    [NativeScriptBinding_ItemSpawnAsyncJointEffect]={.name="spawn_async_joint_effect",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={ARG(NativeScriptType_I32),{.type=NativeScriptType_Text,.mode=NativeScriptArgument_Literal,.numeric_any=false},{.type=NativeScriptType_I32,.mode=NativeScriptArgument_Literal,.numeric_any=false}}},
    [NativeScriptBinding_ItemSpawnAsyncJointEffectWithScalar]={.name="spawn_async_joint_effect",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=4,.arguments={ARG(NativeScriptType_I32),{.type=NativeScriptType_Text,.mode=NativeScriptArgument_Literal,.numeric_any=false},{.type=NativeScriptType_I32,.mode=NativeScriptArgument_Literal,.numeric_any=false},ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ItemRespondToStage]={.name="respond_to_stage",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_ItemRespondToStageWithImpactResponse]={.name="",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={ARG(NativeScriptType_Bool)}},
    PROP(NativeScriptBinding_ControlsHorizontalSmashThreshold,"horizontal_smash_threshold",NativeScriptBinding_Controls,F,NativeScriptType_F32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ControlsHorizontalSmashWindow,"horizontal_smash_window",NativeScriptBinding_Controls,F,NativeScriptType_F32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ControlsHorizontalSmashExtension,"horizontal_smash_extension",NativeScriptBinding_Controls,F,NativeScriptType_F32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    K0(NativeScriptBinding_CallbackAlignVelocityToFacing,"align_velocity_to_facing",NativeScriptBinding_Callbacks,NativeScriptType_Void,NativeScriptFlag_SourceOnly),
    [NativeScriptBinding_StageLineExists]={.name="line_exists",.parent=NativeScriptBinding_Stage,.flags=PURE_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_StageLineNormal]={.name="line_normal",.parent=NativeScriptBinding_Stage,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Void,.argument_count=4,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false}},.publication=NativeScriptOutputs_AlwaysAfterGuard},
    [NativeScriptBinding_ItemBindStageLine]={.name="bind_stage_line",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_ItemReadStageLineSpeed]={.name="read_stage_line_speed",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_ItemSweepSegment]={.name="sweep_segment",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=9,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),{.type=NativeScriptType_F32,.mode=NativeScriptArgument_InputOutput,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_InputOutput,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_InputOutput,.numeric_any=false},{.type=NativeScriptType_I32,.mode=NativeScriptArgument_InputOutput,.numeric_any=false},{.type=NativeScriptType_I32,.mode=NativeScriptArgument_InputOutput,.numeric_any=false},{.type=NativeScriptType_Bool,.mode=NativeScriptArgument_Output,.numeric_any=false}},.publication=NativeScriptOutputs_AlwaysAfterGuard},
    [NativeScriptBinding_TargetsAirborne]={.name="airborne",.parent=NativeScriptBinding_Targets,.flags=PURE_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_TargetsSmashChargeActive]={.name="smash_charge_active",.parent=NativeScriptBinding_Targets,.flags=PURE_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_TargetsCall]={.name="call",.parent=NativeScriptBinding_Targets,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Bool,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_Text,.mode=NativeScriptArgument_Literal,.numeric_any=false}}},
    [NativeScriptBinding_TargetsCallBool]={.name="call_bool",.parent=NativeScriptBinding_Targets,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Bool,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_Text,.mode=NativeScriptArgument_Literal,.numeric_any=false}}},
    [NativeScriptBinding_TargetsCallInt]={.name="call_int",.parent=NativeScriptBinding_Targets,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_I32,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_Text,.mode=NativeScriptArgument_Literal,.numeric_any=false}}},
    [NativeScriptBinding_TargetsCallFloat]={.name="call_float",.parent=NativeScriptBinding_Targets,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_F32,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_Text,.mode=NativeScriptArgument_Literal,.numeric_any=false}}},
    [NativeScriptBinding_ItemAddModelAnimation]={.name="add_model_animation",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Bool,.argument_count=3,.arguments={ARG(NativeScriptType_I32),{.type=NativeScriptType_Text,.mode=NativeScriptArgument_Literal,.numeric_any=false},ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_ItemRequestModelAnimation]={.name="request_model_animation",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={ARG(NativeScriptType_I32),{.type=NativeScriptType_Text,.mode=NativeScriptArgument_Literal,.numeric_any=false},ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ItemRemoveModelAnimation]={.name="remove_model_animation",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_I32),{.type=NativeScriptType_Text,.mode=NativeScriptArgument_Literal,.numeric_any=false}}},
    [NativeScriptBinding_ItemLoadModel]={.name="load_model",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Bool,.argument_count=2,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_ItemDestroyModel]={.name="destroy_model",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_ItemCloneModel]={.name="clone_model",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={ARG(NativeScriptType_I32)}},
    [NativeScriptBinding_ItemSetModelDrawEnabled]={.name="set_model_draw_enabled",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_Bool)}},
    [NativeScriptBinding_ItemSetJointTransform]={.name="set_joint_transform",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=13,.arguments={ARG(NativeScriptType_I32),{.type=NativeScriptType_Text,.mode=NativeScriptArgument_Literal,.numeric_any=false},ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_Bool)}},
    [NativeScriptBinding_ItemJointWorldPosition]={.name="joint_world_position",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=5,.arguments={ARG(NativeScriptType_I32),{.type=NativeScriptType_Text,.mode=NativeScriptArgument_Literal,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false}},.publication=NativeScriptOutputs_AlwaysAfterGuard},
    [NativeScriptBinding_ItemReadJointMatrixRotation]={.name="read_joint_matrix_rotation",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=5,.arguments={ARG(NativeScriptType_I32),{.type=NativeScriptType_Text,.mode=NativeScriptArgument_Literal,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false}},.publication=NativeScriptOutputs_AlwaysAfterGuard},
    [NativeScriptBinding_ItemReadAttachmentMatrixRotation]={.name="read_attachment_matrix_rotation",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=3,.arguments={{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output,.numeric_any=false}},.publication=NativeScriptOutputs_AlwaysAfterGuard},
    [NativeScriptBinding_TargetsIsFighter]={.name="is_fighter",.parent=NativeScriptBinding_Targets,.flags=PURE_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_ItemSetScale]={.name="set_scale",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ItemSetAnimationFrame]={.name="set_animation_frame",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ItemCreateAtPosition]={.name="create_at_position",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_I32,.argument_count=5,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ItemCreateChild]={.name="create_child",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_I32,.argument_count=9,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_F32),{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_Hitbox]={.name="hitbox",.kind=NativeScriptBinding_Namespace,.contexts=F|A,.result_type=NativeScriptType_Namespace,.flags=NativeScriptFlag_SourceOnly},
    [NativeScriptBinding_Hitboxes]={.name="hitboxes",.kind=NativeScriptBinding_Namespace,.contexts=A,.result_type=NativeScriptType_Namespace,.flags=NativeScriptFlag_SourceOnly},
    [NativeScriptBinding_HitboxEnabled]={.name="enabled",.parent=NativeScriptBinding_Hitbox,.flags=PURE_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Bool,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_HitboxRadius]={.name="radius",.parent=NativeScriptBinding_Hitbox,.flags=PURE_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_F32,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_HitboxSetDamage]={.name="set_damage",.parent=NativeScriptBinding_Hitbox,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_HitboxSetRadius]={.name="set_radius",.parent=NativeScriptBinding_Hitbox,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_HitboxesDisableAll]={.name="disable_all",.parent=NativeScriptBinding_Hitboxes,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void},
    [NativeScriptBinding_ItemDisableHitbox]={.name="disable_hitbox",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=1,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_ItemCreationValue]={.name="creation_value",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.kind=NativeScriptBinding_Property,.contexts=A,.result_type=NativeScriptType_F32,.flags=R|NativeScriptFlag_SourceOnly},
    [NativeScriptBinding_ItemSpawnWithScalarAtPosition]={.name="spawn_with_scalar_at_position",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=5,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    PROP(NativeScriptBinding_ItemLaunchAngle,"launch_angle",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_F32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ItemLaunchSpeed,"launch_speed",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_F32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    [NativeScriptBinding_ItemLaunchOrigin]={.name="launch_origin",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.kind=NativeScriptBinding_Namespace,.contexts=A,.result_type=NativeScriptType_Namespace},
    PROP(NativeScriptBinding_ItemLaunchOriginX,"x",NativeScriptBinding_ItemLaunchOrigin,A,NativeScriptType_F32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ItemLaunchOriginY,"y",NativeScriptBinding_ItemLaunchOrigin,A,NativeScriptType_F32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ItemLaunchOriginZ,"z",NativeScriptBinding_ItemLaunchOrigin,A,NativeScriptType_F32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ItemReflectionFacing,"reflect_facing",NATIVE_SCRIPT_ITEM_NAMESPACE,A,NativeScriptType_F32,R|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly),
    [NativeScriptBinding_ItemCancelHitlagRequest]={.name="cancel_hitlag_request",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly},
    [NativeScriptBinding_ItemSurfaceNormal]={.name="surface_normal",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.kind=NativeScriptBinding_Namespace,.contexts=A,.result_type=NativeScriptType_Namespace},
    PROP(NativeScriptBinding_ItemSurfaceNormalX,"x",NativeScriptBinding_ItemSurfaceNormal,A,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ItemSurfaceNormalY,"y",NativeScriptBinding_ItemSurfaceNormal,A,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    PROP(NativeScriptBinding_ItemSurfaceNormalZ,"z",NativeScriptBinding_ItemSurfaceNormal,A,NativeScriptType_F32,W|NativeScriptFlag_SourceOnly),
    [NativeScriptBinding_ItemSweepStageNormal]={.name="sweep_stage_normal",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Bool,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.argument_count=3,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ItemCheckGround]={.name="check_ground",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Bool,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource},
    [NativeScriptBinding_ItemCreateLinked]={.name="create_linked",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_I32,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.argument_count=7,.arguments={ARG(NativeScriptType_I32),ARG(NativeScriptType_I32),ARG(NativeScriptType_I32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ItemSweepStage]={.name="sweep_stage",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Bool,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.argument_count=3,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ItemCurrentOwner]={.name="current_owner",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.kind=NativeScriptBinding_Property,.contexts=A,.result_type=NativeScriptType_U32,.flags=R|NativeScriptFlag_SourceOnly},
    [NativeScriptBinding_ItemInheritOwnerDebugDisplay]={.name="inherit_owner_debug_display",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource},
    [NativeScriptBinding_ItemStageCollisionMode]={.name="stage_collision",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.argument_count=1,.arguments={ARG(NativeScriptType_Bool)}},
    [NativeScriptBinding_ItemAdvanceAnimation]={.name="advance_animation",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource},
    [NativeScriptBinding_ItemAdvanceModelAnimation]={.name="advance_model_animation",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource},
    [NativeScriptBinding_ItemRumble]={.name="rumble",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_Camera]={.name="camera",.kind=NativeScriptBinding_Namespace,.contexts=F|A,.result_type=NativeScriptType_Namespace,.flags=NativeScriptFlag_SourceOnly},
    [NativeScriptBinding_CameraQuake]={.name="quake",.parent=NativeScriptBinding_Camera,.kind=NativeScriptBinding_Intrinsic,.contexts=F|A,.result_type=NativeScriptType_Void,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.argument_count=4,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    NS(NativeScriptBinding_Controller,"controller",0,F),
    [NativeScriptBinding_ItemSpawnJointEffect]={.name="spawn_joint_effect",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Void,.argument_count=2,.arguments={ARG(NativeScriptType_I32),{.type=NativeScriptType_Text,.mode=NativeScriptArgument_Literal,.numeric_any=false}}},
    PROP(NativeScriptBinding_GroundWalkAccelerationMultiplier,"walk_acceleration_multiplier",NativeScriptBinding_Ground,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_GroundWalkMaxSpeed,"walk_max_speed",NativeScriptBinding_Ground,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_AirGroundJumpMomentumMultiplier,"ground_jump_momentum_multiplier",NativeScriptBinding_Air,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_AirJumpHorizontalMax,"jump_horizontal_max",NativeScriptBinding_Air,F,NativeScriptType_F32,R|NativeScriptFlag_Pure),
    PROP(NativeScriptBinding_ActionCurrentMetadata,"current_metadata",NativeScriptBinding_Action,F,NativeScriptType_I32,R|NativeScriptFlag_Pure),
    K0(NativeScriptBinding_DamageInterruptCleanup,"interrupt_cleanup",NativeScriptBinding_Damage,NativeScriptType_Void,NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource),
    [NativeScriptBinding_TargetsReadOpponentAhead]={.name="read_opponent_ahead",.parent=NativeScriptBinding_Targets,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Bool,.argument_count=3,.arguments={{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output}},.publication=NativeScriptOutputs_SuccessOnly},
    [NativeScriptBinding_TargetsReadLockableItemAhead]={.name="read_lockable_item_ahead",.parent=NativeScriptBinding_Targets,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Bool,.argument_count=3,.arguments={{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output},{.type=NativeScriptType_F32,.mode=NativeScriptArgument_Output}},.publication=NativeScriptOutputs_SuccessOnly},
    [NativeScriptBinding_TargetsOverlapsHitbox]={.name="overlaps_hitbox",.parent=NativeScriptBinding_Targets,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Bool,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
    [NativeScriptBinding_TargetsNearestAvailableFighter]={.name="nearest_available_fighter",.parent=NativeScriptBinding_Targets,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_I32,.argument_count=0},
    [NativeScriptBinding_TargetsAttackOverlapsCapsule]={
        .name="attack_overlaps_capsule",.parent=NativeScriptBinding_Targets,
        .flags=KINETIC_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,
        .contexts=F,.result_type=NativeScriptType_Bool,.argument_count=7,
        .arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),
                    ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),
                    ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_TargetsArticleAttackOverlapsCapsule]={
        .name="article_attack_overlaps_capsule",.parent=NativeScriptBinding_Targets,
        .flags=KINETIC_CALL|NativeScriptFlag_SourceOnly,.kind=NativeScriptBinding_Intrinsic,
        .contexts=F,.result_type=NativeScriptType_Bool,.argument_count=8,
        .arguments={ARG(NativeScriptType_I32),
                    ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),
                    ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),
                    ARG(NativeScriptType_F32)}},
    NS(NativeScriptBinding_ItemSlideVelocity,"slide_velocity",NATIVE_SCRIPT_ITEM_NAMESPACE,A),
    PROP(NativeScriptBinding_ItemSlideVelocityX,"x",NativeScriptBinding_ItemSlideVelocity,A,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_ItemSlideVelocityY,"y",NativeScriptBinding_ItemSlideVelocity,A,NativeScriptType_F32,W),
    PROP(NativeScriptBinding_ItemSlideVelocityZ,"z",NativeScriptBinding_ItemSlideVelocity,A,NativeScriptType_F32,W),
    [NativeScriptBinding_CollisionGroundStandardBox]={.name="ground_standard_box",.parent=NativeScriptBinding_Collision,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.argument_count=6,.arguments={ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32),ARG(NativeScriptType_F32)}},
    [NativeScriptBinding_ControllerRumble]={.name="rumble",.parent=NativeScriptBinding_Controller,.kind=NativeScriptBinding_Intrinsic,.contexts=F,.result_type=NativeScriptType_Void,.flags=KINETIC_CALL|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource,.argument_count=2,.arguments={{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true},{.type=NativeScriptType_Invalid,.mode=NativeScriptArgument_Input,.numeric_any=true}}},
};

const NativeScriptBindingDescriptor* nativeScriptBindings_Descriptor(uint32_t semantic)
{
    if((semantic&NATIVE_SCRIPT_ITEM_RESOURCES_DOMAIN_MASK)==NATIVE_SCRIPT_ITEM_RESOURCES_DOMAIN)
        return nativeScriptBindingItemResources_Descriptor(semantic);
    semantic=nativeScriptBindingArticleReference_Base(semantic);
    semantic=nativeScriptBindingSignal_Base(semantic);
    const NativeScriptBindingDescriptor* spawn=nativeScriptBindingSpawn_Descriptor(semantic);
    if(spawn)return spawn;
    semantic=nativeScriptBindingSpawn_Base(semantic);
    return semantic > NativeScriptBinding_Invalid && semantic < NativeScriptBinding_Count ? &catalog[semantic] : NULL;
}

static bool error(NativeScriptDiagnostic* diagnostic, NativeScriptSourceSpan source, const char* message)
{
    if (diagnostic) { diagnostic->source=source;snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",message); }
    return false;
}

static bool valid_context(uint32_t mask)
{
    return mask==F || mask==A || mask==NativeScriptBindings_Effect;
}

static bool accepts_argument(const NativeScriptBindingArgument* spec,
                             const NativeScriptIRNode* value)
{
    if(spec->mode==NativeScriptArgument_Literal && spec->type==NativeScriptType_Text)
        return value->type==NativeScriptType_Text && value->kind==NativeScriptIR_String;
    if (spec->mode==NativeScriptArgument_CodeRef)
        return value->type==NativeScriptType_Null ||
            (value->type==NativeScriptType_Callable&&value->kind==NativeScriptIR_Name&&
             (value->semantic&UINT32_C(0xc0000000))==NATIVE_SCRIPT_FUNCTION_SEMANTIC);
    if(spec->object_or_integral)return value->type==NativeScriptType_Object || nativeScriptExpression_IsIntegral(value->type);
    if (spec->numeric_any) return nativeScriptExpression_IsNumeric(value->type);
    if (spec->mode==NativeScriptArgument_Output || spec->mode==NativeScriptArgument_InputOutput)
        return value->type==spec->type && (value->flags&NativeScriptFlag_Writable) && (value->flags&NativeScriptFlag_Addressable);
    return nativeScriptExpression_Assignable(spec->type,value->type);
}

static bool sameExpression(const NativeScriptIRArena* arena,uint32_t left,uint32_t right,unsigned depth)
{
    if(left==right)return true;
    if(depth>=NATIVE_SCRIPT_EXPRESSION_DEPTH || left>=arena->node_count || right>=arena->node_count)return false;
    const NativeScriptIRNode* a=&arena->nodes[left];const NativeScriptIRNode* b=&arena->nodes[right];
    if(a->kind!=b->kind || a->type!=b->type || a->semantic!=b->semantic ||
        a->operation!=b->operation || a->bits!=b->bits || a->argument_count!=b->argument_count)return false;
    if((a->a==UINT32_MAX)!=(b->a==UINT32_MAX) || (a->b==UINT32_MAX)!=(b->b==UINT32_MAX))return false;
    if(a->a!=UINT32_MAX && !sameExpression(arena,a->a,b->a,depth+1))return false;
    if(a->b!=UINT32_MAX && !sameExpression(arena,a->b,b->b,depth+1))return false;
    for(unsigned i=0;i<a->argument_count;++i)
        if(!sameExpression(arena,arena->arguments[a->argument_first+i],arena->arguments[b->argument_first+i],depth+1))return false;
    return true;
}

bool nativeScriptBindings_ResolveCall(void* context,const NativeScriptIRArena* arena,
    const NativeScriptIRNode* node,uint32_t semantic,NativeScriptIRResolution* result,NativeScriptDiagnostic* diagnostic)
{
    const NativeScriptBindingResolverContext* binding=context;
    if(!binding || !arena || !node || !result || node->kind!=NativeScriptIR_Call || !valid_context(binding->context_mask))
        return error(diagnostic,node?node->source:(NativeScriptSourceSpan){0},"Invalid typed native call context");
    if (node->a>=arena->node_count || node->argument_first>arena->argument_count ||
        node->argument_count>arena->argument_count-node->argument_first)
        return error(diagnostic,node->source,"Native call IR range is invalid.");
    if (semantic==NativeScriptBinding_PhysicsFall&&node->argument_count==2)semantic=NativeScriptBinding_PhysicsFallWithTerminal;
    if(semantic==NativeScriptBinding_StateEnter && node->argument_count==2)semantic=NativeScriptBinding_StateEnterFrame;
    else if(semantic==NativeScriptBinding_StateEnter && node->argument_count==3)semantic=NativeScriptBinding_StateEnterSpeed;
    else if(semantic==NativeScriptBinding_BonesRootRotateX && node->argument_count==2)semantic=NativeScriptBinding_BonesRootRotateXWithJoint;
    else if(semantic==NativeScriptBinding_BonesRootRotateY && node->argument_count==2)semantic=NativeScriptBinding_BonesRootRotateYWithJoint;
    else if(semantic==NativeScriptBinding_BonesRootRotateZ && node->argument_count==2)semantic=NativeScriptBinding_BonesRootRotateZWithJoint;
    else if(semantic==NativeScriptBinding_BonesWorldPosition && node->argument_count==7)semantic=NativeScriptBinding_BonesWorldPositionWithOffset;
    if(semantic==NativeScriptBinding_ItemAttachToOwner&&node->argument_count==2)semantic=NativeScriptBinding_ItemAttachToOwnerAux;
    if(semantic==NativeScriptBinding_ItemStageCollision&&node->argument_count==1)semantic=NativeScriptBinding_ItemStageCollisionMode;
    if(semantic==NativeScriptBinding_CPUQueue&&node->argument_count==2)semantic=NativeScriptBinding_CPUQueueArg;
    else if(semantic==NativeScriptBinding_CPUQueue&&node->argument_count==3)semantic=NativeScriptBinding_CPUQueuePair;
    if(semantic==NativeScriptBinding_ItemSpawnEffectOnBone && node->argument_count==3)semantic=NativeScriptBinding_ItemSpawnEffectOnBoneMode;
    if(semantic==NativeScriptBinding_ItemSpawnAsyncJointEffect && node->argument_count==4)semantic=NativeScriptBinding_ItemSpawnAsyncJointEffectWithScalar;
    if(semantic==NativeScriptBinding_ItemGravity && node->argument_count==1)semantic=NativeScriptBinding_ItemGravityWithSpin;
    else if(semantic==NativeScriptBinding_ItemMirrorVelocity && node->argument_count==3)semantic=NativeScriptBinding_ItemMirrorVelocityWithNormal;
    if(semantic==NativeScriptBinding_ItemBounceToGround && node->argument_count==1)semantic=NativeScriptBinding_ItemBounceToGroundWithContactReset;
    if(semantic==NativeScriptBinding_ItemRespondToStage && node->argument_count==1)semantic=NativeScriptBinding_ItemRespondToStageWithImpactResponse;
    if(semantic==NativeScriptBinding_ItemReleaseAttachment && node->argument_count==4)semantic=NativeScriptBinding_ItemReleaseAttachmentPreserveHeld;
    if(semantic==NativeScriptBinding_StageReadLineIslandBounds && node->argument_count==7)semantic=NativeScriptBinding_StageReadLineIslandBounds3D;
    if(semantic==NativeScriptBinding_CollisionAirAerialLandingCallback && node->argument_count==2)semantic=NativeScriptBinding_CollisionAirAerialLandingWithLedges;
    if(semantic==NativeScriptBinding_PhysicsAirDrift && node->argument_count==1)semantic=NativeScriptBinding_PhysicsAirDriftScaled;
    const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(semantic);
    if (!descriptor || descriptor->kind!=NativeScriptBinding_Intrinsic || !(descriptor->contexts&binding->context_mask))
        return error(diagnostic,node->source,"Unsupported native call or execution context.");
    if(semantic==NativeScriptBinding_SpawnAuthoredProjectile || semantic==NativeScriptBinding_ArticlesLaunchAtPosition || semantic==NativeScriptBinding_ArticlesLaunchProjectile || semantic==NativeScriptBinding_SpawnBoneAttached || semantic==NativeScriptBinding_SpawnAttachedPresentation ||
       semantic==NativeScriptBinding_ArticlesCreateAtPosition || semantic==NativeScriptBinding_ArticlesCreateWithVelocity || semantic==NativeScriptBinding_ArticlesCreateUnownedAtPosition ||
       semantic==NativeScriptBinding_ItemCreateAtPosition || semantic==NativeScriptBinding_ItemCreateChild || semantic==NativeScriptBinding_ItemCreateLinked || semantic==NativeScriptBinding_ArticlesCreateHeldAtCenter)
        return error(diagnostic,node->source,"Authored factory requires an exact load-resolved source owner");
    if(semantic==NativeScriptBinding_ArticlesSignal || semantic==NativeScriptBinding_ItemSignal)
        return error(diagnostic,node->source,"Signal requires an exact load-resolved callback identity");
    if(semantic==NativeScriptBinding_ItemAddModelAnimation||semantic==NativeScriptBinding_ItemRequestModelAnimation||semantic==NativeScriptBinding_ItemRemoveModelAnimation)
        return error(diagnostic,node->source,"Animation call requires its exact load-resolved model selector");
    if(semantic==NativeScriptBinding_ArticlesRef||semantic==NativeScriptBinding_ArticlesReadPrimaryTranslation||semantic==NativeScriptBinding_ArticlesReadBonePosition)
        return error(diagnostic,node->source,"Article reference requires an exact load-resolved declaration identity");
    bool recipient_call=semantic>=NativeScriptBinding_TargetsCall&&semantic<=NativeScriptBinding_TargetsCallFloat;
    if (recipient_call?(node->argument_count<2||node->argument_count>NATIVE_SCRIPT_EXPRESSION_ARGUMENTS):
        node->argument_count!=descriptor->argument_count)
        return error(diagnostic,node->source,"Native call argument count does not match its signature.");
    for (uint32_t i=0;i<node->argument_count;++i) {
        uint32_t argument=arena->arguments[node->argument_first+i];
        if(recipient_call && i>=2) {
            if(argument>=arena->node_count || !nativeScriptExpression_IsNumeric(arena->nodes[argument].type))
                return error(diagnostic,node->source,"Recipient helper arguments must be numeric values");
            continue;
        }
        if (argument>=arena->node_count || !accepts_argument(&descriptor->arguments[i],&arena->nodes[argument]))
            return error(diagnostic,node->source,"Native call argument type or output mode does not match its signature.");
    }
    if(semantic==NativeScriptBinding_ItemCloneModel||semantic==NativeScriptBinding_ItemLoadModel||semantic==NativeScriptBinding_ItemDestroyModel||semantic==NativeScriptBinding_ItemSetModelDrawEnabled||semantic==NativeScriptBinding_ItemSetJointTransform){
        const NativeScriptIRNode* slot=&arena->nodes[arena->arguments[node->argument_first]];
        unsigned minimum=semantic==NativeScriptBinding_ItemSetJointTransform?0:1;
        if(slot->kind!=NativeScriptIR_Integer||slot->type!=NativeScriptType_I32||slot->bits<minimum||slot->bits>3)
            return error(diagnostic,slot->source,"Model slot requires its bounded literal index");
    }
    if(semantic==NativeScriptBinding_ItemRespondStageMask&&
       !nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first]].type))
        return error(diagnostic,node->source,"Item stage response mask requires an integral value");
    if(semantic==NativeScriptBinding_GuardCreate&&
       !nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first]].type))
        return error(diagnostic,node->source,"Guard joint index requires an integral value");
    if(semantic==NativeScriptBinding_CollisionAirLandingCallback&&
       arena->nodes[arena->arguments[node->argument_first]].type==NativeScriptType_Null)
        return error(diagnostic,node->source,"Air landing requires an owned helper, not None");
    if(descriptor==&catalog[NativeScriptBinding_ItemCreateLinked])for(unsigned i=1;i<3;++i)
        if(!nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first+i]].type))
            return error(diagnostic,node->source,"Linked child slots require integral values");
    if(descriptor==&catalog[NativeScriptBinding_ItemCreateChild])for(unsigned i=5;i<9;++i)if(i!=7&&!nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first+i]].type))
        return error(diagnostic,node->source,"Child damage-owner/collision/backlink requires an integral value");
    if((semantic>=NativeScriptBinding_HitboxEnabled&&semantic<=NativeScriptBinding_HitboxSetRadius)||semantic==NativeScriptBinding_ItemDisableHitbox||semantic==NativeScriptBinding_HitboxState||semantic==NativeScriptBinding_HitboxDamage||
       semantic==NativeScriptBinding_HitboxGroup||semantic==NativeScriptBinding_HitboxSetGroup||semantic==NativeScriptBinding_HitboxCurrent||semantic==NativeScriptBinding_HitboxPrevious||semantic==NativeScriptBinding_HitboxSetWorldPosition||
       semantic==NativeScriptBinding_HitboxSetEnabled||semantic==NativeScriptBinding_HitboxClearVictims||semantic==NativeScriptBinding_HitboxCreate||
       semantic==NativeScriptBinding_HitboxLaunchTarget||semantic==NativeScriptBinding_HitboxLaunchFollow)
        if(!nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first]].type))
            return error(diagnostic,node->source,"Hitbox index requires an integral value");
    if(semantic==NativeScriptBinding_HitboxSetGroup&&!nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first+1]].type))
        return error(diagnostic,node->source,"Hitbox group requires an integral value");
    if(semantic==NativeScriptBinding_ControllerRumble||semantic==NativeScriptBinding_ItemRumble||semantic==NativeScriptBinding_CameraQuake)
        for(unsigned i=0;i<((semantic==NativeScriptBinding_ItemRumble||semantic==NativeScriptBinding_ControllerRumble)?2u:1u);++i)
            if(!nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first+i]].type))
                return error(diagnostic,node->source,"Rumble/quake selector requires an integral value");
    if(semantic>=NativeScriptBinding_CPUQueue&&semantic<=NativeScriptBinding_CPUQueuePair) {
        const NativeScriptIRNode* command=&arena->nodes[arena->arguments[node->argument_first]];
        if(command->kind!=NativeScriptIR_Integer||command->type!=NativeScriptType_I32||command->bits>UINT32_MAX||
           nativeScriptCPUControl_CommandOperands((uint32_t)command->bits)!=(int)node->argument_count-1)
            return error(diagnostic,node->source,"CPU queue needs a recognized literal command and its exact operand arity");
    }
    if((semantic>=NativeScriptBinding_TargetsReadPosition&&semantic<=NativeScriptBinding_TargetsIsFighter)||(semantic>=NativeScriptBinding_TargetsDamage&&semantic<=NativeScriptBinding_TargetsFloorLine)||(semantic>=NativeScriptBinding_TargetsModelScale&&semantic<=NativeScriptBinding_TargetsReadCameraTargetPosition)||semantic==NativeScriptBinding_TargetsReadPositionDelta||semantic==NativeScriptBinding_TargetsSmashChargeActive||semantic==NativeScriptBinding_TargetsMotion||
       (semantic>=NativeScriptBinding_TargetsCall&&semantic<=NativeScriptBinding_TargetsCallFloat)) {
        if(!nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first]].type))
            return error(diagnostic,node->source,"Target handle requires an integral value");
        if(semantic==NativeScriptBinding_TargetsReadPosition||semantic==NativeScriptBinding_TargetsReadPositionDelta||semantic==NativeScriptBinding_TargetsShield||semantic==NativeScriptBinding_TargetsReadCameraTargetPosition)for(unsigned a=1;a<node->argument_count;++a)for(unsigned b=1;b<a;++b)
            if(sameExpression(arena,arena->arguments[node->argument_first+a],arena->arguments[node->argument_first+b],0))
                return error(diagnostic,node->source,"Target position needs distinct output destinations");
    }
    if(semantic==NativeScriptBinding_TargetsOverlapsHitbox)
        for(unsigned a=0;a<2;++a)if(!nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first+a]].type))
            return error(diagnostic,node->source,"Hitbox overlap handle and slot must be integral");
    if(semantic==NativeScriptBinding_TargetsReadOpponentAhead || semantic==NativeScriptBinding_TargetsReadLockableItemAhead)
        for(unsigned a=0;a<3;++a)for(unsigned b=0;b<a;++b)
            if(sameExpression(arena,arena->arguments[node->argument_first+a],arena->arguments[node->argument_first+b],0))
                return error(diagnostic,node->source,"Ahead query needs distinct output destinations");
    if(semantic==NativeScriptBinding_CPURecordInput&&arena->nodes[arena->arguments[node->argument_first]].type!=NativeScriptType_I32)
        return error(diagnostic,node->source,"CPU history record requires an exact Int creation ID");
    if(semantic==NativeScriptBinding_CPUReadHistoryPose)for(unsigned a=0;a<4;++a)for(unsigned b=0;b<a;++b)
        if(sameExpression(arena,arena->arguments[node->argument_first+a],arena->arguments[node->argument_first+b],0))
            return error(diagnostic,node->source,"CPU history pose needs distinct output destinations");
    if(semantic==NativeScriptBinding_ItemBounceToGroundWithContactReset&&arena->nodes[arena->arguments[node->argument_first]].type!=NativeScriptType_Bool)
        return error(diagnostic,node->source,"Ground bounce contact reset requires Bool");
    if(semantic==NativeScriptBinding_ItemRespondToStageWithImpactResponse&&arena->nodes[arena->arguments[node->argument_first]].type!=NativeScriptType_Bool)
        return error(diagnostic,node->source,"Stage impact response requires Bool");
    if(semantic==NativeScriptBinding_ItemReleaseAttachmentPreserveHeld&&arena->nodes[arena->arguments[node->argument_first+3]].type!=NativeScriptType_Bool)
        return error(diagnostic,node->source,"Attachment held-item preservation requires Bool");
    if((semantic==NativeScriptBinding_ItemSetAnimationDescriptor||semantic==NativeScriptBinding_ItemAddAnimationDescriptor)&&arena->nodes[arena->arguments[node->argument_first]].type!=NativeScriptType_I32)
        return error(diagnostic,node->source,"Item animation descriptor requires exact Int; use explicit int() conversion");
    if((semantic==NativeScriptBinding_ItemAttachToOwner||semantic==NativeScriptBinding_ItemAttachToOwnerAux||semantic==NativeScriptBinding_ArticlesBindAuxiliary||
        (semantic>=NativeScriptBinding_ItemsBindHeld&&semantic<=NativeScriptBinding_ArticlesBindHeld)||
        (semantic>=NativeScriptBinding_ItemsSetVisible&&semantic<=NativeScriptBinding_ItemsNotifyDrop))&&
       !nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first]].type))
        return error(diagnostic,node->source,"Attachment part/auxiliary handle requires an integral value");
    if(semantic==NativeScriptBinding_ColorRGBA || semantic==NativeScriptBinding_VisualsSetDiffuseColor ||
       semantic==NativeScriptBinding_VisualsInstallOutlineVisibility || semantic==NativeScriptBinding_ItemSetDiffuseColor) {
        for(unsigned argument=0;argument<node->argument_count;++argument)
            if(!nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first+argument]].type))
                return error(diagnostic,node->source,"Color components, packed colors and visibility slots require integral values");
    }
    if((descriptor==&catalog[NativeScriptBinding_ArticlesRef]||descriptor==&catalog[NativeScriptBinding_ArticlesReadPrimaryTranslation]||descriptor==&catalog[NativeScriptBinding_ArticlesReadBonePosition])&&
       !nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first]].type))
        return error(diagnostic,node->source,"Article reference handle requires an integral value");
    if(semantic==NativeScriptBinding_ArticlesLinkOwned||semantic==NativeScriptBinding_ItemLinkId||
       semantic==NativeScriptBinding_ItemLinkReciprocal||semantic==NativeScriptBinding_ItemUnlink||semantic==NativeScriptBinding_ItemCopyLinkCollision) {
        for(unsigned i=0;i<node->argument_count;++i)
            if(!nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first+i]].type))
                return error(diagnostic,node->source,"Article creation IDs and link slots require integral values");
    }
    if(descriptor==&catalog[NativeScriptBinding_ArticlesSignal] &&
       !nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first]].type))
        return error(diagnostic,node->source,"Article signal handle requires an integral value");
    if(descriptor==&catalog[NativeScriptBinding_ArticlesCreateWithVelocity] &&
       !nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first+9]].type))
        return error(diagnostic,node->source,"Article collision group requires an integral value");
    if(semantic==NativeScriptBinding_StateTaunt || semantic==NativeScriptBinding_ItemsCreateEngine) {
        unsigned first=semantic==NativeScriptBinding_StateTaunt?0:8;
        unsigned end=semantic==NativeScriptBinding_StateTaunt?2:9;
        for(unsigned i=first;i<end;++i)
            if(!nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first+i]].type))
                return error(diagnostic,node->source,"Motion and collision-group arguments require integral values");
    }
    if(nativeScriptBindingSpawn_Base(semantic)==NativeScriptBinding_ArticlesCreateHeldAtCenter) {
        for(unsigned i=0;i<2;++i)if(!nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first+(i?4:0)]].type))
            return error(diagnostic,node->source,"Held Article owner/raw part requires an integral value");
    }
    if(semantic==NativeScriptBinding_TargetsReadBonePosition) {
        for(unsigned i=0;i<2;++i)if(!nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first+i]].type))
            return error(diagnostic,node->source,"Target handle/raw part requires an integral value");
    }
    if(semantic==NativeScriptBinding_AnimationClipLength &&
       !nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first]].type))
        return error(diagnostic,node->source,"Animation clip index requires an integral value");
    if((semantic==NativeScriptBinding_BonesIndex || semantic==NativeScriptBinding_BonesWorldPosition || semantic==NativeScriptBinding_BonesWorldPositionWithOffset || semantic==NativeScriptBinding_FamilyMemberId) &&
       !nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first]].type))
        return error(diagnostic,node->source,"Bone/member index requires an integral value");
    if(semantic==NativeScriptBinding_HitlagLink && !nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first]].type))
        return error(diagnostic,node->source,"Hitlag handle requires an integral value");
    if(semantic==NativeScriptBinding_StageSweepBox && !nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first+12]].type))
        return error(diagnostic,node->source,"Stage sweep category requires an integral value");
    if(semantic==NativeScriptBinding_StageEvent || semantic==NativeScriptBinding_StatsActionCount) {
        unsigned selectors=semantic==NativeScriptBinding_StageEvent?1u:2u;
        for(unsigned i=0;i<selectors;++i)
            if(!nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first+i]].type))
                return error(diagnostic,node->source,"Stage channel and statistics selectors require integral values");
    }
    if((semantic==NativeScriptBinding_EffectsSpawnOnPart || semantic==NativeScriptBinding_EffectsSpawnBetween || semantic==NativeScriptBinding_EffectsSpawnAsyncPositionOnPart || semantic==NativeScriptBinding_EffectsSpawnAsyncPositionScalarOnPart || semantic==NativeScriptBinding_EffectsSpawnAsyncAttached || semantic==NativeScriptBinding_EffectsSpawnAsyncWithScalar || semantic==NativeScriptBinding_EffectsSpawnAtPosition || semantic==NativeScriptBinding_EffectsSpawnWithScalarAtPosition||semantic==NativeScriptBinding_EffectsSpawnWithTwoScalarsAtPosition || semantic==NativeScriptBinding_ItemSpawnRootEffect || semantic==NativeScriptBinding_ItemSpawnRootEffectWithOffset || semantic==NativeScriptBinding_ItemSpawnEffectAtPosition || semantic==NativeScriptBinding_ItemSpawnWithScalarAtPosition ||
        semantic==NativeScriptBinding_CallbackMatches) &&
       !nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first]].type))
        return error(diagnostic,node->source,"Effect ID/callback slot requires an integral value");
    if(semantic==NativeScriptBinding_VisualsSetGroup || semantic==NativeScriptBinding_VisualsSetCurrentGroup ||
       semantic==NativeScriptBinding_VisualsCurrentGroup || semantic==NativeScriptBinding_VisualsPreviousGroup || semantic==NativeScriptBinding_ColorStart ||
       semantic==NativeScriptBinding_VisualsSetGroupMaterialFrame || semantic==NativeScriptBinding_VisualsInitializeGroupMaterialAnimation) {
        unsigned integral_count=(semantic==NativeScriptBinding_VisualsSetGroupMaterialFrame||semantic==NativeScriptBinding_VisualsInitializeGroupMaterialAnimation)?3:
            (semantic==NativeScriptBinding_VisualsSetGroup||semantic==NativeScriptBinding_VisualsSetCurrentGroup)?2:1;
        for(unsigned i=0;i<integral_count;++i)
            if(!nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first+i]].type))
                return error(diagnostic,node->source,"Visibility group/color selector requires an integral value");
    }
    if(semantic>=NativeScriptBinding_PoseLoadAnimation&&semantic<=NativeScriptBinding_PoseCopyToLive) {
        int integral=(semantic==NativeScriptBinding_PoseLoadAnimation||semantic==NativeScriptBinding_PoseCopyCostume)?1:
            (semantic==NativeScriptBinding_PoseBlendToLive||semantic==NativeScriptBinding_PoseCopyToLive)?0:-1;
        if(integral>=0&&!nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first+(uint32_t)integral]].type))
            return error(diagnostic,node->source,"Pose clip/start requires an integral value");
    }
    if(((semantic>=NativeScriptBinding_StateEnter && semantic<=NativeScriptBinding_StateSwitchToGround)||
        semantic==NativeScriptBinding_StateEnterAtSpeed) && node->argument_count) {
        if(!nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first]].type))
            return error(diagnostic,node->source,"Motion target requires an integral value");
    }
    if(semantic==NativeScriptBinding_CPURunScript && !nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first]].type))
        return error(diagnostic,node->source,"CPU script index requires an integral value");
    if (semantic==NativeScriptBinding_Command) {
        const NativeScriptIRNode* argument=&arena->nodes[arena->arguments[node->argument_first]];
        if (!nativeScriptExpression_IsIntegral(argument->type) ||
            (argument->kind==NativeScriptIR_Integer&&argument->bits>=4))
            return error(diagnostic,argument->source,"Command selector must be an integral index in 0..3.");
    }
    if(semantic==NativeScriptBinding_BonesWorldPosition || semantic==NativeScriptBinding_BonesWorldPositionWithOffset||semantic==NativeScriptBinding_ItemJointWorldPosition||semantic==NativeScriptBinding_TargetsReadBonePosition||semantic==NativeScriptBinding_ItemReadJointMatrixRotation||semantic==NativeScriptBinding_ItemReadAttachmentMatrixRotation) {
        unsigned first=semantic==NativeScriptBinding_ItemReadAttachmentMatrixRotation?0:
            (semantic==NativeScriptBinding_ItemJointWorldPosition||semantic==NativeScriptBinding_TargetsReadBonePosition||semantic==NativeScriptBinding_ItemReadJointMatrixRotation)?2:1;
        for(unsigned a=first;a<first+3;++a)for(unsigned b=first;b<a;++b)
            if(sameExpression(arena,arena->arguments[node->argument_first+a],arena->arguments[node->argument_first+b],0))
                return error(diagnostic,node->source,"Vector reads need distinct output destinations");
    }
    if(semantic==NativeScriptBinding_ItemSweepSegment) {
        for(unsigned a=3;a<9;++a)for(unsigned b=3;b<a;++b)
            if(sameExpression(arena,arena->arguments[node->argument_first+a],arena->arguments[node->argument_first+b],0))
                return error(diagnostic,node->source,"Item segment sweep needs distinct output destinations");
    }
    if(semantic==NativeScriptBinding_AudioPlayGlobal) {
        for(unsigned i=0;i<3;++i)
            if(arena->nodes[arena->arguments[node->argument_first+i]].type!=NativeScriptType_I32)
                return error(diagnostic,node->source,"Global audio requires Int expressions; use explicit int() conversion");
    }
    if(semantic==NativeScriptBinding_ItemReadJointMatrixRotation &&
       arena->nodes[arena->arguments[node->argument_first]].type!=NativeScriptType_I32)
        return error(diagnostic,node->source,"Joint matrix rotation requires an exact Int model selector");
    if(semantic==NativeScriptBinding_CollisionCopyFrom &&
       arena->nodes[arena->arguments[node->argument_first]].type!=NativeScriptType_I32)
        return error(diagnostic,node->source,"Collision copy requires an Int creation ID; use explicit int() conversion");
    if((semantic==NativeScriptBinding_ItemSetCollisionFacing||semantic==NativeScriptBinding_CaptureMask||semantic==NativeScriptBinding_CaptureArm) &&
       arena->nodes[arena->arguments[node->argument_first]].type!=NativeScriptType_I32)
        return error(diagnostic,node->source,"Collision facing and capture masks require exact Int arguments; use explicit int() conversion");
    if(semantic==NativeScriptBinding_StageLineExists||semantic==NativeScriptBinding_StageLineNormal||
       semantic==NativeScriptBinding_ItemBindStageLine||semantic==NativeScriptBinding_ItemReadStageLineSpeed) {
        if(!nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first]].type))
            return error(diagnostic,node->source,"Collision line requires an integral value");
        if(semantic==NativeScriptBinding_StageLineNormal)for(unsigned a=1;a<4;++a)for(unsigned b=1;b<a;++b)
            if(sameExpression(arena,arena->arguments[node->argument_first+a],arena->arguments[node->argument_first+b],0))
                return error(diagnostic,node->source,"Stage line normal needs distinct output destinations");
    }
    if(semantic>=NativeScriptBinding_StageReadLineIslandBounds&&semantic<=NativeScriptBinding_StageCpuIslandBlocked) {
        unsigned lines=semantic==NativeScriptBinding_StageSameLineIsland?2:1;
        for(unsigned i=0;i<lines;++i)if(!nativeScriptExpression_IsIntegral(arena->nodes[arena->arguments[node->argument_first+i]].type))
            return error(diagnostic,node->source,"Stage island line requires an integral value");
    }
    if(semantic==NativeScriptBinding_StageReadFloor||semantic==NativeScriptBinding_StageReadLineIslandBounds||semantic==NativeScriptBinding_StageReadLineIslandBounds3D||semantic==NativeScriptBinding_StageCompareFloorIslands||semantic==NativeScriptBinding_StageFollowFloor||semantic==NativeScriptBinding_StageIslandBounds) {
        unsigned first=semantic==NativeScriptBinding_StageReadFloor?4:semantic==NativeScriptBinding_StageCompareFloorIslands?8:semantic==NativeScriptBinding_StageFollowFloor?6:semantic==NativeScriptBinding_StageIslandBounds?4:1;
        for(unsigned a=first;a<node->argument_count;++a)for(unsigned b=first;b<a;++b)
            if(sameExpression(arena,arena->arguments[node->argument_first+a],arena->arguments[node->argument_first+b],0))
                return error(diagnostic,node->source,"Stage query needs distinct output destinations");
    }
    if (semantic==NativeScriptBinding_RadiansFloat &&
        arena->nodes[arena->arguments[node->argument_first]].type==NativeScriptType_F64) {
        semantic=NativeScriptBinding_RadiansDouble;descriptor=&catalog[semantic];
    }
    *result=(NativeScriptIRResolution){descriptor->result_type,semantic,
        NativeScriptFlag_Readable | (descriptor->flags&(NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_Pure|NativeScriptFlag_Addressable|NativeScriptFlag_Writable)),0};
    return true;
}

bool nativeScriptBindings_Resolve(void* context, const NativeScriptIRArena* arena,
    const NativeScriptIRNode* node, NativeScriptIRResolution* result,
    NativeScriptDiagnostic* diagnostic)
{
    const NativeScriptBindingResolverContext* binding=context;
    if (!binding || !arena || !node || !result || !valid_context(binding->context_mask))
        return error(diagnostic,node?node->source:(NativeScriptSourceSpan){0},"Invalid native binding resolution context.");
    int constant=nativeScriptConstants_TryResolve(NULL,arena,node,result,diagnostic);
    if(constant)return constant>0;
    if (node->kind==NativeScriptIR_Call) {
        if(node->a>=arena->node_count)return error(diagnostic,node->source,"Native callee is out of range");
        return nativeScriptBindings_ResolveCall(context,arena,node,arena->nodes[node->a].semantic,result,diagnostic);
    }
    if (node->kind!=NativeScriptIR_Name && node->kind!=NativeScriptIR_Member)
        return error(diagnostic,node->source,"Unsupported native selector or index.");
    if (node->text_offset>arena->source_bytes || node->text_length>arena->source_bytes-node->text_offset)
        return error(diagnostic,node->source,"Native binding name range is invalid.");
    uint32_t parent=NativeScriptBinding_Invalid;
    if (node->kind==NativeScriptIR_Member) {
        if (node->a>=arena->node_count)return error(diagnostic,node->source,"Native member receiver is invalid.");
        parent=arena->nodes[node->a].semantic;
        if((parent==NativeScriptBinding_HitboxCurrent||parent==NativeScriptBinding_HitboxPrevious)&&
           (arena->nodes[node->a].kind!=NativeScriptIR_Call||arena->nodes[node->a].type!=NativeScriptType_Namespace))
            return error(diagnostic,node->source,"Hitbox vector requires its integral index selector call");
    }
    for (uint32_t i=1;i<NativeScriptBinding_Count;++i) {
        const NativeScriptBindingDescriptor* descriptor=&catalog[i];
        size_t length=strlen(descriptor->name);
        if (descriptor->parent!=parent || !(descriptor->contexts&binding->context_mask) ||
            length!=node->text_length || memcmp(descriptor->name,arena->source+node->text_offset,length)) continue;
        *result=(NativeScriptIRResolution){descriptor->kind==NativeScriptBinding_Intrinsic?NativeScriptType_Callable:descriptor->result_type,i==NativeScriptBinding_State?NATIVE_SCRIPT_STATE_NAMESPACE:i==NativeScriptBinding_Item?NATIVE_SCRIPT_ITEM_NAMESPACE:i,descriptor->flags,0};
        return true;
    }
    return error(diagnostic,node->source,"Native binding name or context is unsupported.");
}

bool nativeScriptBindings_DescribeCall(void* context,uint32_t semantic,
    NativeScriptCallLayout* result,NativeScriptDiagnostic* diagnostic)
{
    unsigned recipient_count=0;
    if((semantic&NATIVE_SCRIPT_EFFECT_DOMAIN_MASK)==NATIVE_SCRIPT_EFFECT_DOMAIN)return nativeScriptBindingEffect_DescribeCall(context,semantic,result,diagnostic);
    if((semantic&NATIVE_SCRIPT_TARGET_EXPORT_DOMAIN_MASK)==NATIVE_SCRIPT_TARGET_EXPORT_DOMAIN){
        unsigned operation=(semantic&NATIVE_SCRIPT_TARGET_EXPORT_OPERATION_MASK)>>20;
        recipient_count=(semantic&NATIVE_SCRIPT_TARGET_EXPORT_ARGUMENT_MASK)>>NATIVE_SCRIPT_TARGET_EXPORT_ARGUMENT_SHIFT;
        if(operation<1||operation>4)return error(diagnostic,(NativeScriptSourceSpan){0},"Recipient export operation is invalid");
        if(recipient_count<2 || recipient_count>NATIVE_SCRIPT_EXPRESSION_ARGUMENTS)
            return error(diagnostic,(NativeScriptSourceSpan){0},"Recipient export prepared argument count is invalid");
        semantic=NativeScriptBinding_TargetsCall+operation-1;
    }
    if((semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK)==NATIVE_SCRIPT_ITEM_MODEL_WORLD_POSITION)semantic=NativeScriptBinding_ItemJointWorldPosition;
    else if((semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK)==NATIVE_SCRIPT_ITEM_MODEL_TRANSFORM)semantic=NativeScriptBinding_ItemSetJointTransform;
    else if((semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK)==NATIVE_SCRIPT_ITEM_MODEL_MATRIX_ROTATION)semantic=NativeScriptBinding_ItemReadJointMatrixRotation;
    const NativeScriptBindingResolverContext* binding=context;
    const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(semantic);
    if (!binding || !result || !valid_context(binding->context_mask) || !descriptor ||
        descriptor->kind!=NativeScriptBinding_Intrinsic || !(descriptor->contexts&binding->context_mask))
        return error(diagnostic,(NativeScriptSourceSpan){0},"Native call has no supported typed descriptor.");
    NativeScriptCallLayout layout={0};layout.argument_count=recipient_count?recipient_count:descriptor->argument_count;
    layout.publication=descriptor->publication;
    for (uint32_t i=0;i<descriptor->argument_count;++i) {
        const NativeScriptBindingArgument* argument=&descriptor->arguments[i];
        layout.argument_types[i]=argument->numeric_any||argument->object_or_integral||argument->mode==NativeScriptArgument_CodeRef?NativeScriptType_Invalid:argument->type;
        if (argument->mode==NativeScriptArgument_Output || argument->mode==NativeScriptArgument_InputOutput)layout.output_mask|=UINT32_C(1)<<i;
        if (argument->mode==NativeScriptArgument_InputOutput)layout.input_output_mask|=UINT32_C(1)<<i;
    }
    *result=layout;return true;
}

static unsigned runtime_mask(const NativeScriptRuntimeContext* context)
{
    return context->source_class==NativeCallbackSource_Fighter?F:
        context->source_class==NativeCallbackSource_Item?A:
        context->source_class==NativeCallbackSource_ModelEffect?NativeScriptBindings_Effect:0;
}

bool nativeScriptBindings_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* selectors,uint32_t selector_count,
    NativeScriptValue* arguments,uint32_t count,NativeScriptValue* result,
    NativeScriptDiagnostic* diagnostic)
{
    if((semantic&NATIVE_SCRIPT_ITEM_RESOURCES_DOMAIN_MASK)==NATIVE_SCRIPT_ITEM_RESOURCES_DOMAIN)
        return nativeScriptBindingItemResources_Invoke(context,semantic,selectors,selector_count,arguments,count,result,diagnostic);
    if(nativeScriptBindingArticleReference_Handles(semantic))
        return error(diagnostic,(NativeScriptSourceSpan){0},"Article reference operation requires scoped receiver dispatch");
    if((semantic&NATIVE_SCRIPT_EFFECT_DOMAIN_MASK)==NATIVE_SCRIPT_EFFECT_DOMAIN)
        return nativeScriptBindingEffect_Invoke(context,semantic,selectors,selector_count,arguments,count,result,diagnostic);
    uint32_t model_domain=semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK;
    if(model_domain==NATIVE_SCRIPT_ITEM_MODEL_ASYNC_EFFECT||model_domain==NATIVE_SCRIPT_ITEM_MODEL_SYNC_EFFECT) {
        if(selector_count)return error(diagnostic,(NativeScriptSourceSpan){0},"Joint effect has unexpected runtime selectors");
        return nativeScriptBindingArticleFeedback_Invoke(context,semantic,arguments,count,result,diagnostic);
    }
    if(model_domain>=NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADD&&model_domain<=NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_RATE){
        if(selector_count)return error(diagnostic,(NativeScriptSourceSpan){0},"Animation call has unexpected runtime selectors");
        return nativeScriptBindingItemAnimation_Invoke(context,semantic,arguments,count,result,diagnostic);
    }
    (void)selectors;
    bool item_transform=(semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK)==NATIVE_SCRIPT_ITEM_MODEL_TRANSFORM;
    bool item_world_position=(semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK)==NATIVE_SCRIPT_ITEM_MODEL_WORLD_POSITION;
    bool item_matrix_rotation=(semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK)==NATIVE_SCRIPT_ITEM_MODEL_MATRIX_ROTATION;
    bool target_export=(semantic&NATIVE_SCRIPT_TARGET_EXPORT_DOMAIN_MASK)==NATIVE_SCRIPT_TARGET_EXPORT_DOMAIN;
    unsigned target_operation=(semantic&NATIVE_SCRIPT_TARGET_EXPORT_OPERATION_MASK)>>20;
    if(target_export&&(target_operation<1||target_operation>4))return error(diagnostic,(NativeScriptSourceSpan){0},"Recipient export operation is invalid");
    const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(item_transform?NativeScriptBinding_ItemSetJointTransform:item_world_position?NativeScriptBinding_ItemJointWorldPosition:item_matrix_rotation?NativeScriptBinding_ItemReadJointMatrixRotation:
        target_export?NativeScriptBinding_TargetsCall+target_operation-1:semantic);
    if (!context || !result || !descriptor || descriptor->kind!=NativeScriptBinding_Intrinsic ||
        selector_count || !(descriptor->contexts&runtime_mask(context)))
        return error(diagnostic,(NativeScriptSourceSpan){0},"Native intrinsic or source context is unsupported.");
    if(semantic==NativeScriptBinding_HitboxCurrent||semantic==NativeScriptBinding_HitboxPrevious)
        return error(diagnostic,(NativeScriptSourceSpan){0},"Hitbox vector selector requires a scalar projection");
    if (!nativeScriptBindings_SourceAlive(context))
        return error(diagnostic,(NativeScriptSourceSpan){0},"Native intrinsic source has retired.");
    unsigned recipient_count=(semantic&NATIVE_SCRIPT_TARGET_EXPORT_ARGUMENT_MASK)>>NATIVE_SCRIPT_TARGET_EXPORT_ARGUMENT_SHIFT;
    if (target_export?(recipient_count<2 || recipient_count>NATIVE_SCRIPT_EXPRESSION_ARGUMENTS || count!=recipient_count || !arguments):
        (count!=descriptor->argument_count || (count&&!arguments)))
        return error(diagnostic,(NativeScriptSourceSpan){0},"Native intrinsic argument count is invalid.");
    NativeScriptValue inputs[NATIVE_SCRIPT_EXPRESSION_ARGUMENTS];
    for (uint32_t i=0;i<count;++i) {
        if(target_export && i>=2) {
            if(!nativeScriptExpression_IsNumeric(arguments[i].type))
                return error(diagnostic,(NativeScriptSourceSpan){0},"Recipient helper arguments must be numeric values");
            inputs[i]=arguments[i];continue;
        }
        const NativeScriptBindingArgument* spec=&descriptor->arguments[i];
        if((semantic==NativeScriptBinding_ActionMetadata || semantic==NativeScriptBinding_ActionMoveId) &&
           arguments[i].type!=NativeScriptType_I32)
            return error(diagnostic,(NativeScriptSourceSpan){0},"Action metadata fields require exact Int arguments");
        if(semantic==NativeScriptBinding_AudioPlayGlobal && arguments[i].type!=NativeScriptType_I32)
            return error(diagnostic,(NativeScriptSourceSpan){0},"Global audio requires exact Int argument types");
        if((item_matrix_rotation||semantic==NativeScriptBinding_ItemReadJointMatrixRotation) && i==0 && arguments[i].type!=NativeScriptType_I32)
            return error(diagnostic,(NativeScriptSourceSpan){0},"Joint matrix rotation requires an exact Int model selector");
        if(semantic==NativeScriptBinding_CollisionCopyFrom && arguments[i].type!=NativeScriptType_I32)
            return error(diagnostic,(NativeScriptSourceSpan){0},"Collision copy requires an exact Int creation ID");
        if((semantic==NativeScriptBinding_ItemSetCollisionFacing||semantic==NativeScriptBinding_CaptureMask||semantic==NativeScriptBinding_CaptureArm) && arguments[i].type!=NativeScriptType_I32)
            return error(diagnostic,(NativeScriptSourceSpan){0},"Collision facing and capture masks require exact Int arguments");
        if(semantic==NativeScriptBinding_CPURecordInput && arguments[i].type!=NativeScriptType_I32)
            return error(diagnostic,(NativeScriptSourceSpan){0},"CPU history record requires an exact Int creation ID");
        if(semantic==NativeScriptBinding_ItemBounceToGroundWithContactReset && arguments[i].type!=NativeScriptType_Bool)
            return error(diagnostic,(NativeScriptSourceSpan){0},"Ground bounce contact reset requires exact Bool");
        if(semantic==NativeScriptBinding_ItemRespondToStageWithImpactResponse && arguments[i].type!=NativeScriptType_Bool)
            return error(diagnostic,(NativeScriptSourceSpan){0},"Stage impact response requires exact Bool");
        if(semantic==NativeScriptBinding_ItemReleaseAttachmentPreserveHeld && i==3 && arguments[i].type!=NativeScriptType_Bool)
            return error(diagnostic,(NativeScriptSourceSpan){0},"Attachment held-item preservation requires exact Bool");
        if((semantic==NativeScriptBinding_ItemSetAnimationDescriptor||semantic==NativeScriptBinding_ItemAddAnimationDescriptor) && arguments[i].type!=NativeScriptType_I32)
            return error(diagnostic,(NativeScriptSourceSpan){0},"Item animation descriptor requires exact Int");
        if(spec->mode==NativeScriptArgument_Output || spec->mode==NativeScriptArgument_InputOutput) {
            if(arguments[i].type!=spec->type)return error(diagnostic,(NativeScriptSourceSpan){0},"Native output formal type differs from its captured destination");
            inputs[i]=spec->mode==NativeScriptArgument_Output?(NativeScriptValue){.type=spec->type}:arguments[i];
            continue;
        }
        if (spec->mode==NativeScriptArgument_CodeRef) {
            if(semantic==NativeScriptBinding_CollisionAirLandingCallback&&arguments[i].type==NativeScriptType_Null)
                return error(diagnostic,(NativeScriptSourceSpan){0},"Air landing requires an owned helper, not None");
            if (arguments[i].type!=NativeScriptType_Callable&&arguments[i].type!=NativeScriptType_Null)
                return error(diagnostic,(NativeScriptSourceSpan){0},"Callback argument must be an owned code reference or None.");
            inputs[i]=arguments[i];continue;
        }
        if(spec->object_or_integral) {
            if(arguments[i].type!=NativeScriptType_Object && !nativeScriptExpression_IsIntegral(arguments[i].type))
                return error(diagnostic,(NativeScriptSourceSpan){0},"Native target requires an Object or integral handle");
            inputs[i]=arguments[i];continue;
        }
        if(spec->type==NativeScriptType_Object) {
            if(arguments[i].type!=NativeScriptType_Object)return error(diagnostic,(NativeScriptSourceSpan){0},"Native Object input type differs");
            inputs[i]=arguments[i];continue;
        }
        if(spec->type==NativeScriptType_Text) {
            if(spec->mode!=NativeScriptArgument_Literal || arguments[i].type!=NativeScriptType_Text)
                return error(diagnostic,(NativeScriptSourceSpan){0},"Native text input must retain its compiled literal span");
            inputs[i]=arguments[i];continue;
        }
        if (nativeScriptType_SelectorLimit(spec->type)!=0) {
            uint64_t limit=nativeScriptType_SelectorLimit(spec->type);
            if(arguments[i].type!=spec->type||arguments[i].as.u64>=limit)
                return error(diagnostic,(NativeScriptSourceSpan){0},"Native formal requires an exact immutable selector");
            inputs[i]=arguments[i];continue;
        }
        if (!nativeScriptExpression_IsNumeric(arguments[i].type))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Native intrinsic input must be numeric.");
        if(semantic==NativeScriptBinding_ModelGraphsInstall&&!nativeScriptExpression_IsIntegral(arguments[i].type))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Model graph and omission selectors require integral values");
        if(semantic==NativeScriptBinding_ItemRespondStageMask&&!nativeScriptExpression_IsIntegral(arguments[i].type))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Item stage response mask requires an integral value");
        if(semantic==NativeScriptBinding_GuardCreate&&i==0&&!nativeScriptExpression_IsIntegral(arguments[i].type))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Guard joint index requires an integral value");
        if((semantic==NativeScriptBinding_ItemRumble||semantic==NativeScriptBinding_ControllerRumble||(semantic==NativeScriptBinding_CameraQuake&&i==0))&&
           !nativeScriptExpression_IsIntegral(arguments[i].type))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Rumble/quake selector requires an integral value");
        if(descriptor==&catalog[NativeScriptBinding_ItemCreateLinked]&&(i==1||i==2)&&!nativeScriptExpression_IsIntegral(arguments[i].type))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Linked child slots require integral values");
        if(descriptor==&catalog[NativeScriptBinding_ItemCreateChild]&&(i==5||i==6||i==8)&&!nativeScriptExpression_IsIntegral(arguments[i].type))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Child damage-owner/collision/backlink requires an integral value");
        if(i==0&&((semantic>=NativeScriptBinding_HitboxEnabled&&semantic<=NativeScriptBinding_HitboxSetRadius)||semantic==NativeScriptBinding_ItemDisableHitbox||semantic==NativeScriptBinding_HitboxState||semantic==NativeScriptBinding_HitboxDamage||semantic==NativeScriptBinding_HitboxGroup||semantic==NativeScriptBinding_HitboxSetGroup||semantic==NativeScriptBinding_HitboxSetWorldPosition||semantic==NativeScriptBinding_HitboxSetEnabled||semantic==NativeScriptBinding_HitboxClearVictims||semantic==NativeScriptBinding_HitboxCreate||
       semantic==NativeScriptBinding_HitboxLaunchTarget||semantic==NativeScriptBinding_HitboxLaunchFollow)&&!nativeScriptExpression_IsIntegral(arguments[i].type))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Hitbox index requires an integral value");
        if(i==1&&semantic==NativeScriptBinding_HitboxSetGroup&&!nativeScriptExpression_IsIntegral(arguments[i].type))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Hitbox group requires an integral value");
        if(i==0&&semantic>=NativeScriptBinding_CPUQueue&&semantic<=NativeScriptBinding_CPUQueuePair&&arguments[i].type!=NativeScriptType_I32)
            return error(diagnostic,(NativeScriptSourceSpan){0},"CPU queue command must retain its exact I32 literal type");
        if(semantic==NativeScriptBinding_TargetsOverlapsHitbox&&!nativeScriptExpression_IsIntegral(arguments[i].type))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Hitbox overlap handle and slot must be integral");
        if(i==0&&(target_export||(semantic>=NativeScriptBinding_TargetsReadPosition&&semantic<=NativeScriptBinding_TargetsIsFighter)||(semantic>=NativeScriptBinding_TargetsDamage&&semantic<=NativeScriptBinding_TargetsFloorLine)||(semantic>=NativeScriptBinding_TargetsModelScale&&semantic<=NativeScriptBinding_TargetsReadCameraTargetPosition)||semantic==NativeScriptBinding_TargetsReadPositionDelta||semantic==NativeScriptBinding_TargetsSmashChargeActive||semantic==NativeScriptBinding_TargetsMotion)&&!nativeScriptExpression_IsIntegral(arguments[i].type))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Target handle requires an integral value");
        if(i==0&&(semantic==NativeScriptBinding_ItemAttachToOwner||semantic==NativeScriptBinding_ItemAttachToOwnerAux||semantic==NativeScriptBinding_ArticlesBindAuxiliary||
           (semantic>=NativeScriptBinding_ItemsBindHeld&&semantic<=NativeScriptBinding_ArticlesBindHeld)||
           (semantic>=NativeScriptBinding_ItemsSetVisible&&semantic<=NativeScriptBinding_ItemsNotifyDrop))&&
           !nativeScriptExpression_IsIntegral(arguments[i].type))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Attachment part/auxiliary handle requires an integral value");
        if((semantic==NativeScriptBinding_ColorRGBA || semantic==NativeScriptBinding_VisualsSetDiffuseColor ||
            semantic==NativeScriptBinding_VisualsInstallOutlineVisibility || semantic==NativeScriptBinding_ItemSetDiffuseColor) &&
           !nativeScriptExpression_IsIntegral(arguments[i].type))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Color components, packed colors and visibility slots require integral values");
        if((semantic==NativeScriptBinding_ArticlesLinkOwned||semantic==NativeScriptBinding_ItemLinkId||
            semantic==NativeScriptBinding_ItemLinkReciprocal||semantic==NativeScriptBinding_ItemUnlink||semantic==NativeScriptBinding_ItemCopyLinkCollision)&&
           !nativeScriptExpression_IsIntegral(arguments[i].type))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Article creation IDs and link slots require integral values");
        if((((descriptor==&catalog[NativeScriptBinding_ArticlesSignal] || descriptor==&catalog[NativeScriptBinding_ArticlesSignalOwned]) && i==0) ||
            (descriptor==&catalog[NativeScriptBinding_ArticlesCreateWithVelocity] && i==9) ||
            (descriptor==&catalog[NativeScriptBinding_ArticlesLinkOwned] && (i==0||i==2))) &&
           !nativeScriptExpression_IsIntegral(arguments[i].type))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Article handle/collision group requires an integral value");
        if((semantic==NativeScriptBinding_StateTaunt || (semantic==NativeScriptBinding_ItemsCreateEngine&&i==8)) &&
           !nativeScriptExpression_IsIntegral(arguments[i].type))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Motion and collision-group arguments require integral values");
        if(nativeScriptBindingSpawn_Base(semantic)==NativeScriptBinding_ArticlesCreateHeldAtCenter && (i==0||i==4) && !nativeScriptExpression_IsIntegral(arguments[i].type))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Held Article owner/raw part requires an integral value");
        if(semantic==NativeScriptBinding_TargetsReadBonePosition && i<2 && !nativeScriptExpression_IsIntegral(arguments[i].type))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Target handle/raw part requires an integral value");
        if(semantic==NativeScriptBinding_AnimationClipLength && !nativeScriptExpression_IsIntegral(arguments[i].type))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Animation clip index requires an integral value");
        if(i==0 && (semantic==NativeScriptBinding_BonesIndex || semantic==NativeScriptBinding_BonesWorldPosition || semantic==NativeScriptBinding_BonesWorldPositionWithOffset || semantic==NativeScriptBinding_FamilyMemberId) &&
           !nativeScriptExpression_IsIntegral(arguments[i].type))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Bone/member index requires an integral value");
        if(i==0 && semantic==NativeScriptBinding_HitlagLink && !nativeScriptExpression_IsIntegral(arguments[i].type))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Hitlag handle requires an integral value");
        if(i==12 && semantic==NativeScriptBinding_StageSweepBox && !nativeScriptExpression_IsIntegral(arguments[i].type))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Stage sweep category requires an integral value");
        if(((semantic==NativeScriptBinding_StageEvent && i==0) || semantic==NativeScriptBinding_StatsActionCount) &&
           !nativeScriptExpression_IsIntegral(arguments[i].type))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Stage channel and statistics selectors require integral values");
        if(i==0 && (semantic==NativeScriptBinding_StageLineExists||semantic==NativeScriptBinding_StageLineNormal||
           semantic==NativeScriptBinding_ItemBindStageLine||semantic==NativeScriptBinding_ItemReadStageLineSpeed) &&
           !nativeScriptExpression_IsIntegral(arguments[i].type))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Collision line requires an integral value");
        if(semantic>=NativeScriptBinding_StageReadLineIslandBounds&&semantic<=NativeScriptBinding_StageCpuIslandBlocked&&
           i<(semantic==NativeScriptBinding_StageSameLineIsland?2u:1u)&&!nativeScriptExpression_IsIntegral(arguments[i].type))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Stage island line requires an integral value");
        if(i==0 && (semantic==NativeScriptBinding_EffectsSpawnOnPart || semantic==NativeScriptBinding_EffectsSpawnBetween || semantic==NativeScriptBinding_EffectsSpawnAsyncPositionOnPart || semantic==NativeScriptBinding_EffectsSpawnAsyncPositionScalarOnPart || semantic==NativeScriptBinding_EffectsSpawnAsyncAttached || semantic==NativeScriptBinding_EffectsSpawnAsyncWithScalar || semantic==NativeScriptBinding_EffectsSpawnAtPosition || semantic==NativeScriptBinding_EffectsSpawnWithScalarAtPosition||semantic==NativeScriptBinding_EffectsSpawnWithTwoScalarsAtPosition ||
           semantic==NativeScriptBinding_ItemSpawnRootEffect || semantic==NativeScriptBinding_ItemSpawnRootEffectWithOffset || semantic==NativeScriptBinding_ItemSpawnEffectAtPosition || semantic==NativeScriptBinding_ItemSpawnWithScalarAtPosition || semantic==NativeScriptBinding_CallbackMatches) &&
           !nativeScriptExpression_IsIntegral(arguments[i].type))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Effect ID/callback slot requires an integral value");
        if((semantic==NativeScriptBinding_VisualsSetGroup || semantic==NativeScriptBinding_VisualsSetCurrentGroup ||
            semantic==NativeScriptBinding_VisualsCurrentGroup || semantic==NativeScriptBinding_VisualsPreviousGroup ||
            (semantic==NativeScriptBinding_ColorStart && i==0) ||
            ((semantic==NativeScriptBinding_VisualsSetGroupMaterialFrame||semantic==NativeScriptBinding_VisualsInitializeGroupMaterialAnimation)&&i<3)) &&
           !nativeScriptExpression_IsIntegral(arguments[i].type))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Visibility group/color selector requires an integral value");
        if (spec->numeric_any)inputs[i]=arguments[i];
        else if (!nativeScriptValue_Convert(&arguments[i],spec->type,&inputs[i],diagnostic))return false;
    }
    NativeScriptValue value={.type=descriptor->result_type};
    switch (semantic) {
    case NativeScriptBinding_ItemReleaseCamera:
        if(!nativeScriptBindingItemLifecycle_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        break;
    case NativeScriptBinding_ItemHitsStage:
        if(!nativeScriptBindingItemObservations_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        break;
    case NativeScriptBinding_ItemLandOnStage:
        if(!nativeScriptBindingItemTerrain_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        break;
    case NativeScriptBinding_ItemStopLoopSound:
        if(!nativeScriptBindingItem_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        break;
    case NativeScriptBinding_VectorsRotate:
        if(!nativeScriptBindingVectors_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        break;
    case NativeScriptBinding_PhysicsRotateVelocityUnitAxis:
    case NativeScriptBinding_PhysicsMirrorVelocity:
        if(!nativeScriptBindingKinetics_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        break;
    case NativeScriptBinding_StateDownBound:
        if(!nativeScriptBindingState_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        break;
    case NativeScriptBinding_ActionMetadata:
        value.as.i64=(int32_t)(((uint32_t)inputs[0].as.i64&255u) |
            (((uint32_t)inputs[1].as.i64&15u)<<20) |
            (((uint32_t)inputs[2].as.i64&15u)<<16) |
            (((uint32_t)inputs[3].as.i64&255u)<<8));
        break;
    case NativeScriptBinding_ActionMoveId:
        value.as.i64=(uint32_t)inputs[0].as.i64&255u;
        break;
    case NativeScriptBinding_Abs:value.as.f32=fabsf(inputs[0].as.f32);break;
    case NativeScriptBinding_Sqrt:value.as.f32=sqrtf(inputs[0].as.f32);break;
    case NativeScriptBinding_Sin:value.as.f32=sinf(inputs[0].as.f32);break;
    case NativeScriptBinding_Cos:value.as.f32=cosf(inputs[0].as.f32);break;
    case NativeScriptBinding_Min:value.as.f32=fminf(inputs[0].as.f32,inputs[1].as.f32);break;
    case NativeScriptBinding_Max:value.as.f32=fmaxf(inputs[0].as.f32,inputs[1].as.f32);break;
    case NativeScriptBinding_Atan2:value.as.f64=atan2(inputs[0].as.f64,inputs[1].as.f64);break;
    case NativeScriptBinding_ArticleAtan2:case NativeScriptBinding_Atan2Float:value.as.f32=atan2f(inputs[0].as.f32,inputs[1].as.f32);break;
    case NativeScriptBinding_Signbit: {
        uint32_t bits;memcpy(&bits,&inputs[0].as.f32,sizeof(bits));value.as.i64=bits>>31;break;
    }
    case NativeScriptBinding_WrapAngle:value.as.f32=nativeScriptMath_WrapAngle(inputs[0].as.f32);break;
    case NativeScriptBinding_WrapSignedAngle:value.as.f32=nativeScriptMath_WrapSignedAngle(inputs[0].as.f32);break;
    case NativeScriptBinding_RadiansFloat:value.as.f32=inputs[0].as.f32*0.01745329252f;break;
    case NativeScriptBinding_RadiansDouble:value.as.f64=inputs[0].as.f64*(double)0.01745329252f;break;
    case NativeScriptBinding_Int:case NativeScriptBinding_UInt:case NativeScriptBinding_Float:case NativeScriptBinding_Double:
        if (!nativeScriptValue_Convert(&inputs[0],descriptor->result_type,&value,diagnostic))return false;
        break;
    default:
        if(semantic==NativeScriptBinding_ArticlesLinkOwned||semantic==NativeScriptBinding_ItemLinkId||semantic==NativeScriptBinding_ItemLinkReciprocal||semantic==NativeScriptBinding_ItemUnlink||semantic==NativeScriptBinding_ItemCopyLinkCollision) {
            if(!nativeScriptBindingItemLinks_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_ControllerRumble||semantic==NativeScriptBinding_ItemRumble||semantic==NativeScriptBinding_CameraQuake) {
            if(!nativeScriptBindingWorldFeedback_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_ItemInheritOwnerDebugDisplay||semantic==NativeScriptBinding_ItemStageCollision||semantic==NativeScriptBinding_ItemStageCollisionMode||semantic==NativeScriptBinding_ItemReleaseAttachment||semantic==NativeScriptBinding_ItemReleaseAttachmentPreserveHeld) {
            if(!nativeScriptBindingArticleMotion_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_ItemRefreshPhysics||semantic==NativeScriptBinding_ItemRemove||semantic==NativeScriptBinding_ItemBeginExpiry) {
            if(!nativeScriptBindingItemLifecycle_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_CallbackAlignVelocityToFacing) {
            if(!nativeScriptBindingSetup_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_ItemSweepSegment||semantic==NativeScriptBinding_ItemBindStageLine||semantic==NativeScriptBinding_ItemReadStageLineSpeed) {
            if(!nativeScriptBindingItemCollision_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_ItemSetAnimationDescriptor||semantic==NativeScriptBinding_ItemAddAnimationDescriptor) {
            if(!nativeScriptBindingItemAnimation_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(item_transform||item_world_position||item_matrix_rotation||semantic==NativeScriptBinding_ItemReadJointMatrixRotation||semantic==NativeScriptBinding_ItemCloneModel||semantic==NativeScriptBinding_ItemLoadModel||semantic==NativeScriptBinding_ItemDestroyModel||semantic==NativeScriptBinding_ItemSetModelDrawEnabled||semantic==NativeScriptBinding_ItemSetJointTransform||semantic==NativeScriptBinding_ItemSetScale||semantic==NativeScriptBinding_ItemSetAnimationFrame||semantic==NativeScriptBinding_ItemAdvanceAnimation||semantic==NativeScriptBinding_ItemAdvanceModelAnimation||semantic==NativeScriptBinding_ItemSetAppearance||
                  (semantic>=NativeScriptBinding_ItemResetSpin&&semantic<=NativeScriptBinding_ItemInheritOwnerScale)||
                  (semantic>=NativeScriptBinding_ItemHideModel&&semantic<=NativeScriptBinding_ItemSetModelAnimationRate)) {
            if(!nativeScriptBindingItemModel_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if((semantic>=NativeScriptBinding_ControlsTrySideSmash&&semantic<=NativeScriptBinding_StateCrouchWait)||
                  (semantic>=NativeScriptBinding_ControlsTrySideSpecial&&semantic<=NativeScriptBinding_ControlsTryDownSpecial)||
                  semantic==NativeScriptBinding_ControlsTryGuard||semantic==NativeScriptBinding_ControlsTryCatch||semantic==NativeScriptBinding_ControlsTryTaunt||semantic==NativeScriptBinding_ControlsTryHeavyItemThrow) {
            if(!nativeScriptBindingGroundControls_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_CollisionCopyFrom||semantic==NativeScriptBinding_CollisionAirCatchHit) {
            if(!nativeScriptBindingCollision_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_AudioPlayGlobal) {
            if(!nativeScriptBindingAudio_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_ModelGraphsInstall) {
            if(!nativeScriptBindingModelGraph_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if((semantic>=NativeScriptBinding_ItemDisableSurfaceResponse&&semantic<=NativeScriptBinding_ItemSweepStageFlags)||
                  (semantic>=NativeScriptBinding_ItemSweepFloor&&semantic<=NativeScriptBinding_ItemAlignFacingToVelocity)||semantic==NativeScriptBinding_ItemReadAttachmentMatrixRotation||semantic==NativeScriptBinding_ItemCancelHitlagRequest||semantic==NativeScriptBinding_ItemSweepStage||semantic==NativeScriptBinding_ItemSweepStageNormal||semantic==NativeScriptBinding_ItemCheckGround) {
            if(!nativeScriptBindingItemObservations_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_HitboxCreate) {
            if(!nativeScriptBindingHitboxCreate_Invoke(context,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_HitboxState||semantic==NativeScriptBinding_HitboxDamage||semantic==NativeScriptBinding_HitboxGroup||semantic==NativeScriptBinding_HitboxSetGroup||semantic==NativeScriptBinding_HitboxSetWorldPosition||semantic==NativeScriptBinding_HitboxSetEnabled||semantic==NativeScriptBinding_HitboxClearVictims||semantic==NativeScriptBinding_HitboxClearAll||
                  semantic==NativeScriptBinding_HitboxLaunchTarget||semantic==NativeScriptBinding_HitboxLaunchFollow||
                  (semantic==NativeScriptBinding_HitboxSetDamage&&context->source_class==NativeCallbackSource_Fighter)) {
            if(!nativeScriptBindingFighterHitboxes_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic>=NativeScriptBinding_AccessoryCreate&&semantic<=NativeScriptBinding_AccessoryClear) {
            if(!nativeScriptBindingPresentation_AccessoryInvoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic>=NativeScriptBinding_AttachmentsHasRoot&&semantic<=NativeScriptBinding_AttachmentsClearAll) {
            if(!nativeScriptBindingAttachments_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if((semantic>=NativeScriptBinding_HitboxEnabled&&semantic<=NativeScriptBinding_ItemDisableHitbox)) {
            if(!nativeScriptBindingArticleCombat_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_ItemCreateAtPosition||semantic==NativeScriptBinding_ItemCreateChild||semantic==NativeScriptBinding_ItemCreateLinked) {
            return error(diagnostic,(NativeScriptSourceSpan){0},"Child factory owner was not resolved at load");
        } else if(semantic==NativeScriptBinding_TargetsNearestAvailableFighter || semantic==NativeScriptBinding_TargetsOverlapsHitbox || semantic==NativeScriptBinding_TargetsReadOpponentAhead || semantic==NativeScriptBinding_TargetsReadLockableItemAhead ||
                  semantic==NativeScriptBinding_TargetsAttackOverlapsCapsule || semantic==NativeScriptBinding_TargetsArticleAttackOverlapsCapsule ||
                  target_export||(semantic>=NativeScriptBinding_TargetsReadPosition&&semantic<=NativeScriptBinding_TargetsIsFighter) ||semantic==NativeScriptBinding_TargetsSmashChargeActive ||
                  semantic==NativeScriptBinding_TargetsControllerColorPresent || semantic==NativeScriptBinding_TargetsControllerColor || semantic==NativeScriptBinding_TargetsReadBonePosition || semantic==NativeScriptBinding_TargetsMotion || semantic==NativeScriptBinding_TargetsReadPositionDelta ||
                  (semantic>=NativeScriptBinding_TargetsDamage&&semantic<=NativeScriptBinding_TargetsFloorLine)||
                  (semantic>=NativeScriptBinding_TargetsModelScale&&semantic<=NativeScriptBinding_TargetsReadCameraTargetPosition)) {
            if(!nativeScriptBindingTargets_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if((semantic>=NativeScriptBinding_CPUClearTarget&&semantic<=NativeScriptBinding_CPUNearestEnemyAt)||
                  (semantic>=NativeScriptBinding_CPUPrepareDestinationUpdate&&semantic<=NativeScriptBinding_CPUSetApproachSpacing)||
                  (semantic>=NativeScriptBinding_CPURecordInput&&semantic<=NativeScriptBinding_CPUPlanWithoutTarget)) {
            if(!nativeScriptBindingCPUControl_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_ColorRGBA || semantic==NativeScriptBinding_VisualsSetDiffuseColor || semantic==NativeScriptBinding_VisualsInstallOutlineVisibility || semantic==NativeScriptBinding_ItemSetDiffuseColor) {
            if(!nativeScriptBindingColors_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if((semantic>=NativeScriptBinding_ItemsSetVisible&&semantic<=NativeScriptBinding_ItemsNotifyDrop) ||
                  (semantic>=NativeScriptBinding_ItemsNotifyPickup&&semantic<=NativeScriptBinding_ItemsCreateEngine) ||
                  semantic==NativeScriptBinding_ItemsSpawnFood) {
            if(!nativeScriptBindingFighterItems_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_ArticlesBindAuxiliary ||
                  (semantic>=NativeScriptBinding_ItemsClearHeld&&semantic<=NativeScriptBinding_ArticlesBindHeld)) {
            if(!nativeScriptBindingFighterItems_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic>=NativeScriptBinding_GuardCreate&&semantic<=NativeScriptBinding_CollisionGuardHit) {
            if(!nativeScriptBindingGuard_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic>=NativeScriptBinding_ControlsAerialAttackMotion&&semantic<=NativeScriptBinding_ControlsTryAirSpecial) {
            if(!nativeScriptBindingAirInputs_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_PhysicsAnimationVerticalVelocity||semantic==NativeScriptBinding_PhysicsAnimationVerticalVelocityAdjusted||semantic==NativeScriptBinding_CollisionAirLandingCallback||semantic==NativeScriptBinding_PhysicsAirDriftWithLimits||semantic==NativeScriptBinding_PhysicsAirDriftScaled) {
            if(!nativeScriptBindingAirPhysics_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_StatsCountAsThrownItem ||
                  semantic==NativeScriptBinding_StatsRefreshAttack || semantic==NativeScriptBinding_StatsRecordAttack ||
                  semantic==NativeScriptBinding_StatsActionCount || semantic==NativeScriptBinding_StatsNotifyTaunt) {
            if(!nativeScriptBindingFighterObservations_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_CollisionContactReboundAllowed) {
            if(!nativeScriptBindingContact_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_ItemAttachmentOwnerIs||
                  (semantic>=NativeScriptBinding_ItemClearCurrentOwner&&semantic<=NativeScriptBinding_ItemAttachToOwnerAux)) {
            if(!nativeScriptBindingArticleLifecycle_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if((semantic>=NativeScriptBinding_VectorsAngleXY&&semantic<=NativeScriptBinding_VectorsCrossNormalizedZ)||
                  semantic==NativeScriptBinding_VectorsNormalize||semantic==NativeScriptBinding_VectorsNormalizeXY) {
            if(!nativeScriptBindingVectors_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_ArticlesRef||semantic==NativeScriptBinding_ArticlesReadPrimaryTranslation||semantic==NativeScriptBinding_ArticlesReadBonePosition) {
            return error(diagnostic,(NativeScriptSourceSpan){0},"Article reference declaration was not resolved at load");
        } else if((semantic&NATIVE_SCRIPT_SIGNAL_DOMAIN_MASK)==NATIVE_SCRIPT_SIGNAL_DOMAIN) {
            if(!nativeScriptBindingSignal_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_ArticlesSignal || semantic==NativeScriptBinding_ArticlesSignalOwned || semantic==NativeScriptBinding_ItemSignal) {
            return error(diagnostic,(NativeScriptSourceSpan){0},"Signal callback identity was not resolved at load");
        } else if(semantic==NativeScriptBinding_ArticlesCreateAtPosition || semantic==NativeScriptBinding_ArticlesCreateWithVelocity || semantic==NativeScriptBinding_ArticlesCreateUnownedAtPosition || semantic==NativeScriptBinding_ArticlesCreateHeldAtCenter) {
            return error(diagnostic,(NativeScriptSourceSpan){0},"Article factory owner was not resolved at load");
        } else if(semantic==NativeScriptBinding_AbsorberCreate) {
            if(!nativeScriptBindingAbsorber_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_EffectsSpawnAtPosition||semantic==NativeScriptBinding_EffectsSpawnWithScalarAtPosition||semantic==NativeScriptBinding_EffectsSpawnWithTwoScalarsAtPosition) {
            if(!nativeScriptBindingFeedback_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_ItemSpawnRootEffect || semantic==NativeScriptBinding_ItemSpawnRootEffectWithOffset||semantic==NativeScriptBinding_ItemDestroyEffects||semantic==NativeScriptBinding_ItemSpawnEffectAtPosition||semantic==NativeScriptBinding_ItemSpawnWithScalarAtPosition||semantic==NativeScriptBinding_ItemSpawnAsyncJointEffect||semantic==NativeScriptBinding_ItemSpawnAsyncJointEffectWithScalar) {
            if(!nativeScriptBindingArticleFeedback_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if((semantic>=NativeScriptBinding_AttackEnterJab&&semantic<=NativeScriptBinding_CollisionAirAerialLandingCallback)||semantic==NativeScriptBinding_CollisionAirAerialLandingWithLedges) {
            if(!nativeScriptBindingNormalAttacks_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_CallbackMatches||semantic==NativeScriptBinding_CallbackAccessoryBefore||semantic==NativeScriptBinding_CallbackAccessoryHitlag) {
            if(!nativeScriptBindingSetup_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if((semantic>=NativeScriptBinding_FacingFromInputThreshold&&semantic<=NativeScriptBinding_CollisionSkipSoftFloor) ||
                  semantic==NativeScriptBinding_PhysicsGroundToAir||semantic==NativeScriptBinding_WalkEnter||
                  semantic==NativeScriptBinding_WalkUpdateAnimation||semantic==NativeScriptBinding_PhysicsWalk||
                  semantic==NativeScriptBinding_PhysicsGroundAnimationFacingMotion||
                  semantic==NativeScriptBinding_ControlsWalkRequested||semantic==NativeScriptBinding_ControlsWalkStopRequested||
                  semantic==NativeScriptBinding_ControlsTurnRequested||semantic==NativeScriptBinding_ControlsPlatformDropRequested||
                  semantic==NativeScriptBinding_ControlsJumpInput||semantic==NativeScriptBinding_StatePlatformDrop) {
            if(!nativeScriptBindingCommonMovement_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_CollisionGroundStandardBox) {
            if(!nativeScriptBindingCollision_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_CollisionGroundWithNudge) {
            if(!nativeScriptBindingActions_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_VisualsSetVisible||semantic==NativeScriptBinding_VisualsSetGroup||semantic==NativeScriptBinding_ColorStart||semantic==NativeScriptBinding_ColorClearAttack||semantic==NativeScriptBinding_VisualsApplyPartAnimation||
                  semantic==NativeScriptBinding_VisualsSetPartAnimation||semantic==NativeScriptBinding_VisualsShowPartAnimation||semantic==NativeScriptBinding_VisualsHidePartAnimation||
                  semantic==NativeScriptBinding_VisualsSetGroupMaterialFrame||semantic==NativeScriptBinding_VisualsInitializeGroupMaterialAnimation||
                  (semantic>=NativeScriptBinding_VisualsSetCurrentGroup&&semantic<=NativeScriptBinding_VisualsPreviousGroup)) {
            if(!nativeScriptBindingVisuals_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if((semantic>=NativeScriptBinding_CallbackReflectHit&&semantic<=NativeScriptBinding_CallbackTrackShieldHit) ||
                  semantic==NativeScriptBinding_CallbackSecondaryDamage||semantic==NativeScriptBinding_ItemsSetHeldVisible) {
            if(!nativeScriptBindingSetup_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic>=NativeScriptBinding_PoseLoadAnimation&&semantic<=NativeScriptBinding_PoseCopyToLive) {
            if(!nativeScriptBindingPose_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_HitlagRequestRelease||semantic==NativeScriptBinding_HitlagUnlink||semantic==NativeScriptBinding_HitlagLink) {
            if(!nativeScriptBindingHitlag_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_FamilyMemberId||semantic==NativeScriptBinding_FamilyRetireCompanionOnSleep) {
            if(!nativeScriptBindingFamily_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_StageEvent||semantic==NativeScriptBinding_StageSweepBox||semantic==NativeScriptBinding_StageSegmentBlocked||semantic==NativeScriptBinding_StageLineExists||semantic==NativeScriptBinding_StageLineNormal||
                  (semantic>=NativeScriptBinding_StageReadFloor&&semantic<=NativeScriptBinding_StageIslandBounds)) {
            if(!nativeScriptBindingStage_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if((semantic>=NativeScriptBinding_ItemFollowGround&&semantic<=NativeScriptBinding_ItemBounceToGroundWithContactReset)||semantic==NativeScriptBinding_ItemSlideOnGround||semantic==NativeScriptBinding_ItemRespondToStage||semantic==NativeScriptBinding_ItemRespondToStageWithImpactResponse) {
            if(!nativeScriptBindingItemTerrain_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_ItemSetCollisionFacing||semantic==NativeScriptBinding_ItemResetDefaultEcb) {
            if(!nativeScriptBindingItemCollision_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_ItemClearAnimationCommands) {
            if(!nativeScriptBindingItemLifecycle_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_CaptureMask||semantic==NativeScriptBinding_CaptureArm||
                  semantic==NativeScriptBinding_CaptureSetVictimCaptureMotion||semantic==NativeScriptBinding_CaptureInitializeHold||
                  (semantic>=NativeScriptBinding_CaptureSetVictimHoldMotion&&semantic<=NativeScriptBinding_CaptureSyncVictimFacing)||
                  semantic==NativeScriptBinding_ThrowsUpdate||semantic==NativeScriptBinding_PhysicsThrowMotion||
                  semantic==NativeScriptBinding_CaptureReleaseDamage||semantic==NativeScriptBinding_CaptureReleaseDamageFall||
                  semantic==NativeScriptBinding_CaptureDropVictim||semantic==NativeScriptBinding_CaptureBeginAttached||
                  (semantic>=NativeScriptBinding_CaptureBegin&&semantic<=NativeScriptBinding_CaptureConstrainCaught)) {
            if(!nativeScriptBindingCapture_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_CollisionProbeLedge||semantic==NativeScriptBinding_CollisionLedgeAvailable) {
            if(!nativeScriptBindingCollision_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_StateDamageFall||semantic==NativeScriptBinding_StateCliffJumpPhase2||semantic==NativeScriptBinding_StateFinish) {
            if(!nativeScriptBindingState_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_EffectsSpawnOnPart||semantic==NativeScriptBinding_EffectsSpawnBetween||semantic==NativeScriptBinding_EffectsSpawnAsyncPositionOnPart||semantic==NativeScriptBinding_EffectsSpawnAsyncPositionScalarOnPart) {
            if(!nativeScriptBindingEffect_Invoke(context,semantic,NULL,0,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_EffectsSpawn||semantic==NativeScriptBinding_EffectsWind||semantic==NativeScriptBinding_EffectsSpawnAsyncAttached||semantic==NativeScriptBinding_EffectsSpawnAsyncWithScalar||semantic==NativeScriptBinding_EffectsWindAtPosition||
           semantic==NativeScriptBinding_EffectsSpawnWithScalar||semantic==NativeScriptBinding_EffectsSpawnOnJointWithScalar) {
            if(!nativeScriptBindingFeedback_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_ItemTrackAccessory||semantic==NativeScriptBinding_ItemClearAccessory||
                  semantic==NativeScriptBinding_ItemBounceVictim||semantic==NativeScriptBinding_ItemRespondStageMask) {
            if(!nativeScriptBindingItem_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_StateCatchLedge) {
            if(!nativeScriptBindingLedge_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_StateTaunt) {
            if(!nativeScriptBindingState_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_ControlsTauntActions) {
            if(!nativeScriptBindingActions_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_AnimationClipLength) {
            if(!nativeScriptBindingKinetics_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_StateEnterAtSpeed) {
            if(!nativeScriptBindingTransition_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic>=NativeScriptBinding_CallbackAnimation&&semantic<=NativeScriptBinding_CallbackAccessoryCue) {
            if(!nativeScriptBindingSetup_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_FormsActivatePartner) {
            if(!nativeScriptBindingForms_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_StateHeldOpen) {
            if(!nativeScriptBindingHeldState_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_JumpsConfigureMulti||semantic==NativeScriptBinding_JumpsMultiVerticalSpeed||semantic==NativeScriptBinding_JumpBegin||semantic==NativeScriptBinding_JumpUpdateAnimation||
           semantic==NativeScriptBinding_JumpPrepare||semantic==NativeScriptBinding_JumpCheckShortHop||
           semantic==NativeScriptBinding_JumpLaunch||semantic==NativeScriptBinding_PhysicsJumpMotion||semantic==NativeScriptBinding_BonesRootAddRotationY||
           semantic==NativeScriptBinding_ControlsAerialJumpActions||semantic==NativeScriptBinding_CollisionTryWallJump) {
            if(!nativeScriptBindingJump_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic>=NativeScriptBinding_CollisionCheckGroundLedge && semantic<=NativeScriptBinding_ControlsTryAerialJump) {
            if(!nativeScriptBindingLedge_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if((semantic&NATIVE_SCRIPT_SPAWN_DOMAIN_MASK)==NATIVE_SCRIPT_SPAWN_DOMAIN) {
            if(!nativeScriptBindingSpawn_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic==NativeScriptBinding_DamageQuietKO || semantic==NativeScriptBinding_DamageUpdateTimer || semantic==NativeScriptBinding_PhysicsDamageMotion ||
                  semantic==NativeScriptBinding_DamageInterruptCleanup) {
            if(!nativeScriptBindingCombat_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic>=NativeScriptBinding_CPU) {
            if(!nativeScriptBindingActions_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic>=NativeScriptBinding_Articles) {
            if(!nativeScriptBindingSpawn_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic>=NativeScriptBinding_Item) {
            if(!nativeScriptBindingItem_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic>=NativeScriptBinding_Sound) {
            if(!nativeScriptBindingPresentation_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if(semantic>=NativeScriptBinding_StateEnter) {
            if(!nativeScriptBindingTransition_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if (semantic>=NativeScriptBinding_Damage) {
            if (!nativeScriptBindingCombat_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if (semantic>=NativeScriptBinding_Fighter) {
            if (!nativeScriptBindingSetup_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if (semantic>=NativeScriptBinding_State) {
            if (!nativeScriptBindingState_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        } else if (!nativeScriptBindingKinetics_Invoke(context,semantic,inputs,count,&value,diagnostic))return false;
        break;
    }
    if (!nativeScriptBindings_SourceAlive(context))return error(diagnostic,(NativeScriptSourceSpan){0},"Native intrinsic source retired before publication.");
    for(uint32_t i=0;i<descriptor->argument_count;++i)if(descriptor->arguments[i].mode==NativeScriptArgument_Output || descriptor->arguments[i].mode==NativeScriptArgument_InputOutput) {
        if(inputs[i].type!=descriptor->arguments[i].type)return error(diagnostic,(NativeScriptSourceSpan){0},"Native output type differs after primitive execution");
    }
    for(uint32_t i=0;i<descriptor->argument_count;++i)if(descriptor->arguments[i].mode==NativeScriptArgument_Output || descriptor->arguments[i].mode==NativeScriptArgument_InputOutput)arguments[i]=inputs[i];
    *result=value;return true;
}

const NativeScriptVMHost nativeScriptBindingHost = {
    .source_alive=nativeScriptBindings_SourceAlive,
    .read=nativeScriptBindings_Read,.write=nativeScriptBindings_Write,
    .invoke=nativeScriptBindings_Invoke,
};
