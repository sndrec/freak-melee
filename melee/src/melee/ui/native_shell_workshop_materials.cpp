#include "native_shell_workshop_materials.h"
#include "native_shell_workshop_textures.h"

#include <melee/ft/native_asset_dat.h>
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
    struct Flag {
        unsigned bit;
        const char* name;
        const char* hint;
    };
    const Flag flags[] = {
        { RENDER_DIFFUSE, "Diffuse lighting",
          "Apply scene lights to the diffuse surface." },
        { RENDER_SPECULAR, "Specular highlights",
          "Enable highlights using specular tint and shininess." },
        { RENDER_TOON, "Toon shading", "Use the engine's toon shading path." },
        { RENDER_SHADOW, "Shadow rendering", "Use the shadow render path." },
        { RENDER_ZMODE_ALWAYS, "Always pass depth test",
          "Draw regardless of existing depth." },
        { RENDER_NO_ZUPDATE, "Disable depth writes",
          "Do not occlude subsequently drawn geometry." },
        { RENDER_XLU, "Translucent surface",
          "Classify this material for transparent rendering." },
    };
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
    void read_pe(const NativeFighterModelEditor& model,
                 const NativeModelMaterial& mat, unsigned char pe[12],
                 bool* present)
    {
        u32 target;
        const char* external;
        *present = HSD_DatResolve(&model.dat, mat.offset + 20, &target,
                                  &external) == HSD_DAT_REF_LOCAL &&
                   HSD_DatBytes(&model.dat, target, 12) != nullptr;
        if (*present) {
            std::memcpy(pe, HSD_DatBytes(&model.dat, target, 12), 12);
        } else {
            const bool blend = (mat.render_mode & RENDER_XLU) != 0;
            const bool write = (mat.render_mode & RENDER_NO_ZUPDATE) == 0;
            const unsigned char defaults[12] = {
                (unsigned char) (1 | 16 | (write ? 32 : 0) |
                                 (!write || !blend ? 8 : 0)),
                0,
                0,
                0,
                (unsigned char) (blend ? 1 : 0),
                4,
                5,
                15,
                (unsigned char) (mat.render_mode & RENDER_ZMODE_ALWAYS ? 7
                                                                       : 3),
                (unsigned char) (write && blend ? 4 : 7),
                0,
                (unsigned char) (write && blend ? 4 : 7)
            };
            std::memcpy(pe, defaults, 12);
        }
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
    const Flag pe_flags[] = { { 1, "Write color", "" },
                              { 2, "Write alpha", "" },
                              { 4, "Override destination alpha", "" },
                              { 8, "Depth test before alpha test", "" },
                              { 16, "Enable depth test", "" },
                              { 32, "Write depth", "" },
                              { 64, "Dither", "" } };
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
    for (unsigned i = 0; i < std::size(flags); ++i) {
        html += checkbox("workshop-material-flag-" + std::to_string(i),
                         flags[i].name, (mat.render_mode & flags[i].bit) != 0,
                         flags[i].hint);
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
    unsigned char pe[12];
    bool present;
    read_pe(model, mat, pe, &present);
    html += checkbox(
        "workshop-material-custom-pe", "Custom blending and depth", present,
        "Override the render-mode defaults with the settings below.");
    for (unsigned i = 0; i < std::size(pe_flags); ++i) {
        html += checkbox("workshop-material-pe-flag-" + std::to_string(i),
                         pe_flags[i].name, (pe[0] & pe_flags[i].bit) != 0);
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
    return html + "<button id='workshop-material-apply-render' "
                  "class='room-button primary'>Apply material</button></div>" + nativeWorkshopTextures_Controls(model,material);
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
    for (unsigned i = 0; i < std::size(flags); ++i) {
        mode = checked("workshop-material-flag-" + std::to_string(i))
                   ? mode | flags[i].bit
                   : mode & ~flags[i].bit;
    }
    for (unsigned i = 0; i < 8; ++i) {
        mode = checked("workshop-material-texture-" + std::to_string(i))
                   ? mode | (RENDER_TEX0 << i)
                   : mode & ~(RENDER_TEX0 << i);
    }
    if ((mode & RENDER_NO_ZUPDATE) && !(mode & RENDER_XLU)) {
        *error = "The no-depth-write render flag requires a translucent "
                 "surface. For an opaque surface, use Custom blending and "
                 "depth and uncheck Write depth.";
        return false;
    }
    dat.word(mat.offset + 4, mode);
    // Melee routes draw calls using the owning joints' transparency masks as
    // well as the material. Keep both in agreement when changing surface type.
    if ((mode ^ mat.render_mode) & RENDER_BLENDING) {
        bool affected[NATIVE_MODEL_MAX_JOINTS] = {};
        unsigned passes[NATIVE_MODEL_MAX_JOINTS] = {};
        for (unsigned m = 0; m < model.mesh_count; ++m) {
            const auto& mesh = model.meshes[m];
            if (mesh.material == material) {
                affected[mesh.joint] = true;
            }
            if (mesh.material >= model.material_count) {
                continue;
            }
            unsigned render = mesh.material == material
                                  ? mode
                                  : model.materials[mesh.material].render_mode;
            passes[mesh.joint] |=
                (render & RENDER_XLU)
                    ? ((render & RENDER_NO_ZUPDATE) ? JOBJ_TEXEDGE : JOBJ_XLU)
                    : JOBJ_OPA;
        }
        // Do not enable unrelated, deliberately dormant joint geometry.
        // Only owners of the edited material change their local draw passes.
        unsigned flags[NATIVE_MODEL_MAX_JOINTS];
        for (unsigned j = 0; j < model.joint_count; ++j) {
            flags[j] = model.joints[j].flags;
            if (affected[j]) {
                flags[j] = (flags[j] & ~(JOBJ_OPA | JOBJ_XLU | JOBJ_TEXEDGE)) |
                           passes[j];
            }
        }
        for (unsigned j = 0; j < model.joint_count; ++j) {
            if (!affected[j]) {
                continue;
            }
            for (int parent = model.joints[j].parent; parent >= 0;
                 parent = model.joints[parent].parent)
            {
                flags[parent] |= passes[j] << 10;
            }
        }
        for (unsigned j = 0; j < model.joint_count; ++j) {
            if (flags[j] != model.joints[j].flags) {
                dat.word(model.joints[j].offset + 4, flags[j]);
            }
        }
    }
    unsigned char pe[12] = {};
    if (checked("workshop-material-custom-pe")) {
        for (unsigned i = 0; i < std::size(pe_flags); ++i) {
            if (checked("workshop-material-pe-flag-" + std::to_string(i))) {
                pe[0] |= pe_flags[i].bit;
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
        u32 target;
        const char* external;
        if (HSD_DatResolve(&model.dat, mat.offset + 20, &target, &external) !=
                HSD_DAT_REF_LOCAL ||
            !HSD_DatBytes(&model.dat, target, 12))
        {
            target = dat.allocate(12);
        }
        if (target == UINT32_MAX) {
            return false;
        }
        std::memcpy(dat.data.data() + target, pe, 12);
        dat.link(mat.offset + 20, target);
    } else {
        dat.link(mat.offset + 20, 0);
    }
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
