#ifndef NATIVE_FIGHTER_SCRIPT_SPATIAL_H
#define NATIVE_FIGHTER_SCRIPT_SPATIAL_H
#include "native_fighter_item_program.h"
#include "native_fighter_script_code.h"

int nativeScriptSpatial_FighterAction(const ScriptCodeBlock*, const char*, char*, size_t);
int nativeScriptSpatial_ItemAction(const ArticleDeclaration*,
    const NativeFighterParameters*, const char*, char*, size_t);
#endif
