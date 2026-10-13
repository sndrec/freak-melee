#include "native_fighter_script_signal.h"
#include <string.h>

uint32_t nativeScriptSignal_Hash(const char* name)
{
    uint32_t hash = UINT32_C(2166136261);
    for (; *name; ++name) {
        hash ^= (unsigned char) *name;
        hash *= UINT32_C(16777619);
    }
    return hash;
}

bool nativeScriptSignal_Emit(const ScriptCodeBlock* block, const char* source,
                            char* output, size_t capacity)
{
    bool owned = strncmp(source, "articles.signal_owned(", 22) == 0;
    if (!owned && strncmp(source, "articles.signal(", 16) != 0) return false;
    if (!block) return false;
    char arguments[4][192];
    const char* after;
    unsigned name_index = 2;
    bool declared = nativeScriptCode_ParseArguments(strchr(source, '('),
                                                    arguments, 4, &after) && !*after;
    // Historical three-argument inputs remain active C-oracle fixtures only.
    // The interpreter requires the explicit Article declaration for both APIs.
    if(!declared) {
        if(!owned || !nativeScriptCode_ParseArguments(strchr(source,'('),arguments,3,&after) || *after)return false;
        name_index=1;
    }
    const NativeScriptArticleDeclaration* declaration = declared ?
        nativeScriptArticleRefs_Declaration(block->articles, arguments[1]) : NULL;
    if (declared && !declaration)
        return false;
    size_t size = strlen(arguments[name_index]);
    if (size < 3 || arguments[name_index][0] != '"' || arguments[name_index][size - 1] != '"')
        return false;
    arguments[name_index][size - 1] = '\0';
    const char* name = arguments[name_index] + 1;
    if (!nativeScriptCode_Identifier(name, size - 2, 48)) return false;
    char handle[384], value[384];
    if (nativeScriptCode_ExpressionType(block, arguments[0]) != ScriptType_Int ||
        !nativeScriptCode_Expression(block, arguments[0], handle, sizeof(handle)) ||
        !nativeScriptCode_Expression(block, arguments[name_index + 1], value, sizeof(value)))
        return false;
    int length = owned && !declared ? snprintf(output, capacity,
        "engine->signal_owned_article(gobj, (s32)(%s), UINT32_C(%u), (float)(%s));",
        handle, nativeScriptSignal_Hash(name), value) : owned ? snprintf(output,capacity,
        "{ uint32_t signal_handle=(uint32_t)(%s); float signal_value=(float)(%s); "
        "if (engine->article_ref_exists(gobj, signal_handle, %s)) "
        "engine->signal_owned_article(gobj, (s32)signal_handle, UINT32_C(%u), signal_value); }",
        handle,value,declaration->kind,nativeScriptSignal_Hash(name)) : snprintf(output, capacity,
        "{ uint32_t signal_handle=(uint32_t)(%s); float signal_value=(float)(%s); "
        "if (engine->article_ref_exists(gobj, signal_handle, %s)) "
        "engine->signal_article(gobj, signal_handle, UINT32_C(%u), signal_value); }",
        handle, value, declaration->kind, nativeScriptSignal_Hash(name));
    return length >= 0 && (size_t) length < capacity;
}
