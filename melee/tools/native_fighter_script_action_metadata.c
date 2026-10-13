#include "native_fighter_script_action_metadata.h"

#include <stdio.h>

bool nativeScriptActionMetadata_Expression(const ScriptCodeBlock* block,
                                           const char* source, char* output,
                                           size_t capacity, const char** after)
{
    char arguments[4][192], values[4][384];
    if (!nativeScriptCode_ParseArguments(source, arguments, 4, after))
        return false;
    for (unsigned i = 0; i < 4; ++i) {
        if (nativeScriptCode_ExpressionType(block, arguments[i]) != ScriptType_Int ||
            !nativeScriptCode_Expression(block, arguments[i], values[i],
                                        sizeof(values[i]))) return false;
    }
    // Declarations enforce source-order, single evaluation even when arguments
    // call helpers. Unsigned masks and shifts also define negative input values.
    int written = snprintf(output, capacity,
        "({ uint32_t metadata_move = (uint32_t)(%s); "
        "uint32_t metadata_category = (uint32_t)(%s); "
        "uint32_t metadata_animation = (uint32_t)(%s); "
        "uint32_t metadata_statistics = (uint32_t)(%s); "
        "(int32_t)((metadata_move & UINT32_C(255)) | "
        "((metadata_category & UINT32_C(15)) << 20) | "
        "((metadata_animation & UINT32_C(15)) << 16) | "
        "((metadata_statistics & UINT32_C(255)) << 8)); })",
        values[0], values[1], values[2], values[3]);
    return written >= 0 && (size_t)written < capacity;
}
