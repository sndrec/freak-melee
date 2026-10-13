#include "native_fighter_script_guard.h"

#include <string.h>

static const struct { const char* name; const char* field; } fields[] = {
    { "guard.elapsed", "mv.co.guard.x0" },
    { "guard.release_requested", "mv.co.guard.xC" },
    { "guard.minimum_hold_frames", "mv.co.guard.x10" },
    { "guard.reflect_frames", "mv.co.guard.x14" },
    { "guard.powershield_frames", "mv.co.guard.x18" },
    { "guard.input_lock_frames", "mv.co.guard.x1C" },
    { "guard.interrupt_mode", "mv.co.guard.x20" },
    { "guard.delayed_action_frames", "mv.co.guard.x24" },
    { "guard.volume_enabled", "x221A_b7" },
    { "guard.front_only", "x221B_b3" },
    { "guard.filtered_attacks_only", "x221B_b4" },
    { "guard.skip_contact_details", "x221B_b2" },
    { "guard.new_powershield", "x221C_b3" },
    { "guard.reflect_window", "x221C_b1" },
    { "guard.powershield_window", "x221C_b2" },
    { "shield.health", "shield_health" },
    { "input.analog_shield_age", "trigger_analog_timer" },
    { "damage.jump_armor", "dmg.armor1" },
    { "input.horizontal_tilt_age", "x670_timer_lstick_tilt_x" },
};

const char* nativeScriptGuard_Property(const char* name)
{
    // These expressions are literals so no temporary buffer survives a call.
    static const struct { const char* name; const char* code; } properties[] = {
        { "guard.elapsed", "fighter->mv.co.guard.x0" },
        { "guard.release_requested", "fighter->mv.co.guard.xC" },
        { "guard.minimum_hold_frames", "fighter->mv.co.guard.x10" },
        { "guard.reflect_frames", "fighter->mv.co.guard.x14" },
        { "guard.powershield_frames", "fighter->mv.co.guard.x18" },
        { "guard.input_lock_frames", "fighter->mv.co.guard.x1C" },
        { "guard.interrupt_mode", "fighter->mv.co.guard.x20" },
        { "guard.delayed_action_frames", "fighter->mv.co.guard.x24" },
        { "guard.volume_enabled", "fighter->x221A_b7" },
        { "guard.front_only", "fighter->x221B_b3" },
        { "guard.filtered_attacks_only", "fighter->x221B_b4" },
        { "guard.skip_contact_details", "fighter->x221B_b2" },
        { "guard.new_powershield", "fighter->x221C_b3" },
        { "guard.reflect_window", "fighter->x221C_b1" },
        { "guard.powershield_window", "fighter->x221C_b2" },
        { "shield.health", "fighter->shield_health" },
        { "shield.light_amount", "fighter->lightshield_amount" },
        { "shield.bone", "fighter->ft_data->x8->x11" },
        { "shield.initial_size", "fighter->co_attrs.initial_shield_size" },
        { "input.shield_held", "((fighter->input.held_buttons[0] & HSD_PAD_LR) != 0)" },
        { "input.analog_shield_age", "fighter->trigger_analog_timer" },
        { "damage.jump_armor", "fighter->dmg.armor1" },
        { "input.horizontal_tilt_age", "fighter->x670_timer_lstick_tilt_x" },
        { "reflector.active", "fighter->reflecting" },
        { "Shield.start", "NativeFighterShield_Start" },
        { "Shield.hold", "NativeFighterShield_Hold" },
        { "Shield.release", "NativeFighterShield_Release" },
        { "Shield.hit", "NativeFighterShield_Hit" },
        { "Shield.reflect_continue", "NativeFighterShield_ReflectContinue" },
        { "Shield.reflect_start", "NativeFighterShield_ReflectStart" },
        { "shield.start_health", "engine->common_attribute(NativeFighterCommon_ShieldStartHealth)" },
        { "shield.minimum_hold_duration", "engine->common_attribute(NativeFighterCommon_ShieldMinimumHold)" },
        { "shield.pushback_damage_scale", "engine->common_attribute(NativeFighterCommon_ShieldPushbackDamage)" },
        { "shield.pushback_base", "engine->common_attribute(NativeFighterCommon_ShieldPushbackBase)" },
        { "shield.pushback_velocity_scale", "engine->common_attribute(NativeFighterCommon_ShieldPushbackVelocity)" },
        { "shield.pushback_light_min", "engine->common_attribute(NativeFighterCommon_ShieldPushbackLightMin)" },
        { "shield.pushback_light_max", "engine->common_attribute(NativeFighterCommon_ShieldPushbackLightMax)" },
        { "shield.reflect_duration", "engine->common_attribute(NativeFighterCommon_ShieldReflectDuration)" },
        { "shield.reflect_radius", "engine->common_attribute(NativeFighterCommon_ShieldReflectRadius)" },
        { "shield.reflect_damage_multiplier", "engine->common_attribute(NativeFighterCommon_ShieldReflectDamage)" },
        { "shield.reflect_speed_multiplier", "engine->common_attribute(NativeFighterCommon_ShieldReflectSpeed)" },
        { "shield.powershield_duration", "engine->common_attribute(NativeFighterCommon_ShieldPowershieldDuration)" },
    };
    for (unsigned i = 0; i < sizeof(properties) / sizeof(properties[0]); ++i)
        if (strcmp(name, properties[i].name) == 0) return properties[i].code;
    return NULL;
}

bool nativeScriptGuard_Expression(const ScriptCodeBlock* block,
                                  const char* name, const char** source,
                                  char* output, size_t capacity)
{
    static const struct { const char* name; const char* member; } checks[] = {
        { "guard.drain", "drain_guard" },
        { "controls.try_powershield", "check_powershield" },
        { "controls.try_guard_item_throw", "check_guard_item_throw" },
        { "controls.try_guard_platform_drop", "check_guard_platform_drop" },
    };
    for (unsigned i = 0; i < sizeof(checks) / sizeof(checks[0]); ++i) {
        if (strcmp(name, checks[i].name) != 0) continue;
        if (strncmp(*source, "()", 2) != 0) return false;
        *source += 2;
        return snprintf(output, capacity, "engine->%s(gobj)", checks[i].member) < capacity;
    }
    bool initialize = strcmp(name, "visuals.initialize_group_material_animation") == 0;
    bool previous = strcmp(name, "visuals.previous_group") == 0;
    if (!initialize && !previous && strcmp(name, "visuals.current_group") != 0) return false;
    unsigned count = initialize ? 4 : 1;
    char args[4][192], values[4][160]; const char* after;
    if (!nativeScriptCode_ParseArguments(*source, args, count, &after)) return false;
    for (unsigned i = 0; i < count; ++i)
        if (!nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return false;
    if (!initialize && nativeScriptCode_ExpressionType(block, args[0]) != ScriptType_Int) return false;
    *source = after;
    if (initialize)
        return snprintf(output, capacity,
            "engine->initialize_group_material_animation(fighter, %s, %s, %s, %s)",
            values[0], values[1], values[2], values[3]) < capacity;
    return snprintf(output, capacity, "engine->%s_model_group(fighter, %s)",
                    previous ? "previous" : "current", values[0]) < capacity;
}

bool nativeScriptGuard_Action(const ScriptCodeBlock* block, const char* source,
                              char* output, size_t capacity)
{
    for (unsigned i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i) {
        size_t length = strlen(fields[i].name);
        if (strncmp(source, fields[i].name, length) != 0 ||
            source[length] != ' ' || source[length + 1] != '=') continue;
        char value[512];
        if (!nativeScriptCode_Expression(block, source + length + 2, value, sizeof(value))) return false;
        return snprintf(output, capacity,
            nativeScriptTypes_Builtin(fields[i].name) == ScriptType_Bool ? "fighter->%s = (%s) != 0;" : "fighter->%s = (%s);",
            fields[i].field, value) < capacity;
    }
    static const struct { const char* name; const char* code; } simple[] = {
        { "guard.install_shield()", "engine->install_guard(gobj);" },
        { "guard.install_reflector()", "engine->install_guard_reflector(gobj, authored_reflect_hit);" },
        { "guard.consume_release_input()", "engine->consume_guard_release_input(gobj);" },
        { "guard.tick_powershield()", "engine->tick_powershield(gobj);" },
        { "guard.track_sdi()", "fighter->hitlag_cb = engine->guard_sdi; fighter->post_hitlag_cb = engine->guard_post_sdi;" },
        { "guard.enter_hold()", "engine->enter_guard_hold(gobj);" },
        { "guard.enter_release()", "engine->enter_guard_release(gobj);" },
        { "guard.scale_joint()", "engine->scale_guard_joint(fighter);" },
        { "controls.guard_reflect_actions()", "engine->guard_reflect_actions(gobj);" },
        { "physics.guard_motion()", "engine->guard_physics(gobj);" },
        { "physics.guard_hit_motion()", "engine->guard_hit_physics(gobj);" },
        { "collision.guard_standard()", "engine->guard_collision(gobj);" },
        { "collision.guard_hit()", "engine->guard_hit_collision(gobj);" },
        { "jump.update_animation()", "engine->update_aerial_jump(gobj);" },
        { "capture.constrain_caught()", "if (fighter->victim_gobj != NULL) engine->constrain_caught(fighter, (Fighter*)fighter->victim_gobj->user_data);" },
    };
    for (unsigned i = 0; i < sizeof(simple) / sizeof(simple[0]); ++i)
        if (strcmp(source, simple[i].name) == 0)
            return snprintf(output, capacity, "%s", simple[i].code) < capacity;
    static const struct { const char* name; unsigned count; const char* code; } calls[] = {
        { "bones.root.add_rotation_y", 1, "engine->add_joint_rotation_y(fighter->parts[0].joint, %s);" },
        { "capture.enclose_caught", 1, "engine->capture_enclose_caught(gobj, %s);" },
    };
    for (unsigned i = 0; i < sizeof(calls) / sizeof(calls[0]); ++i) {
        size_t length = strlen(calls[i].name);
        if (strncmp(source, calls[i].name, length) != 0 || source[length] != '(') continue;
        char args[1][192], value[160]; const char* after;
        if (!nativeScriptCode_ParseArguments(source + length, args, calls[i].count, &after) || *after ||
            !nativeScriptCode_Expression(block, args[0], value, sizeof(value))) return false;
        return snprintf(output, capacity, calls[i].code, value) < capacity;
    }
    bool translate = strncmp(source, "bones.set_translation(", 22) == 0;
    bool scale = strncmp(source, "bones.set_scale(", 16) == 0;
    if (translate || scale) {
        char args[4][192], value[4][160]; const char* after;
        if (!nativeScriptCode_ParseArguments(strchr(source, '('), args, 4, &after) || *after) return false;
        for (unsigned i = 0; i < 4; ++i)
            if (!nativeScriptCode_Expression(block, args[i], value[i], sizeof(value[i]))) return false;
        return snprintf(output, capacity,
            "{ int bone = %s; if ((unsigned)bone < engine->bone_count(fighter)) engine->%s(fighter->parts[bone].joint, &(Vec3){%s, %s, %s}); }",
            value[0], translate ? "set_joint_translation" : "set_model_scale", value[1], value[2], value[3]) < capacity;
    }
    return false;
}
