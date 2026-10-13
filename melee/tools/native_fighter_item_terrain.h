#ifndef NATIVE_FIGHTER_ITEM_TERRAIN_H
#define NATIVE_FIGHTER_ITEM_TERRAIN_H

#include "native_fighter_item_script.h"

// Exact public collision constants; NULL means the name is not in this unit.
const char* nativeItemTerrain_Constant(const char* name);
// 0: another grammar, 1: emitted expression, -1: malformed terrain expression.
int nativeItemTerrain_Expression(const char* source, const ArticleDeclaration*,
    const NativeFighterParameters*, char* output, size_t capacity,
    const char** after, ScriptExpressionType* type);

#endif
