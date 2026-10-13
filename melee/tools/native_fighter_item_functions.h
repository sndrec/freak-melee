#ifndef NATIVE_FIGHTER_ITEM_FUNCTIONS_H
#define NATIVE_FIGHTER_ITEM_FUNCTIONS_H
#include "native_fighter_item_program.h"

enum { ITEM_FUNCTION_LIMIT = 32, ITEM_FUNCTION_ARGS = 8 };
typedef struct NativeItemFunction {
    char name[32];
    char argument_names[ITEM_FUNCTION_ARGS][32];
    ScriptExpressionType argument_types[ITEM_FUNCTION_ARGS];
    unsigned argument_count, line, first_body_line;
    ScriptExpressionType return_type;
    size_t begin, end;
    const char* source;
    ItemCallback body;
    bool may_invalidate_source;
} NativeItemFunction;
typedef struct NativeItemFunctionProgram {
    NativeItemFunction functions[ITEM_FUNCTION_LIMIT];
    unsigned count;
} NativeItemFunctionProgram;

bool nativeItemFunctions_Parse(ArticleDeclaration*, const char*, size_t, size_t,
    unsigned, NativeFighterScriptError*);
bool nativeItemFunctions_Contains(const ArticleDeclaration*, size_t, bool*);
int nativeItemFunctions_Call(const ArticleDeclaration*, const char*,
    const NativeFighterParameters*, char*, size_t);
ScriptExpressionType nativeItemFunctions_Result(const ArticleDeclaration*, const char*, bool*);
int nativeItemFunctions_Expression(const ArticleDeclaration*, const char*,
    const NativeFighterParameters*, char*, size_t, const char**);
ScriptExpressionType nativeItemFunctions_RecipientResult(const ArticleDeclaration*, const char*);
int nativeItemFunctions_RecipientCall(const ArticleDeclaration*, const char*,
    const NativeFighterParameters*, bool, char*, size_t, const char**);
bool nativeItemFunctions_Compile(ArticleDeclaration*, const NativeFighterParameters*,
    NativeFighterScriptError*);
void nativeItemFunctions_Emit(const ArticleDeclaration*, FILE*);
#endif
