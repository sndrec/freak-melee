#include "native_fighter_material_render.h"
#include "native_asset_dat.h"
extern "C" {
#include <sysdolphin/baselib/displayfunc.h>
#include <sysdolphin/baselib/mobj.h>
}
#include <cstring>

const NativeMaterialRenderFlag nativeMaterialRenderFlags[7] = {
    {RENDER_DIFFUSE, "diffuse_lighting", "Diffuse lighting", "Apply scene lights to the diffuse surface."},
    {RENDER_SPECULAR, "specular_highlights", "Specular highlights", "Enable highlights using specular tint and shininess."},
    {RENDER_TOON, "toon_shading", "Toon shading", "Use the engine's toon shading path."},
    {RENDER_SHADOW, "shadow_rendering", "Shadow rendering", "Use the shadow render path."},
    {RENDER_ZMODE_ALWAYS, "always_pass_depth", "Always pass depth test", "Draw regardless of existing depth."},
    {RENDER_NO_ZUPDATE, "disable_depth_writes", "Disable depth writes", "Leave depth unchanged for subsequently drawn geometry."},
    {RENDER_XLU, "translucent", "Translucent surface", "Classify this material for transparent rendering."},
};
const NativeMaterialRenderFlag nativeMaterialPixelFlags[7] = {
    {1, "write_color", "Write color", ""}, {2, "write_alpha", "Write alpha", ""},
    {4, "override_destination_alpha", "Override destination alpha", ""},
    {8, "depth_before_alpha", "Depth test before alpha test", ""},
    {16, "depth_test", "Enable depth test", ""}, {32, "write_depth", "Write depth", ""},
    {64, "dither", "Dither", ""},
};
bool nativeFighterMaterial_ReadRender(const NativeFighterModelEditor& model, unsigned material, NativeMaterialRenderState& state)
{
    if (material >= model.material_count) return false;
    const auto& mat = model.materials[material];
    state.mode = mat.render_mode;
    u32 target; const char* external;
    state.custom_pixel_engine = HSD_DatResolve(&model.dat, mat.offset + 20, &target, &external) == HSD_DAT_REF_LOCAL && HSD_DatBytes(&model.dat, target, 12);
    if (state.custom_pixel_engine) std::memcpy(state.pixel_engine, HSD_DatBytes(&model.dat, target, 12), 12);
    else {
        const bool blend = (mat.render_mode & RENDER_XLU) != 0;
        const bool write = (mat.render_mode & RENDER_NO_ZUPDATE) == 0;
        const uint8_t defaults[12] = {
            (uint8_t)(1 | 16 | (write ? 32 : 0) | (!write || !blend ? 8 : 0)), 0, 0, 0,
            (uint8_t)(blend ? 1 : 0), 4, 5, 15,
            (uint8_t)(mat.render_mode & RENDER_ZMODE_ALWAYS ? 7 : 3),
            (uint8_t)(write && blend ? 4 : 7), 0, (uint8_t)(write && blend ? 4 : 7)};
        std::memcpy(state.pixel_engine, defaults, 12);
    }
    return true;
}
bool nativeFighterMaterial_StageRender(const NativeFighterModelEditor& model, unsigned material,
    const NativeMaterialRenderState& state, NativeAssetDat& dat, std::string& error)
{
    if (material >= model.material_count) {error = "Choose an existing material."; return false;}
    const auto& mat = model.materials[material];
    const uint32_t mode = state.mode;
    for (unsigned mesh = 0; mesh < model.mesh_count; ++mesh)
        if (model.meshes[mesh].material == material &&
            !nativeFighterModel_MaterialInputs(&model, mesh, material, mode)) {
            error = "A mesh using this material lacks its required UV coordinates. Import UVs or change the texture mapping before enabling the layer."; return false;
        }
    if ((mode & RENDER_NO_ZUPDATE) && !(mode & RENDER_XLU)) {
        error = "The no-depth-write render flag requires a translucent surface. For an opaque surface, use Custom blending and depth and uncheck Write depth."; return false;
    }
    dat.word(mat.offset + 4, mode);
    // Both material and owning joints select the native draw passes.
    if ((mode ^ mat.render_mode) & RENDER_BLENDING) {
        u32 flags[NATIVE_MODEL_MAX_JOINTS];
        nativeFighterModel_MaterialJointFlags(&model, material, mode, flags);
        for (unsigned j = 0; j < model.joint_count; ++j) if (flags[j] != model.joints[j].flags) dat.word(model.joints[j].offset + 4, flags[j]);
    }
    if (state.custom_pixel_engine) {
        const unsigned limits[] = {255, 255, 255, 255, 3, 7, 7, 15, 7, 7, 3, 7};
        for (unsigned i = 0; i < 12; ++i) if (state.pixel_engine[i] > limits[i]) {error = "Enter valid blending, depth and alpha values."; return false;}
        u32 target; const char* external;
        if (HSD_DatResolve(&model.dat, mat.offset + 20, &target, &external) != HSD_DAT_REF_LOCAL || !HSD_DatBytes(&model.dat, target, 12)) target = dat.allocate(12);
        if (target == UINT32_MAX) {error = "Could not allocate pixel-engine settings."; return false;}
        std::memcpy(dat.data.data() + target, state.pixel_engine, 12);
        dat.link(mat.offset + 20, target);
    } else dat.link(mat.offset + 20, 0);
    return true;
}
