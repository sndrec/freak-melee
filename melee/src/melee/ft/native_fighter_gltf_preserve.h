#pragma once
#include "native_fighter_gltf_skin.h"
#include "native_fighter_mesh_source.h"

// Preserve the current attachment or painted weights using the admitted source
// topology. Rigid attachments accept new topology; skinned meshes require exact
// corner correspondence. The output leaves the original project untouched.
bool nativeFighterGltf_Preserve(const NativeFighterModelEditor&,unsigned mesh,
    const cgltf_node*,const cgltf_primitive*,const NativeFighterMeshSource&,
    float scale,NativeGltfSourceSkin*,bool* rigid,char* error,size_t capacity);
