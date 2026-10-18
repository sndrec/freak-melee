#ifndef MELEE_FT_NATIVE_SCRIPT_RESOURCE_METADATA_H
#define MELEE_FT_NATIVE_SCRIPT_RESOURCE_METADATA_H

#include "native_script_source.h"
#include <melee/it/native_article_resource.h>

struct NativeSourceAssetFacts;
struct NativeFighterRig;
enum { NATIVE_SCRIPT_RESOURCE_CAPACITY_MIN=2, NATIVE_SCRIPT_RESOURCE_CAPACITY_MAX=NATIVE_ARTICLE_RESOURCE_NODES };
typedef struct NativeScriptResourceRecipe {
    uint16_t capacity;
    int16_t reserved_part;
} NativeScriptResourceRecipe;

// Transition-only exact owner recipe. Missing declaration yields {0,-1}.
// False preserves output; no runtime collection or native allocation occurs.
bool nativeScriptResourceMetadata_Prepare(const NativeScriptSourceTree*,uint32_t owner,
    const struct NativeSourceAssetFacts*,const struct NativeFighterRig*,
    NativeScriptResourceRecipe*,NativeScriptDiagnostic*);

#endif
