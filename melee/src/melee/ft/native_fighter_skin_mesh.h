#ifndef MELEE_FT_NATIVE_FIGHTER_SKIN_MESH_H
#define MELEE_FT_NATIVE_FIGHTER_SKIN_MESH_H
#include "native_fighter_skin_edit.h"
#include "native_fighter_model_editor.h"
#include <vector>
#include <string>
#include <unordered_map>

struct NativeSkinAttribute {
    uint32_t source, attr, type, count, format, array;
    unsigned fraction, stride, width, components, display_bytes;
};
enum : uint32_t {
    NATIVE_SKIN_TEXTURE_FOLLOW = UINT32_MAX,
    NATIVE_SKIN_TEXTURE_IDENTITY = UINT32_MAX - 1,
};
struct NativeSkinCorner {
    unsigned vertex, matrix;
    unsigned normal_index;
    uint32_t source;
    float normal[9];
    uint32_t textures[8] = { NATIVE_SKIN_TEXTURE_FOLLOW, NATIVE_SKIN_TEXTURE_FOLLOW,
        NATIVE_SKIN_TEXTURE_FOLLOW, NATIVE_SKIN_TEXTURE_FOLLOW, NATIVE_SKIN_TEXTURE_FOLLOW,
        NATIVE_SKIN_TEXTURE_FOLLOW, NATIVE_SKIN_TEXTURE_FOLLOW, NATIVE_SKIN_TEXTURE_FOLLOW };
};
struct NativeSkinMorph {
    uint16_t flags, shape_count;
    unsigned position_count, normal_count, normal_components;
    // Target-major vectors. Logical positions keep their own identities even
    // when their base values coincide. Vertices maps them to the mesh editor.
    std::vector<float> positions, normals;
    std::vector<unsigned> vertices;
    std::vector<float> default_blend;
    std::vector<std::string> target_names;
};
struct NativeSkinPart {
    uint32_t source;
    uint16_t flags;
    std::vector<NativeSkinAttribute> attributes;
    std::vector<NativeSkinWeights> bindings;
    // Independent texture attachments retain their original weights as the
    // surface is painted. Corner selectors distinguish them from following it.
    std::vector<NativeSkinWeights> texture_bindings;
    std::vector<NativeSkinCorner> corners;
    std::vector<unsigned> triangles;
    NativeSkinMorph morph;
    // Fresh imports own descriptor/corner bytes. Indexed non-morph arrays
    // already belong to the asset transaction. Read edits use model.dat.
    std::vector<uint8_t> attribute_bytes;
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
struct NativeSkinTextureReader {
    std::unordered_multimap<uint64_t, unsigned> bindings;
};
bool nativeSkinMesh_ReadTexture(NativeSkinTextureReader&, NativeSkinPart&, NativeSkinCorner&, unsigned channel,
                                 unsigned selector, unsigned position_matrix,
                                 const std::vector<NativeSkinWeights>& bindings);
bool nativeSkinMesh_InverseBind(const NativeFighterModelEditor&, unsigned joint,
                                float output[3][4]);
struct NativeAssetDat;
// Realize the identity bind used implicitly by an unbound skeleton root.
bool nativeSkinMesh_EnsureInverseBind(NativeAssetDat&,const NativeFighterModelEditor&,unsigned joint);
bool nativeSkinMesh_Encode(NativeAssetDat&, const NativeFighterModelEditor&,
                            const NativeSkinMesh&, uint32_t* polygon);
#endif
