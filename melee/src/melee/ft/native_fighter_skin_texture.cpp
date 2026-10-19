#include "native_fighter_skin_mesh.h"
#include <cstring>
#include <cmath>
extern "C" {
#include <dolphin/mtx.h>
}

bool nativeSkinMesh_ReadTextureBases(NativeSkinTextureReader& reader,const HSD_Dat& dat,uint32_t metadata)
{
    u16 constants;u32 bases;
    if (!HSD_DatReadU16(&dat,metadata+18,&constants) ||
        !HSD_DatReadU32(&dat,metadata+20,&bases)) return false;
    reader.constant_mask = constants;
    const char* external;
    if (bases && HSD_DatResolve(&dat,metadata+20,&bases,&external)!=HSD_DAT_REF_LOCAL) return false;
    for (unsigned slot=0;slot<10;++slot) {
        reader.bases[slot]=nativeSkinIdentityBasis;
        if (!bases) continue;
        for (unsigned i=0;i<9;++i)
            if (!HSD_DatReadF32(&dat,bases+(slot*9+i)*4,&reader.bases[slot][i]) ||
                !std::isfinite(reader.bases[slot][i])) return false;
    }
    return true;
}

bool nativeSkinMesh_TextureBasis(const float position[3][4],const float texture[3][4],
                                  bool constant,NativeSkinNormalBasis* basis)
{
    *basis = nativeSkinIdentityBasis;
    if (!constant && !std::memcmp(position,texture,sizeof(Mtx))) return true;
    Mtx inverse;
    if (!constant && !C_MTXInverse((MtxPtr)texture,inverse)) return false;
    // n_new = C_position^-T n_old. Postmultiply the canonical texture normal
    // matrix by C_texture^-T C_position^T to recover its original input space.
    for (unsigned r=0;r<3;++r) for (unsigned c=0;c<3;++c) {
        float value=0;
        for (unsigned k=0;k<3;++k)
            value += (constant ? (r==k?1.0f:0.0f) : inverse[k][r])*position[c][k];
        if (!std::isfinite(value)) return false;
        (*basis)[r*3+c] = value;
    }
    return true;
}

bool nativeSkinMesh_ReadTexture(NativeSkinTextureReader& reader, NativeSkinPart& part, NativeSkinCorner& corner,
                                 unsigned channel, unsigned selector,
                                 unsigned position_matrix,
                                 const std::vector<NativeSkinWeights>& bindings,
                                 const uint8_t* followers,
                                 const NativeSkinNormalBasis* input_basis)
{
    if (channel >= 8) return false;
    const bool direct = selector == 60;
    const unsigned matrix = direct ? 0 : (selector-30)/3;
    NativeSkinNormalBasis basis = input_basis ? *input_basis :
        followers && !direct ? reader.bases[matrix < 10 ? matrix : 0] : nativeSkinIdentityBasis;
    if (selector == 60) {
        if (basis == nativeSkinIdentityBasis) {
        corner.textures[channel] = NATIVE_SKIN_TEXTURE_IDENTITY;
        return true;
        }
    }
    if (!direct && (selector < 30 || selector >= 60 || selector % 3 || matrix >= bindings.size())) return false;
    const bool constant = direct || (followers && (reader.constant_mask & (1U<<matrix)));
    if (!constant && (followers ? followers[matrix] != 255 : matrix == position_matrix)) {
        if (followers && followers[matrix] != position_matrix) return false;
        if (basis != nativeSkinIdentityBasis) return false;
        corner.textures[channel] = NATIVE_SKIN_TEXTURE_FOLLOW;
        return true;
    }
    const NativeSkinWeights weights = constant ? NativeSkinWeights{} : bindings[matrix];
    uint64_t hash = 1469598103934665603ULL;
    for (unsigned i = 0; i < weights.count; ++i) {
        uint32_t bits;std::memcpy(&bits, &weights.weights[i], 4);
        hash = ((hash ^ weights.joints[i]) * 1099511628211ULL ^ bits) * 1099511628211ULL;
    }
    for (float value : basis) {
        uint32_t bits;std::memcpy(&bits,&value,4);
        hash = (hash ^ bits)*1099511628211ULL;
    }
    unsigned index = (unsigned)part.texture_bindings.size();
    const auto range = reader.bindings.equal_range(hash);
    for (auto entry = range.first; entry != range.second; ++entry) {
        if (nativeSkinWeights_Equal(&weights, &part.texture_bindings[entry->second]) &&
            basis == part.texture_bases[entry->second]) {
            index = entry->second;break;
        }
    }
    if (index == part.texture_bindings.size()) {
        part.texture_bindings.push_back(weights);
        part.texture_bases.push_back(basis);
        reader.bindings.emplace(hash, index);
    }
    corner.textures[channel] = index;
    return true;
}
