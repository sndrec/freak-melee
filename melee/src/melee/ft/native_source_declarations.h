#ifndef MELEE_FT_NATIVE_SOURCE_DECLARATIONS_H
#define MELEE_FT_NATIVE_SOURCE_DECLARATIONS_H
#include "native_source_asset_facts.h"
#include "native_script_source.h"

// Immutable source recipes apply only during candidate preparation. Roots,
// motion bounds and rig extents come from independently validated owned facts.
bool nativeSourceDeclarations_Owns(const NativeScriptSourceTree*,uint32_t node);
bool nativeSourceDeclarations_Prepare(const NativeScriptSourceTree*,
    const NativeSourceAssetFacts* input,NativeSourceAssetFacts* output,
    NativeScriptDiagnostic*);
// Revalidation requires the exact already prepared facts. No policy is ignored
// merely because its name is recognized by the source frontend.
bool nativeSourceDeclarations_Validate(const NativeScriptSourceTree*,uint32_t node,
    const NativeSourceAssetFacts*,NativeScriptDiagnostic*);
#endif
