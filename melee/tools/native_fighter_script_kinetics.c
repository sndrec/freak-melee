#include "native_fighter_script_kinetics.h"
#include <string.h>

static bool arguments(const ScriptCodeBlock* block, const char* source,
                      unsigned count, char values[6][256], const char** after)
{
    char args[6][192];
    if (!nativeScriptCode_ParseArguments(source, args, count, after)) return false;
    for (unsigned i = 0; i < count; ++i)
        if (!nativeScriptCode_Expression(block, args[i], values[i], 256)) return false;
    return true;
}

bool nativeScriptKinetics_Expression(const ScriptCodeBlock* block,
    const char* name, const char** source, char* output, size_t capacity)
{
    if (strcmp(name, "vectors.angle") != 0) return false;
    char values[6][256]; const char* after;
    if (!arguments(block, *source, 6, values, &after)) return false;
    int size = snprintf(output, capacity,
        "engine->vector_angle(&(Vec3){%s,%s,%s}, &(Vec3){%s,%s,%s})",
        values[0], values[1], values[2], values[3], values[4], values[5]);
    if (size < 0 || (size_t)size >= capacity) return false;
    *source = after;
    return true;
}

bool nativeScriptKinetics_Action(const ScriptCodeBlock* block, const char* source,
                                char* output, size_t capacity)
{
    if (!block->model_effect && strncmp(source, "position.", 9) == 0 &&
        (source[9] == 'x' || source[9] == 'y' || source[9] == 'z') &&
        strncmp(source + 10, " = ", 3) == 0) {
        char value[1024];
        if (strstr(source + 13, "targets.call(")) return false;
        if (!nativeScriptCode_Expression(block, source + 13, value, sizeof(value))) return false;
        int size = snprintf(output, capacity, "fighter->cur_pos.%c = %s;", source[9], value);
        return size >= 0 && (size_t)size < capacity;
    }
    if (!block->model_effect && strncmp(source, "position.delta.", 15) == 0 &&
        (source[15] == 'x' || source[15] == 'y' || source[15] == 'z') &&
        strncmp(source + 16, " = ", 3) == 0) {
        char value[1024];
        if (strstr(source + 19, "targets.call(")) return false;
        if (!nativeScriptTypes_Assignable(ScriptType_Float, nativeScriptCode_ExpressionType(block, source + 19)) ||
            !nativeScriptCode_Expression(block, source + 19, value, sizeof(value))) return false;
        int size = snprintf(output, capacity, "fighter->pos_delta.%c = %s;", source[15], value);
        return size >= 0 && (size_t)size < capacity;
    }
    static const struct { const char* name; const char* field; } movement_flags[] = {
        { "movement.used_tether = ", "used_tether" },
        { "movement.fastfall = ", "fall_fast" },
    };
    for (unsigned i = 0; i < sizeof(movement_flags) / sizeof(movement_flags[0]); ++i) {
        size_t prefix = strlen(movement_flags[i].name);
        if (block->model_effect || strncmp(source, movement_flags[i].name, prefix) != 0) continue;
        char value[1024];
        if (nativeScriptCode_ExpressionType(block, source + prefix) != ScriptType_Bool ||
            !nativeScriptCode_Expression(block, source + prefix, value, sizeof(value))) return false;
        int size = snprintf(output, capacity, "fighter->%s = (%s) != 0;", movement_flags[i].field, value);
        return size >= 0 && (size_t)size < capacity;
    }
    if (strcmp(source, "state.down_bound()") == 0)
        return snprintf(output, capacity, "engine->enter_down_bound(gobj);") < capacity;
    static const struct { const char* name; unsigned count; } calls[] = {
        { "state.freefall_blend", 6 }, { "physics.mirror_velocity", 3 },
        { "physics.rotate_velocity_unit_axis", 4 },
    };
    for (unsigned i = 0; i < sizeof(calls) / sizeof(calls[0]); ++i) {
        size_t length = strlen(calls[i].name);
        if (strncmp(source, calls[i].name, length) != 0 || source[length] != '(') continue;
        char values[6][256]; const char* after;
        if (!arguments(block, source + length, calls[i].count, values, &after) || *after) return false;
        int size;
        if (i == 0)
            size = snprintf(output, capacity,
                "engine->enter_freefall_blend(gobj,%s,%s,%s,%s,%s,%s);",
                values[0], values[1], values[2], values[3], values[4], values[5]);
        else if (i == 1)
            size = snprintf(output, capacity,
                "engine->vector_mirror(&fighter->self_vel,&(Vec3){%s,%s,%s});",
                values[0], values[1], values[2]);
        else
            size = snprintf(output, capacity,
                "engine->vector_rotate_unit_axis(&fighter->self_vel,&(Vec3){%s,%s,%s},%s);",
                values[0], values[1], values[2], values[3]);
        return size >= 0 && (size_t)size < capacity;
    }
    return false;
}
