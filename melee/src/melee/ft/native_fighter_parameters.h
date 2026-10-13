#ifndef NATIVE_FIGHTER_PARAMETERS_H
#define NATIVE_FIGHTER_PARAMETERS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    NATIVE_FIGHTER_PARAMETER_CAPACITY = 256,
    // Each accepted source line occupies fewer than 256 bytes, including LF.
    NATIVE_FIGHTER_PARAMETER_TEXT_CAPACITY = NATIVE_FIGHTER_PARAMETER_CAPACITY * 256,
};

typedef enum NativeFighterParameterType {
    NativeFighterParameter_Float,
    NativeFighterParameter_Int,
    NativeFighterParameter_Bool,
} NativeFighterParameterType;

typedef struct NativeFighterParameter {
    char name[128];
    NativeFighterParameterType type;
    // IEEE-754 float bits, signed integer bits, or zero/one for bool.
    uint32_t bits;
} NativeFighterParameter;

typedef struct NativeFighterParameters {
    NativeFighterParameter fields[NATIVE_FIGHTER_PARAMETER_CAPACITY];
    uint16_t count;
    uint64_t schema_hash;
} NativeFighterParameters;

// Project-owned, match-immutable parameters. The schema hash ignores values,
// so editing a value preserves the declaration schema.
bool nativeFighterParameters_Parse(const char* source, size_t size,
                                   NativeFighterParameters* parameters,
                                   unsigned* error_line);
// Convert stock-translation declarations into owned values copied from the
// source DAT. The outputs are caller-owned; ordinary move text is unchanged.
// The script retains typed `parameter Name = LITERAL` declarations in their
// original scope, so it is self-contained and can be indexed by the interpreter.
// The sidecar carries matching editable overrides; no DAT offset remains.
// Consumes fighter and article `attribute float/int/bool/byte Name OFFSET`
// drafts. byte copies an unsigned source byte into an owned integer; it is
// import-only, never a live alias into the donor DAT.
// Article values use qualified names (It_Kind_Name.parameter). Unavailable
// stock-family article blocks in a shared import template are omitted.
// bool reads one source byte; float/int read an aligned big-endian word.
bool nativeFighterParameters_Import(const void* fighter_dat, size_t dat_size,
                                   const char* fighter_root, unsigned source_kind,
                                   const char* script, size_t script_size,
                                   char** owned_script, size_t* owned_size,
                                   char** parameter_text,
                                   size_t* parameter_size,
                                   unsigned* error_line);

#ifdef __cplusplus
}
#endif

#endif
