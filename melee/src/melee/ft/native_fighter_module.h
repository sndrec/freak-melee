#ifndef MELEE_FT_NATIVE_FIGHTER_MODULE_H
#define MELEE_FT_NATIVE_FIGHTER_MODULE_H

#ifdef MELEE_NATIVE

#include "native_fighter_runtime.h"
#include "native_fighter_parameters.h"
#include "native_callback_lifetime.h"
#include "native_script_vm.h"
#include "native_fighter_jump.h"
#include "native_fighter_pose.h"
#include "types.h"
#include "kinds/ftCommon/forward.h"
#include <melee/lb/forward.h>
#include <melee/cm/forward.h>
#include <melee/it/types.h>
#include <melee/it/native_article_resource.h>
#include <melee/ef/types.h>
#include <melee/mp/forward.h>
#include <melee/gr/forward.h>
#include <melee/ft/ftcmdscript.h>
#include <melee/ft/ftcpu_profile.h>
#include <melee/db/db.h>

struct NativeItemLifetime;

#include <stddef.h>
#include <dolphin/mtx.h>
#include <sysdolphin/baselib/forward.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { NATIVE_FIGHTER_SOUND_CHANNEL_COUNT = 7 };
typedef void (*NativeFighterSoundChannel)(Fighter*, int, u8, u8);

// One exact-client native DLL owns its definition, callbacks and motion table.
// It exports FreakMelee_GetFighterModule(base, host); there is no ABI migration
// path for modules compiled against a different client build.
// Initialize this structure to zero before Open. Callers must release every
// game reference to these pointers before Close.
typedef struct NativeFighterModule {
    void* library;
    const NativeFighterDefinition* definition;
    const NativeFighterDefinition* base;
    uint64_t parameter_schema;
    uint32_t parameter_bits[NATIVE_FIGHTER_PARAMETER_CAPACITY];
    uint16_t parameter_count;
    bool parameters_bound;
} NativeFighterModule;

// Bind immutable project values at package load. Schema changes require a
// script rebuild; value-only changes do not. No binding occurs during play.
bool nativeFighterModule_SetParameters(
    NativeFighterModule* module,
    const NativeFighterParameters* parameters);

// Exact-client engine functions available to authored callbacks. These are
// immutable process functions; fighter gameplay state stays in tracked objects.
struct plActionStats;
typedef enum NativeFighterCommonAttribute {
    NativeFighterCommon_ThrowHorizontal,
    NativeFighterCommon_ThrowUp,
    NativeFighterCommon_ThrowDown,
    NativeFighterCommon_AerialJumpReverse,
    NativeFighterCommon_TapJumpThreshold,
    NativeFighterCommon_AerialActionDownThreshold,
    NativeFighterCommon_ShieldStartHealth,
    NativeFighterCommon_ShieldMinimumHold,
    NativeFighterCommon_ShieldPushbackDamage,
    NativeFighterCommon_ShieldPushbackBase,
    NativeFighterCommon_ShieldPushbackVelocity,
    NativeFighterCommon_ShieldPushbackLightMin,
    NativeFighterCommon_ShieldPushbackLightMax,
    NativeFighterCommon_ShieldReflectDuration,
    NativeFighterCommon_ShieldReflectRadius,
    NativeFighterCommon_ShieldReflectDamage,
    NativeFighterCommon_ShieldReflectSpeed,
    NativeFighterCommon_ShieldPowershieldDuration,
    NativeFighterCommon_HorizontalSmashThreshold,
    NativeFighterCommon_HorizontalSmashWindow,
    NativeFighterCommon_HorizontalSmashExtension,
    NativeFighterCommon_SmashShakeCount,
    NativeFighterCommon_HorizontalStickDeadzone,
    NativeFighterCommon_StickSmoothing,
} NativeFighterCommonAttribute;
typedef struct NativeFighterHost {
    bool (*activate_partner)(Fighter_GObj*, HSD_GObjEvent stock_finish);
    uint32_t (*family_member_id)(Fighter_GObj*, unsigned member_index);
    bool (*link_hitlag)(Fighter_GObj*, uint32_t target);
    uint32_t (*linked_hitlag_id)(Fighter_GObj*);
    void (*request_hitlag_release)(Fighter_GObj*);
    bool (*copy_collision_from)(Fighter_GObj*, uint32_t source);
    bool (*target_bone_position)(uint32_t target, unsigned part, Vec3* position);
    bool (*target_position)(uint32_t target, Vec3* position);
    bool (*cpu_nearest_enemy_at)(Fighter_GObj*, const Vec3*, uint32_t* target);
    bool (*cpu_record_input)(Fighter_GObj*, uint32_t);
    bool (*cpu_read_history_pose)(Fighter_GObj*, Vec3*, float*);
    bool (*cpu_apply_recorded_input)(Fighter_GObj*);
    void (*cpu_prepare_combat_plan)(Fighter*);
    bool (*cpu_prepare_destination_update)(Fighter*);
    void (*cpu_update_recovery_destination)(Fighter*);
    bool (*cpu_plan_attack)(Fighter*);
    void (*cpu_plan_without_target)(Fighter*);
    void (*cpu_set_destination)(Fighter*, float, float, float);
    bool (*stage_same_line_island)(int32_t, int32_t);
    bool (*stage_cpu_floor_excluded)(int32_t);
    bool (*stage_cpu_island_blocked)(int32_t);
    bool (*stage_read_floor)(float, float, float, float, float, Vec3*, int*,
                             u32*, Vec3*, int, int, int,
                             bool (*)(Fighter_GObj*, int), Fighter_GObj*);
    bool (*target_position_delta)(uint32_t, Vec3*);
    float (*target_damage)(uint32_t);
    float (*target_facing)(uint32_t);
    bool (*target_sleeping)(uint32_t);
    int32_t (*target_floor_line)(uint32_t);
    float (*target_cpu_body_half_width)(uint32_t);
    bool (*article_ref_primary_translation)(HSD_GObj*, uint32_t, ItemKind, Vec3*);
    void (*change_motion_state)(Fighter_GObj*, FtMotionId, MotionFlags,
                                float, float, float, Fighter_GObj*);
    void (*start_animation)(Fighter_GObj*);
    bool (*ground_collision)(Fighter_GObj*);
    GroundOrAir (*air_collision)(Fighter_GObj*);
    bool (*special_ground_collision)(Fighter_GObj*);
    bool (*follow_floor)(int, Vec3*, int*, Vec3*, u32*, Vec3*, float, float);
    void (*set_hitbox_position)(HitCapsule*, Vec3*);
    bool (*check_ground_escape)(Fighter_GObj*);
    void (*create_guard)(Fighter_GObj*, ShieldDesc*, HSD_GObjEvent);
    bool (*ground_box_collision)(Fighter_GObj*, ftCollisionBox*);
    void (*ground_standard_box_collision)(Fighter_GObj*, ftCollisionBox*);
    bool (*air_box_collision)(Fighter_GObj*, ftCollisionBox*);
    void (*ground_to_air)(Fighter*);
    void (*air_to_ground)(Fighter*);
    bool (*animation_frames_remaining)(Fighter_GObj*);
    void (*finish_ground_action)(Fighter_GObj*);
    void (*enter_taunt)(Fighter_GObj*, FtMotionId, FtMotionId);
    void (*enter_fall)(Fighter_GObj*);
    void (*enter_freefall)(Fighter_GObj*, int, int, bool, float, float);
    void (*ground_actions)(Fighter_GObj*);
    void (*taunt_actions)(Fighter_GObj*);
    bool (*air_actions)(Fighter_GObj*);
    void (*ground_friction)(Fighter_GObj*);
    void (*air_drift)(Fighter_GObj*);
    void (*ground_motion)(Fighter_GObj*);
    void (*ground_animation_velocity)(Fighter_GObj*);
    void (*air_motion)(Fighter_GObj*);
    void (*air_gravity_and_friction)(Fighter_GObj*);
    void (*clamp_air_drift)(Fighter*);
    void (*fall)(Fighter*, float, float);
    bool (*quick_air_decel)(Fighter*);
    bool (*quick_decel)(Fighter*, float);
    float (*air_drift_threshold)(void);
    void (*air_decel)(Fighter*, float);
    void (*fall_basic)(Fighter*);
    void (*ascend)(Fighter*, float, float);
    void (*ground_accel)(Fighter*, float, float, float);
    void (*air_accel)(Fighter*, float, float, float);
    void (*air_accel_friction)(Fighter*, float, float, float);
    void (*air_accel_normal)(Fighter*);
    void (*air_accel_from)(Fighter*, float horizontal_velocity);
    void (*ground_to_self_movement)(Fighter_GObj*);
    void (*ground_decelerate)(Fighter*, float);
    void (*clamp_self_vel_x)(Fighter*, float);
    void (*clamp_ground_vel)(Fighter*, float);
    void (*clamp_fall_speed)(Fighter*, float);
    void (*air_standard_collision)(Fighter_GObj*);
    void (*air_catch_hit_collision)(Fighter_GObj*);
    void (*ground_standard_collision)(Fighter_GObj*);
    void (*ground_nudge_collision)(Fighter_GObj*);
    void (*air_special_landing_collision)(Fighter_GObj*,
                                          bool (*)(Fighter_GObj*, int),
                                          void (*)(Fighter_GObj*));
    bool (*fall_special_ledge_check)(Fighter_GObj*, int);
    void (*enter_landing_fall_special)(Fighter_GObj*, bool, float);
    void (*update_facing)(Fighter*);
    void (*set_root_rotation_y)(Fighter*, int, float);
    void (*set_root_rotation_x)(Fighter*, int, float);
    void (*set_root_rotation_z)(Fighter*, int, float);
    void (*align_root_to_floor)(Fighter*);
    double (*angle_atan2)(double, double);
    Fighter_Part (*resolve_bone)(Fighter*, Fighter_Part);
    void (*joint_position)(HSD_JObj*, Vec3*, Vec3*);
    void (*spawn_projectile)(Fighter_GObj*, Vec3*, ItemKind, s32, float);
    void (*spawn_authored_projectile)(Fighter_GObj*, Vec3*, ItemKind, s32,
                                      float, const NativeFighterProjectileLaunch*);
    s32 (*spawn_attached_article)(Fighter_GObj*, ItemKind, Fighter_Part,
                                  s32, s32, s32, s32, float);
    s32 (*spawn_bone_attached_article)(Fighter_GObj*, ItemKind, Fighter_Part);
    bool (*owned_article_exists)(Fighter_GObj*, s32);
    bool (*remove_owned_article)(Fighter_GObj*, s32);
    bool (*pause_owned_article)(Fighter_GObj*, s32);
    bool (*resume_owned_article)(Fighter_GObj*, s32);
    bool (*signal_owned_article)(Fighter_GObj*, s32, uint32_t, float);
    EF_SpawnResult (*spawn_effect)(s32, HSD_GObj*, ...);
    void (*spawn_model_effect)(Fighter_GObj*, const NativeFighterModelEffect*, s32, Vec3*, bool);
    HSD_JObj* (*model_effect_joint)(EF_Effect*, unsigned);
    Fighter_GObj* (*model_effect_creator)(EF_Effect*);
    void (*model_effect_cancel)(EF_Effect*);
    HSD_JObj* (*model_joint_resolve)(HSD_JObj*, const char*);
    float (*model_joint_read)(HSD_JObj*, unsigned);
    void (*model_joint_write)(HSD_JObj*, unsigned, float);
    void (*spawn_wind)(Vec3*, int, float, float, float);
    void (*spawn_wind_at_fighter)(Fighter_GObj*, int, float, float, float);
    void (*spawn_effect_between_parts)(Fighter_GObj*, s32, s32, s32);
    HSD_GObjEvent clear_effects;
    HSD_GObjEvent destroy_effects;
    void (*spawn_effect_on_part)(Fighter_GObj*, s32, s32);
    void (*spawn_effect_with_facing)(Fighter_GObj*, s32, s32);
    HSD_GObjEvent align_velocity_to_facing;
    HSD_GObjEvent mark_effect_active;
    void (*set_hitbox_damage)(HitCapsule*, u32, Fighter_GObj*);
    void (*spawn_async_effect)(HSD_GObj*, void*, u32, u32, HSD_JObj*, ...);
    void (*force_air_and_use_jumps)(Fighter*);
    s32 (*detected_item_kind)(Fighter*);
    HSD_GObjEvent pause_effects;
    HSD_GObjEvent resume_effects;
    void (*create_reflector)(Fighter_GObj*, ReflectDesc*, HSD_GObjEvent);
    void (*update_reflector)(Fighter_GObj*);
    void (*copy_ext_attributes)(Fighter_GObj*);
    void (*reload_ext_attributes)(Fighter_GObj*);
    void (*set_visibility_group)(Fighter_GObj*, int, int);
    void (*set_current_visibility_group)(Fighter_GObj*, int, int);
    uint8_t (*costume_variant)(Fighter*);
    void (*apply_part_animation)(Fighter_GObj*, s32, s32, float);
    float (*random_float)(void);
    s32 (*random_int)(s32 upper_bound);
    s32 (*random_int_excluding)(s32 upper_bound, s32 first, s32 second);
    void (*animation_velocity_y)(Fighter_GObj*);
    void (*animation_velocity_y_adjusted)(Fighter_GObj*);
    void (*animation_velocity_x)(Fighter_GObj*);
    bool (*check_ground_and_ledge)(Fighter_GObj*, int);
    bool (*check_cliff_catch)(Fighter_GObj*);
    void (*enter_cliff_catch)(Fighter_GObj*);
    void (*arm_grab)(Fighter*, s16, HSD_GObjEvent, HSD_GObjEvent,
                     void (*)(Fighter_GObj*, Fighter_GObj*));
    bool (*begin_capture)(Fighter_GObj*, Fighter_GObj*, FtMotionId, bool, bool, bool, bool);
    void (*set_capture_mask)(Fighter*, s16);
    void (*clear_capture_movement)(Fighter_GObj*);
    void (*capture_victim_to_air)(Fighter*, Fighter*);
    void (*follow_capture_victim)(Fighter_GObj*);
    bool (*release_capture_throw)(Fighter_GObj*);
    void (*enter_landing_basic)(Fighter_GObj*);
    bool (*aerial_attack_actions)(Fighter_GObj*);
    Item_GObj* (*spawn_holdable_item)(Fighter_GObj*, uint16_t);
    void (*self_damage)(Fighter_GObj*, float);
    void (*fighter_item_pickup)(Fighter_GObj*, bool, bool, bool);
    void (*fighter_item_drop)(Fighter_GObj*, bool, bool, bool);
    void (*fighter_item_visible)(Fighter_GObj*, bool);
    void (*fighter_item_invisible)(Fighter_GObj*, bool);
    void (*fighter_knockback_enter)(Fighter_GObj*, s32);
    void (*fighter_knockback_exit)(Fighter_GObj*, s32);
    void (*item_gravity)(Item_GObj*);
    void (*item_fall)(Item_GObj*, float, float);
    void (*item_loop_sound)(Item*, enum_t, u8, u8);
    Item_GObj* (*item_spawn_food)(Item_GObj*);
    void (*item_land)(Item_GObj*);
    void (*item_bounce)(Item_GObj*);
    void (*item_change_state)(Item_GObj*, int);
    void (*item_set_lifetime)(Item_GObj*, float);
    bool (*item_reflect)(Item_GObj*);
    bool (*item_stage_collision)(Item_GObj*);
    bool (*item_bounce_stage)(Item_GObj*);
    bool (*item_shield_bounce)(Item_GObj*);
    void (*item_play_sound)(Item_GObj*, s32, u8, u8);
    void (*item_spawn_effect_on_bone)(Item_GObj*, s32, u32, u8);
    void (*item_activate_attached)(Item_GObj*);
    float (*fighter_model_scale)(Fighter_GObj*);
    float (*fighter_root_scale_y)(Fighter_GObj*);
    bool (*fighter_incapacitated)(Fighter_GObj*);
    void (*item_set_uniform_scale)(Item_GObj*, float);
    void (*item_set_child_visible)(Item_GObj*, bool);
    bool (*item_interact)(Item_GObj*, HSD_GObj*);
    float (*sin_float)(float);
    float (*cos_float)(float);
    float (*abs_float)(float);
    float (*sqrt_float)(float);
    float (*min_float)(float, float);
    float (*max_float)(float, float);
    void (*set_model_scale)(HSD_JObj*, Vec3*);
    void (*clear_hitboxes)(Fighter_GObj*);
    void (*clear_attack_color)(Fighter*);
    bool (*start_color)(Fighter*, enum_t, bool);
    void (*set_animation_rate)(Fighter_GObj*, float);
    void (*play_sound)(Fighter*, enum_t, u8, u8);
    void (*loop_sound)(Fighter*, int, u8, u8);
    const NativeFighterSoundChannel* sound_channels;
    float (*floor_friction)(u32);
    bool (*ground_box_edges)(Fighter_GObj*, ftCollisionBox*);
    bool (*air_box_ledge)(Fighter_GObj*, int, ftCollisionBox*);
    float (*angle_atan2_float)(float, float);
    void (*camera_quake)(CmQuakeKind, Vec3*);
    void (*rumble)(Fighter*, u32, u32);
    HSD_GObjEvent refresh_attack;
    HSD_GObjEvent record_attack;
    bool (*item_is_heavy)(Item_GObj*);
    enum_t (*item_hold_kind)(Item_GObj*);
    enum_t (*item_use_category)(Item_GObj*);
    bool (*cstick_attack_edge)(Fighter*);
    FtMotionId (*aerial_attack_motion)(Fighter*);
    void (*enter_air_gun)(Fighter_GObj*);
    void (*set_part_animation)(Fighter_GObj*, s32, s32);
    void (*show_part_animation)(Fighter_GObj*, s32);
    void (*hide_part_animation)(Fighter_GObj*, int);
    bool (*has_model_root)(Fighter_GObj*, const char*);
    bool (*load_model_attachment)(Fighter_GObj*, unsigned, const char*,
                                  unsigned, int, unsigned);
    bool (*add_attachment_dynamics)(Fighter_GObj*, unsigned, unsigned, unsigned);
    HSD_GObjEvent clear_model_attachments;
    bool (*check_turn_input)(Fighter_GObj*);
    bool (*check_taunt)(Fighter_GObj*);
    bool (*check_crouch)(Fighter_GObj*);
    bool (*check_turn)(Fighter_GObj*);
    bool (*check_walk)(Fighter_GObj*);
    bool (*check_ground_jump)(Fighter_GObj*);
    bool (*check_air_jump)(Fighter_GObj*);
    bool (*check_platform_drop)(Fighter_GObj*);
    void (*enter_platform_drop)(Fighter_GObj*, FtMotionId, MotionFlags, float);
    float (*get_bone_rotation_y)(Fighter*, int);
    HSD_GObjEvent finish_action;
    float (*item_joint_value)(Item_GObj*, unsigned, const char*, unsigned);
    void (*item_set_joint_value)(Item_GObj*, unsigned, const char*, unsigned, float);
    bool (*item_sweep_stage)(Item_GObj*, Vec3*, Vec3*, Vec3*);
    bool (*item_segment_sweep)(HSD_GObj*, Vec3*, Vec3*, Vec3*, int*, u32*);
    bool (*stage_line_exists)(int);
    void (*set_collision_line)(CollData*, int);
    bool (*stage_line_speed)(int, Vec3*, Vec3*);
    Vec3* (*stage_line_normal)(int, Vec3*);
    void (*item_set_animation_frame)(HSD_GObj*, float);
    void (*item_set_hitbox_damage)(HitCapsule*, u32, Item_GObj*);
    void (*item_spawn_effect)(Item_GObj*, s32, s32, Vec3*, Vec3*, s32, float);
    void (*item_spawn_joint_effect_at)(Item_GObj*, int, const char*, const Vec3*);
    uint32_t (*item_contact_fighter)(Item_GObj*);
    bool (*target_is_fighter)(uint32_t);
    bool (*target_guard_active)(uint32_t);
    float (*target_model_scale)(uint32_t);
    float (*target_shield)(uint32_t, Vec3*);
    void (*vector_mirror)(Vec3*, Vec3*);
    float (*wrap_angle)(float);
    void (*item_advance_animation)(Item_GObj*);
    HSD_GObjEvent item_pause;
    HSD_GObjEvent item_resume;
    void (*item_remove)(Item_GObj*);
    void (*item_scale_by_attribute)(Item_GObj*, float);
    void (*item_spawn_directional_effect)(Item_GObj*, s32, Fighter_Part,
                                          Vec3*, float);
    void (*notify_item_pickup)(Fighter_GObj*, bool);
    float (*bone_segment_angle)(Fighter_GObj*, Fighter_Part, Vec3*, Vec3*);
    s32 (*spawn_article)(Fighter_GObj*, ItemKind, s32);
    float (*get_bone_rotation_x)(Fighter*, int);
    HSD_GObjEvent item_inherit_owner_scale;
    HSD_GObjEvent item_use_manual_lifetime;
    HSD_GObjEvent ground_animation_motion;
    bool (*item_load_model)(Item_GObj*, unsigned, uint32_t);
    bool (*item_add_model_animation)(Item_GObj*, unsigned, const char*, uint32_t);
    void (*item_request_model_animation)(Item_GObj*, unsigned, const char*, float);
    void (*item_advance_model_animation)(Item_GObj*, unsigned, const char*);
    void (*item_remove_model_animation)(Item_GObj*, unsigned, const char*);
    void (*item_set_model_animation_rate)(Item_GObj*, unsigned, const char*, float);
    bool (*item_read_joint_matrix_rotation)(Item_GObj*, unsigned, const char*, Vec3*);
    void (*item_set_joint_transform)(Item_GObj*, unsigned, const char*,
                                     const Vec3*, const Quaternion*, const Vec3*, bool);
    bool (*item_clone_model)(Item_GObj*, unsigned);
    void (*item_destroy_model)(Item_GObj*, unsigned);
    float (*vector_angle_xy)(Vec3*, Vec3*);
    bool (*skip_soft_floor)(Fighter_GObj*);
    bool (*stage_event)(u32, float);
    struct plActionStats* (*get_action_stats)(int);
    unsigned (*action_count)(struct plActionStats*, ssize_t);
    void (*notify_taunt)(int, int);
    void (*enter_wait_blend)(Fighter_GObj*, float);
    HSD_GObjEvent update_throw;
    HSD_GObjEvent throw_motion;
    FtWalkType (*walk_type)(Fighter_GObj*);
    bool (*walk_requested)(Fighter_GObj*);
    bool (*walk_stop_requested)(Fighter_GObj*);
    void (*enter_walk)(Fighter_GObj*, FtMotionId, MotionFlags, float,
                        float, float, float, float, float, float, float);
    HSD_GObjEvent update_walk_animation;
    HSD_GObjEvent walk_physics;
    float (*animation_length)(Fighter_GObj*);
    FigaTree* (*animation_clip)(Fighter*, int);
    float (*clip_length)(FigaTree*);
    ftCo_JumpInput (*jump_input)(Fighter_GObj*);
    HSD_GObjEvent check_short_hop;
    void (*launch_jump)(Fighter_GObj*, bool, float);
    void (*begin_aerial_jump)(Fighter_GObj*, FtMotionId, Vec3*, bool, bool, bool);
    HSD_GObjEvent jump_physics;
    void (*enter_landing)(Fighter_GObj*, FtMotionId, bool, MotionFlags, float, float);
    void (*initialize_hold)(Fighter_GObj*, FtMotionId);
    void (*set_hold_motion)(Fighter_GObj*, FtMotionId);
    void (*set_throw_motion)(Fighter_GObj*, FtMotionId, float);
    float (*common_attribute)(NativeFighterCommonAttribute);
    HSD_GObjEvent update_damage_timer;
    HSD_GObjEvent damage_physics;
    HSD_GObjPredicate check_heavy_item_throw;
    bool (*damage_reaction_suppressed)(Fighter*);
    float (*scale_knockback)(float);
    s32 (*knockback_level)(float);
    void (*enter_damage_motion)(Fighter_GObj*, int, float);
    void (*enter_damage_reaction)(Fighter_GObj*, float);
    void (*release_held_item)(Item_GObj*, Vec3*, float);
    void (*cpu_command)(Fighter*, u8);
    void (*cpu_command_arg)(Fighter*, u8, u8);
    void (*cpu_command_pair)(Fighter*, u8, u8, u8);
    void (*cpu_run_script)(Fighter*, int);
    // Engine callbacks only. Package motion flags and animation IDs stay owned.
    const MotionState* common_motion_states;
    mp_UnkStruct0* (*find_floor_island)(Vec3*, float);
    void (*island_bounds)(mp_UnkStruct0*, Vec3*, Vec3*);
    void (*set_hurtboxes)(Fighter_GObj*, HurtCapsuleState);
    void (*set_body_status)(Fighter_GObj*, enum_t);
    void (*set_group_material_frame)(Fighter*, int, int, int, float);
    unsigned (*bone_count)(Fighter*);
    ItemKind (*held_item_kind)(Item_GObj*);
    void (*throw_held_item)(Fighter_GObj*, FtMotionId);
    int32_t (*create_held_article_at_center)(Fighter_GObj*, ItemKind, Vec3*, unsigned, float);
    float (*item_dynamic_joint_value)(Item_GObj*, unsigned, unsigned);
    void (*item_set_dynamic_joint_value)(Item_GObj*, unsigned, unsigned, float);
    void (*item_set_animation_descriptor)(Item_GObj*, unsigned);
    s32 (*item_sweep_stage_flags)(Item_GObj*);
    s32 (*item_wall_contact_flags)(Item_GObj*);
    s32 (*item_ceiling_contact_flags)(Item_GObj*);
    void (*item_align_to_floor)(Item_GObj*);
    bool (*item_reflect_from_wall)(Item_GObj*);
    void (*item_reset_default_ecb)(Item_GObj*, itECB*);
    void (*item_set_grounded)(Item*);
    void (*item_set_airborne)(Item*);
    void (*item_bounce_stage_to_ground)(Item_GObj*, HSD_GObjEvent);
    void (*set_collision_facing)(CollData*, int);
    void (*set_bone_hurtbox)(Fighter_GObj*, int, HurtCapsuleState);
    void (*set_held_item_visible)(Fighter_GObj*, bool);
    void (*unlock_ecb)(Fighter*);
    float (*vector_normalize)(Vec3*);
    float (*vector_angle)(Vec3*, Vec3*);
    float (*wrap_signed_angle)(float);
    void (*item_disable_hitboxes)(Item_GObj*);
    void (*initialize_attached_capture)(Fighter_GObj*, Fighter_GObj*, FtMotionId);
    void (*enter_attached_capture_motion)(Fighter_GObj*, FtMotionId);
    void (*enter_attached_throw)(Fighter_GObj*, FtMotionId, MotionFlags, float, bool);
    void (*unlink_capture)(Fighter_GObj*, Fighter_GObj*);
    void (*apply_throw_damage)(Fighter_GObj*, Fighter_GObj*);
    void (*launch_thrown_victim)(Fighter_GObj*, Fighter_GObj*, bool);
    HSD_GObjEvent enter_damage_fall;
    HSD_GObjEvent ground_animation_facing_motion;
    bool (*cpu_ranged_unsafe)(Fighter*, Fighter*, float);
    bool (*fighters_share_floor)(Fighter*, Fighter*);
    void (*cpu_add_candidate)(Fighter*, int);
    FighterKind (*animation_family)(Fighter*);
    int32_t (*create_article_at)(Fighter_GObj*, ItemKind, Vec3*, Vec3*, float, bool, uint32_t, float, bool);
    bool (*stage_segment_intersection)(Vec3*, int*, u32*, Vec3*, int, int,
                                       float, float, float, float);
    int (*play_global_sound)(enum_t, int, int);
    bool (*check_fastfall)(Fighter*);
    void (*fastfall)(Fighter*);
    bool (*probe_ledge)(Fighter_GObj*, float, float);
    Fighter_GObj* (*find_ledge_occupant)(Fighter_GObj*);
    void (*enter_cliff_jump_phase2)(Fighter_GObj*, float);
    bool (*item_spawn_link)(Item_GObj*, unsigned, unsigned, ItemKind, Vec3*, float);
    void (*item_unlink)(Item_GObj*, unsigned);
    bool (*item_link_reciprocal)(Item_GObj*, unsigned, unsigned);
    float (*item_link_value)(Item_GObj*, unsigned, unsigned);
    float (*item_bone_value)(Item_GObj*, unsigned, unsigned);
    float (*item_link_bone_value)(Item_GObj*, unsigned, unsigned, unsigned);
    void (*item_copy_link_collision)(Item_GObj*, unsigned);
    bool (*item_signal_link)(Item_GObj*, unsigned, uint32_t, float);
    void (*item_change_state_flags)(Item_GObj*, enum_t, Item_StateChangeFlags);
    void (*item_set_debug_owner)(Item_GObj*, Fighter_GObj*);
    HSD_GObjEvent clear_kinetics;
    void (*air_decel_normal)(Fighter*);
    void (*update_facing_threshold)(Fighter*);
    u32 (*new_article_collision_group)(void);
    bool (*link_owned_articles)(Fighter_GObj*, s32, unsigned, s32);
    bool (*item_check_ground)(Item_GObj*);
    void (*item_show_model)(Item_GObj*);
    void (*item_hide_model)(Item_GObj*);
    void (*item_attach_to_owner)(Item_GObj*, HSD_GObj*, Fighter_Part);
    void (*item_release_attachment)(Item_GObj*, Vec3*);
    void (*item_spawn_joint_effect)(Item_GObj*, int, const char*);
    void (*item_spawn_async_joint_effect)(Item_GObj*, int, const char*, unsigned);
    bool (*item_stage_collision_land)(Item_GObj*);
    void (*item_stage_response)(Item_GObj*, HSD_GObjEvent);
    void (*item_begin_expiry)(Item_GObj*);
    bool (*item_expire_tick)(Item_GObj*);
    void (*item_stop_motion)(Item_GObj*);
    Fighter_GObj* (*item_find_fighter)(Vec3*, Vec3*, Item_GObj*, float);
    HSD_GObj* (*item_find_target)(Vec3*, float);
    void (*item_center_position)(Item_GObj*, Vec3*);
    float (*vector_normalize_xy)(Vec3*);
    void (*vector_rotate)(Vec3*, int, float);
    bool (*item_owner_overlap)(Item_GObj*, unsigned);
    void (*item_follow_ground)(Item_GObj*, HSD_GObjEvent);
    void (*item_bounce_to_ground)(Item_GObj*, HSD_GObjEvent);
    void (*item_slide_on_ground)(Item_GObj*, HSD_GObjEvent, HSD_GObjEvent);
    HSD_GObjEvent item_align_facing;
    void (*model_animation_rate)(HSD_JObj*, float);
    bool (*model_animation_active)(HSD_JObj*);
    HSD_GObjEvent item_sweep_floor;
    void (*item_stage_contact_response)(Item_GObj*, HSD_GObjEvent);
    bool (*try_wall_jump)(Fighter_GObj*);
    void (*init_hurtbox)(Fighter*, FighterHurtCapsule*, ftHurtboxInit*);
    s32 (*body_hit_status)(Fighter_GObj*);
    HSD_GObjEvent reset_damage_callbacks;
    HSD_GObjEvent enter_crouch_wait;
    bool (*check_side_special)(Fighter_GObj*);
    bool (*check_up_special)(Fighter_GObj*);
    bool (*check_neutral_special)(Fighter_GObj*);
    bool (*check_down_special)(Fighter_GObj*);
    bool (*check_catch)(Fighter_GObj*);
    bool (*check_side_smash)(Fighter_GObj*);
    bool (*check_up_smash)(Fighter_GObj*);
    bool (*check_down_smash)(Fighter_GObj*);
    bool (*check_side_tilt)(Fighter_GObj*);
    bool (*check_up_tilt)(Fighter_GObj*);
    bool (*check_down_tilt)(Fighter_GObj*);
    bool (*check_jab)(Fighter_GObj*);
    bool (*check_ground_roll)(Fighter_GObj*);
    bool (*check_guard_spotdodge)(Fighter_GObj*);
    bool (*capture_attach_item)(Item_GObj*, Fighter_GObj*, unsigned);
    bool (*capture_release_item)(Item_GObj*, unsigned, bool);
    bool (*capture_release_into_enclosure)(Fighter_GObj*);
    int32_t (*create_unowned_article_at)(Fighter_GObj*, ItemKind, Vec3*, Vec3*, float);
    void (*model_advance_animation)(HSD_JObj*);
    float (*initialize_group_material_animation)(Fighter*, int, int, int, float);
    int (*current_model_group)(Fighter*, int);
    int (*previous_model_group)(Fighter*, int);
    bool (*install_model_graph)(Fighter_GObj*, int graph_slot, int omission);
    void (*capture_enclose_caught)(Fighter_GObj*, float);
    void (*constrain_caught)(Fighter*, Fighter*);
    void (*add_joint_rotation_y)(HSD_JObj*, float);
    void (*set_joint_translation)(HSD_JObj*, Vec3*);
    void (*copy_bone_pose)(Fighter*, Fighter_Part, HSD_Joint*);
    HSD_Joint* (*find_costume_pose)(Fighter*, HSD_JObj*, HSD_Joint*);
    HSD_Joint* (*pose_graph)(Fighter_GObj*, int32_t);
    FigaTree* (*pose_clip)(Fighter_GObj*, int32_t);
    bool (*pose_validate)(Fighter_GObj*, NativeFighterPoseOperation, int32_t,
                          int32_t, HSD_Joint*, FigaTree*);
    void (*load_pose_animation)(Fighter*, bool, FigaTree*);
    void (*request_pose_frame)(HSD_JObj*, float);
    void (*copy_blend_pose)(Fighter*, Fighter_Part, HSD_Joint*);
    void (*blend_graph_live)(Fighter*, Fighter_Part, float, float, HSD_Joint*);
    void (*blend_graph_alternate)(Fighter*, Fighter_Part, float, float, HSD_Joint*);
    void (*blend_pose_to_live)(Fighter*, Fighter_Part, float, float);
    void (*copy_pose_to_live)(Fighter*, Fighter_Part);
    HSD_GObjEvent install_guard;
    void (*install_guard_reflector)(Fighter_GObj*, HSD_GObjEvent);
    HSD_GObjEvent consume_guard_release_input;
    HSD_GObjPredicate drain_guard;
    HSD_GObjEvent tick_powershield;
    HSD_GObjEvent guard_sdi;
    HSD_GObjEvent guard_post_sdi;
    HSD_GObjEvent enter_guard_hold;
    HSD_GObjEvent enter_guard_release;
    void (*scale_guard_joint)(Fighter*);
    HSD_GObjEvent guard_reflect_actions;
    HSD_GObjEvent guard_physics;
    HSD_GObjEvent guard_hit_physics;
    HSD_GObjEvent guard_collision;
    HSD_GObjEvent guard_hit_collision;
    HSD_GObjPredicate check_powershield;
    HSD_GObjPredicate check_guard_item_throw;
    HSD_GObjPredicate check_guard_platform_drop;
    HSD_GObjEvent update_aerial_jump;
    int32_t (*create_item_at_position)(Item_GObj*, ItemKind, Vec3*, float);
    HSD_GObjPredicate item_hits_stage;
    void (*item_land_on_stage)(Item_GObj*, HSD_GObjEvent);
    Fighter_GObj* (*item_creator)(Item_GObj*);
    uint32_t (*item_current_owner)(Item_GObj*);
    uint32_t (*item_reflector)(Item_GObj*);
    bool (*target_camera_target_position)(uint32_t, Vec3*);
    void (*create_absorber)(Fighter_GObj*, AbsorbDesc*);
    void (*heal_damage)(Fighter_GObj*, float);
    void (*item_add_animation_descriptor)(Item_GObj*, unsigned);
    bool (*item_read_attachment_matrix_rotation)(Item_GObj*, Vec3*);
    HSD_GObjPredicate check_guard;
    HSD_GObjPredicate check_dash;
    HSD_GObjPredicate check_air_special;
    bool (*article_ref_exists)(HSD_GObj*, uint32_t, ItemKind);
    float (*article_ref_float)(HSD_GObj*, uint32_t, ItemKind, unsigned, int32_t);
    int32_t (*article_ref_int)(HSD_GObj*, uint32_t, ItemKind, unsigned, int32_t);
    bool (*article_ref_bool)(HSD_GObj*, uint32_t, ItemKind, unsigned, int32_t);
    int32_t (*item_link_id)(Item_GObj*, unsigned);
    int32_t (*item_create_child)(Item_GObj*, ItemKind, Vec3*, float,
                                uint32_t, uint32_t, float, unsigned);
    bool (*target_call)(uint32_t target, uint32_t name_hash);
    bool (*target_call_bool)(uint32_t target, uint32_t name_hash);
    int32_t (*target_call_int)(uint32_t target, uint32_t name_hash);
    float (*target_call_float)(uint32_t target, uint32_t name_hash);
    bool (*callback_source_alive)(HSD_GObj*, uint32_t generation,
                                  uint32_t source_class);
    bool (*bind_fighter_item)(Fighter_GObj*, uint32_t handle, bool auxiliary);
    bool (*clear_fighter_item)(Fighter_GObj*, bool auxiliary);
    int32_t (*held_item_id)(Fighter_GObj*, bool auxiliary);
    int32_t (*create_engine_item)(Fighter_GObj*, ItemKind, Vec3*, Vec3*, float,
                                  uint32_t collision_group, float damage);
    bool (*attach_created_item)(Fighter_GObj*, uint32_t handle, int32_t raw_part,
                                bool notify_holder);
    bool (*inherit_created_item_scale)(Fighter_GObj*, uint32_t handle, bool remember);
    void (*item_set_appearance)(Item_GObj*, int descriptor, float frame);
    HSD_GObjEvent item_reset_spin;
    HSD_GObjEvent item_refresh_physics;
    float (*multi_jump_vertical_speed)(Fighter*, int index);
    bool (*configure_multi_jump)(Fighter*, const NativeFighterMultiJumpStats*);
    float (*basic_landing_velocity_threshold)(Fighter*);
    void (*air_drift_with_limits)(Fighter_GObj*, float threshold,
                                  float acceleration, float speed);
    HSD_GObjEvent aerial_jump_actions;
    HSD_GObjEvent enter_fall_aerial;
    // Same immutable authored package, regardless of the item's damage owner.
    bool (*signal_article)(HSD_GObj*, uint32_t handle, uint32_t name, float value);
    void (*item_rumble)(Item_GObj*, s32, s32);
    void (*item_stop_loop_sound)(Item*);
    bool (*article_resource_begin)(Item_GObj*, unsigned count);
    bool (*article_resource_create)(Item_GObj*, unsigned, const NativeArticleResourceInit*);
    bool (*article_resource_load_model)(Item_GObj*, unsigned, uint32_t);
    bool (*article_resource_set_draw)(Item_GObj*, unsigned, bool);
    unsigned (*article_resource_count)(Item_GObj*);
    bool (*article_resource_exists)(Item_GObj*, unsigned);
    bool (*article_resource_bind_external_part)(Item_GObj*, unsigned, unsigned);
    bool (*article_resource_set_capture_anchor)(Item_GObj*, unsigned);
    void (*article_resource_release)(Item_GObj*);
    float (*article_resource_read_joint)(Item_GObj*, unsigned, const uint8_t*, unsigned, unsigned);
    bool (*article_resource_write_joint)(Item_GObj*, unsigned, const uint8_t*, unsigned, unsigned, float);
    bool (*article_resource_set_world_matrix)(Item_GObj*, unsigned, const Mtx);
    float (*article_resource_read_float)(Item_GObj*, unsigned, NativeArticleResourceFloat);
    bool (*article_resource_write_float)(Item_GObj*, unsigned, NativeArticleResourceFloat, float);
    int32_t (*article_resource_read_int)(Item_GObj*, unsigned, NativeArticleResourceInt);
    bool (*article_resource_write_int)(Item_GObj*, unsigned, NativeArticleResourceInt, int32_t);
    bool (*article_resource_sweep_stage)(Item_GObj*, unsigned);
    bool (*article_resource_sweep_point)(Item_GObj*, unsigned);
    bool (*article_resource_follow_floor)(Item_GObj*, unsigned);
    bool (*article_resource_read_line_speed)(Item_GObj*, unsigned, int, Vec3*);
    bool (*article_resource_read_attachment_point)(Item_GObj*, unsigned, const Vec3*, Vec3*);
    bool (*article_resource_read_attachment_matrix)(Item_GObj*, unsigned, Mtx);
    bool (*article_resource_read_world_matrix)(Item_GObj*, unsigned, Mtx);
    bool (*article_resource_add_animation)(Item_GObj*, unsigned, const uint8_t*, unsigned, uint32_t);
    bool (*article_resource_request_animation)(Item_GObj*, unsigned, const uint8_t*, unsigned, float);
    bool (*article_resource_advance_animation)(Item_GObj*, unsigned, const uint8_t*, unsigned);
    bool (*article_resource_remove_animation)(Item_GObj*, unsigned, const uint8_t*, unsigned);
    bool (*article_resource_set_animation_rate)(Item_GObj*, unsigned, const uint8_t*, unsigned, float);
    bool (*article_resource_spawn_effect_at_position)(Item_GObj*, unsigned, int32_t, const Vec3*);
    bool (*article_resource_spawn_effect_facing_at_position)(Item_GObj*, unsigned, int32_t, const Vec3*, float);
    StKind (*stage_internal_kind)(void);
    void (*item_release_camera)(Item_GObj*);
    Vec3* (*vector_cross_normalized)(Vec3*, Vec3*, Vec3*);
    void (*vector_rotate_unit_axis)(Vec3*, Vec3*, float);
    void (*enter_freefall_blend)(Fighter_GObj*, int, int, int, float, float, float);
    HSD_GObjEvent enter_down_bound;
    bool (*stage_line_island_bounds)(int32_t, Vec3*, Vec3*);
    void (*item_set_instance_scale)(Item_GObj*, HSD_JObj*, float);
    void (*item_respond_stage_mask)(Item_GObj*, s32);
    float (*target_instance_scale_y)(uint32_t);
    int32_t (*target_motion)(uint32_t);
    int32_t (*target_held_id)(uint32_t);
    int32_t (*target_held_kind)(uint32_t);
    int32_t (*target_held_motion)(uint32_t);
    float (*target_held_frame)(uint32_t);
    bool (*bind_existing_item)(Fighter_GObj*, uint32_t, bool auxiliary);
    bool (*set_item_visible)(uint32_t, bool visible);
    bool (*set_item_paused)(uint32_t, bool paused);
    bool (*notify_item_drop)(Fighter_GObj*, uint32_t, bool normal);
    void (*clear_hitbox_victims)(HitCapsule*);
    void (*initialize_collision)(CollData*);
    bool (*sweep_collision_box)(CollData*, ftCollisionBox*);
    void (*enter_held_open)(Fighter_GObj*, float blend);
    bool (*notify_held_neutral)(Fighter_GObj*, bool* claimed);
    bool (*target_airborne)(uint32_t);
    HSD_GObj* (*spawn_food)(Item_GObj*, Vec3*);
    uint32_t (*article_ref_current_owner)(HSD_GObj*, uint32_t, ItemKind);
    bool (*target_smash_charge_active)(uint32_t);
    bool (*enter_jab)(Fighter_GObj*, FtMotionId);
    void (*enter_rapid_jab)(Fighter_GObj*, enum_t);
    bool (*try_ground_pickup)(Fighter_GObj*);
    void (*air_aerial_landing_collision)(Fighter_GObj*, HSD_GObjEvent);
    void (*enter_landing_aerial)(Fighter_GObj*, FtMotionId, float);
    const DbLKind* debug_level;
    bool (*item_joint_world_position)(Item_GObj*, unsigned, const char*, Vec3*);
    void (*set_fighter_diffuse_color)(Fighter_GObj*, GXColor*);
    void (*set_item_diffuse_color)(HSD_GObj*, GXColor*);
    bool (*target_controller_color)(uint32_t, GXColor*);
    bool (*install_outline_visibility)(Fighter_GObj*, int32_t);
    bool (*set_item_layered)(Item_GObj*, bool);
    void (*item_disable_hitbox)(Item_GObj*, s32);
    bool (*rapid_jab_should_finish)(Fighter_GObj*);
    // Signature changed with the explicit retained Item result. Moving this field
    // makes the existing offset fingerprint reject older two-argument modules.
    bool (*check_air_item_throw)(Fighter_GObj*, bool*, struct NativeItemLifetime*);
    // Relocation makes the offset fingerprint reject the old Bool-only ABI.
    NativeScriptVMStatus (*contact_rebound_allowed)(Fighter_GObj*, bool*);
} NativeFighterHost;

// A DLL built against another native fighter layout must be rejected before
// its entry point can interpret the host/base pointers. This value is derived
// from the current C layout; it is not a manually maintained format version.
static inline uint64_t nativeFighterModule_LayoutFingerprint(void)
{
    const size_t fields[] = {
        offsetof(NativeFighterDefinition, on_jab_enter),
        offsetof(NativeFighterDefinition, on_rapid_jab_enter),
        offsetof(NativeFighterDefinition, on_down_tilt_enter),
        offsetof(NativeFighterDefinition, on_aerial_attack_motion),
        offsetof(NativeFighterHost, enter_jab),
        offsetof(NativeFighterHost, enter_rapid_jab),
        offsetof(NativeFighterHost, rapid_jab_should_finish),
        offsetof(NativeFighterHost, try_ground_pickup),
        offsetof(NativeFighterHost, air_aerial_landing_collision),
        offsetof(NativeFighterHost, enter_landing_aerial),
        offsetof(NativeFighterHost, debug_level),
        offsetof(NativeFighterHost, item_joint_world_position),
        offsetof(NativeFighterHost, set_fighter_diffuse_color),
        offsetof(NativeFighterHost, set_item_diffuse_color),
        offsetof(NativeFighterHost, target_controller_color),
        offsetof(NativeFighterHost, install_outline_visibility),
        offsetof(NativeFighterHost, article_ref_current_owner),
        offsetof(NativeFighterHost, target_smash_charge_active),
        offsetof(NativeFighterHost, target_airborne),
        offsetof(NativeFighterHost, spawn_food),
        offsetof(NativeFighterHost, enter_held_open),
        offsetof(NativeFighterHost, notify_held_neutral),
        offsetof(NativeFighterDefinition, on_neutral_enter),
        offsetof(NativeFighterDefinition, on_air_resume_motion),
        offsetof(NativeFighterDefinition, on_held_timer_expired),
        offsetof(NativeFighterArticleLogic, on_holder_neutral),
        offsetof(NativeFighterDefinition, on_held_motion),
        offsetof(NativeFighterHost, clear_hitbox_victims),
        offsetof(NativeFighterHost, initialize_collision),
        offsetof(NativeFighterHost, sweep_collision_box),
        offsetof(NativeFighterHost, stage_internal_kind),
        offsetof(NativeFighterHost, item_release_camera),
        offsetof(NativeFighterHost, vector_cross_normalized),
        offsetof(NativeFighterHost, vector_rotate_unit_axis),
        offsetof(NativeFighterHost, enter_freefall_blend),
        offsetof(NativeFighterHost, enter_down_bound),
        offsetof(NativeFighterHost, stage_line_island_bounds),
        offsetof(NativeFighterHost, item_set_instance_scale),
        offsetof(NativeFighterHost, item_respond_stage_mask),
        offsetof(NativeFighterHost, target_instance_scale_y),
        offsetof(NativeFighterHost, target_motion),
        offsetof(NativeFighterHost, target_held_id),
        offsetof(NativeFighterHost, target_held_kind),
        offsetof(NativeFighterHost, target_held_motion),
        offsetof(NativeFighterHost, target_held_frame),
        offsetof(NativeFighterArticleLogic, on_held_status),
        offsetof(NativeFighterArticleLogic, on_held_request),
        offsetof(NativeFighterHost, bind_existing_item),
        offsetof(NativeFighterHost, set_item_visible),
        offsetof(NativeFighterHost, set_item_paused),
        offsetof(NativeFighterHost, notify_item_drop),
        offsetof(NativeFighterArticleLogic, resource_capacity),
        offsetof(NativeFighterArticleLogic, resource_reserved_part),
        NATIVE_ARTICLE_RESOURCE_COLLECTIONS, NATIVE_ARTICLE_RESOURCE_NODES,
        NativeArticleResource_FloatCount, NativeArticleResource_IntCount,
        offsetof(NativeFighterHost, article_resource_begin),
        offsetof(NativeFighterHost, article_resource_create),
        sizeof(NativeArticleResourceInit),
        offsetof(NativeArticleResourceInit, attachment_part),
        offsetof(NativeArticleResourceInit, radius),
        offsetof(NativeArticleResourceInit, collision_category),
        offsetof(NativeArticleResourceInit, position),
        offsetof(NativeFighterHost, article_resource_load_model),
        offsetof(NativeFighterHost, article_resource_set_draw),
        offsetof(NativeFighterHost, article_resource_count),
        offsetof(NativeFighterHost, article_resource_exists),
        offsetof(NativeFighterHost, article_resource_bind_external_part),
        offsetof(NativeFighterHost, article_resource_set_capture_anchor),
        offsetof(NativeFighterHost, article_resource_release),
        offsetof(NativeFighterHost, article_resource_read_joint),
        offsetof(NativeFighterHost, article_resource_write_joint),
        offsetof(NativeFighterHost, article_resource_set_world_matrix),
        offsetof(NativeFighterHost, article_resource_read_float),
        offsetof(NativeFighterHost, article_resource_write_float),
        offsetof(NativeFighterHost, article_resource_read_int),
        offsetof(NativeFighterHost, article_resource_write_int),
        offsetof(NativeFighterHost, article_resource_sweep_stage),
        offsetof(NativeFighterHost, article_resource_sweep_point),
        offsetof(NativeFighterHost, article_resource_follow_floor),
        offsetof(NativeFighterHost, article_resource_read_line_speed),
        offsetof(NativeFighterHost, article_resource_read_attachment_point),
        offsetof(NativeFighterHost, article_resource_read_attachment_matrix),
        offsetof(NativeFighterHost, article_resource_read_world_matrix),
        offsetof(NativeFighterHost, article_resource_add_animation),
        offsetof(NativeFighterHost, article_resource_request_animation),
        offsetof(NativeFighterHost, article_resource_advance_animation),
        offsetof(NativeFighterHost, article_resource_remove_animation),
        offsetof(NativeFighterHost, article_resource_set_animation_rate),
        offsetof(NativeFighterHost, article_resource_spawn_effect_at_position),
        offsetof(NativeFighterHost, article_resource_spawn_effect_facing_at_position),
        offsetof(NativeFighterHost, signal_article),
        offsetof(NativeFighterHost, item_rumble),
        offsetof(NativeFighterHost, item_stop_loop_sound),
        offsetof(NativeFighterDefinition, on_shield_break),
        offsetof(NativeFighterDefinition, on_cpu_projectile_defense),
        offsetof(NativeFighterHost, multi_jump_vertical_speed),
        offsetof(NativeFighterHost, configure_multi_jump),
        sizeof(NativeFighterMultiJumpStats),
        offsetof(NativeFighterMultiJumpStats, vertical_impulses),
        offsetof(NativeFighterMultiJumpStats, count),
        offsetof(NativeFighterMultiJumpStats, first_motion),
        offsetof(NativeFighterMultiJumpStats, helmet_first_motion),
        offsetof(NativeFighterHost, basic_landing_velocity_threshold),
        offsetof(NativeFighterHost, air_drift_with_limits),
        offsetof(NativeFighterHost, aerial_jump_actions),
        offsetof(NativeFighterHost, enter_fall_aerial),
        sizeof(EF_Effect), sizeof(EF_SpawnResult),
        offsetof(EF_Effect, native_generation),
        offsetof(EF_Effect, native_creator_generation),
        offsetof(EF_Effect, native_author),
        offsetof(EF_Effect, native_policy),
        offsetof(EF_Effect, native_attachment_part),
        offsetof(EF_Effect, native_cancelled),
        offsetof(EF_Effect, native_origin_joint),
        offsetof(EF_Effect, native_members),
        offsetof(EF_Effect, native_member_count),
        offsetof(EF_Effect, native_member_index),
        sizeof(EF_ModelMember), sizeof(EF_QueuedEffect),
        offsetof(EF_QueuedEffect, native_policy),
        offsetof(EF_QueuedEffect, native_author),
        offsetof(EF_QueuedEffect, native_creator_generation),
        offsetof(EF_QueuedEffect, native_sample_part),
        sizeof(NativeFighterModelEffect),
        offsetof(NativeFighterDefinition, model_effect_count),
        offsetof(NativeFighterDefinition, model_effects),
        offsetof(NativeFighterHost, spawn_effect),
        offsetof(NativeFighterHost, spawn_model_effect),
        offsetof(NativeFighterHost, model_effect_joint),
        offsetof(NativeFighterHost, model_effect_creator),
        offsetof(NativeFighterHost, model_effect_cancel),
        offsetof(NativeFighterHost, model_joint_resolve),
        offsetof(NativeFighterHost, model_joint_read),
        offsetof(NativeFighterHost, model_joint_write),
        sizeof(NativeFighterDefinition), sizeof(NativeItemDefinition),
        offsetof(NativeFighterDefinition, on_form_activated),
        offsetof(NativeFighterHost, activate_partner),
        offsetof(NativeFighterHost, family_member_id),
        offsetof(NativeFighterHost, link_hitlag),
        offsetof(NativeFighterHost, linked_hitlag_id),
        offsetof(NativeFighterHost, request_hitlag_release),
        offsetof(NativeFighterHost, copy_collision_from),
        offsetof(NativeFighterHost, target_bone_position),
        offsetof(NativeFighterHost, target_position),
        offsetof(NativeFighterHost, cpu_nearest_enemy_at),
        offsetof(NativeFighterHost, article_ref_primary_translation),
        offsetof(Fighter, native_hitlag_link_generation),
        offsetof(NativeFighterDefinition, on_cpu_pending_action),
        sizeof(NativeFighterArticleLogic), sizeof(NativeFighterArticleSignal),
        sizeof(NativeFighterAttachmentSpawn),
        offsetof(NativeFighterAttachmentSpawn, previous_at_bone),
        offsetof(NativeFighterAttachmentSpawn, origin_at_center),
        sizeof(NativeFighterPoseGraph),
        offsetof(NativeFighterPoseGraph, name_hash),
        offsetof(NativeFighterPoseGraph, slot),
        offsetof(NativeFighterPoseGraph, path),
        offsetof(NativeFighterDefinition, pose_graph_count),
        offsetof(NativeFighterDefinition, pose_graphs),
        offsetof(NativeFighterHost, pose_graph),
        offsetof(NativeFighterHost, pose_clip),
        offsetof(NativeFighterHost, pose_validate),
        offsetof(NativeFighterHost, load_pose_animation),
        offsetof(NativeFighterHost, request_pose_frame),
        offsetof(NativeFighterHost, copy_blend_pose),
        offsetof(NativeFighterHost, blend_graph_live),
        offsetof(NativeFighterHost, blend_graph_alternate),
        offsetof(NativeFighterHost, blend_pose_to_live),
        offsetof(NativeFighterHost, copy_pose_to_live),
        sizeof(FtCharacterAssetLayout), sizeof(FtCharacterByteRange),
        sizeof(NativeFighterArticleAsset), sizeof(NativeFighterPartOmission),
        offsetof(NativeFighterPartOmission, insertion_mode),
        offsetof(NativeFighterPartOmission, donor_joint),
        offsetof(FtCharacterAssetLayout, article_mask),
        offsetof(FtCharacterAssetLayout, joint_graph_mask),
        offsetof(FtCharacterAssetLayout, item_format),
        offsetof(FtCharacterAssetLayout, byte_range_count),
        offsetof(FtCharacterAssetLayout, byte_ranges),
        offsetof(NativeFighterArticleAsset, kind),
        offsetof(NativeFighterArticleAsset, slot),
        offsetof(NativeFighterArticleAsset, kind_attribute_offset),
        offsetof(NativeFighterDefinition, asset_layout),
        offsetof(NativeFighterDefinition, article_asset_count),
        offsetof(NativeFighterDefinition, article_assets),
        offsetof(NativeFighterDefinition, omission_count),
        offsetof(NativeFighterDefinition, omissions),
        offsetof(NativeFighterDefinition, sound_mute_channels),
        sizeof(((NativeFighterDefinition*) 0)->sound_mute_channels),
        offsetof(NativeFighterDefinition, effect_bank),
        offsetof(NativeFighterDefinition, demo_motion_names),
        sizeof(Fighter_DemoStrings),
        offsetof(Fighter_DemoStrings, result_filename),
        offsetof(Fighter_DemoStrings, intro_filename),
        offsetof(Fighter_DemoStrings, ending_filename),
        offsetof(Fighter_DemoStrings, vi_wait_filename),
        offsetof(NativeFighterHost, item_add_animation_descriptor),
        offsetof(NativeFighterHost, item_read_attachment_matrix_rotation),
        offsetof(NativeFighterHost, check_guard),
        offsetof(NativeFighterHost, check_dash),
        offsetof(NativeFighterHost, check_air_special),
        offsetof(NativeFighterHost, article_ref_exists),
        offsetof(NativeFighterHost, article_ref_float),
        offsetof(NativeFighterHost, article_ref_int),
        offsetof(NativeFighterHost, article_ref_bool),
        offsetof(NativeFighterHost, item_link_id),
        offsetof(NativeFighterHost, item_create_child),
        offsetof(NativeFighterHost, target_call),
        offsetof(NativeFighterHost, target_call_bool),
        offsetof(NativeFighterHost, target_call_int),
        offsetof(NativeFighterHost, target_call_float),
        offsetof(NativeFighterHost, callback_source_alive),
        sizeof(NativeFighterCallbackExport),
        offsetof(NativeFighterCallbackExport, name_hash),
        offsetof(NativeFighterCallbackExport, callback),
        offsetof(NativeFighterCallbackExport, result_type),
        offsetof(NativeFighterDefinition, callback_export_count),
        offsetof(NativeFighterDefinition, callback_exports),
        NativeFighterCommon_HorizontalSmashThreshold,
        NativeFighterCommon_HorizontalSmashWindow,
        NativeFighterCommon_HorizontalSmashExtension,
        NativeFighterCommon_SmashShakeCount,
        sizeof(NativeFighterArticleModelRoot),
        sizeof(Article),
        offsetof(Article, native_special_attribute_source),
        offsetof(Article, native_model_root_count),
        offsetof(Article, native_model_roots),
        offsetof(Item, native_models),
        offsetof(Item, native_model_draw_disabled_mask),
        offsetof(Item, native_removal_in_progress),
        offsetof(NativeFighterArticleLogic, model_root_count),
        offsetof(NativeFighterArticleLogic, model_roots),
        offsetof(NativeFighterHost, item_load_model),
        offsetof(NativeFighterHost, item_add_model_animation),
        UINT32_C(0x4D504154), // Model animation operations take a subtree path.
        offsetof(NativeFighterHost, item_request_model_animation),
        offsetof(NativeFighterHost, item_advance_model_animation),
        offsetof(NativeFighterHost, item_remove_model_animation),
        offsetof(NativeFighterHost, item_set_model_animation_rate),
        offsetof(NativeFighterHost, item_read_joint_matrix_rotation),
        offsetof(NativeFighterHost, item_creator),
        offsetof(NativeFighterHost, item_current_owner),
        offsetof(NativeFighterHost, item_reflector),
        offsetof(NativeFighterHost, target_camera_target_position),
        offsetof(NativeFighterHost, create_absorber),
        offsetof(NativeFighterHost, heal_damage),
        sizeof(NativeFighterArticleModelAnimation),
        offsetof(NativeFighterArticleModelAnimation, components),
        sizeof(NativeFighterArticleAnimationComponent),
        offsetof(NativeFighterArticleAnimationComponent, attribute_offset),
        offsetof(NativeFighterArticleAnimationComponent, array_index),
        offsetof(NativeFighterArticleLogic, model_animation_count),
        offsetof(NativeFighterArticleLogic, model_animations),
        offsetof(Article, native_model_animation_count),
        offsetof(Article, native_model_animations),
        offsetof(NativeFighterHost, item_set_joint_transform),
        offsetof(NativeFighterArticleLogic, attachment),
        offsetof(NativeFighterArticleLogic, standard_render),
        offsetof(NativeFighterArticleLogic, on_created),
        offsetof(NativeFighterArticleLogic, on_out_of_bounds),
        offsetof(NativeFighterArticleLogic, signals),
        sizeof(NativeFighterProjectile),
        offsetof(NativeFighterProjectile, read_values),
        sizeof(NativeFighterProjectileLaunch),
        offsetof(NativeFighterProjectileLaunch, hit_group),
        offsetof(NativeFighterProjectileLaunch, preserve_facing),
        offsetof(NativeFighterArticleLogic, projectile),
        offsetof(NativeItemDefinition, billboard_png),
        offsetof(NativeItemDefinition, hitbox_size),
        sizeof(NativeFighterHost),
        offsetof(NativeFighterHost, item_pause),
        offsetof(NativeFighterHost, item_resume),
        sizeof(NativeFighterStateField),
        offsetof(NativeFighterStateField, offset),
        offsetof(NativeFighterStateField, count),
        offsetof(NativeFighterStateField, type),
        offsetof(NativeFighterStateField, stride),
        offsetof(NativeFighterStateField, array),
        offsetof(NativeFighterDefinition, state_field_count),
        offsetof(NativeFighterDefinition, state_fields),
        offsetof(NativeFighterArticleLogic, local_state_bytes),
        offsetof(NativeFighterArticleLogic, state_field_count),
        offsetof(NativeFighterArticleLogic, state_fields),
        offsetof(NativeFighterDefinition, on_shield_transition),
        offsetof(NativeFighterDefinition, on_forward_smash_enter),
        offsetof(NativeFighterDefinition, on_up_smash_enter),
        offsetof(NativeFighterDefinition, on_down_smash_enter),
        offsetof(NativeFighterDefinition, on_shield_scale),
        offsetof(NativeFighterDefinition, on_shield_alpha),
        offsetof(NativeFighterDefinition, on_spotdodge_enter),
        offsetof(NativeFighterDefinition, on_wait_enter),
        offsetof(NativeFighterDefinition, on_crouch_wait_enter),
        offsetof(NativeFighterDefinition, on_aerial_attack_enter),
        offsetof(NativeFighterDefinition, on_aerial_action_manual),
        offsetof(NativeFighterDefinition, on_aerial_action_automatic),
        offsetof(NativeFighterDefinition, on_grounded_motion_setup),
        offsetof(NativeFighterDefinition, on_capture_wait_setup),
        offsetof(NativeFighterDefinition, on_capture_wait_enter),
        offsetof(NativeFighterDefinition, on_catch_pull_frame),
        offsetof(NativeFighterDefinition, on_catch_allowed),
        offsetof(NativeFighterDefinition, on_catch_start),
        offsetof(NativeFighterDefinition, on_catch_frame),
        offsetof(NativeFighterDefinition, on_catch_exit),
        offsetof(NativeFighterDefinition, on_catch_wait_release),
        offsetof(NativeFighterDefinition, on_catch_pull_anchor),
        offsetof(NativeFighterDefinition, on_catch_pull_ready),
        offsetof(NativeFighterDefinition, on_catch_pull_follow),
        offsetof(NativeFighterDefinition, on_throw_victim_setup),
        offsetof(NativeFighterDefinition, on_air_tether_input),
        offsetof(NativeFighterHost, stage_segment_intersection),
        offsetof(NativeFighterHost, play_global_sound),
        offsetof(NativeFighterHost, check_fastfall),
        offsetof(NativeFighterHost, fastfall),
        offsetof(NativeFighterHost, probe_ledge),
        offsetof(NativeFighterHost, find_ledge_occupant),
        offsetof(NativeFighterHost, enter_cliff_jump_phase2),
        offsetof(NativeFighterHost, initialize_group_material_animation),
        offsetof(NativeFighterHost, current_model_group),
        offsetof(NativeFighterHost, previous_model_group),
        offsetof(NativeFighterHost, install_model_graph),
        offsetof(NativeFighterHost, item_use_category),
        offsetof(NativeFighterHost, cstick_attack_edge),
        offsetof(NativeFighterHost, aerial_attack_motion),
        offsetof(NativeFighterHost, enter_air_gun),
        offsetof(NativeFighterHost, check_air_item_throw),
        offsetof(NativeFighterHost, contact_rebound_allowed),
        offsetof(NativeFighterHost, check_taunt),
        offsetof(NativeFighterHost, check_crouch),
        offsetof(NativeFighterHost, check_turn),
        offsetof(NativeFighterHost, check_walk),
        offsetof(NativeFighterHost, capture_enclose_caught),
        offsetof(NativeFighterHost, constrain_caught),
        offsetof(NativeFighterHost, add_joint_rotation_y),
        offsetof(NativeFighterHost, install_guard),
        offsetof(NativeFighterHost, update_aerial_jump),
        sizeof(Fighter), offsetof(Fighter, native_module_state),
        offsetof(Fighter, capture_settings),
        sizeof(((Fighter*) 0)->capture_settings),
        sizeof(((Fighter*) 0)->capture_enclosure_settings.escape_invincibility_frames),
        offsetof(Fighter, mv.co.capturekoopa.x8),
        offsetof(Fighter, mv.co.capturekoopa.x10),
        sizeof(Item), offsetof(Item, xDD4_itemVar.native_article.state),
        sizeof(((Item*) 0)->xDD4_itemVar.native_article.state),
        offsetof(Item, xDD4_itemVar.native_article.creator_spawn_id),
        offsetof(NativeFighterDefinition, motion_states),
        offsetof(NativeFighterDefinition, special_ground_state),
        offsetof(NativeFighterDefinition, special_air_state),
        offsetof(NativeFighterDefinition, special_transition_flags),
        offsetof(NativeFighterDefinition, state_bytes),
        offsetof(NativeFighterDefinition, on_tick),
        offsetof(NativeFighterDefinition, on_landing_action),
        offsetof(NativeFighterDefinition, on_throw_animation),
        offsetof(NativeFighterDefinition, on_throw_finish),
        offsetof(NativeFighterDefinition, on_throw_victim_motion),
        offsetof(NativeFighterDefinition, on_heavy_pickup_finish),
        offsetof(NativeFighterDefinition, on_item_swing_motion),
        offsetof(NativeFighterDefinition, on_heavy_throw_enter),
        offsetof(NativeFighterDefinition, on_roll_enter),
        offsetof(NativeFighterDefinition, on_heavy_item_damage),
        offsetof(NativeFighterDefinition, on_cpu_mode_update),
        offsetof(NativeFighterDefinition, sound_policy),
        offsetof(NativeFighterDefinition, ignore_surface_friction),
        sizeof(NativeFighterSoundPolicy),
        offsetof(NativeFighterSoundPolicy, rules),
        offsetof(NativeFighterSoundPolicy, ko_cues),
        offsetof(NativeFighterSoundPolicy, rule_count),
        offsetof(NativeFighterSoundPolicy, ko_cue_count),
        sizeof(NativeFighterSoundIdRule),
        offsetof(NativeFighterSoundIdRule, costume_mask),
        offsetof(NativeFighterSoundIdRule, bank),
        offsetof(NativeFighterSoundIdRule, first),
        offsetof(NativeFighterSoundIdRule, last),
        offsetof(NativeFighterSoundIdRule, offset),
        sizeof(NativeFighterKoCue),
        offsetof(NativeFighterKoCue, costume_mask),
        offsetof(NativeFighterKoCue, sound_id),
        NATIVE_FIGHTER_SOUND_RULE_CAPACITY, NATIVE_FIGHTER_KO_CUE_CAPACITY,
        offsetof(NativeFighterDefinition, on_cpu_output),
        offsetof(NativeFighterHost, cpu_record_input),
        offsetof(NativeFighterHost, cpu_read_history_pose),
        offsetof(NativeFighterHost, cpu_apply_recorded_input),
        offsetof(NativeFighterHost, cpu_prepare_combat_plan),
        offsetof(NativeFighterHost, cpu_prepare_destination_update),
        offsetof(NativeFighterHost, cpu_update_recovery_destination),
        offsetof(NativeFighterHost, cpu_plan_attack),
        offsetof(NativeFighterHost, cpu_plan_without_target),
        offsetof(NativeFighterHost, cpu_set_destination),
        offsetof(NativeFighterHost, stage_same_line_island),
        offsetof(NativeFighterHost, stage_cpu_floor_excluded),
        offsetof(NativeFighterHost, stage_cpu_island_blocked),
        offsetof(NativeFighterHost, stage_read_floor),
        offsetof(NativeFighterHost, target_position_delta),
        offsetof(NativeFighterHost, target_damage),
        offsetof(NativeFighterHost, target_facing),
        offsetof(NativeFighterHost, target_sleeping),
        offsetof(NativeFighterHost, target_floor_line),
        offsetof(NativeFighterHost, target_cpu_body_half_width),
        offsetof(NativeFighterDefinition, on_cpu_prepare),
        offsetof(NativeFighterDefinition, on_cpu_drop_command),
        offsetof(NativeFighterDefinition, on_cpu_stick_step),
        offsetof(NativeFighterDefinition, on_cpu_stick_limit),
        offsetof(NativeFighterDefinition, on_cpu_initialize),
        offsetof(NativeFighterDefinition, on_cpu_spacing_update),
        offsetof(NativeFighterDefinition, on_cpu_attack_motion),
        offsetof(NativeFighterDefinition, on_cpu_grab_motion),
        offsetof(NativeFighterDefinition, on_cpu_attack_decision),
        offsetof(NativeFighterDefinition, on_cpu_capture_decision),
        offsetof(NativeFighterDefinition, on_cpu_pummel_special),
        offsetof(NativeFighterDefinition, on_cpu_ranged_candidates),
        offsetof(NativeFighterDefinition, on_cpu_ranged_allowed),
        offsetof(NativeFighterDefinition, on_cpu_special_candidate),
        offsetof(NativeFighterDefinition, on_cpu_projectile_hit),
        offsetof(NativeFighterDefinition, on_presentation_item),
        offsetof(NativeFighterHost, air_accel_from),
        offsetof(NativeFighterHost, animation_velocity_y_adjusted),
        offsetof(NativeFighterDefinition, on_cpu_threat),
        offsetof(NativeFighterDefinition, on_cpu_travel_decision),
        offsetof(NativeFighterDefinition, on_cpu_falling_recovery_allowed),
        offsetof(NativeFighterDefinition, on_cpu_falling_destination_reached),
        offsetof(NativeFighterDefinition, on_action_metadata),
        offsetof(NativeFighterDefinition, on_cpu_available_jump),
        offsetof(NativeFighterDefinition, on_cpu_charge_decision),
        offsetof(NativeFighterDefinition, on_cpu_charge_cancel),
        offsetof(NativeFighterDefinition, items),
        offsetof(NativeFighterDefinition, article_logics),
        offsetof(NativeFighterDefinition, special_n),
        offsetof(NativeFighterDefinition, special_air_lw),
        offsetof(NativeFighterDefinition, on_trail),
        offsetof(NativeFighterDefinition, on_jab_restart),
        offsetof(NativeFighterDefinition, on_shield_start),
        offsetof(NativeFighterDefinition, on_dynamics_wind),
        offsetof(NativeFighterDefinition, on_dynamics_interaction_start),
        offsetof(NativeFighterDefinition, on_aerial_jump),
        offsetof(NativeFighterDefinition, on_dynamics_ground_collision),
        offsetof(NativeFighterDefinition, on_body_rebound),
        offsetof(NativeFighterDefinition, on_worn_item_jump_sound),
        sizeof(NativeFighterTrail),
        offsetof(NativeFighterHost, change_motion_state),
        offsetof(NativeFighterHost, animation_frames_remaining),
        offsetof(NativeFighterHost, air_accel_friction),
        offsetof(NativeFighterHost, follow_floor),
        offsetof(NativeFighterHost, set_hitbox_position),
        offsetof(NativeFighterHost, check_ground_escape),
        offsetof(NativeFighterHost, update_throw),
        offsetof(NativeFighterHost, throw_motion),
        offsetof(NativeFighterHost, enter_walk),
        offsetof(NativeFighterHost, animation_clip),
        offsetof(NativeFighterHost, initialize_hold),
        offsetof(NativeFighterHost, common_attribute),
        offsetof(NativeFighterHost, begin_aerial_jump),
        offsetof(NativeFighterHost, check_heavy_item_throw),
        offsetof(NativeFighterHost, damage_reaction_suppressed),
        offsetof(NativeFighterHost, release_held_item),
        offsetof(NativeFighterHost, cpu_command),
        offsetof(NativeFighterHost, cpu_run_script),
        offsetof(NativeFighterHost, common_motion_states),
        FT_CPU_SCRIPT_COUNT,
        ftCo_MS_Count,
        offsetof(NativeFighterHost, fighters_share_floor),
        offsetof(NativeFighterHost, ground_standard_box_collision),
        offsetof(NativeFighterHost, find_floor_island),
        offsetof(NativeFighterHost, island_bounds),
        offsetof(NativeFighterHost, create_guard),
        offsetof(NativeFighterHost, set_current_visibility_group),
        offsetof(NativeFighterHost, set_body_status),
        offsetof(NativeFighterHost, set_group_material_frame),
        offsetof(NativeFighterHost, bone_count),
        offsetof(NativeFighterHost, spawn_effect),
        offsetof(NativeFighterHost, spawn_holdable_item),
        offsetof(NativeFighterHost, random_int),
        offsetof(NativeFighterHost, random_int_excluding),
        offsetof(NativeFighterHost, spawn_projectile),
        offsetof(NativeFighterHost, spawn_authored_projectile),
        offsetof(NativeFighterHost, spawn_attached_article),
        offsetof(NativeFighterHost, spawn_bone_attached_article),
        offsetof(NativeFighterHost, owned_article_exists),
        offsetof(NativeFighterHost, remove_owned_article),
        offsetof(NativeFighterHost, pause_owned_article),
        offsetof(NativeFighterHost, resume_owned_article),
        offsetof(NativeFighterHost, signal_owned_article),
        offsetof(NativeFighterHost, item_spawn_directional_effect),
        offsetof(NativeFighterHost, notify_item_pickup),
        offsetof(NativeFighterHost, bone_segment_angle),
        offsetof(NativeFighterHost, item_stage_collision),
        offsetof(NativeFighterHost, model_animation_active),
        offsetof(NativeFighterHost, item_sweep_floor),
        offsetof(NativeFighterHost, item_stage_contact_response),
        offsetof(NativeFighterHost, item_spawn_async_joint_effect),
        offsetof(NativeFighterHost, item_wall_contact_flags),
        offsetof(NativeFighterHost, item_ceiling_contact_flags),
        offsetof(NativeFighterHost, item_align_to_floor),
        offsetof(NativeFighterHost, item_reflect_from_wall),
        offsetof(NativeFighterHost, sound_channels),
        NATIVE_FIGHTER_SOUND_CHANNEL_COUNT,
        offsetof(NativeFighterHost, try_wall_jump),
        offsetof(NativeFighterHost, item_bounce_stage),
        offsetof(NativeFighterHost, item_shield_bounce),
        offsetof(NativeFighterHost, item_play_sound),
        offsetof(NativeFighterHost, item_loop_sound),
        offsetof(NativeFighterHost, item_fall),
        offsetof(NativeFighterHost, item_spawn_effect_on_bone),
        offsetof(NativeFighterHost, item_activate_attached),
        offsetof(NativeFighterHost, fighter_model_scale),
        offsetof(NativeFighterHost, fighter_root_scale_y),
        offsetof(NativeFighterHost, fighter_incapacitated),
        offsetof(NativeFighterHost, item_set_uniform_scale),
        offsetof(NativeFighterHost, item_set_child_visible),
        offsetof(NativeFighterHost, item_interact),
        offsetof(NativeFighterHost, sqrt_float),
        offsetof(NativeFighterDefinition, on_demo_enter),
        offsetof(NativeFighterHost, bind_fighter_item),
        offsetof(NativeFighterHost, clear_fighter_item),
        offsetof(NativeFighterHost, held_item_id),
        offsetof(NativeFighterHost, create_engine_item),
        offsetof(NativeFighterHost, attach_created_item),
        offsetof(NativeFighterHost, inherit_created_item_scale),
        offsetof(Item, native_creator),
        offsetof(Item, native_creator_generation),
        offsetof(Item, native_creator_kind),
        offsetof(Item, native_creator_definition),
        offsetof(Item, throw_scale),
        NATIVE_FIGHTER_PARAMETER_CAPACITY,
        sizeof(NativeFighterModule),
        offsetof(NativeFighterModule, parameter_bits),
        offsetof(NativeFighterModule, parameter_count),
        offsetof(NativeFighterHost, item_set_appearance),
        offsetof(NativeFighterHost, item_reset_spin),
        offsetof(NativeFighterHost, item_refresh_physics),
        offsetof(NativeFighterHost, set_item_layered),
        offsetof(NativeFighterHost, item_disable_hitbox),
        offsetof(FtCharacterAssetLayout, visibility_mask),
        offsetof(Article, model_layers),
        offsetof(Item, model_layers),
    };
    uint64_t hash = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i) {
        hash ^= fields[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

// The module may copy the stock base definition and replace selected fields,
// or return a complete independent definition. `base` may be NULL for probes.
bool nativeFighterModule_Open(const char* path,
                              const NativeFighterDefinition* base,
                              NativeFighterModule* module);
// Materialize verified package bytes under their content ID and load them.
// An existing file must match exactly; module files are immutable while loaded.
bool nativeFighterModule_OpenCached(const char* root, const NativeContentId* id,
                                    unsigned member_index,
                                    const uint8_t* bytes, size_t size,
                                    const NativeFighterDefinition* base,
                                    NativeFighterModule* module);
void nativeFighterModule_Close(NativeFighterModule* module);

#ifdef __cplusplus
}
#endif

#endif
#endif
