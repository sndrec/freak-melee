#ifndef MELEE_FT_NATIVE_SOURCE_ENTITY_MODELS_H
#define MELEE_FT_NATIVE_SOURCE_ENTITY_MODELS_H
#include "native_source_entity_assets.h"
// Preparation-only exact declaration/table construction. No heap/class/material
// initialization or graph loading. Returned descriptors borrow pinned arenas.
bool nativeSourceEntityModels_Prepare(NativeSourceEntityAssets*,
    NativeSourceImportedAssets*,const NativeScriptSourceTree*,NativeScriptDiagnostic*);
#endif
