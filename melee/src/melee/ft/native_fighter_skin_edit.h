#ifndef MELEE_FT_NATIVE_FIGHTER_SKIN_EDIT_H
#define MELEE_FT_NATIVE_FIGHTER_SKIN_EDIT_H
#include <melee/ft/native_fighter_project.h>
#include <melee/ft/native_fighter_skin_weights.h>

typedef struct NativeSkinVertex {
    float position[3];
    NativeSkinWeights skin;
} NativeSkinVertex;
typedef struct NativeSkinBrushPoint { float position[3]; } NativeSkinBrushPoint;

#ifdef __cplusplus
extern "C" {
#endif
// Editing identities include source position and binding, preserving UV seams.
// Corner IDs follow the original GX display-list order, before triangulation.
// Null buffers query counts; insufficient buffers are left unchanged.
unsigned nativeFighterSkin_Vertices(const NativeFighterProject*, unsigned mesh,
                                    NativeSkinVertex*, unsigned capacity,
                                    unsigned* corners, unsigned corner_capacity,
                                    unsigned* corner_count);
bool nativeFighterSkin_Paint(NativeFighterProject*, unsigned mesh, unsigned joint,
                             const NativeSkinBrushPoint*, unsigned center_count,
                             float radius, float strength, bool subtract,
                             unsigned* affected, char* error, size_t error_size);
#ifdef __cplusplus
}
#endif
#endif
