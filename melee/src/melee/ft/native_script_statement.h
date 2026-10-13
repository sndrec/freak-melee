#ifndef MELEE_FT_NATIVE_SCRIPT_STATEMENT_H
#define MELEE_FT_NATIVE_SCRIPT_STATEMENT_H

#include "native_script_source.h"
#include "native_script_expression.h"

typedef struct NativeScriptLocal {
    NativeScriptSourceSpan name;
    uint32_t scope;
    NativeScriptIRType type;
} NativeScriptLocal;

typedef enum NativeScriptDefaultReturnKind {
    NativeScriptDefaultReturn_Reject,
    NativeScriptDefaultReturn_Constant,
    NativeScriptDefaultReturn_Parameter,
} NativeScriptDefaultReturnKind;

// Engine event contracts may define an implicit return, unlike typed helpers.
// Constants retain typed scalar bits; Parameter names an incoming event slot.
// The bytecode emitter makes this explicit code rather than a VM fallback.
typedef struct NativeScriptDefaultReturn {
    uint64_t bits;
    uint32_t argument;
    uint8_t kind, type;
} NativeScriptDefaultReturn;

enum {
    NativeScriptBodyPolicy_None=0,
    NativeScriptBodyPolicy_ReadOnly=1u<<0,
    NativeScriptBodyPolicy_StrictReturn=1u<<1,
    NativeScriptBodyPolicy_TruthReturn=1u<<2,
    NativeScriptBodyPolicy_All=7,
};

typedef struct NativeScriptStatementCompiler {
    NativeScriptSourceTree* tree;
    NativeScriptIRArena* expressions;
    NativeScriptIRResolver bindings;
    void* binding_context;
    NativeScriptLocal* locals;
    uint32_t local_count, local_capacity;
    NativeScriptIRType return_type;
    NativeScriptDefaultReturn default_return;
    uint8_t body_policy;
} NativeScriptStatementCompiler;

// Locals use semantic IDs in this explicit domain; native binding IDs occupy the
// lower range. Names and source remain immutable for the program's lifetime.
#define NATIVE_SCRIPT_LOCAL_SEMANTIC UINT32_C(0x80000000)
bool nativeScriptStatement_Compile(NativeScriptStatementCompiler*,
    uint32_t function_or_callback, NativeScriptDiagnostic*);

#endif
