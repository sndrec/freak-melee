#include "native_fighter_skin_mesh.h"
#include <cstring>

bool nativeSkinMesh_ReadTexture(NativeSkinTextureReader& reader, NativeSkinPart& part, NativeSkinCorner& corner,
                                 unsigned channel, unsigned selector,
                                 unsigned position_matrix,
                                 const std::vector<NativeSkinWeights>& bindings)
{
    if (channel >= 8) return false;
    if (selector == 60) {
        corner.textures[channel] = NATIVE_SKIN_TEXTURE_IDENTITY;
        return true;
    }
    if (selector < 30 || selector >= 60 || selector % 3) return false;
    const unsigned matrix = (selector - 30) / 3;
    if (matrix >= bindings.size()) return false;
    if (matrix == position_matrix) {
        corner.textures[channel] = NATIVE_SKIN_TEXTURE_FOLLOW;
        return true;
    }
    const auto& weights = bindings[matrix];
    uint64_t hash = 1469598103934665603ULL;
    for (unsigned i = 0; i < weights.count; ++i) {
        uint32_t bits;std::memcpy(&bits, &weights.weights[i], 4);
        hash = ((hash ^ weights.joints[i]) * 1099511628211ULL ^ bits) * 1099511628211ULL;
    }
    unsigned index = (unsigned)part.texture_bindings.size();
    const auto range = reader.bindings.equal_range(hash);
    for (auto entry = range.first; entry != range.second; ++entry) {
        if (nativeSkinWeights_Equal(&weights, &part.texture_bindings[entry->second])) {
            index = entry->second;break;
        }
    }
    if (index == part.texture_bindings.size()) {
        part.texture_bindings.push_back(weights);
        reader.bindings.emplace(hash, index);
    }
    corner.textures[channel] = index;
    return true;
}
