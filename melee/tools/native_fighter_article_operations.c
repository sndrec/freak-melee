#include "native_fighter_article_operations.h"
#include "native_fighter_item_program.h"
#include "native_fighter_script_signal.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static bool model_offset(const char* text, uint32_t* result)
{
    if (!isdigit((unsigned char)text[0])) return false;
    char* end; errno = 0;
    unsigned long long offset = strtoull(text, &end, 10);
    if (*end || errno == ERANGE || offset > UINT32_MAX || (offset & 3) != 0)
        return false;
    *result = (uint32_t)offset;
    return true;
}

static bool animation_component(const char* text,
                                NativeFighterArticleAnimationComponent* result)
{
    *result = (NativeFighterArticleAnimationComponent){ UINT32_MAX, UINT32_MAX };
    if (strcmp(text, "None") == 0) return true;
    const char* bracket = strchr(text, '[');
    if (bracket == NULL) return model_offset(text, &result->attribute_offset);
    size_t length = (size_t)(bracket - text);
    char offset[48];
    if (length == 0 || length >= sizeof(offset) ||
        !isdigit((unsigned char)bracket[1])) return false;
    memcpy(offset, text, length); offset[length] = '\0';
    char* end; errno = 0;
    unsigned long long index = strtoull(bracket + 1, &end, 10);
    if (errno == ERANGE || *end != ']' || end[1] != '\0' ||
        index >= UINT32_MAX / 4 || !model_offset(offset, &result->attribute_offset))
        return false;
    result->array_index = (uint32_t)index;
    return true;
}

bool nativeArticleOperations_DeclareModel(const char* line, const char* declaration,
                                          NativeArticleScriptNames* names)
{
    char words[9][48];
    int count = sscanf(line, " %47s %47s %47s %47s %47s %47s %47s %47s %47s",
        words[0], words[1], words[2], words[3], words[4], words[5],
        words[6], words[7], words[8]);
    if (names->count == NATIVE_FIGHTER_ARTICLE_MODEL_ROOT_LIMIT ||
        count < 3 || strcmp(words[0], declaration) != 0 ||
        !nativeScriptCode_Identifier(words[1], strlen(words[1]), sizeof(words[1])))
        return false;
    uint32_t offset = 0;
    NativeFighterArticleAnimationComponent components[3];
    for (unsigned i = 0; i < 3; ++i)
        components[i] = (NativeFighterArticleAnimationComponent){ UINT32_MAX, UINT32_MAX };
    if (count == 3) {
        if (!model_offset(words[2], &offset)) return false;
        if (strcmp(declaration, "model_root") == 0) {
            if (offset > 1020) return false;
        } else {
            if (offset > UINT32_MAX - 8) return false;
            for (unsigned i = 0; i < 3; ++i) components[i].attribute_offset = offset + 4 * i;
        }
    } else {
        if (strcmp(declaration, "model_animation") != 0 ||
            count > 8 || (count & 1) != 0) return false;
        unsigned seen = 0;
        static const char* channels[] = { "joint", "material", "shape" };
        for (int i = 2; i < count; i += 2) {
            unsigned channel = 0;
            while (channel < 3 && strcmp(words[i], channels[channel]) != 0) ++channel;
            if (channel == 3 || (seen & (1u << channel))) return false;
            seen |= 1u << channel;
            if (!animation_component(words[i + 1], &components[channel])) return false;
        }
    }
    uint32_t hash = nativeScriptSignal_Hash(words[1]);
    if (hash == 0) return false;
    for (unsigned i = 0; i < names->count; ++i)
        if (names->entries[i].hash == hash) return false;
    unsigned index = names->count++;
    strcpy(names->entries[index].name, words[1]);
    names->entries[index].hash = hash;
    names->entries[index].offset = offset;
    memcpy(names->entries[index].animation_components, components, sizeof(components));
    return true;
}

bool nativeArticleOperations_EmitModelTable(const NativeArticleScriptNames* names,
                                            unsigned index, bool animation, FILE* output)
{
    if (names->count == 0) return true;
    fprintf(output, "static const %s scripted_article_%u_%s[] = {\n",
        animation ? "NativeFighterArticleModelAnimation" : "NativeFighterArticleModelRoot",
        index, animation ? "model_animations" : "model_roots");
    for (unsigned i = 0; i < names->count; ++i) {
        fprintf(output, "    { UINT32_C(%u), ", names->entries[i].hash);
        if (animation) {
            fputs("{ ", output);
            for (unsigned channel = 0; channel < 3; ++channel) {
                const NativeFighterArticleAnimationComponent* component =
                    &names->entries[i].animation_components[channel];
                fprintf(output, "%s{ UINT32_C(%u), UINT32_C(%u) }",
                    channel ? ", " : "", component->attribute_offset, component->array_index);
            }
            fputs(" }", output);
        }
        else fprintf(output, "%u", names->entries[i].offset);
        fputs(" },\n", output);
    }
    fputs("};\n", output);
    return !ferror(output);
}

static bool root_slot(const char* text, unsigned* slot)
{
    char* end;
    unsigned long value = strtoul(text, &end, 10);
    if (text == end || *end || value < 1 || value > 3) return false;
    *slot = (unsigned) value;
    return true;
}

static bool joint_path(const char* text, char path[40])
{
    size_t length = strlen(text);
    if (length < 2 || length > 41 || text[0] != '"' ||
        text[length - 1] != '"') return false;
    memcpy(path, text + 1, length - 2);
    path[length - 2] = '\0';
    unsigned depth = 0;
    const char* cursor = path;
    while (*cursor) {
        if (++depth > 12 || !isdigit((unsigned char)*cursor)) return false;
        unsigned index = 0;
        while (isdigit((unsigned char)*cursor))
            index = index * 10 + (unsigned)(*cursor++ - '0');
        if (index > 255) return false;
        if (!*cursor) break;
        if (*cursor++ != '/' || !*cursor) return false;
    }
    return true;
}

static bool emit_add_animation(const char* source,
                               const NativeArticleScriptNames* animations,
                               char* output, size_t capacity, const char** after)
{
    char args[3][192], path[40] = ""; const char* end_source; char* end;
    unsigned name = 1;
    if (nativeScriptCode_ParseArguments(strchr(source, '('), args, 3, &end_source)) {
        if (!joint_path(args[1], path)) return false;
        name = 2;
    } else if (!nativeScriptCode_ParseArguments(strchr(source, '('), args, 2, &end_source)) return false;
    unsigned long model = strtoul(args[0], &end, 10);
    if (end == args[0] || *end || model > 3) return false;
    for (unsigned i = 0; i < animations->count; ++i) {
        if (strcmp(args[name], animations->entries[i].name) != 0) continue;
        int written = snprintf(output, capacity,
            "engine->item_add_model_animation(gobj, %lu, \"%s\", UINT32_C(%u))",
            model, path, animations->entries[i].hash);
        if (written < 0 || (size_t)written >= capacity) return false;
        *after = end_source;
        return true;
    }
    return false;
}

int nativeArticleOperations_Expression(const char* source,
                                        const ArticleDeclaration* article,
                                        const NativeFighterParameters* parameters,
                                        char* output, size_t capacity,
                                        const char** after)
{
    if (strncmp(source, "item.add_model_animation(", 25) == 0) {
        return article && emit_add_animation(source, &article->model_animations,
            output, capacity, after) ? 1 : -1;
    }
    if (strncmp(source, "item.hits_stage()", 17) == 0) {
        *after = source + 17;
        int written = snprintf(output, capacity, "engine->item_hits_stage(gobj)");
        return written >= 0 && (size_t)written < capacity ? 1 : -1;
    }
    if (strncmp(source, "item.create_at_position(", 24) == 0) {
        char args[5][192], values[5][512];
        if (!nativeScriptCode_ParseArguments(source + 23, args, 5, after)) return -1;
        for (unsigned i = 0; i < 5; ++i)
            if (!nativeFighterItemScript_Expression(args[i], values[i], sizeof(values[i]),
                                                    article, parameters)) return -1;
        int written = snprintf(output, capacity,
            "engine->create_item_at_position(gobj, (ItemKind)(%s), &(Vec3){ %s, %s, %s }, %s)",
            values[0], values[1], values[2], values[3], values[4]);
        return written >= 0 && (size_t)written < capacity ? 1 : -1;
    }
    if (strncmp(source, "item.link_id(", 13) == 0) {
        char args[1][192], value[512];
        if (!nativeScriptCode_ParseArguments(source + 12, args, 1, after) ||
            !nativeScriptTypes_Assignable(ScriptType_Int,
                nativeFighterItemScript_ExpressionType(args[0], article, parameters)) ||
            !nativeFighterItemScript_Expression(args[0], value, sizeof(value),
                                                article, parameters))
            return -1;
        int written = snprintf(output, capacity,
                                "engine->item_link_id(gobj, (unsigned)(%s))", value);
        return written >= 0 && (size_t) written < capacity ? 1 : -1;
    }
    if (strncmp(source, "item.create_child(", 18) == 0) {
        char args[9][192], values[9][512];
        if (!nativeScriptCode_ParseArguments(source + 17, args, 9, after)) return -1;
        for (unsigned i = 0; i < 9; ++i) {
            bool integer = i == 0 || i == 5 || i == 6 || i == 8;
            if (integer && !nativeScriptTypes_Assignable(ScriptType_Int,
                    nativeFighterItemScript_ExpressionType(args[i], article, parameters)))
                return -1;
            if (!nativeFighterItemScript_Expression(args[i], values[i], sizeof(values[i]),
                                                   article, parameters))
                return -1;
        }
        int written = snprintf(output, capacity,
            "engine->item_create_child(gobj, (ItemKind)(%s), &(Vec3){ %s, %s, %s }, "
            "%s, (uint32_t)(%s), (uint32_t)(%s), %s, (unsigned)(%s))",
            values[0], values[1], values[2], values[3], values[4],
            values[5], values[6], values[7], values[8]);
        return written >= 0 && (size_t) written < capacity ? 1 : -1;
    }
    static const struct {
        const char* name;
        const char* host;
        const char* outputs;
        bool integer;
    } queries[] = {
        { "stage.line_exists(", "stage_line_exists", "", true },
        { "item.read_stage_line_speed(", "stage_line_speed",
          ", &item->pos, &item->x40_vel", true },
        { "targets.is_fighter(", "target_is_fighter", "", false },
        { "targets.guard_active(", "target_guard_active", "", false },
        { "targets.model_scale(", "target_model_scale", "", false },
        { "targets.motion(", "target_motion", "", false },
        { "targets.damage(", "target_damage", "", false },
        { "targets.facing(", "target_facing", "", false },
        { "targets.sleeping(", "target_sleeping", "", false },
        { "targets.floor_line(", "target_floor_line", "", false },
        { "targets.cpu_body_half_width(", "target_cpu_body_half_width", "", false },
        { "targets.airborne(", "target_airborne", "", false },
        { "targets.smash_charge_active(", "target_smash_charge_active", "", false },
    };
    for (unsigned i = 0; i < sizeof(queries) / sizeof(queries[0]); ++i) {
        size_t size = strlen(queries[i].name);
        if (strncmp(source, queries[i].name, size) != 0) continue;
        char args[1][192], value[512];
        if (!nativeScriptCode_ParseArguments(source + size - 1, args, 1, after) ||
            ((!strcmp(queries[i].host,"target_motion") || !strcmp(queries[i].host,"target_airborne") ||
              !strcmp(queries[i].host,"target_smash_charge_active")) &&
             nativeFighterItemScript_ExpressionType(args[0],article,parameters) != ScriptType_Int) ||
            !nativeFighterItemScript_Expression(args[0], value, sizeof(value),
                                               article, parameters)) return -1;
        int written = snprintf(output, capacity, "engine->%s((%s)(%s)%s)",
                                queries[i].host, queries[i].integer ? "int" : "uint32_t",
                                value, queries[i].outputs);
        return written >= 0 && (size_t)written < capacity ? 1 : -1;
    }
    return 0;
}

static const ScriptStateField* output_field(const ScriptStateTable* state,
                                             const char* name, ScriptStateType type)
{
    if (strncmp(name, "item.state.", 11) != 0) return NULL;
    const ScriptStateField* field = nativeFighterScript_StateField(state, name + 11);
    return field != NULL && !field->array && field->type == type ? field : NULL;
}

static bool output_fields(const ScriptStateTable* state, char args[][192],
                           unsigned first, unsigned count,
                           const ScriptStateType* types,
                           const ScriptStateField** fields)
{
    for (unsigned i = 0; i < count; ++i) {
        fields[i] = output_field(state, args[first + i], types[i]);
        if (fields[i] == NULL) return false;
        for (unsigned j = 0; j < i; ++j)
            if (fields[i] == fields[j]) return false;
    }
    return true;
}

bool nativeArticleOperations_Action(const char* line,
                                     const ArticleDeclaration* article,
                                     const NativeFighterParameters* parameters,
                                     const ScriptStateTable* state,
                                     const NativeArticleScriptNames* roots,
                                     const NativeArticleScriptNames* animations,
                                     unsigned article_index, unsigned landed_callback,
                                     char* output, size_t capacity)
{
    if (strcmp(line, "item.land_on_stage()") == 0)
        return snprintf(output, capacity,
            "engine->item_land_on_stage(gobj, scripted_article_%u_logic_%u);",
            article_index, landed_callback) < capacity;
    static const struct { const char* name; const char* code; } scalars[] = {
        { "item.set_animation_frame(", "engine->item_set_animation_frame(gobj, %s);" },
        { "item.bind_stage_line(", "engine->set_collision_line(&item->x378_itemColl, (int)(%s));" },
        { "item.read_stage_line_speed(", "engine->stage_line_speed((int)(%s), &item->pos, &item->x40_vel);" },
    };
    for (unsigned i = 0; i < sizeof(scalars) / sizeof(scalars[0]); ++i) {
        if (strncmp(line, scalars[i].name, strlen(scalars[i].name)) != 0) continue;
        char args[1][192], value[512]; const char* after;
        if (!nativeScriptCode_ParseArguments(strchr(line, '('), args, 1, &after) ||
            *after || !nativeFighterItemScript_Expression(args[0], value, sizeof(value),
                                                         article, parameters)) return false;
        return snprintf(output, capacity, scalars[i].code, value) < capacity;
    }
    if (strncmp(line, "item.add_animation_descriptor(", 30) == 0) {
        char args[1][192], value[512]; const char* after;
        if (!nativeScriptCode_ParseArguments(strchr(line, '('), args, 1, &after) || *after ||
            !nativeFighterItemScript_Expression(args[0], value, sizeof(value), article, parameters)) return false;
        return snprintf(output, capacity, "engine->item_add_animation_descriptor(gobj, (unsigned)(%s));", value) < capacity;
    }
    if (strncmp(line, "item.read_attachment_matrix_rotation(", 37) == 0) {
        char args[3][192]; const char* after;
        const ScriptStateField* fields[3];
        const ScriptStateType types[3] = { ScriptState_Float, ScriptState_Float, ScriptState_Float };
        if (!nativeScriptCode_ParseArguments(strchr(line, '('), args, 3, &after) || *after ||
            !output_fields(state, args, 0, 3, types, fields)) return false;
        return snprintf(output, capacity,
            "{ Vec3 rotation = { 0 }; engine->item_read_attachment_matrix_rotation(gobj, &rotation); "
            "scripted_article_set_state_float(item, %u, rotation.x); "
            "scripted_article_set_state_float(item, %u, rotation.y); "
            "scripted_article_set_state_float(item, %u, rotation.z); }",
            fields[0]->offset, fields[1]->offset, fields[2]->offset) < capacity;
    }
    if (strncmp(line, "item.sweep_segment(", 19) == 0) {
        char args[9][192], values[3][512]; const char* after;
        const ScriptStateField* fields[6];
        static const ScriptStateType types[] = { ScriptState_Float, ScriptState_Float,
            ScriptState_Float, ScriptState_Int, ScriptState_Int, ScriptState_Bool };
        if (!nativeScriptCode_ParseArguments(strchr(line, '('), args, 9, &after) ||
            *after || !output_fields(state, args, 3, 6, types, fields)) return false;
        for (unsigned i = 0; i < 3; ++i)
            if (!nativeFighterItemScript_Expression(args[i], values[i], sizeof(values[i]),
                                                    article, parameters)) return false;
        return snprintf(output, capacity,
            "{ Vec3 previous = { %s, %s, %s }; Vec3 normal = { "
            "scripted_article_state_float(item, %u), scripted_article_state_float(item, %u), scripted_article_state_float(item, %u) }; "
            "int line = scripted_article_state_int(item, %u); u32 flags = (u32)scripted_article_state_int(item, %u); "
            "bool hit = engine->item_segment_sweep(gobj, &previous, &item->pos, &normal, &line, &flags); "
            "scripted_article_set_state_float(item, %u, normal.x); scripted_article_set_state_float(item, %u, normal.y); "
            "scripted_article_set_state_float(item, %u, normal.z); scripted_article_set_state_int(item, %u, line); "
            "scripted_article_set_state_int(item, %u, (int32_t)flags); scripted_article_set_state_bool(item, %u, hit); }",
            values[0], values[1], values[2], fields[0]->offset, fields[1]->offset,
            fields[2]->offset, fields[3]->offset, fields[4]->offset,
            fields[0]->offset, fields[1]->offset, fields[2]->offset, fields[3]->offset,
            fields[4]->offset, fields[5]->offset) < capacity;
    }
    if (strncmp(line, "stage.line_normal(", 18) == 0 ||
        strncmp(line, "targets.shield(", 15) == 0) {
        bool shield = strncmp(line, "targets.", 8) == 0;
        char args[5][192], value[512]; const char* after;
        const ScriptStateField* fields[4];
        static const ScriptStateType types[] = { ScriptState_Float, ScriptState_Float,
            ScriptState_Float, ScriptState_Float };
        if (!nativeScriptCode_ParseArguments(strchr(line, '('), args, shield ? 5 : 4, &after) ||
            *after || !output_fields(state, args, 1, shield ? 4 : 3, types, fields) ||
            !nativeFighterItemScript_Expression(args[0], value, sizeof(value),
                                               article, parameters)) return false;
        char query[600], radius[128] = "";
        if (shield) {
            snprintf(query, sizeof(query), "float radius = engine->target_shield((uint32_t)(%s), &v);", value);
            snprintf(radius, sizeof(radius), "scripted_article_set_state_float(item, %u, radius);", fields[3]->offset);
        } else snprintf(query, sizeof(query), "engine->stage_line_normal((int)(%s), &v);", value);
        return snprintf(output, capacity,
            "{ Vec3 v = { 0 }; %s scripted_article_set_state_float(item, %u, v.x); "
            "scripted_article_set_state_float(item, %u, v.y); scripted_article_set_state_float(item, %u, v.z); %s }",
            query, fields[0]->offset, fields[1]->offset, fields[2]->offset, radius) < capacity;
    }
    if (strncmp(line, "hitbox.set_damage(", 18) == 0) {
        char args[2][192], value[512]; const char* after; char* end;
        if (!nativeScriptCode_ParseArguments(strchr(line, '('), args, 2, &after) || *after) return false;
        unsigned long index = strtoul(args[0], &end, 10);
        if (end == args[0] || *end || index >= 4 ||
            !nativeFighterItemScript_Expression(args[1], value, sizeof(value), article, parameters)) return false;
        return snprintf(output, capacity,
            "engine->item_set_hitbox_damage(&item->x5D4_hitboxes[%lu].hit, (u32)(%s), gobj);",
            index, value) < capacity;
    }
    if (strncmp(line, "targets.read_camera_target_position(", 36) == 0) {
        char args[5][192], handle[512]; const char* after;
        unsigned count = 5;
        if (!nativeScriptCode_ParseArguments(strchr(line, '('), args, count, &after)) {
            count = 4;
            if (!nativeScriptCode_ParseArguments(strchr(line, '('), args, count, &after)) return false;
        }
        const ScriptStateType types[4] = { ScriptState_Float, ScriptState_Float, ScriptState_Float, ScriptState_Bool };
        const ScriptStateField* fields[4];
        if (*after || !output_fields(state, args, 1, count - 1, types, fields) ||
            !nativeFighterItemScript_Expression(args[0], handle, sizeof(handle), article, parameters)) return false;
        int length = snprintf(output, capacity,
            "{ Vec3 position = { 0 }; bool found = engine->target_camera_target_position((uint32_t)(%s), &position); "
            "scripted_article_set_state_float(item, %u, position.x); scripted_article_set_state_float(item, %u, position.y); "
            "scripted_article_set_state_float(item, %u, position.z); ", handle, fields[0]->offset, fields[1]->offset, fields[2]->offset);
        if (length < 0 || (size_t)length >= capacity) return false;
        if (count == 5) return snprintf(output + length, capacity - length,
            "scripted_article_set_state_bool(item, %u, found); }", fields[3]->offset) < capacity - length;
        return snprintf(output + length, capacity - length, "(void)found; }") < capacity - length;
    }
    if (strncmp(line, "item.spawn_effect(", 18) == 0) {
        char args[10][192], values[10][256]; const char* after;
        if (!nativeScriptCode_ParseArguments(strchr(line, '('), args, 10, &after) || *after) return false;
        for (unsigned i = 0; i < 10; ++i)
            if (!nativeFighterItemScript_Expression(args[i], values[i], sizeof(values[i]), article, parameters)) return false;
        return snprintf(output, capacity,
            "engine->item_spawn_effect(gobj, (s32)(%s), (s32)(%s), &(Vec3){ %s, %s, %s }, "
            "&(Vec3){ %s, %s, %s }, (s32)(%s), %s);",
            values[0], values[1], values[2], values[3], values[4], values[5],
            values[6], values[7], values[8], values[9]) < capacity;
    }
    if (strncmp(line, "item.spawn_joint_effect_at(", 27) == 0) {
        char args[5][192], values[4][256], path[40]; const char* after;
        if (!nativeScriptCode_ParseArguments(strchr(line, '('), args, 5, &after) ||
            *after || !joint_path(args[1], path)) return false;
        for (unsigned i = 0; i < 4; ++i)
            if (!nativeFighterItemScript_Expression(args[i == 0 ? 0 : i + 1], values[i],
                    sizeof(values[i]), article, parameters)) return false;
        return snprintf(output, capacity,
            "engine->item_spawn_joint_effect_at(gobj, (int)(%s), \"%s\", &(Vec3){ %s, %s, %s });",
            values[0], path, values[1], values[2], values[3]) < capacity;
    }
    if (strncmp(line, "item.add_model_animation(", 25) == 0) {
        const char* after;
        if (!emit_add_animation(line, animations, output, capacity, &after) || *after) return false;
        size_t length = strlen(output);
        if (length + 1 >= capacity) return false;
        output[length] = ';'; output[length + 1] = '\0';
        return true;
    }
    static const struct { const char* name; const char* host; unsigned count; } model_ops[] = {
        { "item.request_model_animation(", "item_request_model_animation", 2 },
        { "item.advance_model_animation(", "item_advance_model_animation", 1 },
        { "item.remove_model_animation(", "item_remove_model_animation", 1 }
    };
    for (unsigned i = 0; i < sizeof(model_ops)/sizeof(model_ops[0]); ++i) {
        if (strncmp(line, model_ops[i].name, strlen(model_ops[i].name)) != 0) continue;
        char args[3][192], path[40] = "", value[512]; const char* after; char* end;
        unsigned value_index = 1;
        if (nativeScriptCode_ParseArguments(strchr(line, '('), args, model_ops[i].count + 1, &after)) {
            if (!joint_path(args[1], path)) return false;
            value_index = 2;
        } else if (!nativeScriptCode_ParseArguments(strchr(line, '('), args, model_ops[i].count, &after)) return false;
        if (*after) return false;
        unsigned long model = strtoul(args[0], &end, 10);
        if (end == args[0] || *end || model > 3) return false;
        if (model_ops[i].count == 1)
            return snprintf(output, capacity, "engine->%s(gobj, %lu, \"%s\");", model_ops[i].host, model, path) < capacity;
        if (!nativeFighterItemScript_Expression(args[value_index], value, sizeof(value), article, parameters)) return false;
        return snprintf(output, capacity, "engine->%s(gobj, %lu, \"%s\", %s);", model_ops[i].host, model, path, value) < capacity;
    }
    if (strncmp(line, "item.set_model_animation_rate(", 30) == 0) {
        char args[3][192], path[40], rate[512]; const char* after; char* end;
        if (!nativeScriptCode_ParseArguments(strchr(line, '('), args, 3, &after) || *after ||
            !joint_path(args[1], path)) return false;
        unsigned long model = strtoul(args[0], &end, 10);
        if (end == args[0] || *end || model > 3 ||
            !nativeFighterItemScript_Expression(args[2], rate, sizeof(rate), article, parameters)) return false;
        return snprintf(output, capacity, "engine->item_set_model_animation_rate(gobj, %lu, \"%s\", %s);", model, path, rate) < capacity;
    }
    if (strncmp(line, "item.read_joint_matrix_rotation(", 32) == 0) {
        char args[5][192], path[40]; const char* after; char* end;
        const ScriptStateField* fields[3];
        const ScriptStateType types[3] = { ScriptState_Float, ScriptState_Float, ScriptState_Float };
        if (!nativeScriptCode_ParseArguments(strchr(line, '('), args, 5, &after) || *after ||
            !joint_path(args[1], path) || !output_fields(state, args, 2, 3, types, fields)) return false;
        unsigned long model = strtoul(args[0], &end, 10);
        if (end == args[0] || *end || model > 3) return false;
        return snprintf(output, capacity,
            "{ Vec3 rotation = { 0 }; engine->item_read_joint_matrix_rotation(gobj, %lu, \"%s\", &rotation); "
            "scripted_article_set_state_float(item, %u, rotation.x); scripted_article_set_state_float(item, %u, rotation.y); "
            "scripted_article_set_state_float(item, %u, rotation.z); }",
            model, path, fields[0]->offset, fields[1]->offset, fields[2]->offset) < capacity;
    }
    if (strncmp(line, "item.set_model_draw_enabled(", 28) == 0) {
        char args[2][192], value[512]; const char* after; unsigned slot;
        if (!nativeScriptCode_ParseArguments(strchr(line, '('), args, 2, &after) ||
            *after || !root_slot(args[0], &slot) ||
            !nativeFighterItemScript_Expression(args[1], value, sizeof(value),
                                               article, parameters)) return false;
        return snprintf(output, capacity,
            "{ if (%s) item->native_model_draw_disabled_mask &= (u8)~(1u << %u); "
            "else item->native_model_draw_disabled_mask |= (u8)(1u << %u); }",
            value, slot - 1, slot - 1) < capacity;
    }
    if (strncmp(line, "item.load_model(", 16) == 0) {
        char args[2][192]; const char* after; unsigned slot;
        if (!nativeScriptCode_ParseArguments(strchr(line, '('), args, 2, &after) ||
            *after || !root_slot(args[0], &slot)) return false;
        for (unsigned i = 0; i < roots->count; ++i) {
            if (strcmp(args[1], roots->entries[i].name) != 0) continue;
            return snprintf(output, capacity,
                "engine->item_load_model(gobj, %u, UINT32_C(%u));",
                slot, roots->entries[i].hash) < capacity;
        }
        return false;
    }
    if (strncmp(line, "item.set_joint_transform(", 25) == 0) {
        char args[13][192], values[11][512], path[40];
        const char* after; char* end;
        if (!nativeScriptCode_ParseArguments(strchr(line, '('), args, 13, &after) ||
            *after || !joint_path(args[1], path)) return false;
        unsigned long model = strtoul(args[0], &end, 10);
        if (end == args[0] || *end || model > 3) return false;
        for (unsigned i = 0; i < 11; ++i)
            if (!nativeFighterItemScript_Expression(args[i + 2], values[i],
                    sizeof(values[i]), article, parameters)) return false;
        return snprintf(output, capacity,
            "engine->item_set_joint_transform(gobj, %lu, \"%s\", "
            "&(Vec3){ %s, %s, %s }, &(Quaternion){ %s, %s, %s, %s }, "
            "&(Vec3){ %s, %s, %s }, %s);",
            model, path, values[0], values[1], values[2], values[3], values[4],
            values[5], values[6], values[7], values[8], values[9], values[10]) < capacity;
    }
    if (strncmp(line, "item.set_appearance(", 20) == 0) {
        char args[2][192], descriptor[512], frame[512]; const char* after;
        if (!nativeScriptCode_ParseArguments(strchr(line, '('), args, 2, &after) || *after ||
            nativeFighterItemScript_ExpressionType(args[0], article, parameters) != ScriptType_Int ||
            !nativeFighterItemScript_Expression(args[0], descriptor, sizeof(descriptor), article, parameters) ||
            !nativeFighterItemScript_Expression(args[1], frame, sizeof(frame), article, parameters)) return false;
        return snprintf(output, capacity,
            "{ int descriptor = %s; if (descriptor >= 0 && "
            "(u32)descriptor < item->xC4_article_data->xC_itemStates->count) "
            "engine->item_set_appearance(gobj, descriptor, %s); }", descriptor, frame) < capacity;
    }
    return false;
}

// Live holder observations use the current owner target, never creator or saved state.
int nativeArticleOperations_HeldExpression(const char* source,
    const ArticleDeclaration* article, const NativeFighterParameters* parameters,
    char* output, size_t capacity, const char** after, ScriptExpressionType* type)
{
    static const char* names[] = { "id", "kind", "motion", "frame" };
    for (unsigned i = 0; i < 4; ++i) {
        char name[48];
        snprintf(name, sizeof(name), "holder.held_%s", names[i]);
        size_t length = strlen(name);
        if (strncmp(source, name, length) == 0) {
            const char* next = source + length;
            if (isalnum((unsigned char)*next) || *next == '_' || *next == '.') return -1;
            while (isspace((unsigned char)*next)) ++next;
            if (*next == '(' || article == NULL) return -1;
            int written = snprintf(output, capacity, "engine->target_held_%s(engine->item_current_owner(gobj))", names[i]);
            if (written < 0 || (size_t)written >= capacity) return -1;
            *after = source + length;
            *type = i == 3 ? ScriptType_Float : ScriptType_Int;
            return 1;
        }
        snprintf(name, sizeof(name), "targets.held_%s", names[i]);
        length = strlen(name);
        if (strncmp(source, name, length) != 0) continue;
        const char* open = source + length;
        while (isspace((unsigned char)*open)) ++open;
        char args[1][192], value[1024];
        if (article == NULL || *open != '(' ||
            !nativeScriptCode_ParseArguments(open, args, 1, after) ||
            nativeFighterItemScript_ExpressionType(args[0], article, parameters) != ScriptType_Int ||
            !nativeFighterItemScript_Expression(args[0], value, sizeof(value), article, parameters)) return -1;
        int written = snprintf(output, capacity, "engine->target_held_%s((uint32_t)(%s))", names[i], value);
        if (written < 0 || (size_t)written >= capacity) return -1;
        *type = i == 3 ? ScriptType_Float : ScriptType_Int;
        return 1;
    }
    return 0;
}
