#ifndef MELEE_FT_NATIVE_FIGHTER_SKIN_MESH_H
#define MELEE_FT_NATIVE_FIGHTER_SKIN_MESH_H
#include "native_fighter_skin_edit.h"
#include "native_fighter_model_editor.h"
#include <vector>

struct NativeSkinAttribute {
    uint32_t source, attr, type, count, format, array;
    unsigned fraction, stride, width, components, display_bytes;
};
struct NativeSkinCorner {
    unsigned vertex, matrix;
    unsigned normal_index;
    uint32_t source;
    float normal[9];
};
struct NativeSkinMorph {
    uint16_t flags, shape_count;
    unsigned position_count, normal_count, normal_components;
    // Target-major vectors. Logical positions keep their own identities even
    // when their base values coincide. Vertices maps them to the mesh editor.
    std::vector<float> positions, normals;
    std::vector<unsigned> vertices;
};
struct NativeSkinPart {
    uint32_t source;
    uint16_t flags;
    std::vector<NativeSkinAttribute> attributes;
    std::vector<NativeSkinWeights> bindings;
    std::vector<NativeSkinCorner> corners;
    std::vector<unsigned> triangles;
    NativeSkinMorph morph;
};
struct NativeSkinMesh {
    std::vector<NativeSkinVertex> vertices;
    std::vector<unsigned> corner_vertices;
    std::vector<NativeSkinPart> parts;
};
// Asset-transition work only. Retains per-corner source attribute bytes for the
// writer; the source costume must remain alive throughout the transaction.
bool nativeSkinMesh_Read(const NativeFighterModelEditor&, unsigned mesh,
                         NativeSkinMesh*, char* error, size_t error_size);
bool nativeSkinMesh_ReadAttributes(const HSD_Dat&, uint32_t source,
                                    std::vector<NativeSkinAttribute>*);
bool nativeSkinMesh_InverseBind(const NativeFighterModelEditor&, unsigned joint,
                                float output[3][4]);
struct NativeAssetDat;
// Realize the identity bind used implicitly by an unbound skeleton root.
bool nativeSkinMesh_EnsureInverseBind(NativeAssetDat&,const NativeFighterModelEditor&,unsigned joint);
bool nativeSkinMesh_Write(NativeAssetDat&, const NativeFighterModelEditor&,
                          unsigned mesh, const NativeSkinMesh&);
#endif
