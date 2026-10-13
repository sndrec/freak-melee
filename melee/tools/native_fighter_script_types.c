#include "native_fighter_script_types.h"
#include "native_fighter_script_smash.h"
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct TypeParser {
    const char* cursor;
    ScriptTypeResolver resolve;
    const void* context;
    unsigned depth;
} TypeParser;

ScriptExpressionType nativeScriptTypes_State(ScriptStateType type)
{
    return type == ScriptState_Bool ? ScriptType_Bool :
           type == ScriptState_Int ? ScriptType_Int : ScriptType_Float;
}

ScriptExpressionType nativeScriptTypes_CType(const char* type)
{
    return strcmp(type, "bool") == 0 ? ScriptType_Bool :
           strcmp(type, "int32_t") == 0 ? ScriptType_Int : ScriptType_Float;
}

const char* nativeScriptTypes_Name(ScriptExpressionType type)
{
    return type == ScriptType_Bool ? "bool" :
           type == ScriptType_Int ? "int32_t" : "float";
}

bool nativeScriptTypes_Assignable(ScriptExpressionType target,
                                  ScriptExpressionType value)
{
    return value != ScriptType_Invalid &&
           (target != ScriptType_Int || value != ScriptType_Float);
}

ScriptExpressionType nativeScriptTypes_Declaration(const char** text)
{
    static const char* names[] = { "bool ", "int ", "float " };
    static const ScriptExpressionType types[] = {
        ScriptType_Bool, ScriptType_Int, ScriptType_Float
    };
    for (unsigned i = 0; i < 3; ++i) {
        size_t length = strlen(names[i]);
        if (strncmp(*text, names[i], length) == 0) {
            *text += length;
            return types[i];
        }
    }
    return ScriptType_Float;
}

ScriptExpressionType nativeScriptTypes_Builtin(const char* atom)
{
    char name[96];
    size_t length = strcspn(atom, "([ ");
    if (!length || length >= sizeof(name)) return ScriptType_Invalid;
    memcpy(name, atom, length); name[length] = 0;
    ScriptExpressionType smash = nativeScriptSmash_Type(name);
    if (smash != ScriptType_Invalid) return smash;
    static const char* integers[] = {
        "Pose.live", "Pose.blend", "guard.hit_damage",
        "int", "uint", "random.int", "random.int_excluding", "command", "frame.integer", "motion.id",
        "jumps.used", "jumps.max", "bones.count", "bones.index", "player.slot", "costume.index", "costume.variant", "absorber.hits", "absorber.damage",
        "input.horizontal_smash_age", "input.down_release_age", "target.item_kind", "items.held_kind", "items.hold_kind",
        "item.kind", "item.collision_group", "item.motion_state", "item.creation_id", "item.contact_fighter",
        "item.current_owner", "item.reflector", "item.exit_category", "item.collision_mode",
        "owner.motion", "creator.motion", "holder.motion", "targets.motion", "owner.action_category",
        "items.create_engine", "items.held_id", "items.auxiliary_id",
        "audio.play_global", "Collision.floor_mask", "Collision.wall_mask",
        "Collision.left_wall_mask", "Collision.right_wall_mask",
        "Collision.ceiling_mask",
        "creator.action.category", "event.mode", "event.motion", "event.victim_motion", "event.interaction",
        "event.set", "event.set_count", "articles.spawn_attached", "articles.spawn_bone_attached",
        "event.swing_type", "event.swing_variant",
        "event.metadata", "event.move_id", "action.metadata", "action.category", "action.move_id",
        "articles.create_at_position", "articles.create_with_velocity", "articles.create_held_at_center",
        "articles.create_unowned_at_position", "articles.spawn", "stats.action_count", "item.create_at_position",
        "item.link_handle", "item.link_id", "item.create_child", "item.sweep_stage_flags", "stage.internal_kind",
        "item.wall_contact_flags", "item.ceiling_contact_flags",
        "floor.line", "walk.type", "capture.victim_animation_family", "stage.sweep_box", "hitbox.state",
        "cpu.output.left_x", "cpu.output.left_y", "cpu.output.buttons",
        "cpu.output.left_trigger", "cpu.output.right_trigger", "targets.floor_line",
        "cpu.mode", "cpu.level", "cpu.behavior", "cpu.previous_behavior", "cpu.elapsed_frames", "cpu.grab_attempts", "cpu.selected_command",
        "Hitbox.disabled", "Hitbox.enabled",
        "Axis.x", "Axis.y", "Axis.z",
        "visuals.current_group", "visuals.previous_group",
        "items.held_use_category", "controls.aerial_attack_motion",
        "targets.held_id", "targets.held_kind", "targets.held_motion",
        "holder.held_id", "holder.held_kind", "holder.held_motion",
        "event.action",
        "engine.debug_level",
        "fighter.creation_id", "family.member_id", "hitlag.linked_id", "cpu.travel_frames",
        "color.rgba", "visuals.outline_color", "item.outline_color",
        "item.controller_tint", "item.material_opacity", "targets.controller_color",
    };
    static const char* booleans[] = {
        "item.add_model_animation",
        "forms.activate_partner", "hitlag.link", "collision.copy_from", "targets.read_bone_position",
        "hitlag.active", "fighter.incapacitated", "cpu.pending_action", "cpu.combat_candidates_enabled", "cpu.ranged_gate_enabled", "motion.allows_controller_sync",
        "targets.sleeping", "cpu.record_input", "cpu.apply_recorded_input",
        "cpu.prepare_destination_update", "cpu.plan_attack", "cpu.read_history_pose",
        "stage.read_floor", "stage.read_line_island_bounds", "stage.same_line_island",
        "stage.cpu_floor_excluded", "stage.cpu_island_blocked", "targets.read_position_delta",
        "damage.song_immune",
        "visuals.preserve_secondary_material", "visuals.install_outline_visibility",
        "item.layered", "item.controller_tint_enabled", "targets.controller_color_present",
        "item.custom_floor_motion",
        "item.stop_on_surface", "item.reflect_from_wall",
        "attack.enter_jab", "items.try_ground_pickup",
        "input.up_held", "input.z_held", "targets.smash_charge_active",
        "controls.interrupt_allowed",
        "input.attack_held", "input.jump_held", "input.cstick_attack_edge",
        "controls.try_air_item_throw", "collision.contact_rebound_allowed",
        "true", "false", "grounded", "airborne", "effects.active", "items.block_ground_pickup", "animation.finished",
        "input.special_held", "input.special_pressed", "target.is_fighter", "target.is_item",
        "items.held", "capture.ground_link", "cue.reverse.consume", "cue.article.consume",
        "cue.throw.consume", "hitbox.active", "hitbox.enabled", "articles.owned",
        "cpu.ranged_unsafe", "victim.airborne", "event.special_hit",
        "item.grounded", "item.airborne", "item.animation_finished", "item.attachment_active",
        "item.creator_present", "item.remove_at_bounds",
        "item.physics_disabled", "item.suppress_destruction_effects", "item.reverse_spin",
        "item.throw_speed_damage", "item.owner_bound", "item.world_interaction",
        "item.reflection_owner_transfer_disabled", "item.dynamic_ecb", "item.harm_owner",
        "item.harm_teammates", "item.harm_sibling_items", "item.can_be_hit_by_owner",
        "item.can_be_hit_by_teammates", "item.can_be_hit_by_sibling_items", "item.subaction_sounds_muted",
        "item.expire_tick", "item.check_ground", "item.bounce_stage", "item.hits_stage",
        "item.overlaps_owner", "item.attachment_owner_is", "creator.exists", "creator.hitlag",
        "item.spawn_link",
        "articles.bind_held", "articles.bind_auxiliary", "creator.incapacitated",
        "items.attach", "items.inherit_owner_scale", "items.bind_held", "items.bind_auxiliary",
        "items.clear_held", "items.clear_auxiliary",
        "items.bind_existing_held", "items.bind_existing_auxiliary",
        "items.set_visible", "items.set_paused", "items.notify_drop",
        "items.notify_held_neutral", "items.reopen_blocked",
        "owner.incapacitated", "owner.action_detaches_items", "collision.floor_contact",
        "damage.force_top_blast_ko",
        "jumps.configure_multi",
        "model_graphs.install",
        "controls.try_taunt", "controls.try_crouch", "controls.try_turn", "controls.try_walk",
        "guard.active", "guard.reactive", "guard.front_only",
        "guard.filtered_attacks_only", "guard.skip_contact_details",
        "floor.is_platform", "callbacks.accessory_before_is",
        "callbacks.accessory_hitlag_is", "callbacks.death1_is",
        "callbacks.damage_is", "callbacks.damage_secondary_is", "callbacks.death2_is", "reflector.active",
        "input.shield_held", "movement.used_tether", "movement.fastfall",
        "stage.segment_blocked", "collision.probe_ledge", "collision.ledge_available",
        "collision.air", "physics.check_fastfall",
        "collision.ceiling_contact", "collision.left_wall", "collision.right_wall",
        "targets.is_fighter", "targets.guard_active", "targets.airborne", "items.spawn_food", "stage.line_exists", "item.read_stage_line_speed",
    };
    for (unsigned i = 0; i < sizeof(integers) / sizeof(integers[0]); ++i)
        if (strcmp(name, integers[i]) == 0) return ScriptType_Int;
    for (unsigned i = 0; i < sizeof(booleans) / sizeof(booleans[0]); ++i)
        if (strcmp(name, booleans[i]) == 0) return ScriptType_Bool;
    if (strncmp(name, "DebugLevel.", 11) == 0 || strncmp(name, "It_Kind_", 8) == 0 || strncmp(name, "Motion.", 7) == 0 ||
        strncmp(name, "Transition.", 11) == 0 || strncmp(name, "Hurt.", 5) == 0 ||
        strncmp(name, "AnimationFamily.", 16) == 0 ||
        (strncmp(name, "item.command", 12) == 0 && strlen(name) == 13) ||
        (strncmp(name, "owner.command", 13) == 0 && strlen(name) == 14) ||
        (strncmp(name, "creator.command", 15) == 0 && strlen(name) == 16))
        return ScriptType_Int;
    // A valid but unclassified engine query cannot silently feed an integer.
    return ScriptType_Float;
}

static void spaces(TypeParser* parser)
{
    while (isspace((unsigned char)*parser->cursor)) ++parser->cursor;
}

static bool word(const char* text, const char* value)
{
    size_t length = strlen(value);
    return strncmp(text, value, length) == 0 &&
           !isalnum((unsigned char)text[length]) && text[length] != '_';
}

static ScriptExpressionType expression(TypeParser*, unsigned);

static ScriptExpressionType primary(TypeParser* parser)
{
    spaces(parser);
    if (++parser->depth > 32) return ScriptType_Invalid;
    ScriptExpressionType type = ScriptType_Invalid;
    if (*parser->cursor == '(') {
        ++parser->cursor;
        type = expression(parser, 1);
        spaces(parser);
        if (*parser->cursor != ')') type = ScriptType_Invalid;
        else ++parser->cursor;
    } else if (isdigit((unsigned char)*parser->cursor) ||
               (*parser->cursor == '.' && isdigit((unsigned char)parser->cursor[1]))) {
        const char* begin = parser->cursor;
        char* end;
        double number = strtod(begin, &end);
        if (end != begin && isfinite(number)) {
            bool hex = begin[0] == '0' && (begin[1] == 'x' || begin[1] == 'X');
            bool floating = false;
            for (const char* p = begin; p < end; ++p)
                if (*p == '.' || (!hex && (*p == 'e' || *p == 'E')) ||
                    (hex && (*p == 'p' || *p == 'P'))) floating = true;
            type = floating ? ScriptType_Float : ScriptType_Int;
            parser->cursor = end;
        }
    } else if (isalpha((unsigned char)*parser->cursor) || *parser->cursor == '_') {
        const char* begin = parser->cursor;
        while (isalnum((unsigned char)*parser->cursor) ||
               *parser->cursor == '_' || *parser->cursor == '.') ++parser->cursor;
        // Call/array arguments belong to this atom. Inspect their arithmetic,
        // while accepting quoted paths and named enum/bone arguments.
        while (*parser->cursor == '(' || *parser->cursor == '[') {
            char close = *parser->cursor++ == '(' ? ')' : ']';
            spaces(parser);
            while (*parser->cursor && *parser->cursor != close) {
                spaces(parser);
                if (*parser->cursor == '"') {
                    ++parser->cursor;
                    while (*parser->cursor && *parser->cursor != '"') ++parser->cursor;
                    if (*parser->cursor != '"') goto done;
                    ++parser->cursor;
                } else if (expression(parser, 1) == ScriptType_Invalid) goto done;
                spaces(parser);
                if (*parser->cursor != ',') break;
                ++parser->cursor;
            }
            if (*parser->cursor != close) goto done;
            ++parser->cursor;
            if (*parser->cursor == '.') {
                ++parser->cursor;
                while (isalnum((unsigned char)*parser->cursor) ||
                       *parser->cursor == '_' || *parser->cursor == '.') ++parser->cursor;
            }
        }
        size_t length = (size_t)(parser->cursor - begin);
        char atom[1024];
        if (length >= sizeof(atom)) goto done;
        memcpy(atom, begin, length); atom[length] = 0;
        type = parser->resolve(parser->context, atom);
    }
done:
    --parser->depth;
    return type;
}

static ScriptExpressionType unary(TypeParser* parser)
{
    spaces(parser);
    if (word(parser->cursor, "not") || *parser->cursor == '!' ||
        *parser->cursor == '~' || *parser->cursor == '+' || *parser->cursor == '-') {
        bool logical = word(parser->cursor, "not") || *parser->cursor == '!';
        bool bitwise = *parser->cursor == '~';
        parser->cursor += word(parser->cursor, "not") ? 3 : 1;
        if (++parser->depth > 32) return ScriptType_Invalid;
        ScriptExpressionType value = unary(parser);
        --parser->depth;
        if (value == ScriptType_Invalid || (bitwise && value == ScriptType_Float)) return ScriptType_Invalid;
        return logical ? ScriptType_Bool : value == ScriptType_Bool ? ScriptType_Int : value;
    }
    return primary(parser);
}

typedef struct Operator { const char* text; unsigned precedence; unsigned kind; } Operator;
// kind 0 arithmetic, 1 integral-only, 2 comparison/logical.
static const Operator operators[] = {
    { "or", 1, 2 }, { "||", 1, 2 }, { "and", 2, 2 }, { "&&", 2, 2 },
    { "|", 3, 1 }, { "^", 4, 1 }, { "&", 5, 1 },
    { "==", 6, 2 }, { "!=", 6, 2 }, { "<=", 7, 2 }, { ">=", 7, 2 },
    { "<<", 8, 1 }, { ">>", 8, 1 }, { "<", 7, 2 }, { ">", 7, 2 },
    { "+", 9, 0 }, { "-", 9, 0 }, { "*", 10, 0 }, { "/", 10, 0 }, { "%", 10, 1 },
};

static const Operator* next_operator(const char* cursor)
{
    for (unsigned i = 0; i < sizeof(operators) / sizeof(operators[0]); ++i) {
        const Operator* op = &operators[i];
        size_t length = strlen(op->text);
        if (strncmp(cursor, op->text, length) != 0) continue;
        if (isalpha((unsigned char)*op->text) && !word(cursor, op->text)) continue;
        if (length == 1 && (cursor[1] == *cursor || cursor[1] == '=')) continue;
        return op;
    }
    return NULL;
}

static ScriptExpressionType expression(TypeParser* parser, unsigned minimum)
{
    if (++parser->depth > 32) return ScriptType_Invalid;
    ScriptExpressionType value = unary(parser);
    while (value != ScriptType_Invalid) {
        spaces(parser);
        const Operator* op = next_operator(parser->cursor);
        if (op == NULL || op->precedence < minimum) break;
        parser->cursor += strlen(op->text);
        ScriptExpressionType right = expression(parser, op->precedence + 1);
        if (right == ScriptType_Invalid ||
            (op->kind == 1 && (value == ScriptType_Float || right == ScriptType_Float))) {
            value = ScriptType_Invalid; break;
        }
        value = op->kind == 2 ? ScriptType_Bool :
                value == ScriptType_Float || right == ScriptType_Float ? ScriptType_Float : ScriptType_Int;
    }
    --parser->depth;
    return value;
}

ScriptExpressionType nativeScriptTypes_Expression(const char* source,
                                                  ScriptTypeResolver resolve,
                                                  const void* context)
{
    TypeParser parser = { source, resolve, context, 0 };
    ScriptExpressionType type = expression(&parser, 1);
    spaces(&parser);
    return *parser.cursor ? ScriptType_Invalid : type;
}
