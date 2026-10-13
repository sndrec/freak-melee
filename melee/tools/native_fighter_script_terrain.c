#include "native_fighter_script_terrain.h"

#include <ctype.h>

#include <stdio.h>
#include <string.h>

// Literal field selection; only the bounded runtime capsule index is dynamic.
typedef struct CapsuleCoordinate {
    char index[192];
    const char* field;
    char axis;
    const char* after;
} CapsuleCoordinate;

static int coordinate(const char* name, const char* source, CapsuleCoordinate* ref)
{
    if (strcmp(name, "hitbox.current") == 0) ref->field = "x4C";
    else if (strcmp(name, "hitbox.previous") == 0) ref->field = "x58";
    else return 0;
    char args[1][192]; const char* after;
    while (isspace((unsigned char)*source)) ++source;
    if (!nativeScriptCode_ParseArguments(source, args, 1, &after) ||
        after[0] != '.' || (after[1] != 'x' && after[1] != 'y' && after[1] != 'z') ||
        isalnum((unsigned char)after[2]) || after[2] == '_' || after[2] == '.' || after[2] == '(')
        return -1;
    const char* next = after + 2;
    while (isspace((unsigned char)*next)) ++next;
    if (*next == '(' || *next == '[' || *next == '.') return -1;
    strcpy(ref->index, args[0]); ref->axis = after[1]; ref->after = after + 2;
    return 1;
}

static int coordinate_source(const char* source, CapsuleCoordinate* ref)
{
    char name[32]; size_t length = 0;
    while (isalnum((unsigned char)source[length]) || source[length] == '_' || source[length] == '.') {
        if (length + 1 >= sizeof(name)) return 0;
        ++length;
    }
    memcpy(name, source, length); name[length] = 0;
    return coordinate(name, source + length, ref);
}

ScriptExpressionType nativeScriptTerrain_Type(const ScriptCodeBlock* block, const char* source)
{
    CapsuleCoordinate ref;
    if (block == NULL || block->model_effect || coordinate_source(source, &ref) != 1 ||
        *ref.after || nativeScriptCode_ExpressionType(block, ref.index) != ScriptType_Int)
        return ScriptType_Invalid;
    return ScriptType_Float;
}

bool nativeScriptTerrain_Expression(const ScriptCodeBlock* block, const char* name,
                                   const char** source, char* output, size_t capacity)
{
    CapsuleCoordinate ref; char index[512];
    if (block == NULL || block->model_effect || coordinate(name, *source, &ref) != 1 ||
        nativeScriptCode_ExpressionType(block, ref.index) != ScriptType_Int ||
        !nativeScriptCode_Expression(block, ref.index, index, sizeof(index))) return false;
    int length = snprintf(output, capacity,
        "({ int32_t capsule_index = (%s); (uint32_t)capsule_index < 4 ? "
        "fighter->x914[capsule_index].%s.%c : 0.0f; })", index, ref.field, ref.axis);
    if (length < 0 || (size_t)length >= capacity) return false;
    *source = ref.after; return true;
}

static bool coordinate_write(const ScriptCodeBlock* block, const char* source,
                             char* output, size_t capacity)
{
    CapsuleCoordinate ref; char index[512], value[512];
    if (block == NULL || block->model_effect || coordinate_source(source, &ref) != 1) return false;
    source = ref.after;
    while (isspace((unsigned char)*source)) ++source;
    if (*source++ != '=' || *source == '=') return false;
    while (isspace((unsigned char)*source)) ++source;
    ScriptExpressionType type = nativeScriptCode_ExpressionType(block, source);
    if ((type != ScriptType_Int && type != ScriptType_Float) ||
        nativeScriptCode_ExpressionType(block, ref.index) != ScriptType_Int ||
        !nativeScriptCode_Expression(block, ref.index, index, sizeof(index)) ||
        !nativeScriptCode_Expression(block, source, value, sizeof(value))) return false;
    int length = snprintf(output, capacity,
        "{ int32_t capsule_index = (%s); if ((uint32_t)capsule_index < 4) { "
        "float capsule_value = (%s); if (isfinite(capsule_value)) "
        "fighter->x914[capsule_index].%s.%c = capsule_value; } }", index, value, ref.field, ref.axis);
    return length >= 0 && (size_t)length < capacity;
}

// Query outputs are declared locals. They cannot be C expressions, persistent
// getters, attributes or arbitrary addresses supplied by a package.
static bool output_local(const ScriptCodeBlock* block, const char* name)
{
    for (unsigned i = 0; i < block->local_count; ++i)
        if (strcmp(block->locals[i], name) == 0) return true;
    return false;
}

bool nativeScriptTerrain_Action(const ScriptCodeBlock* block, const char* source,
                                char* output, size_t capacity)
{
    if (coordinate_write(block, source, output, capacity)) return true;
    static const char compare[] = "stage.compare_floor_islands(";
    if (strncmp(source, compare, sizeof(compare) - 1) == 0) {
        char args[12][192], values[8][512];
        const char* after;
        if (!nativeScriptCode_ParseArguments(source + sizeof(compare) - 2,
                                             args, 12, &after) || *after)
            return false;
        for (unsigned i = 0; i < 8; ++i)
            if (!nativeScriptCode_Expression(block, args[i], values[i],
                                             sizeof(values[i]))) return false;
        for (unsigned i = 8; i < 12; ++i) {
            if (!output_local(block, args[i])) return false;
            for (unsigned j = 8; j < i; ++j)
                if (strcmp(args[i], args[j]) == 0) return false;
        }
        int length = snprintf(output, capacity,
            "{ Vec3 origin = { %s, %s, %s }, other = { %s, %s, %s }; "
            "float depth = %s; bool compare_other = (%s); "
            "mp_UnkStruct0* first = engine->find_floor_island(&origin, depth); "
            "mp_UnkStruct0* second = NULL; Vec3 left = {0}, right = {0}; "
            "if (first != NULL && compare_other) second = engine->find_floor_island(&other, depth); "
            "if (first != NULL) engine->island_bounds(first, &left, &right); "
            "local_%s = first != NULL; local_%s = first != NULL && first == second; "
            "local_%s = left.x; local_%s = right.x; }",
            values[0], values[1], values[2], values[3], values[4], values[5],
            values[6], values[7], args[8], args[9], args[10], args[11]);
        return length >= 0 && (size_t) length < capacity;
    }
    static const char bounds[] = "stage.island_bounds(";
    if (strncmp(source, bounds, sizeof(bounds) - 1) == 0) {
        char args[9][192], values[4][128];
        const char* after;
        if (!nativeScriptCode_ParseArguments(source + sizeof(bounds) - 2,
                                              args, 9, &after) || *after)
            return false;
        for (unsigned i = 0; i < 4; ++i)
            if (!nativeScriptCode_Expression(block, args[i], values[i],
                                             sizeof(values[i]))) return false;
        for (unsigned i = 4; i < 9; ++i) {
            if (!output_local(block, args[i])) return false;
            for (unsigned j = 4; j < i; ++j)
                if (strcmp(args[i], args[j]) == 0) return false;
        }
        int length = snprintf(output, capacity,
            "{ Vec3 origin = { %s, %s, %s }, left = origin, right = origin; "
            "mp_UnkStruct0* island = engine->find_floor_island(&origin, %s); "
            "if (island != NULL) engine->island_bounds(island, &left, &right); "
            "local_%s = island != NULL; local_%s = left.x; local_%s = left.y; "
            "local_%s = right.x; local_%s = right.y; }",
            values[0], values[1], values[2], values[3], args[4], args[5], args[6],
            args[7], args[8]);
        return length >= 0 && (size_t) length < capacity;
    }
    static const char follow[] = "stage.follow_floor(";
    if (strncmp(source, follow, sizeof(follow) - 1) == 0) {
        char args[10][192], values[6][128];
        const char* after;
        if (!nativeScriptCode_ParseArguments(source + sizeof(follow) - 2,
                                              args, 10, &after) || *after)
            return false;
        for (unsigned i = 0; i < 6; ++i)
            if (!nativeScriptCode_Expression(block, args[i], values[i],
                                             sizeof(values[i]))) return false;
        for (unsigned i = 6; i < 10; ++i) {
            if (!output_local(block, args[i])) return false;
            for (unsigned j = 6; j < i; ++j)
                if (strcmp(args[i], args[j]) == 0) return false;
        }
        int length = snprintf(output, capacity,
            "{ Vec3 origin = { %s, %s, %s }, point = origin; "
            "bool found = engine->follow_floor(%s, &origin, 0, &point, 0, 0, %s, %s); "
            "if (!found) point = origin; local_%s = found; "
            "local_%s = point.x; local_%s = point.y; local_%s = point.z; }",
            values[1], values[2], values[3], values[0], values[4], values[5],
            args[6], args[7], args[8], args[9]);
        return length >= 0 && (size_t) length < capacity;
    }
    static const char position[] = "hitbox.set_world_position(";
    if (strncmp(source, position, sizeof(position) - 1) == 0) {
        char args[4][192], values[4][128];
        const char* after;
        if (!nativeScriptCode_ParseArguments(source + sizeof(position) - 2,
                                              args, 4, &after) || *after)
            return false;
        for (unsigned i = 0; i < 4; ++i)
            if (!nativeScriptCode_Expression(block, args[i], values[i],
                                             sizeof(values[i]))) return false;
        int length = snprintf(output, capacity,
            "{ int index = %s; if ((unsigned) index < 4) "
            "engine->set_hitbox_position(&fighter->x914[index], "
            "&(Vec3){ %s, %s, %s }); }",
            values[0], values[1], values[2], values[3]);
        return length >= 0 && (size_t) length < capacity;
    }
    return false;
}
