#ifndef NATIVE_FIGHTER_ARTICLE_RESOURCES_H
#define NATIVE_FIGHTER_ARTICLE_RESOURCES_H

#include "native_fighter_article_operations.h"

typedef struct NativeArticleScriptResources {
    uint16_t capacity;
    int16_t reserved_part;
    bool declared;
} NativeArticleScriptResources;

// A declaration is immutable package metadata, not a runtime count/reservation.
bool nativeArticleResources_Declare(const char*, NativeArticleScriptResources*);
// Different prefixes return 0, invalid resource syntax -1, successful emission 1.
int nativeArticleResources_Expression(const char*, const ArticleDeclaration*,
    const NativeFighterParameters*, const NativeArticleScriptResources*,
    const NativeArticleScriptNames* roots, const NativeArticleScriptNames* animations,
    char*, size_t, const char** after, ScriptExpressionType*);
int nativeArticleResources_Assignment(const char* target, const char* value,
    const ArticleDeclaration*, const NativeFighterParameters*,
    const NativeArticleScriptResources*, char*, size_t);
int nativeArticleResources_Action(const char*, const ArticleDeclaration*,
    const NativeFighterParameters*, const NativeArticleScriptResources*,
    const NativeArticleScriptNames*, const NativeArticleScriptNames*,
    const ScriptStateTable*, char*, size_t);

#endif
