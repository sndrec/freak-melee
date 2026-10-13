#include "native_fighter_script_jump.h"

#include <string.h>

const char* nativeScriptJump_Property(const char* name)
{
    if (strcmp(name, "air.drift_threshold") == 0)
        return "engine->air_drift_threshold()";
    if (strcmp(name, "collision.basic_landing_velocity_threshold") == 0)
        return "engine->basic_landing_velocity_threshold(fighter)";
    if (strcmp(name, "damage.force_top_blast_ko") == 0)
        return "fighter->x2222_b3";
    return NULL;
}

bool nativeScriptJump_Expression(const ScriptCodeBlock* block,
                                 const char* name, const char** source,
                                 char* output, size_t capacity)
{
    if (strcmp(name, "jumps.configure_multi") == 0) {
        char args[13][192], values[13][512];
        const char* after;
        if (!nativeScriptCode_ParseArguments(*source, args, 13, &after)) return false;
        for (unsigned i = 0; i < 13; ++i) {
            if ((i == 0 || i >= 10) &&
                nativeScriptCode_ExpressionType(block, args[i]) != ScriptType_Int)
                return false;
            if (!nativeScriptCode_Expression(block, args[i], values[i],
                                             sizeof(values[i]))) return false;
        }
        *source = after;
        int size = snprintf(output, capacity,
            "engine->configure_multi_jump(fighter,&(NativeFighterMultiJumpStats){"
            ".turn_frames=%s,.reverse_threshold=%s,.horizontal_impulse=%s,"
            ".acceleration_multiplier=%s,.speed_multiplier=%s,"
            ".vertical_impulses={%s,%s,%s,%s,%s},.count=%s,"
            ".first_motion=%s,.helmet_first_motion=%s})",
            values[0], values[1], values[2], values[3], values[4],
            values[5], values[6], values[7], values[8], values[9],
            values[10], values[11], values[12]);
        return size >= 0 && (size_t) size < capacity;
    }
    if (strcmp(name, "jumps.multi_vertical_speed") != 0) return false;
    char args[1][192], value[512];
    const char* after;
    if (!nativeScriptCode_ParseArguments(*source, args, 1, &after) ||
        nativeScriptCode_ExpressionType(block, args[0]) != ScriptType_Int ||
        !nativeScriptCode_Expression(block, args[0], value, sizeof(value)))
        return false;
    *source = after;
    return snprintf(output, capacity,
                    "engine->multi_jump_vertical_speed(fighter, (int32_t)(%s))",
                    value) < capacity;
}

bool nativeScriptJump_Action(const ScriptCodeBlock* block, const char* source,
                             char* output, size_t capacity)
{
    static const char configure[] = "jumps.configure_multi";
    if (strncmp(source, configure, sizeof(configure) - 1) == 0 &&
        source[sizeof(configure) - 1] == '(') {
        const char* after = source + sizeof(configure) - 1;
        char expression[1024];
        if (!nativeScriptJump_Expression(block, configure, &after, expression,
                                          sizeof(expression)) || *after)
            return false;
        int size = snprintf(output, capacity, "(void)(%s);", expression);
        return size >= 0 && (size_t) size < capacity;
    }
    static const struct { const char* name; const char* code; } simple[] = {
        { "controls.aerial_jump_actions()", "engine->aerial_jump_actions(gobj);" },
        { "state.fall_aerial()", "engine->enter_fall_aerial(gobj);" },
    };
    for (unsigned i = 0; i < sizeof(simple) / sizeof(simple[0]); ++i)
        if (strcmp(source, simple[i].name) == 0)
            return snprintf(output, capacity, "%s", simple[i].code) < capacity;
    const char* flag = "damage.force_top_blast_ko";
    size_t length = strlen(flag);
    if (strncmp(source, flag, length) == 0 &&
        source[length] == ' ' && source[length + 1] == '=') {
        char value[512];
        if (!nativeScriptTypes_Assignable(ScriptType_Bool,
                nativeScriptCode_ExpressionType(block, source + length + 2)) ||
            !nativeScriptCode_Expression(block, source + length + 2, value,
                                         sizeof(value))) return false;
        return snprintf(output, capacity, "fighter->x2222_b3 = (%s);", value) < capacity;
    }
    const char* drift = "physics.air_drift_with_limits";
    length = strlen(drift);
    if (strncmp(source, drift, length) != 0 || source[length] != '(')
        return false;
    char args[3][192], values[3][512];
    const char* after;
    if (!nativeScriptCode_ParseArguments(source + length, args, 3, &after) ||
        *after) return false;
    for (unsigned i = 0; i < 3; ++i)
        if (!nativeScriptCode_Expression(block, args[i], values[i],
                                         sizeof(values[i]))) return false;
    return snprintf(output, capacity,
                    "engine->air_drift_with_limits(gobj, %s, %s, %s);",
                    values[0], values[1], values[2]) < capacity;
}
