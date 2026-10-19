#ifndef MELEE_FT_NATIVE_FIGHTER_SKIN_MORPH_H
#define MELEE_FT_NATIVE_FIGHTER_SKIN_MORPH_H
#include "native_fighter_skin_mesh.h"
#include "native_mesh_vertex_ids.h"
struct FtCostumeAssets;
struct NativeAssetDat;

struct NativeSkinChunk {
    std::vector<unsigned> corners;
    std::vector<uint8_t> slots;
    std::vector<NativeSkinWeights> bindings;
};
struct NativeSkinMorphEncoded {
    uint32_t polygon, shape, positions, normals;
    unsigned normal_components;
    std::vector<uint16_t> corner_positions, corner_normals;
};
bool nativeSkinMorph_Read(const NativeFighterModelEditor&, unsigned owner,
                           const FtCostumeAssets&, NativeSkinPart*, NativeSkinMesh*);
bool nativeSkinMorph_Encode(NativeAssetDat&, const NativeFighterModelEditor&,
                             const NativeSkinPart&, const NativeSkinMesh&,
                             NativeSkinMorphEncoded*);
// Canonical envelope domain for a rigid morph. Returns its deform/root joint.
int nativeSkinMorph_RigidDomain(const NativeFighterModelEditor&, unsigned owner,
                                 float position[3][4], float normal[3][4]);
bool nativeSkinMesh_WriteChunk(NativeAssetDat&, const NativeFighterModelEditor&,
                                const NativeSkinPart&, const NativeSkinChunk&,
                                const NativeSkinMesh&, uint32_t positions,
                                bool local_positions,
                                std::vector<NativeMeshVertexIdRow>*,
                                const NativeSkinMorphEncoded*, uint32_t* polygon);
#endif
