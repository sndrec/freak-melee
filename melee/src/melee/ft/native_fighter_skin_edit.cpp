#include "native_fighter_skin_mesh.h"
#include "native_asset_dat.h"
#include "native_fighter_costume_compact.h"
extern "C" {
#include <melee/ft/ftcostume_assets.h>
#include <sysdolphin/baselib/jobj.h>
}
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <array>
#include <map>

namespace {
void fail(char* error, size_t size, const char* message)
{
    if (error && size) std::snprintf(error, size, "%s", message);
}
void real(NativeAssetDat& dat, u32 offset, float value)
{
    u32 bits; std::memcpy(&bits, &value, 4); dat.word(offset, bits);
}
struct Chunk {
    std::vector<unsigned> corners;
    std::vector<u8> slots;
    std::vector<NativeSkinWeights> bindings;
};
unsigned palette_slot(const std::vector<NativeSkinWeights>& bindings,
                       const NativeSkinWeights& weights)
{
    unsigned slot = 0;
    while (slot < bindings.size() && !nativeSkinWeights_Equal(&bindings[slot], &weights)) ++slot;
    return slot;
}
bool write_chunk(NativeAssetDat& dat, const NativeFighterModelEditor& model,
                  const NativeSkinPart& part, const Chunk& chunk,
                  u32 positions, u32* polygon)
{
    const auto& source = model.dat;
    std::vector<u8> display;
    display.push_back(0x90);
    display.push_back((u8)(chunk.corners.size() >> 8));
    display.push_back((u8)chunk.corners.size());
    const auto normal = std::find_if(part.attributes.begin(), part.attributes.end(),
        [](const auto& attr) { return attr.attr == 10 || attr.attr == 25; });
    const unsigned normal_components = normal == part.attributes.end() ? 0 : normal->count ? 9 : 3;
    std::vector<float> normal_values;
    std::vector<u16> normal_indices;
    std::map<std::array<float, 9>, u16> normal_lookup;
    if (normal_components) for (unsigned index : chunk.corners) {
        const auto& corner = part.corners[index];
        std::array<float, 9> key;
        std::copy(corner.normal, corner.normal + 9, key.begin());
        auto [entry, added] = normal_lookup.emplace(key, (u16)(normal_values.size() / normal_components));
        if (added) normal_values.insert(normal_values.end(), corner.normal, corner.normal + normal_components);
        normal_indices.push_back(entry->second);
    }
    const u32 normals = normal_components ? dat.allocate(normal_values.size() * 4) : 0;
    if (normals == UINT32_MAX) return false;
    for (unsigned i = 0; i < normal_values.size(); ++i) real(dat, normals + i * 4, normal_values[i]);
    for (unsigned i = 0; i < chunk.corners.size(); ++i) {
        const auto& corner = part.corners[chunk.corners[i]];
        display.push_back((u8)(chunk.slots[i] * 3));
        u32 cursor = corner.source;
        for (const auto& attr : part.attributes) {
            const u8* bytes = source.data + cursor;
            if (attr.attr == 9 || attr.attr == 10 || attr.attr == 25) {
                const unsigned index = attr.attr == 9 ? corner.vertex : normal_indices[i];
                display.push_back((u8)(index >> 8)); display.push_back((u8)index);
            } else if (attr.attr >= 1 && attr.attr <= 8) {
                // Envelope-generated texture matrices follow the same binding
                // as this corner's position; GX_IDENTITY remains unchanged.
                if (bytes[0] != 60 && bytes[0] != 30 + corner.matrix * 3) return false;
                display.push_back(bytes[0] == 60 ? 60 : (u8)(30 + chunk.slots[i] * 3));
            } else if (attr.attr != 0) {
                display.insert(display.end(), bytes, bytes + attr.display_bytes);
            }
            cursor += attr.display_bytes;
        }
    }
    display.resize((display.size() + 31) & ~size_t(31), 0);
    if (display.size() / 32 > UINT16_MAX) return false;
    const u32 commands = dat.allocate(display.size(), 32);
    const u32 descriptors = dat.allocate((part.attributes.size() + 2) * 24);
    const u32 table = dat.allocate((chunk.bindings.size() + 1) * 4);
    *polygon = dat.allocate(24);
    if (commands == UINT32_MAX || descriptors == UINT32_MAX || table == UINT32_MAX ||
        *polygon == UINT32_MAX) return false;
    std::memcpy(dat.data.data() + commands, display.data(), display.size());
    unsigned descriptor = 0;
    dat.word(descriptors, 0); dat.word(descriptors + 4, 1); ++descriptor;
    for (const auto& attr : part.attributes) {
        if (attr.attr == 0) continue;
        const u32 at = descriptors + descriptor++ * 24;
        std::memcpy(dat.data.data() + at, source.data + attr.source, 24);
        if (attr.attr == 9 || attr.attr == 10 || attr.attr == 25) {
            const unsigned components = attr.attr == 9 ? 3 : normal_components;
            dat.word(at + 4, 3); // GX_INDEX16.
            dat.word(at + 8, attr.attr == 9 || components == 9 ? 1 : 0);
            dat.word(at + 12, 4); // GX_F32.
            dat.data[at + 16] = 0;
            dat.half(at + 18, (u16)(components * 4));
            dat.link(at + 20, attr.attr == 9 ? positions : normals);
        } else if (attr.type > 1) dat.link_local(at + 20, attr.array);
        else dat.link(at + 20, 0);
    }
    dat.word(descriptors + descriptor * 24, 255);
    for (unsigned slot = 0; slot < chunk.bindings.size(); ++slot) {
        const auto& weights = chunk.bindings[slot];
        const unsigned count = weights.count == 1 ? 2 : weights.count;
        const u32 entries = dat.allocate((count + 1) * 8);
        if (entries == UINT32_MAX) return false;
        dat.link(table + slot * 4, entries);
        unsigned first = 0;
        for (unsigned i = 1; i < weights.count; ++i)
            if (weights.weights[i] < weights.weights[first]) first = i;
        for (unsigned i = 0; i < count; ++i) {
            const unsigned from = weights.count == 1 ? 0 : i == 0 ? first : i <= first ? i - 1 : i;
            const unsigned bone = weights.joints[from];
            const u32 offset = model.joints[bone].offset;
            if(!nativeSkinMesh_EnsureInverseBind(dat,model,bone))return false;
            dat.link_local(entries + i * 8, offset);
            real(dat, entries + i * 8 + 4, weights.count == 1 ? .5f : weights.weights[from]);
        }
    }
    u32 name; const char* external;
    const auto name_kind = HSD_DatResolve(&source, part.source, &name, &external);
    if (name_kind == HSD_DAT_REF_LOCAL) dat.link_local(*polygon, name);
    else if (name_kind != HSD_DAT_REF_NULL) return false;
    dat.link(*polygon + 8, descriptors);
    dat.half(*polygon + 12, (part.flags & ~0x3000) | POBJ_ENVELOPE);
    dat.half(*polygon + 14, (u16)(display.size() / 32));
    dat.link(*polygon + 16, commands); dat.link(*polygon + 20, table);
    return true;
}
}
bool nativeSkinMesh_EnsureInverseBind(NativeAssetDat& dat,const NativeFighterModelEditor& model,unsigned bone)
{
    if(bone>=model.joint_count)return false;
    const u32 slot=model.joints[bone].offset+56;
    u32 target;const char* external;
    const auto kind=HSD_DatResolve(&model.dat,slot,&target,&external);
    if(kind==HSD_DAT_REF_LOCAL) {
        Mtx inverse;return nativeSkinMesh_InverseBind(model,bone,inverse);
    }
    if(kind!=HSD_DAT_REF_NULL || !(model.joints[bone].flags&JOBJ_SKELETON_ROOT))return false;
    if(NativeAssetDat::read32(dat.data.data()+slot))return true;
    const u32 identity=dat.allocate(48);
    if(identity==UINT32_MAX)return false;
    for(unsigned axis=0;axis<3;++axis)real(dat,identity+(axis*4+axis)*4,1);
    dat.link(slot,identity);return true;
}
bool nativeSkinMesh_Write(NativeAssetDat& dat, const NativeFighterModelEditor& model,
                 unsigned mesh, const NativeSkinMesh& skin)
{
    if (skin.vertices.size() > 65536) return false;
    const u32 positions = dat.allocate(skin.vertices.size() * 12);
    if (positions == UINT32_MAX) return false;
    for (unsigned i = 0; i < skin.vertices.size(); ++i)
        for (unsigned axis = 0; axis < 3; ++axis)
            real(dat, positions + i * 12 + axis * 4, skin.vertices[i].position[axis]);
    u32 previous = 0;
    for (const auto& part : skin.parts) {
        size_t stride = 1;
        for (const auto& attr : part.attributes)
            if (attr.attr != 0)
                stride += attr.attr == 9 || attr.attr == 10 || attr.attr == 25 ? 2 : attr.display_bytes;
        const size_t max_corners = std::min(size_t(65532),
            ((size_t(UINT16_MAX) * 32 - 3) / stride / 3) * 3);
        if (max_corners < 3) return false;
        Chunk chunk;
        auto flush = [&]() {
            if (chunk.corners.empty()) return true;
            u32 emitted;
            if (!write_chunk(dat, model, part, chunk, positions, &emitted)) return false;
            dat.link(previous ? previous + 4 : model.meshes[mesh].offset + 12, emitted);
            previous = emitted; chunk = {}; return true;
        };
        for (unsigned start = 0; start < part.triangles.size(); start += 3) {
            unsigned additional = 0;
            for (unsigned c = 0; c < 3; ++c) {
                const auto& weights = skin.vertices[part.corners[part.triangles[start + c]].vertex].skin;
                bool found = palette_slot(chunk.bindings, weights) < chunk.bindings.size();
                for (unsigned before = 0; before < c; ++before)
                    if (nativeSkinWeights_Equal(&weights,
                        &skin.vertices[part.corners[part.triangles[start + before]].vertex].skin)) found = true;
                additional += !found;
            }
            if ((chunk.bindings.size() + additional > 10 || chunk.corners.size() + 3 > max_corners) && !flush()) return false;
            for (unsigned c = 0; c < 3; ++c) {
                const unsigned corner = part.triangles[start + c];
                const auto& weights = skin.vertices[part.corners[corner].vertex].skin;
                const unsigned slot = palette_slot(chunk.bindings, weights);
                if (slot == chunk.bindings.size()) chunk.bindings.push_back(weights);
                chunk.corners.push_back(corner); chunk.slots.push_back((u8)slot);
            }
        }
        if (!flush()) return false;
    }
    return previous != 0;
}

unsigned nativeFighterSkin_Vertices(const NativeFighterProject* project, unsigned mesh,
                                    NativeSkinVertex* vertices, unsigned capacity,
                                    unsigned* corners, unsigned corner_capacity,
                                    unsigned* corner_count)
{
    if (corner_count) *corner_count = 0;
    NativeFighterModelEditor model = {};
    NativeSkinMesh skin;
    if (!project || !nativeFighterModel_Open(&model, const_cast<NativeFighterProject*>(project)) ||
        !nativeSkinMesh_Read(model, mesh, &skin, nullptr, 0)) return 0;
    if (corner_count) *corner_count = (unsigned)skin.corner_vertices.size();
    if ((!vertices || capacity >= skin.vertices.size()) &&
        (!corners || corner_capacity >= skin.corner_vertices.size())) {
        if (vertices) std::memcpy(vertices, skin.vertices.data(), skin.vertices.size() * sizeof(*vertices));
        if (corners) std::memcpy(corners, skin.corner_vertices.data(), skin.corner_vertices.size() * sizeof(*corners));
    }
    return (unsigned)skin.vertices.size();
}

bool nativeFighterSkin_Paint(NativeFighterProject* project, unsigned mesh, unsigned joint,
                             const NativeSkinBrushPoint* centers, unsigned count,
                             float radius, float strength, bool subtract,
                             unsigned* affected, char* error, size_t error_size)
{
    if (affected) *affected = 0;
    if (!project || !centers || !count || count > 4096 || !std::isfinite(radius) ||
        radius <= 0 || radius > 1000 || !std::isfinite(strength) || strength <= 0 || strength > 1) {
        fail(error, error_size, "Enter a valid brush radius and strength."); return false;
    }
    for (unsigned i = 0; i < count; ++i) for (float value : centers[i].position)
        if (!std::isfinite(value)) return false;
    NativeFighterModelEditor model = {};
    Mtx inverse;
    if (!nativeFighterModel_Open(&model, project) || joint >= model.joint_count ||
        !(model.joints[joint].flags & JOBJ_SKELETON) || !nativeSkinMesh_InverseBind(model, joint, inverse)) {
        fail(error, error_size, "Select a deform bone with an inverse bind matrix."); return false;
    }
    NativeSkinMesh skin;
    if (!nativeSkinMesh_Read(model, mesh, &skin, error, error_size)) return false;
    unsigned changed = 0;
    for (auto& vertex : skin.vertices) {
        float distance = radius * radius;
        for (unsigned center = 0; center < count; ++center) {
            float current = 0;
            for (unsigned axis = 0; axis < 3; ++axis) {
                const float delta = vertex.position[axis] - centers[center].position[axis]; current += delta * delta;
            }
            distance = std::min(distance, current);
        }
        if (distance >= radius * radius) continue;
        const auto before = vertex.skin;
        const float change = strength * (1 - std::sqrt(distance) / radius);
        const float weight = std::clamp(nativeSkinWeights_Get(&vertex.skin, (u16)joint) +
                                         (subtract ? -change : change), 0.0f, 1.0f);
        if (!nativeSkinWeights_Set(&vertex.skin, (u16)joint, weight)) {
            fail(error, error_size, "A vertex already has the maximum 16 bone influences."); return false;
        }
        changed += !nativeSkinWeights_Equal(&before, &vertex.skin);
    }
    if (!changed) return true;
    NativeAssetDat dat;
    std::vector<u8> converted;
    if (!dat.open(project->costume_dat, project->costume_size) || !nativeSkinMesh_Write(dat, model, mesh, skin) ||
        !dat.finish(&converted, UINT32_MAX)) {
        fail(error, error_size, "Could not rebuild this mesh's weights and vertex attributes."); return false;
    }
    if (!nativeFighterCostume_Compact(project, &converted, nullptr, error, error_size)) return false;
    std::vector<u8> arena(FT_COSTUME_IMPORT_ARENA_SIZE);
    FtCostumeAssets imported;
    if (!ftCostumeAssets_Import(converted.data(), converted.size(), model.root_name, nullptr,
            arena.data(), arena.size(), FT_COSTUME_IMPORT_RECORD_CAPACITY, &imported)) {
        fail(error, error_size, "The painted model failed geometry validation."); return false;
    }
    u8* bytes = (u8*)std::malloc(converted.size());
    if (!bytes) return false;
    std::memcpy(bytes, converted.data(), converted.size());
    std::free(project->costume_dat); project->costume_dat = bytes; project->costume_size = converted.size();
    if (affected) *affected = changed;
    return true;
}
