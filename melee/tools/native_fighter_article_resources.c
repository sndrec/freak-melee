#include "native_fighter_article_resources.h"
#include <melee/it/native_article_resource.h>
#include <melee/ft/ftparts.h>

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static bool emit(char* output, size_t capacity, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int written = vsnprintf(output, capacity, format, args);
    va_end(args);
    return written >= 0 && (size_t)written < capacity;
}

static bool decimal(const char* text, unsigned maximum, unsigned* result)
{
    if (!isdigit((unsigned char)*text)) return false;
    errno = 0;
    char* after;
    unsigned long value = strtoul(text, &after, 10);
    if (*after || errno == ERANGE || value > maximum) return false;
    *result = (unsigned)value;
    return true;
}

bool nativeArticleResources_Declare(const char* line,
                                    NativeArticleScriptResources* declaration)
{
    char capacity[32], reservation[32];
    int used = 0;
    if (declaration->declared ||
        sscanf(line, "    resources capacity %31s%n", capacity, &used) != 1)
        return false;
    unsigned count, part;
    if (!decimal(capacity, NATIVE_ARTICLE_RESOURCE_NODES, &count) || count < 2)
        return false;
    int reserved = -1;
    if (line[used]) {
        int tail = 0;
        if (sscanf(line + used, " reserve_part %31s%n", reservation, &tail) != 1 ||
            line[used + tail] || !decimal(reservation, MAX_FT_PARTS - 1, &part)) return false;
        reserved = (int)part;
    }
    *declaration = (NativeArticleScriptResources){
        .capacity = (uint16_t)count,
        .reserved_part = (int16_t)reserved,
        .declared = true,
    };
    return true;
}

typedef struct ResourceReference {
    char index[192];
    char path[40];
    const char* suffix;
    bool joint;
} ResourceReference;

static bool joint_path(const char* argument, char* path)
{
    size_t size = strlen(argument);
    if (size < 2 || size > 41 || argument[0] != '"' || argument[size - 1] != '"')
        return false;
    memcpy(path, argument + 1, size - 2);
    path[size - 2] = 0;
    const char* cursor = path;
    unsigned depth = 0;
    while (*cursor) {
        if (++depth > 12 || !isdigit((unsigned char)*cursor)) return false;
        unsigned ordinal = 0;
        do {
            ordinal = ordinal * 10 + (unsigned)(*cursor++ - '0');
            if (ordinal > 255) return false;
        } while (isdigit((unsigned char)*cursor));
        if (!*cursor) break;
        if (*cursor++ != '/' || !*cursor) return false;
    }
    return true;
}

static int reference(const char* source, ResourceReference* result)
{
    if (strncmp(source, "item.resource(", 14) != 0) return 0;
    char args[1][192];
    if (!nativeScriptCode_ParseArguments(source + 13, args, 1, &result->suffix))
        return -1;
    strcpy(result->index, args[0]);
    result->joint = false;
    if (strncmp(result->suffix, ".joint(", 7) == 0) {
        if (!nativeScriptCode_ParseArguments(result->suffix + 6, args, 1,
                &result->suffix) || !joint_path(args[0], result->path)) return -1;
        result->joint = true;
    }
    return 1;
}

static bool expression(const char* source, ScriptExpressionType type,
                       const ArticleDeclaration* article,
                       const NativeFighterParameters* parameters,
                       char* output, size_t capacity)
{
    return nativeScriptTypes_Assignable(type,
        nativeFighterItemScript_ExpressionType(source, article, parameters)) &&
        nativeFighterItemScript_Expression(source, output, capacity, article, parameters);
}

typedef struct ResourceProperty {
    const char* suffix;
    const char* channel;
    ScriptExpressionType type;
    bool writable;
} ResourceProperty;

static const ResourceProperty geometry[] = {
    { ".position.x", "NativeArticleResource_PositionX", ScriptType_Float, true },
    { ".position.y", "NativeArticleResource_PositionY", ScriptType_Float, true },
    { ".position.z", "NativeArticleResource_PositionZ", ScriptType_Float, true },
    { ".velocity.x", "NativeArticleResource_VelocityX", ScriptType_Float, true },
    { ".velocity.y", "NativeArticleResource_VelocityY", ScriptType_Float, true },
    { ".velocity.z", "NativeArticleResource_VelocityZ", ScriptType_Float, true },
    { ".collision.position.x", "NativeArticleResource_CollisionPositionX", ScriptType_Float, true },
    { ".collision.position.y", "NativeArticleResource_CollisionPositionY", ScriptType_Float, true },
    { ".collision.position.z", "NativeArticleResource_CollisionPositionZ", ScriptType_Float, true },
    { ".collision.previous.x", "NativeArticleResource_CollisionPreviousX", ScriptType_Float, true },
    { ".collision.previous.y", "NativeArticleResource_CollisionPreviousY", ScriptType_Float, true },
    { ".collision.previous.z", "NativeArticleResource_CollisionPreviousZ", ScriptType_Float, true },
    { ".collision.floor_normal.x", "NativeArticleResource_FloorNormalX", ScriptType_Float, false },
    { ".collision.floor_normal.y", "NativeArticleResource_FloorNormalY", ScriptType_Float, false },
    { ".collision.floor_normal.z", "NativeArticleResource_FloorNormalZ", ScriptType_Float, false },
    { ".collision.left_wall_normal.x", "NativeArticleResource_LeftWallNormalX", ScriptType_Float, false },
    { ".collision.left_wall_normal.y", "NativeArticleResource_LeftWallNormalY", ScriptType_Float, false },
    { ".collision.left_wall_normal.z", "NativeArticleResource_LeftWallNormalZ", ScriptType_Float, false },
    { ".collision.right_wall_normal.x", "NativeArticleResource_RightWallNormalX", ScriptType_Float, false },
    { ".collision.right_wall_normal.y", "NativeArticleResource_RightWallNormalY", ScriptType_Float, false },
    { ".collision.right_wall_normal.z", "NativeArticleResource_RightWallNormalZ", ScriptType_Float, false },
    { ".collision.ceiling_normal.x", "NativeArticleResource_CeilingNormalX", ScriptType_Float, false },
    { ".collision.ceiling_normal.y", "NativeArticleResource_CeilingNormalY", ScriptType_Float, false },
    { ".collision.ceiling_normal.z", "NativeArticleResource_CeilingNormalZ", ScriptType_Float, false },
    { ".surface_line", "NativeArticleResource_SurfaceLine", ScriptType_Int, true },
    { ".collision.environment", "NativeArticleResource_EnvironmentFlags", ScriptType_Int, false },
    { ".collision.floor_line", "NativeArticleResource_FloorLine", ScriptType_Int, false },
    { ".collision.left_wall_line", "NativeArticleResource_LeftWallLine", ScriptType_Int, false },
    { ".collision.right_wall_line", "NativeArticleResource_RightWallLine", ScriptType_Int, false },
    { ".collision.ceiling_line", "NativeArticleResource_CeilingLine", ScriptType_Int, false },
};

static const struct {
    const char* suffix;
    unsigned channel;
    ScriptExpressionType type;
    bool writable;
} joints[] = {
    { ".translation.x", 0, ScriptType_Float, true },
    { ".translation.y", 1, ScriptType_Float, true },
    { ".translation.z", 2, ScriptType_Float, true },
    { ".rotation.x", 3, ScriptType_Float, true },
    { ".rotation.y", 4, ScriptType_Float, true },
    { ".rotation.z", 5, ScriptType_Float, true },
    { ".scale.x", 6, ScriptType_Float, true },
    { ".scale.y", 7, ScriptType_Float, true },
    { ".scale.z", 8, ScriptType_Float, true },
    { ".exists", 9, ScriptType_Bool, false },
    { ".visible", 10, ScriptType_Bool, true },
    { ".quaternion.x", 11, ScriptType_Float, true },
    { ".quaternion.y", 12, ScriptType_Float, true },
    { ".quaternion.z", 13, ScriptType_Float, true },
    { ".quaternion.w", 14, ScriptType_Float, true },
    { ".self_visible", 15, ScriptType_Bool, true },
};

static bool prefix_property(const char* source, const char* property, const char** after)
{
    size_t size = strlen(property);
    if (strncmp(source, property, size) ||
        isalnum((unsigned char)source[size]) || source[size] == '_' ||
        source[size] == '.' || source[size] == '(' || source[size] == '[') return false;
    *after = source + size;
    return true;
}

static bool named_ordinal(const NativeArticleScriptNames* names, const char* name,
                       uint32_t* ordinal)
{
    for (unsigned i = 0; i < names->count; ++i)
        if (!strcmp(name, names->entries[i].name)) {
            *ordinal = i;
            return true;
        }
    return false;
}

// Generate the decoded literal in the caller; runtime never reparses a path.
static bool path_literal(const char* path, char* output, size_t capacity,
                         unsigned* depth)
{
    *depth=0;
    if(!*path)return emit(output,capacity,"NULL");
    size_t used=0;
    int written=snprintf(output,capacity,"(const uint8_t[]){");
    if(written<0||(size_t)written>=capacity)return false;
    used=(size_t)written;
    while(*path){
        unsigned value=0;
        do {value=value*10+(unsigned)(*path++-'0');}while(isdigit((unsigned char)*path));
        written=snprintf(output+used,capacity-used,"%s%u",*depth?",":"",value);
        if(written<0||(size_t)written>=capacity-used)return false;
        used+=(size_t)written;++*depth;
        if(*path)++path;
    }
    return emit(output+used,capacity-used,"}");
}

int nativeArticleResources_Expression(const char* source,
    const ArticleDeclaration* article, const NativeFighterParameters* parameters,
    const NativeArticleScriptResources* resources,
    const NativeArticleScriptNames* roots, const NativeArticleScriptNames* animations,
    char* output, size_t capacity, const char** after, ScriptExpressionType* type)
{
    if (strncmp(source, "item.resources", 14) == 0) {
        if (!resources->declared) return -1;
        if (prefix_property(source, "item.resources.count", after)) {
            *type = ScriptType_Int;
            return emit(output, capacity, "engine->article_resource_count(gobj)") ? 1 : -1;
        }
        if (strncmp(source, "item.resources.begin(", 21)) return -1;
        char args[1][192], value[512];
        if (!nativeScriptCode_ParseArguments(source + 20, args, 1, after) ||
            !expression(args[0], ScriptType_Int, article, parameters, value, sizeof(value)))
            return -1;
        *type = ScriptType_Bool;
        return emit(output, capacity, "engine->article_resource_begin(gobj, (unsigned)(%s))", value) ? 1 : -1;
    }
    ResourceReference ref;
    int found = reference(source, &ref);
    if (found <= 0) return found;
    if (!resources->declared) return -1;
    char index[512], decoded[128]; unsigned depth=0;
    if (!expression(ref.index, ScriptType_Int, article, parameters, index, sizeof(index))) return -1;
    if(ref.joint&&!path_literal(ref.path,decoded,sizeof(decoded),&depth))return -1;
    if (ref.joint) {
        for (unsigned i = 0; i < sizeof(joints) / sizeof(joints[0]); ++i) {
            if (!prefix_property(ref.suffix, joints[i].suffix, after)) continue;
            *type = joints[i].type;
            return emit(output, capacity, "engine->article_resource_read_joint(gobj, (unsigned)(%s), %s, %u, %u)",
                index, decoded, depth, joints[i].channel) ? 1 : -1;
        }
    } else {
        for (unsigned i = 0; i < sizeof(geometry) / sizeof(geometry[0]); ++i) {
            if (!prefix_property(ref.suffix, geometry[i].suffix, after)) continue;
            *type = geometry[i].type;
            return emit(output, capacity, "engine->article_resource_read_%s(gobj, (unsigned)(%s), %s)",
                *type == ScriptType_Int ? "int" : "float", index, geometry[i].channel) ? 1 : -1;
        }
        if (prefix_property(ref.suffix, ".exists", after)) {
            *type = ScriptType_Bool;
            return emit(output, capacity, "engine->article_resource_exists(gobj, (unsigned)(%s))", index) ? 1 : -1;
        }
    }
    char args[6][192], values[6][512];
    const char* cursor = ref.suffix;
    static const struct {
        const char* call;
        const char* host;
        bool joint;
    } no_args[] = {
        { ".bind_capture_anchor()", "set_capture_anchor", false },
        { ".sweep_stage()", "sweep_stage", false },
        { ".sweep_point()", "sweep_point", false },
        { ".follow_floor()", "follow_floor", false },
        { ".advance_animation()", "advance_animation", true },
        { ".remove_animation()", "remove_animation", true },
    };
    for (unsigned i = 0; i < sizeof(no_args) / sizeof(no_args[0]); ++i) {
        if (ref.joint != no_args[i].joint || !prefix_property(cursor, no_args[i].call, after)) continue;
        *type = ScriptType_Bool;
        return ref.joint ?
            (emit(output, capacity, "engine->article_resource_%s(gobj, (unsigned)(%s), %s, %u)",
                no_args[i].host, index, decoded, depth) ? 1 : -1) :
            (emit(output, capacity, "engine->article_resource_%s(gobj, (unsigned)(%s))",
                no_args[i].host, index) ? 1 : -1);
    }
    if (!ref.joint && !strncmp(cursor, ".create(", 8)) {
        bool positioned = nativeScriptCode_ParseArguments(cursor + 7, args, 6, after);
        if (!positioned && !nativeScriptCode_ParseArguments(cursor + 7, args, 3, after)) return -1;
        unsigned count = positioned ? 6 : 3;
        for (unsigned i = 0; i < count; ++i)
            if (!expression(args[i], i == 1 || i >= 3 ? ScriptType_Float : ScriptType_Int,
                    article, parameters, values[i], sizeof(values[i]))) return -1;
        *type = ScriptType_Bool;
        return emit(output, capacity,
            "engine->article_resource_create(gobj, (unsigned)(%s), &(const NativeArticleResourceInit){"
            ".attachment_part = (unsigned)(%s), .radius = %s, .collision_category = (unsigned)(%s), "
            ".position = {%s, %s, %s}})", index, values[0], values[1], values[2],
            positioned ? values[3] : "0", positioned ? values[4] : "0",
            positioned ? values[5] : "0") ? 1 : -1;
    }
    bool load = !ref.joint && !strncmp(cursor, ".load_model(", 12);
    bool animate = ref.joint && !strncmp(cursor, ".add_animation(", 15);
    if (load || animate) {
        uint32_t hash;
        if (!nativeScriptCode_ParseArguments(cursor + (load ? 11 : 14), args, 1, after) ||
            !named_ordinal(load ? roots : animations, args[0], &hash)) return -1;
        *type = ScriptType_Bool;
        return load ?
            (emit(output, capacity, "engine->article_resource_load_model(gobj, (unsigned)(%s), UINT32_C(%u))", index, hash) ? 1 : -1) :
            (emit(output, capacity, "engine->article_resource_add_animation(gobj, (unsigned)(%s), %s, %u, UINT32_C(%u))",
                index, decoded, depth, hash) ? 1 : -1);
    }
    static const struct {
        const char* call;
        const char* host;
        bool joint;
        ScriptExpressionType type;
    } one_arg[] = {
        { ".set_draw(", "set_draw", false, ScriptType_Bool },
        { ".bind_part(", "bind_external_part", false, ScriptType_Int },
        { ".request_animation(", "request_animation", true, ScriptType_Float },
        { ".set_animation_rate(", "set_animation_rate", true, ScriptType_Float },
    };
    for (unsigned i = 0; i < sizeof(one_arg) / sizeof(one_arg[0]); ++i) {
        size_t length = strlen(one_arg[i].call);
        if (ref.joint != one_arg[i].joint || strncmp(cursor, one_arg[i].call, length)) continue;
        if (!nativeScriptCode_ParseArguments(cursor + length - 1, args, 1, after) ||
            !expression(args[0], one_arg[i].type, article, parameters, values[0], sizeof(values[0]))) return -1;
        *type = ScriptType_Bool;
        return ref.joint ?
            (emit(output, capacity, "engine->article_resource_%s(gobj, (unsigned)(%s), %s, %u, %s)",
                one_arg[i].host, index, decoded, depth, values[0]) ? 1 : -1) :
            (emit(output, capacity, "engine->article_resource_%s(gobj, (unsigned)(%s), %s)",
                one_arg[i].host, index, values[0]) ? 1 : -1);
    }
    return -1;
}

int nativeArticleResources_Assignment(const char* target, const char* value,
    const ArticleDeclaration* article, const NativeFighterParameters* parameters,
    const NativeArticleScriptResources* resources, char* output, size_t capacity)
{
    ResourceReference ref;
    int found = reference(target, &ref);
    if (found <= 0) return found;
    if (!resources->declared) return -1;
    char index[512], translated[512], decoded[128]; unsigned depth=0;
    if (!expression(ref.index, ScriptType_Int, article, parameters, index, sizeof(index))) return -1;
    if(ref.joint&&!path_literal(ref.path,decoded,sizeof(decoded),&depth))return -1;
    if (ref.joint) {
        for (unsigned i = 0; i < sizeof(joints) / sizeof(joints[0]); ++i) {
            if (strcmp(ref.suffix, joints[i].suffix)) continue;
            if (!joints[i].writable || !expression(value, joints[i].type, article,
                    parameters, translated, sizeof(translated))) return -1;
            return emit(output, capacity, "engine->article_resource_write_joint(gobj, (unsigned)(%s), %s, %u, %u, %s);",
                index, decoded, depth, joints[i].channel, translated) ? 1 : -1;
        }
    } else {
        for (unsigned i = 0; i < sizeof(geometry) / sizeof(geometry[0]); ++i) {
            if (strcmp(ref.suffix, geometry[i].suffix)) continue;
            if (!geometry[i].writable || !expression(value, geometry[i].type, article,
                    parameters, translated, sizeof(translated))) return -1;
            return emit(output, capacity, "engine->article_resource_write_%s(gobj, (unsigned)(%s), %s, %s);",
                geometry[i].type == ScriptType_Int ? "int" : "float", index,
                geometry[i].channel, translated) ? 1 : -1;
        }
    }
    return -1;
}

int nativeArticleResources_Action(const char* line,
    const ArticleDeclaration* article, const NativeFighterParameters* parameters,
    const NativeArticleScriptResources* resources, const NativeArticleScriptNames* roots,
    const NativeArticleScriptNames* animations, const ScriptStateTable* state,
    char* output, size_t capacity)
{
    // Assignment validation belongs to the assignment emitter, after the
    // item's general action dispatcher has split its target and value.
    if (strstr(line, " = ")) return 0;
    if (!strcmp(line, "item.resources.release()")) {
        if (!resources->declared) return -1;
        return emit(output, capacity, "engine->article_resource_release(gobj);") ? 1 : -1;
    }
    ResourceReference ref;
    int reference_found = reference(line, &ref);
    if (reference_found < 0) return -1;
    if (reference_found > 0 && !ref.joint) {
        bool facing = !strncmp(ref.suffix, ".spawn_effect_facing_at_position(", 33);
        bool position = !strncmp(ref.suffix, ".spawn_effect_at_position(", 26);
        if (facing || position) {
            char args[5][192], values[5][512], index[512]; const char* end;
            unsigned count = facing ? 5 : 4;
            if (!resources->declared ||
                nativeFighterItemScript_ExpressionType(ref.index, article, parameters) != ScriptType_Int ||
                !nativeScriptCode_ParseArguments(ref.suffix + (facing ? 32 : 25),
                    args, count, &end) || *end ||
                nativeFighterItemScript_ExpressionType(args[0], article, parameters) != ScriptType_Int ||
                !expression(ref.index, ScriptType_Int, article, parameters, index, sizeof(index))) return -1;
            for (unsigned i = 0; i < count; ++i) {
                ScriptExpressionType type = nativeFighterItemScript_ExpressionType(args[i], article, parameters);
                if ((i == 0 ? type != ScriptType_Int :
                     type != ScriptType_Int && type != ScriptType_Float) ||
                    !expression(args[i], i == 0 ? ScriptType_Int : ScriptType_Float,
                        article, parameters, values[i], sizeof(values[i]))) return -1;
            }
            // Source order is explicit. The shared lifetime emitter guards any
            // mutating helper in each initializer before the next operand.
            if (facing) return emit(output, capacity,
                "{ unsigned resource_index = (unsigned)(%s); int32_t resource_effect = (%s); "
                "float resource_x = (%s); float resource_y = (%s); float resource_z = (%s); "
                "float resource_facing = (%s); engine->article_resource_spawn_effect_facing_at_position("
                "gobj, resource_index, resource_effect, &(Vec3){resource_x, resource_y, resource_z}, resource_facing); }",
                index, values[0], values[1], values[2], values[3], values[4]) ? 1 : -1;
            return emit(output, capacity,
                "{ unsigned resource_index = (unsigned)(%s); int32_t resource_effect = (%s); "
                "float resource_x = (%s); float resource_y = (%s); float resource_z = (%s); "
                "engine->article_resource_spawn_effect_at_position(gobj, resource_index, resource_effect, "
                "&(Vec3){resource_x, resource_y, resource_z}); }",
                index, values[0], values[1], values[2], values[3]) ? 1 : -1;
        }
        static const struct {
            const char* call;
            const char* host;
            bool write;
        } matrices[] = {
            { ".read_attachment_matrix(", "read_attachment_matrix", false },
            { ".read_world_matrix(", "read_world_matrix", false },
            { ".set_world_matrix(", "set_world_matrix", true },
        };
        for (unsigned i = 0; i < sizeof(matrices) / sizeof(matrices[0]); ++i) {
            size_t length = strlen(matrices[i].call);
            if (strncmp(ref.suffix, matrices[i].call, length)) continue;
            char args[1][192], index[512]; const char* end;
            if (!resources->declared ||
                !nativeScriptCode_ParseArguments(ref.suffix + length - 1, args, 1, &end) || *end ||
                strncmp(args[0], "item.state.", 11) ||
                !expression(ref.index, ScriptType_Int, article, parameters, index, sizeof(index))) return -1;
            const ScriptStateField* field = nativeFighterScript_StateField(state, args[0] + 11);
            if (!field || !field->array || field->type != ScriptState_Float || field->count != 12) return -1;
            if (matrices[i].write) return emit(output, capacity,
                "{ Mtx matrix; memcpy(matrix, item->xDD4_itemVar.native_article.state + %u, sizeof(matrix)); "
                "engine->article_resource_%s(gobj, (unsigned)(%s), matrix); }",
                field->offset, matrices[i].host, index) ? 1 : -1;
            return emit(output, capacity,
                "{ Mtx matrix = { 0 }; engine->article_resource_%s(gobj, (unsigned)(%s), matrix); "
                "memcpy(item->xDD4_itemVar.native_article.state + %u, matrix, sizeof(matrix)); }",
                matrices[i].host, index, field->offset) ? 1 : -1;
        }
        bool point = !strncmp(ref.suffix, ".read_attachment_point(", 23);
        bool speed = !strncmp(ref.suffix, ".read_line_speed(", 17);
        if (point || speed) {
            char args[6][192], values[3][512], index[512]; const char* end;
            unsigned input_count = point ? 3 : 1;
            if (!resources->declared ||
                !nativeScriptCode_ParseArguments(ref.suffix + (point ? 22 : 16), args,
                    input_count + 3, &end) || *end ||
                !expression(ref.index, ScriptType_Int, article, parameters, index, sizeof(index))) return -1;
            for (unsigned i = 0; i < input_count; ++i)
                if (!expression(args[i], point ? ScriptType_Float : ScriptType_Int,
                        article, parameters, values[i], sizeof(values[i]))) return -1;
            const ScriptStateField* fields[3];
            for (unsigned i = 0; i < 3; ++i) {
                if (strncmp(args[input_count + i], "item.state.", 11)) return -1;
                fields[i] = nativeFighterScript_StateField(state, args[input_count + i] + 11);
                if (!fields[i] || fields[i]->array || fields[i]->type != ScriptState_Float) return -1;
                for (unsigned j = 0; j < i; ++j)
                    if (fields[j] == fields[i]) return -1;
            }
            char query[2048];
            if (!(point ? emit(query, sizeof(query),
                    "engine->article_resource_read_attachment_point(gobj, (unsigned)(%s), &(Vec3){ %s, %s, %s }, &result);",
                    index, values[0], values[1], values[2]) : emit(query, sizeof(query),
                    "engine->article_resource_read_line_speed(gobj, (unsigned)(%s), (int)(%s), &result);", index, values[0]))) return -1;
            return emit(output, capacity,
                "{ Vec3 result = { 0 }; %s scripted_article_set_state_float(item, %u, result.x); "
                "scripted_article_set_state_float(item, %u, result.y); "
                "scripted_article_set_state_float(item, %u, result.z); }",
                query, fields[0]->offset, fields[1]->offset, fields[2]->offset) ? 1 : -1;
        }
    }
    const char* after;
    ScriptExpressionType type;
    char translated[2048];
    int found = nativeArticleResources_Expression(line, article, parameters, resources,
        roots, animations, translated, sizeof(translated), &after, &type);
    if (found <= 0) return found;
    if (*after || type != ScriptType_Bool) return -1;
    return emit(output, capacity, "%s;", translated) ? 1 : -1;
}
