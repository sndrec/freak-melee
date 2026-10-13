#include "native_fighter_script_collision.h"
#include <stdio.h>
#include <string.h>

bool nativeScriptCollision_Expression(const ScriptCodeBlock* block,
                                      const char* name, const char** source,
                                      char* output, size_t capacity)
{
    if (strcmp(name, "stage.sweep_box") != 0 || block == NULL ||
        block->model_effect) return false;
    char args[13][192], values[13][512];
    const char* after;
    if (!nativeScriptCode_ParseArguments(*source, args, 13, &after)) return false;
    for (unsigned i = 0; i < 13; ++i) {
        ScriptExpressionType type = nativeScriptCode_ExpressionType(block, args[i]);
        if ((i == 12 && type != ScriptType_Int) ||
            (i != 12 && type != ScriptType_Int && type != ScriptType_Float) ||
            !nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return false;
    }
    // Sample each input once, in order. Preserve initialization and both
    // position pushes from the ordinary fixed-ECB sweep contract.
    int written = snprintf(output, capacity,
        "({ Vec3 previous; previous.x=(%s); previous.y=(%s); previous.z=(%s); "
        "Vec3 current; current.x=(%s); current.y=(%s); current.z=(%s); "
        "ftCollisionBox box; box.top=(%s); box.bottom=(%s); "
        "box.left.x=(%s); box.left.y=(%s); box.right.x=(%s); box.right.y=(%s); "
        "int32_t mode=(%s); CollData probe; engine->initialize_collision(&probe); "
        "probe.last_pos=probe.cur_pos; probe.cur_pos=previous; "
        "probe.last_pos=probe.cur_pos; probe.cur_pos=current; "
        "probe.x34_flags.b1234=mode; engine->sweep_collision_box(&probe,&box); "
        "(int32_t)probe.env_flags; })",
        values[0], values[1], values[2], values[3], values[4], values[5],
        values[6], values[7], values[8], values[9], values[10], values[11], values[12]);
    if (written < 0 || (size_t)written >= capacity) return false;
    *source = after;
    return true;
}

bool nativeScriptCollision_Action(const ScriptCodeBlock* block,
                                  const char* source, char* output,
                                  size_t capacity)
{
    static const struct { const char* name; const char* member; } methods[] = {
        { "hitbox.clear_victims", "clear_hitbox_victims" },
    };
    if (block == NULL || block->model_effect) return false;
    for (unsigned i = 0; i < sizeof(methods)/sizeof(methods[0]); ++i) {
        size_t length = strlen(methods[i].name);
        if (strncmp(source, methods[i].name, length) != 0 || source[length] != '(') continue;
        char args[1][192], value[512]; const char* after;
        if (!nativeScriptCode_ParseArguments(source + length, args, 1, &after) ||
            *after || nativeScriptCode_ExpressionType(block, args[0]) != ScriptType_Int ||
            !nativeScriptCode_Expression(block, args[0], value, sizeof(value))) return false;
        int written = snprintf(output, capacity,
            "{ int32_t index=(%s); if ((uint32_t)index < 4) engine->%s(&fighter->x914[index]); }",
            value, methods[i].member);
        return written >= 0 && (size_t)written < capacity;
    }
    return false;
}
