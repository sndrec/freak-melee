#include "native_fighter_skin_morph.h"
#include "native_asset_dat.h"
extern "C" {
#include <sysdolphin/baselib/shape.h>
}
#include <array>
#include <cmath>
#include <map>
#include <numeric>

namespace {
void real(NativeAssetDat& dat, u32 offset, float value)
{
    u32 bits; std::memcpy(&bits, &value, 4); dat.word(offset, bits);
}
bool target_arrays(NativeAssetDat& dat, const std::vector<float>& source,
                     unsigned old_count, unsigned components, unsigned targets,
                     const std::vector<unsigned>& logical, unsigned attr,
                     u32* descriptor, u32* table, uint32_t* array)
{
    if (!components) { *descriptor = *table = *array = 0; return source.empty(); }
    if (source.size() != size_t(targets)*old_count*components || logical.size() > UINT16_MAX) return false;
    std::vector<float> physical;
    std::vector<u16> indices;
    std::map<std::array<u32, 9>, u16> lookup;
    for (unsigned target = 0; target < targets; ++target) {
        for (unsigned from : logical) {
            if (from >= old_count) return false;
            const float* point = source.data()+(size_t(target)*old_count+from)*components;
            std::array<u32, 9> key = {};
            for (unsigned c = 0; c < components; ++c) {
                if (!std::isfinite(point[c])) return false;
                std::memcpy(&key[c], point+c, 4);
            }
            auto found = lookup.find(key);
            unsigned index;
            // Keep a dense base prefix for ordinary envelope admission. Later
            // targets can reuse any identical physical vector through their
            // index tables without merging logical morph vertices.
            if (target == 0 || found == lookup.end()) {
                index = (unsigned)(physical.size()/components);
                if (index > UINT16_MAX) return false;
                physical.insert(physical.end(), point, point+components);
                lookup.emplace(key, (u16)index);
            } else index = found->second;
            indices.push_back((u16)index);
        }
    }
    *descriptor = dat.allocate(48); *table = dat.allocate(size_t(targets)*4);
    *array = dat.allocate(physical.size()*4);
    if (*descriptor == UINT32_MAX || *table == UINT32_MAX || *array == UINT32_MAX) return false;
    dat.word(*descriptor, attr); dat.word(*descriptor+4, GX_INDEX16);
    dat.word(*descriptor+8, attr == GX_VA_POS ? GX_POS_XYZ : GX_NRM_XYZ);
    dat.word(*descriptor+12, GX_F32); dat.half(*descriptor+18, (u16)(components*4));
    dat.link(*descriptor+20, *array); dat.word(*descriptor+24, GX_VA_NULL);
    for (unsigned i = 0; i < physical.size(); ++i) real(dat, *array+i*4, physical[i]);
    for (unsigned target = 0; target < targets; ++target) {
        const u32 row = dat.allocate(logical.size()*2);
        if (row == UINT32_MAX) return false;
        dat.link(*table+target*4, row);
        for (unsigned i = 0; i < logical.size(); ++i)
            dat.half(row+i*2, indices[size_t(target)*logical.size()+i]);
    }
    return true;
}
}

bool nativeSkinMorph_Encode(NativeAssetDat& dat, const NativeFighterModelEditor& model,
                             const NativeSkinPart& part, const NativeSkinMesh& mesh,
                             NativeSkinMorphEncoded* out)
{
    const auto& morph = part.morph;
    const unsigned mode = morph.flags & 3;
    if ((mode != SHAPESET_AVERAGE && mode != SHAPESET_ADDITIVE) || !morph.shape_count ||
        !morph.position_count || morph.position_count > HSD_DEFAULT_MAX_SHAPE_VERTICES ||
        morph.vertices.size() != morph.position_count || part.triangles.empty() || part.triangles.size()%3) return false;
    std::vector<unsigned> positions(morph.position_count), normals(morph.normal_count);
    std::iota(positions.begin(), positions.end(), 0);
    std::iota(normals.begin(), normals.end(), 0);
    std::map<unsigned, unsigned> vertex_indices;
    for (unsigned i = 0; i < morph.vertices.size(); ++i) {
        if (morph.vertices[i] >= mesh.vertices.size() || !vertex_indices.emplace(morph.vertices[i], i).second) return false;
    }
    out->corner_positions.resize(part.corners.size()); out->corner_normals.resize(part.corners.size());
    std::map<std::array<u32, 2+2*NATIVE_SKIN_MAX_INFLUENCES>, unsigned> normal_bindings;
    std::vector<bool> used_normal(morph.normal_count);
    for (unsigned i = 0; i < part.corners.size(); ++i) {
        const auto& corner = part.corners[i];
        const auto found = vertex_indices.find(corner.vertex);
        if (found == vertex_indices.end()) return false;
        out->corner_positions[i] = (u16)found->second;
        if (!morph.normal_count) continue;
        if (corner.normal_index >= morph.normal_count) return false;
        const auto& weights = mesh.vertices[corner.vertex].skin;
        if (!weights.count || weights.count > NATIVE_SKIN_MAX_INFLUENCES) return false;
        std::array<u32, 2+2*NATIVE_SKIN_MAX_INFLUENCES> key = {};
        key[0] = corner.normal_index; key[1] = weights.count;
        for (unsigned w = 0; w < weights.count; ++w) {
            key[2+w*2] = weights.joints[w];
            std::memcpy(&key[3+w*2], &weights.weights[w], 4);
        }
        auto binding = normal_bindings.find(key);
        unsigned normal;
        if (binding != normal_bindings.end()) normal = binding->second;
        else {
            normal = used_normal[corner.normal_index] ? (unsigned)normals.size() : corner.normal_index;
            if (used_normal[corner.normal_index]) normals.push_back(corner.normal_index);
            used_normal[corner.normal_index] = true; normal_bindings.emplace(key, normal);
        }
        if ((size_t(normal)+1)*(morph.normal_components/3) > HSD_DEFAULT_MAX_SHAPE_NORMALS) return false;
        out->corner_normals[i] = (u16)normal;
    }
    if (normals.size()*size_t(morph.normal_components/3) > HSD_DEFAULT_MAX_SHAPE_NORMALS) return false;
    out->normal_components = morph.normal_components;
    const unsigned targets = morph.shape_count + (mode == SHAPESET_ADDITIVE);
    out->shape = dat.allocate(32); out->polygon = dat.allocate(24);
    if (out->shape == UINT32_MAX || out->polygon == UINT32_MAX) return false;
    u32 descriptors[2], tables[2];
    if (!target_arrays(dat, morph.positions, morph.position_count, 3, targets, positions, GX_VA_POS,
                        &descriptors[0], &tables[0], &out->positions) ||
        !target_arrays(dat, morph.normals, morph.normal_count, morph.normal_components, targets, normals,
                        morph.normal_components == 9 ? GX_VA_NBT : GX_VA_NRM,
                        &descriptors[1], &tables[1], &out->normals)) return false;
    dat.half(out->shape, morph.flags | SHAPESET_SKIN); dat.half(out->shape+2, morph.shape_count);
    dat.word(out->shape+4, morph.position_count); dat.word(out->shape+16, (u32)normals.size());
    for (unsigned a = 0; a < 2; ++a) {
        dat.link(out->shape+8+a*12, descriptors[a]); dat.link(out->shape+12+a*12, tables[a]);
    }
    const u32 attrs = dat.allocate((part.attributes.size()+2)*24);
    if (attrs == UINT32_MAX) return false;
    dat.word(attrs, GX_VA_PNMTXIDX); dat.word(attrs+4, GX_DIRECT);
    unsigned descriptor = 1;
    for (const auto& attr : part.attributes) {
        if (attr.attr == GX_VA_PNMTXIDX) continue;
        const u32 at = attrs+descriptor++*24;
        std::memcpy(dat.data.data()+at, model.dat.data+attr.source, 24);
        if (attr.attr == GX_VA_POS || attr.attr == GX_VA_NRM || attr.attr == GX_VA_NBT) {
            const bool position = attr.attr == GX_VA_POS;
            const unsigned components = position ? 3 : morph.normal_components;
            dat.word(at+4, GX_INDEX16); dat.word(at+8, position || components == 9 ? 1 : 0);
            dat.word(at+12, GX_F32); dat.data[at+16] = 0; dat.half(at+18, (u16)(components*4));
            dat.link(at+20, position ? out->positions : out->normals);
        } else if (attr.type > GX_DIRECT) dat.link_local(at+20, attr.array);
        else dat.link(at+20, 0);
    }
    dat.word(attrs+descriptor*24, GX_VA_NULL);
    std::vector<u8> display;
    for (unsigned start = 0; start < part.triangles.size();) {
        const unsigned count = (unsigned)std::min(size_t(65532), part.triangles.size()-start);
        display.insert(display.end(), {GX_TRIANGLES, (u8)(count>>8), (u8)count});
        for (unsigned i = 0; i < count; ++i) {
            const unsigned corner_index = part.triangles[start+i];
            if (corner_index >= part.corners.size()) return false;
            const auto& corner = part.corners[corner_index];
            display.push_back(0);
            u32 cursor = corner.source;
            for (const auto& attr : part.attributes) {
                if (attr.attr == GX_VA_POS || attr.attr == GX_VA_NRM || attr.attr == GX_VA_NBT) {
                    const unsigned index = attr.attr == GX_VA_POS ? out->corner_positions[corner_index] : out->corner_normals[corner_index];
                    display.push_back((u8)(index>>8)); display.push_back((u8)index);
                } else if (attr.attr != GX_VA_PNMTXIDX) {
                    const u8* bytes = model.dat.data+cursor;
                    display.insert(display.end(), bytes, bytes+attr.display_bytes);
                }
                cursor += attr.display_bytes;
            }
        }
        start += count;
    }
    display.resize((display.size()+31)&~size_t(31));
    if (display.size()/32 > UINT16_MAX) return false;
    const u32 commands = dat.allocate(display.size(), 32);
    if (commands == UINT32_MAX) return false;
    std::memcpy(dat.data.data()+commands, display.data(), display.size());
    u32 name; const char* external;
    const auto kind = HSD_DatResolve(&model.dat, part.source, &name, &external);
    if (kind == HSD_DAT_REF_LOCAL) dat.link_local(out->polygon, name);
    else if (kind != HSD_DAT_REF_NULL) return false;
    dat.link(out->polygon+8, attrs); dat.half(out->polygon+12, part.flags);
    dat.half(out->polygon+14, (u16)(display.size()/32));
    dat.link(out->polygon+16, commands); dat.link(out->polygon+20, out->shape);
    return true;
}
