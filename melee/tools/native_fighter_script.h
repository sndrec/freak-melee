#ifndef NATIVE_FIGHTER_SCRIPT_H
#define NATIVE_FIGHTER_SCRIPT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/native_fighter_behavior_source.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NativeFighterScriptError {
    unsigned line;
    char message[128];
} NativeFighterScriptError;

typedef enum ScriptStateType {
    ScriptState_Bool,
    ScriptState_Float,
    ScriptState_Int,
} ScriptStateType;

typedef struct ScriptStateField {
    char name[32];
    ScriptStateType type;
    uint16_t offset;
    uint16_t count;
    bool array;
} ScriptStateField;

enum { SCRIPT_STATE_FIELDS = 64 };

typedef struct ScriptStateTable {
    ScriptStateField fields[SCRIPT_STATE_FIELDS];
    unsigned count;
    unsigned bytes;
    unsigned bytes_limit;
    bool has_float;
    bool has_int;
    struct {
        char name[32];
        uint16_t offset;
        bool is_int;
        bool is_bool;
    } attributes[NATIVE_FIGHTER_PARAMETER_CAPACITY];
    unsigned attribute_count;
    const NativeFighterParameters* parameters;
} ScriptStateTable;

// Test-only oracle construction. Ordinary source packages own these facts in
// their validated FMA/DAT; they are never inferred from source declarations.
typedef struct NativeFighterScriptConstruction {
    const char* fighter_root;
    uint32_t motion_first,motion_count;
} NativeFighterScriptConstruction;

// One named block for each special move. Any paired ground/air entry can own
// its animation, interrupt, physics and collision callbacks. Every special
// supports an authored accessory callback; neutral also has a throw-flag cue.
// Entry/cue speed edits
// and explicit stock inheritance remain available for partial moves. A move
// that authors phase callbacks must provide every ground and air phase.
// `state bool/float/int Name` declarations before the moves
// reserve fighter-local bytes in the rollback-tracked module state. Interrupt
// phases can read and assign them through `state.Name`. Stock-clone scripts
// can append item declarations and non-special action/lifecycle callbacks.
// Typed `func` helpers share fighter behavior across callbacks without an
// interpreter. Helpers use the same state and parameters; recursion is rejected.
// The parser checks the complete source before any C is written.
bool nativeFighterScript_GenerateC(const char* source, size_t source_size,
                                   const char* code_key, const char* asset_root,
                                   const NativeFighterParameters* parameters,
                                   const NativeFighterScriptConstruction* construction,
                                   FILE* output,
                                   NativeFighterScriptError* error);
// Identifies the exact authored bytes used for a compiled script module.
uint64_t nativeFighterScript_SourceHash(const void* source, size_t source_size);

#ifdef __cplusplus
}
#endif

#endif
