#ifndef HSD_NATIVE_MODEL_VIEW_H
#define HSD_NATIVE_MODEL_VIEW_H
#include <sysdolphin/baselib/forward.h>

#include <dolphin/mtx.h>
#ifdef __cplusplus
extern "C" {
#endif
// Transient authoring state. Never modifies costume materials or mesh flags.
void nativeModelView_Clear(void);
void nativeModelView_Begin(unsigned count, bool override_visibility);
bool nativeModelView_OverridesVisibility(const HSD_JObj*);
u32 nativeModelView_JointFlags(const HSD_JObj*);
u32 nativeModelView_MeshFlags(const HSD_DObj*);
void nativeModelView_Mesh(unsigned index, HSD_JObj* owner, HSD_DObj* mesh,
                          bool hidden, bool highlight, HSD_JObj* weight_bone);
bool nativeModelView_Draw(HSD_DObj* mesh, Mtx view);
int nativeModelView_Pick(const float origin[3], const float direction[3]);
// Current world positions keyed by the display list's position-array indices.
// Caller-owned buffers; present marks referenced vertices. Direct or ambiguous
// position layouts return false instead of inventing a correspondence.
bool nativeModelView_VertexPositions(unsigned mesh, Vec3* positions,
                                     u8* present, unsigned capacity);
#ifdef __cplusplus
}
#endif
#endif
