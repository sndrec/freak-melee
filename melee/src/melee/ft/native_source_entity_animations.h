#ifndef MELEE_FT_NATIVE_SOURCE_ENTITY_ANIMATIONS_H
#define MELEE_FT_NATIVE_SOURCE_ENTITY_ANIMATIONS_H
#include "native_source_entity_assets.h"
// Exact named immutable bundles, prepared after entity/root owner assignment.
// Descriptor and compressed-stream pointers retain their character/blueprint
// arena and copied DAT. No HSD runtime objects or public compiler admission.
bool nativeSourceEntityAnimations_Prepare(NativeSourceEntityAssets*,
    NativeSourceImportedAssets*,const NativeScriptSourceTree*,NativeScriptDiagnostic*);
#endif
