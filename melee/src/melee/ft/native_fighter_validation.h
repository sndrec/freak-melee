#pragma once
#include "native_fighter_project.h"
#include "native_script_ir.h"

typedef struct NativeFighterValidation {
    NativeScriptDiagnostic* entries;
    unsigned capacity, count, total, functions_checked;
    unsigned functions, instructions, parameters, imports;
    bool accepted, metadata_complete, bodies_complete, package_diagnostic;
} NativeFighterValidation;

#ifdef __cplusplus
extern "C" {
#endif
// Validate an independent candidate against the draft's owned assets and
// release it without publishing engine state or writing any draft files.
// Caller owns the diagnostic storage; count is bounded and total includes all
// discovered errors. False means infrastructure failure, not invalid source.
bool nativeFighterProject_CheckSource(const NativeFighterProject*,
    const char* source, size_t source_size, const char* parameters,
    size_t parameter_size, unsigned member, NativeFighterValidation*);
#ifdef __cplusplus
}
#endif
