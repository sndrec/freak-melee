#include "native_fighter_script_code.h"
#include "native_fighter_script_functions.h"
#include "native_fighter_script_lifetime.h"
#include "native_fighter_script_motion_policy.h"
#include "native_fighter_script_colors.h"
#include "native_fighter_script_model_effect.h"
#include "native_fighter_script_pose.h"
#include "native_fighter_script_sound_policy.h"
#include "native_fighter_script_surface_policy.h"
#include "native_fighter_item_script.h"
#include <melee/ft/ftcharacter_assets.h>
#include <melee/ft/ftparts.h>

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static void nativeScriptCode_EmitStockRequirements(const ScriptMove* move, FILE* output)
{
    if (move->ground_entry == ScriptEntry_Stock)
        fprintf(output, " || base->%s == 0",
                nativeScriptCode_SpecialField(move->special, false));
    if (move->air_entry == ScriptEntry_Stock)
        fprintf(output, " || base->%s == 0",
                nativeScriptCode_SpecialField(move->special, true));
}

static void nativeScriptCode_EmitStockBindings(const ScriptMove* move, FILE* output)
{
    const char* suffix = nativeScriptCode_SpecialSuffix(move->special);
    if (move->ground_entry == ScriptEntry_Stock) {
        const char* field = nativeScriptCode_SpecialField(move->special, false);
        fprintf(output,
                "    inherited_ground_%s = base->%s;\n"
                "    generated.%s = authored_ground_%s;\n",
                suffix, field, field, suffix);
    }
    if (move->air_entry == ScriptEntry_Stock) {
        const char* field = nativeScriptCode_SpecialField(move->special, true);
        fprintf(output,
                "    inherited_air_%s = base->%s;\n"
                "    generated.%s = authored_air_%s;\n",
                suffix, field, field, suffix);
    }
}

static bool nativeScriptCode_HandlerComplete(const ScriptMove* move, unsigned handler)
{
    if (handler == 1)
        return move->ground_entry != ScriptEntry_None &&
               move->ground_entry_code.count != 0 &&
               !move->ground_entry_code.last_opens;
    if (handler == 2)
        return move->air_entry != ScriptEntry_None &&
               move->air_entry_code.count != 0 &&
               !move->air_entry_code.last_opens;
    if (handler == 3)
        return move->stock_article || move->fireball_bone != NULL ||
               move->neutral_cue_code.count != 0 ||
               move->effect_bone != NULL || move->cue_speed_set ||
               move->cue_vertical_speed_set;
    if (handler == 4)
        return move->ground_animation.count != 0 &&
               !move->ground_animation.last_opens;
    if (handler == 5)
        return move->air_animation.count != 0 &&
               !move->air_animation.last_opens;
    if (handler == 6)
        return move->ground_interrupt.count != 0 &&
               !move->ground_interrupt.last_opens;
    if (handler == 7)
        return move->air_interrupt.count != 0 &&
               !move->air_interrupt.last_opens;
    if (handler == 8)
        return move->ground_physics.count != 0 &&
               !move->ground_physics.last_opens;
    if (handler == 9)
        return move->air_physics.count != 0 &&
               !move->air_physics.last_opens;
    if (handler == 10)
        return move->ground_collision.count != 0 &&
               !move->ground_collision.last_opens;
    if (handler == 11)
        return move->air_collision.count != 0 &&
               !move->air_collision.last_opens;
    return handler == 12 && move->accessory_code.count != 0 &&
           !move->accessory_code.last_opens;
}

typedef struct ScriptStateHandler {
    char name[64];
    ScriptCodeBlock code;
    unsigned line;
} ScriptStateHandler;

enum { SCRIPT_FIGHTER_HANDLERS = 512 };

static bool state_handler_is_predicate(const char* name)
{
    return strcmp(name, "wait_input") == 0 || strcmp(name, "action_status") == 0 ||
           strcmp(name, "aerial_action_manual") == 0 ||
           strcmp(name, "aerial_action_automatic") == 0 ||
           strcmp(name, "aerial_attack_motion") == 0 ||
           strcmp(name, "action_metadata") == 0 ||
           strcmp(name, "shield_scale") == 0 ||
           strcmp(name, "shield_alpha") == 0 ||
           strcmp(name, "catch_pull_frame") == 0 ||
           strcmp(name, "catch_allowed") == 0 ||
           strcmp(name, "catch_frame") == 0 ||
           strcmp(name, "catch_pull_anchor") == 0 ||
           strcmp(name, "catch_pull_ready") == 0 ||
           strcmp(name, "catch_pull_follow") == 0 ||
           strcmp(name, "air_tether_input") == 0 ||
           strcmp(name, "throw_finish") == 0 ||
           strcmp(name, "throw_victim_motion") == 0 || strcmp(name, "held_motion") == 0 ||
           strcmp(name, "air_resume_motion") == 0 || strcmp(name, "held_timer_expired") == 0 ||
           strcmp(name, "heavy_pickup_finish") == 0 ||
           strcmp(name, "idle_variants") == 0 ||
           strcmp(name, "motion_is_angled") == 0 ||
           strcmp(name, "jab_restart") == 0 ||
           strcmp(name, "jab_start") == 0 ||
           strcmp(name, "item_swing_motion") == 0 ||
           strcmp(name, "dynamics_wind") == 0 ||
           strcmp(name, "dynamics_ground_collision") == 0 ||
           strcmp(name, "dynamics_interaction_start") == 0 ||
           strcmp(name, "worn_item_jump_sound") == 0 ||
           strcmp(name, "body_rebound") == 0 ||
           strcmp(name, "cpu_prepare") == 0 ||
           strcmp(name, "cpu_drop_command") == 0 ||
           strcmp(name, "cpu_mode_update") == 0 || strcmp(name, "cpu_output") == 0 ||
           strcmp(name, "cpu_stick_step") == 0 ||
           strcmp(name, "cpu_stick_limit") == 0 ||
           strcmp(name, "cpu_attack_motion") == 0 ||
           strcmp(name, "cpu_grab_motion") == 0 ||
           strcmp(name, "cpu_attack_decision") == 0 ||
           strcmp(name, "cpu_travel_decision") == 0 ||
           strcmp(name, "cpu_pending_action") == 0 ||
           strcmp(name, "cpu_falling_recovery_allowed") == 0 ||
           strcmp(name, "cpu_falling_destination_reached") == 0 ||
           strcmp(name, "cpu_available_jump") == 0 ||
           strcmp(name, "cpu_capture_decision") == 0 ||
           strcmp(name, "cpu_projectile_defense") == 0 ||
           strcmp(name, "cpu_projectile_hit") == 0 ||
           strcmp(name, "cpu_charge_decision") == 0 ||
           strcmp(name, "cpu_pummel_special") == 0 ||
           strcmp(name, "cpu_ranged_allowed") == 0 ||
           strcmp(name, "cpu_special_candidate") == 0 ||
           strcmp(name, "cpu_threat") == 0;
}

static bool state_handler_target(const char* name, unsigned* motion,
                                 const char** field)
{
    const char* prefix = "state_";
    if (strncmp(name, prefix, strlen(prefix)) != 0) return false;
    char* end = NULL;
    unsigned long number = strtoul(name + strlen(prefix), &end, 10);
    if (end == name + strlen(prefix) || number > 65535 ||
        *end != '_') return false;
    const char* phase = end + 1;
    *field = strcmp(phase, "animation") == 0 ? "anim_cb" :
             strcmp(phase, "interrupt") == 0 ? "input_cb" :
             strcmp(phase, "physics") == 0 ? "phys_cb" :
             strcmp(phase, "collision") == 0 ? "coll_cb" : NULL;
    *motion = (unsigned) number;
    return *field != NULL;
}

static const struct { const char* name; const char* field; } fighter_handlers[] = {
        { "load", "on_load" },
        { "load_special_attrs", "on_load_special_attrs" },
        { "form_activated", "on_form_activated" },
        { "tick", "on_tick" },
        { "late_update", "on_late_update" },
        { "color_animation_reset", "on_color_animation_reset" },
        { "death", "on_death" },
        { "landing", "on_landing" },
        { "landing_action", "on_landing_action" },
        { "throw_animation", "on_throw_animation" },
        { "throw_finish", "on_throw_finish" },
        { "throw_victim_motion", "on_throw_victim_motion" },
        { "held_motion", "on_held_motion" },
        { "neutral_enter", "on_neutral_enter" },
        { "air_resume_motion", "on_air_resume_motion" },
        { "held_timer_expired", "on_held_timer_expired" },
        { "heavy_pickup_finish", "on_heavy_pickup_finish" },
        { "item_swing_motion", "on_item_swing_motion" },
        { "heavy_throw_enter", "on_heavy_throw_enter" },
        { "roll_enter", "on_roll_enter" },
        { "heavy_item_damage", "on_heavy_item_damage" },
        { "cpu_initialize", "on_cpu_initialize" },
        { "cpu_mode_update", "on_cpu_mode_update" },
        { "cpu_output", "on_cpu_output" },
        { "cpu_prepare", "on_cpu_prepare" },
        { "cpu_drop_command", "on_cpu_drop_command" },
        { "cpu_stick_step", "on_cpu_stick_step" },
        { "cpu_stick_limit", "on_cpu_stick_limit" },
        { "cpu_pending_action", "on_cpu_pending_action" },
        { "cpu_spacing_update", "on_cpu_spacing_update" },
        { "cpu_attack_motion", "on_cpu_attack_motion" },
        { "cpu_grab_motion", "on_cpu_grab_motion" },
        { "cpu_attack_decision", "on_cpu_attack_decision" },
        { "cpu_travel_decision", "on_cpu_travel_decision" },
        { "cpu_falling_recovery_allowed", "on_cpu_falling_recovery_allowed" },
        { "cpu_falling_destination_reached", "on_cpu_falling_destination_reached" },
        { "cpu_available_jump", "on_cpu_available_jump" },
        { "cpu_capture_decision", "on_cpu_capture_decision" },
        { "cpu_projectile_defense", "on_cpu_projectile_defense" },
        { "cpu_pummel_special", "on_cpu_pummel_special" },
        { "cpu_ranged_candidates", "on_cpu_ranged_candidates" },
        { "cpu_ranged_allowed", "on_cpu_ranged_allowed" },
        { "cpu_special_candidate", "on_cpu_special_candidate" },
        { "cpu_projectile_hit", "on_cpu_projectile_hit" },
        { "presentation_item", "on_presentation_item" },
        { "cpu_threat", "on_cpu_threat" },
        { "cpu_charge_decision", "on_cpu_charge_decision" },
        { "cpu_charge_cancel", "on_cpu_charge_cancel" },
        { "taunt", "on_taunt" },
        { "demo_enter", "on_demo_enter" },
        { "shield_break", "on_shield_break" },
        { "wait_input", "on_wait_input" },
        { "wait_enter", "on_wait_enter" },
        { "crouch_wait_enter", "on_crouch_wait_enter" },
        { "aerial_attack_enter", "on_aerial_attack_enter" },
        { "aerial_action_manual", "on_aerial_action_manual" },
        { "aerial_action_automatic", "on_aerial_action_automatic" },
        { "grounded_motion_setup", "on_grounded_motion_setup" },
        { "action_status", "on_action_status" },
        { "action_metadata", "on_action_metadata" },
        { "idle_variants", "on_idle_variants" },
        { "motion_is_angled", "on_motion_is_angled" },
        { "jab_restart", "on_jab_restart" },
        { "jab_start", "on_jab_start" },
        { "jab_enter", "on_jab_enter" },
        { "rapid_jab_enter", "on_rapid_jab_enter" },
        { "down_tilt_enter", "on_down_tilt_enter" },
        { "aerial_attack_motion", "on_aerial_attack_motion" },
        { "forward_smash_start", "on_forward_smash_start" },
        { "forward_smash_enter", "on_forward_smash_enter" },
        { "up_smash_enter", "on_up_smash_enter" },
        { "down_smash_enter", "on_down_smash_enter" },
        { "auxiliary_item_attach", "on_auxiliary_item_attach" },
        { "auxiliary_item_visibility_reset", "on_auxiliary_item_visibility_reset" },
        { "cpu_recovery", "on_cpu_recovery" },
        { "shield_start", "on_shield_start" },
        { "shield_reflect_start", "on_shield_reflect_start" },
        { "shield_transition", "on_shield_transition" },
        { "shield_scale", "on_shield_scale" },
        { "shield_alpha", "on_shield_alpha" },
        { "spotdodge_enter", "on_spotdodge_enter" },
        { "capture_wait_setup", "on_capture_wait_setup" },
        { "capture_wait_enter", "on_capture_wait_enter" },
        { "catch_pull_frame", "on_catch_pull_frame" },
        { "catch_allowed", "on_catch_allowed" },
        { "catch_start", "on_catch_start" },
        { "catch_frame", "on_catch_frame" },
        { "catch_exit", "on_catch_exit" },
        { "catch_wait_release", "on_catch_wait_release" },
        { "catch_pull_anchor", "on_catch_pull_anchor" },
        { "catch_pull_ready", "on_catch_pull_ready" },
        { "catch_pull_follow", "on_catch_pull_follow" },
        { "air_tether_input", "on_air_tether_input" },
        { "throw_victim_setup", "on_throw_victim_setup" },
        { "dynamics_wind", "on_dynamics_wind" },
        { "dynamics_ground_collision", "on_dynamics_ground_collision" },
        { "dynamics_interaction_start", "on_dynamics_interaction_start" },
        { "aerial_jump", "on_aerial_jump" },
        { "worn_item_jump_sound", "on_worn_item_jump_sound" },
        { "body_rebound", "on_body_rebound" },
        { "trail", "on_trail" },
        { "death1", "on_death1" },
        { "user_data_remove", "on_user_data_remove" },
        { "absorb", "on_absorb" },
        { "item_pickup_ext", "on_item_pickup_ext" },
        { "item_drop_ext", "on_item_drop_ext" },
        { "item_pickup", "on_item_pickup" },
        { "item_drop", "on_item_drop" },
        { "item_invisible", "on_item_invisible" },
        { "item_visible", "on_item_visible" },
        { "knockback_enter", "on_knockback_enter" },
        { "knockback_exit", "on_knockback_exit" },
        { "special_n", "special_n" },
        { "special_air_n", "special_air_n" },
        { "special_s", "special_s" },
        { "special_air_s", "special_air_s" },
        { "special_hi", "special_hi" },
        { "special_air_hi", "special_air_hi" },
        { "special_lw", "special_lw" },
        { "special_air_lw", "special_air_lw" },
};

static const char* state_handler_field(const char* name)
{
    for (unsigned i = 0; i < sizeof(fighter_handlers) / sizeof(fighter_handlers[0]); ++i)
        if (strcmp(name, fighter_handlers[i].name) == 0) return fighter_handlers[i].field;
    return NULL;
}

static bool independent_load_handlers(const ScriptStateHandler* handlers,
                                      unsigned count,
                                      NativeFighterScriptError* error)
{
    bool load = false, attrs = false;
    for (unsigned i = 0; i < count; ++i) {
        load |= strcmp(handlers[i].name, "load") == 0;
        attrs |= strcmp(handlers[i].name, "load_special_attrs") == 0;
    }
    return (load && attrs) || nativeScriptCode_Fail(error, 0,
        "Independent behavior needs authored load and load_special_attrs handlers (pass is allowed).");
}

static unsigned motion_field_bit(const char* field)
{
    return strcmp(field, "anim_cb") == 0 ? 1u :
           strcmp(field, "input_cb") == 0 ? 2u :
           strcmp(field, "phys_cb") == 0 ? 4u : 8u;
}

static void emit_independent_setup(FILE* output)
{
    fputs("    uint8_t authored_motion_fields[1024] = { 0 };\n"
          "    generated.source_kind = Ft_Kind_None;\n"
          "    generated.neutral_article = 0;\n"
          "    generated.items = 0; generated.item_count = 0;\n"
          "    generated.article_logics = 0; generated.article_logic_count = 0;\n"
          "    generated.owns_model_attachments = false;\n", output);
    for (unsigned i = 0; i < sizeof(fighter_handlers) / sizeof(fighter_handlers[0]); ++i)
        fprintf(output, "    generated.%s = 0;\n", fighter_handlers[i].field);
}

static void emit_independent_guard(FILE* output)
{
    fputs("    for (unsigned i = 0; i < base->motion_state_count; ++i) {\n"
          "        const MotionState* original = &base->motion_states[i];\n"
          "        unsigned required = (original->anim_cb != 0 ? 1u : 0u) |\n"
          "            (original->input_cb != 0 ? 2u : 0u) |\n"
          "            (original->phys_cb != 0 ? 4u : 0u) |\n"
          "            (original->coll_cb != 0 ? 8u : 0u);\n"
          "        if ((authored_motion_fields[i] & required) != required) return 0;\n"
          "    }\n", output);
}

static bool block_owns_attachments(const ScriptCodeBlock* block)
{
    for (unsigned i = 0; i < block->count; ++i)
        if (strstr(block->lines[i].generated, "engine->load_model_attachment(") != NULL)
            return true;
    return false;
}

static bool state_handler_is_item_event(const char* name)
{
    return strcmp(name, "item_pickup_ext") == 0 ||
           strcmp(name, "item_drop_ext") == 0 ||
           strcmp(name, "item_pickup") == 0 ||
           strcmp(name, "item_drop") == 0;
}

static bool state_handler_action_allowed(const char* name)
{
    const char* field = state_handler_field(name);
    return field != NULL && strncmp(name, "special_", 8) != 0;
}

static void emit_project_parameters(const NativeFighterParameters* parameters,
                                    FILE* output)
{
    if (parameters == NULL || parameters->count == 0) return;
    fprintf(output,
        "static union { float f; int32_t i; uint32_t bits; } "
        "script_parameters[%u];\n"
        "__declspec(dllexport) int __cdecl "
        "FreakMelee_SetFighterParameters(const uint32_t* values, "
        "unsigned count, uint64_t schema) {\n"
        "    if (values == 0 || count != %u || "
        "schema != 0x%016llxULL) return 0;\n"
        "    for (unsigned i = 0; i < count; ++i) "
        "script_parameters[i].bits = values[i];\n"
        "    return 1;\n}\n",
        parameters->count, parameters->count,
        (unsigned long long) parameters->schema_hash);
}

static bool generate_state_callbacks(const char* source, size_t source_size,
                                     const char* code_key,
                                     const NativeFighterScriptConstruction* construction, FILE* output,
                                     NativeFighterScriptError* error,
                                     const NativeFighterItemProgram* items,
                                     const char* asset_root,
                                     const NativeFighterParameters* parameters,
                                     uint64_t source_hash,
                                     ScriptFunctionProgram* functions,
                                     bool independent, const ScriptMotionPolicies* policies, uint16_t visibility_mask)
{
    ScriptStateHandler* handlers = calloc(SCRIPT_FIGHTER_HANDLERS, sizeof(*handlers));
    if (handlers == NULL) return nativeScriptCode_Fail(error, 0, "Out of memory.");
    ScriptStateTable state = { .bytes = 2, .parameters = parameters };
    unsigned count = 0, line_number = 0;
    bool group_seen = false, on_hit = false;
    const char* fighter_root = construction->fighter_root;
    unsigned motion_first = construction->motion_first, motion_count = construction->motion_count;
    ScriptStateHandler* active = NULL;
    bool valid = true;
    size_t offset = 0;
    while (offset < source_size && valid) {
        size_t end = offset;
        while (end < source_size && source[end] != '\n') ++end;
        size_t size = end - offset;
        ++line_number;
        if (size && source[offset + size - 1] == '\r') --size;
        if (size >= 512 || memchr(source + offset, '\0', size) != NULL) {
            valid = nativeScriptCode_Fail(error, line_number, "Line is too long or contains NUL.");
            break;
        }
        char line[512];
        memcpy(line, source + offset, size);
        line[size] = '\0';
        offset = end < source_size ? end + 1 : end;
        while (size && (line[size - 1] == ' ' || line[size - 1] == '\t'))
            line[--size] = '\0';
        const char* nonspace = line;
        while (*nonspace == ' ') ++nonspace;
        if (!size || *nonspace == '#') continue;
        if (strncmp(line, "state ", 6) == 0 && !group_seen) {
            if (!nativeFighterScript_AddStateField(&state, line))
                valid = nativeScriptCode_Fail(error, line_number,
                             "Expected a unique fighter state field.");
            continue;
        }
        unsigned group_prefix = strncmp(line, "move ", 5) == 0 ? 5 :
            strncmp(line, "action ", 7) == 0 ? 7 : 0;
        if (group_prefix && size > group_prefix + 1 &&
            line[size - 1] == ':' &&
            nativeScriptCode_Identifier(line + group_prefix,
                                        size - group_prefix - 1, 64)) {
            if (active != NULL && active->code.last_opens)
                valid = nativeScriptCode_Fail(error, line_number, "Incomplete handler block.");
            group_seen = true;
            active = NULL;
            continue;
        }
        if (strncmp(line, "    on ", 7) == 0 && size > 8 &&
            line[size - 1] == ':') {
            if (!group_seen || count == SCRIPT_FIGHTER_HANDLERS ||
                (active != NULL && (active->code.count == 0 ||
                                    active->code.last_opens))) {
                valid = nativeScriptCode_Fail(error, line_number, "Incomplete or excess handler.");
                continue;
            }
            char name[64];
            size_t length = size - 8;
            if (length >= sizeof(name)) {
                valid = nativeScriptCode_Fail(error, line_number, "Handler name is too long.");
                continue;
            }
            memcpy(name, line + 7, length);
            name[length] = '\0';
            unsigned motion = 0;
            const char* field = NULL;
            if (state_handler_field(name) == NULL &&
                strcmp(name, "damage") != 0 &&
                strcmp(name, "secondary_damage") != 0 &&
                strcmp(name, "death2") != 0 &&
                strcmp(name, "pre_hitlag") != 0 &&
                strcmp(name, "post_hitlag") != 0 &&
                strcmp(name, "hurtbox_detect") != 0 &&
                strcmp(name, "deal_damage") != 0 &&
                strcmp(name, "state_change") != 0 &&
                strcmp(name, "reflect") != 0 &&
                strcmp(name, "reflect_hit") != 0 &&
                !state_handler_target(name, &motion, &field)) {
                valid = nativeScriptCode_Fail(error, line_number, "Unknown state handler.");
                continue;
            }
            if (field != NULL && (motion < motion_first ||
                                  motion - motion_first >= motion_count)) {
                valid = nativeScriptCode_Fail(error, line_number,
                             "State handler is outside the base motion table.");
                continue;
            }
            for (unsigned i = 0; i < count; ++i)
                if (strcmp(handlers[i].name, name) == 0) {
                    valid = nativeScriptCode_Fail(error, line_number, "Duplicate state handler.");
                    break;
                }
            if (!valid) continue;
            active = &handlers[count++];
            strcpy(active->name, name);
            active->line = line_number;
            active->code.predicate = state_handler_is_predicate(name);
            active->code.floating_result = strcmp(name, "shield_scale") == 0 || strcmp(name, "catch_pull_frame") == 0;
            active->code.shield_transition_event = strcmp(name, "shield_transition") == 0;
            active->code.motion_event = strcmp(name, "motion_is_angled") == 0 ||
                                       strcmp(name, "jab_restart") == 0 ||
                                       strcmp(name, "throw_victim_motion") == 0 || strcmp(name, "aerial_attack_motion") == 0;
            active->code.victim_motion_event = strcmp(name, "throw_victim_motion") == 0;
            active->code.held_motion_event = strcmp(name, "held_motion") == 0;
            active->code.air_resume_event = strcmp(name, "air_resume_motion") == 0;
            active->code.neutral_enter_event = strcmp(name, "neutral_enter") == 0;
            active->code.landing_action_event = strcmp(name, "landing_action") == 0;
            active->code.item_swing_event = strcmp(name, "item_swing_motion") == 0;
            active->code.demo_event = strcmp(name, "demo_enter") == 0;
            active->code.action_metadata_event = strcmp(name, "action_metadata") == 0;
            active->code.cpu_prepare_event = strcmp(name, "cpu_prepare") == 0;
            active->code.strict_integer_result = active->code.cpu_prepare_event ||
                strcmp(name, "cpu_stick_step") == 0 || strcmp(name, "cpu_stick_limit") == 0;
            active->code.strict_boolean_result = strcmp(name, "cpu_drop_command") == 0 ||
                strcmp(name, "cpu_mode_update") == 0 || strcmp(name, "cpu_output") == 0;
            active->code.integer_result = active->code.strict_integer_result || strcmp(name, "item_swing_motion") == 0 ||
                                          strcmp(name, "action_metadata") == 0 ||
                                          strcmp(name, "throw_victim_motion") == 0 || strcmp(name, "held_motion") == 0 ||
                                          strcmp(name, "air_resume_motion") == 0 ||
                                          strcmp(name, "dynamics_interaction_start") == 0 ||
                                          strcmp(name, "cpu_projectile_hit") == 0 ||
                                          strcmp(name, "jab_start") == 0 || strcmp(name, "aerial_attack_motion") == 0;
            active->code.trail_event = strcmp(name, "trail") == 0;
            active->code.scalar_event = strcmp(name, "cpu_spacing_update") == 0;
            active->code.smash_angle_event = strcmp(name, "forward_smash_enter") == 0;
            active->code.target_geometry_event = strcmp(name, "cpu_special_candidate") == 0;
            active->code.cpu_projectile_defense_event = strcmp(name, "cpu_projectile_defense") == 0;
            active->code.cpu_projectile_hit_event = strcmp(name, "cpu_projectile_hit") == 0;
            active->code.other_fighter_event = strcmp(name, "cpu_threat") == 0 || strcmp(name, "capture_wait_setup") == 0 || strcmp(name, "capture_wait_enter") == 0;
            active->code.dynamics_set_event = strcmp(name, "dynamics_interaction_start") == 0;
            active->code.state = &state;
            active->code.functions = functions;
            active->code.articles = nativeFighterItemScript_ArticleCatalog(items);
            active->code.item_event =
                state_handler_is_item_event(name);
            active->code.collision = field != NULL &&
                                     strcmp(field, "coll_cb") == 0;
            active->code.entry = strncmp(name, "special_", 8) == 0;
            active->code.air_entry = strncmp(name, "special_air_", 12) == 0;
            const char* special = active->code.air_entry ? name + 12 :
                active->code.entry ? name + 8 : NULL;
            active->code.special = special == NULL
                ? ScriptSpecial_Unset :
                strcmp(special, "n") == 0 ? ScriptSpecial_Neutral :
                strcmp(special, "s") == 0 ? ScriptSpecial_Side :
                strcmp(special, "hi") == 0 ? ScriptSpecial_Up :
                strcmp(special, "lw") == 0 ? ScriptSpecial_Down :
                ScriptSpecial_Unset;
            active->code.motion_state_first = motion_first;
            active->code.motion_state_count = motion_count;
            continue;
        }
        if (active == NULL || !nativeScriptCode_AddLine(&active->code, line)) {
            valid = nativeScriptCode_Fail(error, line_number,
                         "Invalid statement, expression, or indentation.");
            continue;
        }
        if (strstr(line, "callbacks.on_hit_state(") != NULL) on_hit = true;
    }
    if (valid && (count == 0 || active == NULL ||
                  active->code.count == 0 || active->code.last_opens))
        valid = nativeScriptCode_Fail(error, line_number,
                     "Fighter script needs complete handlers.");
    if (valid && !nativeScriptFunctions_Compile(functions, &state, nativeFighterItemScript_ArticleCatalog(items), false,
                                                motion_first, motion_count, error))
        valid = false;
    if (valid && !nativeScriptModelEffect_Compile(functions->model_effects, &state,
        nativeFighterItemScript_ArticleCatalog(items), functions, error)) valid = false;
    bool has_secondary_damage = false, uses_secondary_damage = false;
    bool has_damage = false, has_death1 = false, has_death2 = false, has_detect = false;
    bool has_pre_hitlag = false, has_post_hitlag = false;
    bool uses_hitlag = false;
    bool has_deal_damage = false;
    bool has_state_change = false, uses_state_change = false;
    bool has_reflect = false, uses_reflect = false;
    bool has_reflect_hit = false, uses_reflect_hit = false;
    bool uses_damage = false, uses_death1 = false, uses_death2 = false, uses_detect = false;
    bool uses_deal_damage = false;
    for (unsigned i = 0; i < count; ++i) {
        has_damage |= strcmp(handlers[i].name, "damage") == 0;
        has_secondary_damage |= strcmp(handlers[i].name, "secondary_damage") == 0;
        has_death1 |= strcmp(handlers[i].name, "death1") == 0;
        has_death2 |= strcmp(handlers[i].name, "death2") == 0;
        has_pre_hitlag |= strcmp(handlers[i].name, "pre_hitlag") == 0;
        has_post_hitlag |= strcmp(handlers[i].name, "post_hitlag") == 0;
        has_detect |= strcmp(handlers[i].name, "hurtbox_detect") == 0;
        has_deal_damage |= strcmp(handlers[i].name, "deal_damage") == 0;
        has_state_change |= strcmp(handlers[i].name, "state_change") == 0;
        has_reflect |= strcmp(handlers[i].name, "reflect") == 0;
        has_reflect_hit |= strcmp(handlers[i].name, "reflect_hit") == 0;
        for (unsigned j = 0; j < handlers[i].code.count; ++j) {
            uses_reflect |= strstr(handlers[i].code.lines[j].text,
                "callbacks.track_reflect()") != NULL;
            uses_reflect_hit |= strstr(handlers[i].code.lines[j].text, "callbacks.track_reflect_hit()") != NULL ||
                strstr(handlers[i].code.lines[j].text, "guard.install_reflector()") != NULL;
            uses_state_change |= strstr(handlers[i].code.lines[j].text,
                "callbacks.track_state_change()") != NULL;
            uses_damage |= strstr(handlers[i].code.lines[j].text,
                                  "callbacks.track_damage()") != NULL;
            uses_secondary_damage |= strstr(handlers[i].code.lines[j].text,
                                  "callbacks.track_secondary_damage()") != NULL;
            uses_death1 |= strstr(handlers[i].code.lines[j].text,
                                  "callbacks.track_death1()") != NULL;
            uses_death2 |= (strstr(handlers[i].code.lines[j].text, "callbacks.track_death2()") != NULL || strstr(handlers[i].code.lines[j].text, "callbacks.track_death3()") != NULL);
            uses_hitlag |= strstr(handlers[i].code.lines[j].text,
                                  "callbacks.track_hitlag()") != NULL;
            uses_detect |= strstr(handlers[i].code.lines[j].text,
                                  "callbacks.track_hurtbox_detect()") != NULL;
            uses_deal_damage |= strstr(handlers[i].code.lines[j].text,
                                  "callbacks.track_deal_damage()") != NULL;
        }
    }
    for (unsigned i = 0; i < functions->count; ++i) {
        const ScriptCodeBlock* code = &functions->functions[i].code;
        uses_damage |= nativeScriptCode_Contains(code, "callbacks.track_damage()");
        uses_secondary_damage |= nativeScriptCode_Contains(code, "callbacks.track_secondary_damage()");
        uses_death1 |= nativeScriptCode_Contains(code, "callbacks.track_death1()");
        uses_death2 |= (nativeScriptCode_Contains(code, "callbacks.track_death2()") || nativeScriptCode_Contains(code, "callbacks.track_death3()"));
        uses_hitlag |= nativeScriptCode_Contains(code, "callbacks.track_hitlag()");
        uses_detect |= nativeScriptCode_Contains(code, "callbacks.track_hurtbox_detect()");
        uses_deal_damage |= nativeScriptCode_Contains(code, "callbacks.track_deal_damage()");
        uses_state_change |= nativeScriptCode_Contains(code, "callbacks.track_state_change()");
        uses_reflect |= nativeScriptCode_Contains(code, "callbacks.track_reflect()");
        uses_reflect_hit |= nativeScriptCode_Contains(code, "callbacks.track_reflect_hit()") ||
            nativeScriptCode_Contains(code, "guard.install_reflector()");
        on_hit |= nativeScriptCode_Contains(code, "callbacks.on_hit_state(");
    }
    if (valid && uses_secondary_damage && !has_secondary_damage)
        valid = nativeScriptCode_Fail(error, line_number,
                     "callbacks.track_secondary_damage() needs an `on secondary_damage:` handler.");
    if (valid && uses_damage && !has_damage)
        valid = nativeScriptCode_Fail(error, line_number,
                     "callbacks.track_damage() needs an `on damage:` handler.");
    if (valid && uses_death1 && !has_death1)
        valid = nativeScriptCode_Fail(error, line_number,
                     "callbacks.track_death1() needs an `on death1:` handler.");
    if (valid && uses_death2 && !has_death2)
        valid = nativeScriptCode_Fail(error, line_number,
                     "callbacks.track_death2() needs an `on death2:` handler.");
    if (valid && uses_hitlag && (!has_pre_hitlag || !has_post_hitlag))
        valid = nativeScriptCode_Fail(error, line_number,
                     "callbacks.track_hitlag() needs pre/post hitlag handlers.");
    if (valid && uses_detect && !has_detect)
        valid = nativeScriptCode_Fail(error, line_number,
                     "callbacks.track_hurtbox_detect() needs an `on hurtbox_detect:` handler.");
    if (valid && uses_deal_damage && !has_deal_damage)
        valid = nativeScriptCode_Fail(error, line_number,
                     "callbacks.track_deal_damage() needs an `on deal_damage:` handler.");
    if (valid && uses_reflect && !has_reflect)
        valid = nativeScriptCode_Fail(error, 0,
                    "callbacks.track_reflect() needs an `on reflect:` handler.");
    if (valid && uses_reflect_hit && !has_reflect_hit)
        valid = nativeScriptCode_Fail(error, 0,
                    "callbacks.track_reflect_hit() needs an `on reflect_hit:` handler.");
    if (valid && uses_state_change && !has_state_change)
        valid = nativeScriptCode_Fail(error, line_number,
                     "callbacks.track_state_change() needs an `on state_change:` handler.");
    if (valid && independent)
        valid = independent_load_handlers(handlers, count, error);
    if (!valid) { free(handlers); return false; }

    fputs("#include <melee/ft/native_fighter_module.h>\n"
          "#include <melee/it/forward.h>\n"
          "#include <melee/mp/forward.h>\n"
          "#include <melee/ft/types.h>\n"
          "#include <melee/lb/types.h>\n"
          "#include <sysdolphin/baselib/controller.h>\n"
          "#include <sysdolphin/baselib/gobj.h>\n"
          "#include <sysdolphin/baselib/jobj.h>\n"
          "#include <math.h>\n#include <string.h>\n"
          "static const NativeFighterHost* engine;\n"
          "static NativeFighterDefinition generated;\n"
          "static MotionState authored_states[1024];\n"
          "static unsigned script_state_base;\n"
          "static int script_signbit(float value) {\n"
          "    uint32_t bits; memcpy(&bits, &value, sizeof(bits));\n"
          "    return (int) (bits >> 31);\n}\n", output);
    emit_project_parameters(parameters, output);
    if (has_secondary_damage)
        fputs("static void authored_secondary_damage(HSD_GObj* gobj);\n", output);
    if (has_damage)
        fputs("static void authored_damage(HSD_GObj* gobj);\n", output);
    if (has_death2)
        fputs("static void authored_death2(HSD_GObj* gobj);\n", output);
    if (has_pre_hitlag)
        fputs("static void authored_pre_hitlag(HSD_GObj* gobj);\n", output);
    if (has_post_hitlag)
        fputs("static void authored_post_hitlag(HSD_GObj* gobj);\n", output);
    if (has_detect)
        fputs("static void authored_hurtbox_detect(HSD_GObj* gobj);\n", output);
    if (has_deal_damage)
        fputs("static void authored_deal_damage(HSD_GObj* gobj);\n", output);
    if (has_reflect)
        fputs("static void authored_reflect(HSD_GObj* gobj);\n", output);
    if (has_reflect_hit)
        fputs("static void authored_reflect_hit(HSD_GObj* gobj);\n", output);
    if (has_state_change)
        fputs("static void authored_state_change(HSD_GObj* gobj);\n", output);
    if (state.has_float)
        fputs("static float script_state_get_float(Fighter* fighter, unsigned offset) {\n"
              "    float value; memcpy(&value, fighter->native_module_state + offset, 4); return value;\n}\n"
              "static void script_state_set_float(Fighter* fighter, unsigned offset, float value) {\n"
              "    memcpy(fighter->native_module_state + offset, &value, 4);\n}\n",
              output);
    if (state.has_int)
        fputs("static int32_t script_state_get_int(Fighter* fighter, unsigned offset) {\n"
              "    int32_t value; memcpy(&value, fighter->native_module_state + offset, 4); return value;\n}\n"
              "static void script_state_set_int(Fighter* fighter, unsigned offset, int32_t value) {\n"
              "    memcpy(fighter->native_module_state + offset, &value, 4);\n}\n",
              output);
    if (on_hit) {
        fputs("static void authored_on_hit(HSD_GObj* gobj) {\n"
              "    Fighter* fighter = gobj->user_data;\n", output);
        nativeScriptLifetime_EmitEntry(output, ScriptCallback_Fighter);
        fputs("    uint16_t target;\n"
              "    memcpy(&target, fighter->native_module_state + script_state_base, sizeof(target));\n",
              output);
        nativeScriptLifetime_EmitLine(output,
            "    engine->change_motion_state(gobj, target, 0, 1, 1, 0, 0);\n",
            functions, ScriptCallback_Fighter, ScriptCallback_Void);
        nativeScriptLifetime_EmitLine(output, "    engine->start_animation(gobj);\n",
            functions, ScriptCallback_Fighter, ScriptCallback_Void);
        fputs("}\n", output);
    }
    if (!nativeScriptState_EmitArrayAccess(output, nativeScriptState_ArrayTypes(&state) |
            nativeFighterItemScript_StateArrayTypes(items)) ||
        !nativeScriptState_EmitMetadata(output, &state, "authored_state_fields", true)) return false;
    fputs("static NativeFighterStateField combined_state_fields[64];\n", output);
    nativeScriptPose_Emit(functions->poses, output);
    nativeScriptModelEffect_Emit(functions->model_effects, output);
    nativeScriptFunctions_Emit(functions, output);
    if (!nativeFighterItemScript_Emit(items, asset_root, output, error)) {
        free(handlers);
        return false;
    }
    for (unsigned i = 0; i < count; ++i) {
        const char* callback = strcmp(handlers[i].name, "damage") == 0
            ? "authored_damage" :
            strcmp(handlers[i].name, "secondary_damage") == 0
            ? "authored_secondary_damage" :
            strcmp(handlers[i].name, "death2") == 0
            ? "authored_death2" :
            strcmp(handlers[i].name, "pre_hitlag") == 0
            ? "authored_pre_hitlag" :
            strcmp(handlers[i].name, "post_hitlag") == 0
            ? "authored_post_hitlag" :
            strcmp(handlers[i].name, "hurtbox_detect") == 0
            ? "authored_hurtbox_detect" :
            strcmp(handlers[i].name, "deal_damage") == 0
            ? "authored_deal_damage" :
            strcmp(handlers[i].name, "reflect") == 0
            ? "authored_reflect" :
            strcmp(handlers[i].name, "reflect_hit") == 0
            ? "authored_reflect_hit" :
            strcmp(handlers[i].name, "state_change") == 0
            ? "authored_state_change" : NULL;
        if (callback != NULL)
            fprintf(output,
                    "static void %s(HSD_GObj* gobj) {\n"
                    "    Fighter* fighter = gobj->user_data;\n", callback);
        else
            fprintf(output,
                    "static %s authored_handler_%u(HSD_GObj* gobj%s) {\n"
                    "    Fighter* fighter = gobj->user_data;\n", handlers[i].code.floating_result ? "float" : handlers[i].code.integer_result ? "int" : handlers[i].code.predicate ? "bool" : "void", i,
                    handlers[i].code.cpu_prepare_event ? ", uint32_t history_source" :
                    handlers[i].code.held_motion_event ? ", int32_t motion, int32_t previous_motion" :
                    handlers[i].code.air_resume_event ? ", int32_t previous_motion" :
                    handlers[i].code.neutral_enter_event ? ", float blend" :
                    strcmp(handlers[i].name, "throw_victim_motion") == 0
                        ? ", int motion, int victim_motion" : handlers[i].code.item_swing_event
                        ? ", int swing_type, int swing_variant" : handlers[i].code.demo_event
                        ? ", int demo_mode" : handlers[i].code.action_metadata_event
                        ? ", int action_metadata" : handlers[i].code.shield_transition_event
                        ? ", int shield_mode, bool special_hit" : state_handler_is_item_event(handlers[i].name)
                        ? ", bool flag" : handlers[i].code.motion_event
                        ? ", int motion" : handlers[i].code.trail_event
                        ? ", NativeFighterTrail* trail" : handlers[i].code.target_geometry_event
                        ? ", float target_angle, float target_distance" : handlers[i].code.cpu_projectile_defense_event
                        ? ", int projectile_category" : handlers[i].code.cpu_projectile_hit_event
                        ? ", int prediction_frames, bool unblockable" : handlers[i].code.smash_angle_event
                        ? ", float smash_angle" : handlers[i].code.scalar_event
                        ? ", float event_value" : handlers[i].code.other_fighter_event
                        ? ", HSD_GObj* other_gobj" : handlers[i].code.dynamics_set_event
                        ? ", int set, int set_count" : "");
        if (handlers[i].code.landing_action_event)
            fputs("    const bool script_landing_interrupt_allowed = fighter->mv.co.landing.allow_interrupt;\n", output);
        nativeScriptCode_EmitLines(&handlers[i].code, output);
        if (handlers[i].code.held_motion_event || strcmp(handlers[i].name, "aerial_attack_motion") == 0) fputs("    return motion;\n", output);
        else if (handlers[i].code.air_resume_event) fputs("    return previous_motion;\n", output);
        else if (strcmp(handlers[i].name, "throw_victim_motion") == 0) fputs("    return victim_motion;\n", output);
        else if (handlers[i].code.floating_result) fputs("    return 0.0f;\n", output);
        else if (handlers[i].code.item_swing_event) fputs("    return -1;\n", output);
        else if (handlers[i].code.action_metadata_event) fputs("    return action_metadata;\n", output);
        else if (handlers[i].code.integer_result) fputs("    return 0;\n", output);
        else if (handlers[i].code.predicate) fputs("    return false;\n", output);
        fputs("}\n", output);
    }
    char item_host_checks[4096];
    if (!nativeFighterItemScript_HostChecks(items, item_host_checks,
                                            sizeof(item_host_checks))) {
        free(handlers);
        return nativeScriptCode_Fail(error, 0, "Too many item host requirements.");
    }
    fprintf(output,
            "__declspec(dllexport) const NativeFighterDefinition* __cdecl\n"
            "FreakMelee_GetFighterModule(const NativeFighterDefinition* base, "
            "const NativeFighterHost* host) {\n"
            "    if (!base || !host || !base->motion_states || !base->fighter_root || "
            "strcmp(base->fighter_root, \"%s\") != 0 || "
            "base->motion_state_first != %u || "
            "base->motion_state_count != %u || "
            "%s > 256 - %u%s",
            fighter_root, motion_first, motion_count,
            state.has_float || state.has_int ? "((base->state_bytes + 3u) & ~3u)" : "base->state_bytes",
            state.bytes, item_host_checks);
    nativeScriptMotionPolicy_EmitChecks(policies, output);
    if (on_hit) fputs(" || host->callback_source_alive == 0", output);
    for (unsigned i = 0; i < count; ++i)
        nativeScriptCode_EmitHostChecks(&handlers[i].code, output);
    for (unsigned i = 0; i < functions->count; ++i)
        nativeScriptCode_EmitHostChecks(&functions->functions[i].code, output);
    nativeScriptModelEffect_EmitChecks(functions->model_effects, output);
    fprintf(output,
            ") return 0;\n"
            "    engine = host; script_state_base = %s;\n"
            "    generated = *base; generated.code_key = \"%s\";\n"
            "    generated.state_bytes = script_state_base + %u;\n"
            "    memcpy(authored_states, base->motion_states, "
            "base->motion_state_count * sizeof(MotionState));\n"
            "    generated.motion_states = authored_states;\n",
            state.has_float || state.has_int ? "(base->state_bytes + 3u) & ~3u" : "base->state_bytes",
            code_key, state.bytes);
    if (independent) emit_independent_setup(output);
    nativeScriptFunctions_EmitDefinitionExports(functions, output);
    nativeScriptPose_EmitDefinition(functions->poses, output);
    nativeScriptSoundPolicy_EmitDefinition(functions->sound_policy, output);
    nativeScriptSurfacePolicy_EmitDefinition(functions->surface_policy, output);
    nativeScriptModelEffect_EmitDefinition(functions->model_effects, output);
    if (!nativeScriptState_EmitCombinedMetadata(output, &state)) return false;
    nativeScriptMotionPolicy_EmitBindings(policies, output, independent);
    nativeScriptColors_EmitVisibility(visibility_mask, output);
    bool owns_attachments = false;
    for (unsigned i = 0; i < count; ++i)
        owns_attachments |= block_owns_attachments(&handlers[i].code);
    for (unsigned i = 0; i < functions->count; ++i)
        owns_attachments |= block_owns_attachments(&functions->functions[i].code);
    if (owns_attachments)
        fputs("    generated.owns_model_attachments = true;\n", output);
    for (unsigned i = 0; i < count; ++i) {
        unsigned motion;
        const char* field;
        if (state_handler_target(handlers[i].name, &motion, &field)) {
            fprintf(output,
                    "    authored_states[%u].%s = authored_handler_%u;\n",
                    motion - motion_first, field, i);
            if (independent)
                fprintf(output, "    authored_motion_fields[%u] |= %uu;\n",
                        motion - motion_first, motion_field_bit(field));
        } else if (state_handler_field(handlers[i].name) != NULL)
            fprintf(output,
                    "    generated.%s = authored_handler_%u;\n",
                    state_handler_field(handlers[i].name), i);
    }
    if (nativeFighterItemScript_Count(items) != 0)
        fprintf(output,
                "    generated.items = scripted_items;\n"
                "    generated.item_count = %u;\n",
                nativeFighterItemScript_Count(items));
    if (nativeFighterItemScript_ArticleCount(items) != 0)
        fprintf(output,
                "    generated.article_logics = scripted_article_logics;\n"
                "    generated.article_logic_count = %u;\n",
                nativeFighterItemScript_ArticleCount(items));
    if (independent) emit_independent_guard(output);
    fputs("    return &generated;\n}\n"
          "__declspec(dllexport) unsigned long long __cdecl "
          "FreakMelee_GetFighterModuleLayout(void) { "
          "return nativeFighterModule_LayoutFingerprint(); }\n", output);
    fprintf(output,
            "__declspec(dllexport) unsigned long long __cdecl "
            "FreakMelee_GetScriptSourceHash(void) { "
            "return 0x%016llxULL; }\n",
            (unsigned long long) source_hash);
    free(handlers);
    return !ferror(output) || nativeScriptCode_Fail(error, 0, "Could not write generated C.");
}

static bool generate_special_callbacks(const char* source, size_t source_size,
                                       const char* code_key,
                                       const char* asset_root, FILE* output,
                                       NativeFighterScriptError* error,
                                       const NativeFighterItemProgram* items,
                                       const NativeFighterParameters* parameters,
                                       uint64_t source_hash,
                                     ScriptFunctionProgram* functions,
                                     bool independent, const ScriptMotionPolicies* policies, uint16_t visibility_mask)
{
    ScriptStateTable state = { .bytes = 1, .parameters = parameters };
    static ScriptMove moves[4];
    static ScriptStateHandler action_handlers[SCRIPT_FIGHTER_HANDLERS];
    memset(moves, 0, sizeof(moves));
    memset(action_handlers, 0, sizeof(action_handlers));
    unsigned move_count = 0;
    unsigned action_handler_count = 0;
    ScriptStateHandler* active_action = NULL;
    bool action_block = false;
    static ScriptMove move;
    memset(&move, 0, sizeof(move));
    move.ground_entry_code.state = &state;
    move.ground_entry_code.functions = functions;
    move.ground_entry_code.articles = nativeFighterItemScript_ArticleCatalog(items);
    move.ground_entry_code.entry = true;
    move.air_entry_code.state = &state;
    move.air_entry_code.functions = functions;
    move.air_entry_code.articles = nativeFighterItemScript_ArticleCatalog(items);
    move.air_entry_code.entry = true;
    move.air_entry_code.air_entry = true;
    move.ground_animation.state = &state;
    move.ground_animation.functions = functions;
    move.ground_animation.articles = nativeFighterItemScript_ArticleCatalog(items);
    move.air_animation.state = &state;
    move.air_animation.functions = functions;
    move.air_animation.articles = nativeFighterItemScript_ArticleCatalog(items);
    move.ground_interrupt.state = &state;
    move.ground_interrupt.functions = functions;
    move.ground_interrupt.articles = nativeFighterItemScript_ArticleCatalog(items);
    move.air_interrupt.state = &state;
    move.air_interrupt.functions = functions;
    move.air_interrupt.articles = nativeFighterItemScript_ArticleCatalog(items);
    move.ground_physics.state = &state;
    move.ground_physics.functions = functions;
    move.ground_physics.articles = nativeFighterItemScript_ArticleCatalog(items);
    move.air_physics.state = &state;
    move.air_physics.functions = functions;
    move.air_physics.articles = nativeFighterItemScript_ArticleCatalog(items);
    move.ground_collision.state = &state;
    move.ground_collision.functions = functions;
    move.ground_collision.articles = nativeFighterItemScript_ArticleCatalog(items);
    move.ground_collision.collision = true;
    move.air_collision.state = &state;
    move.air_collision.functions = functions;
    move.air_collision.articles = nativeFighterItemScript_ArticleCatalog(items);
    move.air_collision.collision = true;
    move.air_collision.air_entry = true;
    move.neutral_cue_code.state = &state;
    move.neutral_cue_code.functions = functions;
    move.neutral_cue_code.articles = nativeFighterItemScript_ArticleCatalog(items);
    move.accessory_code.state = &state;
    move.accessory_code.functions = functions;
    move.accessory_code.articles = nativeFighterItemScript_ArticleCatalog(items);
    unsigned line_number = 0;
    unsigned phase = 0; // 0: move, 1: first handler, 2: operations, 3: actions
    unsigned active_handler = 0; // 1/2: entry, 3: cue, 4-11: state phases
    size_t offset = 0;
    while (offset < source_size) {
        size_t end = offset;
        while (end < source_size && source[end] != '\n') ++end;
        size_t size = end - offset;
        ++line_number;
        if (size != 0 && source[offset + size - 1] == '\r') --size;
        if (size >= 512 || memchr(source + offset, '\0', size) != NULL)
            return nativeScriptCode_Fail(error, line_number, "Line is too long or contains NUL.");
        char line[512];
        memcpy(line, source + offset, size);
        line[size] = '\0';
        offset = end < source_size ? end + 1 : end;
        while (size != 0 && (line[size - 1] == ' ' || line[size - 1] == '\t'))
            line[--size] = '\0';
        const char* nonspace = line;
        while (*nonspace == ' ' || *nonspace == '\t') ++nonspace;
        if (*nonspace == '\0' || *nonspace == '#') continue;

        if ((phase == 0 || phase == 2) &&
            strncmp(line, "action ", 7) == 0) {
            if (phase == 2 && !nativeScriptCode_HandlerComplete(&move, active_handler))
                return nativeScriptCode_Fail(error, line_number,
                            "Previous handler needs an operation.");
            if (phase == 2) moves[move_count++] = move;
            phase = 3;
        }
        if (phase == 3) {
            if (strncmp(line, "action ", 7) == 0) {
                if (action_block && (active_action == NULL ||
                    active_action->code.count == 0 ||
                    active_action->code.last_opens))
                    return nativeScriptCode_Fail(error, line_number,
                                "Previous action needs a complete handler.");
                if (size < 9 || line[size - 1] != ':' ||
                    !nativeScriptCode_Identifier(line + 7, size - 8, 64))
                    return nativeScriptCode_Fail(error, line_number, "Expected `action Name:`.");
                action_block = true;
                active_action = NULL;
                continue;
            }
            if (strncmp(line, "    on ", 7) == 0 && size > 8 &&
                line[size - 1] == ':') {
                if (!action_block ||
                    action_handler_count == sizeof(action_handlers) /
                                            sizeof(action_handlers[0]) ||
                    (active_action != NULL &&
                     (active_action->code.count == 0 ||
                      active_action->code.last_opens)))
                    return nativeScriptCode_Fail(error, line_number,
                                "Incomplete or excess action handler.");
                char name[64];
                size_t name_size = size - 8;
                if (name_size >= sizeof(name))
                    return nativeScriptCode_Fail(error, line_number,
                                "Action handler name is too long.");
                memcpy(name, line + 7, name_size);
                name[name_size] = '\0';
                unsigned motion;
                const char* field = NULL;
                if (!state_handler_target(name, &motion, &field) &&
                    !state_handler_action_allowed(name) &&
                    strcmp(name, "damage") != 0 &&
                    strcmp(name, "secondary_damage") != 0 &&
                    strcmp(name, "death2") != 0 &&
                    strcmp(name, "pre_hitlag") != 0 &&
                    strcmp(name, "post_hitlag") != 0 &&
                    strcmp(name, "hurtbox_detect") != 0 &&
                    strcmp(name, "deal_damage") != 0 &&
                    strcmp(name, "state_change") != 0 &&
                    strcmp(name, "reflect") != 0 &&
                    strcmp(name, "reflect_hit") != 0 &&
                    strcmp(name, "grab_catch") != 0 &&
                    strcmp(name, "item_catch") != 0 &&
                    strcmp(name, "grabbed") != 0)
                    return nativeScriptCode_Fail(error, line_number,
                                "Expected a motion-state or fighter lifecycle callback.");
                for (unsigned i = 0; i < action_handler_count; ++i)
                    if (strcmp(action_handlers[i].name, name) == 0)
                        return nativeScriptCode_Fail(error, line_number,
                                    "Duplicate action state handler.");
                active_action = &action_handlers[action_handler_count++];
                strcpy(active_action->name, name);
                active_action->line = line_number;
                active_action->code.predicate = state_handler_is_predicate(name);
            active_action->code.floating_result = strcmp(name, "shield_scale") == 0 || strcmp(name, "catch_pull_frame") == 0;
            active_action->code.shield_transition_event = strcmp(name, "shield_transition") == 0;
                active_action->code.motion_event = strcmp(name, "motion_is_angled") == 0 ||
                                                   strcmp(name, "jab_restart") == 0 ||
                                                   strcmp(name, "throw_victim_motion") == 0 || strcmp(name, "aerial_attack_motion") == 0;
                active_action->code.victim_motion_event = strcmp(name, "throw_victim_motion") == 0;
                active_action->code.held_motion_event = strcmp(name, "held_motion") == 0;
                active_action->code.air_resume_event = strcmp(name, "air_resume_motion") == 0;
                active_action->code.neutral_enter_event = strcmp(name, "neutral_enter") == 0;
                active_action->code.landing_action_event = strcmp(name, "landing_action") == 0;
            active_action->code.item_swing_event = strcmp(name, "item_swing_motion") == 0;
                active_action->code.demo_event = strcmp(name, "demo_enter") == 0;
                active_action->code.action_metadata_event = strcmp(name, "action_metadata") == 0;
                active_action->code.cpu_prepare_event = strcmp(name, "cpu_prepare") == 0;
            active_action->code.strict_integer_result = active_action->code.cpu_prepare_event ||
                strcmp(name, "cpu_stick_step") == 0 || strcmp(name, "cpu_stick_limit") == 0;
            active_action->code.strict_boolean_result = strcmp(name, "cpu_drop_command") == 0 ||
                strcmp(name, "cpu_mode_update") == 0 || strcmp(name, "cpu_output") == 0;
            active_action->code.integer_result = active_action->code.strict_integer_result || strcmp(name, "item_swing_motion") == 0 ||
                                                    strcmp(name, "action_metadata") == 0 ||
                                                    strcmp(name, "throw_victim_motion") == 0 || strcmp(name, "held_motion") == 0 ||
                                                    strcmp(name, "air_resume_motion") == 0 ||
                                                    strcmp(name, "dynamics_interaction_start") == 0 ||
                                                    strcmp(name, "cpu_projectile_hit") == 0 ||
                                                    strcmp(name, "jab_start") == 0 || strcmp(name, "aerial_attack_motion") == 0;
                active_action->code.trail_event = strcmp(name, "trail") == 0;
                active_action->code.scalar_event = strcmp(name, "cpu_spacing_update") == 0;
                active_action->code.smash_angle_event = strcmp(name, "forward_smash_enter") == 0;
                active_action->code.target_geometry_event = strcmp(name, "cpu_special_candidate") == 0;
                active_action->code.cpu_projectile_defense_event = strcmp(name, "cpu_projectile_defense") == 0;
                active_action->code.cpu_projectile_hit_event = strcmp(name, "cpu_projectile_hit") == 0;
                active_action->code.other_fighter_event = strcmp(name, "cpu_threat") == 0 || strcmp(name, "capture_wait_setup") == 0 || strcmp(name, "capture_wait_enter") == 0;
                active_action->code.dynamics_set_event = strcmp(name, "dynamics_interaction_start") == 0;
                active_action->code.state = &state;
                active_action->code.functions = functions;
                active_action->code.articles = nativeFighterItemScript_ArticleCatalog(items);
                active_action->code.special_program = true;
                active_action->code.item_event =
                    state_handler_is_item_event(name);
                active_action->code.grabbed_event =
                    strcmp(name, "grabbed") == 0;
                active_action->code.grab_catch_event =
                    strcmp(name, "grab_catch") == 0;
                active_action->code.collision =
                    field != NULL && strcmp(field, "coll_cb") == 0;
                active_action->code.motion_state_first = 0;
                active_action->code.motion_state_count = 65536;
                continue;
            }
            if (active_action == NULL ||
                !nativeScriptCode_AddLine(&active_action->code, line))
                return nativeScriptCode_Fail(error, line_number,
                            "Invalid action statement or indentation.");
            continue;
        }
        if (phase == 2 && strncmp(line, "move ", 5) == 0) {
            if (!nativeScriptCode_HandlerComplete(&move, active_handler))
                return nativeScriptCode_Fail(error, line_number,
                            "Previous handler needs an operation.");
            if (move_count == 4)
                return nativeScriptCode_Fail(error, line_number,
                            "Only four special moves are supported.");
            moves[move_count++] = move;
            move = (ScriptMove){ 0 };
            move.ground_entry_code.state = &state;
            move.ground_entry_code.functions = functions;
            move.ground_entry_code.articles = nativeFighterItemScript_ArticleCatalog(items);
            move.ground_entry_code.entry = true;
            move.air_entry_code.state = &state;
            move.air_entry_code.functions = functions;
            move.air_entry_code.articles = nativeFighterItemScript_ArticleCatalog(items);
            move.air_entry_code.entry = true;
            move.air_entry_code.air_entry = true;
            move.ground_animation.state = &state;
            move.ground_animation.functions = functions;
            move.ground_animation.articles = nativeFighterItemScript_ArticleCatalog(items);
            move.air_animation.state = &state;
            move.air_animation.functions = functions;
            move.air_animation.articles = nativeFighterItemScript_ArticleCatalog(items);
            move.ground_interrupt.state = &state;
            move.ground_interrupt.functions = functions;
            move.ground_interrupt.articles = nativeFighterItemScript_ArticleCatalog(items);
            move.air_interrupt.state = &state;
            move.air_interrupt.functions = functions;
            move.air_interrupt.articles = nativeFighterItemScript_ArticleCatalog(items);
            move.ground_physics.state = &state;
            move.ground_physics.functions = functions;
            move.ground_physics.articles = nativeFighterItemScript_ArticleCatalog(items);
            move.air_physics.state = &state;
            move.air_physics.functions = functions;
            move.air_physics.articles = nativeFighterItemScript_ArticleCatalog(items);
            move.ground_collision.state = &state;
            move.ground_collision.functions = functions;
            move.ground_collision.articles = nativeFighterItemScript_ArticleCatalog(items);
            move.ground_collision.collision = true;
            move.air_collision.state = &state;
            move.air_collision.functions = functions;
            move.air_collision.articles = nativeFighterItemScript_ArticleCatalog(items);
            move.air_collision.collision = true;
            move.air_collision.air_entry = true;
            move.neutral_cue_code.state = &state;
            move.neutral_cue_code.functions = functions;
            move.neutral_cue_code.articles = nativeFighterItemScript_ArticleCatalog(items);
            move.accessory_code.state = &state;
            move.accessory_code.functions = functions;
            move.accessory_code.articles = nativeFighterItemScript_ArticleCatalog(items);
            phase = 0;
            active_handler = 0;
        }

        if (strcmp(line, "    entry explicit") == 0) {
            if (phase != 1 || move.explicit_entry || active_handler != 0)
                return nativeScriptCode_Fail(error, line_number, "Entry policy belongs before move handlers.");
            move.explicit_entry = true;
            continue;
        }
        if (phase == 0) {
            if (strncmp(line, "state ", 6) == 0) {
                if (move_count != 0 ||
                    !nativeFighterScript_AddStateField(&state, line))
                    return nativeScriptCode_Fail(error, line_number,
                                "Expected a unique `state bool/float/int Name` before moves.");
                continue;
            }
            if (strncmp(line, "attribute ", 10) == 0) {
                if (!nativeScriptCode_AddAttribute(&state, line))
                    return nativeScriptCode_Fail(error, line_number,
                        "Expected `attribute float/int/bool Name BYTE_OFFSET` within fighter attributes.");
                continue;
            }
            if (size < 7 || strncmp(line, "move ", 5) != 0 ||
                line[size - 1] != ':' ||
                !nativeScriptCode_Identifier(line + 5, size - 6, sizeof(move.name)))
                return nativeScriptCode_Fail(error, line_number, "Expected `move Name:`.");
            memcpy(move.name, line + 5, size - 6);
            phase = 1;
        } else if (phase == 1 ||
                   (phase == 2 && strncmp(line, "    on ", 7) == 0)) {
            if (phase == 2 && !nativeScriptCode_HandlerComplete(&move, active_handler))
                return nativeScriptCode_Fail(error, line_number,
                            "Previous handler needs an operation.");
            ScriptSpecial selected = ScriptSpecial_Unset;
            unsigned handler = 0;
            if (strcmp(line, "    on special_n:") == 0) {
                selected = ScriptSpecial_Neutral; handler = 1;
            } else if (strcmp(line, "    on special_air_n:") == 0) {
                selected = ScriptSpecial_Neutral; handler = 2;
            } else if (strcmp(line, "    on special_s:") == 0) {
                selected = ScriptSpecial_Side; handler = 1;
            } else if (strcmp(line, "    on special_air_s:") == 0) {
                selected = ScriptSpecial_Side; handler = 2;
            } else if (strcmp(line, "    on special_hi:") == 0) {
                selected = ScriptSpecial_Up; handler = 1;
            } else if (strcmp(line, "    on special_air_hi:") == 0) {
                selected = ScriptSpecial_Up; handler = 2;
            } else if (strcmp(line, "    on special_lw:") == 0) {
                selected = ScriptSpecial_Down; handler = 1;
            } else if (strcmp(line, "    on special_air_lw:") == 0) {
                selected = ScriptSpecial_Down; handler = 2;
            } else if (strcmp(line, "    on neutral_special_cue:") == 0) {
                selected = ScriptSpecial_Neutral; handler = 3;
            } else if (strcmp(line, "    on ground_animation:") == 0) {
                selected = move.special; handler = 4;
            } else if (strcmp(line, "    on air_animation:") == 0) {
                selected = move.special; handler = 5;
            } else if (strcmp(line, "    on ground_interrupt:") == 0) {
                selected = move.special; handler = 6;
            } else if (strcmp(line, "    on air_interrupt:") == 0) {
                selected = move.special; handler = 7;
            } else if (strcmp(line, "    on ground_physics:") == 0) {
                selected = move.special; handler = 8;
            } else if (strcmp(line, "    on air_physics:") == 0) {
                selected = move.special; handler = 9;
            } else if (strcmp(line, "    on ground_collision:") == 0) {
                selected = move.special; handler = 10;
            } else if (strcmp(line, "    on air_collision:") == 0) {
                selected = move.special; handler = 11;
            } else if (strcmp(line, "    on accessory:") == 0) {
                selected = move.special; handler = 12;
            }
            if (selected == ScriptSpecial_Unset ||
                (move.special != ScriptSpecial_Unset &&
                 move.special != selected) ||
                (handler == 1 && move.ground) ||
                (handler == 2 && move.air) ||
                (handler == 3 && (move.cue || move.accessory_code.count != 0)) ||
                (handler == 4 && move.ground_animation.count != 0) ||
                (handler == 5 && move.air_animation.count != 0) ||
                (handler == 6 && move.ground_interrupt.count != 0) ||
                (handler == 7 && move.air_interrupt.count != 0) ||
                (handler == 8 && move.ground_physics.count != 0) ||
                (handler == 9 && move.air_physics.count != 0) ||
                (handler == 10 && move.ground_collision.count != 0) ||
                (handler == 11 && move.air_collision.count != 0) ||
                (handler == 12 && (move.accessory_code.count != 0 || move.cue))) {
                return nativeScriptCode_Fail(error, line_number,
                            "Expected an unused handler for this special.");
            }
            if (move.special == ScriptSpecial_Unset)
                move.special_line = line_number;
            move.special = selected;
            if (handler == 1) move.ground_entry_code.special = selected;
            if (handler == 2) move.air_entry_code.special = selected;
            if (handler == 4) move.ground_animation.special = selected;
            if (handler == 5) move.air_animation.special = selected;
            if (handler == 6) move.ground_interrupt.special = selected;
            if (handler == 7) move.air_interrupt.special = selected;
            if (handler == 8) move.ground_physics.special = selected;
            if (handler == 9) move.air_physics.special = selected;
            if (handler == 10) move.ground_collision.special = selected;
            if (handler == 11) move.air_collision.special = selected;
            if (handler == 12) move.accessory_code.special = selected;
            active_handler = handler;
            if (handler == 1) move.ground = true;
            if (handler == 2) move.air = true;
            if (handler == 3) move.cue = true;
            phase = 2;
        } else {
            if (active_handler == 4 || active_handler == 5) {
                ScriptCodeBlock* block = active_handler == 4
                    ? &move.ground_animation : &move.air_animation;
                if (!nativeScriptCode_AddLine(block, line))
                    return nativeScriptCode_Fail(error, line_number,
                                "Invalid animation statement, expression or indentation.");
                continue;
            }
            if (active_handler == 8 || active_handler == 9) {
                ScriptCodeBlock* block = active_handler == 8
                    ? &move.ground_physics : &move.air_physics;
                if (!nativeScriptCode_AddLine(block, line))
                    return nativeScriptCode_Fail(error, line_number,
                                "Invalid physics statement, expression or indentation.");
                continue;
            }
            if (active_handler == 10 || active_handler == 11) {
                ScriptCodeBlock* block = active_handler == 10
                    ? &move.ground_collision : &move.air_collision;
                if (!nativeScriptCode_AddLine(block, line))
                    return nativeScriptCode_Fail(error, line_number,
                                "Invalid collision statement, expression or indentation.");
                continue;
            }
            if (active_handler == 12) {
                if (!nativeScriptCode_AddLine(&move.accessory_code, line))
                    return nativeScriptCode_Fail(error, line_number,
                                    "Invalid accessory statement, expression or indentation.");
                continue;
            }
            if (active_handler == 3 &&
                (strncmp(line, "        let ", 12) == 0 ||
                 (strncmp(line, "        state.", 14) == 0 &&
                  strstr(line, " = ") != NULL) ||
                 strncmp(line, "        articles.spawn_projectile(", 34) == 0 ||
                 strncmp(line, "        articles.launch_projectile(", 35) == 0 ||
                 strncmp(line, "        articles.spawn_authored_projectile(",
                         43) == 0)) {
                if (!nativeScriptCode_AddLine(&move.neutral_cue_code, line))
                    return nativeScriptCode_Fail(error, line_number,
                                "Invalid neutral cue statement or expression.");
                continue;
            }
            if (active_handler == 6 || active_handler == 7) {
                ScriptCodeBlock* block = active_handler == 6
                    ? &move.ground_interrupt : &move.air_interrupt;
                if (!nativeScriptCode_AddLine(block, line))
                    return nativeScriptCode_Fail(error, line_number,
                                "Invalid interrupt statement, expression or indentation.");
                continue;
            }
            if (active_handler == 1 || active_handler == 2) {
                if (strncmp(line, "        ", 8) != 0)
                    return nativeScriptCode_Fail(error, line_number,
                                "Entry statement needs handler indentation.");
                ScriptCodeBlock* block = active_handler == 1
                    ? &move.ground_entry_code : &move.air_entry_code;
                ScriptEntry* entry = active_handler == 1
                    ? &move.ground_entry : &move.air_entry;
                const char* action = line + 8;
                char authored[64], authored_air[64];
                snprintf(authored, sizeof(authored), "state.enter(%sSpecial)",
                         nativeScriptCode_SpecialEnum(move.special));
                snprintf(authored_air, sizeof(authored_air),
                         "state.enter(Air%sSpecial)", nativeScriptCode_SpecialEnum(move.special));
                bool explicit_entry =
                    strstr(action, "state.enter_flags(") != NULL;
                if (explicit_entry) {
                    if (*entry == ScriptEntry_Stock)
                        return nativeScriptCode_Fail(error, line_number,
                                    "Cannot combine inherited and authored entry.");
                    *entry = ScriptEntry_Authored;
                }
                bool stock = strcmp(action, "stock.enter()") == 0;
                bool state = strcmp(action, authored) == 0 ||
                    (active_handler == 1 &&
                     strcmp(action, authored_air) == 0);
                if (!stock && !state &&
                    (strstr(line, "stock.enter()") != NULL ||
                     strstr(line, "state.enter(") != NULL))
                    return nativeScriptCode_Fail(error, line_number,
                                "State entry must be a top-level statement.");
                if (stock || state) {
                    if (*entry != ScriptEntry_None ||
                        (stock && block->count != 0))
                        return nativeScriptCode_Fail(error, line_number,
                                    "Entry must occur once; stock entry must come first.");
                    *entry = stock ? ScriptEntry_Stock : ScriptEntry_Authored;
                }
                if (*entry == ScriptEntry_Stock && !stock &&
                    strncmp(action, "motion.", 7) != 0)
                    return nativeScriptCode_Fail(error, line_number,
                                "Inherited entry only accepts motion after stock.enter().");
                if (!nativeScriptCode_AddLine(block, line))
                    return nativeScriptCode_Fail(error, line_number,
                                "Invalid entry statement, expression or indentation.");
                // Explicit entry owns the whole sequence, including a helper
                // that chooses and enters a state. No implicit entry is emitted.
                if (move.explicit_entry && *entry == ScriptEntry_None)
                    *entry = ScriptEntry_Authored;
                continue;
            }
            if (strcmp(line, "        state.once()") == 0) {
                if (active_handler != 3 || move.cue_once ||
                    move.stock_article || move.fireball_bone != NULL ||
                    move.effect_bone != NULL || move.cue_speed_set ||
                    move.cue_vertical_speed_set)
                    return nativeScriptCode_Fail(error, line_number,
                                "State once must begin the cue handler.");
                move.cue_once = true;
                continue;
            }
            if (strcmp(line, "        stock.neutral_article()") == 0) {
                if (move.special != ScriptSpecial_Neutral ||
                    active_handler != 3 || move.stock_article ||
                    move.fireball_bone != NULL ||
                    move.cue_speed_set || move.cue_vertical_speed_set)
                    return nativeScriptCode_Fail(error, line_number,
                                "Stock article must precede the cue impulse.");
                move.stock_article = true;
                continue;
            }
            static const char article_prefix[] =
                "        articles.spawn(MarioFireball, ";
            if (strncmp(line, article_prefix, sizeof(article_prefix) - 1) == 0) {
                if (active_handler != 3 || move.stock_article ||
                    move.fireball_bone != NULL || move.cue_speed_set ||
                    move.cue_vertical_speed_set || line[size - 1] != ')')
                    return nativeScriptCode_Fail(error, line_number,
                                "Fireball must precede cue movement.");
                move.fireball_bone = nativeScriptCode_BoneSymbol(
                    line + sizeof(article_prefix) - 1,
                    size - sizeof(article_prefix));
                if (move.fireball_bone == NULL)
                    return nativeScriptCode_Fail(error, line_number,
                                "Expected LeftHand, RightHand, or Hip bone.");
                continue;
            }
            static const char effect_prefix[] = "        effects.spawn(";
            if (strncmp(line, effect_prefix, sizeof(effect_prefix) - 1) == 0) {
                if (active_handler != 3 || move.effect_bone != NULL ||
                    move.cue_speed_set || move.cue_vertical_speed_set)
                    return nativeScriptCode_Fail(error, line_number,
                                "Effect must precede cue movement.");
                char* tail = NULL;
                errno = 0;
                unsigned long id = strtoul(
                    line + sizeof(effect_prefix) - 1, &tail, 10);
                if (errno != 0 || id > 65535 || tail == NULL ||
                    strncmp(tail, ", ", 2) != 0 || line[size - 1] != ')')
                    return nativeScriptCode_Fail(error, line_number,
                                "Expected effects.spawn(number, Bone).");
                move.effect_bone = nativeScriptCode_BoneSymbol(
                    tail + 2, (size_t) (line + size - 1 - (tail + 2)));
                if (move.effect_bone == NULL)
                    return nativeScriptCode_Fail(error, line_number,
                                "Expected LeftHand, RightHand, or Hip bone.");
                move.effect_id = (unsigned) id;
                continue;
            }
            static const char* speed_prefixes[4] = {
                "        motion.add_facing_speed(",
                "        motion.add_vertical_speed(",
                "        motion.set_facing_speed(",
                "        motion.set_vertical_speed("
            };
            const char* prefix = NULL;
            bool vertical = false, replace = false;
            for (unsigned operation = 0; operation < 4; ++operation)
                if (strncmp(line, speed_prefixes[operation],
                            strlen(speed_prefixes[operation])) == 0) {
                    prefix = speed_prefixes[operation];
                    vertical = (operation & 1u) != 0;
                    replace = (operation & 2u) != 0;
                    break;
                }
            if (prefix == NULL)
                return nativeScriptCode_Fail(error, line_number,
                            "Expected motion.add_*_speed(number) or motion.set_*_speed(number).");
            size_t prefix_size = strlen(prefix);
            if (size <= prefix_size + 1 ||
                line[size - 1] != ')')
                return nativeScriptCode_Fail(error, line_number,
                            "Expected a speed operation with one numeric argument.");
            line[size - 1] = '\0';
            char* number = line + prefix_size;
            char* tail = NULL;
            errno = 0;
            float speed = strtof(number, &tail);
            if (errno != 0 || tail == number || *tail != '\0' ||
                !isfinite(speed) || speed < -20.0f || speed > 20.0f)
                return nativeScriptCode_Fail(error, line_number,
                            "Speed must be a finite number from -20 to 20.");
            if (vertical && move.cue_vertical_speed_set)
                return nativeScriptCode_Fail(error, line_number,
                            "Only one cue vertical speed operation is supported.");
            if (!vertical && move.cue_speed_set)
                return nativeScriptCode_Fail(error, line_number,
                            "Only one cue speed operation is supported.");
            if (vertical) {
                move.cue_vertical_speed_set = true;
                move.cue_vertical_speed_replace = replace;
                move.cue_vertical_speed = speed;
            } else {
                move.cue_speed_set = true;
                move.cue_speed_replace = replace;
                move.cue_speed = speed;
            }
        }
    }
    if (phase == 2 && !nativeScriptCode_HandlerComplete(&move, active_handler))
        return nativeScriptCode_Fail(error, line_number,
                    "Handler needs an operation.");
    if (phase == 3 && (active_action == NULL ||
                       active_action->code.count == 0 ||
                       active_action->code.last_opens))
        return nativeScriptCode_Fail(error, line_number,
                    "Action needs a complete state handler.");
    if (phase != 2 && phase != 3 && functions->export_count == 0 &&
        nativeFighterItemScript_Count(items) == 0 &&
        nativeFighterItemScript_ArticleCount(items) == 0)
        return nativeScriptCode_Fail(error, line_number, "Move needs at least one handler.");
    if (phase == 2) {
        if (!move.ground && !move.air)
            return nativeScriptCode_Fail(error, line_number,
                        "Move needs at least one handler.");
        if (move_count == 4)
            return nativeScriptCode_Fail(error, line_number,
                        "Only four special moves are supported.");
        moves[move_count++] = move;
    }
    if (!nativeScriptFunctions_Compile(functions, &state, nativeFighterItemScript_ArticleCatalog(items), true, 0, 65536, error))
        return false;
    if (!nativeScriptModelEffect_Compile(functions->model_effects, &state,
        nativeFighterItemScript_ArticleCatalog(items), functions, error)) return false;
    if (independent &&
        !independent_load_handlers(action_handlers, action_handler_count, error))
        return false;
    if (independent)
        for (unsigned i = 0; i < move_count; ++i)
            if (moves[i].ground_entry == ScriptEntry_Stock ||
                moves[i].air_entry == ScriptEntry_Stock || moves[i].stock_article)
                return nativeScriptCode_Fail(error, moves[i].special_line,
                    "Independent behavior cannot delegate stock entry or article handlers.");
    bool has_items = nativeFighterItemScript_Count(items) != 0;
    bool has_articles = nativeFighterItemScript_ArticleCount(items) != 0;
    bool any_authored = action_handler_count != 0 || policies->count != 0;
    bool has_secondary_damage = false, uses_secondary_damage = false;
    bool has_damage = false, has_death1 = false, has_death2 = false, has_detect = false;
    bool has_pre_hitlag = false, has_post_hitlag = false;
    bool uses_hitlag = false;
    bool has_deal_damage = false;
    bool has_state_change = false, uses_state_change = false;
    bool has_reflect = false, uses_reflect = false;
    bool has_reflect_hit = false, uses_reflect_hit = false;
    bool has_grab_catch = false, has_item_catch = false, has_grabbed = false, uses_grab = false;
    bool uses_damage = false, uses_death1 = false, uses_death2 = false, uses_detect = false;
    bool uses_deal_damage = false;
    for (unsigned i = 0; i < action_handler_count; ++i) {
        has_damage |= strcmp(action_handlers[i].name, "damage") == 0;
        has_secondary_damage |= strcmp(action_handlers[i].name, "secondary_damage") == 0;
        has_death1 |= strcmp(action_handlers[i].name, "death1") == 0;
        has_death2 |= strcmp(action_handlers[i].name, "death2") == 0;
        has_pre_hitlag |= strcmp(action_handlers[i].name, "pre_hitlag") == 0;
        has_post_hitlag |= strcmp(action_handlers[i].name, "post_hitlag") == 0;
        has_detect |= strcmp(action_handlers[i].name, "hurtbox_detect") == 0;
        has_deal_damage |= strcmp(action_handlers[i].name, "deal_damage") == 0;
        has_state_change |= strcmp(action_handlers[i].name,
                                   "state_change") == 0;
        has_reflect |= strcmp(action_handlers[i].name, "reflect") == 0;
        has_reflect_hit |= strcmp(action_handlers[i].name, "reflect_hit") == 0;
        has_grab_catch |= strcmp(action_handlers[i].name,
                                 "grab_catch") == 0;
        has_item_catch |= strcmp(action_handlers[i].name, "item_catch") == 0;
        has_grabbed |= strcmp(action_handlers[i].name,
                              "grabbed") == 0;
    }
    for (unsigned i = 0; i < move_count; ++i) {
        const ScriptCodeBlock* blocks[] = {
            &moves[i].ground_entry_code, &moves[i].air_entry_code,
            &moves[i].ground_animation, &moves[i].air_animation,
            &moves[i].ground_interrupt, &moves[i].air_interrupt,
            &moves[i].ground_physics, &moves[i].air_physics,
            &moves[i].ground_collision, &moves[i].air_collision,
            &moves[i].neutral_cue_code, &moves[i].accessory_code,
        };
        for (unsigned j = 0; j < sizeof(blocks) / sizeof(blocks[0]); ++j)
            for (unsigned k = 0; k < blocks[j]->count; ++k) {
                uses_damage |= strstr(blocks[j]->lines[k].text,
                                      "callbacks.track_damage()") != NULL;
                uses_secondary_damage |= strstr(blocks[j]->lines[k].text,
                                      "callbacks.track_secondary_damage()") != NULL;
                uses_death1 |= strstr(blocks[j]->lines[k].text,
                                      "callbacks.track_death1()") != NULL;
                uses_death2 |= (strstr(blocks[j]->lines[k].text, "callbacks.track_death2()") != NULL || strstr(blocks[j]->lines[k].text, "callbacks.track_death3()") != NULL);
                uses_hitlag |= strstr(blocks[j]->lines[k].text,
                                      "callbacks.track_hitlag()") != NULL;
                uses_detect |= strstr(blocks[j]->lines[k].text,
                                      "callbacks.track_hurtbox_detect()") != NULL;
                uses_deal_damage |= strstr(blocks[j]->lines[k].text,
                                      "callbacks.track_deal_damage()") != NULL;
                uses_reflect |= strstr(blocks[j]->lines[k].text,
                                      "callbacks.track_reflect()") != NULL;
                uses_reflect_hit |= strstr(blocks[j]->lines[k].text, "callbacks.track_reflect_hit()") != NULL ||
                strstr(blocks[j]->lines[k].text, "guard.install_reflector()") != NULL;
                uses_state_change |= strstr(blocks[j]->lines[k].text,
                                      "callbacks.track_state_change()") != NULL;
                uses_grab |= strstr(blocks[j]->lines[k].text,
                                    "capture.arm(") != NULL;
            }
    }
    for (unsigned i = 0; i < action_handler_count; ++i)
        for (unsigned j = 0; j < action_handlers[i].code.count; ++j) {
            uses_damage |= strstr(action_handlers[i].code.lines[j].text,
                                  "callbacks.track_damage()") != NULL;
            uses_secondary_damage |= strstr(action_handlers[i].code.lines[j].text,
                                  "callbacks.track_secondary_damage()") != NULL;
            uses_death1 |= strstr(action_handlers[i].code.lines[j].text,
                                  "callbacks.track_death1()") != NULL;
            uses_death2 |= (strstr(action_handlers[i].code.lines[j].text, "callbacks.track_death2()") != NULL || strstr(action_handlers[i].code.lines[j].text, "callbacks.track_death3()") != NULL);
            uses_hitlag |= strstr(action_handlers[i].code.lines[j].text,
                                  "callbacks.track_hitlag()") != NULL;
            uses_detect |= strstr(action_handlers[i].code.lines[j].text,
                                  "callbacks.track_hurtbox_detect()") != NULL;
            uses_deal_damage |= strstr(action_handlers[i].code.lines[j].text,
                                  "callbacks.track_deal_damage()") != NULL;
            uses_reflect |= strstr(action_handlers[i].code.lines[j].text,
                                  "callbacks.track_reflect()") != NULL;
            uses_reflect_hit |= strstr(action_handlers[i].code.lines[j].text, "callbacks.track_reflect_hit()") != NULL ||
                strstr(action_handlers[i].code.lines[j].text, "guard.install_reflector()") != NULL;
            uses_state_change |= strstr(action_handlers[i].code.lines[j].text,
                                  "callbacks.track_state_change()") != NULL;
        }
    for (unsigned i = 0; i < functions->count; ++i) {
        const ScriptCodeBlock* code = &functions->functions[i].code;
        uses_damage |= nativeScriptCode_Contains(code, "callbacks.track_damage()");
        uses_secondary_damage |= nativeScriptCode_Contains(code, "callbacks.track_secondary_damage()");
        uses_death1 |= nativeScriptCode_Contains(code, "callbacks.track_death1()");
        uses_death2 |= (nativeScriptCode_Contains(code, "callbacks.track_death2()") || nativeScriptCode_Contains(code, "callbacks.track_death3()"));
        uses_hitlag |= nativeScriptCode_Contains(code, "callbacks.track_hitlag()");
        uses_detect |= nativeScriptCode_Contains(code, "callbacks.track_hurtbox_detect()");
        uses_deal_damage |= nativeScriptCode_Contains(code, "callbacks.track_deal_damage()");
        uses_state_change |= nativeScriptCode_Contains(code, "callbacks.track_state_change()");
        uses_reflect |= nativeScriptCode_Contains(code, "callbacks.track_reflect()");
        uses_reflect_hit |= nativeScriptCode_Contains(code, "callbacks.track_reflect_hit()") ||
            nativeScriptCode_Contains(code, "guard.install_reflector()");
        uses_grab |= nativeScriptCode_Contains(code, "capture.arm(");
    }
    if (uses_secondary_damage && !has_secondary_damage)
        return nativeScriptCode_Fail(error, line_number,
                    "callbacks.track_secondary_damage() needs an `on secondary_damage:` handler.");
    if (uses_damage && !has_damage)
        return nativeScriptCode_Fail(error, line_number,
                    "callbacks.track_damage() needs an `on damage:` handler.");
    if (uses_death1 && !has_death1)
        return nativeScriptCode_Fail(error, line_number,
                    "callbacks.track_death1() needs an `on death1:` handler.");
    if (uses_death2 && !has_death2)
        return nativeScriptCode_Fail(error, line_number,
                    "callbacks.track_death2() needs an `on death2:` handler.");
    if (uses_hitlag && (!has_pre_hitlag || !has_post_hitlag))
        return nativeScriptCode_Fail(error, line_number,
                    "callbacks.track_hitlag() needs pre/post hitlag handlers.");
    if (uses_detect && !has_detect)
        return nativeScriptCode_Fail(error, line_number,
                    "callbacks.track_hurtbox_detect() needs an `on hurtbox_detect:` handler.");
    if (uses_deal_damage && !has_deal_damage)
        return nativeScriptCode_Fail(error, line_number,
                    "callbacks.track_deal_damage() needs an `on deal_damage:` handler.");
    if (uses_reflect && !has_reflect)
        return nativeScriptCode_Fail(error, 0,
                    "callbacks.track_reflect() needs an `on reflect:` handler.");
    if (uses_reflect_hit && !has_reflect_hit)
        return nativeScriptCode_Fail(error, 0,
                    "callbacks.track_reflect_hit() needs an `on reflect_hit:` handler.");
    if (uses_state_change && !has_state_change)
        return nativeScriptCode_Fail(error, line_number,
                    "callbacks.track_state_change() needs an `on state_change:` handler.");
    if (uses_grab && (!has_grab_catch || !has_grabbed))
        return nativeScriptCode_Fail(error, line_number,
                    "capture.arm() needs `on grab_catch:` and `on grabbed:` handlers.");
    unsigned seen_specials = 0;
    const ScriptMove* neutral = NULL;
    for (unsigned i = 0; i < move_count; ++i) {
        const ScriptMove* current = &moves[i];
        unsigned bit = 1u << current->special;
        if (seen_specials & bit)
            return nativeScriptCode_Fail(error, current->special_line,
                        "Each special may appear in only one move block.");
        seen_specials |= bit;
        bool ground_authored = current->ground_entry == ScriptEntry_Authored;
        bool air_authored = current->air_entry == ScriptEntry_Authored;
        bool authored = ground_authored || air_authored;
        if (authored && (current->ground_entry == ScriptEntry_Stock ||
                         current->air_entry == ScriptEntry_Stock))
            return nativeScriptCode_Fail(error, current->special_line,
                        "Cannot mix authored and inherited entry handlers.");
        bool phases = current->ground_animation.count ||
                      current->air_animation.count ||
                      current->ground_interrupt.count ||
                      current->air_interrupt.count ||
                      current->ground_physics.count || current->air_physics.count ||
                      current->ground_collision.count ||
                      current->air_collision.count ||
                      current->accessory_code.count;
        if (phases && !authored)
            return nativeScriptCode_Fail(error, current->special_line,
                        "State phase handlers need an authored entry.");
        if (phases &&
            ((ground_authored && (!current->ground_animation.count ||
              !current->ground_interrupt.count || !current->ground_physics.count ||
              !current->ground_collision.count)) ||
             (air_authored && (!current->air_animation.count ||
              !current->air_interrupt.count || !current->air_physics.count ||
              !current->air_collision.count))))
            return nativeScriptCode_Fail(error, current->special_line,
                        "A script-owned special needs every supported ground/air phase.");
        if ((!ground_authored && (current->ground_animation.count ||
              current->ground_interrupt.count || current->ground_physics.count ||
              current->ground_collision.count)) ||
            (!air_authored && (current->air_animation.count ||
              current->air_interrupt.count || current->air_physics.count ||
              current->air_collision.count)))
            return nativeScriptCode_Fail(error, current->special_line,
                        "Ground/air phase has no authored entry.");
        if (current->cue && !authored)
            return nativeScriptCode_Fail(error, current->special_line,
                        "Cue handler needs authored ground and air states.");
        if (current->special == ScriptSpecial_Neutral) neutral = current;
        any_authored |= authored;
    }

    fputs("#include <melee/ft/native_fighter_module.h>\n"
          "#include <melee/it/forward.h>\n"
          "#include <melee/mp/forward.h>\n"
          "#include <melee/ft/kinds/ftMario/types.h>\n"
          "#include <melee/ft/kinds/ftCommon/types.h>\n"
          "#include <melee/ft/types.h>\n"
          "#include <melee/lb/types.h>\n"
          "#include <sysdolphin/baselib/controller.h>\n"
          "#include <sysdolphin/baselib/gobj.h>\n"
          "#include <sysdolphin/baselib/jobj.h>\n"
          "#include <math.h>\n"
          "#include <string.h>\n"
          "static NativeFighterDefinition generated;\n"
          "static unsigned script_state_base;\n"
          "static int script_signbit(float value) {\n"
          "    uint32_t bits; memcpy(&bits, &value, sizeof(bits));\n"
          "    return (int) (bits >> 31);\n}\n", output);
    emit_project_parameters(parameters, output);
    if (any_authored || has_items || has_articles || functions->count != 0)
        fputs("static const NativeFighterHost* engine;\n", output);
    char item_host_checks[4096];
    if (!nativeFighterItemScript_HostChecks(items, item_host_checks,
                                            sizeof(item_host_checks)))
        return nativeScriptCode_Fail(error, 0, "Too many item host requirements.");
    if (any_authored)
        fputs("static MotionState authored_states[1024];\n"
              "static HSD_GObjEvent transition_cue[NativeFighterSpecial_Count];\n"
              "static HSD_GObjEvent transition_exit[NativeFighterSpecial_Count];\n",
              output);
    if (has_secondary_damage)
        fputs("static void authored_secondary_damage(HSD_GObj* gobj);\n", output);
    if (has_damage)
        fputs("static void authored_damage(HSD_GObj* gobj);\n", output);
    if (has_death2)
        fputs("static void authored_death2(HSD_GObj* gobj);\n", output);
    if (has_pre_hitlag)
        fputs("static void authored_pre_hitlag(HSD_GObj* gobj);\n", output);
    if (has_post_hitlag)
        fputs("static void authored_post_hitlag(HSD_GObj* gobj);\n", output);
    if (has_detect)
        fputs("static void authored_hurtbox_detect(HSD_GObj* gobj);\n", output);
    if (has_deal_damage)
        fputs("static void authored_deal_damage(HSD_GObj* gobj);\n", output);
    if (has_reflect)
        fputs("static void authored_reflect(HSD_GObj* gobj);\n", output);
    if (has_reflect_hit)
        fputs("static void authored_reflect_hit(HSD_GObj* gobj);\n", output);
    if (has_state_change)
        fputs("static void authored_state_change(HSD_GObj* gobj);\n", output);
    if (!has_item_catch && uses_grab)
        fputs("#define authored_item_catch NULL\n", output);
    if (has_grab_catch)
        fputs("static void authored_grab_catch(HSD_GObj* gobj);\n", output);
    if (has_item_catch)
        fputs("static void authored_item_catch(HSD_GObj* gobj);\n", output);
    if (has_grabbed)
        fputs("static void authored_grabbed(HSD_GObj* victim_gobj, HSD_GObj* gobj);\n",
              output);
    if (any_authored)
        fputs("static void authored_reset_root_x(HSD_GObj* gobj) {\n"
              "    engine->set_root_rotation_x(gobj->user_data, 0, 0);\n"
              "}\n", output);
    if (state.has_float)
        fputs("static float script_state_get_float(Fighter* fighter, "
              "unsigned offset) {\n"
              "    float value;\n"
              "    memcpy(&value, fighter->native_module_state + offset, "
              "sizeof(value));\n"
              "    return value;\n"
              "}\n"
              "static void script_state_set_float(Fighter* fighter, "
              "unsigned offset, float value) {\n"
              "    memcpy(fighter->native_module_state + offset, &value, "
              "sizeof(value));\n"
              "}\n", output);
    if (state.has_int)
        fputs("static int32_t script_state_get_int(Fighter* fighter, "
              "unsigned offset) {\n"
              "    int32_t value;\n"
              "    memcpy(&value, fighter->native_module_state + offset, "
              "sizeof(value));\n"
              "    return value;\n"
              "}\n"
              "static void script_state_set_int(Fighter* fighter, "
              "unsigned offset, int32_t value) {\n"
              "    memcpy(fighter->native_module_state + offset, &value, "
              "sizeof(value));\n"
              "}\n", output);
    if (!nativeScriptState_EmitArrayAccess(output, nativeScriptState_ArrayTypes(&state) |
            nativeFighterItemScript_StateArrayTypes(items)) ||
        !nativeScriptState_EmitMetadata(output, &state, "authored_state_fields", true)) return false;
    fputs("static NativeFighterStateField combined_state_fields[64];\n", output);
    nativeScriptPose_Emit(functions->poses, output);
    nativeScriptModelEffect_Emit(functions->model_effects, output);
    nativeScriptFunctions_Emit(functions, output);
    if (!nativeFighterItemScript_Emit(items, asset_root, output, error))
        return false;
    for (unsigned i = 0; i < move_count; ++i) {
        const ScriptMove* current = &moves[i];
        const char* suffix = nativeScriptCode_SpecialSuffix(current->special);
        if (current->ground_entry == ScriptEntry_Stock)
            fprintf(output, "static HSD_GObjEvent inherited_ground_%s;\n",
                    suffix);
        if (current->air_entry == ScriptEntry_Stock)
            fprintf(output, "static HSD_GObjEvent inherited_air_%s;\n",
                    suffix);
    }
    if (neutral != NULL && neutral->stock_article)
        fputs("static HSD_GObjEvent inherited_neutral_article;\n", output);
    if (neutral != NULL && neutral->cue_once)
        fputs("static void authored_exit(HSD_GObj* gobj) {\n"
              "    Fighter* fighter = gobj->user_data;\n"
              "    if (fighter->motion_id != generated.special_ground_state[NativeFighterSpecial_Neutral] &&\n"
              "        fighter->motion_id != generated.special_air_state[NativeFighterSpecial_Neutral])\n"
              "        fighter->native_module_state[script_state_base] = 0;\n"
              "}\n", output);
    const bool raw_cue_effect = neutral != NULL && neutral->cue &&
        (neutral->stock_article || neutral->fireball_bone != NULL);
    if (neutral != NULL && neutral->cue) {
        fputs("static void authored_cue(HSD_GObj* gobj) {\n"
              "    Fighter* fighter = gobj->user_data;\n", output);
        if (raw_cue_effect)
            nativeScriptLifetime_EmitEntry(output, ScriptCallback_Fighter);
        fputs("    if (!fighter->throw_flags_b0) return;\n", output);
        if (neutral->cue_once)
            fputs("    if (fighter->native_module_state[script_state_base] != 0) {\n"
                  "        fighter->throw_flags_b0 = 0;\n"
                  "        return;\n"
                  "    }\n"
                  "    fighter->native_module_state[script_state_base] = 1;\n", output);
        if (!neutral->stock_article)
            fputs("    fighter->throw_flags_b0 = 0;\n", output);
        if (neutral->stock_article)
            fputs("    inherited_neutral_article(gobj);\n"
                  "    if (!engine->callback_source_alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;\n"
                  "    fighter->throw_flags_b0 = 0;\n", output);
        // A raw constructor and authored statements may both need a generation
        // snapshot. Their lexical scopes preserve direct calls and local names.
        if (raw_cue_effect) fputs("    {\n", output);
        nativeScriptCode_EmitLines(&neutral->neutral_cue_code, output);
        if (raw_cue_effect) fputs("    }\n", output);
        if (neutral->fireball_bone != NULL) {
            fprintf(output,
                    "    HSD_JObj* article_joint = fighter->parts[engine->resolve_bone(fighter, %s)].joint;\n"
                    "    Vec3 article_position;\n"
                    "    engine->joint_position(article_joint, 0, &article_position);\n",
                    neutral->fireball_bone);
            nativeScriptLifetime_EmitLine(output,
                "    engine->spawn_projectile(gobj, &article_position, It_Kind_Mario_Fire, 0, fighter->facing_dir);\n",
                functions, ScriptCallback_Fighter, ScriptCallback_Void);
        }
        if (neutral->effect_bone != NULL)
            fprintf(output,
                    "    HSD_JObj* effect_joint = fighter->parts[engine->resolve_bone(fighter, %s)].joint;\n"
                    "    engine->spawn_effect(%u, gobj, effect_joint, "
                    "&fighter->facing_dir);\n",
                    neutral->effect_bone, neutral->effect_id);
        if (neutral->cue_speed_set)
            fprintf(output,
                    "    if (fighter->ground_or_air == GA_Ground)\n"
                    "        fighter->gr_vel %s fighter->facing_dir * %.9ef;\n"
                    "    else\n"
                    "        fighter->self_vel.x %s fighter->facing_dir * %.9ef;\n",
                    neutral->cue_speed_replace ? "=" : "+=", neutral->cue_speed,
                    neutral->cue_speed_replace ? "=" : "+=", neutral->cue_speed);
        if (neutral->cue_vertical_speed_set)
            fprintf(output,
                    "    if (fighter->ground_or_air == GA_Air)\n"
                    "        fighter->self_vel.y %s %.9ef;\n",
                    neutral->cue_vertical_speed_replace ? "=" : "+=",
                    neutral->cue_vertical_speed);
        fputs("}\n", output);
    }

    for (unsigned i = 0; i < move_count; ++i) {
        const ScriptMove* current = &moves[i];
        if (current->accessory_code.count != 0)
            nativeScriptCode_EmitBlock(&current->accessory_code, false, "cue",
                            nativeScriptCode_SpecialSuffix(current->special), output);
    }

    for (unsigned i = 0; i < move_count; ++i) {
        const ScriptMove* current = &moves[i];
        const char* suffix = nativeScriptCode_SpecialSuffix(current->special);
        if (current->ground_entry == ScriptEntry_Stock) {
            fprintf(output,
                    "static void authored_ground_%s(HSD_GObj* gobj) {\n"
                    "    Fighter* fighter = gobj->user_data;\n", suffix);
            nativeScriptCode_EmitLines(&current->ground_entry_code, output);
            fputs("}\n", output);
        }
        if (current->air_entry == ScriptEntry_Stock) {
            fprintf(output,
                    "static void authored_air_%s(HSD_GObj* gobj) {\n"
                    "    Fighter* fighter = gobj->user_data;\n", suffix);
            nativeScriptCode_EmitLines(&current->air_entry_code, output);
            fputs("}\n", output);
        }
    }
    for (unsigned i = 0; i < move_count; ++i) {
        ScriptMove* current = &moves[i];
        if (current->ground_entry != ScriptEntry_Authored &&
            current->air_entry != ScriptEntry_Authored) continue;
        const char* suffix = nativeScriptCode_SpecialSuffix(current->special);
        char accessory[64];
        snprintf(accessory, sizeof(accessory), "authored_ground_cue_%s", suffix);
        if (current->ground_entry == ScriptEntry_Authored) {
            nativeScriptCode_EmitBlock(&current->ground_animation, false, "animation",
                            suffix, output);
            nativeScriptCode_EmitBlock(&current->ground_interrupt, false, "interrupt",
                            suffix, output);
            nativeScriptCode_EmitBlock(&current->ground_physics, false, "physics",
                            suffix, output);
            nativeScriptCode_EmitCollision(current, false, output);
            fprintf(output,
                    "static void authored_ground_%s(HSD_GObj* gobj) {\n"
                    "    Fighter* fighter = gobj->user_data;\n", suffix);
            if (!current->explicit_entry)
                fputs("    fighter->cmd_vars[0] = 0;\n"
                      "    fighter->throw_flags = 0;\n", output);
            nativeScriptCode_EmitLines(&current->ground_entry_code, output);
            if (!current->explicit_entry)
                fprintf(output, "    fighter->accessory4_cb = %s;\n",
                        current->cue ? "authored_cue" :
                        current->accessory_code.count ? accessory : "0");
            if (current->cue_once)
                fputs("    fighter->native_module_state[script_state_base] = 0;\n"
                      "    fighter->x21EC = authored_exit;\n", output);
            fputs("}\n", output);
        }
        if (current->air_entry == ScriptEntry_Authored) {
            nativeScriptCode_EmitBlock(&current->air_animation, true, "animation",
                            suffix, output);
            nativeScriptCode_EmitBlock(&current->air_interrupt, true, "interrupt",
                            suffix, output);
            nativeScriptCode_EmitBlock(&current->air_physics, true, "physics",
                            suffix, output);
            nativeScriptCode_EmitCollision(current, true, output);
            fprintf(output,
                    "static void authored_air_%s(HSD_GObj* gobj) {\n"
                    "    Fighter* fighter = gobj->user_data;\n", suffix);
            if (!current->explicit_entry)
                fputs("    fighter->cmd_vars[0] = 0;\n"
                      "    fighter->throw_flags = 0;\n", output);
            nativeScriptCode_EmitLines(&current->air_entry_code, output);
            if (!current->explicit_entry)
                fprintf(output, "    fighter->accessory4_cb = %s;\n",
                        current->cue ? "authored_cue" :
                        current->accessory_code.count ? accessory : "0");
            if (current->cue_once)
                fputs("    fighter->native_module_state[script_state_base] = 0;\n"
                      "    fighter->x21EC = authored_exit;\n", output);
            fputs("}\n", output);
        }
    }

    for (unsigned i = 0; i < action_handler_count; ++i) {
        const char* callback = strcmp(action_handlers[i].name, "damage") == 0
            ? "authored_damage" :
            strcmp(action_handlers[i].name, "secondary_damage") == 0
            ? "authored_secondary_damage" :
            strcmp(action_handlers[i].name, "death2") == 0
            ? "authored_death2" :
            strcmp(action_handlers[i].name, "pre_hitlag") == 0
            ? "authored_pre_hitlag" :
            strcmp(action_handlers[i].name, "post_hitlag") == 0
            ? "authored_post_hitlag" :
            strcmp(action_handlers[i].name, "hurtbox_detect") == 0
            ? "authored_hurtbox_detect" :
            strcmp(action_handlers[i].name, "deal_damage") == 0
            ? "authored_deal_damage" :
            strcmp(action_handlers[i].name, "reflect") == 0
            ? "authored_reflect" :
            strcmp(action_handlers[i].name, "reflect_hit") == 0
            ? "authored_reflect_hit" :
            strcmp(action_handlers[i].name, "state_change") == 0
            ? "authored_state_change" :
            strcmp(action_handlers[i].name, "grab_catch") == 0
            ? "authored_grab_catch" :
            strcmp(action_handlers[i].name, "item_catch") == 0
            ? "authored_item_catch" :
            strcmp(action_handlers[i].name, "grabbed") == 0
            ? "authored_grabbed" : NULL;
        // Native grabbed callbacks enter as (victim, holder). Authored gobj
        // and fighter always denote the script owner, matching the VM.
        if (callback != NULL)
            fprintf(output,
                    "static void %s(HSD_GObj* %s%s) {\n"
                    "    Fighter* fighter = gobj->user_data;\n", callback,
                    strcmp(action_handlers[i].name, "grabbed") == 0 ? "victim_gobj" : "gobj",
                    strcmp(action_handlers[i].name, "grabbed") == 0
                        ? ", HSD_GObj* gobj" : "");
        else
            fprintf(output,
                    "static %s authored_action_%u(HSD_GObj* gobj%s) {\n"
                    "    Fighter* fighter = gobj->user_data;\n", action_handlers[i].code.floating_result ? "float" : action_handlers[i].code.integer_result ? "int" : action_handlers[i].code.predicate ? "bool" : "void", i,
                    action_handlers[i].code.cpu_prepare_event ? ", uint32_t history_source" :
                    action_handlers[i].code.held_motion_event ? ", int32_t motion, int32_t previous_motion" :
                    action_handlers[i].code.air_resume_event ? ", int32_t previous_motion" :
                    action_handlers[i].code.neutral_enter_event ? ", float blend" :
                    strcmp(action_handlers[i].name, "throw_victim_motion") == 0
                        ? ", int motion, int victim_motion" : action_handlers[i].code.item_swing_event
                        ? ", int swing_type, int swing_variant" : action_handlers[i].code.demo_event
                        ? ", int demo_mode" : action_handlers[i].code.action_metadata_event
                        ? ", int action_metadata" : action_handlers[i].code.shield_transition_event
                        ? ", int shield_mode, bool special_hit" : state_handler_is_item_event(action_handlers[i].name)
                        ? ", bool flag" : action_handlers[i].code.motion_event
                        ? ", int motion" : action_handlers[i].code.trail_event
                        ? ", NativeFighterTrail* trail" : action_handlers[i].code.target_geometry_event
                        ? ", float target_angle, float target_distance" : action_handlers[i].code.cpu_projectile_defense_event
                        ? ", int projectile_category" : action_handlers[i].code.cpu_projectile_hit_event
                        ? ", int prediction_frames, bool unblockable" : action_handlers[i].code.smash_angle_event
                        ? ", float smash_angle" : action_handlers[i].code.scalar_event
                        ? ", float event_value" : action_handlers[i].code.other_fighter_event
                        ? ", HSD_GObj* other_gobj" : action_handlers[i].code.dynamics_set_event
                        ? ", int set, int set_count" : "");
        if (action_handlers[i].code.landing_action_event)
            fputs("    const bool script_landing_interrupt_allowed = fighter->mv.co.landing.allow_interrupt;\n", output);
        nativeScriptCode_EmitLines(&action_handlers[i].code, output);
        if (action_handlers[i].code.held_motion_event || strcmp(action_handlers[i].name, "aerial_attack_motion") == 0) fputs("    return motion;\n", output);
        else if (action_handlers[i].code.air_resume_event) fputs("    return previous_motion;\n", output);
        else if (strcmp(action_handlers[i].name, "throw_victim_motion") == 0) fputs("    return victim_motion;\n", output);
        else if (action_handlers[i].code.floating_result) fputs("    return 0.0f;\n", output);
        else if (action_handlers[i].code.item_swing_event) fputs("    return -1;\n", output);
        else if (action_handlers[i].code.action_metadata_event) fputs("    return action_metadata;\n", output);
        else if (action_handlers[i].code.integer_result) fputs("    return 0;\n", output);
        else if (action_handlers[i].code.predicate) fputs("    return false;\n", output);
        fputs("}\n", output);
    }

    fputs("__declspec(dllexport) const NativeFighterDefinition* __cdecl\n"
          "FreakMelee_GetFighterModule(const NativeFighterDefinition* base, "
          "const NativeFighterHost* host) {\n"
          "    if (base == 0 || host == 0", output);
    for (unsigned i = 0; i < action_handler_count; ++i) {
        unsigned motion;
        const char* field;
        if (!state_handler_target(action_handlers[i].name, &motion, &field))
            continue;
        fprintf(output,
                " || %u < base->motion_state_first || "
                "%u - base->motion_state_first >= base->motion_state_count",
                motion, motion);
    }
    bool owns_attachments = false;
    for (unsigned i = 0; i <= move_count; ++i) {
        const ScriptMove* current = i < move_count ? &moves[i] : NULL;
        if (current != NULL) {
            nativeScriptCode_EmitStockRequirements(current, output);
            if (current->special == ScriptSpecial_Neutral &&
                current->stock_article)
                fputs(" || base->neutral_article == 0", output);
            if (current->fireball_bone != NULL)
                fputs(" || host->resolve_bone == 0 || "
                      "host->joint_position == 0 || "
                      "host->spawn_projectile == 0", output);
            if (current->effect_bone != NULL)
                fputs(" || host->resolve_bone == 0 || host->spawn_effect == 0", output);
            for (unsigned plane = 0; plane < 2; ++plane) {
                if ((plane == 0 ? current->ground_entry : current->air_entry)
                    != ScriptEntry_Authored) continue;
                const char* special = nativeScriptCode_SpecialEnum(current->special);
                const char* side = plane ? "air" : "ground";
                fprintf(output,
                " || base->special_%s_state[NativeFighterSpecial_%s] == 0"
                " || base->special_%s_state[NativeFighterSpecial_%s] < base->motion_state_first"
                " || base->special_%s_state[NativeFighterSpecial_%s] - base->motion_state_first >= base->motion_state_count",
                side, special, side, special, side, special);
            }
        }
        const ScriptCodeBlock* code_blocks[12 + SCRIPT_FIGHTER_HANDLERS + SCRIPT_FUNCTIONS];
        unsigned block_count = 0;
        if (current != NULL) {
            const ScriptCodeBlock* move_blocks[] = {
                &current->ground_entry_code, &current->air_entry_code,
                &current->ground_animation, &current->air_animation,
                &current->ground_interrupt, &current->air_interrupt,
                &current->ground_physics, &current->air_physics,
                &current->ground_collision, &current->air_collision,
                &current->neutral_cue_code, &current->accessory_code,
            };
            memcpy(code_blocks, move_blocks, sizeof(move_blocks));
            block_count = sizeof(move_blocks) / sizeof(move_blocks[0]);
        } else {
            for (unsigned action = 0; action < action_handler_count; ++action)
                code_blocks[block_count++] = &action_handlers[action].code;
            for (unsigned function = 0; function < functions->count; ++function)
                code_blocks[block_count++] = &functions->functions[function].code;
        }
        for (unsigned phase = 0; phase < block_count; ++phase) {
            const ScriptCodeBlock* block = code_blocks[phase];
            nativeScriptCode_EmitHostChecks(block, output);
            owns_attachments |= block_owns_attachments(block);
            for (unsigned line = 0; line < block->count; ++line) {
                const char* state_call = strstr(block->lines[line].text,
                                                "state.enter(");
                if (state_call == NULL)
                    state_call = strstr(block->lines[line].text,
                                        "state.enter_flags(");
                if (state_call == NULL)
                    state_call = strstr(block->lines[line].text,
                                        "state.enter_at(");
                if (state_call != NULL &&
                    (strstr(state_call, "state.enter_flags(") == state_call ||
                     strstr(state_call, "state.enter_at(") == state_call ||
                     (state_call[12] >= '0' && state_call[12] <= '9'))) {
                    unsigned motion;
                    if (sscanf(state_call, "state.enter(%u", &motion) != 1 &&
                        sscanf(state_call, "state.enter_flags(%u", &motion) != 1 &&
                        sscanf(state_call, "state.enter_at(%u", &motion) != 1)
                        return nativeScriptCode_Fail(error, 0, "Invalid motion state reference.");
                    fprintf(output,
                            " || %u < base->motion_state_first || "
                            "%u - base->motion_state_first >= base->motion_state_count",
                            motion, motion);
                }

            }
        }
    }
    if (any_authored && move_count != 0)
        fputs(" || host->change_motion_state == 0 || "
              "host->start_animation == 0", output);
    if (any_authored)
        fputs(" || base->motion_states == 0 || "
              "base->motion_state_count == 0 || "
              "base->motion_state_count > 1024", output);
    nativeScriptMotionPolicy_EmitChecks(policies, output);
    nativeScriptModelEffect_EmitChecks(functions->model_effects, output);
    fputs(item_host_checks, output);
    if (raw_cue_effect) fputs(" || host->callback_source_alive == 0", output);
    fprintf(output, ") return 0;\n"
                    "    generated = *base;\n"
                    "    generated.code_key = \"%s\";\n", code_key);
    if (independent) emit_independent_setup(output);
    nativeScriptFunctions_EmitDefinitionExports(functions, output);
    nativeScriptPose_EmitDefinition(functions->poses, output);
    nativeScriptSoundPolicy_EmitDefinition(functions->sound_policy, output);
    nativeScriptSurfacePolicy_EmitDefinition(functions->surface_policy, output);
    nativeScriptModelEffect_EmitDefinition(functions->model_effects, output);
    if (owns_attachments)
        fputs("    generated.owns_model_attachments = true;\n", output);
    for (unsigned i = 0; i < move_count; ++i)
        nativeScriptCode_EmitStockBindings(&moves[i], output);
    if (neutral != NULL && neutral->stock_article)
        fputs("    inherited_neutral_article = base->neutral_article;\n", output);
    if (any_authored || has_items || has_articles || functions->count != 0)
        fputs("    engine = host;\n", output);
    if (any_authored)
        fputs("    memcpy(authored_states, base->motion_states, "
              "base->motion_state_count * sizeof(MotionState));\n"
              "    generated.motion_states = authored_states;\n", output);
    if (has_items)
        fprintf(output,
                "    generated.items = scripted_items;\n"
                "    generated.item_count = %u;\n",
                nativeFighterItemScript_Count(items));
    if (has_articles)
        fprintf(output,
                "    generated.article_logics = scripted_article_logics;\n"
                "    generated.article_logic_count = %u;\n",
                nativeFighterItemScript_ArticleCount(items));
    nativeScriptMotionPolicy_EmitBindings(policies, output, independent);
    nativeScriptColors_EmitVisibility(visibility_mask, output);
    for (unsigned i = 0; i < move_count; ++i) {
        const ScriptMove* current = &moves[i];
        if (current->ground_entry != ScriptEntry_Authored &&
            current->air_entry != ScriptEntry_Authored) continue;
        char accessory[64];
        snprintf(accessory, sizeof(accessory), "authored_ground_cue_%s",
                 nativeScriptCode_SpecialSuffix(current->special));
        fprintf(output,
                "    transition_cue[NativeFighterSpecial_%s] = %s;\n"
                "    transition_exit[NativeFighterSpecial_%s] = %s;\n",
                nativeScriptCode_SpecialEnum(current->special),
                current->cue ? "authored_cue" :
                current->accessory_code.count ? accessory : "0",
                nativeScriptCode_SpecialEnum(current->special),
                current->cue_once ? "authored_exit" : "0");
    }
    for (unsigned i = 0; i < move_count; ++i) {
        const ScriptMove* current = &moves[i];
        if (current->ground_entry != ScriptEntry_Authored &&
            current->air_entry != ScriptEntry_Authored) continue;
        const char* special = nativeScriptCode_SpecialEnum(current->special);
        const char* suffix = nativeScriptCode_SpecialSuffix(current->special);
        const char* ground = "base->special_ground_state";
        const char* air = "base->special_air_state";
        if (current->ground_entry == ScriptEntry_Authored) {
            fprintf(output,
                "    authored_states[%s[NativeFighterSpecial_%s] - base->motion_state_first].coll_cb = authored_ground_collision_%s;\n",
                ground, special, suffix);
            if (independent)
                fprintf(output, "    authored_motion_fields[%s[NativeFighterSpecial_%s] - base->motion_state_first] |= 8u;\n", ground, special);
        }
        if (current->air_entry == ScriptEntry_Authored) {
            fprintf(output,
                "    authored_states[%s[NativeFighterSpecial_%s] - base->motion_state_first].coll_cb = authored_air_collision_%s;\n",
                air, special, suffix);
            if (independent)
                fprintf(output, "    authored_motion_fields[%s[NativeFighterSpecial_%s] - base->motion_state_first] |= 8u;\n", air, special);
        }
        const char* fields[] = { "anim_cb", "anim_cb", "input_cb", "input_cb",
                                 "phys_cb", "phys_cb" };
        const char* callbacks[] = { "ground_animation", "air_animation",
                                    "ground_interrupt", "air_interrupt",
                                    "ground_physics", "air_physics" };
        const bool active[] = { current->ground_animation.count != 0,
                                current->air_animation.count != 0,
                                current->ground_interrupt.count != 0,
                                current->air_interrupt.count != 0,
                                current->ground_physics.count != 0,
                                current->air_physics.count != 0 };
        for (unsigned phase = 0; phase < 6; ++phase) {
            if ((phase % 2 ? current->air_entry : current->ground_entry)
                != ScriptEntry_Authored) continue;
            if (!active[phase]) continue;
            fprintf(output,
                    "    authored_states[%s[NativeFighterSpecial_%s] - base->motion_state_first].%s = authored_%s_%s;\n",
                    (phase & 1u) ? air : ground, special, fields[phase],
                    callbacks[phase], suffix);
            if (independent)
                fprintf(output, "    authored_motion_fields[%s[NativeFighterSpecial_%s] - base->motion_state_first] |= %uu;\n",
                        (phase & 1u) ? air : ground, special,
                        motion_field_bit(fields[phase]));
        }
        for (unsigned plane = 0; plane < 2; ++plane)
            if ((plane == 0 ? current->ground_entry : current->air_entry)
                == ScriptEntry_Authored)
                fprintf(output, "    generated.%s = authored_%s_%s;\n",
                    nativeScriptCode_SpecialField(current->special, plane != 0),
                    plane ? "air" : "ground", suffix);
            else
                fprintf(output, "    generated.%s = 0;\n",
                    nativeScriptCode_SpecialField(current->special, plane != 0));
    }
    for (unsigned i = 0; i < action_handler_count; ++i) {
        unsigned motion;
        const char* field;
        if (state_handler_target(action_handlers[i].name, &motion, &field)) {
            fprintf(output,
                    "    authored_states[%u - base->motion_state_first].%s = "
                    "authored_action_%u;\n", motion, field, i);
            if (independent)
                fprintf(output, "    authored_motion_fields[%u - base->motion_state_first] |= %uu;\n",
                        motion, motion_field_bit(field));
        } else if (state_handler_field(action_handlers[i].name) != NULL)
            fprintf(output,
                    "    generated.%s = authored_action_%u;\n",
                    state_handler_field(action_handlers[i].name), i);
    }
    if (state.count != 0 || (neutral != NULL && neutral->cue_once))
        fprintf(output,
                "    script_state_base = %s;\n"
                "    if (script_state_base > 256 - %u) return 0;\n"
                "    generated.state_bytes = script_state_base + %u;\n",
                state.has_float || state.has_int ? "(base->state_bytes + 3u) & ~3u" : "base->state_bytes",
                state.bytes, state.bytes);
    if (!nativeScriptState_EmitCombinedMetadata(output, &state)) return false;
    if (independent) emit_independent_guard(output);
    fputs("    return &generated;\n}\n", output);
    fputs("__declspec(dllexport) unsigned long long __cdecl "
          "FreakMelee_GetFighterModuleLayout(void) { "
          "return nativeFighterModule_LayoutFingerprint(); }\n", output);
    fprintf(output,
            "__declspec(dllexport) unsigned long long __cdecl "
            "FreakMelee_GetScriptSourceHash(void) { "
            "return 0x%016llxULL; }\n",
            (unsigned long long) source_hash);
    if (ferror(output)) return nativeScriptCode_Fail(error, 0, "Could not write generated C.");
    return true;
}

static bool parse_behavior_declaration(char* source, size_t size,
                                      bool* independent,
                                      NativeFighterScriptError* error)
{
    bool block = false, seen = false, code_seen = false;
    unsigned line_number = 0, block_line = 0;
    for (size_t start = 0; start < size;) {
        size_t end = start;
        while (end < size && source[end] != '\n') ++end;
        size_t length = end - start;
        ++line_number;
        if (length && source[start + length - 1] == '\r') --length;
        while (length && (source[start + length - 1] == ' ' ||
                          source[start + length - 1] == '\t')) --length;
        const char* text = source + start;
        size_t indent = 0;
        while (indent < length && (text[indent] == ' ' || text[indent] == '\t')) ++indent;
        if (indent == length || text[indent] == '#') {
            start = end < size ? end + 1 : end;
            continue;
        }
        bool header = length == 8 && memcmp(text, "fighter:", 8) == 0;
        if (header) {
            if (seen || code_seen)
                return nativeScriptCode_Fail(error, line_number,
                    "The fighter declaration must be unique and precede functions, moves, actions and articles.");
            block = seen = true;
            block_line = line_number;
        } else if (block && indent != 0) {
            if (*independent || length != 24 ||
                memcmp(text, "    behavior independent", 24) != 0)
                return nativeScriptCode_Fail(error, line_number,
                    "Expected one `    behavior independent` in the fighter declaration.");
            *independent = true;
        } else {
            if (block && !*independent)
                return nativeScriptCode_Fail(error, block_line,
                    "The fighter declaration needs `    behavior independent`.");
            block = false;
            if (length - indent >= 8 && memcmp(text + indent, "behavior", 8) == 0 &&
                (length - indent == 8 || text[indent + 8] == ' '))
                return nativeScriptCode_Fail(error, line_number,
                    "Behavior policy belongs in a top-level fighter declaration.");
            code_seen |= indent == 0 &&
                (length >= 5 && memcmp(text, "move ", 5) == 0 ||
                 length >= 5 && memcmp(text, "func ", 5) == 0 ||
                 length >= 12 && memcmp(text, "export func ", 12) == 0 ||
                 length >= 7 && memcmp(text, "action ", 7) == 0 ||
                 length >= 8 && memcmp(text, "article ", 8) == 0 ||
                 length >= 5 && memcmp(text, "item ", 5) == 0);
            start = end < size ? end + 1 : end;
            continue;
        }
        memset(source + start, ' ', end - start);
        start = end < size ? end + 1 : end;
    }
    return !seen || *independent || nativeScriptCode_Fail(error, block_line,
        "The fighter declaration needs `    behavior independent`.");
}

bool nativeFighterScript_GenerateC(const char* source, size_t source_size,
                                   const char* code_key, const char* asset_root,
                                   const NativeFighterParameters* parameters,
                                   const NativeFighterScriptConstruction* construction,
                                   FILE* output,
                                   NativeFighterScriptError* error)
{
    if (error == NULL) return false;
    *error = (NativeFighterScriptError){ 0 };
    if (source == NULL || output == NULL || code_key == NULL ||
        !nativeScriptCode_ValidKey(code_key) || source_size == 0 ||
        source_size > NATIVE_FIGHTER_BEHAVIOR_SOURCE_CAPACITY)
        return nativeScriptCode_Fail(error, 0, "Invalid source or fighter code key.");

    if (construction != NULL && (construction->fighter_root == NULL ||
        !nativeScriptCode_Identifier(construction->fighter_root, strlen(construction->fighter_root), 64) ||
        construction->motion_count == 0 || construction->motion_count > 1024 ||
        construction->motion_first > UINT16_MAX ||
        construction->motion_count > UINT16_MAX + 1u - construction->motion_first))
        return nativeScriptCode_Fail(error, 0, "Invalid explicit oracle root or motion bounds.");

    char* function_source = malloc(source_size + 1);
    if (function_source == NULL) return nativeScriptCode_Fail(error, 0, "Out of memory parsing functions.");
    memcpy(function_source, source, source_size); function_source[source_size] = 0;
    ScriptSoundPolicy sound_policy;
    if (!nativeScriptSoundPolicy_Parse(function_source, source_size, &sound_policy, error)) {
        free(function_source);
        return false;
    }
    bool independent = false;
    ScriptSurfacePolicy surface_policy;
    if (!nativeScriptSurfacePolicy_Parse(function_source, source_size, &surface_policy, error)) {
        free(function_source);
        return false;
    }
    if (!parse_behavior_declaration(function_source, source_size, &independent,
                                     error)) {
        free(function_source);
        return false;
    }
    uint16_t visibility_mask;
    if (!nativeScriptColors_ParseVisibility(function_source, source_size, &visibility_mask, error)) {
        free(function_source);
        return false;
    }
    ScriptMotionPolicies policies;
    if (!nativeScriptMotionPolicy_Parse(function_source, source_size, &policies, error)) {
        free(function_source);
        return false;
    }
    ScriptPoseProgram poses;
    if (!nativeScriptPose_Parse(function_source, source_size, &poses, error)) {
        free(function_source);
        return false;
    }
    ScriptModelEffectProgram* effects = nativeScriptModelEffect_Parse(function_source, source_size, error);
    if (effects == NULL) { free(function_source); return false; }
    ScriptFunctionProgram* functions = nativeScriptFunctions_Parse(function_source, source_size, error);
    if (functions == NULL) { nativeScriptModelEffect_Free(effects); free(function_source); return false; }
    functions->model_effects = effects;
    functions->poses = &poses;
    functions->sound_policy = &sound_policy;
    functions->surface_policy = &surface_policy;
    char* fighter_source = NULL;
    NativeFighterItemProgram* items = nativeFighterItemScript_Parse(
        function_source, source_size, &fighter_source, construction != NULL ? 2 : 1,
        parameters, functions, error);
    free(function_source);
    if (items == NULL) { nativeScriptModelEffect_Free(effects); free(functions); return false; }
    uint64_t hash = nativeFighterScript_SourceHash(source, source_size);
    bool generated = construction != NULL
        ? generate_state_callbacks(fighter_source, source_size, code_key, construction,
                                   output, error, items, asset_root,
                                   parameters, hash, functions, independent, &policies, visibility_mask)
        : generate_special_callbacks(fighter_source, source_size, code_key,
                                     asset_root, output, error, items,
                                     parameters, hash, functions, independent, &policies, visibility_mask);
    nativeFighterItemScript_Free(items);
    nativeScriptModelEffect_Free(effects);
    free(functions);
    free(fighter_source);
    return generated;
}
