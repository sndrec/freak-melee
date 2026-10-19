#include "native_fighter_skin_mesh.h"
#include "native_fighter_skin_morph.h"
#include "native_asset_dat.h"
#include "native_mesh_vertex_ids.h"
extern "C" {
#include <melee/ft/ftcostume_assets.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mtx.h>
}
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <unordered_map>

namespace {
bool reference(const HSD_Dat& dat, u32 slot, u32* target)
{
    const char* external;
    return HSD_DatResolve(&dat, slot, target, &external) == HSD_DAT_REF_LOCAL &&
           *target < dat.data_size;
}
u16 half(const u8* p) { return u16(p[0]) << 8 | p[1]; }
}
bool nativeSkinMesh_ReadAttributes(const HSD_Dat& dat, uint32_t source,
                 std::vector<NativeSkinAttribute>* output)
{
    bool seen[26] = {};
    for (unsigned i = 0; i < 27; ++i, source += 24) {
        u32 attr;
        if (!HSD_DatReadU32(&dat, source, &attr)) return false;
        if (attr == 255) {
            std::sort(output->begin(), output->end(), [](const auto& a, const auto& b) {
                return (a.attr == 25 ? 10 : a.attr) < (b.attr == 25 ? 10 : b.attr);
            });
            return seen[9];
        }
        const u8* bytes = HSD_DatBytes(&dat, source, 24);
        const unsigned hardware = attr == 25 ? 10 : attr;
        if (!bytes || hardware > 20 || seen[hardware]) return false;
        seen[hardware] = true;
        NativeSkinAttribute a = {};
        a.source = source; a.attr = attr;
        a.type = NativeAssetDat::read32(bytes + 4);
        a.count = NativeAssetDat::read32(bytes + 8);
        a.format = NativeAssetDat::read32(bytes + 12);
        a.fraction = hardware == 10 ? (a.format <= 1 ? 6 : a.format <= 3 ? 14 : 0) : bytes[16];
        a.stride = half(bytes + 18);
        if (!a.type) continue;
        if (a.type > 3 || (attr <= 8 && a.type != 1)) return false;
        if (attr <= 8) a.width = a.components = 1;
        else if (attr == 11 || attr == 12) {
            const unsigned sizes[] = { 2, 3, 4, 2, 3, 4 };
            if (a.format > 5 || a.count > 1) return false;
            a.width = sizes[a.format];
        } else {
            if (a.format > 4 || a.fraction > 31 || a.count > (hardware == 10 ? 2u : 1u)) return false;
            a.components = attr == 9 ? (a.count ? 3 : 2) :
                hardware == 10 ? (a.count == 1 || (a.count == 2 && a.type == 1) ? 9 : 3) :
                a.count ? 2 : 1;
            a.width = a.components * (a.format <= 1 ? 1 : a.format <= 3 ? 2 : 4);
        }
        a.display_bytes = a.type == 1 ? a.width : (a.type == 2 ? 1 : 2);
        if (hardware == 10 && a.count == 2 && a.type != 1) a.display_bytes *= 3;
        if (a.type > 1) {
            u32 array;
            if (!a.stride || !reference(dat, source + 20, &array)) return false;
            a.array = (uint32_t)array;
        }
        output->push_back(a);
    }
    return false;
}
namespace {
float scalar(const NativeSkinAttribute& a, const u8* p)
{
    if (a.format == 4) {
        const u32 bits = NativeAssetDat::read32(p);
        float value; std::memcpy(&value, &bits, 4); return value;
    }
    const float value = a.format == 0 ? p[0] : a.format == 1 ? (int8_t)p[0] :
        a.format == 2 ? (float)half(p) : (float)(int16_t)half(p);
    return std::ldexp(value, -(int)a.fraction);
}
bool vectors(const HSD_Dat& dat, const NativeSkinAttribute& a, u32 display,
              float* output, u32* source)
{
    const unsigned groups = (a.attr == 10 || a.attr == 25) && a.count == 2 && a.type != 1 ? 3 : 1;
    const unsigned width = a.format <= 1 ? 1 : a.format <= 3 ? 2 : 4;
    for (unsigned group = 0; group < groups; ++group) {
        size_t offset = display;
        if (a.type != 1) {
            const unsigned step = a.type == 2 ? 1 : 2;
            const u8* index = HSD_DatBytes(&dat, display + group * step, step);
            if (!index) return false;
            offset = size_t(a.array) + size_t(a.type == 2 ? index[0] : half(index)) * a.stride;
        }
        if (offset > UINT32_MAX) return false;
        const u8* bytes = HSD_DatBytes(&dat, (u32)offset, a.width);
        if (!bytes) return false;
        if (source) *source = (u32)offset;
        for (unsigned c = 0; c < a.components; ++c) {
            const float value = scalar(a, bytes + c * width);
            if (!std::isfinite(value)) return false;
            output[group * 3 + c] = value;
        }
    }
    return true;
}
int joint_index(const NativeFighterModelEditor& model, u32 offset)
{
    for (unsigned i = 0; i < model.joint_count; ++i)
        if (model.joints[i].offset == offset) return (int)i;
    return -1;
}
bool bindings(const NativeFighterModelEditor& model, u32 table,
               std::vector<NativeSkinWeights>* groups, bool single[10])
{
    for (unsigned slot = 0; slot < 11; ++slot) {
        u32 entry, value;
        if (!reference(model.dat, table + slot * 4, &entry))
            return HSD_DatReadU32(&model.dat, table + slot * 4, &value) && value == 0 && !groups->empty();
        if (slot == 10) return false;
        NativeSkinWeights weights = {};
        double sum = 0;
        for (;;) {
            u32 offset;
            if (!reference(model.dat, entry, &offset)) {
                if (!HSD_DatReadU32(&model.dat, entry, &value) || value) return false;
                break;
            }
            if (weights.count == NATIVE_SKIN_MAX_INFLUENCES) return false;
            const int joint = joint_index(model, offset);
            float weight;
            if (joint < 0 || !HSD_DatReadF32(&model.dat, entry + 4, &weight) ||
                !std::isfinite(weight) || weight < 0) return false;
            if (!weights.count && weight >= 1.0f) {
                single[slot] = true;
                weight = 1.0f;
            }
            weights.joints[weights.count] = (u16)joint;
            weights.weights[weights.count++] = weight;
            sum += weight; entry += 8;
            if (single[slot]) break;
        }
        if (std::fabs(sum - 1.0) > .01) return false;
        // Reading an existing envelope must preserve its authored float values.
        // Normalizing again on every stroke changes untouched stock vertices.
        NativeSkinWeights ordered = {};
        double sums[NATIVE_SKIN_MAX_INFLUENCES] = {};
        for (unsigned i = 0; i < weights.count; ++i) {
            if (weights.weights[i] == 0) continue;
            unsigned at = 0;
            while (at < ordered.count && ordered.joints[at] < weights.joints[i]) ++at;
            if (at == ordered.count || ordered.joints[at] != weights.joints[i]) {
                for (unsigned j = ordered.count; j > at; --j) {
                    ordered.joints[j] = ordered.joints[j-1]; sums[j] = sums[j-1];
                }
                ordered.joints[at] = weights.joints[i]; sums[at] = 0; ++ordered.count;
            }
            sums[at] += weights.weights[i];
        }
        if (!ordered.count) return false;
        for (unsigned i = 0; i < ordered.count; ++i) ordered.weights[i] = (float)sums[i];
        weights = ordered;
        groups->push_back(weights);
    }
    return false;
}
struct Identity {
    u64 source;
    NativeSkinVertex vertex;
};
u64 identity_hash(const Identity& key)
{
    u64 hash = 1469598103934665603ULL;
    auto add = [&](u32 value) { hash = (hash ^ value) * 1099511628211ULL; };
    add((u32)key.source); add((u32)(key.source >> 32));
    for (float value : key.vertex.position) {
        u32 bits; std::memcpy(&bits, &value, 4); add(bits);
    }
    for (unsigned i = 0; i < key.vertex.skin.count; ++i) {
        u32 bits; std::memcpy(&bits, &key.vertex.skin.weights[i], 4);
        add(key.vertex.skin.joints[i]); add(bits);
    }
    return hash;
}
}

bool nativeSkinMesh_InverseBind(const NativeFighterModelEditor& model,
                                unsigned joint, Mtx output)
{
    if (joint >= model.joint_count) return false;
    u32 source;
    if (!reference(model.dat, model.joints[joint].offset + 56, &source)) {
        u32 value;
        if (!(model.joints[joint].flags & JOBJ_SKELETON_ROOT) ||
            !HSD_DatReadU32(&model.dat, model.joints[joint].offset + 56, &value) || value) return false;
        C_MTXIdentity(output); return true;
    }
    for (unsigned r = 0; r < 3; ++r) for (unsigned c = 0; c < 4; ++c)
        if (!HSD_DatReadF32(&model.dat, source + (r * 4 + c) * 4, &output[r][c]) ||
            !std::isfinite(output[r][c])) return false;
    return true;
}

bool nativeSkinMesh_Read(const NativeFighterModelEditor& model, unsigned mesh,
                         NativeSkinMesh* out, char* error, size_t error_size)
{
    auto fail = [&]() {
        if (error && error_size) std::snprintf(error, error_size,
            "This mesh has unsupported or invalid skin, morph or vertex data.");
        return false;
    };
    if (!out || mesh >= model.mesh_count) return fail();
    *out = {};
    const auto& dat = model.dat;
    std::vector<NativeMeshVertexIdRow> vertex_ids;
    if (!nativeMeshVertexIds_Read(dat, &vertex_ids)) return fail();
    const unsigned owner = model.meshes[mesh].joint;
    const bool root = (model.joints[owner].flags & JOBJ_SKELETON_ROOT) != 0;
    u32 polygon;
    if (!reference(dat, model.meshes[mesh].offset + 12, &polygon)) return fail();
    std::unordered_map<u64, std::vector<unsigned>> lookup;
    std::vector<Identity> identities;
    FtCostumeAssets imported = {};
    std::unique_ptr<void, decltype(&std::free)> imported_arena(nullptr, &std::free);
    for (unsigned part_index = 0; part_index < model.meshes[mesh].polygon_count; ++part_index) {
        const u8* header = HSD_DatBytes(&dat, polygon, 24);
        if (!header) return fail();
        NativeSkinPart part = {};
        NativeSkinTextureReader texture_reader;
        part.source = polygon; part.flags = half(header + 12);
        const auto* ids = nativeMeshVertexIds_Find(vertex_ids, (uint32_t)polygon);
        const unsigned type = part.flags & 0x3000;
        if (type == POBJ_SHAPEANIM) {
            if (!imported_arena) {
                if (!ftCostumeAssets_Load(model.project->costume_dat, model.project->costume_size,
                                         model.root_name, nullptr, &imported)) return fail();
                imported_arena.reset(imported.arena);
            }
            if (!nativeSkinMorph_Read(model, owner, imported, &part, out)) return fail();
            identities.resize(out->vertices.size());
            out->parts.push_back(std::move(part));
            if (part_index + 1 < model.meshes[mesh].polygon_count &&
                !reference(dat, polygon + 4, &polygon)) return fail();
            continue;
        }
        if (type != POBJ_SKIN && type != POBJ_ENVELOPE) return fail();
        u32 descriptors, display, table;
        if (!reference(dat, polygon + 8, &descriptors) ||
            !nativeSkinMesh_ReadAttributes(dat, descriptors, &part.attributes) ||
            !reference(dat, polygon + 16, &display)) return fail();
        bool single[10] = {};
        int rigid_bone=-1;
        if (type == POBJ_ENVELOPE) {
            if (!reference(dat, polygon + 20, &table) ||
                !bindings(model, table, &part.bindings, single)) return fail();
        } else {
            int bone = (int)owner;
            while (bone >= 0 && !(model.joints[bone].flags & 3)) bone = model.joints[bone].parent;
            Mtx inverse;
            if (bone < 0 || !nativeSkinMesh_InverseBind(model, bone, inverse)) return fail();
            if (!root && (model.joints[bone].flags & JOBJ_SKELETON_ROOT)) {
                for (unsigned r = 0; r < 3; ++r) for (unsigned c = 0; c < 4; ++c)
                    if (inverse[r][c] != (r == c ? 1.0f : 0.0f)) return fail();
            }
            NativeSkinWeights rigid = {}; rigid.count = 1;
            rigid.joints[0] = (u16)bone; rigid.weights[0] = 1;
            part.bindings.push_back(rigid);
            rigid_bone=bone;
            u32 secondary;const char* external;
            const auto reference_kind=HSD_DatResolve(&dat,polygon+20,&secondary,&external);
            if(reference_kind==HSD_DAT_REF_LOCAL) {
                const int other=joint_index(model,secondary);
                if(other<0)return fail();
                // A helper owner contributes an animated model-node transform.
                // Its own two rigid slots can share the nearest skeleton bone;
                // a distinct second joint needs a separate coordinate domain.
                if(!root && !(model.joints[owner].flags & JOBJ_SKELETON) && other!=(int)owner)
                    return fail();
                rigid.joints[0]=(u16)(other==(int)owner?bone:other);
                part.bindings.push_back(rigid);
            } else if(reference_kind!=HSD_DAT_REF_NULL)return fail();
        }
        Mtx conversion[10], normal_conversion[10];
        for (unsigned slot = 0; slot < part.bindings.size(); ++slot) {
            C_MTXIdentity(conversion[slot]); C_MTXIdentity(normal_conversion[slot]);
            if (root && (type == POBJ_SKIN || single[slot])) {
                Mtx inverse;
                if (!nativeSkinMesh_InverseBind(model, part.bindings[slot].joints[0], inverse) ||
                    !C_MTXInverse(inverse, conversion[slot])) return fail();
                for (unsigned r = 0; r < 3; ++r) for (unsigned c = 0; c < 3; ++c)
                    normal_conversion[slot][r][c] = inverse[c][r];
            } else if(type==POBJ_SKIN && (int)part.bindings[slot].joints[0]!=rigid_bone) {
                // HSD's non-root envelope domain is inverse(owner bind).
                // Put the secondary joint's local vertices in that domain.
                Mtx owner_inverse,other_inverse,other_bind,normal_inverse;
                if(!nativeSkinMesh_InverseBind(model,owner,owner_inverse) ||
                   !nativeSkinMesh_InverseBind(model,part.bindings[slot].joints[0],other_inverse) ||
                   !C_MTXInverse(other_inverse,other_bind))return fail();
                C_MTXConcat(owner_inverse,other_bind,conversion[slot]);
                if(!C_MTXInverse(conversion[slot],normal_inverse))return fail();
                for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)
                    normal_conversion[slot][r][c]=normal_inverse[c][r];
            }
        }
        const size_t bytes = size_t(half(header + 14)) * 32;
        if (!HSD_DatBytes(&dat, display, bytes)) return fail();
        size_t cursor = display, end = cursor + bytes;
        while (cursor < end) {
            const unsigned command = dat.data[cursor++];
            if (!command) continue;
            if ((command != 0x80 && command != 0x90 && command != 0x98 && command != 0xA0) ||
                end - cursor < 2) return fail();
            const unsigned count = half(dat.data + cursor); cursor += 2;
            if (count < 3 || (command == 0x80 && count % 4) ||
                (command == 0x90 && count % 3)) return fail();
            const unsigned start = (unsigned)part.corners.size();
            for (unsigned i = 0; i < count; ++i) {
                NativeSkinCorner corner = {}; corner.source = (u32)cursor;
                NativeSkinVertex vertex = {};
                unsigned textures[8] = {}, texture_mask = 0;
                u32 position_source = 0;
                for (const auto& attr : part.attributes) {
                    if (cursor + attr.display_bytes > end) return fail();
                    if (attr.attr == 0) {
                        if (dat.data[cursor] % 3) return fail();
                        corner.matrix = dat.data[cursor] / 3;
                    } else if (attr.attr >= 1 && attr.attr <= 8) {
                        textures[attr.attr-1] = dat.data[cursor];
                        texture_mask |= 1u << (attr.attr-1);
                    } else if (attr.attr == 9) {
                        if (!vectors(dat, attr, (u32)cursor, vertex.position, &position_source)) return fail();
                    } else if (attr.attr == 10 || attr.attr == 25) {
                        if (!vectors(dat, attr, (u32)cursor, corner.normal, nullptr)) return fail();
                    } else if (attr.type > 1) {
                        const unsigned index = attr.type == 2 ? dat.data[cursor] : half(dat.data + cursor);
                        const size_t offset = size_t(attr.array) + size_t(index) * attr.stride;
                        if (offset > UINT32_MAX || !HSD_DatBytes(&dat, (u32)offset, attr.width)) return fail();
                    }
                    cursor += attr.display_bytes;
                }
                if (corner.matrix >= part.bindings.size()) return fail();
                for (unsigned channel = 0; channel < 8; ++channel) {
                    if (!(texture_mask & (1u << channel))) continue;
                    if (!nativeSkinMesh_ReadTexture(texture_reader, part, corner, channel, textures[channel],
                                                     corner.matrix, part.bindings)) return fail();
                    if (corner.textures[channel] >= NATIVE_SKIN_TEXTURE_IDENTITY) continue;
                    const unsigned slot = (textures[channel]-30)/3;
                    for (unsigned r=0; r<3; ++r) for (unsigned c=0; c<3; ++c)
                        if (conversion[slot][r][c] != conversion[corner.matrix][r][c]) {
                            if (error && error_size) std::snprintf(error,error_size,
                                "This texture attachment uses a different normal coordinate basis.");
                            return false;
                        }
                }
                vertex.skin = part.bindings[corner.matrix];
                Vec3 point = { vertex.position[0], vertex.position[1], vertex.position[2] }, converted;
                C_MTXMultVec(conversion[corner.matrix], &point, &converted);
                std::memcpy(vertex.position, &converted, sizeof(converted));
                for (float value : vertex.position) if (!std::isfinite(value)) return fail();
                for (unsigned n = 0; n < 3; ++n) {
                    std::memcpy(&point, corner.normal + n * 3, sizeof(point));
                    C_MTXMultVecSR(normal_conversion[corner.matrix], &point, &converted);
                    std::memcpy(corner.normal + n * 3, &converted, sizeof(converted));
                }
                for (float value : corner.normal) if (!std::isfinite(value)) return fail();
                if (ids && part.corners.size() >= ids->ids.size()) return fail();
                const u64 identity = ids ? (UINT64_C(1) << 32) | ids->ids[part.corners.size()] : position_source;
                Identity key = { identity, vertex };
                auto& bucket = lookup[identity_hash(key)];
                unsigned id = UINT32_MAX;
                for (unsigned candidate : bucket) {
                    const auto& existing = identities[candidate];
                    if (existing.source == key.source &&
                        !std::memcmp(existing.vertex.position, vertex.position, sizeof(vertex.position)) &&
                        nativeSkinWeights_Equal(&existing.vertex.skin, &vertex.skin)) { id = candidate; break; }
                }
                if (id == UINT32_MAX) {
                    id = (unsigned)out->vertices.size();
                    bucket.push_back(id); identities.push_back(key); out->vertices.push_back(vertex);
                }
                corner.vertex = id;
                out->corner_vertices.push_back(id); part.corners.push_back(corner);
            }
            auto triangle = [&](unsigned a, unsigned b, unsigned c) {
                part.triangles.insert(part.triangles.end(), { start + a, start + b, start + c });
            };
            if (command == 0x90) for (unsigned i = 0; i < count; i += 3) triangle(i, i + 1, i + 2);
            else if (command == 0x80) for (unsigned i = 0; i < count; i += 4) {
                triangle(i, i + 1, i + 2); triangle(i, i + 2, i + 3);
            } else for (unsigned i = 2; i < count; ++i) {
                if (command == 0xA0) triangle(0, i - 1, i);
                else triangle(i % 2 ? i - 1 : i - 2, i % 2 ? i - 2 : i - 1, i);
            }
        }
        if (part.corners.empty() || (ids && ids->ids.size() != part.corners.size())) return fail();
        out->parts.push_back(std::move(part));
        if (part_index + 1 < model.meshes[mesh].polygon_count &&
            !reference(dat, polygon + 4, &polygon)) return fail();
    }
    return !out->vertices.empty();
}
