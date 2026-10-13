#include "native_fighter_script_smash.h"

#include <stdio.h>
#include <string.h>

typedef enum SmashWrite { SmashWrite_Mode, SmashWrite_Float,
    SmashWrite_Int, SmashWrite_Index, SmashWrite_Period } SmashWrite;
static const struct {
    const char* name;
    const char* expression;
    SmashWrite write;
} fields[] = {
    { "smash.charge_mode", "fighter->smash_attrs.state", SmashWrite_Mode },
    { "smash.charge_frames", "fighter->smash_attrs.x2118_frames", SmashWrite_Float },
    { "smash.charge_limit", "fighter->smash_attrs.x211C_holdFrame", SmashWrite_Float },
    { "smash.saved_animation_rate", "fighter->smash_attrs.x2124_frameSpeedMul", SmashWrite_Float },
    { "smash.charge_color", "fighter->smash_attrs.x2128", SmashWrite_Int },
    { "smash.shake_index", "fighter->smash_attrs.x212C", SmashWrite_Index },
    { "smash.shake_period", "fighter->smash_attrs.x212D", SmashWrite_Period },
};
static const struct { const char* name; const char* expression; } constants[] = {
    { "Smash.none", "0" }, { "Smash.precharge", "1" },
    { "Smash.charging", "2" }, { "Smash.release", "3" },
    { "Smash.manual_charge", "4" },
    { "smash.shake_table_count", "((int32_t)engine->common_attribute(NativeFighterCommon_SmashShakeCount))" },
};

const char* nativeScriptSmash_Property(const char* name)
{
    for (unsigned i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i)
        if (!strcmp(name, fields[i].name)) return fields[i].expression;
    for (unsigned i = 0; i < sizeof(constants) / sizeof(constants[0]); ++i)
        if (!strcmp(name, constants[i].name)) return constants[i].expression;
    return NULL;
}

ScriptExpressionType nativeScriptSmash_Type(const char* name)
{
    for (unsigned i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i)
        if (!strcmp(name, fields[i].name))
            return fields[i].write == SmashWrite_Float ? ScriptType_Float : ScriptType_Int;
    for (unsigned i = 0; i < sizeof(constants) / sizeof(constants[0]); ++i)
        if (!strcmp(name, constants[i].name)) return ScriptType_Int;
    return ScriptType_Invalid;
}

bool nativeScriptSmash_Action(const ScriptCodeBlock* block, const char* source,
                             char* output, size_t capacity)
{
    if (!block || block->model_effect) return false;
    for (unsigned i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i) {
        size_t length = strlen(fields[i].name);
        if (strncmp(source, fields[i].name, length) ||
            strncmp(source + length, " = ", 3)) continue;
        const char* expression = source + length + 3;
        ScriptExpressionType type = nativeScriptCode_ExpressionType(block, expression);
        if (fields[i].write == SmashWrite_Float ?
            !nativeScriptTypes_Assignable(ScriptType_Float, type) : type != ScriptType_Int)
            return false;
        char value[1024];
        if (!nativeScriptCode_Expression(block, expression, value, sizeof(value))) return false;
        int result;
        switch (fields[i].write) {
        case SmashWrite_Mode:
            result = snprintf(output, capacity,
                "{ int32_t smash_value = %s; if ((uint32_t)smash_value <= 4) %s = (SmashState)smash_value; }",
                value, fields[i].expression);
            break;
        case SmashWrite_Index:
        case SmashWrite_Period:
            result = snprintf(output, capacity,
                "{ int32_t smash_value = %s; int32_t smash_count = (int32_t)engine->common_attribute(NativeFighterCommon_SmashShakeCount); "
                "if (smash_count > 0 && smash_count <= 255 && smash_value >= %u && "
                "smash_value %s smash_count) %s = (uint8_t)smash_value; }",
                value, fields[i].write == SmashWrite_Index ? 0 : 1,
                fields[i].write == SmashWrite_Index ? "<" : "<=", fields[i].expression);
            break;
        default:
            result = snprintf(output, capacity, "%s = %s;", fields[i].expression, value);
            break;
        }
        return result >= 0 && (size_t)result < capacity;
    }
    return false;
}
