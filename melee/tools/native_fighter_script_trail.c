#include "native_fighter_script_trail.h"

#include <string.h>
#include <ctype.h>

const char* nativeScriptTrail_Field(const char* name)
{
    static const char* fields[] = {
        "bone", "axis", "inner_distance", "outer_distance",
        "inner_fade", "outer_fade", "start_alpha", "end_alpha",
        "inner_red", "inner_green", "inner_blue",
        "outer_red", "outer_green", "outer_blue",
    };
    if (strncmp(name, "trail.", 6) != 0) return NULL;
    for (unsigned i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i)
        if (strcmp(name + 6, fields[i]) == 0) return fields[i];
    return NULL;
}

bool nativeScriptTrail_Assignment(const ScriptCodeBlock* block,
                                  const char* source, char* output,
                                  size_t capacity)
{
    if (!block->trail_event) return false;
    const char* assignment = strstr(source, " = ");
    if (assignment == NULL || assignment - source >= 64) return false;
    char name[64];
    memcpy(name, source, assignment - source);
    name[assignment - source] = 0;
    const char* field = nativeScriptTrail_Field(name);
    char value[1024];
    if (field == NULL || !nativeScriptCode_Expression(block, assignment + 3,
                                                     value, sizeof(value)))
        return false;
    int length = snprintf(output, capacity, "trail->%s = %s;", field, value);
    return length >= 0 && (size_t) length < capacity;
}

bool nativeScriptTrail_ReadOnlyLine(const char* source, const char* generated)
{
    // Rendering descriptors may read pose/state but must never tick it. Reject
    // engine calls, helper calls, and assignments outside this output/local
    // scope, including hidden RNG consumption in an expression.
    if (strstr(generated, "engine->") || strstr(generated, "script_fn_") ||
        strstr(source, ".consume(") ||
        strstr(generated, "script_state_set_")) return false;
    const char* expression = generated;
    for (const char* c = expression; *c; ++c) {
        if ((c[0] == '+' && c[1] == '+') || (c[0] == '-' && c[1] == '-'))
            return false;
        if (*c == '=' && c[1] != '=' &&
            (c == expression || (c[-1] != '=' && c[-1] != '!' &&
                                 c[-1] != '<' && c[-1] != '>'))) {
            const char* end = c;
            while (end > expression && isspace((unsigned char) end[-1])) --end;
            const char* target = end;
            while (target > expression &&
                   (isalnum((unsigned char) target[-1]) ||
                    strchr("_>-", target[-1]))) --target;
            if (end - target >= 64) return false;
            char name[64];
            memcpy(name, target, end - target); name[end - target] = 0;
            if (strncmp(name, "local_", 6) != 0 &&
                strncmp(name, "trail->", 7) != 0) return false;
        }
    }
    return strncmp(generated, "trail->", 7) == 0 ||
        strncmp(generated, "float local_", 12) == 0 ||
        strncmp(generated, "local_", 6) == 0 ||
        strncmp(generated, "if (", 4) == 0 ||
        strncmp(generated, "else", 4) == 0 ||
        strcmp(source, "pass") == 0 || strcmp(source, "return") == 0;
}
