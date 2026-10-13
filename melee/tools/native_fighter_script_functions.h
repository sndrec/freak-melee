#ifndef NATIVE_FIGHTER_SCRIPT_FUNCTIONS_H
#define NATIVE_FIGHTER_SCRIPT_FUNCTIONS_H

#include "native_fighter_script_code.h"

enum { SCRIPT_FUNCTION_ARGS = 8 };

typedef struct ScriptFunction {
    char name[32];
    char argument_names[SCRIPT_FUNCTION_ARGS][32];
    const char* argument_types[SCRIPT_FUNCTION_ARGS];
    unsigned argument_count;
    ScriptExpressionType return_type;
    bool exported;
    uint32_t export_hash;
    bool may_invalidate_source;
    bool disabled;
    bool model_effect_only;
    unsigned line;
    unsigned body_lines[SCRIPT_CODE_LINES];
    char body[SCRIPT_CODE_LINES][512];
    unsigned body_count;
    ScriptCodeBlock code;
} ScriptFunction;

struct ScriptFunctionProgram {
    ScriptFunction functions[SCRIPT_FUNCTIONS];
    unsigned count;
    unsigned export_count;
    uint8_t export_order[SCRIPT_FUNCTIONS];
    struct ScriptModelEffectProgram* model_effects;
    const struct ScriptPoseProgram* poses;
    const struct ScriptSoundPolicy* sound_policy;
    const struct ScriptSurfacePolicy* surface_policy;
    bool model_effect_context;
};

// Scan identifier tokens outside quoted strings/comments. These are context
// and dependency queries, not an alternate statement parser.
ScriptFunctionSet nativeScriptFunctions_References(const ScriptFunctionProgram*, const char*);
bool nativeScriptFunctions_ModelEffectContext(const char*);

// Extract declarations without moving source lines. Allocation is confined to
// the authoring/build transition; generated functions allocate nothing.
ScriptFunctionProgram* nativeScriptFunctions_Parse(char* source, size_t size,
                                                   NativeFighterScriptError* error);
bool nativeScriptFunctions_Compile(ScriptFunctionProgram* program,
                                    const ScriptStateTable* state,
                                    const NativeScriptArticleCatalog* articles,
                                    bool special_program,
                                    unsigned motion_first, unsigned motion_count,
                                    NativeFighterScriptError* error);
bool nativeScriptFunctions_Call(const ScriptCodeBlock* caller, const char* source,
                                 char* output, size_t capacity);
ScriptExpressionType nativeScriptFunctions_Result(const ScriptCodeBlock*, const char*);
int nativeScriptFunctions_Expression(const ScriptCodeBlock*, const char*,
                                     char*, size_t, const char**);
bool nativeScriptFunctions_BindCallback(ScriptCodeBlock* caller, const char* source,
                                         char* output, size_t capacity);
bool nativeScriptFunctions_CallbackPointer(const ScriptCodeBlock* caller,
                                            const char* name,
                                            char* output, size_t capacity);
bool nativeScriptFunctions_CallbackExpression(const ScriptCodeBlock* caller,
                                                const char* name,
                                                const char** source,
                                                char* output, size_t capacity);
void nativeScriptFunctions_Emit(const ScriptFunctionProgram* program, FILE* output);
void nativeScriptFunctions_EmitDefinitionExports(const ScriptFunctionProgram* program,
                                                FILE* output);

#endif
