#include "native_shell_workshop_materials.h"
#include "native_shell_workshop_textures.h"

#include <melee/ft/native_asset_dat.h>
#include <melee/ft/native_fighter_material_render.h>
extern "C" {
#include <sysdolphin/baselib/displayfunc.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mobj.h>
}
#include <cstdio>
#include <cstdlib>

#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/ElementFormControl.h>

namespace {
    std::string checkbox(const std::string& id, const char* label, bool on,
                         const char* hint = "")
    {
        return "<label class='workshop-material-check' title='" +
               std::string(hint) + "'><input type='checkbox' id='" + id + "'" +
               (on ? " checked" : "") + " /> " + label + "</label>";
    }
    std::string choice(const std::string& id, const char* label,
                       unsigned value, const char* const* names,
                       unsigned count)
    {
        std::string result = "<label>" + std::string(label) +
                             "</label><select class='room-input' id='" + id +
                             "'>";
        for (unsigned i = 0; i < count; ++i) {
            result += "<option value='" + std::to_string(i) + "'" +
                      (i == value ? " selected" : "") + ">" + names[i] +
                      "</option>";
        }
        return result + "</select>";
    }
    const char* const compare[] = { "Never",
                                    "Less",
                                    "Equal",
                                    "Less or equal",
                                    "Greater",
                                    "Not equal",
                                    "Greater or equal",
                                    "Always" };
    const char* const blend[] = { "Disabled", "Blend", "Logic operation",
                                  "Subtract" };
    const char* const source_factors[] = { "Zero",
                                           "One",
                                           "Destination color",
                                           "Inverse destination color",
                                           "Source alpha",
                                           "Inverse source alpha",
                                           "Destination alpha",
                                           "Inverse destination alpha" };
    const char* const destination_factors[] = { "Zero",
                                                "One",
                                                "Source color",
                                                "Inverse source color",
                                                "Source alpha",
                                                "Inverse source alpha",
                                                "Destination alpha",
                                                "Inverse destination alpha" };
    const char* const logic[] = {
        "Clear",        "And",          "Reverse and", "Copy",
        "Inverse and",  "No operation", "Xor",         "Or",
        "Nor",          "Equivalent",   "Invert",      "Reverse or",
        "Inverse copy", "Inverse or",   "Nand",        "Set"
    };
    const char* const alpha_op[] = { "And", "Or", "Xor", "Equivalent" };

}

std::string
nativeWorkshopMaterials_Controls(const NativeFighterModelEditor& model,
                                 unsigned material)
{
    if (material >= model.material_count) {
        return {};
    }
    const auto& mat = model.materials[material];
    std::string html;
    if (mat.color_offset) {
        html += "<div class='workshop-model-inspector-section'><div class='eyebrow'>LIGHTING COLORS</div>";
        const uint8_t* colors[] = {mat.ambient,mat.specular};
        for (unsigned i = 0; i < 2; ++i) {
            char text[16]; std::snprintf(text,sizeof(text),"#%02X%02X%02X%02X",colors[i][0],colors[i][1],colors[i][2],colors[i][3]);
            html += "<label>" + std::string(i ? "Specular RGBA" : "Ambient RGBA") +
                    "</label><input id='workshop-material-color-" + std::to_string(i) + "' class='room-input' type='text' value='" + text + "'/>";
        }
        html += "<label>Diffuse alpha (0-255)</label><input id='workshop-material-diffuse-alpha' class='room-input' type='text' value='" + std::to_string(mat.diffuse[3]) + "'/></div>";
    }
    html += "<div class='workshop-model-inspector-section'><div "
                       "class='eyebrow'>RENDERING</div>";
    const char* color[] = { "Engine default", "Material color",
                            "Vertex colors", "Material and vertex colors" };
    html += choice("workshop-material-color-source", "Diffuse color source",
                   mat.render_mode & 3, color, 4);
    html += choice("workshop-material-alpha-source", "Opacity source",
                   (mat.render_mode >> 13) & 3, color, 4);
    for (unsigned i = 0; i < std::size(nativeMaterialRenderFlags); ++i) {
        html += checkbox("workshop-material-flag-" + std::to_string(i),
                         nativeMaterialRenderFlags[i].label, (mat.render_mode & nativeMaterialRenderFlags[i].bit) != 0,
                         nativeMaterialRenderFlags[i].description);
    }
    for (unsigned i = 0; i < 8; ++i) {
        std::string label = "Texture layer " + std::to_string(i + 1);
        html += checkbox("workshop-material-texture-" + std::to_string(i),
                         label.c_str(),
                         (mat.render_mode & (RENDER_TEX0 << i)) != 0,
                         "Enable this texture layer when the material has "
                         "that layer attached.");
    }
    html += "</div><div class='workshop-model-inspector-section'><div "
            "class='eyebrow'>BLENDING AND DEPTH</div>";
    NativeMaterialRenderState rendering;
    if (!nativeFighterMaterial_ReadRender(model, material, rendering)) return {};
    const uint8_t* pe = rendering.pixel_engine;
    const bool present = rendering.custom_pixel_engine;
    html += checkbox(
        "workshop-material-custom-pe", "Custom blending and depth", present,
        "Override the render-mode defaults with the settings below.");
    for (unsigned i = 0; i < std::size(nativeMaterialPixelFlags); ++i) {
        html += checkbox("workshop-material-pe-flag-" + std::to_string(i),
                         nativeMaterialPixelFlags[i].label, (pe[0] & nativeMaterialPixelFlags[i].bit) != 0);
    }
    html +=
        choice("workshop-material-pe-4", "Blend operation", pe[4], blend, 4);
    html += choice("workshop-material-pe-5", "Source factor", pe[5],
                   source_factors, 8);
    html += choice("workshop-material-pe-6", "Destination factor", pe[6],
                   destination_factors, 8);
    html +=
        choice("workshop-material-pe-7", "Logic operation", pe[7], logic, 16);
    html += choice("workshop-material-pe-8", "Depth comparison", pe[8],
                   compare, 8);
    html += choice("workshop-material-pe-9", "Alpha comparison A", pe[9],
                   compare, 8);
    html += choice("workshop-material-pe-10", "Combine alpha tests", pe[10],
                   alpha_op, 4);
    html += choice("workshop-material-pe-11", "Alpha comparison B", pe[11],
                   compare, 8);
    const char* labels[] = { "Alpha reference A (0-255)",
                             "Alpha reference B (0-255)",
                             "Destination alpha (0-255)" };
    for (unsigned i = 1; i <= 3; ++i) {
        html += "<label>" + std::string(labels[i - 1]) +
                "</label><input class='room-input' type='text' "
                "id='workshop-material-pe-" +
                std::to_string(i) + "' value='" + std::to_string(pe[i]) +
                "' />";
    }
    return html + "</div>" + nativeWorkshopTextures_Controls(model,material);
}

bool nativeWorkshopMaterials_Apply(Rml::ElementDocument* document,
                                   NativeFighterModelEditor& model,
                                   unsigned material, std::string* error)
{
    if (material >= model.material_count) {
        return false;
    }
    auto checked = [&](const std::string& id) {
        auto* e = document->GetElementById(id);
        return e && e->HasAttribute("checked");
    };
    auto value = [&](const std::string& id, unsigned max, unsigned* out) {
        auto* e = dynamic_cast<Rml::ElementFormControl*>(
            document->GetElementById(id));
        if (!e) {
            return false;
        }
        std::string s = e->GetValue();
        char* end;
        long n = std::strtol(s.c_str(), &end, 10);
        if (s.empty() || *end || n < 0 || (unsigned long) n > max) {
            return false;
        }
        *out = (unsigned) n;
        return true;
    };
    NativeAssetDat dat;
    if (!dat.open(model.project->costume_dat, model.project->costume_size)) {
        return false;
    }
    const auto& mat = model.materials[material];
    if (mat.color_offset) {
        for (unsigned i = 0; i < 2; ++i) {
            auto* control = dynamic_cast<Rml::ElementFormControl*>(document->GetElementById("workshop-material-color-" + std::to_string(i)));
            const auto text = control ? control->GetValue() : std::string{};
            char* end = nullptr;
            const unsigned long rgba = text.size() == 9 && text[0] == '#' ? std::strtoul(text.c_str()+1,&end,16) : 0;
            if (!end || *end) { *error = "Enter ambient and specular colors as #RRGGBBAA."; return false; }
            dat.word(mat.color_offset + (i ? 8 : 0), (uint32_t) rgba);
        }
        unsigned alpha;
        if (!value("workshop-material-diffuse-alpha",255,&alpha)) { *error = "Diffuse alpha must be 0-255."; return false; }
        dat.data[mat.color_offset + 7] = (uint8_t) alpha;
    }
    if (!nativeWorkshopTextures_Apply(document,model,material,dat,error)) return false;
    unsigned mode = mat.render_mode;
    unsigned color, alpha;
    if (!value("workshop-material-color-source", 3, &color) ||
        !value("workshop-material-alpha-source", 3, &alpha))
    {
        return false;
    }
    mode = (mode & ~(3u | RENDER_ALPHA_BITS)) | color | (alpha << 13);
    for (unsigned i = 0; i < std::size(nativeMaterialRenderFlags); ++i) {
        mode = checked("workshop-material-flag-" + std::to_string(i))
                   ? mode | nativeMaterialRenderFlags[i].bit
                   : mode & ~nativeMaterialRenderFlags[i].bit;
    }
    for (unsigned i = 0; i < 8; ++i) {
        mode = checked("workshop-material-texture-" + std::to_string(i))
                   ? mode | (RENDER_TEX0 << i)
                   : mode & ~(RENDER_TEX0 << i);
    }
    NativeMaterialRenderState rendering;
    if (!nativeFighterMaterial_ReadRender(model, material, rendering)) return false;
    rendering.mode = mode;
    rendering.custom_pixel_engine = checked("workshop-material-custom-pe");
    auto* pe = rendering.pixel_engine;
    pe[0] &= ~127u;
    if (checked("workshop-material-custom-pe")) {
        for (unsigned i = 0; i < std::size(nativeMaterialPixelFlags); ++i) {
            if (checked("workshop-material-pe-flag-" + std::to_string(i))) {
                pe[0] |= nativeMaterialPixelFlags[i].bit;
            }
        }
        const unsigned limits[] = {
            0, 255, 255, 255, 3, 7, 7, 15, 7, 7, 3, 7
        };
        for (unsigned i = 1; i < 12; ++i) {
            unsigned n;
            if (!value("workshop-material-pe-" + std::to_string(i), limits[i],
                       &n))
            {
                *error = "Enter valid blending, depth and alpha values.";
                return false;
            }
            pe[i] = (unsigned char) n;
        }
    }
    if (!nativeFighterMaterial_StageRender(model, material, rendering, dat, *error)) return false;
    std::vector<uint8_t> result;
    if (!dat.finish(&result)) {
        return false;
    }
    auto* bytes = (uint8_t*) std::malloc(result.size());
    if (!bytes) {
        return false;
    }
    std::memcpy(bytes, result.data(), result.size());
    std::free(model.project->costume_dat);
    model.project->costume_dat = bytes;
    model.project->costume_size = result.size();
    return nativeFighterModel_Open(&model, model.project);
}
