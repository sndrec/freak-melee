#include "native_fighter_script_lifetime.h"
#include "native_fighter_script_functions.h"
#include <ctype.h>
#include <string.h>

typedef struct CallbackOperation { const char* name; bool value; } CallbackOperation;
// These engine operations can synchronously enter another authored callback.
// Pure queries, math, RNG and writes that only install future callbacks are not
// effects. Return values are captured with Clang __auto_type, preserving their
// exact integer, enum, pointer or floating C type.
static const CallbackOperation operations[] = {
    { "activate_partner", true },
    { "request_hitlag_release", false },
    { "target_bone_position", true },
    { "cpu_nearest_enemy_at", true },
    { "cpu_record_input", true }, { "cpu_read_history_pose", true },
    { "cpu_apply_recorded_input", true }, { "cpu_prepare_combat_plan", false },
    { "cpu_prepare_destination_update", true }, { "cpu_update_recovery_destination", false },
    { "cpu_plan_attack", true }, { "cpu_plan_without_target", false },
    { "cpu_set_destination", false }, { "target_position_delta", true },
    { "stage_read_floor", true }, { "stage_same_line_island", true },
    { "stage_cpu_floor_excluded", true }, { "stage_cpu_island_blocked", true },
    { "stage_line_island_bounds", true },
    { "article_ref_primary_translation", true },
    { "target_position", true },
    { "sound_channels", false },
    { "target_call", true }, { "signal_owned_article", true }, { "signal_article", true },
    { "target_call_bool", true }, { "target_call_int", true }, { "target_call_float", true },
    { "item_signal_link", true }, { "remove_owned_article", true },
    { "change_motion_state", false }, { "start_animation", false },
    { "ground_to_air", false }, { "air_to_ground", false },
    { "finish_ground_action", false }, { "enter_taunt", false },
    { "enter_fall", false }, { "enter_freefall", false },
    { "enter_freefall_blend", false }, { "enter_down_bound", false },
    { "force_air_and_use_jumps", false },
    { "ground_actions", false }, { "taunt_actions", false }, { "air_actions", true },
    { "aerial_attack_actions", true }, { "enter_cliff_catch", false },
    { "enter_landing_basic", false }, { "enter_landing_fall_special", false },
    { "enter_wait_blend", false }, { "enter_held_open", false }, { "enter_walk", false },
    { "enter_platform_drop", false }, { "finish_action", false },
    { "launch_jump", false }, { "begin_aerial_jump", false },
    { "enter_landing", false }, { "initialize_hold", false },
    { "set_hold_motion", false }, { "set_throw_motion", false },
    { "enter_damage_motion", false }, { "enter_damage_reaction", false },
    { "enter_damage_fall", false }, { "enter_crouch_wait", false },
    { "enter_cliff_jump_phase2", false },
    { "throw_motion", false }, { "update_throw", false },
    { "throw_held_item", false }, { "release_held_item", false },
    { "fighter_item_pickup", false }, { "fighter_item_drop", false },
    { "notify_item_pickup", false }, { "self_damage", false },
    { "reset_damage_callbacks", false },
    { "fighter_knockback_enter", false }, { "fighter_knockback_exit", false },
    { "begin_capture", false }, { "capture_victim_to_air", false },
    { "release_capture_throw", false }, { "initialize_attached_capture", false },
    { "enter_attached_capture_motion", false }, { "enter_attached_throw", false },
    { "unlink_capture", false }, { "apply_throw_damage", false },
    { "launch_thrown_victim", false }, { "capture_attach_item", false },
    { "capture_release_item", false }, { "capture_release_into_enclosure", false },
    { "capture_enclose_caught", false },
    { "spawn_projectile", false }, { "spawn_authored_projectile", false },
    { "spawn_attached_article", true }, { "spawn_bone_attached_article", true },
    { "spawn_holdable_item", true }, { "spawn_article", true },
    { "create_article_at", true }, { "create_unowned_article_at", true },
    { "create_held_article_at_center", true }, { "create_item_at_position", true },
    { "create_engine_item", true }, { "attach_created_item", true },
    { "notify_item_drop", true }, { "notify_held_neutral", true },
    { "item_create_child", true }, { "item_spawn_link", true },
    { "item_spawn_food", true }, { "spawn_food", true }, { "item_remove", false },
    { "item_change_state", false }, { "item_change_state_flags", false },
    { "item_refresh_physics", false },
    { "item_activate_attached", false }, { "item_interact", true },
    { "item_attach_to_owner", false }, { "item_release_attachment", false },
    { "item_set_airborne", false }, { "item_land", false }, { "item_bounce", false },
    { "item_land_on_stage", false }, { "item_stage_response", false },
    { "item_stage_contact_response", false }, { "item_follow_ground", false },
    { "article_resource_sweep_stage", true }, { "article_resource_sweep_point", true },
    { "article_resource_spawn_effect_at_position", true },
    { "article_resource_spawn_effect_facing_at_position", true },
    { "item_bounce_to_ground", false }, { "item_bounce_stage_to_ground", false },
    { "item_slide_on_ground", false }, { "item_stage_collision_land", true },
    { "item_begin_expiry", false }, { "item_expire_tick", true },
    { "item_advance_animation", false },
    { "air_standard_collision", false }, { "air_catch_hit_collision", false },
    { "ground_standard_collision", false }, { "ground_nudge_collision", false },
    { "ground_standard_box_collision", false }, { "air_special_landing_collision", false },
    { "check_ground_escape", true }, { "check_turn_input", true },
    { "enter_air_gun", false }, { "check_air_item_throw", true },
    { "contact_rebound_allowed", true },
    { "check_taunt", true }, { "check_crouch", true },
    { "check_turn", true }, { "check_walk", true },
    { "check_ground_jump", true }, { "check_air_jump", true },
    { "try_wall_jump", true }, { "check_platform_drop", true },
    { "check_heavy_item_throw", true }, { "check_side_special", true },
    { "check_up_special", true }, { "check_neutral_special", true },
    { "check_down_special", true }, { "check_catch", true },
    { "check_side_smash", true }, { "check_up_smash", true }, { "check_down_smash", true },
    { "check_side_tilt", true }, { "check_up_tilt", true }, { "check_down_tilt", true },
    { "check_jab", true }, { "check_ground_roll", true },
    { "enter_jab", true }, { "enter_rapid_jab", false },
    { "rapid_jab_should_finish", true }, { "try_ground_pickup", true },
    { "air_aerial_landing_collision", false }, { "enter_landing_aerial", false },
    { "check_guard", true }, { "check_dash", true }, { "check_air_special", true },
    { "enter_guard_hold", false }, { "enter_guard_release", false },
    { "guard_reflect_actions", false }, { "guard_collision", false },
    { "guard_hit_collision", false }, { "check_powershield", true },
    { "check_guard_item_throw", true }, { "check_guard_platform_drop", true },
    { "update_aerial_jump", false },
    { "aerial_jump_actions", false }, { "enter_fall_aerial", false },
    { "spawn_model_effect", false }, { "model_effect_cancel", false },
    { "model_advance_animation", false },
    { "item_add_model_animation", true },
    { "item_advance_model_animation", false },
    { "item_remove_model_animation", false },
    { "load_pose_animation", false }, { "copy_bone_pose", false },
    { "copy_blend_pose", false }, { "blend_graph_live", false },
    { "blend_graph_alternate", false }, { "blend_pose_to_live", false },
    { "copy_pose_to_live", false },
};

typedef struct LifetimeCall {
    const char* begin;
    const char* open;
    const char* after;
    bool value;
} LifetimeCall;

static const char* call_end(const char* open)
{
    unsigned depth = 0; char quote = 0;
    for (const char* cursor = open; *cursor; ++cursor) {
        if (quote) {
            if (*cursor == '\\' && cursor[1]) ++cursor;
            else if (*cursor == quote) quote = 0;
        } else if (*cursor == '"' || *cursor == '\'') quote = *cursor;
        else if (*cursor == '(') ++depth;
        else if (*cursor == ')' && --depth == 0) return cursor + 1;
    }
    return NULL;
}

static bool find_call(const char* source, const ScriptFunctionProgram* functions,
                     ScriptCallbackClass source_class, bool direct_only,
                     LifetimeCall* call)
{
    char quote = 0;
    for (const char* cursor = source; *cursor; ++cursor) {
        if (quote) {
            if (*cursor == '\\' && cursor[1]) ++cursor;
            else if (*cursor == quote) quote = 0;
            continue;
        }
        if (*cursor == '"' || *cursor == '\'') { quote = *cursor; continue; }
        const char* name = cursor;
        bool engine = strncmp(cursor, "engine->", 8) == 0;
        bool effect_helper = strncmp(cursor, "script_effect_fn_", 17) == 0;
        bool helper = effect_helper || strncmp(cursor, "script_fn_", 10) == 0;
        bool signal = strncmp(cursor, "scripted_article_", 17) == 0;
        if (!engine && !helper && !signal) continue;
        if (cursor != source && (isalnum((unsigned char)cursor[-1]) || cursor[-1] == '_')) continue;
        if (engine) name += 8;
        const char* end = name;
        while (isalnum((unsigned char)*end) || *end == '_') ++end;
        size_t length = (size_t)(end - name);
        // A read-only callback table can be invoked directly by a bounded
        // integer index. Preserve its whole callee expression when lowering.
        // Classification still uses the actual table member name above.
        if (engine && *end == '[') {
            unsigned brackets = 0;
            char index_quote = 0;
            do {
                char c = *end++;
                if (!c) { --end; break; }
                if (index_quote) {
                    if (c == '\\' && *end) ++end;
                    else if (c == index_quote) index_quote = 0;
                } else if (c == '"' || c == '\'') index_quote = c;
                else if (c == '[') ++brackets;
                else if (c == ']') --brackets;
            } while (brackets);
            if (brackets) continue;
        }
        if (*end != '(') continue; // Callback binding is not an invocation.
        bool hazard = false, value = false;
        if (engine) {
            for (unsigned i = 0; i < sizeof(operations) / sizeof(operations[0]); ++i)
                if (strlen(operations[i].name) == length && !memcmp(name, operations[i].name, length)) {
                    hazard = true; value = operations[i].value; break;
                }
        } else if (signal) {
            char symbol[96];
            if (length < sizeof(symbol)) {
                memcpy(symbol, name, length); symbol[length] = 0;
                hazard = strstr(symbol, "_signal_") != NULL ||
                         strstr(symbol, "_mutating_helper_") != NULL ||
                         strstr(symbol, "_mutating_value_helper_") != NULL;
                value = strstr(symbol, "_mutating_value_helper_") != NULL;
            }
        } else if (!direct_only && functions) {
            for (unsigned i = 0; i < functions->count; ++i) {
                const ScriptFunction* function = &functions->functions[i];
                unsigned prefix = effect_helper ? 17 : 10;
                if (length == strlen(function->name) + prefix &&
                    !memcmp(name + prefix, function->name, length - prefix)) {
                    hazard = source_class != ScriptCallback_Fighter || function->may_invalidate_source;
                    value = function->return_type != ScriptType_Invalid; break;
                }
            }
        }
        if (!hazard) continue;
        const char* after = call_end(end);
        if (!after) continue;
        *call = (LifetimeCall){ cursor, end, after, value };
        return true;
    }
    return false;
}

bool nativeScriptLifetime_DirectEffect(const char* generated)
{
    LifetimeCall call;
    return find_call(generated, NULL, ScriptCallback_Fighter, true, &call);
}

bool nativeScriptLifetime_LineMayInvalidate(const char* generated,
    const ScriptFunctionProgram* functions, ScriptCallbackClass source_class)
{
    LifetimeCall call;
    return find_call(generated, functions, source_class, false, &call);
}

bool nativeScriptLifetime_BlockMayInvalidate(const ScriptCodeBlock* block)
{
    if (block->may_invalidate_source || block->landing) return true;
    if (block->functions)
        for (unsigned i = 0; i < block->functions->count; ++i)
            if (nativeScriptDependencies_Test(block->function_calls, i) &&
                block->functions->functions[i].may_invalidate_source) return true;
    return false;
}

void nativeScriptLifetime_EmitEntry(FILE* output, ScriptCallbackClass source_class)
{
    fprintf(output, "    const uint32_t callback_generation = (uint32_t)%s;\n",
        source_class == ScriptCallback_Fighter ? "fighter->x8_spawnNum" :
        source_class == ScriptCallback_ModelEffect ? "effect->native_generation" : "item->x1C");
}

static void emit_range(FILE* output, const char* begin, const char* end,
    const ScriptFunctionProgram* functions, ScriptCallbackClass source_class,
    ScriptCallbackResult result, unsigned depth)
{
    LifetimeCall call;
    if (depth > 128) {
        fputs("({ _Static_assert(0, \"Callback expression nesting exceeds compiler bound\"); })", output); return;
    }
    if (!find_call(begin, functions, source_class, false, &call) || call.begin >= end) {
        fwrite(begin, 1, (size_t)(end - begin), output); return;
    }
    fwrite(begin, 1, (size_t)(call.begin - begin), output);
    bool contact=(size_t)(call.open-call.begin)==strlen("engine->contact_rebound_allowed") &&
        !memcmp(call.begin,"engine->contact_rebound_allowed",(size_t)(call.open-call.begin));
    fputs(contact ? "({ bool callback_result; NativeScriptVMStatus callback_status = " :
          call.value ? "({ __auto_type callback_result = " : "({ ", output);
    fwrite(call.begin, 1, (size_t)(call.open + 1 - call.begin), output);
    emit_range(output, call.open + 1, call.after - 1, functions, source_class, result, depth + 1);
    if(contact) {
        fputs("); if (callback_status != NativeScriptVM_Ok) return",output);
        fputs(result == ScriptCallback_Void ? "; " : result == ScriptCallback_Float ? " 0.0f; " :
              result == ScriptCallback_Int ? " 0; " : " false; ",output);
        fputs(source_class == ScriptCallback_ModelEffect ? "if (!engine->callback_source_alive(effect_gobj, callback_generation, " :
              "if (!engine->callback_source_alive(gobj, callback_generation, ",output);
    } else fputs(source_class == ScriptCallback_ModelEffect ? "); if (!engine->callback_source_alive(effect_gobj, callback_generation, " :
        "); if (!engine->callback_source_alive(gobj, callback_generation, ", output);
    fputs(source_class == ScriptCallback_Fighter ? "NativeCallbackSource_Fighter" :
        source_class == ScriptCallback_ModelEffect ? "NativeCallbackSource_ModelEffect" : "NativeCallbackSource_Item", output);
    fputs(")) return", output);
    fputs(result == ScriptCallback_Void ? "; " : result == ScriptCallback_Float ? " 0.0f; " :
          result == ScriptCallback_Int ? " 0; " : " false; ", output);
    if (call.value) fputs("callback_result; ", output);
    fputs("})", output);
    emit_range(output, call.after, end, functions, source_class, result, depth);
}

void nativeScriptLifetime_EmitLine(FILE* output, const char* generated,
    const ScriptFunctionProgram* functions, ScriptCallbackClass source_class,
    ScriptCallbackResult result)
{
    emit_range(output, generated, generated + strlen(generated), functions, source_class, result, 0);
}

const char* nativeScriptLifetime_TargetHost(const char* source)
{
    if (!strncmp(source, "not ", 4)) source += 4;
    if (!strncmp(source, "targets.call(", 13)) return "target_call";
    if (!strncmp(source, "targets.call_bool(", 18)) return "target_call_bool";
    if (!strncmp(source, "targets.call_int(", 17)) return "target_call_int";
    if (!strncmp(source, "targets.call_float(", 19)) return "target_call_float";
    return NULL;
}

ScriptExpressionType nativeScriptLifetime_TargetType(const char* source)
{
    const char* name = nativeScriptLifetime_TargetHost(source);
    if (!name) return ScriptType_Invalid;
    if (!strcmp(name, "target_call_int")) return ScriptType_Int;
    if (!strcmp(name, "target_call_float")) return ScriptType_Float;
    return ScriptType_Bool;
}

int nativeScriptLifetime_TargetCall(const char* source, char arguments[2][192], bool* negate)
{
    if (!strstr(source, "targets.call")) return 0;
    *negate = false;
    if (strncmp(source, "not ", 4) == 0) { *negate = true; source += 4; }
    if (!nativeScriptLifetime_TargetHost(source)) return -1;
    const char* after;
    if (!nativeScriptCode_ParseArguments(strchr(source, '('), arguments, 2, &after) || *after ||
        strstr(arguments[0], "targets.call") || strstr(arguments[1], "targets.call")) return -1;
    size_t length = strlen(arguments[1]);
    if (length < 3 || length > 33 || arguments[1][0] != '"' || arguments[1][length - 1] != '"' ||
        !nativeScriptCode_Identifier(arguments[1] + 1, length - 2, 32)) return -1;
    return 1;
}

bool nativeScriptLifetime_TargetSurface(const char* source)
{
    if (!strstr(source, "targets.call")) return true;
    const char* condition = strncmp(source, "if ", 3) == 0 ? source + 3 :
                            strncmp(source, "elif ", 5) == 0 ? source + 5 : NULL;
    char args[2][192]; bool negate;
    if (condition) {
        const char* colon = strchr(condition, ':');
        if (!colon || (size_t)(colon - condition) >= 512) return false;
        char predicate[512]; memcpy(predicate, condition, colon - condition);
        predicate[colon - condition] = 0;
        int result = nativeScriptLifetime_TargetCall(predicate, args, &negate);
        if (result < 0) return false;
        return !colon[1] || (colon[1] == ' ' && nativeScriptLifetime_TargetSurface(colon + 2));
    }
    if (strncmp(source, "while ", 6) == 0) return false;
    if (strncmp(source, "return ", 7) == 0) {
        // Retain the existing dispatch-success statement contract. Typed
        // recipient values can also be returned directly by a typed helper.
        return strncmp(source + 7, "targets.call(", 13) &&
            nativeScriptLifetime_TargetCall(source + 7, args, &negate) == 1;
    }
    const char* assignment = strstr(source, " = ");
    return nativeScriptLifetime_TargetCall(assignment ? assignment + 3 : source, args, &negate) == 1;
}
