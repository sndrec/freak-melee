#include "native_fighter_script_vectors.h"
#include <stdio.h>
#include <string.h>

int nativeScriptVectors_CrossComponent(const char* name,
    const char values[6][128], char* output, size_t capacity)
{
    static const char prefix[] = "vectors.cross_normalized_";
    if (strncmp(name, prefix, sizeof(prefix)-1) != 0) return 0;
    char axis = name[sizeof(prefix)-1];
    if ((axis != 'x' && axis != 'y' && axis != 'z') || name[sizeof(prefix)] != 0)
        return -1;
    int length = snprintf(output, capacity,
        "({ Vec3 cross_a = { %s, %s, %s }, cross_b = { %s, %s, %s }, cross_result; "
        "engine->vector_cross_normalized(&cross_a, &cross_b, &cross_result); cross_result.%c; })",
        values[0], values[1], values[2], values[3], values[4], values[5], axis);
    return length >= 0 && (size_t)length < capacity ? 1 : -1;
}

bool nativeScriptVectors_Action(const ScriptCodeBlock* block, const char* source,
                                char* output, size_t capacity)
{
    static const char prefix[] = "vectors.rotate(";
    if (!block || block->model_effect || strncmp(source, prefix, sizeof(prefix)-1)) return false;
    char args[8][192], values[5][128]; const char* after;
    if (!nativeScriptCode_ParseArguments(source+sizeof(prefix)-2, args, 8, &after) || *after) return false;
    for (unsigned i=0; i<5; ++i) {
        ScriptExpressionType type = nativeScriptCode_ExpressionType(block, args[i]);
        if ((i==3 && type!=ScriptType_Int) ||
            (i!=3 && type!=ScriptType_Int && type!=ScriptType_Float) ||
            !nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return false;
    }
    for (unsigned i=5; i<8; ++i) {
        bool found=false;
        for (unsigned j=0; j<block->local_count; ++j)
            if (!strcmp(args[i],block->locals[j]) && block->local_types[j]==ScriptType_Float) found=true;
        if (!found) return false;
        for (unsigned j=5; j<i; ++j) if (!strcmp(args[i],args[j])) return false;
    }
    int written=snprintf(output,capacity,
        "{ Vec3 rotated; rotated.x=(%s); rotated.y=(%s); rotated.z=(%s); "
        "int32_t axis=(%s); float angle=(%s); "
        "if (axis==1 || axis==2 || axis==4) engine->vector_rotate(&rotated,axis,angle); "
        "local_%s=rotated.x; local_%s=rotated.y; local_%s=rotated.z; }",
        values[0],values[1],values[2],values[3],values[4],args[5],args[6],args[7]);
    return written>=0 && (size_t)written<capacity;
}
