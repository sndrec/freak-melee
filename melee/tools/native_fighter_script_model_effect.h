#ifndef NATIVE_FIGHTER_SCRIPT_MODEL_EFFECT_H
#define NATIVE_FIGHTER_SCRIPT_MODEL_EFFECT_H

#include "native_fighter_script_code.h"

enum { SCRIPT_MODEL_EFFECTS = 32 };
typedef struct ScriptModelEffect {
    char name[32];
    uint32_t hash;
    int32_t recipe;
    unsigned line, body_count;
    unsigned body_lines[SCRIPT_CODE_LINES];
    char body[SCRIPT_CODE_LINES][512];
    ScriptCodeBlock code;
} ScriptModelEffect;

typedef struct ScriptModelEffectProgram {
    ScriptModelEffect effects[SCRIPT_MODEL_EFFECTS];
    uint8_t order[SCRIPT_MODEL_EFFECTS];
    unsigned count;
    ScriptFunctionSet helper_calls;
    ScriptFunctionProgram* functions;
} ScriptModelEffectProgram;

ScriptModelEffectProgram* nativeScriptModelEffect_Parse(char*, size_t,
                                                        NativeFighterScriptError*);
void nativeScriptModelEffect_Free(ScriptModelEffectProgram*);
bool nativeScriptModelEffect_Compile(ScriptModelEffectProgram*, const ScriptStateTable*,
    const NativeScriptArticleCatalog*, ScriptFunctionProgram*, NativeFighterScriptError*);
void nativeScriptModelEffect_Emit(const ScriptModelEffectProgram*, FILE*);
void nativeScriptModelEffect_EmitChecks(const ScriptModelEffectProgram*, FILE*);
void nativeScriptModelEffect_EmitDefinition(const ScriptModelEffectProgram*, FILE*);
// 0 means this is not an effect property/action, -1 means malformed.
int nativeScriptModelEffect_Expression(const ScriptCodeBlock*, const char*, char*,
                                      size_t, const char**);
ScriptExpressionType nativeScriptModelEffect_Type(const ScriptCodeBlock*, const char*);
int nativeScriptModelEffect_Action(ScriptCodeBlock*, const char*, char*, size_t);
bool nativeScriptModelEffect_Spawn(const ScriptCodeBlock*, const char*, char*, size_t);
bool nativeScriptModelEffect_AllowedName(const ScriptCodeBlock*, const char*);

#endif
