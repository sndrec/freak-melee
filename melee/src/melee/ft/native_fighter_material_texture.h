#pragma once
#include "native_fighter_model_editor.h"
#include <string>
struct NativeAssetDat;

struct NativeMaterialTextureState {
    u32 animation_id = 0, coordinate_source = 4, flags = 0;
    float transform[9] = {0,0,0,1,1,1,0,0,0};
    float blend = 1;
    u32 wrap[2] = {}, magnification = 1;
    uint8_t repeat[2] = {1,1};
    bool lod_enabled = false, clamp_bias = false, edge_lod = false;
    u32 minification = 1, anisotropy = 0;
    float lod_bias = 0;
    bool combiner_enabled = false;
    uint8_t combine[28] = {};
    u32 active = 0;
    bool operator==(const NativeMaterialTextureState&) const = default;
};
unsigned nativeFighterMaterial_TextureLayers(const NativeFighterModelEditor&, unsigned material, u32 offsets[8]);
bool nativeFighterMaterial_ReadTexture(const NativeFighterModelEditor&, unsigned material, unsigned layer, NativeMaterialTextureState&);
bool nativeFighterMaterial_StageTexture(const NativeFighterModelEditor&, unsigned material, unsigned layer,
    const NativeMaterialTextureState&, NativeAssetDat&, std::string& error);
