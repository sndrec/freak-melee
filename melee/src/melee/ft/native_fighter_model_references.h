#pragma once
#include "native_fighter_model_editor.h"
// Mesh and texture ordinals follow the costume's traversal order. Preserve
// references to retained objects when an editor operation changes that order.
#ifdef __cplusplus
extern "C" {
#endif
bool nativeFighterModel_RemapSelectors(const NativeFighterModelEditor* before,
                                       const NativeFighterModelEditor* after,
                                       NativeFighterProject* project);
// Fighter-data undo keeps the selectors belonging to the current costume.
bool nativeFighterModel_RestoreSelectors(NativeFighterProject* project,
                                         const void* current_fighter,
                                         size_t bytes);
#ifdef __cplusplus
}
struct NativeAssetDat;
bool nativeFighterModel_RemapMaterialAnimations(
    const NativeFighterModelEditor& before,
    const NativeFighterModelEditor& after, NativeAssetDat& costume);
#endif
