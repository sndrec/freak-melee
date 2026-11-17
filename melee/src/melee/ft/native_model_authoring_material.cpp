#include "native_model_authoring_internal.h"
#include "native_asset_dat.h"
extern "C" {
#include <sysdolphin/baselib/mobj.h>
}

namespace NativeModelAuthoring {
namespace {
struct PixelField {const char* name; unsigned maximum;};
const PixelField pixels[] = {{"flags", 255}, {"alpha_reference_a", 255}, {"alpha_reference_b", 255}, {"destination_alpha", 255},
    {"blend_operation", 3}, {"source_factor", 7}, {"destination_factor", 7}, {"logic_operation", 15},
    {"depth_comparison", 7}, {"alpha_comparison_a", 7}, {"alpha_test_operation", 3}, {"alpha_comparison_b", 7}};
std::string schema()
{
    const std::string color = R"({"type":"array","minItems":4,"maxItems":4,"items":{"type":"integer","minimum":0,"maximum":255}})";
    std::string fields = "\"ambient\":" + color + ",\"diffuse\":" + color + ",\"specular\":" + color +
        R"(,"alpha":{"type":"number","minimum":0,"maximum":1},"shininess":{"type":"number","minimum":0,"maximum":1000},"color_source":{"type":"integer","minimum":0,"maximum":3},"opacity_source":{"type":"integer","minimum":0,"maximum":3},"texture_layers":{"type":"array","minItems":8,"maxItems":8,"items":{"type":"boolean"}})";
    for (const auto& flag : nativeMaterialRenderFlags) fields += ",\"" + std::string(flag.name) + "\":{\"type\":\"boolean\",\"description\":\"" + flag.description + "\"}";
    fields += R"(,"pixel_engine":{"type":"object","properties":{"enabled":{"type":"boolean"})";
    for (const auto& flag : nativeMaterialPixelFlags) fields += ",\"" + std::string(flag.name) + "\":{\"type\":\"boolean\"}";
    for (unsigned i = 1; i < std::size(pixels); ++i) fields += ",\"" + std::string(pixels[i].name) + "\":{\"type\":\"integer\",\"minimum\":0,\"maximum\":" + std::to_string(pixels[i].maximum) + "}";
    fields += R"(},"additionalProperties":false})";
    return "{\"type\":\"object\",\"properties\":{" + fields + "},\"additionalProperties\":false}";
}
}
const char* material_schema() {static const auto result = schema(); return result.c_str();}
void material_properties(const NativeFighterModelEditor& model, unsigned material, Reply& r, yyjson_mut_val* out)
{
    const auto& mat = model.materials[material];
    const uint8_t* colors[] = {mat.ambient, mat.diffuse, mat.specular}; const char* names[] = {"ambient", "diffuse", "specular"};
    for (unsigned c = 0; c < 3; ++c) {auto* color = yyjson_mut_arr(r.document); yyjson_mut_obj_add_val(r.document, out, names[c], color);
        for (unsigned channel = 0; channel < 4; ++channel) yyjson_mut_arr_add_uint(r.document, color, colors[c][channel]);}
    yyjson_mut_obj_add_real(r.document, out, "alpha", mat.alpha); yyjson_mut_obj_add_real(r.document, out, "shininess", mat.shininess);
    NativeMaterialRenderState state{}; nativeFighterMaterial_ReadRender(model, material, state);
    yyjson_mut_obj_add_uint(r.document, out, "color_source", state.mode & 3); yyjson_mut_obj_add_uint(r.document, out, "opacity_source", (state.mode >> 13) & 3);
    for (const auto& flag : nativeMaterialRenderFlags) yyjson_mut_obj_add_bool(r.document, out, flag.name, (state.mode & flag.bit) != 0);
    auto* layers = yyjson_mut_arr(r.document); yyjson_mut_obj_add_val(r.document, out, "texture_layers", layers);
    for (unsigned layer = 0; layer < 8; ++layer) yyjson_mut_arr_add_bool(r.document, layers, (state.mode & (RENDER_TEX0 << layer)) != 0);
    auto* pe = yyjson_mut_obj(r.document); yyjson_mut_obj_add_val(r.document, out, "pixel_engine", pe);
    yyjson_mut_obj_add_bool(r.document, pe, "enabled", state.custom_pixel_engine);
    for (const auto& flag : nativeMaterialPixelFlags) yyjson_mut_obj_add_bool(r.document, pe, flag.name, (state.pixel_engine[0] & flag.bit) != 0);
    for (unsigned i = 1; i < 12; ++i) yyjson_mut_obj_add_uint(r.document, pe, pixels[i].name, state.pixel_engine[i]);
}
bool edit_material(NativeFighterProject& p, unsigned material, yyjson_val* properties, Reply& r)
{
    std::unique_ptr<NativeFighterModelEditor> model;
    if (!open(p, model, r)) return false;
    if (material >= model->material_count || !yyjson_is_obj(properties) || !yyjson_obj_size(properties)) {r.fail("invalid_material_edit", "Choose a material and supply its changed properties."); return false;}
    const auto before = model->materials[material];
    NativeMaterialRenderState original; nativeFighterMaterial_ReadRender(*model, material, original);
    const char* colors[] = {"ambient", "diffuse", "specular"};
    for (unsigned c = 0; c < 3; ++c) if (auto* color = yyjson_obj_get(properties, colors[c]))
        for (unsigned channel = 0; channel < 4; ++channel) if (!nativeFighterModel_SetMaterialColor(model.get(), material, c, channel, (uint8_t)yyjson_get_uint(yyjson_arr_get(color, channel)))) {
            r.fail("material_edit_rejected", "This material has no editable color record."); return false;
        }
    const char* floats[] = {"alpha", "shininess"};
    for (unsigned i = 0; i < 2; ++i) if (has(properties, floats[i]) && !nativeFighterModel_SetMaterialFloat(model.get(), material, i != 0, (float)real(properties, floats[i]))) {
        r.fail("material_edit_rejected", "The material float is unavailable or outside its native range."); return false;
    }
    NativeMaterialRenderState state; nativeFighterMaterial_ReadRender(*model, material, state);
    if (has(properties, "color_source")) state.mode = (state.mode & ~3u) | number(properties, "color_source");
    if (has(properties, "opacity_source")) state.mode = (state.mode & ~RENDER_ALPHA_BITS) | (number(properties, "opacity_source") << 13);
    for (const auto& flag : nativeMaterialRenderFlags) if (has(properties, flag.name)) state.mode = boolean(properties, flag.name) ? state.mode | flag.bit : state.mode & ~flag.bit;
    if (auto* layers = yyjson_obj_get(properties, "texture_layers")) for (unsigned i = 0; i < 8; ++i) state.mode = yyjson_get_bool(yyjson_arr_get(layers, i)) ? state.mode | (RENDER_TEX0 << i) : state.mode & ~(RENDER_TEX0 << i);
    if (auto* pe = yyjson_obj_get(properties, "pixel_engine")) {
        if (has(pe, "enabled")) state.custom_pixel_engine = boolean(pe, "enabled");
        for (const auto& flag : nativeMaterialPixelFlags) if (has(pe, flag.name)) state.pixel_engine[0] = (uint8_t)(boolean(pe, flag.name) ? state.pixel_engine[0] | flag.bit : state.pixel_engine[0] & ~flag.bit);
        for (unsigned i = 1; i < 12; ++i) if (has(pe, pixels[i].name)) state.pixel_engine[i] = (uint8_t)number(pe, pixels[i].name);
    }
    const auto& after = model->materials[material];
    if (!std::memcmp(before.ambient, after.ambient, 4) && !std::memcmp(before.diffuse, after.diffuse, 4) &&
        !std::memcmp(before.specular, after.specular, 4) && before.alpha == after.alpha && before.shininess == after.shininess &&
        state.mode == original.mode && state.custom_pixel_engine == original.custom_pixel_engine &&
        (!state.custom_pixel_engine || !std::memcmp(state.pixel_engine, original.pixel_engine, 12))) return true;
    NativeAssetDat dat; std::string error; std::vector<uint8_t> bytes;
    if (!dat.open(p.costume_dat, p.costume_size) || !nativeFighterMaterial_StageRender(*model, material, state, dat, error) || !dat.finish(&bytes)) {r.fail("material_edit_rejected", error.empty() ? "Could not encode the material rendering change." : error); return false;}
    auto* next = (uint8_t*)std::malloc(bytes.size());
    if (!next) {r.fail("allocation_failed", "Could not allocate the edited material."); return false;}
    std::memcpy(next, bytes.data(), bytes.size()); std::free(p.costume_dat); p.costume_dat = next; p.costume_size = bytes.size();
    return true;
}
}
