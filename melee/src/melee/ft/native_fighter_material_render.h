#pragma once
#include "native_fighter_model_editor.h"
#include <string>
struct NativeAssetDat;

struct NativeMaterialRenderState {
    uint32_t mode;
    bool custom_pixel_engine;
    uint8_t pixel_engine[12];
};
struct NativeMaterialRenderFlag {
    uint32_t bit;
    const char* name;
    const char* label;
    const char* description;
};
extern const NativeMaterialRenderFlag nativeMaterialRenderFlags[7];
extern const NativeMaterialRenderFlag nativeMaterialPixelFlags[7];
bool nativeFighterMaterial_ReadRender(const NativeFighterModelEditor&, unsigned material, NativeMaterialRenderState&);
// Stage a rendering change alongside other material/texture fields. The caller
// finishes and publishes its DAT transaction after all fields are valid.
bool nativeFighterMaterial_StageRender(const NativeFighterModelEditor&, unsigned material,
    const NativeMaterialRenderState&, NativeAssetDat&, std::string& error);
