#ifndef MELEE_FT_NATIVE_SOURCE_RIG_VALIDATION_H
#define MELEE_FT_NATIVE_SOURCE_RIG_VALIDATION_H

#include "native_source_assets.h"

bool nativeSourceAssets_ValidateRig(NativeSourceImportedAssets* assets,
                                    NativeScriptDiagnostic* diagnostic);

struct NativeScriptItemPrograms;
bool nativeSourceAssets_ValidateExternalParts(const NativeSourceImportedAssets* assets,
    const struct NativeScriptItemPrograms* items, NativeScriptDiagnostic* diagnostic);

#endif
