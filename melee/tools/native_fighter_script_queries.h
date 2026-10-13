#ifndef NATIVE_FIGHTER_SCRIPT_QUERIES_H
#define NATIVE_FIGHTER_SCRIPT_QUERIES_H
#include "native_fighter_script_code.h"
typedef struct ArticleDeclaration ArticleDeclaration;
// 0: unrelated name; 1: valid expression; -1: invalid query/context.
int nativeScriptQueries_Fighter(const ScriptCodeBlock*, const char*, const char**,
                               char*, size_t);
int nativeScriptQueries_Article(const ArticleDeclaration*,
    const NativeFighterParameters*, const char*, const char**, char*, size_t);
int nativeScriptQueries_Action(const ScriptCodeBlock*, const char*, char*, size_t);
#endif
