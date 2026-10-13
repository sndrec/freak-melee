#ifndef NATIVE_FIGHTER_SCRIPT_COLORS_H
#define NATIVE_FIGHTER_SCRIPT_COLORS_H
#include "native_fighter_item_program.h"
#include "native_fighter_script_code.h"

const char* nativeScriptColors_Property(const char*, bool article);
bool nativeScriptColors_ParseVisibility(char*, size_t, uint16_t*, NativeFighterScriptError*);
void nativeScriptColors_EmitVisibility(uint16_t, FILE*);
bool nativeScriptColors_FighterExpression(const ScriptCodeBlock*, const char*,
    const char**, char*, size_t);
bool nativeScriptColors_ItemExpression(const ArticleDeclaration*,
    const NativeFighterParameters*, const char*, const char**, char*, size_t);
int nativeScriptColors_FighterAction(const ScriptCodeBlock*, const char*, char*, size_t);
int nativeScriptColors_ItemAction(const ArticleDeclaration*,
    const NativeFighterParameters*, const char*, char*, size_t);
#endif
