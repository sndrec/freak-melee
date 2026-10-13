#ifndef MELEE_FT_NATIVE_SCRIPT_EFFECT_PROGRAM_H
#define MELEE_FT_NATIVE_SCRIPT_EFFECT_PROGRAM_H
#include "native_script_compile.h"
#include <melee/ft/forward.h>
#include <melee/ef/types.h>

enum { NATIVE_SCRIPT_EFFECT_PROGRAMS=64,NATIVE_SCRIPT_EFFECT_SELECTORS=128 };
typedef struct NativeScriptEffectSelector {
    uint32_t semantic,path_offset,path_length;
    uint8_t depth,path[12],member,channel,argument_count;
} NativeScriptEffectSelector;
typedef struct NativeScriptEffectProgram {
    const NativeScriptCompileBank* bank;
    uint64_t identity;
    FighterKind author_kind;
    uint32_t owner,update;
    int32_t gfx_id;
    EF_ModelConstructorFamily family;
    uint32_t member_count;
    NativeScriptEffectSelector selectors[NATIVE_SCRIPT_EFFECT_SELECTORS];
    uint32_t selector_count;
} NativeScriptEffectProgram;
typedef struct NativeScriptEffectPrograms {
    NativeScriptEffectProgram programs[NATIVE_SCRIPT_EFFECT_PROGRAMS];
    uint32_t count;
} NativeScriptEffectPrograms;
bool nativeScriptEffectMetadata_Owns(const NativeScriptSourceTree*,uint32_t node);
bool nativeScriptEffectMetadata_Validate(const NativeScriptSourceTree*,uint32_t node,NativeScriptDiagnostic*);
bool nativeScriptEffectPrograms_Prepare(const NativeScriptCompileBank*,FighterKind,uint64_t identity,
    NativeScriptEffectPrograms*,NativeScriptDiagnostic*);
const NativeScriptEffectProgram* nativeScriptEffect_Program(FighterKind,uint32_t owner);
#endif
