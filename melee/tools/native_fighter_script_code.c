#include "native_fighter_script_terrain.h"
#include "native_fighter_script_code.h"
#include "native_fighter_script_spatial.h"
#include "native_fighter_script_colors.h"
#include "native_fighter_script_lifetime.h"
#include "native_fighter_script_functions.h"
#include "native_fighter_script_projectile.h"
#include "native_fighter_script_trail.h"
#include "native_fighter_script_signal.h"
#include "native_fighter_script_model_effect.h"
#include "native_fighter_script_pose.h"
#include "native_fighter_script_queries.h"
#include <melee/ft/ftcharacter_assets.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

bool nativeScriptCode_Contains(const ScriptCodeBlock* block,
                                const char* expression)
{
    for (unsigned i = 0; i < block->count; ++i)
        if (strstr(block->lines[i].text, expression) != NULL) return true;
    return false;
}

const char* nativeScriptCode_SpecialSuffix(ScriptSpecial special)
{
    return special == ScriptSpecial_Neutral ? "n" :
           special == ScriptSpecial_Side ? "s" :
           special == ScriptSpecial_Up ? "hi" : "lw";
}

const char* nativeScriptCode_SpecialEnum(ScriptSpecial special)
{
    return special == ScriptSpecial_Neutral ? "Neutral" :
           special == ScriptSpecial_Side ? "Side" :
           special == ScriptSpecial_Up ? "Up" : "Down";
}

const char* nativeScriptCode_SpecialField(ScriptSpecial special, bool air)
{
    if (special == ScriptSpecial_Neutral)
        return air ? "special_air_n" : "special_n";
    if (special == ScriptSpecial_Side)
        return air ? "special_air_s" : "special_s";
    if (special == ScriptSpecial_Up)
        return air ? "special_air_hi" : "special_hi";
    return air ? "special_air_lw" : "special_lw";
}

uint64_t nativeFighterScript_SourceHash(const void* source, size_t source_size)
{
    const uint8_t* bytes = source;
    uint64_t hash = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < source_size; ++i) {
        hash ^= bytes[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

bool nativeScriptCode_Fail(NativeFighterScriptError* error, unsigned line,
                 const char* message)
{
    error->line = line;
    snprintf(error->message, sizeof(error->message), "%s", message);
    return false;
}

bool nativeScriptCode_Identifier(const char* text, size_t size, size_t maximum)
{
    if (size == 0 || size >= maximum ||
        !((text[0] >= 'A' && text[0] <= 'Z') ||
          (text[0] >= 'a' && text[0] <= 'z') || text[0] == '_')) return false;
    for (size_t i = 1; i < size; ++i) {
        if (!((text[i] >= 'A' && text[i] <= 'Z') ||
              (text[i] >= 'a' && text[i] <= 'z') ||
              (text[i] >= '0' && text[i] <= '9') ||
              text[i] == '_')) return false;
    }
    return true;
}

bool nativeScriptCode_ValidKey(const char* key)
{
    size_t length = strnlen(key, 32);
    if (length == 0 || length >= 32) return false;
    for (size_t i = 0; i < length; ++i) {
        char c = key[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
              c == '_')) return false;
    }
    return true;
}

const char* nativeScriptCode_BoneSymbol(const char* name, size_t size)
{
    if (size == 6 && memcmp(name, "TransN", size) == 0)
        return "FtPart_TransN";
    if (size == 7 && memcmp(name, "TransN2", size) == 0)
        return "FtPart_TransN2";
    if (size == 5 && memcmp(name, "XRotN", size) == 0)
        return "FtPart_XRotN";
    if (size == 5 && memcmp(name, "YRotN", size) == 0)
        return "FtPart_YRotN";
    if (size == 3 && memcmp(name, "Top", size) == 0)
        return "FtPart_TopN";
    if (size == 8 && memcmp(name, "LeftHand", size) == 0)
        return "FtPart_L1stNb";
    if (size == 9 && memcmp(name, "RightHand", size) == 0)
        return "FtPart_R1stNb";
    if (size == 10 && memcmp(name, "RightThumb", size) == 0)
        return "FtPart_RThumbNb";
    if (size == 9 && memcmp(name, "LeftThumb", size) == 0)
        return "FtPart_LThumbNb";
    if (size == 3 && memcmp(name, "Hip", size) == 0)
        return "FtPart_HipN";
    if (size == 8 && memcmp(name, "LeftFoot", size) == 0)
        return "FtPart_LFootJA";
    if (size == 9 && memcmp(name, "RightFoot", size) == 0)
        return "FtPart_RFootJA";
    if (size == 13 && memcmp(name, "RightShoulder", size) == 0)
        return "FtPart_RShoulderN";
    if (size == 12 && memcmp(name, "LeftShoulder", size) == 0)
        return "FtPart_LShoulderN";
    return NULL;
}

bool nativeScriptCode_ParseScalar(const char* text, const ScriptStateTable* table,
                         ScriptScalar* scalar)
{
    bool program_parameter = strncmp(text, "program.attrs.", 14) == 0;
    if (program_parameter || strncmp(text, "attrs.", 6) == 0) {
        const char* field = text + (program_parameter ? 14 : 6);
        if (table != NULL && table->parameters != NULL)
            for (unsigned i = 0; i < table->parameters->count; ++i)
                if (strcmp(field, table->parameters->fields[i].name) == 0) {
                    scalar->parameter_index_plus_one = (uint16_t) (i + 1);
                    scalar->parameter_type = table->parameters->fields[i].type;
                    return true;
                }
        // Explicit program attributes select only the shared published
        // parameter array; never fall back to per-Fighter DAT offsets.
        if (program_parameter) return false;
        if (table != NULL)
            for (unsigned i = 0; i < table->attribute_count; ++i)
                if (strcmp(field, table->attributes[i].name) == 0) {
                    scalar->declared_offset_plus_one =
                        table->attributes[i].offset + 1;
                    scalar->declared_int = table->attributes[i].is_int;
                    scalar->declared_bool = table->attributes[i].is_bool;
                    return true;
                }
        return false;
    }
    char* tail = NULL;
    errno = 0;
    float number = strtof(text, &tail);
    if (errno != 0 || tail == text || *tail != '\0' || !isfinite(number) ||
        number < -20.0f || number > 20.0f) return false;
    scalar->number = number;
    return true;
}

bool nativeScriptCode_AddAttribute(ScriptStateTable* table, const char* line)
{
    static const char float_prefix[] = "attribute float ";
    static const char int_prefix[] = "attribute int ";
    static const char bool_prefix[] = "attribute bool ";
    bool is_bool = strncmp(line, bool_prefix, sizeof(bool_prefix) - 1) == 0;
    bool is_int = strncmp(line, int_prefix, sizeof(int_prefix) - 1) == 0;
    const char* name = NULL;
    if (is_bool) name = line + sizeof(bool_prefix) - 1;
    else if (is_int) name = line + sizeof(int_prefix) - 1;
    else if (strncmp(line, float_prefix, sizeof(float_prefix) - 1) == 0)
        name = line + sizeof(float_prefix) - 1;
    if (name == NULL ||
        table->attribute_count ==
            sizeof(table->attributes) / sizeof(table->attributes[0]))
        return false;
    const char* separator = strchr(name, ' ');
    if (separator == NULL || separator - name >=
            sizeof(table->attributes[0].name) ||
        !nativeScriptCode_Identifier(name, (size_t) (separator - name),
                    sizeof(table->attributes[0].name))) return false;
    char* end = NULL;
    errno = 0;
    unsigned long offset = strtoul(separator + 1, &end, 0);
    if (errno != 0 || end == separator + 1 || *end != '\0' ||
        (!is_bool && (offset & 3) != 0) ||
        offset > FT_CHARACTER_EXT_ATTR_CAPACITY - (is_bool ? 1 : sizeof(float)))
        return false;
    if (table->parameters != NULL)
        for (unsigned i = 0; i < table->parameters->count; ++i)
            if (strlen(table->parameters->fields[i].name) ==
                    (size_t) (separator - name) &&
                memcmp(table->parameters->fields[i].name, name,
                       (size_t) (separator - name)) == 0)
                return false;
    for (unsigned i = 0; i < table->attribute_count; ++i)
        if (strlen(table->attributes[i].name) == (size_t) (separator - name) &&
            memcmp(table->attributes[i].name, name,
                   (size_t) (separator - name)) == 0)
            return false;
    unsigned index = table->attribute_count++;
    memcpy(table->attributes[index].name, name,
           (size_t) (separator - name));
    table->attributes[index].name[separator - name] = '\0';
    table->attributes[index].offset = (uint16_t) offset;
    table->attributes[index].is_int = is_int;
    table->attributes[index].is_bool = is_bool;
    return true;
}

static bool append_code(char* output, size_t capacity, size_t* used,
                        const char* text)
{
    size_t length = strlen(text);
    if (length >= capacity - *used) return false;
    memcpy(output + *used, text, length + 1);
    *used += length;
    return true;
}

static bool interrupt_local(const ScriptCodeBlock* block,
                            const char* name)
{
    for (unsigned i = 0; i < block->local_count; ++i)
        if (strcmp(block->locals[i], name) == 0) return true;
    return false;
}

static ScriptExpressionType fighter_atom_type(const void* context, const char* atom)
{
    const ScriptCodeBlock* block = context;
    const char* query_names[] = { "cpu.nearest_enemy_at", "articles.read_primary_translation", "targets.read_position", "targets.read_position_delta", "cpu.read_history_pose", "stage.read_floor", "stage.read_line_island_bounds" };
    for (unsigned i = 0; i < sizeof(query_names) / sizeof(*query_names); ++i) {
        size_t n = strlen(query_names[i]);
        if (!strncmp(atom, query_names[i], n) && (atom[n] == '(' || !atom[n])) {
            char emitted[4096]; const char* after = atom + n;
            return nativeScriptQueries_Fighter(block, query_names[i], &after,
                emitted, sizeof(emitted)) == 1 && !*after ?
                (i == 0 ? ScriptType_Int : ScriptType_Bool) : ScriptType_Invalid;
        }
    }
    const char* call = strchr(atom, '(');
    if (call != NULL && (size_t)(call - atom) < 96) {
        char name[96], emitted[1536]; const char* after = call;
        memcpy(name, atom, call - atom); name[call - atom] = 0;
        int cpu = nativeScriptCpu_Expression(block, name, &after, emitted, sizeof(emitted));
        if (cpu) return cpu > 0 && !*after ? ScriptType_Bool : ScriptType_Invalid;
    }
    if (!strncmp(atom, "hitbox.current", 14) || !strncmp(atom, "hitbox.previous", 15))
        return nativeScriptTerrain_Type(block, atom);
    ScriptExpressionType helper_type = nativeScriptFunctions_Result(block, atom);
    if (helper_type != ScriptType_Invalid) return helper_type;
    if (block->model_effect && (!strncmp(atom, "effect.", 7) || !strncmp(atom, "creator.", 8)))
        return nativeScriptModelEffect_Type(block, atom);
    ScriptExpressionType target_type = nativeScriptLifetime_TargetType(atom);
    if (target_type != ScriptType_Invalid) return target_type;
    if (strcmp(atom, "event.history_source") == 0)
        return block->cpu_prepare_event ? ScriptType_Int : ScriptType_Invalid;
    if (strcmp(atom, "event.previous_motion") == 0)
        return block->held_motion_event || block->air_resume_event ? ScriptType_Int : ScriptType_Invalid;
    if (strcmp(atom, "event.victim_motion") == 0)
        return block->victim_motion_event ? ScriptType_Int : ScriptType_Invalid;
    if (strcmp(atom, "event.blend") == 0)
        return block->neutral_enter_event ? ScriptType_Float : ScriptType_Invalid;
    if (strcmp(atom, "event.interrupt_allowed") == 0)
        return block->landing_action_event ? ScriptType_Bool : ScriptType_Invalid;
    if (strcmp(atom, "event.prediction_frames") == 0)
        return block->cpu_projectile_hit_event ? ScriptType_Int : ScriptType_Invalid;
    if (strcmp(atom, "event.unblockable") == 0)
        return block->cpu_projectile_hit_event ? ScriptType_Bool : ScriptType_Invalid;
    if (strcmp(atom, "event.category") == 0)
        return block->cpu_projectile_defense_event ? ScriptType_Int : ScriptType_Invalid;
    NativeScriptArticleReference article_ref;
    int article_found = nativeScriptArticleRefs_Reference(block->articles, atom, &article_ref);
    if (article_found < 0) return ScriptType_Invalid;
    if (article_found > 0) {
        if (*article_ref.after ||
            nativeScriptCode_ExpressionType(block, article_ref.handle) != ScriptType_Int ||
            (article_ref.field && nativeScriptCode_ExpressionType(block, article_ref.index) != ScriptType_Int))
            return ScriptType_Invalid;
        return nativeScriptTypes_State(article_ref.type);
    }
    NativeScriptStateReference reference;
    int found = nativeScriptState_Reference(block->state, atom, "state.", &reference);
    if (found < 0) return ScriptType_Invalid;
    if (found > 0 && *reference.after == 0)
        return nativeScriptTypes_State(reference.field->type);
    for (unsigned i = 0; i < block->local_count; ++i)
        if (strcmp(block->locals[i], atom) == 0) return block->local_types[i];
    if (strncmp(atom, "attrs.", 6) == 0 || strncmp(atom, "program.attrs.", 14) == 0) {
        ScriptScalar scalar;
        if (!nativeScriptCode_ParseScalar(atom, block->state, &scalar)) return ScriptType_Invalid;
        if (scalar.parameter_index_plus_one)
            return scalar.parameter_type == NativeFighterParameter_Float ? ScriptType_Float :
                   scalar.parameter_type == NativeFighterParameter_Bool ? ScriptType_Bool : ScriptType_Int;
        return scalar.declared_bool ? ScriptType_Bool : scalar.declared_int ? ScriptType_Int : ScriptType_Float;
    }
    return nativeScriptTypes_Builtin(atom);
}

ScriptExpressionType nativeScriptCode_ExpressionType(const ScriptCodeBlock* block,
                                                      const char* source)
{
    return nativeScriptTypes_Expression(source, fighter_atom_type, block);
}

bool nativeScriptCode_ParseArguments(const char* source, char arguments[][192],
                                 unsigned expected, const char** after)
{
    if (*source != '(') return false;
    unsigned count = 0, depth = 1;
    const char* start = source + 1;
    for (const char* cursor = start; *cursor != '\0'; ++cursor) {
        if (*cursor == '(') ++depth;
        if (*cursor == ')') {
            if (--depth == 0) {
                if (expected == 0) {
                    const char* empty = start;
                    while (empty < cursor && isspace((unsigned char)*empty)) ++empty;
                    if (empty != cursor) return false;
                    *after = cursor + 1;
                    return true;
                }
                if (count >= expected) return false;
                const char* first = start;
                const char* last = cursor;
                while (first < last && isspace((unsigned char) *first)) ++first;
                while (last > first && isspace((unsigned char) last[-1])) --last;
                size_t length = (size_t) (last - first);
                if (length == 0 || length >= 192) return false;
                memcpy(arguments[count], first, length);
                arguments[count++][length] = '\0';
                *after = cursor + 1;
                return count == expected;
            }
        }
        if (*cursor != ',' || depth != 1) continue;
        if (count >= expected) return false;
        const char* first = start;
        const char* last = cursor;
        while (first < last && isspace((unsigned char) *first)) ++first;
        while (last > first && isspace((unsigned char) last[-1])) --last;
        size_t length = (size_t) (last - first);
        if (length == 0 || length >= 192) return false;
        memcpy(arguments[count], first, length);
        arguments[count++][length] = '\0';
        start = cursor + 1;
    }
    return false;
}

bool nativeScriptCode_Expression(const ScriptCodeBlock* block,
                                 const char* source, char* output,
                                 size_t capacity)
{
    char dispatch_args[2][192]; bool negate;
    int dispatch = nativeScriptLifetime_TargetCall(source, dispatch_args, &negate);
    if (block->model_effect && dispatch != 0) return false;
    if (dispatch < 0) return false;
    if (dispatch > 0) {
        char handle[1024];
        if (nativeScriptCode_ExpressionType(block, dispatch_args[0]) != ScriptType_Int ||
            !nativeScriptCode_Expression(block, dispatch_args[0], handle, sizeof(handle))) return false;
        dispatch_args[1][strlen(dispatch_args[1]) - 1] = 0;
        return snprintf(output, capacity, "%sengine->%s((uint32_t)(%s), UINT32_C(%u))",
            negate ? "!" : "", nativeScriptLifetime_TargetHost(source), handle,
            nativeScriptSignal_Hash(dispatch_args[1] + 1)) < capacity;
    }
    size_t used = 0;
    output[0] = '\0';
    while (*source != '\0') {
        char helper[1024]; const char* helper_after;
        int helper_call = nativeScriptFunctions_Expression(block, source, helper, sizeof(helper), &helper_after);
        if (helper_call < 0) return false;
        if (helper_call > 0) {
            if (!append_code(output, capacity, &used, helper)) return false;
            source = helper_after;
            continue;
        }
        if (block->model_effect) {
            char emitted[1200]; const char* after;
            int effect = nativeScriptModelEffect_Expression(block, source, emitted, sizeof(emitted), &after);
            if (effect < 0 || !strncmp(source, "state.", 6) || !strncmp(source, "articles.", 9)) return false;
            if (effect > 0) {
                if (!append_code(output, capacity, &used, emitted)) return false;
                source = after; continue;
            }
        }
        NativeScriptArticleReference article_ref;
        int article_found = nativeScriptArticleRefs_Reference(block->articles, source, &article_ref);
        if (article_found < 0) return false;
        if (article_found > 0) {
            char handle[1024], index[1024] = "0", translated[2500];
            if (nativeScriptCode_ExpressionType(block, article_ref.handle) != ScriptType_Int ||
                !nativeScriptCode_Expression(block, article_ref.handle, handle, sizeof(handle)) ||
                (article_ref.field &&
                 (nativeScriptCode_ExpressionType(block, article_ref.index) != ScriptType_Int ||
                  !nativeScriptCode_Expression(block, article_ref.index, index, sizeof(index)))) ||
                !nativeScriptArticleRefs_Emit(&article_ref, handle, index, translated, sizeof(translated)) ||
                !append_code(output, capacity, &used, translated)) return false;
            source = article_ref.after;
            continue;
        }
        NativeScriptStateReference reference;
        int state_reference = nativeScriptState_Reference(block->state, source,
                                                          "state.", &reference);
        if (state_reference < 0) return false;
        if (state_reference > 0 && reference.field->array) {
            char index[1024], access[1200];
            if (!nativeScriptCode_Expression(block, reference.index, index, sizeof(index)) ||
                !nativeScriptState_ArrayRead(reference.field,
                    "fighter->native_module_state + script_state_base", index,
                    access, sizeof(access)) ||
                !append_code(output, capacity, &used, access)) return false;
            source = reference.after;
            continue;
        }
        if (source[0] == '0' && (source[1] == 'x' || source[1] == 'X')) {
            char* end;
            unsigned long long value = strtoull(source, &end, 16);
            if (end == source + 2 || value > UINT32_MAX ||
                isalnum((unsigned char) *end) || *end == '_') return false;
            char literal[24];
            snprintf(literal, sizeof(literal), "%llu", value);
            if (!append_code(output, capacity, &used, literal)) return false;
            source = end;
            continue;
        }
        if (isspace((unsigned char) *source)) {
            if (!append_code(output, capacity, &used, " ")) return false;
            ++source;
            continue;
        }
        if (isalpha((unsigned char) *source) || *source == '_') {
            char name[160]; // Full parameter name plus program.attrs prefix.
            size_t length = 0;
            while (isalnum((unsigned char) source[length]) ||
                   source[length] == '_' || source[length] == '.') {
                if (length + 1 >= sizeof(name)) return false;
                ++length;
            }
            memcpy(name, source, length);
            name[length] = '\0';
            source += length;
            if (block->item_event && (!strncmp(name, "hitlag.", 7) ||
                !strcmp(name, "fighter.incapacitated") || !strcmp(name, "spawn.horizontal_offset") ||
                !strncmp(name, "family.", 7) || !strcmp(name, "cpu.travel_frames") ||
                !strcmp(name, "cpu.pending_action") || !strcmp(name, "targets.read_bone_position"))) return false;
            if (block->model_effect && !nativeScriptModelEffect_AllowedName(block, name)) return false;
            if (strcmp(name, "hitbox.current") == 0 || strcmp(name, "hitbox.previous") == 0) {
                char coordinate[1200];
                if (!nativeScriptTerrain_Expression(block, name, &source, coordinate, sizeof(coordinate)) ||
                    !append_code(output, capacity, &used, coordinate)) return false;
                continue;
            }
            const char* replacement = NULL;
            bool allows_parenthesis = false;
            if (strcmp(name, "command") == 0 &&
                source[0] == '(' && source[1] >= '0' &&
                source[1] <= '3' && source[2] == ')') {
                char access[32];
                snprintf(access, sizeof(access),
                         "fighter->cmd_vars[%c]", source[1]);
                if (!append_code(output, capacity, &used, access))
                    return false;
                source += 3;
                continue;
            } else if (strcmp(name, "cue.reverse.consume") == 0 &&
                       strncmp(source, "()", 2) == 0) {
                replacement = "(fighter->throw_flags_b3 ? "
                    "(fighter->throw_flags_b3 = 0, 1) : 0)";
                source += 2;
            } else if (strcmp(name, "cue.article.consume") == 0 &&
                       strncmp(source, "()", 2) == 0) {
                replacement = "(fighter->throw_flags_b0 ? "
                    "(fighter->throw_flags_b0 = 0, 1) : 0)";
                source += 2;
            } else if (strcmp(name, "cue.throw.consume") == 0 &&
                       strncmp(source, "()", 2) == 0) {
                replacement = "(fighter->throw_flags_b1 ? "
                    "(fighter->throw_flags_b1 = 0, 1) : 0)";
                source += 2;
            } else if (strcmp(name, "input.x") == 0) {
                replacement = "fighter->input.lstick[0].x";
            } else if (strcmp(name, "input.y") == 0) {
                replacement = "fighter->input.lstick[0].y";
            } else if (strcmp(name, "input.special_held") == 0) {
                replacement = "(fighter->input.held_buttons[0] & HSD_PAD_B)";
            } else if (strcmp(name, "input.special_pressed") == 0) {
                replacement = "(fighter->input.pressed_buttons & HSD_PAD_B)";
            } else if (strcmp(name, "input.horizontal_smash_age") == 0) {
                replacement = "fighter->x673";
            } else if (strcmp(name, "frame") == 0) {
                replacement = "fighter->cur_anim_frame";
            } else if (strcmp(name, "frame.integer") == 0) {
                replacement = "((int) fighter->cur_anim_frame)";
            } else if (strcmp(name, "items.block_ground_pickup") == 0) {
                replacement = "fighter->x2223_b4";
            } else if (strcmp(name, "effects.active") == 0) {
                replacement = "fighter->x2219_b0";
            } else if (strcmp(name, "velocity.x") == 0) {
                replacement = "fighter->self_vel.x";
            } else if (strcmp(name, "velocity.y") == 0) {
                replacement = "fighter->self_vel.y";
            } else if (strcmp(name, "velocity.z") == 0) {
                replacement = "fighter->self_vel.z";
            } else if (strcmp(name, "acceleration.x") == 0) {
                replacement = "fighter->x74_self_accel.x";
            } else if (strcmp(name, "acceleration.z") == 0) {
                replacement = "fighter->x74_self_accel.z";
            } else if (strcmp(name, "acceleration.y") == 0) {
                replacement = "fighter->x74_self_accel.y";
            } else if (strcmp(name, "air.drift_max") == 0) {
                replacement = "fighter->co_attrs.air_drift_max";
            } else if (strcmp(name, "air.drift_stick_mul") == 0) {
                replacement = "fighter->co_attrs.air_drift_stick_mul";
            } else if (strcmp(name, "air.terminal_velocity") == 0) {
                replacement = "fighter->co_attrs.terminal_velocity";
            } else if (strcmp(name, "jumps.max") == 0) {
                replacement = "fighter->co_attrs.max_jumps";
            } else if (strcmp(name, "jumps.used") == 0) {
                replacement = "fighter->x1968_jumpsUsed";
            } else if (block->grab_catch_event &&
                       strcmp(name, "victim.airborne") == 0) {
                replacement = "(((Fighter*) fighter->victim_gobj->user_data)->ground_or_air == GA_Air)";
            } else if (block->shield_transition_event &&
                       strcmp(name, "event.mode") == 0) {
                replacement = "shield_mode";
            } else if (block->shield_transition_event &&
                       strcmp(name, "event.special_hit") == 0) {
                replacement = "special_hit";
            } else if (strcmp(name, "capture.ground_link") == 0) {
                replacement = "fighter->x221B_b7";
            } else if (strcmp(name, "cpu.ranged_unsafe") == 0 &&
                       *source == '(') {
                char args[1][192], translated[512], call[640];
                const char* after;
                if (!nativeScriptCode_ParseArguments(source, args, 1, &after) ||
                    !nativeScriptCode_Expression(block, args[0], translated,
                                                 sizeof(translated)) ||
                    snprintf(call, sizeof(call),
                             "engine->cpu_ranged_unsafe(fighter, fighter->cpu.x44, %s)",
                             translated) >= sizeof(call) ||
                    !append_code(output, capacity, &used, call)) return false;
                source = after;
                continue;
            } else if (block->other_fighter_event &&
                       strcmp(name, "other.position.x") == 0) {
                replacement = "(((Fighter*) other_gobj->user_data)->cur_pos.x)";
            } else if (block->other_fighter_event &&
                       strcmp(name, "other.position.y") == 0) {
                replacement = "(((Fighter*) other_gobj->user_data)->cur_pos.y)";
            } else if (block->other_fighter_event &&
                       strcmp(name, "other.cpu.body_half_width") == 0) {
                replacement = "(((Fighter*) other_gobj->user_data)->cpu.x564)";
            } else if (strcmp(name, "physics.quick_decel") == 0 &&
                       *source == '(') {
                const char* end = strchr(source + 1, ')');
                if (end == NULL || end - source >= 192) return false;
                char argument[192], translated[512], call[640];
                size_t length = (size_t) (end - source - 1);
                memcpy(argument, source + 1, length);
                argument[length] = '\0';
                if (!nativeScriptCode_Expression(block, argument, translated,
                                          sizeof(translated)) ||
                    snprintf(call, sizeof(call),
                             "engine->quick_decel(fighter, %s)",
                             translated) >= sizeof(call) ||
                    !append_code(output, capacity, &used, call)) return false;
                source = end + 1;
                continue;
            } else if (strcmp(name, "ground.velocity") == 0) {
                replacement = "fighter->gr_vel";
            } else if (strcmp(name, "ground.friction") == 0) {
                replacement = "fighter->co_attrs.ground_friction";
            } else if (strcmp(name, "motion.id") == 0) {
                replacement = "fighter->motion_id";
            } else if (strcmp(name, "target.is_fighter") == 0) {
                replacement = "(fighter->unk_gobj != NULL && "
                    "fighter->unk_gobj->classifier == HSD_GOBJ_CLASS_FIGHTER)";
            } else if (strcmp(name, "target.is_item") == 0) {
                replacement = "(fighter->unk_gobj != NULL && "
                    "fighter->unk_gobj->classifier == HSD_GOBJ_CLASS_ITEM)";
            } else if (strcmp(name, "target.item_kind") == 0) {
                replacement = "engine->detected_item_kind(fighter)";
            } else if (strcmp(name, "animation.finished") == 0) {
                replacement = "(!engine->animation_frames_remaining(gobj))";
            } else if (strcmp(name, "items.held") == 0) {
                replacement = "(fighter->item_gobj != NULL)";
            } else if (strcmp(name, "items.held_kind") == 0) {
                replacement = "(fighter->item_gobj != NULL ? engine->held_item_kind(fighter->item_gobj) : -1)";
            } else if (strcmp(name, "hitbox.active") == 0 &&
                       *source == '(') {
                char* end = NULL;
                unsigned long index = strtoul(source + 1, &end, 0);
                if (end == source + 1 || index >= 4 ||
                    end[0] != ')') return false;
                char access[96];
                snprintf(access, sizeof(access),
                    "(fighter->x914[%lu].state == HitCapsule_Enabled)", index);
                if (!append_code(output, capacity, &used, access)) return false;
                source = end + 1;
                continue;
            } else if (strcmp(name, "articles.spawn_attached") == 0 &&
                       *source == '(') {
                char args[7][192], values[5][512], call[2048];
                const char* after;
                if (!nativeScriptCode_ParseArguments(source, args, 7, &after) ||
                    strncmp(args[0], "It_Kind_", 8) != 0 ||
                    !nativeScriptCode_Identifier(args[0], strlen(args[0]), sizeof(args[0])))
                    return false;
                const char* bone = nativeScriptCode_BoneSymbol(args[1], strlen(args[1]));
                if (bone == NULL) return false;
                for (unsigned i = 0; i < 5; ++i)
                    if (!nativeScriptCode_Expression(block, args[i + 2], values[i],
                                              sizeof(values[i]))) return false;
                if (snprintf(call, sizeof(call),
                    "engine->spawn_attached_article(gobj, %s, %s, "
                    "(s32) (%s), (s32) (%s), (s32) (%s), (s32) (%s), "
                    "(float) (%s))", args[0], bone, values[0], values[1],
                    values[2], values[3], values[4]) >= sizeof(call) ||
                    !append_code(output, capacity, &used, call)) return false;
                source = after;
                continue;
            } else if (strcmp(name, "articles.create_unowned_at_position") == 0 && *source == '(') {
                char args[8][192], values[8][256], call[2560]; const char* after;
                if (!nativeScriptCode_ParseArguments(source, args, 8, &after)) return false;
                for (unsigned i = 0; i < 8; ++i)
                    if (!nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return false;
                if (snprintf(call, sizeof(call),
                    "engine->create_unowned_article_at(gobj, (ItemKind)(%s), &(Vec3){ %s, %s, %s }, &(Vec3){ %s, %s, %s }, %s)",
                    values[0], values[1], values[2], values[3], values[4], values[5], values[6], values[7]) >= sizeof(call) ||
                    !append_code(output, capacity, &used, call)) return false;
                source = after; continue;
            } else if (strcmp(name, "articles.create_held_at_center") == 0 && *source == '(') {
                char args[6][192], values[6][384], call[2560]; const char* after;
                if (!nativeScriptCode_ParseArguments(source, args, 6, &after)) return false;
                for (unsigned i = 0; i < 6; ++i)
                    if (!nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return false;
                if (snprintf(call, sizeof(call),
                    "engine->create_held_article_at_center(gobj, (ItemKind)(%s), &(Vec3){ %s, %s, %s }, (unsigned)(%s), %s)",
                    values[0], values[1], values[2], values[3], values[4], values[5]) >= sizeof(call) ||
                    !append_code(output, capacity, &used, call)) return false;
                source = after; continue;
            } else if (strcmp(name, "articles.spawn_bone_attached") == 0 &&
                       *source == '(') {
                char args[2][192], kind[384], call[512];
                const char* after;
                if (!nativeScriptCode_ParseArguments(source, args, 2, &after) ||
                    !nativeScriptCode_Expression(block, args[0], kind, sizeof(kind)))
                    return false;
                const char* bone = nativeScriptCode_BoneSymbol(args[1], strlen(args[1]));
                if (bone == NULL ||
                    snprintf(call, sizeof(call),
                             "engine->spawn_bone_attached_article(gobj, %s, %s)",
                             kind, bone) >= sizeof(call) ||
                    !append_code(output, capacity, &used, call)) return false;
                source = after;
                continue;
            } else if (strcmp(name, "articles.owned") == 0 &&
                       *source == '(') {
                const char* end = strchr(source + 1, ')');
                if (end == NULL || end - source >= 192) return false;
                char argument[192], translated[512], call[640];
                size_t length = (size_t) (end - source - 1);
                memcpy(argument, source + 1, length);
                argument[length] = '\0';
                if (!nativeScriptCode_Expression(block, argument, translated,
                                          sizeof(translated)) ||
                    snprintf(call, sizeof(call),
                             "engine->owned_article_exists(gobj, (s32) (%s))",
                             translated) >= sizeof(call) ||
                    !append_code(output, capacity, &used, call)) return false;
                source = end + 1;
                continue;
            } else if (strcmp(name, "items.pull") == 0 &&
                       *source == '(') {
                const char* end = strchr(source + 1, ')');
                if (end == NULL || end - source >= 192) return false;
                char argument[192], translated[512], call[640];
                size_t length = (size_t) (end - source - 1);
                memcpy(argument, source + 1, length);
                argument[length] = '\0';
                if (!nativeScriptCode_Expression(block, argument, translated,
                                          sizeof(translated)) ||
                    snprintf(call, sizeof(call),
                             "engine->spawn_holdable_item(gobj, (uint16_t) (%s))",
                             translated) >= sizeof(call) ||
                    !append_code(output, capacity, &used, call)) return false;
                source = end + 1;
                continue;
            } else if (block->collision &&
                       strcmp(name, "collision.left_ground") == 0) {
                replacement = "(!engine->ground_collision(gobj))";
            } else if (block->collision &&
                       strcmp(name, "collision.special_ground_lost") == 0) {
                replacement = "(!engine->special_ground_collision(gobj))";
            } else if (block->collision &&
                       strcmp(name, "collision.landed") == 0) {
                replacement = "(engine->air_collision(gobj) == GA_Air)";
            } else if (block->collision &&
                       strcmp(name, "collision.wall_contact") == 0) {
                replacement = "((fighter->coll_data.env_flags & "
                    "(Collide_LeftWallMask | Collide_RightWallMask)) != 0)";
            } else if (block->collision &&
                       strcmp(name, "collision.wall_ahead") == 0) {
                replacement = "((fighter->facing_dir == 1 && "
                    "(fighter->coll_data.env_flags & Collide_LeftWallMask)) "
                    "|| (fighter->facing_dir == -1 && "
                    "(fighter->coll_data.env_flags & Collide_RightWallMask)))";
            } else if (block->collision &&
                       strcmp(name, "collision.wall_hug_ahead") == 0) {
                replacement = "((fighter->facing_dir == 1 && "
                    "(fighter->coll_data.env_flags & Collide_LeftWallHug)) "
                    "|| (fighter->facing_dir == -1 && "
                    "(fighter->coll_data.env_flags & Collide_RightWallHug)))";
            } else if (strcmp(name, "collision.check_ground_ledge") == 0 && *source == '(') {
                char args[1][192], value[192], call[256]; const char* after;
                if (!nativeScriptCode_ParseArguments(source, args, 1, &after) ||
                    !nativeScriptCode_Expression(block, args[0], value, sizeof(value))) return false;
                int written = snprintf(call, sizeof(call), "engine->check_ground_and_ledge(gobj, (int)(%s))", value);
                if (written < 0 || written >= sizeof(call) ||
                    !append_code(output, capacity, &used, call)) return false;
                source = after;
                continue;
            } else if (strcmp(name, "collision.try_wall_jump") == 0 &&
                       strncmp(source, "()", 2) == 0) {
                if (!append_code(output, capacity, &used,
                                 "engine->try_wall_jump(gobj)")) return false;
                source += 2;
                continue;
            } else if (strcmp(name, "collision.cliff_catch") == 0 &&
                       strncmp(source, "()", 2) == 0) {
                if (!append_code(output, capacity, &used,
                                 "engine->check_cliff_catch(gobj)"))
                    return false;
                source += 2;
                continue;
            } else if (block->collision &&
                       (strcmp(name, "collision.ground_box") == 0 ||
                        strcmp(name, "collision.air_box") == 0)) {
                if (*source != '(') return false;
                const bool ground = strcmp(name, "collision.ground_box") == 0;
                char values[6][160];
                const char* cursor = source + 1;
                for (unsigned i = 0; i < 6; ++i) {
                    const char* begin = cursor;
                    unsigned nested = 0;
                    while (*cursor != '\0') {
                        if (*cursor == '(') ++nested;
                        else if (*cursor == ')' && nested != 0) --nested;
                        else if (nested == 0 &&
                                 (*cursor == ',' || *cursor == ')')) break;
                        ++cursor;
                    }
                    if (*cursor != (i == 5 ? ')' : ',') ||
                        cursor - begin >= 192) return false;
                    char argument[192];
                    size_t length = (size_t) (cursor - begin);
                    memcpy(argument, begin, length);
                    argument[length] = '\0';
                    if (!nativeScriptCode_Expression(block, argument, values[i],
                                              sizeof(values[i]))) return false;
                    ++cursor;
                }
                char box[1024];
                int length = snprintf(box, sizeof(box),
                    "engine->%s_box_collision(gobj, &(ftCollisionBox){ %s, %s, { %s, %s }, { %s, %s } })",
                    ground ? "ground" : "air", values[0], values[1],
                    values[2], values[3], values[4], values[5]);
                if (length < 0 || length >= sizeof(box) ||
                    !append_code(output, capacity, &used, box)) return false;
                source = cursor;
                continue;
            } else if (strcmp(name, "airborne") == 0) {
                replacement = "(fighter->ground_or_air == GA_Air)";
            } else if (strcmp(name, "grounded") == 0) {
                replacement = "(fighter->ground_or_air == GA_Ground)";
            } else if (strcmp(name, "motion.launch_angle") == 0) {
                replacement = "fighter->lstick_angle";
            } else if (strcmp(name, "facing.direction") == 0) {
                replacement = "fighter->facing_dir";
            } else if (strcmp(name, "event.flag") == 0 &&
                       block->item_event) {
                replacement = "flag";
            } else if (strcmp(name, "event.history_source") == 0 && block->cpu_prepare_event) {
                replacement = "((int32_t) history_source)";
            } else if (strcmp(name, "event.previous_motion") == 0 && (block->held_motion_event || block->air_resume_event)) {
                replacement = "previous_motion";
            } else if (strcmp(name, "event.blend") == 0 && block->neutral_enter_event) {
                replacement = "blend";
            } else if (strcmp(name, "event.interrupt_allowed") == 0 && block->landing_action_event) {
                replacement = "script_landing_interrupt_allowed";
            } else if (strcmp(name, "event.victim_motion") == 0 &&
                       block->victim_motion_event) {
                replacement = "victim_motion";
            } else if (strcmp(name, "event.set") == 0 && block->dynamics_set_event) {
                replacement = "set";
            } else if (strcmp(name, "event.set_count") == 0 && block->dynamics_set_event) {
                replacement = "set_count";
            } else if (strcmp(name, "event.motion") == 0 &&
                       (block->motion_event || block->held_motion_event)) {
                replacement = "motion";
            } else if (strcmp(name, "event.swing_type") == 0 &&
                       block->item_swing_event) {
                replacement = "swing_type";
            } else if (strcmp(name, "event.mode") == 0 && block->demo_event) {
                replacement = "demo_mode";
            } else if (strcmp(name, "event.metadata") == 0 && block->action_metadata_event) {
                replacement = "action_metadata";
            } else if (strcmp(name, "event.move_id") == 0 && block->action_metadata_event) {
                replacement = "((uint32_t) action_metadata & 255u)";
            } else if (strcmp(name, "event.swing_variant") == 0 &&
                       block->item_swing_event) {
                replacement = "swing_variant";
            } else if (strcmp(name, "floor.normal.x") == 0) {
                replacement = "fighter->coll_data.floor.normal.x";
            } else if (strcmp(name, "floor.normal.y") == 0) {
                replacement = "fighter->coll_data.floor.normal.y";
            } else if (strcmp(name, "pi_half") == 0) {
                replacement = "(3.14159265358979323846 / 2.0)";
            } else if (strcmp(name, "abs") == 0) {
                allows_parenthesis = true;
                replacement = "fabsf";
            } else if (strcmp(name, "signbit") == 0) {
                allows_parenthesis = true;
                replacement = "script_signbit";
            } else if (strcmp(name, "random") == 0 || strcmp(name, "random.float") == 0) {
                allows_parenthesis = true;
                replacement = "engine->random_float";
            } else if (strcmp(name, "random.int") == 0) {
                allows_parenthesis = true;
                replacement = "engine->random_int";
            } else if (strcmp(name, "random.int_excluding") == 0) {
                allows_parenthesis = true;
                replacement = "engine->random_int_excluding";
            } else if (strcmp(name, "atan2") == 0) {
                allows_parenthesis = true;
                replacement = "engine->angle_atan2";
            } else if (strcmp(name, "sin") == 0) {
                allows_parenthesis = true;
                replacement = "engine->sin_float";
            } else if (strcmp(name, "cos") == 0) {
                allows_parenthesis = true;
                replacement = "engine->cos_float";
            } else if (strcmp(name, "radians") == 0) {
                allows_parenthesis = true;
                replacement = "MTXDegToRad";
            } else if (strcmp(name, "and") == 0) {
                allows_parenthesis = true;
                replacement = "&&";
            } else if (strcmp(name, "or") == 0) {
                allows_parenthesis = true;
                replacement = "||";
            } else if (strcmp(name, "not") == 0) {
                allows_parenthesis = true;
                replacement = "!";
            } else if (strcmp(name, "true") == 0 ||
                       strcmp(name, "false") == 0) {
                replacement = name;
            } else if (strncmp(name, "It_Kind_", 8) == 0 ||
                       strncmp(name, "It_PKind_", 9) == 0) {
                replacement = name;
            } else if (strncmp(name, "state.", 6) == 0) {
                const char* next = source;
                while (isspace((unsigned char)*next)) ++next;
                if (*next == '(') return false;
                const ScriptStateField* field = nativeFighterScript_StateField(block->state,
                                                              name + 6);
                char access[112];
                if (field == NULL || field->array) return false;
                int length = field->type == ScriptState_Bool
                    ? snprintf(access, sizeof(access),
                               "fighter->native_module_state[script_state_base + %u]",
                               field->offset)
                    : field->type == ScriptState_Float
                    ? snprintf(access, sizeof(access),
                               "script_state_get_float(fighter, script_state_base + %u)",
                               field->offset)
                    : snprintf(access, sizeof(access),
                               "script_state_get_int(fighter, script_state_base + %u)",
                               field->offset);
                if (length < 0 || length >= sizeof(access) ||
                    !append_code(output, capacity, &used, access)) return false;
                continue;
            } else if (strncmp(name, "attrs.", 6) == 0 || strncmp(name, "program.attrs.", 14) == 0) {
                const char* next = source;
                while (isspace((unsigned char)*next)) ++next;
                if (*next == '(') return false;
                ScriptScalar scalar = { 0 };
                char attribute[160];
                if (!nativeScriptCode_ParseScalar(name, block->state, &scalar)) return false;
                if (scalar.parameter_index_plus_one != 0) {
                    if (snprintf(attribute, sizeof(attribute),
                        "script_parameters[%u].%s",
                        scalar.parameter_index_plus_one - 1,
                        scalar.parameter_type == NativeFighterParameter_Float
                            ? "f" : "i") >= sizeof(attribute)) return false;
                } else if (scalar.declared_offset_plus_one != 0) {
                    if (snprintf(attribute, sizeof(attribute),
                        "(*(const %s*)((const uint8_t*) fighter->dat_attrs + %u))",
                        scalar.declared_bool ? "uint8_t" : scalar.declared_int ? "int32_t" : "float",
                        scalar.declared_offset_plus_one - 1) >=
                        sizeof(attribute)) return false;
                } else return false;
                replacement = attribute;
                if (!append_code(output, capacity, &used, replacement))
                    return false;
                continue;
            } else if (interrupt_local(block, name)) {
                const char* next = source;
                while (isspace((unsigned char)*next)) ++next;
                if (*next == '(') return false;
                char local[40];
                snprintf(local, sizeof(local), "local_%s", name);
                if (!append_code(output, capacity, &used, local)) return false;
                continue;
            } else {
                replacement = strcmp(name, "event.prediction_frames") == 0 && block->cpu_projectile_hit_event
                    ? "prediction_frames" : strcmp(name, "event.unblockable") == 0 && block->cpu_projectile_hit_event
                    ? "unblockable" : strcmp(name, "event.category") == 0 && block->cpu_projectile_defense_event
                    ? "projectile_category" : strcmp(name, "event.value") == 0 && block->scalar_event
                    ? "event_value" : strcmp(name, "event.angle") == 0 && block->smash_angle_event
                    ? "smash_angle" : strcmp(name, "event.angle") == 0 && block->target_geometry_event
                    ? "target_angle" : strcmp(name, "event.distance") == 0 && block->target_geometry_event
                    ? "target_distance" : nativeScriptPrimitives_Property(name);
                allows_parenthesis = strcmp(name, "min") == 0 || strcmp(name, "max") == 0 ||
                    strcmp(name, "float") == 0 || strcmp(name, "int") == 0 ||
                    strcmp(name, "uint") == 0 || strcmp(name, "atan2_float") == 0 ||
                    strcmp(name, "sqrt") == 0 || strcmp(name, "wrap_signed_angle") == 0;
                if (replacement == NULL) {
                    char primitive[1024];
                    if (!nativeScriptPrimitives_Expression(block, name, &source,
                                                           primitive, sizeof(primitive)) ||
                        !append_code(output, capacity, &used, primitive)) return false;
                    continue;
                }
            }
            // Properties are values, never callable. Expression primitives
            // consume their own argument list before reaching this point.
            const char* next = source;
            while (isspace((unsigned char)*next)) ++next;
            if (*next == '(' && !allows_parenthesis) return false;
            if (!append_code(output, capacity, &used, replacement)) return false;
            continue;
        }
        if (strchr("0123456789.+-*/%()<>=!,&|^~", *source) == NULL)
            return false;
        char symbol[2] = { *source++, '\0' };
        if (!append_code(output, capacity, &used, symbol)) return false;
    }
    return used != 0;
}


static bool translate_code_action(ScriptCodeBlock* block,
                                  const char* source, char* output,
                                  size_t capacity, bool register_local)
{
    const char* dispatch_assignment = strstr(source, " = ");
    if (dispatch_assignment && strstr(dispatch_assignment + 3, "targets.call(")) {
        const char* target = source;
        ScriptExpressionType type;
        if (strncmp(target, "let ", 4) == 0) {
            target += 4;
            type = nativeScriptTypes_Declaration(&target);
        } else {
            char name[96]; size_t size = (size_t)(dispatch_assignment - target);
            if (size >= sizeof(name)) return false;
            memcpy(name, target, size); name[size] = 0;
            type = nativeScriptCode_ExpressionType(block, name);
        }
        if (type != ScriptType_Bool) return false;
    }
    if (!block->model_effect && nativeScriptLifetime_TargetHost(source) != NULL) {
        char expression[1024];
        return nativeScriptCode_Expression(block, source, expression, sizeof(expression)) &&
               snprintf(output, capacity, "%s;", expression) < capacity;
    }
    if (nativeScriptFunctions_Call(block, source, output, capacity)) return true;
    if (block->model_effect) {
        int effect = nativeScriptModelEffect_Action(block, source, output, capacity);
        if (effect) return effect > 0;
    }
    if (nativeScriptModelEffect_Spawn(block, source, output, capacity)) return true;
    if (nativeScriptFunctions_BindCallback(block, source, output, capacity)) return true;
    int spatial = nativeScriptSpatial_FighterAction(block, source, output, capacity);
    if (spatial) return spatial > 0;
    int color = nativeScriptColors_FighterAction(block, source, output, capacity);
    if (color) return color > 0;
    if (nativeScriptTerrain_Action(block, source, output, capacity)) return true;
    if (nativeScriptCpu_Action(block, source, output, capacity)) return true;
    if (nativeScriptPose_Action(block, source, output, capacity)) return true;
    if (nativeScriptPrimitives_Action(block, source, output, capacity)) return true;
    if (strcmp(source, "return") == 0)
        return !block->predicate && snprintf(output, capacity, "return;") < capacity;
    if (strncmp(source, "return ", 7) == 0) {
        char value[1024];
        return block->predicate &&
               (!block->strict_integer_result || nativeScriptCode_ExpressionType(block, source + 7) == ScriptType_Int) &&
               (!block->strict_boolean_result || nativeScriptCode_ExpressionType(block, source + 7) == ScriptType_Bool) &&
               (!(block->held_motion_event || block->air_resume_event) || nativeScriptCode_ExpressionType(block, source + 7) == ScriptType_Int) &&
               (!block->integer_result || nativeScriptTypes_Assignable(ScriptType_Int,
                    nativeScriptCode_ExpressionType(block, source + 7))) &&
               nativeScriptCode_Expression(block, source + 7, value, sizeof(value)) &&
               snprintf(output, capacity, "return (%s);", value) < capacity;
    }
    if (strcmp(source, "physics.animation_velocity()") == 0)
        return snprintf(output, capacity,
            "engine->animation_velocity_y(gobj); "
            "engine->animation_velocity_x(gobj);") < capacity;
    if (strcmp(source, "physics.animation_velocity_plain()") == 0)
        return snprintf(output, capacity,
            "engine->animation_velocity_x(gobj);") < capacity;
    if (strcmp(source, "physics.ground_animation_velocity()") == 0)
        return snprintf(output, capacity,
            "engine->ground_animation_velocity(gobj);") < capacity;
    if (strcmp(source, "physics.align_to_floor()") == 0)
        return snprintf(output, capacity,
            "engine->align_root_to_floor(fighter);") < capacity;
    if (strcmp(source, "bones.root.reset_z()") == 0)
        return snprintf(output, capacity,
            "engine->set_root_rotation_z(fighter, 0, 0);") < capacity;
    static const char ground_decelerate[] = "physics.ground_decelerate(";
    if (strncmp(source, ground_decelerate,
                sizeof(ground_decelerate) - 1) == 0) {
        size_t size = strlen(source);
        size_t prefix = sizeof(ground_decelerate) - 1;
        if (size <= prefix + 1 || source[size - 1] != ')' ||
            size - prefix - 1 >= 192) return false;
        char expression[192], translated[512];
        memcpy(expression, source + prefix, size - prefix - 1);
        expression[size - prefix - 1] = '\0';
        if (!nativeScriptCode_Expression(block, expression, translated,
                                  sizeof(translated))) return false;
        return snprintf(output, capacity,
            "engine->ground_decelerate(fighter, %s);", translated) < capacity;
    }
    if (strcmp(source, "fighter.copy_ext_attributes()") == 0)
        return snprintf(output, capacity,
            "engine->copy_ext_attributes(gobj);") < capacity;
    if (strcmp(source, "fighter.reload_ext_attributes()") == 0)
        return snprintf(output, capacity,
            "engine->reload_ext_attributes(gobj);") < capacity;
    if (strcmp(source, "fighter.enable_multi_jump()") == 0)
        return snprintf(output, capacity,
            "fighter->can_multijump = true; "
            "fighter->x2D0 = fighter->dat_attrs;") < capacity;
    if (strcmp(source, "fighter.enable_walljump()") == 0)
        return snprintf(output, capacity,
            "fighter->can_walljump = true;") < capacity;
    if (strcmp(source, "state.basic_landing()") == 0)
        return snprintf(output, capacity, "engine->enter_landing_basic(gobj);") < capacity;
    if (strcmp(source, "cue.throw.reset()") == 0)
        return snprintf(output, capacity,
            "fighter->throw_flags = 0;") < capacity;
    if (strcmp(source, "effects.clear_all()") == 0)
        return snprintf(output, capacity,
            "engine->clear_effects(gobj);") < capacity;
    if (strcmp(source, "effects.destroy_all()") == 0)
        return snprintf(output, capacity,
            "engine->destroy_effects(gobj);") < capacity;
    if (strcmp(source, "callbacks.align_velocity_to_facing()") == 0)
        return snprintf(output, capacity,
            "fighter->x21F8 = engine->align_velocity_to_facing;") < capacity;
    if (strcmp(source, "effects.install_hitlag_marker()") == 0)
        return snprintf(output, capacity,
            "fighter->accessory4_cb = engine->mark_effect_active;") < capacity;
    static const char hitbox_damage[] = "hitbox.set_damage(";
    if (strncmp(source, hitbox_damage, sizeof(hitbox_damage) - 1) == 0) {
        char args[2][192], value[512];
        const char* after;
        char* end = NULL;
        if (!nativeScriptCode_ParseArguments(source + sizeof(hitbox_damage) - 2,
                                  args, 2, &after) || *after != '\0')
            return false;
        unsigned long index = strtoul(args[0], &end, 0);
        if (end == args[0] || *end != '\0' || index >= 4 ||
            !nativeScriptCode_Expression(block, args[1], value, sizeof(value)))
            return false;
        return snprintf(output, capacity,
            "engine->set_hitbox_damage(&fighter->x914[%lu], (u32) (%s), gobj);",
            index, value) < capacity;
    }
    if (strcmp(source, "physics.force_air_and_use_jumps()") == 0)
        return snprintf(output, capacity,
            "engine->force_air_and_use_jumps(fighter);") < capacity;
    static const char between[] = "effects.spawn_between(";
    if (strncmp(source, between, sizeof(between) - 1) == 0) {
        char args[3][192], values[3][160];
        const char* after;
        if (!nativeScriptCode_ParseArguments(source + sizeof(between) - 2,
                                             args, 3, &after) || *after) return false;
        for (unsigned i = 0; i < 3; ++i)
            if (!nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return false;
        return snprintf(output, capacity,
            "engine->spawn_effect_between_parts(gobj, %s, %s, %s);",
            values[0], values[1], values[2]) < capacity;
    }
    static const char async_effect[] = "effects.spawn_async_on_bone(";
    if (strncmp(source, async_effect, sizeof(async_effect) - 1) == 0) {
        char plain_args[2][192], value[128];
        const char* after;
        if (nativeScriptCode_ParseArguments(source + sizeof(async_effect) - 2,
                                            plain_args, 2, &after) && !*after) {
            const char* bone = nativeScriptCode_BoneSymbol(plain_args[1],
                                                           strlen(plain_args[1]));
            if (bone == NULL || !nativeScriptCode_Expression(block, plain_args[0],
                                                             value, sizeof(value)))
                return false;
            return snprintf(output, capacity,
                "engine->spawn_async_effect(gobj, &fighter->x60C, 1, %s, "
                "fighter->parts[engine->resolve_bone(fighter, %s)].joint);",
                value, bone) < capacity;
        }
        unsigned effect;
        char bone_name[32], angle[160], translated[512];
        int consumed = 0;
        const char* args = source + sizeof(async_effect) - 1;
        if (sscanf(args, "%u, %31[^,], %n", &effect, bone_name,
                   &consumed) != 2 || consumed == 0 || effect > 65535)
            return false;
        const char* angle_start = args + consumed;
        size_t size = strlen(angle_start);
        if (size < 2 || angle_start[size - 1] != ')' ||
            size - 1 >= sizeof(angle)) return false;
        memcpy(angle, angle_start, size - 1);
        angle[size - 1] = '\0';
        const char* bone = nativeScriptCode_BoneSymbol(bone_name, strlen(bone_name));
        if (bone == NULL ||
            !nativeScriptCode_Expression(block, angle, translated,
                                  sizeof(translated))) return false;
        return snprintf(output, capacity,
            "{ float angle = %s; Fighter_Part part = "
            "engine->resolve_bone(fighter, %s); "
            "engine->spawn_async_effect(gobj, &fighter->x60C, 3, %u, "
            "fighter->parts[part].joint, &angle); }",
            translated, bone, effect) < capacity;
    }
    static const char world_wind[] = "effects.wind_at_fighter(";
    if (strncmp(source, world_wind, sizeof(world_wind) - 1) == 0) {
        unsigned kind;
        float width, height, depth;
        int consumed = 0;
        if (sscanf(source, "effects.wind_at_fighter(%u, %f, %f, %f)%n",
                   &kind, &width, &height, &depth, &consumed) != 4 ||
            source[consumed] != '\0' || kind > 255 ||
            !isfinite(width) || !isfinite(height) || !isfinite(depth))
            return false;
        return snprintf(output, capacity,
            "engine->spawn_wind_at_fighter(gobj, %u, %#.9gf, %#.9gf, %#.9gf);",
            kind, width, height, depth) < capacity;
    }
    static const struct { const char* name; const char* host; } item_events[] = {
        { "fighter.item_pickup(", "fighter_item_pickup" },
        { "fighter.item_drop(", "fighter_item_drop" },
    };
    for (unsigned i = 0; i < sizeof(item_events) / sizeof(item_events[0]);
         ++i) {
        size_t prefix = strlen(item_events[i].name);
        if (strncmp(source, item_events[i].name, prefix) != 0) continue;
        unsigned item_part, hand_part;
        int consumed = 0;
        if (!block->item_event ||
            sscanf(source + prefix, "event.flag, %u, %u)%n",
                   &item_part, &hand_part, &consumed) != 2 ||
            source[prefix + consumed] != '\0' ||
            item_part > 1 || hand_part > 1) return false;
        return snprintf(output, capacity,
                        "engine->%s(gobj, flag, %u, %u);",
                        item_events[i].host, item_part, hand_part) < capacity;
    }
    static const struct {
        const char* name;
        const char* host;
        unsigned max;
    } item_modes[] = {
        { "fighter.item_visible(", "fighter_item_visible", 1 },
        { "fighter.item_invisible(", "fighter_item_invisible", 1 },
        { "fighter.knockback_enter(", "fighter_knockback_enter", 255 },
        { "fighter.knockback_exit(", "fighter_knockback_exit", 255 },
    };
    for (unsigned i = 0; i < sizeof(item_modes) / sizeof(item_modes[0]);
         ++i) {
        size_t prefix = strlen(item_modes[i].name);
        if (strncmp(source, item_modes[i].name, prefix) != 0) continue;
        unsigned mode;
        int consumed = 0;
        if (sscanf(source + prefix, "%u)%n", &mode, &consumed) != 1 ||
            source[prefix + consumed] != '\0' ||
            mode > item_modes[i].max) return false;
        return snprintf(output, capacity,
                        "engine->%s(gobj, %u);", item_modes[i].host,
                        mode) < capacity;
    }
    if (strncmp(source, "articles.create_unowned_at_position(", 35) == 0 ||
        strncmp(source, "articles.create_held_at_center(", 30) == 0) {
        char expression[3072];
        if (!nativeScriptCode_Expression(block, source, expression, sizeof(expression))) return false;
        return snprintf(output, capacity, "%s;", expression) < capacity;
    }
    if (strncmp(source, "articles.spawn_projectile(", 26) == 0 ||
        strncmp(source, "articles.spawn_authored_projectile(", 35) == 0 ||
        strncmp(source, "articles.launch_projectile(", 27) == 0 ||
        strncmp(source, "articles.launch_at_position(", 28) == 0 ||
        strncmp(source, "articles.create_at_position(", 28) == 0)
        return nativeScriptProjectile_Emit(block, source, output, capacity);
    if (strncmp(source, "articles.signal_owned(", 22) == 0 ||
        strncmp(source, "articles.signal(", 16) == 0)
        return nativeScriptSignal_Emit(block, source, output, capacity);
    static const struct { const char* source; const char* target; } item_ops[] = {
        { "items.pull(", "engine->spawn_holdable_item(gobj, (uint16_t) (%s));" },
        { "fighter.self_damage(", "engine->self_damage(gobj, %s);" },
        { "articles.remove_owned(", "engine->remove_owned_article(gobj, (s32) (%s));" },
        { "articles.pause_owned(", "engine->pause_owned_article(gobj, (s32) (%s));" },
        { "articles.resume_owned(", "engine->resume_owned_article(gobj, (s32) (%s));" },
    };
    for (unsigned i = 0; i < sizeof(item_ops) / sizeof(item_ops[0]); ++i) {
        size_t prefix = strlen(item_ops[i].source);
        size_t size = strlen(source);
        if (strncmp(source, item_ops[i].source, prefix) != 0) continue;
        if (size <= prefix + 1 || source[size - 1] != ')' ||
            size - prefix - 1 >= 192) return false;
        char input[192], value[512];
        memcpy(input, source + prefix, size - prefix - 1);
        input[size - prefix - 1] = '\0';
        if (!nativeScriptCode_Expression(block, input, value, sizeof(value)))
            return false;
        return snprintf(output, capacity, item_ops[i].target, value) < capacity;
    }
    if (strcmp(source, "collision.cliff_catch()") == 0)
        return snprintf(output, capacity,
            "engine->check_cliff_catch(gobj);") < capacity;
    if (strcmp(source, "state.catch_ledge()") == 0)
        return snprintf(output, capacity,
            "engine->enter_cliff_catch(gobj);") < capacity;
    if (strcmp(source, "controls.try_aerial_jump()") == 0)
        return snprintf(output, capacity,
            "engine->aerial_attack_actions(gobj);") < capacity;
    if (strcmp(source, "state.land()") == 0)
        return snprintf(output, capacity,
            "engine->enter_landing_basic(gobj);") < capacity;
    if (strncmp(source, "state.land_special(", 19) == 0) {
        const size_t size = strlen(source);
        char arguments[192], translated[512];
        if (size <= 20 || source[size - 1] != ')' ||
            size - 20 >= sizeof(arguments)) return false;
        memcpy(arguments, source + 19, size - 20);
        arguments[size - 20] = '\0';
        if (!nativeScriptCode_Expression(block, arguments, translated,
                                  sizeof(translated))) return false;
        return snprintf(output, capacity,
            "engine->enter_landing_fall_special(gobj, %s);",
            translated) < capacity;
    }
    if (strncmp(source, "state.landing_fall_special(", 27) == 0) {
        const size_t size = strlen(source);
        char arguments[192], translated[512];
        if (size <= 28 || source[size - 1] != ')' ||
            size - 28 >= sizeof(arguments)) return false;
        memcpy(arguments, source + 27, size - 28);
        arguments[size - 28] = '\0';
        if (!nativeScriptCode_Expression(block, arguments, translated,
                                  sizeof(translated))) return false;
        return snprintf(output, capacity,
            "engine->enter_landing_fall_special(gobj, false, %s);",
            translated) < capacity;
    }
    if (strncmp(source, "state.taunt(", 12) == 0) {
        unsigned right, left;
        int consumed = 0;
        if (sscanf(source, "state.taunt(%u, %u)%n", &right, &left,
                   &consumed) != 2 || source[consumed] != '\0' ||
            (right >= ftCo_MS_Count && (right < block->motion_state_first ||
             right - block->motion_state_first >= block->motion_state_count)) ||
            (left >= ftCo_MS_Count && (left < block->motion_state_first ||
             left - block->motion_state_first >= block->motion_state_count)))
            return false;
        return snprintf(output, capacity,
            "engine->enter_taunt(gobj, %u, %u);", right, left) < capacity;
    }
    if (strncmp(source, "state.enter_flags(", 18) == 0) {
        unsigned motion;
        int flags;
        int consumed = 0;
        if (sscanf(source, "state.enter_flags(%u, %i)%n", &motion,
                   &flags, &consumed) != 2 ||
            source[consumed] != '\0' || motion > 65535 || flags < 0 ||
            (block->motion_state_count != 0 &&
             (motion < block->motion_state_first ||
              motion - block->motion_state_first >=
                  block->motion_state_count))) return false;
        return snprintf(output, capacity,
            "engine->change_motion_state(gobj, %u, %u, 0, 1, 0, 0);",
            motion, flags) < capacity;
    }
    if (strncmp(source, "state.enter_at(", 15) == 0) {
        char args[3][192], translated[512];
        const char* after;
        char* end = NULL;
        if (!nativeScriptCode_ParseArguments(source + 14, args, 3, &after) ||
            *after != '\0') return false;
        unsigned long motion = strtoul(args[0], &end, 0);
        if (args[0][0] == '-' || end == args[0] || *end != '\0' ||
            motion > 65535 ||
            (block->motion_state_count != 0 &&
             (motion < block->motion_state_first ||
              motion - block->motion_state_first >= block->motion_state_count)))
            return false;
        errno = 0;
        unsigned long flags = strtoul(args[1], &end, 0);
        if (errno != 0 || args[1][0] == '-' || end == args[1] ||
            *end != '\0' || flags > UINT32_MAX ||
            !nativeScriptCode_Expression(block, args[2], translated,
                                  sizeof(translated))) return false;
        return snprintf(output, capacity,
            "engine->change_motion_state(gobj, %lu, %lu, %s, 1, 0, 0);",
            motion, flags, translated) < capacity;
    }
    if (strncmp(source, "state.enter(", 12) == 0 &&
        source[12] >= '0' && source[12] <= '9') {
        unsigned motion;
        char frame[128] = "1";
        char speed[128] = "1";
        int consumed = 0;
        if (sscanf(source, "state.enter(%u, %127[^,], %127[^)])%n",
                   &motion, frame, speed, &consumed) != 3 ||
            source[consumed] != '\0') {
            consumed = 0;
            strcpy(speed, "1");
            if (sscanf(source, "state.enter(%u, %127[^)])%n", &motion,
                       frame, &consumed) != 2 || source[consumed] != '\0') {
                consumed = 0;
                strcpy(frame, "1");
                if (sscanf(source, "state.enter(%u)%n", &motion,
                           &consumed) != 1 || source[consumed] != '\0')
                    return false;
            }
        }
        if (block->motion_state_count != 0 &&
            (motion < block->motion_state_first ||
             motion - block->motion_state_first >= block->motion_state_count))
            return false;
        char translated[512], translated_speed[512];
        if (!nativeScriptCode_Expression(block, frame, translated,
                                  sizeof(translated)) ||
            !nativeScriptCode_Expression(block, speed, translated_speed,
                                  sizeof(translated_speed))) return false;
        // ChangeMotionState already evaluates the requested frame. Advancing
        // again skips it, including first-frame initialization in physics.
        return snprintf(output, capacity,
            "engine->change_motion_state(gobj, %u, 0, %s, %s, 0, 0);",
            motion, translated, translated_speed) < capacity;
    }
    if (strncmp(source, "state.transition(", 17) == 0) {
        unsigned motion;
        char special[32], frame[128], translated[512];
        int consumed = 0;
        if (sscanf(source, "state.transition(%u, %31[^,], %127[^)])%n",
                   &motion, special, frame, &consumed) != 3 ||
            source[consumed] != '\0' ||
            (block->motion_state_count != 0 &&
             (motion < block->motion_state_first ||
              motion - block->motion_state_first >= block->motion_state_count)) ||
            !nativeScriptCode_Expression(block, frame, translated,
                                  sizeof(translated))) return false;
        const char* slot = strcmp(special, "Neutral") == 0 ? "Neutral" :
                           strcmp(special, "Side") == 0 ? "Side" :
                           strcmp(special, "Up") == 0 ? "Up" :
                           strcmp(special, "Down") == 0 ? "Down" : NULL;
        if (slot == NULL) return false;
        return snprintf(output, capacity,
            "engine->change_motion_state(gobj, %u, "
            "generated.special_transition_flags[NativeFighterSpecial_%s], "
            "%s, 1, 0, 0);", motion, slot, translated) < capacity;
    }
    if (strncmp(source, "callbacks.on_hit_state(", 23) == 0) {
        unsigned motion;
        int consumed = 0;
        if (sscanf(source, "callbacks.on_hit_state(%u)%n", &motion,
                   &consumed) != 1 || source[consumed] != '\0' ||
            motion < block->motion_state_first ||
            motion - block->motion_state_first >= block->motion_state_count)
            return false;
        return snprintf(output, capacity,
            "{ uint16_t target = %u; "
            "memcpy(fighter->native_module_state + script_state_base, "
            "&target, sizeof(target)); "
            "fighter->deal_dmg_cb = authored_on_hit; }",
            motion) < capacity;
    }
    if (strncmp(source, "visuals.hide_group(", 19) == 0) {
        unsigned group;
        int consumed = 0;
        if (sscanf(source, "visuals.hide_group(%u)%n", &group,
                   &consumed) != 1 || source[consumed] != '\0' ||
            group > 1) return false;
        return snprintf(output, capacity,
            "engine->set_visibility_group(gobj, %u, 0);", group) < capacity;
    }
    if (strncmp(source, "visuals.apply_part_animation(", 29) == 0) {
        unsigned part, animation;
        float blend_frames;
        int consumed = 0;
        if (sscanf(source, "visuals.apply_part_animation(%u, %u, %f)%n",
                   &part, &animation, &blend_frames, &consumed) != 3 ||
            source[consumed] != '\0' || part >= 5 || animation > 255 ||
            !isfinite(blend_frames) || blend_frames < 0.0f)
            return false;
        return snprintf(output, capacity,
            "engine->apply_part_animation(gobj, %u, %u, %#.9gf);",
            part, animation, blend_frames) < capacity;
    }
    static const char root_effect[] = "effects.spawn_root(";
    size_t root_prefix = sizeof(root_effect) - 1;
    if (strncmp(source, root_effect, root_prefix) == 0) {
        char* tail = NULL;
        errno = 0;
        unsigned long id = strtoul(source + root_prefix, &tail, 0);
        if (errno != 0 || tail == source + root_prefix ||
            id > 0xFFFF || strcmp(tail, ")") != 0) return false;
        return snprintf(output, capacity,
            "engine->spawn_effect(%lu, gobj, gobj->hsd_obj); "
            "fighter->x2219_b0 = 1;", id) < capacity;
    }
    static const char bone_effect[] = "effects.spawn_on_bone(";
    if (strncmp(source, bone_effect, sizeof(bone_effect) - 1) == 0) {
        char args[2][192];
        const char* after;
        char* end = NULL;
        if (!nativeScriptCode_ParseArguments(source + sizeof(bone_effect) - 2,
                                  args, 2, &after) || *after != '\0')
            return false;
        unsigned long effect = strtoul(args[0], &end, 0);
        const char* bone = nativeScriptCode_BoneSymbol(args[1], strlen(args[1]));
        if (end == args[0] || *end != '\0' || effect > 65535 ||
            bone == NULL) return false;
        return snprintf(output, capacity,
            "{ Fighter_Part part = engine->resolve_bone(fighter, %s); "
            "engine->spawn_effect(%lu, gobj, fighter->parts[part].joint); "
            "fighter->x2219_b0 = true; }", bone, effect) < capacity;
    }
    if (strcmp(source, "effects.pause_during_hitlag()") == 0)
        return snprintf(output, capacity,
            "fighter->pre_hitlag_cb = engine->pause_effects; "
            "fighter->post_hitlag_cb = engine->resume_effects;") < capacity;
    if (strcmp(source, "callbacks.reset_root_x_on_damage()") == 0)
        return snprintf(output, capacity,
            "fighter->take_dmg_cb = authored_reset_root_x; "
            "fighter->death2_cb = authored_reset_root_x;") < capacity;
    if (strcmp(source, "callbacks.clear_damage_death()") == 0)
        return snprintf(output, capacity,
            "fighter->take_dmg_cb = 0; fighter->death2_cb = 0;") < capacity;
    if (strcmp(source, "callbacks.track_damage()") == 0)
        return snprintf(output, capacity,
            "fighter->take_dmg_cb = authored_damage;") < capacity;
    if (strcmp(source, "callbacks.track_death2()") == 0)
        return snprintf(output, capacity,
            "fighter->death2_cb = authored_death2;") < capacity;
    if (strcmp(source, "callbacks.track_death3()") == 0)
        return snprintf(output, capacity,
            "fighter->death3_cb = authored_death2;") < capacity;
    if (strcmp(source, "callbacks.track_hitlag()") == 0)
        return snprintf(output, capacity,
            "fighter->pre_hitlag_cb = authored_pre_hitlag; "
            "fighter->post_hitlag_cb = authored_post_hitlag;") < capacity;
    if (strcmp(source, "callbacks.track_deal_damage()") == 0)
        return snprintf(output, capacity,
            "fighter->deal_dmg_cb = authored_deal_damage;") < capacity;
    if (strcmp(source, "callbacks.track_state_change()") == 0)
        return snprintf(output, capacity,
            "fighter->x21EC = authored_state_change;") < capacity;
    if (strcmp(source, "stats.count_as_thrown_item()") == 0)
        return snprintf(output, capacity,
            "fighter->x2070.count_thrown_items = true;") < capacity;
    if (strncmp(source, "capture.arm(", 12) == 0) {
        unsigned mask;
        int consumed = 0;
        if (sscanf(source, "capture.arm(%u)%n", &mask, &consumed) != 1 ||
            source[consumed] != '\0' || mask > 32767) return false;
        return snprintf(output, capacity,
            "engine->arm_grab(fighter, %u, authored_grab_catch, authored_item_catch, authored_grabbed);",
            mask) < capacity;
    }
    if (strncmp(source, "capture.begin(", 14) == 0 &&
        block->grabbed_event) {
        char args[5][192], values[5][128]; const char* after;
        if (!nativeScriptCode_ParseArguments(source + 13, args, 5, &after) || *after) return false;
        for (unsigned i = 0; i < 5; ++i)
            if (!nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return false;
        return snprintf(output, capacity,
            "engine->begin_capture(victim_gobj, gobj, %s, %s, %s, %s, %s);",
            values[0], values[1], values[2], values[3], values[4]) < capacity;
    }
    if (strncmp(source, "capture.mask(", 13) == 0) {
        unsigned mask;
        int consumed = 0;
        if (sscanf(source, "capture.mask(%u)%n", &mask, &consumed) != 1 ||
            source[consumed] != '\0' || mask > 32767) return false;
        return snprintf(output, capacity,
            "engine->set_capture_mask(fighter, %u);", mask) < capacity;
    }
    if (strcmp(source, "capture.clear_movement()") == 0)
        return snprintf(output, capacity,
            "engine->clear_capture_movement(gobj);") < capacity;
    if (strcmp(source, "capture.attach_to_victim()") == 0 &&
        block->grab_catch_event)
        return snprintf(output, capacity,
            "engine->capture_victim_to_air((Fighter*) fighter->victim_gobj->user_data, fighter);") < capacity;
    if (strcmp(source, "capture.follow_victim()") == 0)
        return snprintf(output, capacity,
            "fighter->accessory4_cb = engine->follow_capture_victim;") < capacity;
    if (strcmp(source, "capture.release_throw()") == 0)
        return snprintf(output, capacity,
            "engine->release_capture_throw(gobj);") < capacity;
    static const char capture_ground_link[] = "capture.ground_link = ";
    if (strncmp(source, capture_ground_link, sizeof(capture_ground_link)-1) == 0) {
        char value[192];
        if (!nativeScriptCode_Expression(block, source+sizeof(capture_ground_link)-1,
                                          value, sizeof(value))) return false;
        return snprintf(output, capacity, "fighter->x221B_b7 = (%s) != 0;", value) < capacity;
    }
    if (strcmp(source, "callbacks.track_hurtbox_detect()") == 0)
        return snprintf(output, capacity,
            "fighter->hurtbox_detect_cb = authored_hurtbox_detect;") < capacity;
    if (strcmp(source, "reflector.disable()") == 0)
        return snprintf(output, capacity,
            "fighter->reflecting = false;") < capacity;
    if (strcmp(source, "reflector.retain()") == 0)
        return snprintf(output, capacity,
            "fighter->reflecting = true;") < capacity;
    if (strcmp(source, "reflector.update()") == 0)
        return snprintf(output, capacity,
            "engine->update_reflector(gobj);") < capacity;
    if (strcmp(source, "animation.start()") == 0)
        return snprintf(output, capacity, "engine->start_animation(gobj);") < capacity;
    static const char* accessory_slots[] = { "Neutral", "Side", "Up", "Down" };
    for (unsigned i = 0; i < 4; ++i) {
        char call[64];
        snprintf(call, sizeof(call), "callbacks.track_accessory(%s)", accessory_slots[i]);
        if (strcmp(source, call) == 0) {
            if (block->special == ScriptSpecial_Unset &&
                !block->special_program) return false;
            return snprintf(output, capacity,
                "fighter->accessory4_cb = transition_cue[NativeFighterSpecial_%s];",
                accessory_slots[i]) < capacity;
        }
    }
    if (strcmp(source, "articles.stop_cue()") == 0)
        return snprintf(output, capacity,
            "fighter->accessory4_cb = 0;") < capacity;
    if (strcmp(source, "pass") == 0)
        return snprintf(output, capacity, ";") < capacity;
    static const struct {
        const char* syntax;
        const char* engine_step;
    } cross_state[] = {
        { "state.switch_to_air(", "ground_to_air" },
        { "state.switch_to_ground(", "air_to_ground" },
    };
    for (unsigned i = 0; i < sizeof(cross_state) / sizeof(cross_state[0]);
         ++i) {
        size_t prefix = strlen(cross_state[i].syntax);
        if (strncmp(source, cross_state[i].syntax, prefix) != 0) continue;
        unsigned motion;
        int flags;
        int consumed = 0;
        char arguments[96];
        size_t length = strlen(source);
        if (length <= prefix + 2 || source[length - 1] != ')' ||
            length - prefix >= sizeof(arguments)) return false;
        memcpy(arguments, source + prefix, length - prefix - 1);
        arguments[length - prefix - 1] = '\0';
        if (sscanf(arguments, "%u, %i%n", &motion, &flags,
                   &consumed) != 2 || arguments[consumed] != '\0' ||
            motion > 65535 || flags < 0 ||
            (block->motion_state_count != 0 &&
             (motion < block->motion_state_first ||
              motion - block->motion_state_first >=
                  block->motion_state_count))) return false;
        return snprintf(output, capacity,
            "engine->%s(fighter); "
            "engine->change_motion_state(gobj, %u, %u, "
            "fighter->cur_anim_frame, 1, 0, 0);",
            cross_state[i].engine_step, motion, flags) < capacity;
    }
    if (strcmp(source, "state.to_air()") == 0 ||
        strcmp(source, "state.to_ground()") == 0) {
        if (block->special == ScriptSpecial_Unset) return false;
        const char* special = nativeScriptCode_SpecialEnum(block->special);
        bool air = strcmp(source, "state.to_air()") == 0;
        return snprintf(output, capacity,
            "engine->%s(fighter); "
            "engine->change_motion_state(gobj, generated.special_%s_state[NativeFighterSpecial_%s], "
            "generated.special_transition_flags[NativeFighterSpecial_%s], fighter->cur_anim_frame, 1, 0, 0); "
            "fighter->accessory4_cb = transition_cue[NativeFighterSpecial_%s]; "
            "fighter->x21EC = transition_exit[NativeFighterSpecial_%s];",
            air ? "ground_to_air" : "air_to_ground",
            air ? "air" : "ground", special, special, special, special) < capacity;
    }
    if (block->collision) {
        const char* special = nativeScriptCode_SpecialEnum(block->special);
        if (strcmp(source, "collision.air_standard()") == 0)
            return snprintf(output, capacity,
                            "engine->air_standard_collision(gobj);") < capacity;
        if (strcmp(source, "collision.air_catch_hit()") == 0)
            return snprintf(output, capacity,
                            "engine->air_catch_hit_collision(gobj);") < capacity;
        if (strcmp(source, "collision.ground_standard()") == 0)
            return snprintf(output, capacity,
                            "engine->ground_standard_collision(gobj);") < capacity;
        if (strcmp(source, "collision.ground_with_nudge()") == 0)
            return snprintf(output, capacity,
                            "engine->ground_nudge_collision(gobj);") < capacity;
        static const char landing[] = "collision.air_special_landing(";
        size_t prefix = sizeof(landing) - 1;
        size_t size = strlen(source);
        if (strncmp(source, landing, prefix) == 0) {
            if (block->landing || size <= prefix + 7 || source[size - 1] != ')')
                return false;
            char args[128];
            if (size - prefix >= sizeof(args)) return false;
            memcpy(args, source + prefix, size - prefix - 1);
            args[size - prefix - 1] = '\0';
            char* comma = strstr(args, ", ");
            if (comma == NULL) return false;
            *comma = '\0';
            if (strcmp(args, "false") != 0 && strcmp(args, "true") != 0)
                return false;
            if (!nativeScriptCode_ParseScalar(comma + 2, block->state,
                              &block->landing_lag)) return false;
            block->landing = true;
            block->landing_interrupt = args[0] == 't';
            return snprintf(output, capacity,
                "engine->air_special_landing_collision(gobj, engine->fall_special_ledge_check, authored_%s_landing_%s);",
                block->air_entry ? "air" : "ground",
                nativeScriptCode_SpecialSuffix(block->special)) < capacity;
        }
    }
    if (block->entry) {
        char state_name[64], air_state_name[64];
        snprintf(state_name, sizeof(state_name), "state.enter(%sSpecial)",
                 nativeScriptCode_SpecialEnum(block->special));
        snprintf(air_state_name, sizeof(air_state_name),
                 "state.enter(Air%sSpecial)", nativeScriptCode_SpecialEnum(block->special));
        bool explicit_air = !block->air_entry &&
            strcmp(source, air_state_name) == 0;
        if (strcmp(source, state_name) == 0 || explicit_air) {
            int result = snprintf(output, capacity,
                "engine->change_motion_state(gobj, generated.special_%s_state[NativeFighterSpecial_%s], 0, 0, 1, 0, 0); engine->start_animation(gobj);",
                block->air_entry || explicit_air ? "air" : "ground",
                nativeScriptCode_SpecialEnum(block->special));
            return result >= 0 && result < capacity;
        }
        if (strcmp(source, "stock.enter()") == 0) {
            int result = snprintf(output, capacity,
                "inherited_%s_%s(gobj);",
                block->air_entry ? "air" : "ground",
                nativeScriptCode_SpecialSuffix(block->special));
            return result >= 0 && result < capacity;
        }
    }
    static const struct {
        const char* script;
        const char* target;
        const char* operation;
        bool facing;
    } velocity_ops[] = {
        { "motion.add_facing_speed(", "", "+=", true },
        { "motion.set_facing_speed(", "", "=", true },
        { "motion.add_vertical_speed(", "fighter->self_vel.y", "+=", false },
        { "motion.set_vertical_speed(", "fighter->self_vel.y", "=", false },
        { "motion.scale_horizontal_speed(", "fighter->self_vel.x", "*=", false },
        { "motion.divide_horizontal_speed(", "fighter->self_vel.x", "/=", false },
    };
    for (unsigned i = 0; i < sizeof(velocity_ops) / sizeof(velocity_ops[0]); ++i) {
        size_t prefix_size = strlen(velocity_ops[i].script);
        if (strncmp(source, velocity_ops[i].script, prefix_size) != 0)
            continue;
        if (velocity_ops[i].facing && !block->entry) return false;
        size_t size = strlen(source);
        if (size <= prefix_size + 1 || source[size - 1] != ')')
            return false;
        size_t length = size - prefix_size - 1;
        if (length >= 192) return false;
        char argument[192], generated[1024];
        memcpy(argument, source + prefix_size, length);
        argument[length] = '\0';
        if (!nativeScriptCode_Expression(block, argument, generated,
                                  sizeof(generated))) return false;
        const char* target = velocity_ops[i].facing
            ? (block->air_entry ? "fighter->self_vel.x" : "fighter->gr_vel")
            : velocity_ops[i].target;
        int result = snprintf(output, capacity, "%s %s %s%s%s;",
                              target, velocity_ops[i].operation,
                              velocity_ops[i].facing ? "fighter->facing_dir * (" : "",
                              generated,
                              velocity_ops[i].facing ? ")" : "");
        return result >= 0 && result < capacity;
    }
    static const struct { const char* script; const char* generated; } calls[] = {
        { "controls.ground_actions()", "engine->ground_actions(gobj);" },
        { "controls.taunt_actions()", "engine->taunt_actions(gobj);" },
        { "controls.air_actions()", "engine->air_actions(gobj);" },
        { "facing.from_input()", "engine->update_facing(fighter);" },
        { "physics.ground_friction()", "engine->ground_friction(gobj);" },
        { "physics.air_drift()", "engine->air_drift(gobj);" },
        { "physics.ground_motion()", "engine->ground_motion(gobj);" },
        { "physics.air_motion()", "engine->air_motion(gobj);" },
        { "physics.air_gravity_and_friction()",
          "engine->air_gravity_and_friction(gobj);" },
        { "physics.clamp_air_drift()",
          "engine->clamp_air_drift(fighter);" },
        { "physics.switch_to_ground()",
          "engine->air_to_ground(fighter);" },
        { "physics.quick_air_decel()", "engine->quick_air_decel(fighter);" },
        { "physics.fall_basic()", "engine->fall_basic(fighter);" },
        { "physics.ground_to_self()", "engine->ground_to_self_movement(gobj);" },
        { "state.wait()", "engine->finish_ground_action(gobj);" },
        { "state.fall()", "engine->enter_fall(gobj);" },
    };
    for (unsigned i = 0; i < sizeof(calls) / sizeof(calls[0]); ++i)
        if (strcmp(source, calls[i].script) == 0) {
            snprintf(output, capacity, "%s", calls[i].generated);
            return true;
        }
    static const char air_accel_common[] = "physics.air_accel_common(";
    if (strncmp(source, air_accel_common,
                sizeof(air_accel_common) - 1) == 0) {
        size_t prefix = sizeof(air_accel_common) - 1;
        size_t size = strlen(source);
        if (size <= prefix + 1 || source[size - 1] != ')' ||
            size - prefix - 1 >= 192) return false;
        char arguments[192], generated[1024];
        memcpy(arguments, source + prefix, size - prefix - 1);
        arguments[size - prefix - 1] = '\0';
        const char* comma = strchr(arguments, ',');
        if (comma == NULL || strchr(comma + 1, ',') != NULL ||
            !nativeScriptCode_Expression(block, arguments, generated,
                                  sizeof(generated))) return false;
        return snprintf(output, capacity,
            "engine->air_accel(fighter, engine->air_drift_threshold(), %s);",
            generated) < capacity;
    }
    static const struct {
        const char* script;
        const char* host;
        unsigned arguments;
        bool zero_threshold;
    } movement[] = {
        { "physics.ascend(", "ascend", 2, false },
        { "physics.ground_accel(", "ground_accel", 2, true },
        { "physics.air_accel(", "air_accel", 2, true },
        { "physics.clamp_horizontal_speed(", "clamp_self_vel_x", 1, false },
        { "physics.clamp_ground_speed(", "clamp_ground_vel", 1, false },
        { "physics.clamp_fall_speed(", "clamp_fall_speed", 1, false },
        { "physics.air_decel(", "air_decel", 1, false },
    };
    for (unsigned i = 0; i < sizeof(movement) / sizeof(movement[0]); ++i) {
        size_t prefix_size = strlen(movement[i].script);
        if (strncmp(source, movement[i].script, prefix_size) != 0)
            continue;
        size_t size = strlen(source);
        if (size <= prefix_size + 1 || source[size - 1] != ')') return false;
        size_t length = size - prefix_size - 1;
        if (length >= 192) return false;
        char arguments[192], generated[1024];
        memcpy(arguments, source + prefix_size, length);
        arguments[length] = '\0';
        unsigned argument_count = 1;
        for (size_t j = 0; j < length; ++j)
            argument_count += arguments[j] == ',';
        if (argument_count != movement[i].arguments ||
            !nativeScriptCode_Expression(block, arguments, generated,
                                  sizeof(generated))) return false;
        int result = snprintf(output, capacity, "engine->%s(fighter, %s%s);",
                              movement[i].host,
                              movement[i].zero_threshold ? "0.0f, " : "",
                              generated);
        return result >= 0 && result < capacity;
    }
    static const char fall[] = "physics.fall(";
    static const char scale[] = "motion.scale_velocity(";
    static const char planar_scale[] = "motion.scale_planar_velocity(";
    const char* prefix = NULL;
    bool falling = false;
    bool planar = false;
    if (strncmp(source, fall, sizeof(fall) - 1) == 0) {
        prefix = fall;
        falling = true;
    } else if (strncmp(source, scale, sizeof(scale) - 1) == 0) {
        prefix = scale;
    } else if (strncmp(source, planar_scale,
                       sizeof(planar_scale) - 1) == 0) {
        prefix = planar_scale;
        planar = true;
    }
    if (prefix != NULL) {
        size_t prefix_size = strlen(prefix);
        size_t size = strlen(source);
        if (size <= prefix_size + 1 || source[size - 1] != ')')
            return false;
        char expression[192], generated[1024];
        size_t length = size - prefix_size - 1;
        if (length >= sizeof(expression)) return false;
        memcpy(expression, source + prefix_size, length);
        expression[length] = '\0';
        if (falling) {
            const char* comma = strchr(expression, ',');
            if (comma != NULL && strchr(comma + 1, ',') != NULL)
                return false;
        }
        if (!nativeScriptCode_Expression(block, expression, generated,
                                  sizeof(generated))) return false;
        int result = falling
            ? strchr(expression, ',') != NULL
                ? snprintf(output, capacity,
                           "engine->fall(fighter, %s);", generated)
                : snprintf(output, capacity,
                           "engine->fall(fighter, %s, "
                           "fighter->co_attrs.terminal_velocity);", generated)
            : planar ? snprintf(output, capacity,
                       "do { float scale = %s; "
                       "fighter->self_vel.x *= scale; "
                       "fighter->self_vel.y *= scale; } while (0);",
                       generated)
            : snprintf(output, capacity,
                       "do { float scale = %s; "
                       "fighter->self_vel.x *= scale; "
                       "fighter->self_vel.y *= scale; "
                       "fighter->self_vel.z *= scale; } while (0);",
                       generated);
        return result >= 0 && result < capacity;
    }
    static const char freefall[] = "state.freefall(";
    if (strncmp(source, freefall, sizeof(freefall) - 1) == 0) {
        char arguments[5][192], values[5][1024]; const char* after;
        if (!nativeScriptCode_ParseArguments(source + sizeof(freefall) - 2,
                arguments, 5, &after) || *after) return false;
        for (unsigned i = 0; i < 5; ++i) {
            ScriptExpressionType type = nativeScriptCode_ExpressionType(block, arguments[i]);
            if ((i < 2 && type != ScriptType_Int) || (i == 2 && type != ScriptType_Bool) ||
                (i > 2 && !nativeScriptTypes_Assignable(ScriptType_Float, type)) ||
                !nativeScriptCode_Expression(block, arguments[i], values[i], sizeof(values[i])))
                return false;
        }
        int result = snprintf(output, capacity,
            "engine->enter_freefall(gobj, %s, %s, %s, %s, %s);",
            values[0], values[1], values[2], values[3], values[4]);
        return result >= 0 && (size_t)result < capacity;
    }
    static const char rotate_y[] = "bones.root.rotate_y(";
    static const char rotate_x[] = "bones.root.rotate_x(";
    static const char rotate_z[] = "bones.root.rotate_z(";
    size_t size = strlen(source);
    bool y_rotation = strncmp(source, rotate_y, sizeof(rotate_y) - 1) == 0;
    bool x_rotation = strncmp(source, rotate_x, sizeof(rotate_x) - 1) == 0;
    bool z_rotation = strncmp(source, rotate_z, sizeof(rotate_z) - 1) == 0;
    if ((x_rotation || y_rotation || z_rotation) &&
        size > sizeof(rotate_y) && source[size - 1] == ')') {
        char expression[192], generated[1024];
        size_t length = size - sizeof(rotate_y);
        if (length >= sizeof(expression)) return false;
        memcpy(expression, source + sizeof(rotate_y) - 1, length);
        expression[length] = '\0';
        unsigned joint = 0;
        char* value = expression;
        if (isdigit((unsigned char) expression[0])) {
            char* end = NULL;
            unsigned long parsed = strtoul(expression, &end, 10);
            if (*end == ',' && end[1] == ' ') {
                if (parsed > 255) return false;
                joint = (unsigned) parsed;
                value = end + 2;
            }
        }
        if (!nativeScriptCode_Expression(block, value, generated,
                                  sizeof(generated))) return false;
        return snprintf(output, capacity,
                        "engine->set_root_rotation_%c(fighter, %u, %s);",
                        x_rotation ? 'x' : y_rotation ? 'y' : 'z', joint, generated) < capacity;
    }
    bool declaration = strncmp(source, "let ", 4) == 0;
    if (strncmp(source, "trail.", 6) == 0)
        return nativeScriptTrail_Assignment(block, source, output, capacity);
    const char* assignment = strstr(source, " = ");
    if (assignment == NULL) return false;
    const char* name_start = source + (declaration ? 4 : 0);
    if (!declaration && strncmp(name_start, "articles.ref(", 13) == 0) return false;
    ScriptExpressionType local_type = declaration
        ? nativeScriptTypes_Declaration(&name_start) : ScriptType_Invalid;
    size_t name_size = (size_t) (assignment - name_start);
    char name[64];
    if (!declaration && strncmp(name_start, "state.", 6) == 0) {
        NativeScriptStateReference reference;
        int found = nativeScriptState_Reference(block->state, name_start,
                                                "state.", &reference);
        if (found < 0 || reference.after != assignment) return false;
        if (reference.field->array) {
            if (strstr(assignment + 3, "targets.call(") && reference.field->type != ScriptState_Bool) return false;
            char index[1024], value[1024];
            return nativeScriptCode_Expression(block, reference.index, index, sizeof(index)) &&
                nativeScriptCode_Expression(block, assignment + 3, value, sizeof(value)) &&
                nativeScriptState_ArrayWrite(reference.field,
                    "fighter->native_module_state + script_state_base", index, value,
                    output, capacity);
        }
    }
    bool launch_angle = !declaration &&
        name_size == sizeof("motion.launch_angle") - 1 &&
        memcmp(name_start, "motion.launch_angle", name_size) == 0;
    bool effect_active = !declaration &&
        name_size == sizeof("effects.active") - 1 &&
        memcmp(name_start, "effects.active", name_size) == 0;
    bool velocity_x = !declaration && name_size == sizeof("velocity.x") - 1 &&
        memcmp(name_start, "velocity.x", name_size) == 0;
    bool velocity_y = !declaration && name_size == sizeof("velocity.y") - 1 &&
        memcmp(name_start, "velocity.y", name_size) == 0;
    bool velocity_z = !declaration && name_size == sizeof("velocity.z") - 1 &&
        memcmp(name_start, "velocity.z", name_size) == 0;
    bool acceleration_x = !declaration &&
        name_size == sizeof("acceleration.x") - 1 &&
        memcmp(name_start, "acceleration.x", name_size) == 0;
    bool acceleration_y = !declaration &&
        name_size == sizeof("acceleration.y") - 1 &&
        memcmp(name_start, "acceleration.y", name_size) == 0;
    bool acceleration_z = !declaration &&
        name_size == sizeof("acceleration.z") - 1 &&
        memcmp(name_start, "acceleration.z", name_size) == 0;
    bool jumps_used = !declaration &&
        name_size == sizeof("jumps.used") - 1 &&
        memcmp(name_start, "jumps.used", name_size) == 0;
    bool ground_acceleration = !declaration &&
        name_size == sizeof("ground.acceleration") - 1 &&
        memcmp(name_start, "ground.acceleration", name_size) == 0;
    bool ground_velocity = !declaration &&
        name_size == sizeof("ground.velocity") - 1 &&
        memcmp(name_start, "ground.velocity", name_size) == 0;
    bool facing = !declaration &&
        name_size == sizeof("facing.direction") - 1 &&
        memcmp(name_start, "facing.direction", name_size) == 0;
    bool horizontal_smash_age = !declaration &&
        name_size == sizeof("input.horizontal_smash_age") - 1 &&
        memcmp(name_start, "input.horizontal_smash_age", name_size) == 0;
    int command_index = !declaration && name_size == 10 &&
        memcmp(name_start, "command(", 8) == 0 &&
        name_start[8] >= '0' && name_start[8] <= '3' &&
        name_start[9] == ')' ? name_start[8] - '0' : -1;
    bool persistent = !declaration && name_size > 6 &&
        memcmp(name_start, "state.", 6) == 0;
    if (name_size >= sizeof(name) ||
        (!launch_angle &&
         !effect_active &&
         !velocity_x && !velocity_y && !velocity_z &&
         !acceleration_x && !acceleration_y && !acceleration_z && !jumps_used &&
         !ground_velocity && !ground_acceleration && !facing && !horizontal_smash_age &&
         command_index < 0 && !persistent &&
         !nativeScriptCode_Identifier(name_start, name_size,
                     declaration ? sizeof(block->locals[0]) :
                                   sizeof(name)))) return false;
    memcpy(name, name_start, name_size);
    name[name_size] = '\0';
    const ScriptStateField* field = persistent
        ? nativeFighterScript_StateField(block->state, name + 6) : NULL;
    if (persistent && (field == NULL || field->array)) return false;
    if (declaration) {
        if (register_local &&
            (block->local_count == SCRIPT_CODE_LOCALS ||
             interrupt_local(block, name))) return false;
    } else if (!launch_angle &&
               !effect_active &&
               !velocity_x && !velocity_y && !velocity_z &&
               !acceleration_x && !acceleration_y && !acceleration_z && !jumps_used &&
               !ground_velocity && !ground_acceleration && !facing && !horizontal_smash_age &&
               command_index < 0 && !persistent &&
               !interrupt_local(block, name)) {
        return false;
    }
    char generated[1024];
    if (!declaration)
        for (unsigned i = 0; i < block->local_count; ++i)
            if (strcmp(block->locals[i], name) == 0) local_type = block->local_types[i];
    if (local_type != ScriptType_Invalid &&
        !nativeScriptTypes_Assignable(local_type,
            nativeScriptCode_ExpressionType(block, assignment + 3))) return false;
    if (strstr(assignment + 3, "targets.call(") &&
        !(local_type == ScriptType_Bool || (persistent && field->type == ScriptState_Bool) || effect_active)) return false;
    if (!nativeScriptCode_Expression(block, assignment + 3, generated,
                              sizeof(generated))) return false;
    int length = declaration
        ? snprintf(output, capacity, "%s local_%s = %s;",
                   nativeScriptTypes_Name(local_type), name, generated)
        : launch_angle
            ? snprintf(output, capacity, "fighter->lstick_angle = %s;",
                       generated)
        : effect_active
            ? snprintf(output, capacity, "fighter->x2219_b0 = (%s) != 0;",
                       generated)
        : velocity_x
            ? snprintf(output, capacity, "fighter->self_vel.x = %s;",
                       generated)
        : velocity_y
            ? snprintf(output, capacity, "fighter->self_vel.y = %s;",
                       generated)
        : velocity_z
            ? snprintf(output, capacity, "fighter->self_vel.z = %s;",
                       generated)
        : acceleration_x
            ? snprintf(output, capacity, "fighter->x74_self_accel.x = %s;",
                       generated)
        : acceleration_y
            ? snprintf(output, capacity, "fighter->x74_self_accel.y = %s;",
                       generated)
        : jumps_used
            ? snprintf(output, capacity, "fighter->x1968_jumpsUsed = %s;",
                       generated)
        : ground_acceleration
            ? snprintf(output, capacity, "fighter->xE4_ground_accel_1 = %s;", generated)
        : acceleration_z
            ? snprintf(output, capacity, "fighter->x74_self_accel.z = %s;", generated)
        : ground_velocity
            ? snprintf(output, capacity, "fighter->gr_vel = %s;",
                       generated)
        : facing
            ? snprintf(output, capacity, "fighter->facing_dir = %s;",
                       generated)
        : horizontal_smash_age
            ? snprintf(output, capacity, "fighter->x673 = %s;", generated)
        : command_index >= 0
            ? snprintf(output, capacity, "fighter->cmd_vars[%d] = %s;",
                       command_index, generated)
            : persistent && field->type == ScriptState_Bool
                ? snprintf(output, capacity,
                           "fighter->native_module_state[script_state_base + %u] = (uint8_t) ((%s) != 0);",
                           field->offset, generated)
            : persistent && field->type == ScriptState_Float
                ? snprintf(output, capacity,
                           "script_state_set_float(fighter, script_state_base + %u, %s);",
                           field->offset, generated)
            : persistent
                ? snprintf(output, capacity,
                           "script_state_set_int(fighter, script_state_base + %u, %s);",
                           field->offset, generated)
            : snprintf(output, capacity, "local_%s = %s;", name, generated);
    if (length < 0 || length >= capacity) return false;
    if (declaration && register_local) {
        memcpy(block->locals[block->local_count], name, name_size + 1);
        block->local_depth[block->local_count] = block->last_depth;
        block->local_types[block->local_count] = local_type;
        ++block->local_count;
    }
    return true;
}

static bool translate_code_line(ScriptCodeBlock* block,
                                      const char* source, char* output,
                                      size_t capacity, bool register_local,
                                      bool* opens)
{
    *opens = false;
    bool if_branch = strncmp(source, "if ", 3) == 0;
    bool elif_branch = strncmp(source, "elif ", 5) == 0;
    bool else_branch = strncmp(source, "else:", 5) == 0;
    bool while_branch = strncmp(source, "while ", 6) == 0;
    if (!if_branch && !elif_branch && !else_branch && !while_branch)
        return translate_code_action(block, source, output, capacity,
                                     register_local);
    const char* condition = source + (while_branch ? 6 : elif_branch ? 5 : 3);
    const char* colon = else_branch ? source + 4 : strchr(condition, ':');
    if (colon == NULL || (!else_branch && colon == condition)) return false;
    char expression[192], generated[1024];
    if (!else_branch) {
        size_t size = (size_t) (colon - condition);
        if (size >= sizeof(expression)) return false;
        memcpy(expression, condition, size);
        expression[size] = '\0';
        if (!nativeScriptCode_Expression(block, expression, generated,
                                  sizeof(generated))) return false;
        if (strcmp(expression, "grounded") == 0)
            strcpy(generated, "fighter->ground_or_air == GA_Ground");
        else if (strcmp(expression, "airborne") == 0)
            strcpy(generated, "fighter->ground_or_air == GA_Air");
    }
    const char* branch = while_branch ? "while" : else_branch ? "else" :
        elif_branch ? "else if" : "if";
    if (colon[1] == '\0') {
        *opens = true;
        return else_branch
            ? snprintf(output, capacity, "else {") < capacity
            : snprintf(output, capacity, "%s (%s) {", branch, generated) < capacity;
    }
    if (colon[1] != ' ') return false;
    if (strncmp(colon + 2, "let ", 4) == 0) return false;
    char action[1024];
    if (!translate_code_action(block, colon + 2, action,
                               sizeof(action), register_local)) return false;
    return else_branch
        ? snprintf(output, capacity, "else { %s }", action) < capacity
        : snprintf(output, capacity, "%s (%s) { %s }",
                   branch, generated, action) < capacity;
}

void nativeScriptCode_EmitScalar(const ScriptScalar* scalar, FILE* output)
{
    if (scalar->parameter_index_plus_one != 0)
        fprintf(output, "script_parameters[%u].%s",
                scalar->parameter_index_plus_one - 1,
                scalar->parameter_type == NativeFighterParameter_Float
                    ? "f" : "i");
    else if (scalar->declared_offset_plus_one != 0)
        fprintf(output,
            "(*(const %s*)((const uint8_t*) fighter->dat_attrs + %u))",
            scalar->declared_bool ? "uint8_t" : scalar->declared_int ? "int32_t" : "float",
            scalar->declared_offset_plus_one - 1);
    else
        fprintf(output, "%.9ef", scalar->number);
}

void nativeScriptCode_EmitLines(const ScriptCodeBlock* block, FILE* output);

void nativeScriptCode_EmitCollision(const ScriptMove* move, bool air, FILE* output)
{
    const ScriptCodeBlock* block = air
        ? &move->air_collision : &move->ground_collision;
    const char* phase = air ? "air" : "ground";
    const char* suffix = nativeScriptCode_SpecialSuffix(move->special);
    if (block->landing) {
        fprintf(output,
                "static void authored_%s_landing_%s(HSD_GObj* gobj) {\n",
                phase, suffix);
        fputs("    Fighter* fighter = gobj->user_data;\n", output);
        nativeScriptLifetime_EmitEntry(output, ScriptCallback_Fighter);
        fprintf(output, "    engine->enter_landing_fall_special(gobj, %s, ",
                block->landing_interrupt ? "true" : "false");
        nativeScriptCode_EmitScalar(&block->landing_lag, output);
        fputs(");\n    if (!engine->callback_source_alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;\n}\n", output);
    }
    fprintf(output,
            "static void authored_%s_collision_%s(HSD_GObj* gobj) {\n"
            "    Fighter* fighter = gobj->user_data;\n",
            phase, suffix);
    nativeScriptCode_EmitLines(block, output);
    fputs("}\n", output);
}

bool nativeScriptCode_AddLine(ScriptCodeBlock* block, const char* line)
{
    unsigned spaces = 0;
    while (line[spaces] == ' ') ++spaces;
    if (spaces < 8 || (spaces - 8) % 4 != 0 || spaces > 20 ||
        block->count == SCRIPT_CODE_LINES) return false;
    unsigned depth = (spaces - 8) / 4;
    if (block->count == 0) {
        if (depth != 0) return false;
    } else if ((block->last_opens && depth != block->last_depth + 1) ||
               (!block->last_opens && depth > block->last_depth)) {
        return false;
    }
    const char* source = line + spaces;
    if (!nativeScriptLifetime_TargetSurface(source)) return false;
    size_t length = strlen(source);
    if (length == 0 || length >= sizeof(block->lines[0].text)) return false;
    if (strncmp(source, "elif ", 5) == 0 ||
        strncmp(source, "else:", 5) == 0) {
        const ScriptCodeLine* previous = NULL;
        for (unsigned i = block->count; i != 0; --i)
            if (block->lines[i - 1].depth == depth) {
                previous = &block->lines[i - 1];
                break;
            }
        if (previous == NULL ||
            (strncmp(previous->text, "if ", 3) != 0 &&
             strncmp(previous->text, "elif ", 5) != 0)) return false;
    }
    while (block->local_count != 0 &&
           block->local_depth[block->local_count - 1] > depth)
        --block->local_count;
    block->last_depth = (uint8_t) depth;
    char generated[sizeof(block->lines[0].generated)];
    bool opens;
    if (!translate_code_line(block, source, generated,
                                  sizeof(generated), true, &opens)) return false;
    if (block->trail_event && !nativeScriptTrail_ReadOnlyLine(source, generated))
        return false;
    ScriptCodeLine* next = &block->lines[block->count++];
    memcpy(next->text, source, length + 1);
    memcpy(next->generated, generated, strlen(generated) + 1);
    next->depth = (uint8_t) depth;
    next->opens = opens;
    block->may_invalidate_source |= nativeScriptLifetime_DirectEffect(generated);
    if (block->functions) nativeScriptDependencies_Union(&block->function_calls, nativeScriptFunctions_References(block->functions, source));
    block->last_opens = opens;
    return true;
}

void nativeScriptCode_EmitHostChecks(const ScriptCodeBlock* block, FILE* output)
{
    if (nativeScriptLifetime_BlockMayInvalidate(block))
        fputs(" || host->callback_source_alive == 0", output);
    if (block->landing)
        fputs(" || host->enter_landing_fall_special == 0", output);
    if (nativeScriptCode_Contains(block, "callbacks.reset_root_x_on_damage()"))
        fputs(" || host->set_root_rotation_x == 0", output);
    if (nativeScriptCode_Contains(block, "callbacks.on_hit_state("))
        fputs(" || host->change_motion_state == 0 || host->start_animation == 0", output);
    for (unsigned i = 0; i < block->count; ++i) {
        const char* cursor = block->lines[i].generated;
        while ((cursor = strstr(cursor, "engine->")) != NULL) {
            cursor += 8;
            size_t length = 0;
            while (isalnum((unsigned char) cursor[length]) || cursor[length] == '_')
                ++length;
            if (length != 0)
                fprintf(output, " || host->%.*s == 0", (int) length, cursor);
            if (length == sizeof("sound_channels") - 1 &&
                !memcmp(cursor, "sound_channels", length)) {
                for (unsigned channel = 0; channel < 7; ++channel)
                    fprintf(output, " || host->sound_channels[%u] == 0", channel);
            }
            cursor += length;
        }
    }
}

void nativeScriptCode_EmitLines(const ScriptCodeBlock* block, FILE* output)
{
    if (!block->model_effect && nativeScriptLifetime_BlockMayInvalidate(block))
        nativeScriptLifetime_EmitEntry(output, ScriptCallback_Fighter);
    ScriptCallbackResult result = block->floating_result ? ScriptCallback_Float :
        block->integer_result ? ScriptCallback_Int : block->predicate ? ScriptCallback_Bool : ScriptCallback_Void;
    unsigned open_depth = 0;
    for (unsigned i = 0; i < block->count; ++i) {
        const ScriptCodeLine* line = &block->lines[i];
        while (open_depth > line->depth) {
            --open_depth;
            fprintf(output, "%*s}\n", (int) (4 + open_depth * 4), "");
        }
        fprintf(output, "%*s", (int) (4 + line->depth * 4), "");
        nativeScriptLifetime_EmitLine(output, line->generated, block->functions,
            block->model_effect ? ScriptCallback_ModelEffect : ScriptCallback_Fighter, result);
        fputc('\n', output);
        if (line->opens) open_depth = line->depth + 1;
    }
    while (open_depth != 0) {
        --open_depth;
        fprintf(output, "%*s}\n", (int) (4 + open_depth * 4), "");
    }
}

void nativeScriptCode_EmitBlock(const ScriptCodeBlock* block, bool air,
                            const char* phase, const char* suffix,
                            FILE* output)
{
    if (block->count == 0) return;
    fprintf(output,
            "static void authored_%s_%s_%s(HSD_GObj* gobj) {\n"
            "    Fighter* fighter = gobj->user_data;\n",
            air ? "air" : "ground", phase, suffix);
    nativeScriptCode_EmitLines(block, output);
    fputs("}\n", output);
}
