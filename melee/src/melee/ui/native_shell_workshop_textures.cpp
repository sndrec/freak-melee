#include "native_shell_workshop_textures.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

#include <melee/ft/native_asset_dat.h>
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
    const Choice color_inputs[] = { { 0, "Previous RGB" },
                                    { 1, "Previous alpha" },
                                    { 2, "Register 0 RGB" },
                                    { 3, "Register 0 alpha" },
                                    { 4, "Register 1 RGB" },
                                    { 5, "Register 1 alpha" },
                                    { 6, "Register 2 RGB" },
                                    { 7, "Register 2 alpha" },
                                    { 8, "Texture RGB" },
                                    { 9, "Texture alpha" },
                                    { 10, "Lighting RGB" },
                                    { 11, "Lighting alpha" },
                                    { 12, "One" },
                                    { 13, "Half" },
                                    { 14, "Constant selection" },
                                    { 15, "Zero" },
                                    { 128, "Constant RGB" },
                                    { 129, "Constant R" },
                                    { 130, "Constant G" },
                                    { 131, "Constant B" },
                                    { 132, "Constant A" },
                                    { 133, "Texture register 0 RGB" },
                                    { 134, "Texture register 0 alpha" },
                                    { 135, "Texture register 1 RGB" },
                                    { 136, "Texture register 1 alpha" } };
    const Choice alpha_inputs[] = { { 0, "Previous alpha" },
                                    { 1, "Register 0 alpha" },
                                    { 2, "Register 1 alpha" },
                                    { 3, "Register 2 alpha" },
                                    { 4, "Texture alpha" },
                                    { 5, "Lighting alpha" },
                                    { 6, "Constant selection" },
                                    { 7, "Zero" },
                                    { 64, "Constant R" },
                                    { 65, "Constant G" },
                                    { 66, "Constant B" },
                                    { 67, "Constant A" },
                                    { 68, "Texture register 0 alpha" },
                                    { 69, "Texture register 1 alpha" } };
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
                       unsigned count)
    {
        Choice entries[32];
        for (unsigned i = 0; i < count; ++i) {
            entries[i] = { i, names[i] };
        }
        return choices(id, label, value, entries, count);
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
    unsigned layers(const NativeFighterModelEditor& model, unsigned material,
                    u32 output[8])
    {
        unsigned n = 0;
        u32 at = model.materials[material].texture_offset;
        while (at && n < 8 && HSD_DatBytes(&model.dat, at, 92)) {
            for (unsigned i = 0; i < n; ++i) {
                if (output[i] == at) {
                    return n;
                }
            }
            output[n++] = at;
            at = pointer(model.dat, at + 4);
        }
        return n;
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
    const unsigned count = layers(model, material, offsets);
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
        html += choice(prefix + "id", "Texture animation ID", id, ids, 8);
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
                                c ? alpha_inputs : color_inputs, c ? 14 : 25);
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
        auto* e = dynamic_cast<Rml::ElementFormControl*>(
            document->GetElementById(id));
        if (!e) {
            return false;
        }
        const auto s = e->GetValue();
        char* end;
        *value = std::strtof(s.c_str(), &end);
        return !s.empty() && !*end && std::isfinite(*value);
    };
    auto whole = [&](const std::string& id, unsigned max, unsigned* value) {
        float f;
        if (!number(id, &f) || f < 0 || f > max || f != std::floor(f)) {
            return false;
        }
        *value = (unsigned) f;
        return true;
    };
    auto write_float = [&](const std::string& id, u32 offset) {
        float f;
        if (!number(id, &f)) {
            return false;
        }
        uint32_t raw;
        std::memcpy(&raw, &f, 4);
        dat.word(offset, raw);
        return true;
    };
    auto rgba = [&](const std::string& id, u32 offset) {
        auto* e = dynamic_cast<Rml::ElementFormControl*>(
            document->GetElementById(id));
        if (!e) {
            return false;
        }
        const auto s = e->GetValue();
        if (s.size() != 9 || s[0] != '#') {
            return false;
        }
        char* end;
        const unsigned long color = std::strtoul(s.c_str() + 1, &end, 16);
        if (*end) {
            return false;
        }
        dat.word(offset, (uint32_t) color);
        return true;
    };
    u32 offsets[8];
    const unsigned count = layers(model, material, offsets);
    for (unsigned layer = 0; layer < count; ++layer) {
        const u32 at = offsets[layer];
        const std::string p =
            "workshop-texture-" + std::to_string(layer) + "-";
        unsigned id, src, mode, c, a, ws, wt, mag, rs, rt;
        if (!whole(p + "id", 255, &id) || !whole(p + "source", 20, &src) ||
            !whole(p + "mapping", 6, &mode) || !whole(p + "color", 8, &c) ||
            !whole(p + "alpha", 7, &a) || !whole(p + "wrap-s", 2, &ws) ||
            !whole(p + "wrap-t", 2, &wt) || !whole(p + "mag", 1, &mag) ||
            !whole(p + "repeat-s", 255, &rs) ||
            !whole(p + "repeat-t", 255, &rt) || !rs || !rt ||
            !write_float(p + "blend", at + 68))
        {
            goto invalid;
        }
        u32 flags;
        HSD_DatReadU32(&model.dat, at + 64, &flags);
        flags = (flags & ~(15u | (15u << 16) | (15u << 20))) | mode |
                (c << 16) | (a << 20);
        for (unsigned i = 0; i < 6; ++i) {
            flags = checked(p + "flag-" + std::to_string(i))
                        ? flags | light_bits[i]
                        : flags & ~light_bits[i];
        }
        dat.word(at + 8, id);
        dat.word(at + 12, src);
        dat.word(at + 52, ws);
        dat.word(at + 56, wt);
        dat.word(at + 64, flags);
        dat.word(at + 72, mag);
        dat.data[at + 60] = rs;
        dat.data[at + 61] = rt;
        for (unsigned i = 0; i < 9; ++i) {
            if (!write_float(p + "transform-" + std::to_string(i),
                             at + 16 + i * 4))
            {
                goto invalid;
            }
        }
        if (checked(p + "lod")) {
            unsigned min, aniso;
            if (!whole(p + "min", 5, &min) ||
                !whole(p + "anisotropy", 2, &aniso))
            {
                goto invalid;
            }
            u32 lod = pointer(model.dat, at + 84);
            if (!lod) {
                lod = dat.allocate(16);
            }
            if (lod == UINT32_MAX) {
                goto invalid;
            }
            dat.word(lod, min);
            dat.word(lod + 12, aniso);
            dat.data[lod + 8] = checked(p + "clamp");
            dat.data[lod + 9] = checked(p + "edge");
            if (!write_float(p + "bias", lod + 4)) {
                goto invalid;
            }
            dat.link(at + 84, lod);
        } else {
            dat.link(at + 84, 0);
        }
        if (checked(p + "tev")) {
            u32 tev = pointer(model.dat, at + 88);
            if (!tev) {
                tev = dat.allocate(32);
            }
            if (tev == UINT32_MAX) {
                goto invalid;
            }
            for (unsigned i = 0; i < 16; ++i) {
                unsigned v;
                if (i == 6 || i == 7) {
                    v = checked(p + "tev-" + std::to_string(i));
                } else if (!whole(p + "tev-" + std::to_string(i),
                                  i < 2    ? 15
                                  : i < 4  ? 2
                                  : i < 6  ? 3
                                  : i < 12 ? 136
                                           : 69,
                                  &v))
                {
                    goto invalid;
                }
                dat.data[tev + i] = (uint8_t) v;
            }
            for (unsigned i = 0; i < 3; ++i) {
                if (!rgba(p + "tev-color-" + std::to_string(i),
                          tev + 16 + i * 4))
                {
                    goto invalid;
                }
            }
            unsigned active = 0;
            for (unsigned bit = 0; bit < 12; ++bit) {
                if (checked(p + "active-" + std::to_string(bit))) {
                    active |= 1u << bit;
                }
            }
            if (checked(p + "active-30")) {
                active |= 1u << 30;
            }
            if (checked(p + "active-31")) {
                active |= 1u << 31;
            }
            dat.word(tev + 28, active);
            dat.link(at + 88, tev);
        } else {
            dat.link(at + 88, 0);
        }
    }
    return true;
invalid:
    *error = "Enter finite texture transforms, valid filtering choices, and "
             "RGBA colors such as #FFFFFFFF.";
    return false;
}
