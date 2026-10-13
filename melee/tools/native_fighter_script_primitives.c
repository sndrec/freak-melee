#include "native_fighter_script_code.h"
#include "native_fighter_script_signal.h"
#include "native_fighter_script_vectors.h"
#include "native_fighter_script_projectile.h"
#include "native_fighter_script_guard.h"
#include "native_fighter_script_jump.h"
#include "native_fighter_script_action_metadata.h"
#include "native_fighter_script_functions.h"
#include "native_fighter_script_kinetics.h"
#include "native_fighter_script_collision.h"
#include "native_fighter_script_smash.h"
#include "native_fighter_script_colors.h"
#include "native_fighter_script_pose.h"
#include "native_fighter_script_sound.h"
#include "native_fighter_script_relations.h"
#include "native_fighter_script_queries.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

const char* nativeScriptPrimitives_Property(const char* name)
{
    const char* relation = nativeScriptRelations_Property(name);
    if (relation != NULL) return relation;
    const char* pose = nativeScriptPose_Constant(name);
    if (pose != NULL) return pose;
    const char* color = nativeScriptColors_Property(name, false);
    if (color != NULL) return color;
    const char* smash = nativeScriptSmash_Property(name);
    if (smash != NULL) return smash;
    const char* guard = nativeScriptGuard_Property(name);
    if (guard != NULL) return guard;
    const char* jump = nativeScriptJump_Property(name);
    if (jump != NULL) return jump;
    static const struct { const char* name; const char* code; } properties[] = {
        { "Hitbox.disabled", "HitCapsule_Disabled" },
        { "engine.debug_level", "(*engine->debug_level)" },
        { "fighter.creation_id", "fighter->x8_spawnNum" },
        { "DebugLevel.master", "DbLKind_Master" },
        { "DebugLevel.no_debug_rom", "DbLKind_NoDebugRom" },
        { "DebugLevel.debug_develop", "DbLKind_DebugDevelop" },
        { "DebugLevel.debug_rom", "DbLKind_DebugRom" },
        { "DebugLevel.develop", "DbLKind_Develop" },
        { "Hitbox.enabled", "HitCapsule_Enabled" },
        { "Axis.x", "1" }, { "Axis.y", "2" }, { "Axis.z", "4" },
        { "controls.interrupt_allowed", "fighter->allow_interrupt" },
        { "damage.song_immune", "fighter->x2222_b2" },
        { "input.jump_held", "((fighter->input.held_buttons[0] & HSD_PAD_XY) != 0)" },
        { "items.held_use_category", "(fighter->item_gobj != NULL ? engine->item_use_category(fighter->item_gobj) : -1)" },
        { "controls.tap_jump_threshold", "engine->common_attribute(NativeFighterCommon_TapJumpThreshold)" },
        { "controls.horizontal_stick_deadzone", "engine->common_attribute(NativeFighterCommon_HorizontalStickDeadzone)" },
        { "controls.stick_smoothing", "engine->common_attribute(NativeFighterCommon_StickSmoothing)" },
        { "controls.aerial_action_down_threshold", "engine->common_attribute(NativeFighterCommon_AerialActionDownThreshold)" },
        { "input.attack_held", "((fighter->input.held_buttons[0] & HSD_PAD_A) != 0)" },
        { "player.slot", "fighter->player_id" },
        { "stage.internal_kind", "engine->stage_internal_kind()" },
        { "player.secondary", "fighter->x221F_b4" },
        { "position.delta.x", "fighter->pos_delta.x" },
        { "position.delta.y", "fighter->pos_delta.y" },
        { "position.delta.z", "fighter->pos_delta.z" },
        { "collision.position.x", "fighter->coll_data.cur_pos.x" },
        { "collision.position.y", "fighter->coll_data.cur_pos.y" },
        { "collision.position.z", "fighter->coll_data.cur_pos.z" },
        { "movement.used_tether", "fighter->used_tether" },
        { "movement.fastfall", "fighter->fall_fast" },
        { "movement.special_landing_lag", "fighter->x2EC" },
        { "Collision.left_wall_mask", "Collide_LeftWallMask" },
        { "Collision.floor_mask", "Collide_FloorMask" },
        { "Collision.wall_mask", "Collide_WallMask" },
        { "Collision.ceiling_mask", "Collide_CeilingMask" },
        { "Collision.right_wall_mask", "Collide_RightWallMask" },
        { "input.down_held", "((fighter->input.held_buttons[0] & HSD_PAD_DPADDOWN) != 0)" },
        { "input.up_held", "((fighter->input.held_buttons[0] & HSD_PAD_DPADUP) != 0)" },
        { "input.z_held", "((fighter->input.held_buttons[0] & HSD_PAD_Z) != 0)" },
        { "input.down_release_age", "fighter->x682" },
        { "items.held_id", "engine->held_item_id(gobj, false)" },
        { "items.reopen_blocked", "fighter->x2221_b5" },
        { "items.auxiliary_id", "engine->held_item_id(gobj, true)" },
        { "items.heavy", "(fighter->item_gobj != NULL && engine->item_is_heavy(fighter->item_gobj))" },
        { "items.hold_kind", "(fighter->item_gobj != NULL ? engine->item_hold_kind(fighter->item_gobj) : 0)" },
        { "reflector.hit_direction", "fighter->ReflectAttr.x1A2C_reflectHitDirection" },
        { "reflector.keep_owner", "fighter->x2218_b4" },
        { "absorber.active", "fighter->x2218_b6" },
        { "absorber.hit", "fighter->x2218_b7" },
        { "absorber.skip_position_update", "fighter->absorb_hit.skip_update_pos" },
        { "absorber.damage", "fighter->AbsorbAttr.x1A44_damageTaken" },
        { "absorber.hits", "fighter->AbsorbAttr.x1A48_hitsTaken" },
        { "absorber.direction", "fighter->AbsorbAttr.x1A40_absorbHitDirection" },
        { "capture.armed", "fighter->x221E_b6" },
        { "visuals.held_item_visible", "fighter->x221E_b3" },
        { "capture.victim_freeze_state", "(fighter->victim_gobj != NULL && ((Fighter*)fighter->victim_gobj->user_data)->x2222_b6)" },
        { "bones.root.rotation_x", "engine->get_bone_rotation_x(fighter, 0)" },
        { "bones.count", "engine->bone_count(fighter)" },
        { "bones.root.rotation_y", "engine->get_bone_rotation_y(fighter, 0)" },
        { "costume.index", "fighter->x619_costume_id" },
        { "costume.variant", "engine->costume_variant(fighter)" },
        { "Transition.skip_attack_count", "Ft_MF_SkipAttackCount" },
        { "Transition.common_ground_air", "ftCommon_GroundAirColl_MF" },
        { "Transition.keep_hurt_status", "Ft_MF_KeepColAnimHitStatus" },
        { "Transition.skip_hitboxes", "Ft_MF_SkipHit" },
        { "Transition.skip_model", "Ft_MF_SkipModel" },
        { "ground.acceleration", "fighter->xE4_ground_accel_1" },
        { "ground.walk_acceleration", "fighter->co_attrs.walk_accel_mul" },
        { "ground.walk_speed", "fighter->co_attrs.walk_max_vel" },
        { "ground.mid_walk_point", "fighter->co_attrs.mid_walk_point" },
        { "air.jump_momentum", "fighter->co_attrs.ground_to_air_jump_momentum_multiplier" },
        { "air.jump_horizontal_speed", "fighter->co_attrs.jump_h_max_velocity" },
        { "air.jump_horizontal_multiplier", "fighter->co_attrs.air_jump_h_multiplier" },
        { "animation.id", "fighter->x5F4_arr[0].idx" },
        { "animation.rate", "fighter->frame_speed_mul" },
        { "motion.fall_animation", "((fighter->motion_id == ftCo_MS_Fall || fighter->motion_id == ftCo_MS_FallAerial) ? fighter->mv.co.fall.smid : -1)" },
        { "animation.hip_root_compensation", "fighter->x2221_b2" },
        { "action.category", "fighter->x2070.x2071_b0_3" },
        { "action.move_id", "fighter->x2070.x2073" },
        { "hurtboxes.status", "engine->body_hit_status(gobj)" },
        { "Hurt.height_mid", "HurtHeight_Mid" },
        { "animation.delta.x", "fighter->x6A4_transNOffset.x" },
        { "animation.delta.y", "fighter->x6A4_transNOffset.y" },
        { "animation.delta.z", "fighter->x6A4_transNOffset.z" },
        { "air.gravity", "fighter->co_attrs.gravity" },
        { "stats.landing_air_neutral_lag", "fighter->co_attrs.landingairn_lag" },
        { "stats.landing_air_forward_lag", "fighter->co_attrs.landingairf_lag" },
        { "stats.landing_air_back_lag", "fighter->co_attrs.landingairb_lag" },
        { "stats.landing_air_up_lag", "fighter->co_attrs.landingairhi_lag" },
        { "stats.landing_air_down_lag", "fighter->co_attrs.landingairlw_lag" },
        { "input.attack_pressed", "(fighter->input.pressed_buttons & HSD_PAD_A)" },
        { "input.shield_pressed", "(fighter->input.pressed_buttons & HSD_PAD_LR)" },
        { "facing.animation_direction", "fighter->facing_dir1" },
        { "input.attack_or_special_pressed", "(fighter->input.pressed_buttons & HSD_PAD_AB)" },
        { "controls.throw_horizontal_threshold", "engine->common_attribute(NativeFighterCommon_ThrowHorizontal)" },
        { "controls.horizontal_smash_threshold", "engine->common_attribute(NativeFighterCommon_HorizontalSmashThreshold)" },
        { "controls.horizontal_smash_window", "engine->common_attribute(NativeFighterCommon_HorizontalSmashWindow)" },
        { "controls.horizontal_smash_extension", "engine->common_attribute(NativeFighterCommon_HorizontalSmashExtension)" },
        { "controls.throw_up_threshold", "engine->common_attribute(NativeFighterCommon_ThrowUp)" },
        { "controls.throw_down_threshold", "engine->common_attribute(NativeFighterCommon_ThrowDown)" },
        { "controls.aerial_jump_reverse_threshold", "engine->common_attribute(NativeFighterCommon_AerialJumpReverse)" },
        { "walk.type", "engine->walk_type(gobj)" },
        { "animation.length", "engine->animation_length(gobj)" },
        { "damage.armor", "fighter->dmg.armor0" },
        { "damage.in_hitstun", "fighter->x221C_b6" },
        { "damage.reaction_suppressed", "engine->damage_reaction_suppressed(fighter)" },
        { "damage.knockback_level", "engine->knockback_level(engine->scale_knockback(fighter->dmg.kb_applied))" },
        { "capture.enclosure_model_slot", "fighter->capture_enclosure_settings.model_slot" },
        { "capture.enclosure_launch_x", "fighter->capture_enclosure_settings.launch_velocity.x" },
        { "capture.enclosure_launch_y", "fighter->capture_enclosure_settings.launch_velocity.y" },
        { "capture.enclosure_launch_z", "fighter->capture_enclosure_settings.launch_velocity.z" },
        { "capture.enclosure_damage_behavior", "fighter->capture_enclosure_settings.damage_behavior" },
        { "capture.enclosure_shrink_fraction", "fighter->capture_enclosure_settings.shrink_fraction" },
        { "capture.enclosure_shrink_frames", "fighter->capture_enclosure_settings.shrink_frames" },
        { "capture.enclosure_lifetime", "fighter->capture_enclosure_settings.lifetime" },
        { "capture.enclosure_damage_time_scale", "fighter->capture_enclosure_settings.damage_time_scale" },
        { "capture.enclosure_escape_x", "fighter->capture_enclosure_settings.escape_velocity.x" },
        { "capture.enclosure_escape_y", "fighter->capture_enclosure_settings.escape_velocity.y" },
        { "capture.enclosure_escape_z", "fighter->capture_enclosure_settings.escape_velocity.z" },
        { "capture.enclosure_escape_invincibility_frames", "fighter->capture_enclosure_settings.escape_invincibility_frames" },
        { "capture.timer_damage_multiplier", "fighter->capture_settings.timer_damage_multiplier" },
        { "capture.timer_decay", "fighter->capture_settings.timer_decay" },
        { "capture.mash_reduction", "fighter->capture_settings.mash_reduction" },
        { "capture.mash_pause_frames", "fighter->capture_settings.mash_pause_frames" },
        { "capture.mash_animation_rate", "fighter->capture_settings.mash_animation_rate" },
        { "capture.wiggle_scale", "fighter->capture_settings.wiggle_scale" },
        { "capture.wiggle_limit", "fighter->capture_settings.wiggle_limit" },
        { "capture.victim_frame", "(fighter->victim_gobj != NULL ? ((Fighter*) fighter->victim_gobj->user_data)->cur_anim_frame : 0)" },
        { "capture.victim_damage", "(fighter->victim_gobj != NULL ? ((Fighter*) fighter->victim_gobj->user_data)->dmg.x1830_percent : 0)" },
        { "input.previous_x", "fighter->input.lstick[1].x" },
        { "input.previous_y", "fighter->input.lstick[1].y" },
        { "capture.has_item", "(fighter->target_item_gobj != NULL)" },
        { "capture.victim_visible", "(fighter->victim_gobj != NULL && !((Fighter*)fighter->victim_gobj->user_data)->invisible)" },
        { "capture.has_victim", "(fighter->victim_gobj != NULL)" },
        { "capture.victim_animation_family", "(fighter->victim_gobj != NULL ? engine->animation_family((Fighter*) fighter->victim_gobj->user_data) : Ft_Kind_None)" },
        { "AnimationFamily.peach", "Ft_Kind_Peach" },
        { "AnimationFamily.none", "Ft_Kind_None" },
        { "AnimationFamily.zelda", "Ft_Kind_Zelda" },
        { "Motion.thrown_down_alternate", "ftCo_MS_ThrownlwWomen" },
        { "floor.line", "fighter->coll_data.floor.index" },
        { "floor.is_platform", "((fighter->coll_data.floor.flags & LINE_FLAG_PLATFORM) != 0)" },
        { "floor.normal.z", "fighter->coll_data.floor.normal.z" },
        { "Hurt.normal", "HurtCapsule_Enabled" },
        { "Hurt.invincible", "HurtCapsule_Disabled" },
        { "Hurt.intangible", "HurtCapsule_Intangible" },
        { "guard.active", "fighter->x221B_b0" },
        { "guard.reactive", "fighter->x221B_b1" },
        { "guard.melee_hitlag", "fighter->shield_unk0" },
        { "guard.article_hitlag", "fighter->shield_unk1" },
        { "guard.hit_damage", "fighter->x19A4" },
        { "guard.hit_facing", "fighter->specialn_facing_dir" },
        { "position.x", "fighter->cur_pos.x" },
        { "position.y", "fighter->cur_pos.y" },
        { "position.z", "fighter->cur_pos.z" },
        { "model.instance_scale_y", "fighter->x34_scale.y" },
        { "air.friction", "fighter->co_attrs.aerial_friction" },
        { "model.base_scale", "fighter->co_attrs.model_scaling" },
        { "model.effect_scale", "fighter->co_attrs.xBC.size" },
        { "model.scale.x", "((HSD_JObj*) gobj->hsd_obj)->scale.x" },
        { "model.scale.y", "((HSD_JObj*) gobj->hsd_obj)->scale.y" },
        { "model.scale.z", "((HSD_JObj*) gobj->hsd_obj)->scale.z" },
        { "ceiling.normal.x", "fighter->coll_data.ceiling.normal.x" },
        { "ceiling.normal.y", "fighter->coll_data.ceiling.normal.y" },
        { "ceiling.normal.z", "fighter->coll_data.ceiling.normal.z" },
        { "wall.right.normal.x", "fighter->coll_data.right_facing_wall.normal.x" },
        { "wall.right.normal.y", "fighter->coll_data.right_facing_wall.normal.y" },
        { "wall.right.normal.z", "fighter->coll_data.right_facing_wall.normal.z" },
        { "collision.floor_contact", "((fighter->coll_data.env_flags & Collide_FloorMask) != 0)" },
        { "collision.ceiling_contact", "((fighter->coll_data.env_flags & Collide_CeilingMask) != 0)" },
        { "floor.friction", "engine->floor_friction(fighter->coll_data.floor.flags)" },
        { "wall.left.normal.x", "fighter->coll_data.left_facing_wall.normal.x" },
        { "wall.left.normal.y", "fighter->coll_data.left_facing_wall.normal.y" },
        { "wall.left.normal.z", "fighter->coll_data.left_facing_wall.normal.z" },
        { "collision.left_wall", "((fighter->coll_data.env_flags & Collide_LeftWallMask) != 0)" },
        { "collision.right_wall", "((fighter->coll_data.env_flags & Collide_RightWallMask) != 0)" },
        { "ecb.right.x", "fighter->coll_data.ecb.right.x" },
        { "ecb.left.x", "fighter->coll_data.ecb.left.x" },
        { "ecb.top.y", "fighter->coll_data.ecb.top.y" },
        { "ecb.bottom.y", "fighter->coll_data.ecb.bottom.y" },
        { "Motion.catch_dash", "ftCo_MS_CatchDash" },
        { "Motion.fall_special", "ftCo_MS_FallSpecial" },
        { "Motion.fall_aerial", "ftCo_MS_FallAerial" },
        { "Motion.held_open", "ftCo_MS_ItemParasolOpen" },
        { "Motion.held_fall", "ftCo_MS_ItemParasolFall" },
        { "Motion.held_fall_special", "ftCo_MS_ItemParasolFallSpecial" },
        { "Motion.up_smash", "ftCo_MS_AttackHi4" },
        { "Motion.throw_forward", "ftCo_MS_ThrowF" },
        { "Motion.throw_back", "ftCo_MS_ThrowB" },
        { "Motion.throw_up", "ftCo_MS_ThrowHi" },
        { "Motion.throw_down", "ftCo_MS_ThrowLw" },
        { "Motion.held_wait", "ftCo_MS_ShoulderedWait" },
        { "Motion.held_turn", "ftCo_MS_ShoulderedTurn" },
        { "Motion.held_throw_forward", "ftCo_MS_ThrownFF" },
        { "Motion.captured_damage_ground", "ftCo_MS_CaptureDamageKoopa" },
        { "Motion.captured_damage_air", "ftCo_MS_CaptureDamageKoopaAir" },
        { "Motion.captured_throw_forward", "ftCo_MS_ThrownKoopaF" },
        { "Motion.captured_throw_back", "ftCo_MS_ThrownKoopaB" },
        { "Motion.captured_throw_air_forward", "ftCo_MS_ThrownKoopaAirF" },
        { "Motion.captured_throw_air_back", "ftCo_MS_ThrownKoopaAirB" },
        { "wrap_signed_angle", "engine->wrap_signed_angle" },
        { "min", "engine->min_float" },
        { "max", "engine->max_float" },
        { "pi", "3.14159265358979323846" },
        { "float", "(float)" },
        { "int", "(int32_t)" },
        { "uint", "(uint32_t)" },
        { "atan2_float", "engine->angle_atan2_float" },
        { "sqrt", "engine->sqrt_float" },
    };
    for (unsigned i = 0; i < sizeof(properties) / sizeof(properties[0]); ++i)
        if (strcmp(name, properties[i].name) == 0) return properties[i].code;
    return nativeScriptCpu_Property(name);
}

// Asset symbols are literals, never executable C supplied by a package.
static bool root_literal(const char* text)
{
    size_t length = strlen(text);
    if (length < 3 || length >= 128 || text[0] != '"' || text[length - 1] != '"')
        return false;
    for (size_t i = 1; i + 1 < length; ++i)
        if (!isalnum((unsigned char) text[i]) && text[i] != '_' &&
            text[i] != '.' && text[i] != '-') return false;
    return true;
}

bool nativeScriptPrimitives_Expression(const ScriptCodeBlock* block,
                                       const char* name, const char** source,
                                       char* output, size_t capacity)
{
    int cpu = nativeScriptCpu_Expression(block, name, source, output, capacity);
    if (cpu) return cpu > 0;
    int query = nativeScriptQueries_Fighter(block, name, source, output, capacity);
    if (query) return query > 0;
    if (nativeScriptRelations_Expression(block, name, source, output, capacity)) return true;
    if (nativeScriptCollision_Expression(block, name, source, output, capacity)) return true;
    if (strcmp(name, "forms.activate_partner") == 0) {
        if (!block || block->item_event || block->model_effect ||
            strncmp(*source, "()", 2) != 0) return false;
        int written = snprintf(output, capacity, "engine->activate_partner(gobj, NULL)");
        if (written < 0 || (size_t)written >= capacity) return false;
        *source += 2;
        return true;
    }
    if (nativeScriptColors_FighterExpression(block, name, source, output, capacity)) return true;
    if (nativeScriptGuard_Expression(block, name, source, output, capacity)) return true;
    if (nativeScriptJump_Expression(block, name, source, output, capacity)) return true;
    if (nativeScriptFunctions_CallbackExpression(block, name, source, output, capacity)) return true;
    if (nativeScriptKinetics_Expression(block, name, source, output, capacity)) return true;
    if (strcmp(name, "model_graphs.install") == 0) {
        if (block == NULL || block->item_event || block->model_effect) return false;
        char args[2][192], values[2][256]; const char* after;
        if (!nativeScriptCode_ParseArguments(*source, args, 2, &after)) return false;
        for (unsigned i = 0; i < 2; ++i)
            if (nativeScriptCode_ExpressionType(block, args[i]) != ScriptType_Int ||
                !nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return false;
        int written = snprintf(output, capacity,
            "({ int32_t graph_slot = (%s); int32_t omission = (%s); engine->install_model_graph(gobj, graph_slot, omission); })",
            values[0], values[1]);
        if (written < 0 || (size_t)written >= capacity) return false;
        *source = after; return true;
    }
    static const struct { const char* name; const char* host; } target_queries[] = {
        { "targets.is_fighter", "target_is_fighter" },
        { "targets.motion", "target_motion" },
        { "targets.damage", "target_damage" },
        { "targets.facing", "target_facing" },
        { "targets.sleeping", "target_sleeping" },
        { "targets.floor_line", "target_floor_line" },
        { "targets.cpu_body_half_width", "target_cpu_body_half_width" },
        { "targets.airborne", "target_airborne" },
        { "targets.smash_charge_active", "target_smash_charge_active" },
        { "targets.held_id", "target_held_id" },
        { "targets.held_kind", "target_held_kind" },
        { "targets.held_motion", "target_held_motion" },
        { "targets.held_frame", "target_held_frame" },
    };
    for (unsigned q=0; q < sizeof(target_queries)/sizeof(target_queries[0]); ++q) {
        if (strcmp(name, target_queries[q].name) != 0) continue;
        char args[1][192], target[512]; const char* after;
        if (!block || block->model_effect ||
            !nativeScriptCode_ParseArguments(*source,args,1,&after) ||
            nativeScriptCode_ExpressionType(block,args[0]) != ScriptType_Int ||
            !nativeScriptCode_Expression(block,args[0],target,sizeof(target))) return false;
        *source=after;
        return snprintf(output,capacity,"engine->%s((uint32_t)(%s))",target_queries[q].host,target) < capacity;
    }
    if (strcmp(name, "attack.enter_jab") == 0) {
        char args[1][192], motion[512]; const char* after;
        if (block == NULL || block->model_effect || block->item_event ||
            !nativeScriptCode_ParseArguments(*source, args, 1, &after) ||
            nativeScriptCode_ExpressionType(block, args[0]) != ScriptType_Int ||
            !nativeScriptCode_Expression(block, args[0], motion, sizeof(motion))) return false;
        int written = snprintf(output, capacity, "engine->enter_jab(gobj, (FtMotionId)(%s))", motion);
        if (written < 0 || (size_t)written >= capacity) return false;
        *source = after;
        return true;
    }
    if (strcmp(name, "items.spawn_food") == 0) {
        if (block == NULL || block->model_effect) return false;
        char args[3][192], values[3][256]; const char* after;
        if (!nativeScriptCode_ParseArguments(*source, args, 3, &after)) return false;
        for (unsigned i = 0; i < 3; ++i) {
            if (!nativeScriptTypes_Assignable(ScriptType_Float,
                    nativeScriptCode_ExpressionType(block, args[i])) ||
                !nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i])))
                return false;
        }
        // Sequence coordinate expressions before the synchronous constructor.
        // Lifetime lowering guards each effectful argument and the returned
        // pointer before any enclosing assignment or later argument executes.
        int written = snprintf(output, capacity,
            "({ Vec3 food_position; food_position.x = (%s); food_position.y = (%s); "
            "food_position.z = (%s); engine->spawn_food(gobj, &food_position) != NULL; })",
            values[0], values[1], values[2]);
        if (written < 0 || (size_t)written >= capacity) return false;
        *source = after;
        return true;
    }
    if (strcmp(name, "action.metadata") == 0) {
        const char* after;
        if (!nativeScriptActionMetadata_Expression(block, *source, output, capacity, &after))
            return false;
        *source = after;
        return true;
    }
    if (strcmp(name, "stage.segment_blocked") == 0 || strcmp(name, "audio.play_global") == 0 ||
        strcmp(name, "collision.probe_ledge") == 0) {
        if (block == NULL || block->model_effect) return false;
        bool stage = strcmp(name, "stage.segment_blocked") == 0;
        bool audio = strcmp(name, "audio.play_global") == 0;
        unsigned count = stage ? 4 : audio ? 3 : 2;
        char args[4][192], values[4][256]; const char* after;
        if (!nativeScriptCode_ParseArguments(*source, args, count, &after)) return false;
        for (unsigned i = 0; i < count; ++i) {
            ScriptExpressionType type = nativeScriptCode_ExpressionType(block, args[i]);
            if (audio ? type != ScriptType_Int : !nativeScriptTypes_Assignable(ScriptType_Float, type)) return false;
            if (!nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return false;
        }
        int written;
        if (stage) written = snprintf(output, capacity,
            "engine->stage_segment_intersection(NULL, NULL, NULL, NULL, -1, -1, %s, %s, %s, %s)",
            values[0], values[1], values[2], values[3]);
        else if (audio) written = snprintf(output, capacity,
            "engine->play_global_sound((enum_t)(%s), (int)(%s), (int)(%s))", values[0], values[1], values[2]);
        else written = snprintf(output, capacity, "engine->probe_ledge(gobj, %s, %s)", values[0], values[1]);
        if (written < 0 || (size_t)written >= capacity) return false;
        *source = after; return true;
    }
    if (strcmp(name, "physics.check_fastfall") == 0 || strcmp(name, "collision.ledge_available") == 0 ||
        strcmp(name, "collision.air") == 0) {
        if (block == NULL || block->model_effect) return false;
        char args[1][192]; const char* after;
        if (!nativeScriptCode_ParseArguments(*source, args, 0, &after)) return false;
        const char* code = strcmp(name, "physics.check_fastfall") == 0 ? "engine->check_fastfall(fighter)" :
            strcmp(name, "collision.ledge_available") == 0 ? "(engine->find_ledge_occupant(gobj) == NULL)" :
            "(engine->air_collision(gobj) != GA_Ground)";
        int written = snprintf(output, capacity, "%s", code);
        if (written < 0 || (size_t)written >= capacity) return false;
        *source = after; return true;
    }
    if (strcmp(name, "items.notify_held_neutral") == 0) {
        if (block == NULL || block->model_effect) return false;
        char args[1][192]; const char* after;
        if (!nativeScriptCode_ParseArguments(*source, args, 0, &after)) return false;
        int written = snprintf(output, capacity, "engine->notify_held_neutral(gobj, NULL)");
        if (written < 0 || (size_t)written >= capacity) return false;
        *source = after; return true;
    }
    if (strcmp(name, "items.create_engine") == 0) {
        if (block == NULL || block->model_effect) return false;
        char args[10][192], values[10][256]; const char* after;
        if (!nativeScriptCode_ParseArguments(*source, args, 10, &after)) return false;
        for (unsigned i = 0; i < 10; ++i) {
            ScriptExpressionType type = nativeScriptCode_ExpressionType(block, args[i]);
            if ((i == 0 || i == 8) ? type != ScriptType_Int :
                !nativeScriptTypes_Assignable(ScriptType_Float, type)) return false;
            if (!nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return false;
        }
        // Evaluate each authored argument once, in source order. Lifetime lowering
        // guards the direct engine call before its result reaches the caller.
        int length = snprintf(output, capacity,
            "({ int32_t kind = (%s); Vec3 pos; pos.x = (%s); pos.y = (%s); pos.z = (%s); "
            "Vec3 vel; vel.x = (%s); vel.y = (%s); vel.z = (%s); "
            "float facing = (%s); uint32_t group = (uint32_t)(%s); float damage = (%s); "
            "engine->create_engine_item(gobj, (ItemKind)kind, &pos, &vel, facing, group, damage); })",
            values[0], values[1], values[2], values[3], values[4], values[5],
            values[6], values[7], values[8], values[9]);
        if (length < 0 || (size_t)length >= capacity) return false;
        *source = after; return true;
    }
    if (strcmp(name, "items.attach") == 0 || strcmp(name, "items.inherit_owner_scale") == 0) {
        if (block == NULL || block->model_effect) return false;
        bool attach = strcmp(name, "items.attach") == 0;
        char args[3][192], values[3][256]; const char* after;
        unsigned count = attach ? 3 : 2;
        if (!nativeScriptCode_ParseArguments(*source, args, count, &after) ||
            nativeScriptCode_ExpressionType(block, args[0]) != ScriptType_Int ||
            nativeScriptCode_ExpressionType(block, args[1]) != (attach ? ScriptType_Int : ScriptType_Bool) ||
            (attach && nativeScriptCode_ExpressionType(block, args[2]) != ScriptType_Bool)) return false;
        for (unsigned i = 0; i < count; ++i)
            if (!nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return false;
        int length = attach ? snprintf(output, capacity,
            "({ uint32_t handle = (uint32_t)(%s); int32_t part = (%s); "
            "bool notify_holder = (%s); engine->attach_created_item(gobj, handle, part, notify_holder); })",
            values[0], values[1], values[2]) : snprintf(output, capacity,
            "({ uint32_t handle = (uint32_t)(%s); %s option = (%s); engine->%s(gobj, handle, option); })",
            values[0], "bool", values[1], "inherit_created_item_scale");
        if (length < 0 || (size_t)length >= capacity) return false;
        *source = after; return true;
    }
    if (strcmp(name, "items.clear_held") == 0 || strcmp(name, "items.clear_auxiliary") == 0) {
        if (block == NULL || block->model_effect) return false;
        char args[1][192]; const char* after;
        if (!nativeScriptCode_ParseArguments(*source, args, 0, &after)) return false;
        int length = snprintf(output, capacity, "engine->clear_fighter_item(gobj, %s)",
            strcmp(name, "items.clear_auxiliary") == 0 ? "true" : "false");
        if (length < 0 || (size_t)length >= capacity) return false;
        *source = after; return true;
    }
    if (strcmp(name, "articles.bind_held") == 0 ||
        strcmp(name, "articles.bind_auxiliary") == 0 ||
        strcmp(name, "items.bind_held") == 0 || strcmp(name, "items.bind_auxiliary") == 0) {
        if (block == NULL || block->model_effect) return false;
        char args[1][192], handle[512]; const char* after;
        if (!nativeScriptCode_ParseArguments(*source, args, 1, &after) ||
            nativeScriptCode_ExpressionType(block, args[0]) != ScriptType_Int ||
            !nativeScriptCode_Expression(block, args[0], handle, sizeof(handle))) return false;
        *source = after;
        return snprintf(output, capacity,
            "engine->bind_fighter_item(gobj, (uint32_t)(%s), %s)", handle,
            (strcmp(name, "articles.bind_auxiliary") == 0 || strcmp(name, "items.bind_auxiliary") == 0) ? "true" : "false") < capacity;
    }
    if (strcmp(name, "items.bind_existing_held") == 0 ||
        strcmp(name, "items.bind_existing_auxiliary") == 0 ||
        strcmp(name, "items.set_visible") == 0 ||
        strcmp(name, "items.set_paused") == 0 ||
        strcmp(name, "items.notify_drop") == 0) {
        if (block == NULL || block->model_effect) return false;
        bool binding = strncmp(name, "items.bind_existing_", 20) == 0;
        char args[2][192], values[2][512]; const char* after;
        unsigned count = binding ? 1 : 2;
        if (!nativeScriptCode_ParseArguments(*source, args, count, &after) ||
            nativeScriptCode_ExpressionType(block, args[0]) != ScriptType_Int ||
            (!binding && nativeScriptCode_ExpressionType(block, args[1]) != ScriptType_Bool))
            return false;
        for (unsigned i = 0; i < count; ++i)
            if (!nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return false;
        const char* host = binding ? "bind_existing_item" :
            strcmp(name, "items.set_visible") == 0 ? "set_item_visible" :
            strcmp(name, "items.set_paused") == 0 ? "set_item_paused" : "notify_item_drop";
        const char* source_arg = binding || strcmp(name, "items.notify_drop") == 0 ? "gobj, " : "";
        const char* option = binding ?
            (strcmp(name, "items.bind_existing_auxiliary") == 0 ? "true" : "false") : "option";
        int written = binding ? snprintf(output, capacity,
            "({ uint32_t handle = (uint32_t)(%s); engine->%s(%shandle, %s); })",
            values[0], host, source_arg, option) : snprintf(output, capacity,
            "({ uint32_t handle = (uint32_t)(%s); bool option = (%s); engine->%s(%shandle, option); })",
            values[0], values[1], host, source_arg);
        if (written < 0 || (size_t)written >= capacity) return false;
        *source = after;
        return true;
    }
    if (strcmp(name, "articles.new_collision_group") == 0) {
        if (strncmp(*source, "()", 2)) return false;
        *source += 2;
        return snprintf(output, capacity, "engine->new_article_collision_group()") < capacity;
    }
    if (strcmp(name, "articles.create_at_position") == 0 || strcmp(name, "articles.create_with_velocity") == 0) {
        const char* after;
        if (!nativeScriptProjectile_CreateExpression(block, *source, strcmp(name, "articles.create_with_velocity") == 0,
                                                      output, capacity, &after)) return false;
        *source = after;
        return true;
    }
    if (strcmp(name, "animation.clip_length") == 0) {
        char* end;
        if (**source != '(') return false;
        const char* start = *source + 1;
        while (isspace((unsigned char) *start)) ++start;
        if (!isdigit((unsigned char) *start)) return false;
        unsigned long action = strtoul(start, &end, 0);
        while (isspace((unsigned char) *end)) ++end;
        if (*end != ')' || action >= 65536) return false;
        *source = end + 1;
        return snprintf(output, capacity,
            "engine->clip_length(engine->animation_clip(fighter, %lu))", action) < capacity;
    }
    if (strcmp(name, "cue.consume") == 0) {
        // Subaction cue bits are one-shot signals. Named convenience cues
        // still select their documented bit; any move may consume another.
        if ((*source)[0] != '(' || (*source)[1] < '0' ||
            (*source)[1] > '7' || (*source)[2] != ')') return false;
        char bit = (*source)[1];
        *source += 3;
        return snprintf(output, capacity,
            "(fighter->throw_flags_b%c ? (fighter->throw_flags_b%c = 0, 1) : 0)",
            bit, bit) < capacity;
    }
    if (strcmp(name, "bones.index") == 0) {
        char args[1][192]; const char* after;
        if (!nativeScriptCode_ParseArguments(*source, args, 1, &after)) return false;
        const char* bone = nativeScriptCode_BoneSymbol(args[0], strlen(args[0]));
        if (bone == NULL) return false;
        *source = after;
        return snprintf(output, capacity, "engine->resolve_bone(fighter, %s)", bone) < capacity;
    }
    static const struct { const char* name; const char* code; } aerial_queries[] = {
        { "input.cstick_attack_edge", "engine->cstick_attack_edge(fighter)" },
        { "controls.aerial_attack_motion", "engine->aerial_attack_motion(fighter)" },
        { "controls.try_air_item_throw", "engine->check_air_item_throw(gobj, NULL, NULL)" },
        { "collision.contact_rebound_allowed", "engine->contact_rebound_allowed(gobj, &callback_result)" },
    };
    for (unsigned i = 0; i < sizeof(aerial_queries) / sizeof(aerial_queries[0]); ++i) {
        if (strcmp(name, aerial_queries[i].name) != 0) continue;
        if (block->model_effect || strncmp(*source, "()", 2) != 0) return false;
        *source += 2;
        return snprintf(output, capacity, "%s", aerial_queries[i].code) < capacity;
    }
    static const struct { const char* name; const char* member; } controls[] = {
        { "controls.turn_requested", "check_turn_input" },
        { "controls.try_taunt", "check_taunt" },
        { "controls.try_crouch", "check_crouch" },
        { "controls.try_turn", "check_turn" },
        { "controls.try_walk", "check_walk" },
        { "controls.try_ground_jump", "check_ground_jump" },
        { "controls.try_roll", "check_ground_escape" },
        { "controls.try_air_jump", "check_air_jump" },
        { "controls.platform_drop_requested", "check_platform_drop" },
        { "controls.walk_requested", "walk_requested" },
        { "controls.walk_stop_requested", "walk_stop_requested" },
        { "controls.jump_input", "jump_input" },
        { "controls.try_heavy_item_throw", "check_heavy_item_throw" },
        { "controls.try_side_special", "check_side_special" },
        { "controls.try_up_special", "check_up_special" },
        { "controls.try_neutral_special", "check_neutral_special" },
        { "controls.try_down_special", "check_down_special" },
        { "controls.try_catch", "check_catch" },
        { "controls.try_side_smash", "check_side_smash" },
        { "controls.try_up_smash", "check_up_smash" },
        { "controls.try_down_smash", "check_down_smash" },
        { "controls.try_side_tilt", "check_side_tilt" },
        { "controls.try_up_tilt", "check_up_tilt" },
        { "controls.try_down_tilt", "check_down_tilt" },
        { "controls.try_jab", "check_jab" },
        { "items.try_ground_pickup", "try_ground_pickup" },
        { "controls.try_spotdodge", "check_ground_roll" },
        { "controls.try_guard_spotdodge", "check_guard_spotdodge" },
        { "controls.try_guard", "check_guard" },
        { "controls.try_dash", "check_dash" },
        { "controls.try_air_special", "check_air_special" },
    };
    for (unsigned i = 0; i < sizeof(controls) / sizeof(controls[0]); ++i) {
        if (strcmp(name, controls[i].name) != 0) continue;
        if (strncmp(*source, "()", 2) != 0) return false;
        *source += 2;
        return snprintf(output, capacity, "engine->%s(gobj)", controls[i].member) < capacity;
    }
    if (strcmp(name, "action.flag") == 0) {
        if (**source != '(') return false;
        char* end; unsigned long bit = strtoul(*source + 1, &end, 10);
        if (end == *source + 1 || *end != ')' || bit < 4 || bit > 7) return false;
        *source = end + 1;
        return snprintf(output, capacity, "fighter->x2070.x2071_b%lu", bit) < capacity;
    }
    if (strcmp(name, "attachments.has_root") == 0) {
        char args[1][192]; const char* after;
        if (!nativeScriptCode_ParseArguments(*source, args, 1, &after) ||
            !root_literal(args[0])) return false;
        *source = after;
        return snprintf(output, capacity, "engine->has_model_root(gobj, %s)",
                        args[0]) < capacity;
    }
    if (strcmp(name, "hitbox.enabled") == 0 || strcmp(name, "hitbox.state") == 0 || strcmp(name, "hitbox.group") == 0 ||
        strcmp(name, "hitbox.damage") == 0) {
        char* end;
        if (**source != '(') return false;
        unsigned long index = strtoul(*source + 1, &end, 0);
        if (end == *source + 1 || *end != ')' || index >= 4) return false;
        *source = end + 1;
        return snprintf(output, capacity, strcmp(name, "hitbox.damage") == 0
            ? "fighter->x914[%lu].damage" : strcmp(name, "hitbox.group") == 0
            ? "fighter->x914[%lu].x4"
            : strcmp(name, "hitbox.state") == 0 ? "fighter->x914[%lu].state"
            : "(fighter->x914[%lu].state != HitCapsule_Disabled)", index) < capacity;
    }
    if (strcmp(name, "articles.spawn") == 0) {
        char args[2][192], values[2][128]; const char* after;
        if (!nativeScriptCode_ParseArguments(*source, args, 2, &after)) return false;
        for (unsigned i = 0; i < 2; ++i)
            if (!nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return false;
        *source = after;
        return snprintf(output, capacity, "engine->spawn_article(gobj, %s, %s)", values[0], values[1]) < capacity;
    }
    if (strcmp(name, "stage.event") == 0) {
        char args[2][192], value[384]; const char* after;
        if (!nativeScriptCode_ParseArguments(*source, args, 2, &after)) return false;
        size_t size = strlen(args[0]);
        if (size < 3 || size > 63 || args[0][0] != '"' || args[0][size - 1] != '"') return false;
        args[0][size - 1] = 0;
        for (size_t i = 1; i < size - 1; ++i)
            if (!isalnum((unsigned char) args[0][i]) && args[0][i] != '_' && args[0][i] != '.') return false;
        if (!nativeScriptCode_Expression(block, args[1], value, sizeof(value))) return false;
        *source = after;
        return snprintf(output, capacity, "engine->stage_event(UINT32_C(%u), %s)",
            nativeScriptSignal_Hash(args[0] + 1), value) < capacity;
    }
    if (strcmp(name, "stats.action_count") == 0) {
        char args[2][192], values[2][384]; const char* after;
        if (!nativeScriptCode_ParseArguments(*source, args, 2, &after)) return false;
        for (unsigned i = 0; i < 2; ++i)
            if (!nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return false;
        *source = after;
        return snprintf(output, capacity,
            "engine->action_count(engine->get_action_stats(%s), %s)", values[0], values[1]) < capacity;
    }
    if (strcmp(name, "collision.skip_soft_floor") == 0) {
        if (strncmp(*source, "()", 2) != 0) return false;
        *source += 2;
        return snprintf(output, capacity, "engine->skip_soft_floor(gobj)") < capacity;
    }
    if (strncmp(name, "vectors.cross_normalized_", sizeof("vectors.cross_normalized_")-1) == 0) {
        char args[6][192], values[6][128]; const char* after;
        if (!nativeScriptCode_ParseArguments(*source, args, 6, &after)) return false;
        for (unsigned i=0;i<6;++i)
            if (!nativeScriptCode_Expression(block,args[i],values[i],sizeof(values[i]))) return false;
        if (nativeScriptVectors_CrossComponent(name,values,output,capacity) != 1) return false;
        *source=after; return true;
    }
    if (strcmp(name, "vectors.angle_xy") == 0) {
        char args[4][192], values[4][128]; const char* after;
        if (!nativeScriptCode_ParseArguments(*source, args, 4, &after)) return false;
        for (unsigned i = 0; i < 4; ++i)
            if (!nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return false;
        *source = after;
        return snprintf(output, capacity,
            "engine->vector_angle_xy(&(Vec3){ %s, %s, 0 }, &(Vec3){ %s, %s, 0 })",
            values[0], values[1], values[2], values[3]) < capacity;
    }
    if (strcmp(name, "bones.segment_angle") == 0) {
        char args[7][192], values[6][128]; const char* after;
        if (!nativeScriptCode_ParseArguments(*source, args, 7, &after)) return false;
        const char* bone = nativeScriptCode_BoneSymbol(args[0], strlen(args[0]));
        if (bone == NULL) return false;
        for (unsigned i = 0; i < 6; ++i)
            if (!nativeScriptCode_Expression(block, args[i + 1], values[i], sizeof(values[i])))
                return false;
        *source = after;
        return snprintf(output, capacity,
            "engine->bone_segment_angle(gobj, %s, &(Vec3){ %s, %s, %s }, &(Vec3){ %s, %s, %s })",
            bone, values[0], values[1], values[2], values[3], values[4], values[5]) < capacity;
    }
    bool edge = strcmp(name, "collision.ground_box_edges") == 0;
    bool ledge = strcmp(name, "collision.air_box_ledge") == 0;
    if (!edge && !ledge) return false;
    char args[7][192], values[7][128];
    const char* after;
    unsigned count = ledge ? 7 : 6;
    if (!nativeScriptCode_ParseArguments(*source, args, count, &after)) return false;
    for (unsigned i = 0; i < count; ++i)
        if (!nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i])))
            return false;
    int length = snprintf(output, capacity,
        "engine->%s(gobj, %s%s&(ftCollisionBox){ %s, %s, { %s, %s }, { %s, %s } })",
        edge ? "ground_box_edges" : "air_box_ledge",
        ledge ? values[0] : "", ledge ? ", " : "",
        values[ledge], values[ledge + 1], values[ledge + 2],
        values[ledge + 3], values[ledge + 4], values[ledge + 5]);
    if (length < 0 || length >= capacity) return false;
    *source = after;
    return true;
}

bool nativeScriptPrimitives_Action(ScriptCodeBlock* block, const char* source,
                                    char* output, size_t capacity)
{
    int query = nativeScriptQueries_Action(block, source, output, capacity);
    if (query) return query > 0;
    if (nativeScriptRelations_Action(block, source, output, capacity)) return true;
    static const char* expression_methods[] = {
        "forms.activate_partner",
        "items.create_engine", "items.attach", "items.inherit_owner_scale",
        "items.bind_held", "items.bind_auxiliary", "items.clear_held", "items.clear_auxiliary",
        "items.bind_existing_held", "items.bind_existing_auxiliary",
        "items.set_visible", "items.set_paused", "items.notify_drop", "items.notify_held_neutral",
        "stage.segment_blocked", "stage.sweep_box", "audio.play_global", "collision.probe_ledge",
        "physics.check_fastfall", "collision.ledge_available", "collision.air", "model_graphs.install",
        "input.cstick_attack_edge", "controls.aerial_attack_motion",
        "controls.try_air_item_throw", "collision.contact_rebound_allowed"
    };
    for (unsigned i = 0; i < sizeof(expression_methods) / sizeof(expression_methods[0]); ++i) {
        size_t length = strlen(expression_methods[i]);
        if (strncmp(source, expression_methods[i], length) != 0 || source[length] != '(') continue;
        char expression[2048]; const char* arguments = source + length;
        if (!nativeScriptPrimitives_Expression(block, expression_methods[i], &arguments,
                expression, sizeof(expression)) || *arguments) return false;
        int written = snprintf(output, capacity, "%s;", expression);
        return written >= 0 && (size_t)written < capacity;
    }

    int sound = nativeScriptSound_Action(block, source, output, capacity);
    if (sound != 0) return sound > 0;
    if (nativeScriptCollision_Action(block, source, output, capacity)) return true;
    if (nativeScriptVectors_Action(block, source, output, capacity)) return true;
    if (nativeScriptSmash_Action(block, source, output, capacity)) return true;
    if (nativeScriptGuard_Action(block, source, output, capacity)) return true;
    if (nativeScriptJump_Action(block, source, output, capacity)) return true;
    if (nativeScriptKinetics_Action(block, source, output, capacity)) return true;
    static const struct { const char* name; const char* field; ScriptExpressionType type; } status_fields[] = {
        { "controls.interrupt_allowed", "allow_interrupt", ScriptType_Bool },
        { "damage.song_immune", "x2222_b2", ScriptType_Bool },
    };
    for (unsigned i=0; i<sizeof(status_fields)/sizeof(status_fields[0]); ++i) {
        size_t length=strlen(status_fields[i].name);
        if (strncmp(source,status_fields[i].name,length) || strncmp(source+length," = ",3)) continue;
        if (block->model_effect || nativeScriptCode_ExpressionType(block,source+length+3)!=status_fields[i].type) return false;
        char value[512];
        if (!nativeScriptCode_Expression(block,source+length+3,value,sizeof(value))) return false;
        int written=snprintf(output,capacity,"fighter->%s = %s;",status_fields[i].field,value);
        return written>=0 && (size_t)written<capacity;
    }
    static const char wind[] = "effects.wind(";
    if (strncmp(source, wind, sizeof(wind) - 1) == 0) {
        if (block->item_event) return false;
        char arguments[6][192], values[5][384];
        const char* after;
        if (!nativeScriptCode_ParseArguments(source + sizeof(wind) - 2,
                arguments, 6, &after) || *after) return false;
        const char* bone = nativeScriptCode_BoneSymbol(arguments[0], strlen(arguments[0]));
        if (bone == NULL || !nativeScriptTypes_Assignable(ScriptType_Int,
                nativeScriptCode_ExpressionType(block, arguments[2]))) return false;
        for (unsigned i = 0; i < 5; ++i)
            if (!nativeScriptCode_Expression(block, arguments[i + 1],
                    values[i], sizeof(values[i]))) return false;
        // Own the argument evaluation order; each expression runs once.
        int length = snprintf(output, capacity,
            "{ Fighter_Part part = engine->resolve_bone(fighter, %s); "
            "float wind_offset = (%s); int32_t wind_frames = (%s); "
            "float wind_strength = (%s), wind_decay = (%s), wind_frequency = (%s); "
            "Vec3 pos; engine->joint_position(fighter->parts[part].joint, 0, &pos); "
            "pos.x += wind_offset * fighter->facing_dir; "
            "engine->spawn_wind(&pos, wind_frames, wind_strength, wind_decay, wind_frequency); }",
            bone, values[0], values[1], values[2], values[3], values[4]);
        return length >= 0 && (size_t) length < capacity;
    }
    static const char motion_id[] = "motion.id = ";
    if (strncmp(source, motion_id, sizeof(motion_id) - 1) == 0) {
        const char* expression = source + sizeof(motion_id) - 1;
        char value[512];
        if (block->model_effect || nativeScriptCode_ExpressionType(block, expression) != ScriptType_Int ||
            !nativeScriptCode_Expression(block, expression, value, sizeof(value))) return false;
        return snprintf(output, capacity,
            "{ int32_t published_motion = (%s); "
            "if (published_motion >= 0 && (published_motion < ftCo_MS_Count || "
            "(published_motion >= generated.motion_state_first && "
            "(uint32_t)(published_motion - generated.motion_state_first) < generated.motion_state_count))) "
            "fighter->motion_id = published_motion; }", value) < capacity;
    }
    static const char fall_animation[] = "motion.fall_animation = ";
    if (strncmp(source, fall_animation, sizeof(fall_animation) - 1) == 0) {
        char value[512];
        if (!nativeScriptCode_Expression(block, source + sizeof(fall_animation) - 1,
                                         value, sizeof(value))) return false;
        return snprintf(output, capacity,
            "if (fighter->motion_id == ftCo_MS_Fall || fighter->motion_id == ftCo_MS_FallAerial) fighter->mv.co.fall.smid = (%s);",
            value) < capacity;
    }
    static const char link[] = "articles.link_owned(";
    if (strncmp(source, link, sizeof(link) - 1) == 0) {
        char args[3][192], values[3][192]; const char* after;
        if (!nativeScriptCode_ParseArguments(source + sizeof(link) - 2, args, 3, &after) || *after) return false;
        for (unsigned i = 0; i < 3; ++i)
            if (!nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return false;
        return snprintf(output, capacity, "engine->link_owned_articles(gobj, (s32)(%s), (unsigned)(%s), (s32)(%s));",
                        values[0], values[1], values[2]) < capacity;
    }
    static const char attached_effect[] = "effects.spawn_async_attached(";
    if (strncmp(source, attached_effect, sizeof(attached_effect) - 1) == 0) {
        char args[2][192], value[192], direct[384]; const char* after;
        if (!nativeScriptCode_ParseArguments(source + sizeof(attached_effect) - 2, args, 2, &after) || *after) return false;
        const char* bone = nativeScriptCode_BoneSymbol(args[1], strlen(args[1]));
        if (!nativeScriptCode_Expression(block, args[0], value, sizeof(value))) return false;
        if (bone != NULL)
            return snprintf(output, capacity,
                "engine->spawn_async_effect(gobj, &fighter->x60C, 0, %s, fighter->parts[engine->resolve_bone(fighter, %s)].joint);",
                value, bone) < capacity;
        if (!nativeScriptCode_Expression(block, args[1], direct, sizeof(direct))) return false;
        return snprintf(output, capacity,
            "{ int32_t bone = (int32_t)(%s); if (bone >= 0 && (unsigned)bone < engine->bone_count(fighter) && fighter->parts[bone].joint != NULL) "
            "engine->spawn_async_effect(gobj, &fighter->x60C, 0, %s, fighter->parts[bone].joint); }",
            direct, value) < capacity;
    }
    static const char drift_from[] = "physics.air_accel_from(";
    if (strncmp(source, drift_from, sizeof(drift_from) - 1) == 0) {
        char argument[1][192], value[512]; const char* after;
        if (!nativeScriptCode_ParseArguments(source + sizeof(drift_from) - 2, argument, 1, &after) || *after ||
            !nativeScriptCode_Expression(block, argument[0], value, sizeof(value))) return false;
        int length = snprintf(output, capacity, "engine->air_accel_from(fighter, %s);", value);
        return length >= 0 && (size_t)length < capacity;
    }
    static const struct { const char* name; const char* code; } simple[] = {
        { "callbacks.track_death1()", "fighter->death1_cb = generated.on_death1;" },
        { "callbacks.clear_death1()", "fighter->death1_cb = 0;" },
        { "stats.notify_taunt()", "engine->notify_taunt(fighter->player_id, fighter->x221F_b4);" },
        { "callbacks.track_reflect_hit()", "fighter->reflect_hit_cb = authored_reflect_hit;" },
        { "callbacks.clear_reflect_hit()", "fighter->reflect_hit_cb = 0;" },
        { "capture.release_into_enclosure()", "engine->capture_release_into_enclosure(gobj);" },
        { "capture.release_damage_fall()", "if (fighter->victim_gobj != NULL) { Fighter_GObj* victim = fighter->victim_gobj; engine->apply_throw_damage(gobj, victim); engine->enter_damage_fall(victim); }" },
        { "physics.animation_vertical_velocity()", "engine->animation_velocity_y(gobj);" },
        { "physics.animation_vertical_velocity_adjusted()", "engine->animation_velocity_y_adjusted(gobj);" },
        { "damage.reset_callbacks()", "engine->reset_damage_callbacks(gobj);" },
        { "state.air_gun()", "engine->enter_air_gun(gobj);" },
        { "state.crouch_wait()", "engine->enter_crouch_wait(gobj);" },
        { "physics.ground_to_air()", "engine->ground_to_air(fighter);" },
        { "physics.clear_kinetics()", "engine->clear_kinetics(gobj);" },
        { "physics.air_decel_normal()", "engine->air_decel_normal(fighter);" },
        { "physics.fastfall()", "engine->fastfall(fighter);" },
        { "state.damage_fall()", "engine->enter_damage_fall(gobj);" },
        { "physics.air_accel_normal()", "engine->air_accel_normal(fighter);" },
        { "facing.from_input_threshold()", "engine->update_facing_threshold(fighter);" },
        { "animation.clear_motion_samples()", "fighter->x6D8 = (Vec3){0}; fighter->x6C0 = (Vec3){0}; fighter->lstick_angle = 0; fighter->x6A4_transNOffset = (Vec3){0}; fighter->x68C_transNPos = (Vec3){0};" },
        { "physics.ground_animation_velocity()", "engine->ground_animation_motion(gobj);" },
        { "state.finish()", "engine->finish_action(gobj);" },
        { "throws.update()", "engine->update_throw(gobj);" },
        { "physics.throw_motion()", "engine->throw_motion(gobj);" },
        { "walk.update_animation()", "engine->update_walk_animation(gobj);" },
        { "physics.walk()", "engine->walk_physics(gobj);" },
        { "jump.check_short_hop()", "engine->check_short_hop(gobj);" },
        { "physics.jump_motion()", "engine->jump_physics(gobj);" },
        { "damage.update_timer()", "engine->update_damage_timer(gobj);" },
        { "physics.damage_motion()", "engine->damage_physics(gobj);" },
        { "controls.try_heavy_item_throw()", "engine->check_heavy_item_throw(gobj);" },
        { "physics.ground_animation_facing_motion()", "engine->ground_animation_facing_motion(gobj);" },
        { "capture.drop_victim()", "if (fighter->victim_gobj != NULL) { Fighter_GObj* victim = fighter->victim_gobj; engine->unlink_capture(gobj, victim); engine->enter_fall(victim); }" },
        { "capture.sync_victim_facing()", "if (fighter->victim_gobj != NULL) ((Fighter*) fighter->victim_gobj->user_data)->facing_dir = fighter->facing_dir;" },
        { "attachments.clear_all()", "engine->clear_model_attachments(gobj);" },
        { "hitbox.clear_all()", "engine->clear_hitboxes(gobj);" },
        { "color.clear_attack()", "engine->clear_attack_color(fighter);" },
        { "callbacks.track_reflect()", "fighter->x21F8 = authored_reflect;" },
        { "callbacks.clear_deal_damage()", "fighter->deal_dmg_cb = 0;" },
        { "stats.refresh_attack()", "engine->refresh_attack(gobj);" },
        { "stats.record_attack()", "engine->record_attack(gobj);" },
        { "guard.update_position()", "fighter->shield_hit.skip_update_pos = false;" },
        { "callbacks.clear_hitlag()", "fighter->pre_hitlag_cb = 0; fighter->post_hitlag_cb = 0;" },
        { "collision.unlock_ecb()", "engine->unlock_ecb(fighter);" },
        { "callbacks.track_secondary_damage()", "fighter->take_dmg_2_cb = authored_secondary_damage;" },
        { "callbacks.clear_secondary_damage()", "fighter->take_dmg_2_cb = 0;" },
    };
    for (unsigned i = 0; i < sizeof(simple) / sizeof(simple[0]); ++i)
        if (strcmp(source, simple[i].name) == 0)
            return snprintf(output, capacity, "%s", simple[i].code) < capacity;

    if (strncmp(source, "items.reopen_blocked = ", 23) == 0) {
        char value[512];
        if (!nativeScriptTypes_Assignable(ScriptType_Bool, nativeScriptCode_ExpressionType(block, source + 23)) ||
            !nativeScriptCode_Expression(block, source + 23, value, sizeof(value))) return false;
        return snprintf(output, capacity, "fighter->x2221_b5 = %s;", value) < capacity;
    }
    static const struct { const char* name; const char* field; } guard_fields[] = {
        { "guard.active", "x221B_b0" },
        { "reflector.keep_owner", "x2218_b4" },
        { "absorber.active", "x2218_b6" },
        { "absorber.hit", "x2218_b7" },
        { "absorber.skip_position_update", "absorb_hit.skip_update_pos" },
        { "capture.armed", "x221E_b6" },
        { "guard.reactive", "x221B_b1" },
        { "guard.melee_hitlag", "shield_unk0" },
        { "guard.article_hitlag", "shield_unk1" },
        { "damage.armor", "dmg.armor0" },
        { "damage.reverse_knockdown_pose", "x2226_b1" },
        { "capture.enclosure_model_slot", "capture_enclosure_settings.model_slot" },
        { "capture.enclosure_launch_x", "capture_enclosure_settings.launch_velocity.x" },
        { "capture.enclosure_launch_y", "capture_enclosure_settings.launch_velocity.y" },
        { "capture.enclosure_launch_z", "capture_enclosure_settings.launch_velocity.z" },
        { "capture.enclosure_damage_behavior", "capture_enclosure_settings.damage_behavior" },
        { "capture.enclosure_shrink_fraction", "capture_enclosure_settings.shrink_fraction" },
        { "capture.enclosure_shrink_frames", "capture_enclosure_settings.shrink_frames" },
        { "capture.enclosure_lifetime", "capture_enclosure_settings.lifetime" },
        { "capture.enclosure_damage_time_scale", "capture_enclosure_settings.damage_time_scale" },
        { "capture.enclosure_escape_x", "capture_enclosure_settings.escape_velocity.x" },
        { "capture.enclosure_escape_y", "capture_enclosure_settings.escape_velocity.y" },
        { "capture.enclosure_escape_z", "capture_enclosure_settings.escape_velocity.z" },
        { "capture.enclosure_escape_invincibility_frames", "capture_enclosure_settings.escape_invincibility_frames" },
        { "capture.timer_damage_multiplier", "capture_settings.timer_damage_multiplier" },
        { "capture.timer_decay", "capture_settings.timer_decay" },
        { "capture.mash_reduction", "capture_settings.mash_reduction" },
        { "capture.mash_pause_frames", "capture_settings.mash_pause_frames" },
        { "capture.mash_animation_rate", "capture_settings.mash_animation_rate" },
        { "capture.wiggle_scale", "capture_settings.wiggle_scale" },
        { "capture.wiggle_limit", "capture_settings.wiggle_limit" },

        { "animation.delta.x", "x6A4_transNOffset.x" },
        { "animation.hip_root_compensation", "x2221_b2" },
        { "animation.delta.y", "x6A4_transNOffset.y" },
        { "animation.delta.z", "x6A4_transNOffset.z" },
    };
    for (unsigned i = 0; i < sizeof(guard_fields) / sizeof(guard_fields[0]); ++i) {
        size_t length = strlen(guard_fields[i].name);
        if (strncmp(source, guard_fields[i].name, length) != 0 ||
            strncmp(source + length, " = ", 3) != 0) continue;
        char value[512];
        if (!nativeScriptCode_Expression(block, source + length + 3, value, sizeof(value))) return false;
        return snprintf(output, capacity, "fighter->%s = %s;", guard_fields[i].field, value) < capacity;
    }
    if (strncmp(source, "capture.victim_visible = ", 25) == 0) {
        char value[512];
        if (!nativeScriptCode_Expression(block, source + 25, value, sizeof(value))) return false;
        return snprintf(output, capacity,
            "if (fighter->victim_gobj != NULL) ((Fighter*)fighter->victim_gobj->user_data)->invisible = !(%s);", value) < capacity;
    }
    if (strncmp(source, "capture.set_victim_body_status(", 31) == 0) {
        char args[1][192], value[512]; const char* after;
        if (!nativeScriptCode_ParseArguments(source + 30, args, 1, &after) || *after ||
            !nativeScriptCode_Expression(block, args[0], value, sizeof(value))) return false;
        return snprintf(output, capacity,
            "if (fighter->victim_gobj != NULL) engine->set_body_status(fighter->victim_gobj, %s);", value) < capacity;
    }
    if (strncmp(source, "capture.attach_item(", 20) == 0 ||
        strncmp(source, "capture.release_item(", 21) == 0) {
        bool release = source[8] == 'r';
        char args[2][192], bone[512], clear[512]; const char* after;
        if (!nativeScriptCode_ParseArguments(strchr(source, '('), args, release ? 2 : 1, &after) || *after ||
            !nativeScriptCode_Expression(block, args[0], bone, sizeof(bone)) ||
            (release && !nativeScriptCode_Expression(block, args[1], clear, sizeof(clear)))) return false;
        if (release)
            return snprintf(output, capacity,
                "if (fighter->target_item_gobj != NULL) { unsigned bone = %s; if (bone < engine->bone_count(fighter)) { "
                "engine->capture_release_item(fighter->target_item_gobj, bone, %s); } }", bone, clear) < capacity;
        return snprintf(output, capacity,
            "if (fighter->target_item_gobj != NULL) { unsigned bone = %s; if (bone < engine->bone_count(fighter)) "
            "engine->capture_attach_item(fighter->target_item_gobj, gobj, bone); }", bone) < capacity;
    }
    if (strncmp(source, "absorber.create(", 16) == 0) {
        char args[5][192], values[5][384]; const char* after;
        if (!nativeScriptCode_ParseArguments(source + 15, args, 5, &after) || *after) return false;
        for (unsigned i = 0; i < 5; ++i)
            if (!nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return false;
        return snprintf(output, capacity,
            "{ int32_t bone = (int32_t)(%s); if (bone >= 0 && (unsigned)bone < engine->bone_count(fighter)) "
            "engine->create_absorber(gobj, &(AbsorbDesc){ bone, { %s, %s, %s }, %s }); }",
            values[0], values[1], values[2], values[3], values[4]) < capacity;
    }
    if (strncmp(source, "guard.create(", 13) == 0) {
        char args[6][192], values[5][128], callback[64]; const char* after;
        if (!block || block->model_effect || block->item_event ||
            !nativeScriptCode_ParseArguments(source + 12, args, 6, &after) || *after ||
            nativeScriptCode_ExpressionType(block,args[0])!=ScriptType_Int ||
            !nativeScriptFunctions_CallbackPointer(block,args[5],callback,sizeof(callback))) return false;
        for (unsigned i = 0; i < 5; ++i) {
            if (i && !nativeScriptTypes_Assignable(ScriptType_Float,nativeScriptCode_ExpressionType(block,args[i]))) return false;
            if (!nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return false;
        }
        return snprintf(output, capacity,
            "{ int bone = %s; float radius = %s; "
            "if (bone >= 0 && (unsigned)bone < engine->bone_count(fighter) && isfinite(radius) && radius >= 0) "
            "engine->create_guard(gobj, &(ShieldDesc){ .bone = bone, .pos = { %s, %s, %s }, .radius = radius }, %s); }",
            values[0], values[4], values[1], values[2], values[3], callback) < capacity;
    }

    if (strncmp(source, "visuals.set_group(", 18) == 0) {
        char args[2][192], values[2][128]; const char* after;
        if (!nativeScriptCode_ParseArguments(source + 17, args, 2, &after) || *after)
            return false;
        for (unsigned i = 0; i < 2; ++i)
            if (!nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i])))
                return false;
        return snprintf(output, capacity,
            "{ int group = %s, value = %s; "
            "if (group >= 0 && group < fighter->x5AC.model_num && "
            "group < sizeof(fighter->x5F4_arr) / sizeof(fighter->x5F4_arr[0]) && value >= -1 && value <= 255) "
            "engine->set_visibility_group(gobj, group, value); }",
            values[0], values[1]) < capacity;
    }
    if (strncmp(source, "visuals.set_current_group(", 26) == 0) {
        char args[2][192], values[2][128]; const char* after;
        if (!nativeScriptCode_ParseArguments(source + 25, args, 2, &after) || *after)
            return false;
        for (unsigned i = 0; i < 2; ++i)
            if (!nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i])))
                return false;
        return snprintf(output, capacity,
            "{ int group = %s, value = %s; "
            "if (group >= 0 && group < fighter->x5AC.model_num && "
            "group < sizeof(fighter->x5F4_arr) / sizeof(fighter->x5F4_arr[0]) && value >= -1 && value <= 255) "
            "engine->set_current_visibility_group(gobj, group, value); }",
            values[0], values[1]) < capacity;
    }

    if (strncmp(source, "stage.event(", 12) == 0) {
        const char* arguments = source + 11;
        char call[768];
        if (!nativeScriptPrimitives_Expression(block, "stage.event", &arguments, call, sizeof(call)) || *arguments) return false;
        return snprintf(output, capacity, "%s;", call) < capacity;
    }
    if (strncmp(source, "capture.begin_attached(", 23) == 0 && block->grabbed_event) {
        char args[1][192], motion[128]; const char* after;
        if (!nativeScriptCode_ParseArguments(source + 22, args, 1, &after) || *after ||
            !nativeScriptCode_Expression(block, args[0], motion, sizeof(motion))) return false;
        return snprintf(output, capacity,
            "engine->initialize_attached_capture(victim_gobj, gobj, %s);", motion) < capacity;
    }
    if (strncmp(source, "capture.release_damage(", 23) == 0) {
        char args[2][192], values[2][128]; const char* after;
        if (!nativeScriptCode_ParseArguments(source + 22, args, 2, &after) || *after) return false;
        for (unsigned i = 0; i < 2; ++i)
            if (!nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return false;
        return snprintf(output, capacity,
            "if (fighter->victim_gobj != NULL) { Fighter_GObj* victim = fighter->victim_gobj; "
            "engine->apply_throw_damage(gobj, victim); engine->launch_thrown_victim(victim, (%s) ? gobj : NULL, %s); }",
            values[0], values[1]) < capacity;
    }
    if (strncmp(source, "bones.world_position(", 21) == 0) {
        char args[7][192], bone[128], offset[3][192]; const char* after;
        bool has_offset = nativeScriptCode_ParseArguments(source + 20, args, 7, &after);
        if ((!has_offset && !nativeScriptCode_ParseArguments(source + 20, args, 4, &after)) || *after ||
            block->model_effect || nativeScriptCode_ExpressionType(block, args[0]) != ScriptType_Int ||
            !nativeScriptCode_Expression(block, args[0], bone, sizeof(bone))) return false;
        for (unsigned i = 0; i < 3; ++i)
            if (has_offset && ((nativeScriptCode_ExpressionType(block, args[i+4]) != ScriptType_Int &&
                nativeScriptCode_ExpressionType(block, args[i+4]) != ScriptType_Float) ||
                !nativeScriptCode_Expression(block, args[i + 4], offset[i], sizeof(offset[i])))) return false;
        char stores[3][160];
        for (unsigned i = 0; i < 3; ++i) {
            for (unsigned j=0; j<i; ++j) if (!strcmp(args[i+1], args[j+1])) return false;
            int written;
            if (!strncmp(args[i+1], "state.", 6)) {
                const ScriptStateField* field = nativeFighterScript_StateField(block->state, args[i+1]+6);
                if (!field || field->array || field->type != ScriptState_Float) return false;
                written = snprintf(stores[i], sizeof(stores[i]),
                    "script_state_set_float(fighter, script_state_base + %u, pos.%c);", field->offset, 'x'+i);
            } else {
                bool found=false;
                for (unsigned j=0; j<block->local_count; ++j)
                    if (!strcmp(block->locals[j], args[i+1]) && block->local_types[j] == ScriptType_Float) found=true;
                if (!found) return false;
                written=snprintf(stores[i], sizeof(stores[i]), "local_%s = pos.%c;", args[i+1], 'x'+i);
            }
            if (written<0 || (size_t)written>=sizeof(stores[i])) return false;
        }
        int written = snprintf(output, capacity,
            "{ int32_t bone = (%s); Vec3 pos = { 0 }, offset; offset.x=(%s); offset.y=(%s); offset.z=(%s); "
            "if (bone >= 0 && (unsigned)bone < engine->bone_count(fighter) && fighter->parts[bone].joint != NULL) "
            "engine->joint_position(fighter->parts[bone].joint, %s, &pos); "
            "%s %s %s }",
            bone, has_offset ? offset[0] : "0", has_offset ? offset[1] : "0", has_offset ? offset[2] : "0",
            has_offset ? "&offset" : "0", stores[0], stores[1], stores[2]);
        return written>=0 && (size_t)written<capacity;
    }

    if (strncmp(source, "bones.set_scale(", 16) == 0) {
        char args[4][192], values[3][192]; const char* after;
        if (!nativeScriptCode_ParseArguments(source + 15, args, 4, &after) || *after) return false;
        const char* bone = nativeScriptCode_BoneSymbol(args[0], strlen(args[0]));
        if (bone == NULL) return false;
        for (unsigned i = 0; i < 3; ++i)
            if (!nativeScriptCode_Expression(block, args[i + 1], values[i], sizeof(values[i]))) return false;
        return snprintf(output, capacity,
            "{ Fighter_Part part = engine->resolve_bone(fighter, %s); if (fighter->parts[part].joint != NULL) engine->set_model_scale(fighter->parts[part].joint, &(Vec3){ %s, %s, %s }); }",
            bone, values[0], values[1], values[2]) < capacity;
    }
    if (strncmp(source, "bones.set_direct_animation_copy(", 32) == 0) {
        char args[2][192], value[512]; const char* after;
        if (!nativeScriptCode_ParseArguments(source + 31, args, 2, &after) || *after) return false;
        const char* bone = nativeScriptCode_BoneSymbol(args[0], strlen(args[0]));
        char joint[512];
        if (!nativeScriptCode_Expression(block, args[1], value, sizeof(value))) return false;
        if (bone != NULL) {
            if (snprintf(joint, sizeof(joint), "engine->resolve_bone(fighter, %s)", bone) >= sizeof(joint)) return false;
        } else if (!nativeScriptCode_Expression(block, args[0], joint, sizeof(joint))) return false;
        return snprintf(output, capacity,
            "{ int part = (int)(%s); if (part >= 0 && (unsigned)part < engine->bone_count(fighter)) fighter->parts[part].flags_b4 = %s; }",
            joint, value) < capacity;
    }
    if (strncmp(source, "bones.rotate_", 13) == 0 &&
        (source[13] == 'x' || source[13] == 'y' || source[13] == 'z') &&
        source[14] == '(') {
        char args[2][192], value[512]; const char* after;
        if (!nativeScriptCode_ParseArguments(source + 14, args, 2, &after) || *after) return false;
        const char* bone = nativeScriptCode_BoneSymbol(args[0], strlen(args[0]));
        if (bone == NULL || !nativeScriptCode_Expression(block, args[1], value, sizeof(value))) return false;
        return snprintf(output, capacity,
            "engine->set_root_rotation_%c(fighter, engine->resolve_bone(fighter, %s), %s);",
            source[13], bone, value) < capacity;
    }
    if (strncmp(source, "items.block_ground_pickup = ", 28) == 0) {
        char value[512];
        if (block == NULL || block->item_event || block->model_effect ||
            nativeScriptCode_ExpressionType(block, source + 28) != ScriptType_Bool ||
            !nativeScriptCode_Expression(block, source + 28, value, sizeof(value))) return false;
        return snprintf(output, capacity, "fighter->x2223_b4 = (%s);", value) < capacity;
    }
    const bool healing_filter = strncmp(source, "items.reject_healing_pickup(", 28) == 0;
    const bool companion_sleep = strncmp(source, "family.retire_companion_on_sleep(", 33) == 0;
    if (healing_filter || companion_sleep) {
        char args[1][192], value[512]; const char* after;
        if (block == NULL || block->item_event || block->model_effect ||
            !nativeScriptCode_ParseArguments(strchr(source, '('), args, 1, &after) || *after ||
            nativeScriptCode_ExpressionType(block, args[0]) != ScriptType_Bool ||
            !nativeScriptCode_Expression(block, args[0], value, sizeof(value))) return false;
        return snprintf(output, capacity, "fighter->%s = (%s);",
                        healing_filter ? "x2222_b4" : "x2222_b5", value) < capacity;
    }
    if (strncmp(source, "hurtboxes.define(", 17) == 0) {
        char args[11][192], values[11][160]; const char* after;
        if (!nativeScriptCode_ParseArguments(strchr(source, '('), args, 11, &after) || *after) return false;
        for (unsigned i = 0; i < 11; ++i)
            if (!nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return false;
        return snprintf(output, capacity,
            "{ int index = %s, bone = %s; if ((unsigned)index < fighter->hurt_capsules_len && (unsigned)bone < engine->bone_count(fighter)) engine->init_hurtbox(fighter, &fighter->hurt_capsules[index], &(ftHurtboxInit){ bone, %s, %s, { %s, %s, %s }, { %s, %s, %s }, %s }); }",
            values[0], values[1], values[2], values[3], values[4], values[5], values[6], values[7], values[8], values[9], values[10]) < capacity;
    }
    static const struct { const char* name; unsigned count; const char* code; } calls[] = {
        { "hurtboxes.set_all", 1, "{ int status = %s; if (status >= HurtCapsule_Enabled && status <= HurtCapsule_Intangible) engine->set_hurtboxes(gobj, status); }" },
        { "hurtboxes.set_body_status", 1, "{ int status = %s; if (status >= HurtCapsule_Enabled && status <= HurtCapsule_Intangible) engine->set_body_status(gobj, status); }" },
        { "hurtboxes.set_bone", 2, "{ int bone = %s, status = %s; if (bone >= 0 && (unsigned)bone < engine->bone_count(fighter) && status >= HurtCapsule_Enabled && status <= HurtCapsule_Intangible) for (unsigned capsule = 0; capsule < fighter->hurt_capsules_len; ++capsule) if (fighter->hurt_capsules[capsule].capsule.bone_idx == bone) { engine->set_bone_hurtbox(gobj, bone, status); break; } }" },
        { "items.set_held_visible", 1, "engine->set_held_item_visible(gobj, %s);" },
        { "items.throw_held", 1, "engine->throw_held_item(gobj, %s);" },
        { "articles.set_attached_visible", 1, "fighter->x221E_b4 = (%s);" },
        { "effects.spawn_at_position", 4, "engine->spawn_effect(%s, gobj, &(Vec3){ %s, %s, %s });" },
        { "effects.spawn_scaled_at_position", 5, "engine->spawn_effect(%s, gobj, &(Vec3){ %s, %s, %s }, &(float){ %s });" },
        { "effects.spawn_facing_at_position", 5, "engine->spawn_effect(%s, gobj, &(Vec3){ %s, %s, %s }, &(float){ %s });" },
        { "effects.spawn_on_part", 2, "engine->spawn_effect_on_part(gobj, %s, %s);" },
        { "effects.spawn_with_facing", 2, "engine->spawn_effect_with_facing(gobj, %s, %s);" },
        { "state.platform_drop", 3, "engine->enter_platform_drop(gobj, %s, %s, %s);" },
        { "visuals.set_part_animation", 2, "engine->set_part_animation(gobj, %s, %s);" },
        { "visuals.set_visible", 1, "fighter->invisible = !(%s);" },
        { "visuals.set_group_material_frame", 4, "engine->set_group_material_frame(fighter, %s, %s, %s, %s);" },
        { "visuals.show_part_animation", 1, "engine->show_part_animation(gobj, %s);" },
        { "visuals.hide_part_animation", 1, "engine->hide_part_animation(gobj, %s);" },
        { "model.set_scale", 3, "engine->set_model_scale((HSD_JObj*) gobj->hsd_obj, &(Vec3){ %s, %s, %s });" },
        { "color.start", 2, "engine->start_color(fighter, %s, %s);" },
        { "state.wait_blend", 1, "engine->enter_wait_blend(gobj, %s);" },
        { "state.held_open", 1, "engine->enter_held_open(gobj, %s);" },
        { "collision.ground_standard_box", 6, "engine->ground_standard_box_collision(gobj, &(ftCollisionBox){ %s, %s, { %s, %s }, { %s, %s } });" },
        { "animation.set_rate", 1, "engine->set_animation_rate(gobj, %s);" },
        { "animation.set_timer", 1, "fighter->cur_anim_frame = %s;" },
        { "items.notify_pickup", 1, "engine->notify_item_pickup(gobj, %s);" },
        { "sound.play", 3, "engine->play_sound(fighter, %s, %s, %s);" },
        { "sound.loop", 3, "engine->loop_sound(fighter, %s, %s, %s);" },
        { "controller.rumble", 2, "engine->rumble(fighter, %s, %s);" },
        { "state.enter_at_speed", 4, "engine->change_motion_state(gobj, %s, %s, %s, %s, 0, 0);" },
        { "physics.air_accel_friction", 3, "engine->air_accel_friction(fighter, %s, %s, %s);" },
        { "state.landing_special", 2, "engine->enter_landing_fall_special(gobj, %s, %s);" },
        { "damage.heal", 1, "engine->heal_damage(gobj, %s);" },
        { "state.cliff_jump_phase2", 1, "engine->enter_cliff_jump_phase2(gobj, %s);" },
        { "state.landing", 5, "engine->enter_landing(gobj, %s, %s, %s, %s, %s);" },
        { "state.landing_aerial", 2, "engine->enter_landing_aerial(gobj, %s, %s);" },
        { "attack.enter_rapid_jab", 1, "engine->enter_rapid_jab(gobj, %s);" },
        { "jump.prepare", 2, "fighter->mv.co.kneebend.jump_input = %s; fighter->mv.co.kneebend.is_short_hop = %s;" },
        { "jump.launch", 2, "engine->launch_jump(gobj, %s, %s);" },
        { "jump.begin", 7, "engine->begin_aerial_jump(gobj, %s, &(Vec3){ %s, %s, %s }, %s, %s, %s);" },
        { "damage.enter", 2, "engine->enter_damage_motion(gobj, %s, %s);" },
        { "damage.default_reaction", 1, "engine->enter_damage_reaction(gobj, %s);" },
        { "items.release_held", 4, "if (fighter->item_gobj != NULL) engine->release_held_item(fighter->item_gobj, &(Vec3){ %s, %s, %s }, %s);" },
        { "capture.set_victim_capture_motion", 1, "if (fighter->victim_gobj != NULL) engine->enter_attached_capture_motion(fighter->victim_gobj, %s);" },
        { "capture.set_victim_attached_throw", 4, "if (fighter->victim_gobj != NULL) engine->enter_attached_throw(fighter->victim_gobj, %s, %s, %s, %s);" },
        { "capture.initialize_hold", 1, "if (fighter->victim_gobj != NULL) engine->initialize_hold(fighter->victim_gobj, %s);" },
        { "capture.set_victim_hold_motion", 1, "if (fighter->victim_gobj != NULL) engine->set_hold_motion(fighter->victim_gobj, %s);" },
        { "capture.set_victim_throw_motion", 2, "if (fighter->victim_gobj != NULL) engine->set_throw_motion(fighter->victim_gobj, %s, %s);" },
        { "walk.enter", 10, "engine->enter_walk(gobj, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s);" },
    };
    for (unsigned i = 0; i < sizeof(calls) / sizeof(calls[0]); ++i) {
        size_t length = strlen(calls[i].name);
        if (strncmp(source, calls[i].name, length) != 0 || source[length] != '(')
            continue;
        if (block->model_effect && strcmp(calls[i].name, "state.held_open") == 0) return false;
        char args[10][192], values[10][384] = { 0 };
        const char* after;
        if (!nativeScriptCode_ParseArguments(source + length, args, calls[i].count, &after) ||
            *after != '\0') return false;
        if ((!strcmp(calls[i].name, "attack.enter_rapid_jab") ||
             !strcmp(calls[i].name, "state.landing_aerial")) &&
            (block->model_effect || block->item_event ||
             nativeScriptCode_ExpressionType(block, args[0]) != ScriptType_Int)) return false;
        for (unsigned j = 0; j < calls[i].count; ++j)
            if (!nativeScriptCode_Expression(block, args[j], values[j], sizeof(values[j])))
                return false;
        return snprintf(output, capacity, calls[i].code,
                        values[0], values[1], values[2], values[3], values[4],
                        values[5], values[6], values[7], values[8], values[9]) < capacity;
    }

    if (strncmp(source, "reflector.create(", 17) == 0) {
        char args[9][192], values[9][128]; const char* after;
        if (!nativeScriptCode_ParseArguments(source + 16, args, 9, &after) || *after)
            return false;
        for (unsigned i = 0; i < 9; ++i)
            if (!nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i])))
                return false;
        return snprintf(output, capacity,
            "engine->create_reflector(gobj, &(ReflectDesc){ %s, %s, { %s, %s, %s }, %s, %s, %s, %s }, 0);",
            values[0], values[1], values[2], values[3], values[4], values[5],
            values[6], values[7], values[8]) < capacity;
    }
    if (strncmp(source, "attachments.load(", 17) == 0) {
        char args[5][192], values[4][128]; const char* after;
        if (!nativeScriptCode_ParseArguments(source + 16, args, 5, &after) ||
            *after || !root_literal(args[1])) return false;
        const unsigned positions[] = { 0, 2, 3, 4 };
        for (unsigned i = 0; i < 4; ++i)
            if (!nativeScriptCode_Expression(block, args[positions[i]], values[i],
                                             sizeof(values[i]))) return false;
        return snprintf(output, capacity,
            "engine->load_model_attachment(gobj, %s, %s, %s, %s, %s);",
            values[0], args[1], values[1], values[2], values[3]) < capacity;
    }
    if (strncmp(source, "attachments.add_dynamics(", 25) == 0) {
        char args[3][192], values[3][128]; const char* after;
        if (!nativeScriptCode_ParseArguments(source + 24, args, 3, &after) || *after)
            return false;
        for (unsigned i = 0; i < 3; ++i)
            if (!nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i])))
                return false;
        return snprintf(output, capacity,
            "engine->add_attachment_dynamics(gobj, %s, %s, %s);",
            values[0], values[1], values[2]) < capacity;
    }
    static const struct { const char* name; const char* member; } indexed[] = {
        { "hitbox.set_enabled", "state" }, { "hitbox.set_group", "x4" },
    };
    for (unsigned i = 0; i < sizeof(indexed) / sizeof(indexed[0]); ++i) {
        size_t length = strlen(indexed[i].name);
        if (strncmp(source, indexed[i].name, length) != 0 || source[length] != '(')
            continue;
        char args[2][192], value[256], index_value[256];
        const char* after;
        char* end;
        if (!nativeScriptCode_ParseArguments(source + length, args, 2, &after) || *after)
            return false;
        if (i == 0) {
            if (nativeScriptCode_ExpressionType(block, args[0]) != ScriptType_Int ||
                nativeScriptCode_ExpressionType(block, args[1]) != ScriptType_Bool ||
                !nativeScriptCode_Expression(block, args[0], index_value, sizeof(index_value)) ||
                !nativeScriptCode_Expression(block, args[1], value, sizeof(value))) return false;
            int written = snprintf(output, capacity,
                "{ int32_t index=(%s); bool enabled=(%s); if ((uint32_t)index < 4) "
                "{ fighter->x914[index].state = enabled ? HitCapsule_Enabled : HitCapsule_Disabled; "
                "if (enabled) fighter->x2219_b3 = true; } }",
                index_value, value);
            return written >= 0 && (size_t)written < capacity;
        }
        unsigned long index = strtoul(args[0], &end, 0);
        if (end == args[0] || *end || index >= 4 ||
            !nativeScriptCode_Expression(block, args[1], value, sizeof(value))) return false;
        return snprintf(output, capacity,
            "fighter->x914[%lu].x4 = %s;", index, value) < capacity;
    }

    // Effect argument layouts are explicit, since different engine effects
    // consume either position+rotation or position+facing+rotation.
    bool rotated = strncmp(source, "effects.spawn_rotated(", 22) == 0;
    bool oriented = strncmp(source, "effects.spawn_oriented(", 23) == 0;
    bool quake = strncmp(source, "camera.quake(", 13) == 0;
    if (!rotated && !oriented && !quake) return false;
    const char* open = strchr(source, '(');
    unsigned count = quake ? 4 : oriented ? 6 : 5;
    char args[6][192], values[6][128] = { 0 };
    const char* after;
    if (!nativeScriptCode_ParseArguments(open, args, count, &after) || *after) return false;
    for (unsigned i = 0; i < count; ++i)
        if (!nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i])))
            return false;
    if (quake) return snprintf(output, capacity,
        "engine->camera_quake(%s, &(Vec3){ %s, %s, %s });",
        values[0], values[1], values[2], values[3]) < capacity;
    return snprintf(output, capacity,
        "{ Vec3 pos = { %s, %s, %s }; float angle = %s; %s%s%s"
        "engine->spawn_effect(%s, gobj, &pos, %s&angle); }",
        values[1], values[2], values[3], values[oriented ? 5 : 4],
        oriented ? "float facing = " : "", oriented ? values[4] : "",
        oriented ? "; " : "", values[0], oriented ? "&facing, " : "") < capacity;
}
