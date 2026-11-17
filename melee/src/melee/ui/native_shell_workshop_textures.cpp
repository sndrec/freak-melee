#include "native_shell_workshop_textures.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

#include <melee/ft/native_asset_dat.h>
#include <melee/ft/native_fighter_material_texture.h>
#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/ElementFormControl.h>

namespace {
    struct Choice {
        unsigned value;
        const char* name;
    };
    const char* const wraps[] = { "Clamp", "Repeat", "Mirror" };
    const char* const filters[] = { "Nearest",
                                    "Linear",
                                    "Nearest / nearest mip",
                                    "Linear / nearest mip",
                                    "Nearest / linear mip",
                                    "Linear / linear mip" };
    const char* const coords[] = {
        "UV",   "Reflection", "Specular highlight", "Shadow",
        "Toon", "Gradation",  "Backlight"
    };
    const char* const color_maps[] = {
        "Disabled", "Alpha mask",   "RGB mask", "Blend",   "Multiply",
        "Replace",  "Pass through", "Add",      "Subtract"
    };
    const char* const alpha_maps[] = { "Disabled", "Alpha mask",
                                       "Blend",    "Multiply",
                                       "Replace",  "Pass through",
                                       "Add",      "Subtract" };
    const char* const sources[] = {
        "Position",       "Normal",         "Binormal",       "Tangent",
        "UV 0",           "UV 1",           "UV 2",           "UV 3",
        "UV 4",           "UV 5",           "UV 6",           "UV 7",
        "Generated UV 0", "Generated UV 1", "Generated UV 2", "Generated UV 3",
        "Generated UV 4", "Generated UV 5", "Generated UV 6", "Vertex color 0",
        "Vertex color 1"
    };
    const Choice operations[] = { { 0, "Add" },
                                  { 1, "Subtract" },
                                  { 8, "Compare R8 greater" },
                                  { 9, "Compare R8 equal" },
                                  { 10, "Compare GR16 greater" },
                                  { 11, "Compare GR16 equal" },
                                  { 12, "Compare BGR24 greater" },
                                  { 13, "Compare BGR24 equal" },
                                  { 14, "Compare RGB8 / A8 greater" },
                                  { 15, "Compare RGB8 / A8 equal" } };
    const Choice color_inputs[] = {
        {8,"Texture RGB"}, {9,"Texture alpha"}, {12,"One"}, {13,"Half"}, {15,"Zero"},
        {128,"Constant RGB"}, {129,"Constant R"}, {130,"Constant G"}, {131,"Constant B"}, {132,"Constant A"},
        {133,"Texture register 0 RGB"}, {134,"Texture register 0 alpha"}, {135,"Texture register 1 RGB"}, {136,"Texture register 1 alpha"}
    };
    const Choice alpha_inputs[] = {
        {4,"Texture alpha"}, {7,"Zero"}, {64,"Constant R"}, {65,"Constant G"}, {66,"Constant B"}, {67,"Constant A"},
        {68,"Texture register 0 alpha"}, {69,"Texture register 1 alpha"}
    };
    std::string input(const std::string& id, const std::string& label,
                      const std::string& value)
    {
        return "<label>" + label + "</label><input id='" + id +
               "' class='room-input' type='text' value='" + value + "'/>";
    }
    std::string numeric(const std::string& id, const char* label, float value)
    {
        char text[32];
        std::snprintf(text, sizeof(text), "%.9g", value);
        return input(id, label, text);
    }
    std::string check(const std::string& id, const char* label, bool value)
    {
        return "<label class='workshop-material-check'><input id='" + id +
               "' type='checkbox'" + (value ? " checked" : "") + "/> " +
               label + "</label>";
    }
    std::string choices(const std::string& id, const char* label,
                        unsigned value, const Choice* entries, unsigned count)
    {
        std::string s = "<label>" + std::string(label) +
                        "</label><select id='" + id + "' class='room-input'>";
        bool found = false;
        for (unsigned i = 0; i < count; ++i) {
            const bool selected = value == entries[i].value;
            found |= selected;
            s += "<option value='" + std::to_string(entries[i].value) + "'" +
                 (selected ? " selected" : "") + ">" + entries[i].name +
                 "</option>";
        }
        if (!found) {
            s += "<option value='" + std::to_string(value) +
                 "' selected>Stored value " + std::to_string(value) +
                 "</option>";
        }
        return s + "</select>";
    }
    std::string choice(const std::string& id, const char* label,
                       unsigned value, const char* const* names,
                       unsigned count, bool disabled = false)
    {
        Choice entries[32];
        for (unsigned i = 0; i < count; ++i) {
            entries[i] = { i, names[i] };
        }
        auto markup = choices(id, label, value, entries, count);
        if (disabled) markup.insert(markup.find("<select ") + 8, "disabled title='Animated costumes use texture IDs tied to their render slots. Replace images to change their appearance.' ");
        return markup;
    }
    std::string color(const std::string& id, const char* label,
                      const uint8_t* rgba)
    {
        char text[16];
        std::snprintf(text, sizeof(text), "#%02X%02X%02X%02X", rgba[0],
                      rgba[1], rgba[2], rgba[3]);
        return input(id, label, text);
    }
    u32 pointer(const HSD_Dat& dat, u32 slot)
    {
        u32 target = 0;
        const char* external;
        return HSD_DatResolve(&dat, slot, &target, &external) ==
                       HSD_DAT_REF_LOCAL
                   ? target
                   : 0;
    }
    const unsigned light_bits[] = { 16, 32, 64, 128, 256, 1u << 24 };
    const char* const light_names[] = {
        "Diffuse contribution", "Specular contribution",
        "Ambient contribution", "Extended contribution",
        "Shadow contribution",  "Bump mapping"
    };
}

std::string
nativeWorkshopTextures_Controls(const NativeFighterModelEditor& model,
                                unsigned material)
{
    std::string html;
    u32 offsets[8];
    const unsigned count = nativeFighterMaterial_TextureLayers(model, material, offsets);
    for (unsigned layer = 0; layer < count; ++layer) {
        const u32 at = offsets[layer];
        const auto& dat = model.dat;
        u32 id, src, flags, ws, wt, mag;
        float blend;
        HSD_DatReadU32(&dat, at + 8, &id);
        HSD_DatReadU32(&dat, at + 12, &src);
        HSD_DatReadU32(&dat, at + 64, &flags);
        HSD_DatReadU32(&dat, at + 52, &ws);
        HSD_DatReadU32(&dat, at + 56, &wt);
        HSD_DatReadU32(&dat, at + 72, &mag);
        HSD_DatReadF32(&dat, at + 68, &blend);
        const auto prefix = "workshop-texture-" + std::to_string(layer) + "-";
        html += "<div class='workshop-model-inspector-section'><div "
                "class='eyebrow'>TEXTURE LAYER " +
                std::to_string(layer + 1) + "</div>";
        const char* ids[] = { "0", "1", "2", "3", "4", "5", "6", "7" };
        html += choice(prefix + "id", "Texture animation ID", id, ids, 8, model.project->facts.costume_matanim[0] != 0);
        html +=
            choice(prefix + "source", "Coordinates from", src, sources, 21);
        html += choice(prefix + "mapping", "Mapping", flags & 15, coords, 7);
        html += choice(prefix + "color", "Color operation", (flags >> 16) & 15,
                       color_maps, 9);
        html += choice(prefix + "alpha", "Alpha operation", (flags >> 20) & 15,
                       alpha_maps, 8);
        html += numeric(prefix + "blend", "Blend amount", blend);
        for (unsigned i = 0; i < 6; ++i) {
            html += check(prefix + "flag-" + std::to_string(i), light_names[i],
                          flags & light_bits[i]);
        }
        const char* labels[] = { "Rotation X (radians)",
                                 "Rotation Y (radians)",
                                 "Rotation Z (radians)",
                                 "Scale U",
                                 "Scale V",
                                 "Scale W",
                                 "Offset U",
                                 "Offset V",
                                 "Offset W" };
        for (unsigned i = 0; i < 9; ++i) {
            float value;
            HSD_DatReadF32(&dat, at + 16 + i * 4, &value);
            html += numeric(prefix + "transform-" + std::to_string(i),
                            labels[i], value);
        }
        html += choice(prefix + "wrap-s", "Wrap U", ws, wraps, 3) +
                choice(prefix + "wrap-t", "Wrap V", wt, wraps, 3);
        const auto* bytes = HSD_DatBytes(&dat, at, 92);
        html += numeric(prefix + "repeat-s", "Repeat U (1-255)", bytes[60]) +
                numeric(prefix + "repeat-t", "Repeat V (1-255)", bytes[61]);
        html +=
            choice(prefix + "mag", "Magnification filter", mag, filters, 2);
        const u32 image = pointer(dat, at + 76);
        if (image) {
            u16 w, h;
            HSD_DatReadU16(&dat, image + 4, &w);
            HSD_DatReadU16(&dat, image + 6, &h);
            html += "<p>Image: " + std::to_string(w) + " × " +
                    std::to_string(h) + " pixels.</p>";
        }
        const u32 lod = pointer(dat, at + 84);
        u32 min = 1, aniso = 0;
        float bias = 0;
        uint8_t lflags[2] = {};
        if (lod && HSD_DatBytes(&dat, lod, 16)) {
            HSD_DatReadU32(&dat, lod, &min);
            HSD_DatReadU32(&dat, lod + 12, &aniso);
            HSD_DatReadF32(&dat, lod + 4, &bias);
            std::memcpy(lflags, HSD_DatBytes(&dat, lod + 8, 2), 2);
        }
        html += check(prefix + "lod", "Custom mipmap filtering", lod != 0);
        html +=
            choice(prefix + "min", "Minification filter", min, filters, 6) +
            numeric(prefix + "bias", "Mipmap bias", bias);
        const char* anisotropy[] = { "1×", "2×", "4×" };
        html +=
            choice(prefix + "anisotropy", "Anisotropy", aniso, anisotropy, 3);
        html += check(prefix + "clamp", "Clamp mipmap bias", lflags[0]) +
                check(prefix + "edge", "Edge mipmap selection", lflags[1]);
        const u32 tev = pointer(dat, at + 88);
        uint8_t combine[32] = {};
        for(unsigned i=8;i<16;++i) combine[i]=i<12?15:7;
        if (tev && HSD_DatBytes(&dat, tev, 32)) {
            std::memcpy(combine, HSD_DatBytes(&dat, tev, 32), 32);
        }
        html += check(prefix + "tev", "Custom color combiner", tev != 0);
        const char* biases[] = { "Zero", "Add half", "Subtract half" };
        const char* scales[] = { "1×", "2×", "4×", "0.5×" };
        for (unsigned c = 0; c < 2; ++c) {
            const std::string which = c ? "Alpha" : "Color";
            html += choices(prefix + "tev-" + std::to_string(c),
                            (which + " operation").c_str(), combine[c],
                            operations, 10);
            html +=
                choice(prefix + "tev-" + std::to_string(2 + c),
                       (which + " bias").c_str(), combine[2 + c], biases, 3);
            html +=
                choice(prefix + "tev-" + std::to_string(4 + c),
                       (which + " scale").c_str(), combine[4 + c], scales, 4);
            html += check(prefix + "tev-" + std::to_string(6 + c),
                          (which + " clamp to 0-1").c_str(), combine[6 + c]);
            for (unsigned operand = 0; operand < 4; ++operand) {
                const unsigned idx = 8 + c * 4 + operand;
                const std::string label =
                    which + " input " + char('A' + operand);
                html += choices(prefix + "tev-" + std::to_string(idx),
                                label.c_str(), combine[idx],
                                c ? alpha_inputs : color_inputs, c ? 8 : 14);
            }
        }
        for (unsigned i = 0; i < 3; ++i) {
            html += color(prefix + "tev-color-" + std::to_string(i),
                          i == 0   ? "Constant RGBA"
                          : i == 1 ? "Texture register 0 RGBA"
                                   : "Texture register 1 RGBA",
                          combine + 16 + i * 4);
        }
        u32 active = 0;
        if (tev) {
            HSD_DatReadU32(&dat, tev + 28, &active);
        }
        for (unsigned bit = 0; bit < 12; ++bit) {
            const std::string label = std::string(bit < 4   ? "Constant "
                                                  : bit < 8 ? "Register 0 "
                                                            : "Register 1 ") +
                                      "RGBA"[bit % 4] + " override";
            html += check(prefix + "active-" + std::to_string(bit),
                          label.c_str(), active & (1u << bit));
        }
        html += check(prefix + "active-30", "Enable custom color expression",
                      active & (1u << 30));
        html += check(prefix + "active-31", "Enable custom alpha expression",
                      active & (1u << 31));
        html += "</div>";
    }
    return html;
}

bool nativeWorkshopTextures_Apply(Rml::ElementDocument* document,
                                  const NativeFighterModelEditor& model,
                                  unsigned material, NativeAssetDat& dat,
                                  std::string* error)
{
    auto checked = [&](const std::string& id) {
        auto* e = document->GetElementById(id);
        return e && e->HasAttribute("checked");
    };
    auto number = [&](const std::string& id, float* value) {
        auto* e = dynamic_cast<Rml::ElementFormControl*>(document->GetElementById(id));
        if (!e) return false;
        const auto text = e->GetValue(); char* end;
        *value = std::strtof(text.c_str(), &end);
        return !text.empty() && !*end && std::isfinite(*value);
    };
    auto whole = [&](const std::string& id, unsigned maximum, auto* value) {
        float f;
        if (!number(id, &f) || f < 0 || f > maximum || f != std::floor(f)) return false;
        *value = (unsigned)f; return true;
    };
    auto rgba = [&](const std::string& id, uint8_t* output) {
        auto* e = dynamic_cast<Rml::ElementFormControl*>(document->GetElementById(id));
        if (!e) return false;
        const auto text = e->GetValue();
        if (text.size() != 9 || text[0] != '#') return false;
        char* end; const auto color = std::strtoul(text.c_str() + 1, &end, 16);
        if (*end) return false;
        for (unsigned i=0;i<4;++i) output[i]=(uint8_t)(color >> (24-i*8));
        return true;
    };
    u32 offsets[8];
    const unsigned count=nativeFighterMaterial_TextureLayers(model,material,offsets);
    for (unsigned layer=0;layer<count;++layer) {
        NativeMaterialTextureState state;
        if (!nativeFighterMaterial_ReadTexture(model,material,layer,state)) goto invalid;
        const std::string p="workshop-texture-"+std::to_string(layer)+"-";
        unsigned mode,color,alpha,rs,rt;
        if (!whole(p+"id",7,&state.animation_id) || !whole(p+"source",20,&state.coordinate_source) ||
            !whole(p+"mapping",6,&mode) || !whole(p+"color",8,&color) || !whole(p+"alpha",7,&alpha) ||
            !whole(p+"wrap-s",2,&state.wrap[0]) || !whole(p+"wrap-t",2,&state.wrap[1]) ||
            !whole(p+"mag",1,&state.magnification) || !whole(p+"repeat-s",255,&rs) ||
            !whole(p+"repeat-t",255,&rt) || !number(p+"blend",&state.blend)) goto invalid;
        state.repeat[0]=(uint8_t)rs;state.repeat[1]=(uint8_t)rt;
        state.flags=(state.flags & ~(15u | (15u<<16) | (15u<<20))) | mode | (color<<16) | (alpha<<20);
        for (unsigned i=0;i<6;++i)
            state.flags=checked(p+"flag-"+std::to_string(i)) ? state.flags|light_bits[i] : state.flags&~light_bits[i];
        for (unsigned i=0;i<9;++i) if (!number(p+"transform-"+std::to_string(i),&state.transform[i])) goto invalid;
        state.lod_enabled=checked(p+"lod");
        if (state.lod_enabled) {
            if (!whole(p+"min",5,&state.minification) || !whole(p+"anisotropy",2,&state.anisotropy) ||
                !number(p+"bias",&state.lod_bias)) goto invalid;
            state.clamp_bias=checked(p+"clamp");state.edge_lod=checked(p+"edge");
        }
        state.combiner_enabled=checked(p+"tev");
        if (state.combiner_enabled) {
            for (unsigned i=0;i<16;++i) {
                unsigned value;
                if (i==6 || i==7) value=checked(p+"tev-"+std::to_string(i));
                else if (!whole(p+"tev-"+std::to_string(i),i<2?15:i<4?2:i<6?3:i<12?136:69,&value)) goto invalid;
                state.combine[i]=(uint8_t)value;
            }
            for (unsigned i=0;i<3;++i) if (!rgba(p+"tev-color-"+std::to_string(i),state.combine+16+i*4)) goto invalid;
            state.active &= ~((1u<<30)|(1u<<31)|4095u);
            for (unsigned bit=0;bit<12;++bit) if (checked(p+"active-"+std::to_string(bit))) state.active|=1u<<bit;
            if (checked(p+"active-30")) state.active|=1u<<30;
            if (checked(p+"active-31")) state.active|=1u<<31;
        }
        if (!nativeFighterMaterial_StageTexture(model,material,layer,state,dat,*error)) return false;
    }
    return true;
invalid:
    *error="Enter finite texture transforms, valid filtering choices, and RGBA colors such as #FFFFFFFF.";
    return false;
}
