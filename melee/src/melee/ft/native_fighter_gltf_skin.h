#pragma once

#include "native_fighter_model_editor.h"
#include "native_fighter_skin_weights.h"
#include "../../../extern/cgltf/cgltf.h"
#include <vector>
#include <string>

struct NativeGltfSourceSkin {
    std::vector<NativeSkinWeights> vertices;
    float bind_shape[16]{}; // Column-major; translation is in Melee units.
    unsigned morph_count=0;
    std::vector<std::string> morph_order;
};

// Match source names/roles and hierarchy to the existing rig. All weighted
// joints must agree on one bind-shape transform; no target joints are changed.
bool nativeFighterGltf_ReadSkin(const NativeFighterModelEditor& model,
    unsigned owner, const cgltf_node* node, const cgltf_primitive* primitive,
    float scale, NativeGltfSourceSkin* output, char* error, size_t error_size);
