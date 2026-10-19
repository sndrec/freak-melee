#include "native_fighter_skin_mesh.h"
#include "native_fighter_skin_morph.h"
#include "native_asset_dat.h"
#include "native_fighter_costume_compact.h"
#include "native_fighter_authoring.h"
#include "native_mesh_vertex_ids.h"
extern "C" {
#include <melee/ft/ftcostume_assets.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mtx.h>
}
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <array>
#include <map>
#include <memory>
#include <unordered_map>

namespace {
void fail(char* error, size_t size, const char* message)
{
    if (error && size) std::snprintf(error, size, "%s", message);
}
void real(NativeAssetDat& dat, u32 offset, float value)
{
    u32 bits; std::memcpy(&bits, &value, 4); dat.word(offset, bits);
}
unsigned palette_slot(const NativeSkinChunk& chunk, const NativeSkinWeights& weights)
{
    unsigned slot = 0;
    while (slot < chunk.bindings.size() &&
           !nativeSkinWeights_Equal(&chunk.bindings[slot], &weights)) ++slot;
    return slot;
}
unsigned texture_slot(const NativeSkinChunk& chunk, const NativeSkinWeights& weights, uint32_t texture)
{
    unsigned slot = 0;
    while (slot < chunk.texture_bindings.size() &&
           (chunk.texture_ids[slot] != texture ||
            !nativeSkinWeights_Equal(&chunk.texture_bindings[slot], &weights))) ++slot;
    return slot;
}
}
bool nativeSkinMesh_WriteChunk(NativeAssetDat& dat, const NativeFighterModelEditor& model,
                  const NativeSkinPart& part, const NativeSkinChunk& chunk,
                  const NativeSkinMesh& skin, uint32_t positions, bool local_positions,
                  std::vector<NativeMeshVertexIdRow>* vertex_ids,
                  const NativeSkinMorphEncoded* morph, uint32_t* polygon)
{
    const auto& source = model.dat;
    const bool separate = !chunk.texture_bindings.empty();
    const u8* attribute_data=part.attribute_bytes.empty()?source.data:part.attribute_bytes.data();
    std::unordered_map<unsigned, u16> position_indices;
    if (local_positions) {
        position_indices.reserve(chunk.corners.size());
        std::vector<unsigned> vertices;
        for (unsigned corner : chunk.corners) {
            const unsigned vertex = part.corners[corner].vertex;
            auto [entry, added] = position_indices.emplace(vertex, (u16)vertices.size());
            if (added) vertices.push_back(vertex);
        }
        positions = dat.allocate(vertices.size()*12);
        if (positions == UINT32_MAX) return false;
        for (unsigned i = 0; i < vertices.size(); ++i)
            for (unsigned axis = 0; axis < 3; ++axis)
                real(dat, positions+i*12+axis*4, skin.vertices[vertices[i]].position[axis]);
    }
    std::vector<u8> display;
    display.push_back(0x90);
    display.push_back((u8)(chunk.corners.size() >> 8));
    display.push_back((u8)chunk.corners.size());
    const auto normal = std::find_if(part.attributes.begin(), part.attributes.end(),
        [](const auto& attr) { return attr.attr == 10 || attr.attr == 25; });
    const unsigned normal_components = morph ? morph->normal_components :
        normal == part.attributes.end() ? 0 : normal->count ? 9 : 3;
    std::vector<float> normal_values;
    std::vector<u16> normal_indices;
    std::map<std::array<float, 9>, u16> normal_lookup;
    if (normal_components && !morph) for (unsigned index : chunk.corners) {
        const auto& corner = part.corners[index];
        std::array<float, 9> key;
        std::copy(corner.normal, corner.normal + 9, key.begin());
        auto [entry, added] = normal_lookup.emplace(key, (u16)(normal_values.size() / normal_components));
        if (added) normal_values.insert(normal_values.end(), corner.normal, corner.normal + normal_components);
        normal_indices.push_back(entry->second);
    }
    if (morph) positions = morph->positions;
    const u32 normals = morph ? morph->normals : normal_components ? dat.allocate(normal_values.size() * 4) : 0;
    if (normals == UINT32_MAX) return false;
    for (unsigned i = 0; i < normal_values.size(); ++i) real(dat, normals + i * 4, normal_values[i]);
    for (unsigned i = 0; i < chunk.corners.size(); ++i) {
        const auto& corner = part.corners[chunk.corners[i]];
        display.push_back((u8)(chunk.slots[i] * 3));
        u32 cursor = corner.source;
        for (const auto& attr : part.attributes) {
            const u8* bytes = attribute_data + cursor;
            if (attr.attr == 9 || attr.attr == 10 || attr.attr == 25) {
                const unsigned index = morph ? (attr.attr == 9 ? morph->corner_positions[chunk.corners[i]] :
                    morph->corner_normals[chunk.corners[i]]) : attr.attr == 9
                    ? (local_positions ? position_indices.at(corner.vertex) : corner.vertex) : normal_indices[i];
                display.push_back((u8)(index >> 8)); display.push_back((u8)index);
            } else if (attr.attr >= 1 && attr.attr <= 8) {
                const uint32_t texture = corner.textures[attr.attr-1];
                if (texture == NATIVE_SKIN_TEXTURE_IDENTITY) display.push_back(60);
                else {
                    unsigned slot = chunk.slots[i];
                    if (separate) {
                        if (texture != NATIVE_SKIN_TEXTURE_FOLLOW && texture >= part.texture_bindings.size()) return false;
                        const auto& weights = texture == NATIVE_SKIN_TEXTURE_FOLLOW ?
                            skin.vertices[corner.vertex].skin : part.texture_bindings[texture];
                        slot = texture_slot(chunk, weights, texture);
                        if (slot >= chunk.texture_bindings.size()) return false;
                    } else if (texture != NATIVE_SKIN_TEXTURE_FOLLOW) return false;
                    display.push_back((u8)(30 + slot * 3));
                }
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
        std::memcpy(dat.data.data() + at, attribute_data + attr.source, 24);
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
    auto write_binding = [&](const NativeSkinWeights& weights) -> u32 {
        const unsigned count = weights.count == 1 ? 2 : weights.count;
        const u32 entries = dat.allocate((count + 1) * 8);
        if (entries == UINT32_MAX) return UINT32_MAX;
        unsigned first = 0;
        for (unsigned i = 1; i < weights.count; ++i)
            if (weights.weights[i] < weights.weights[first]) first = i;
        for (unsigned i = 0; i < count; ++i) {
            const unsigned from = weights.count == 1 ? 0 : i == 0 ? first : i <= first ? i - 1 : i;
            const unsigned bone = weights.joints[from];
            const u32 offset = model.joints[bone].offset;
            if(!nativeSkinMesh_EnsureInverseBind(dat,model,bone))return UINT32_MAX;
            dat.link_local(entries + i * 8, offset);
            real(dat, entries + i * 8 + 4, weights.count == 1 ? .5f : weights.weights[from]);
        }
        return entries;
    };
    std::vector<u32> position_entries;
    for (const auto& weights : chunk.bindings) {
        const u32 entries = write_binding(weights);
        if (entries == UINT32_MAX) return false;
        dat.link(table + position_entries.size()*4, entries);
        position_entries.push_back(entries);
    }
    u32 matrix_data = table;
    if (separate) {
        const u32 textures = dat.allocate((chunk.texture_bindings.size()+1)*4);
        matrix_data = dat.allocate(20);
        if (textures == UINT32_MAX || matrix_data == UINT32_MAX) return false;
        dat.link(matrix_data, table); dat.link(matrix_data+4, textures);
        std::memset(dat.data.data()+matrix_data+8, 255, 10);
        for (unsigned slot = 0; slot < chunk.texture_bindings.size(); ++slot) {
            u32 entries;
            if (chunk.texture_ids[slot] == NATIVE_SKIN_TEXTURE_FOLLOW) {
                const unsigned position = palette_slot(chunk, chunk.texture_bindings[slot]);
                if (position >= position_entries.size()) return false;
                entries = position_entries[position];
                dat.data[matrix_data+8+slot] = (u8)position;
            } else entries = write_binding(chunk.texture_bindings[slot]);
            if (entries == UINT32_MAX) return false;
            dat.link(textures+slot*4, entries);
        }
    }
    u32 name; const char* external;
    const auto name_kind = part.source==UINT32_MAX?HSD_DAT_REF_NULL:HSD_DatResolve(&source, part.source, &name, &external);
    if (!morph && name_kind == HSD_DAT_REF_LOCAL) dat.link_local(*polygon, name);
    else if (!morph && name_kind != HSD_DAT_REF_NULL) return false;
    dat.link(*polygon + 8, descriptors);
    dat.half(*polygon + 12, (part.flags & ~0x3000) |
        (separate ? POBJ_SEPARATE_ENVELOPES : POBJ_ENVELOPE));
    dat.half(*polygon + 14, (u16)(display.size() / 32));
    dat.link(*polygon + 16, commands); dat.link(*polygon + 20, matrix_data);
    if (local_positions) {
        NativeMeshVertexIdRow row = { (uint32_t)*polygon, {} };
        row.ids.reserve(chunk.corners.size());
        for (unsigned corner : chunk.corners) row.ids.push_back(part.corners[corner].vertex);
        vertex_ids->push_back(std::move(row));
    }
    return true;
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
bool nativeSkinMesh_Encode(NativeAssetDat& dat, const NativeFighterModelEditor& model,
                            const NativeSkinMesh& skin,uint32_t* polygon)
{
    if (!polygon || skin.vertices.size() > UINT32_MAX/12) return false;
    *polygon=0;
    const bool local_positions = skin.vertices.size() > 65536;
    std::vector<NativeMeshVertexIdRow> vertex_ids;
    if (local_positions && !nativeMeshVertexIds_Read(model.dat, &vertex_ids)) return false;
    const u32 positions = local_positions ? 0 : dat.allocate(skin.vertices.size() * 12);
    if (positions == UINT32_MAX) return false;
    if (!local_positions) for (unsigned i = 0; i < skin.vertices.size(); ++i)
        for (unsigned axis = 0; axis < 3; ++axis)
            real(dat, positions + i * 12 + axis * 4, skin.vertices[i].position[axis]);
    u32 previous = 0;
    unsigned sections = 0;
    for (const auto& part : skin.parts) {
        const bool shaped = (part.flags & 0x3000) == POBJ_SHAPEANIM;
        NativeSkinMorphEncoded morph = {};
        u32 private_previous = 0;
        unsigned private_sections = 0;
        if (shaped) {
            if (++sections > NATIVE_MODEL_MAX_POLYGONS || !nativeSkinMorph_Encode(dat, model, part, skin, &morph)) return false;
            if(previous)dat.link(previous+4,morph.polygon);else *polygon=morph.polygon;
            previous = morph.polygon;
        }
        size_t stride = 1;
        for (const auto& attr : part.attributes)
            if (attr.attr != 0)
                stride += attr.attr == 9 || attr.attr == 10 || attr.attr == 25 ? 2 : attr.display_bytes;
        const size_t max_corners = std::min(size_t(65532),
            ((size_t(UINT16_MAX) * 32 - 3) / stride / 3) * 3);
        if (max_corners < 3) return false;
        NativeSkinChunk chunk;
        auto flush = [&]() {
            if (chunk.corners.empty()) return true;
            if ((shaped ? ++private_sections : ++sections) > NATIVE_MODEL_MAX_POLYGONS) return false;
            uint32_t emitted;
            if (!nativeSkinMesh_WriteChunk(dat, model, part, chunk, skin, positions,
                    !shaped && local_positions, &vertex_ids, shaped ? &morph : nullptr, &emitted)) return false;
            if (shaped) {
                dat.link(private_previous ? private_previous+4 : morph.shape+28, emitted);
                private_previous = emitted;
            } else {
                if(previous)dat.link(previous+4,emitted);else *polygon=emitted;
                previous = emitted;
            }
            chunk = {}; return true;
        };
        for (unsigned start = 0; start < part.triangles.size(); start += 3) {
            const bool separate = !part.texture_bindings.empty();
            const NativeSkinWeights* requested[3];
            const NativeSkinWeights* textures[27];
            uint32_t texture_ids[27];
            unsigned count = 0, texture_count = 0;
            for (unsigned c = 0; c < 3; ++c) {
                const auto& corner = part.corners[part.triangles[start+c]];
                const auto& position = skin.vertices[corner.vertex].skin;
                unsigned index = 0;
                while (index < count && !nativeSkinWeights_Equal(requested[index], &position)) ++index;
                if (index == count) requested[count++] = &position;
                if (!separate) continue;
                for (const auto& attr : part.attributes) {
                    if (attr.attr < 1 || attr.attr > 8) continue;
                    const uint32_t texture = corner.textures[attr.attr-1];
                    if (texture == NATIVE_SKIN_TEXTURE_IDENTITY) continue;
                    if (texture != NATIVE_SKIN_TEXTURE_FOLLOW && texture >= part.texture_bindings.size()) return false;
                    const auto& weights = texture == NATIVE_SKIN_TEXTURE_FOLLOW ? position : part.texture_bindings[texture];
                    unsigned t = 0;
                    while (t < texture_count && (texture_ids[t] != texture ||
                        !nativeSkinWeights_Equal(textures[t], &weights))) ++t;
                    if (t == texture_count) {
                        textures[texture_count] = &weights;texture_ids[texture_count++] = texture;
                    }
                }
            }
            // GX has independent ten-slot position/normal and texture sets.
            // Keep every triangle together within each set's actual limit.
            if (texture_count > 10) return false;
            unsigned additional = 0, additional_textures = 0;
            for (unsigned i = 0; i < count; ++i)
                additional += palette_slot(chunk, *requested[i]) == chunk.bindings.size();
            for (unsigned i = 0; i < texture_count; ++i)
                additional_textures += texture_slot(chunk, *textures[i], texture_ids[i]) == chunk.texture_bindings.size();
            if ((chunk.bindings.size()+additional > 10 || chunk.texture_bindings.size()+additional_textures > 10 ||
                 chunk.corners.size()+3 > max_corners) && !flush()) return false;
            for (unsigned i = 0; i < count; ++i)
                if (palette_slot(chunk, *requested[i]) == chunk.bindings.size()) chunk.bindings.push_back(*requested[i]);
            for (unsigned i = 0; i < texture_count; ++i)
                if (texture_slot(chunk, *textures[i], texture_ids[i]) == chunk.texture_bindings.size()) {
                    chunk.texture_bindings.push_back(*textures[i]);chunk.texture_ids.push_back(texture_ids[i]);
                }
            for (unsigned c = 0; c < 3; ++c) {
                const unsigned corner = part.triangles[start + c];
                const auto& weights = skin.vertices[part.corners[corner].vertex].skin;
                const unsigned slot = palette_slot(chunk, weights);
                chunk.corners.push_back(corner); chunk.slots.push_back((u8)slot);
            }
        }
        if (!flush()) return false;
    }
    return previous != 0 && (!local_positions || nativeMeshVertexIds_Write(dat, vertex_ids));
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
    std::unique_ptr<NativeFighterAuthoring,decltype(&std::free)> authoring(nullptr,&std::free);
    if(const auto* source=nativeFighterMeshSource_Get(project,(uint32_t)model.meshes[mesh].offset)) {
        const unsigned owner=model.meshes[mesh].joint;
        bool rigid=false,other=false;
        for(const auto& part:skin.parts) {
            rigid|=(part.flags&0x3000)==POBJ_SKIN;other|=(part.flags&0x3000)!=POBJ_SKIN;
        }
        if(rigid && (model.joints[owner].flags&JOBJ_SKELETON_ROOT)) {
            if(other){fail(error,error_size,"This imported mesh has several source coordinate domains.");return false;}
            for(const auto& part:skin.parts)for(const auto& binding:part.bindings)
                if(binding.count!=1 || binding.joints[0]!=owner){fail(error,error_size,"This imported mesh has several source coordinate domains.");return false;}
            Mtx inverse,conversion;bool different=false;
            if(!nativeSkinMesh_InverseBind(model,owner,inverse)||!C_MTXInverse(inverse,conversion))return false;
            for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)different|=conversion[r][c]!=(r==c?1.f:0.f);
            if(different) {
                // The reader converts rigid root-local geometry into bind
                // coordinates before painting. Reimport needs that same basis.
                Mtx previous,composed;C_MTXIdentity(previous);
                if(source->has_rest_adjustment)for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)
                    previous[r][c]=source->rest_adjustment[r+c*4];
                C_MTXConcat(conversion,previous,composed);
                auto updated=*source;updated.has_rest_adjustment=true;
                std::memset(updated.rest_adjustment,0,sizeof(updated.rest_adjustment));updated.rest_adjustment[15]=1;
                for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)updated.rest_adjustment[r+c*4]=composed[r][c];
                authoring.reset((NativeFighterAuthoring*)std::malloc(sizeof(NativeFighterAuthoring)));
                if(!authoring)return false;
                *authoring=*project->authoring;
                auto candidate=*project;candidate.authoring=authoring.get();
                if(!nativeFighterMeshSource_Set(&candidate,&updated))return false;
            }
        }
    }
    NativeAssetDat dat;
    std::vector<u8> converted;
    uint32_t polygon;
    if (!dat.open(project->costume_dat, project->costume_size) || !nativeSkinMesh_Encode(dat, model, skin, &polygon)) {
        fail(error, error_size, "Could not rebuild this mesh's weights and vertex attributes."); return false;
    }
    dat.link(model.meshes[mesh].offset+12,polygon);
    if(!dat.finish(&converted,UINT32_MAX)) {
        fail(error,error_size,"Could not encode the painted costume.");return false;
    }
    if (!nativeFighterCostume_Compact(project, &converted, nullptr, error, error_size)) return false;
    FtCostumeAssets imported;
    if (!ftCostumeAssets_Load(converted.data(), converted.size(), model.root_name, nullptr, &imported)) {
        fail(error, error_size, "The painted model failed geometry validation."); return false;
    }
    std::free(imported.arena);
    u8* bytes = (u8*)std::malloc(converted.size());
    if (!bytes) return false;
    std::memcpy(bytes, converted.data(), converted.size());
    std::free(project->costume_dat); project->costume_dat = bytes; project->costume_size = converted.size();
    if(authoring){std::free(project->authoring);project->authoring=authoring.release();}
    if (affected) *affected = changed;
    return true;
}
