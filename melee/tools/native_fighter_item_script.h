#ifndef NATIVE_FIGHTER_ITEM_SCRIPT_H
#define NATIVE_FIGHTER_ITEM_SCRIPT_H

#include "native_fighter_script.h"
#include "native_fighter_script_code.h"

typedef struct NativeFighterItemProgram NativeFighterItemProgram;
typedef struct ArticleDeclaration ArticleDeclaration;
// Shared expression parser for the dedicated article-operation emitters.
bool nativeFighterItemScript_Expression(const char*, char*, size_t,
                                         const ArticleDeclaration*,
                                         const NativeFighterParameters*);
ScriptExpressionType nativeFighterItemScript_ExpressionType(
    const char*, const ArticleDeclaration*, const NativeFighterParameters*);

// Keep line numbers intact in the fighter source while extracting item blocks.
NativeFighterItemProgram* nativeFighterItemScript_Parse(
    const char* source, size_t size, char** fighter_source,
    unsigned fighter_reserved_state_bytes,
    const NativeFighterParameters* parameters, const ScriptFunctionProgram* functions,
    NativeFighterScriptError* error);
unsigned nativeFighterItemScript_StateArrayTypes(const NativeFighterItemProgram*);
const NativeScriptArticleCatalog* nativeFighterItemScript_ArticleCatalog(
    const NativeFighterItemProgram*);

bool nativeFighterItemScript_Emit(const NativeFighterItemProgram* program,
                                  const char* asset_root, FILE* output,
                                  NativeFighterScriptError* error);
unsigned nativeFighterItemScript_Count(const NativeFighterItemProgram* program);
unsigned nativeFighterItemScript_ArticleCount(
    const NativeFighterItemProgram* program);
bool nativeFighterItemScript_HostChecks(const NativeFighterItemProgram* program,
                                        char* output, size_t capacity);
void nativeFighterItemScript_Free(NativeFighterItemProgram* program);

#endif
