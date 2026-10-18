#ifndef MELEE_FT_NATIVE_FIGHTER_REST_MATRICES_H
#define MELEE_FT_NATIVE_FIGHTER_REST_MATRICES_H
#include "native_fighter_model_editor.h"
#include <dolphin/mtx.h>
#ifdef __cplusplus
extern "C" {
#endif
// Authoring-only HSD rest transforms, indexed by costume preorder. Optional
// accumulated_scale receives the parent-scale compensation used by HSD_MtxSRT.
// Callers provide at least model->joint_count entries in each supplied array.
bool nativeFighterModel_RestMatrices(const NativeFighterModelEditor*, Mtx* world,
                                     Vec3* accumulated_scale);
#ifdef __cplusplus
}
#endif
#endif
