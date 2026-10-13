#ifndef NATIVE_FIGHTER_ARTICLE_OPERATIONS_H
#define NATIVE_FIGHTER_ARTICLE_OPERATIONS_H

#include "native_fighter_item_script.h"
#include <melee/ft/native_fighter_runtime.h>

typedef struct NativeArticleScriptNames {
    struct {
        char name[48];
        uint32_t hash;
        uint32_t offset;
        NativeFighterArticleAnimationComponent animation_components[3];
    } entries[NATIVE_FIGHTER_ARTICLE_MODEL_ROOT_LIMIT];
    unsigned count;
} NativeArticleScriptNames;

bool nativeArticleOperations_DeclareModel(const char*, const char* declaration, NativeArticleScriptNames*);
bool nativeArticleOperations_EmitModelTable(const NativeArticleScriptNames*,
                                          unsigned article_index, bool animation, FILE*);
// Returns 0 when the prefix belongs to another parser, 1 on success, -1 on error.
int nativeArticleOperations_Expression(const char*, const ArticleDeclaration*,
                                        const NativeFighterParameters*, char*,
                                        size_t, const char** after);
int nativeArticleOperations_HeldExpression(const char*, const ArticleDeclaration*,
    const NativeFighterParameters*, char*, size_t, const char** after,
    ScriptExpressionType* type);
bool nativeArticleOperations_Action(const char*, const ArticleDeclaration*,
                                     const NativeFighterParameters*,
                                     const ScriptStateTable*,
                                     const NativeArticleScriptNames*, const NativeArticleScriptNames*,
                                     unsigned article_index, unsigned landed_callback,
                                     char* output, size_t capacity);

#endif
