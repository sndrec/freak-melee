#pragma once
#include "native_fighter_gltf_skin.h"
#include "native_fighter_mesh_source.h"
struct NativeAssetDat;

// Prepared positions and normals share the ordinary importer's placement.
// Target deltas use its linear/normal transforms without translating offsets.
struct NativeGltfMorphImport {
    const cgltf_mesh* mesh=nullptr;
    const cgltf_primitive* primitive=nullptr;
    const cgltf_node* node=nullptr;
    const std::vector<NativeSkinWeights>* weights=nullptr;
    const NativeGltfSourceSkin* previous=nullptr;
    std::vector<float> positions,normals,normal_scale;
    float transform[16],normal_transform[9];
    float scale;
    uint32_t texcoords=0;
};
bool nativeFighterGltf_EncodeMorph(NativeAssetDat&,const NativeFighterModelEditor&,
    unsigned owner,const NativeGltfMorphImport&,uint32_t* polygon,
    NativeFighterMeshSource*,char* error,size_t error_size);
