#include "native_fighter_gltf_import.h"
#include "native_fighter_costume_dat.h"

#ifdef MELEE_NATIVE

#define CGLTF_IMPLEMENTATION
#include "../../../extern/cgltf/cgltf.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <sysdolphin/baselib/dat.h>
#include <melee/ft/native_fighter_model_editor.h>
#include <melee/ft/native_gx_image.h>
#include <melee/ft/ftcostume_assets.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/displayfunc.h>
#include <sysdolphin/baselib/tobj.h>
#ifdef __cplusplus
}
#endif

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <limits>
#include <string>
#include <vector>

namespace {

void fail(char* error, size_t capacity, const char* message)
{
    if (error && capacity) std::snprintf(error, capacity, "%s", message);
}

struct Source {
    cgltf_data* data = nullptr;
    ~Source() { cgltf_free(data); }
    bool open(const char* path, char* error, size_t capacity)
    {
        cgltf_options options = {};
        if (!path || cgltf_parse_file(&options, path, &data) !=
                         cgltf_result_success ||
            cgltf_load_buffers(&options, data, path) != cgltf_result_success ||
            cgltf_validate(data) != cgltf_result_success) {
            fail(error, capacity, "Could not parse this glTF/GLB and its buffers.");
            return false;
        }
        return true;
    }
};

struct Primitive {
    const cgltf_primitive* mesh = nullptr;
    const cgltf_node* node = nullptr;
    const char* name = nullptr;
};

bool find_primitive(const cgltf_data* data, unsigned wanted, Primitive* out,
                    unsigned* count)
{
    unsigned cursor = 0;
    // A mesh can be instanced by multiple scene nodes. Import each source
    // primitive once, using its first node's placement as the starting pose.
    for (cgltf_size mesh = 0; mesh < data->meshes_count; ++mesh) {
        const cgltf_node* node = nullptr;
        for (cgltf_size n = 0; n < data->nodes_count; ++n)
            if (data->nodes[n].mesh == &data->meshes[mesh]) {
                node = &data->nodes[n];
                break;
            }
        for (cgltf_size p = 0; p < data->meshes[mesh].primitives_count; ++p) {
            if (cursor == wanted && out) {
                out->mesh = &data->meshes[mesh].primitives[p];
                out->node = node;
                out->name = data->meshes[mesh].name;
            }
            ++cursor;
        }
    }
    if (count) *count = cursor;
    return wanted < cursor;
}

const cgltf_accessor* attribute(const cgltf_primitive* primitive,
                                cgltf_attribute_type type)
{
    for (cgltf_size i = 0; i < primitive->attributes_count; ++i)
        if (primitive->attributes[i].type == type &&
            primitive->attributes[i].index == 0)
            return primitive->attributes[i].data;
    return nullptr;
}

void be16(uint8_t* bytes, uint16_t value)
{
    bytes[0] = value >> 8;
    bytes[1] = value;
}

void be32(uint8_t* bytes, uint32_t value)
{
    bytes[0] = value >> 24;
    bytes[1] = value >> 16;
    bytes[2] = value >> 8;
    bytes[3] = value;
}

uint32_t read32(const uint8_t* bytes)
{
    return uint32_t(bytes[0]) << 24 | uint32_t(bytes[1]) << 16 |
           uint32_t(bytes[2]) << 8 | bytes[3];
}

uint16_t read16(const uint8_t* bytes)
{
    return uint16_t(bytes[0]) << 8 | bytes[1];
}

float readfloat(const uint8_t* bytes)
{
    const uint32_t bits = read32(bytes);
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

void befloat(uint8_t* bytes, float value)
{
    uint32_t bits;
    std::memcpy(&bits, &value, 4);
    be32(bytes, bits);
}


bool encode_attribute(NativeCostumeDat* dat, uint32_t descriptor,
                      uint32_t array, unsigned count, unsigned components,
                      unsigned attribute_id, unsigned component_count)
{
    if (array == UINT32_MAX) return false;
    dat->word(descriptor, attribute_id);
    dat->word(descriptor + 4, 3); // GX_INDEX16
    dat->word(descriptor + 8, component_count);
    dat->word(descriptor + 12, 4); // GX_F32
    dat->half(descriptor + 18, components * 4);
    dat->link(descriptor + 20, array);
    return count != 0;
}

struct SkinBone {
    uint16_t joint;
    int16_t parent;
    float position[3];
    float parent_position[3];
};

struct SkinBinding {
    std::vector<SkinBone> bones;
};

struct SkinPalette {
    uint16_t joint[2];
    uint8_t weight; // first joint, in thirty-seconds; second gets the rest
    bool operator==(const SkinPalette& other) const {
        return joint[0] == other.joint[0] &&
               joint[1] == other.joint[1] && weight == other.weight;
    }
};

bool invert_affine(const float m[12], float out[12])
{
    const float c[9] = {
        m[5] * m[10] - m[6] * m[9],
        m[6] * m[8] - m[4] * m[10],
        m[4] * m[9] - m[5] * m[8],
        m[9] * m[2] - m[10] * m[1],
        m[10] * m[0] - m[8] * m[2],
        m[8] * m[1] - m[9] * m[0],
        m[1] * m[6] - m[2] * m[5],
        m[2] * m[4] - m[0] * m[6],
        m[0] * m[5] - m[1] * m[4],
    };
    const float determinant = m[0] * c[0] + m[1] * c[1] + m[2] * c[2];
    if (!std::isfinite(determinant) || std::fabs(determinant) < 1e-9f)
        return false;
    for (unsigned row = 0; row < 3; ++row)
        for (unsigned column = 0; column < 3; ++column)
            out[row * 4 + column] = c[column * 3 + row] / determinant;
    for (unsigned row = 0; row < 3; ++row) {
        out[row * 4 + 3] = 0.0f;
        for (unsigned column = 0; column < 3; ++column)
            out[row * 4 + 3] -= out[row * 4 + column] *
                                m[column * 4 + 3];
    }
    return true;
}

bool read_bind_inverse(const NativeFighterModelEditor& model,
                       unsigned joint, float out[12])
{
    u32 offset;
    const char* external;
    if (joint >= model.joint_count ||
        HSD_DatResolve(&model.dat, model.joints[joint].offset + 56,
                       &offset, &external) != HSD_DAT_REF_LOCAL ||
        HSD_DatBytes(&model.dat, offset, 48) == nullptr) return false;
    for (unsigned i = 0; i < 12; ++i)
        if (!HSD_DatReadF32(&model.dat, offset + i * 4, &out[i]) ||
            !std::isfinite(out[i])) return false;
    return true;
}

bool prepare_skin(const NativeFighterModelEditor& model, unsigned owner,
                  SkinBinding* skin, char* error, size_t error_size)
{
    if (!(model.joints[owner].flags & JOBJ_SKELETON_ROOT) ||
        !(model.joints[owner].flags & 0x40000)) {
        fail(error, error_size,
             "Automatic skinning needs a visible skeleton root on this fighter.");
        return false;
    }
    for (unsigned i = 0; i < model.joint_count; ++i) {
        float inverse[12], bind[12];
        if (!(model.joints[i].flags & JOBJ_SKELETON) ||
            !read_bind_inverse(model, i, inverse) ||
            !invert_affine(inverse, bind)) continue;
        SkinBone bone = {};
        bone.joint = (uint16_t) i;
        bone.parent = model.joints[i].parent;
        for (unsigned axis = 0; axis < 3; ++axis)
            bone.position[axis] = bind[axis * 4 + 3];
        skin->bones.push_back(bone);
    }
    if (skin->bones.empty()) {
        fail(error, error_size, "This fighter has no deform bones to skin to.");
        return false;
    }
    for (SkinBone& bone : skin->bones) {
        const SkinBone* ancestor = nullptr;
        int parent = bone.parent;
        while (parent >= 0 && ancestor == nullptr) {
            for (const SkinBone& candidate : skin->bones)
                if (candidate.joint == parent) {
                    ancestor = &candidate;
                    break;
                }
            if (!ancestor) parent = model.joints[parent].parent;
        }
        for (unsigned axis = 0; axis < 3; ++axis)
            bone.parent_position[axis] = ancestor
                ? ancestor->position[axis] : bone.position[axis];
    }
    return true;
}

SkinPalette automatic_palette(const SkinBinding& skin, const float p[3])
{
    float best[2] = { std::numeric_limits<float>::infinity(),
                      std::numeric_limits<float>::infinity() };
    unsigned chosen[2] = { 0, 0 };
    for (unsigned i = 0; i < skin.bones.size(); ++i) {
        const SkinBone& bone = skin.bones[i];
        float along = 0.0f, length_squared = 0.0f;
        for (unsigned axis = 0; axis < 3; ++axis) {
            const float delta = bone.position[axis] -
                                bone.parent_position[axis];
            along += (p[axis] - bone.parent_position[axis]) * delta;
            length_squared += delta * delta;
        }
        const float t = length_squared > 1e-5f
            ? std::clamp(along / length_squared, 0.0f, 1.0f) : 0.0f;
        float distance = 0.0f;
        for (unsigned axis = 0; axis < 3; ++axis) {
            const float closest = bone.parent_position[axis] + t *
                (bone.position[axis] - bone.parent_position[axis]);
            const float delta = p[axis] - closest;
            distance += delta * delta;
        }
        if (distance < best[0]) {
            best[1] = best[0]; chosen[1] = chosen[0];
            best[0] = distance; chosen[0] = i;
        } else if (distance < best[1]) {
            best[1] = distance; chosen[1] = i;
        }
    }
    SkinPalette palette = {};
    palette.joint[0] = skin.bones[chosen[0]].joint;
    if (skin.bones.size() == 1) {
        palette.joint[1] = palette.joint[0];
        palette.weight = 32;
    } else {
        palette.joint[1] = skin.bones[chosen[1]].joint;
        const float first = 1.0f / (best[0] + 0.25f);
        const float second = 1.0f / (best[1] + 0.25f);
        palette.weight = (uint8_t) std::clamp(
            std::lround(32.0f * first / (first + second)), 0l, 32l);
        if (palette.weight == 0) {
            palette.joint[0] = palette.joint[1];
            palette.weight = 32;
        } else if (palette.weight == 32) {
            palette.joint[1] = palette.joint[0];
        }
    }
    return palette;
}

struct SkinChunk {
    std::vector<uint16_t> indices;
    std::vector<uint8_t> palette_index;
    std::vector<SkinPalette> palettes;
};

bool write_skin_chunk(NativeCostumeDat* dat, const SkinChunk& chunk,
                      uint32_t descriptors,
                      const NativeFighterModelEditor& model,
                      bool uv, uint32_t* polygon)
{
    std::vector<uint8_t> display;
    display.reserve(3 + chunk.indices.size() * (uv ? 7 : 5));
    display.push_back(0x90); // GX_TRIANGLES / VAT 0.
    display.push_back((uint8_t) (chunk.indices.size() >> 8));
    display.push_back((uint8_t) chunk.indices.size());
    for (size_t i = 0; i < chunk.indices.size(); ++i) {
        display.push_back((uint8_t) (chunk.palette_index[i] * 3));
        for (unsigned attr = 0; attr < (uv ? 3u : 2u); ++attr) {
            display.push_back((uint8_t) (chunk.indices[i] >> 8));
            display.push_back((uint8_t) chunk.indices[i]);
        }
    }
    display.resize((display.size() + 31) & ~size_t(31), 0);
    if (display.size() / 32 > UINT16_MAX) return false;
    const uint32_t commands = dat->allocate(display.size(), 32);
    const uint32_t table = dat->allocate((chunk.palettes.size() + 1) * 4);
    *polygon = dat->allocate(24);
    if (commands == UINT32_MAX || table == UINT32_MAX ||
        *polygon == UINT32_MAX) return false;
    std::memcpy(dat->data.data() + commands, display.data(), display.size());
    for (unsigned i = 0; i < chunk.palettes.size(); ++i) {
        const SkinPalette& palette = chunk.palettes[i];
        // A skeleton-root DObj has no model-node correction. Even a single
        // influence must take the envelope blending path so its inverse bind
        // matrix is applied, rather than the rigid one-bone shortcut.
        const uint32_t entries = dat->allocate(24);
        if (entries == UINT32_MAX) return false;
        dat->link(table + i * 4, entries);
        dat->link(entries, model.joints[palette.joint[0]].offset);
        befloat(dat->data.data() + entries + 4,
                palette.weight == 32 ? 0.5f : palette.weight / 32.0f);
        dat->link(entries + 8, model.joints[palette.joint[1]].offset);
        befloat(dat->data.data() + entries + 12,
                palette.weight == 32 ? 0.5f :
                    (32 - palette.weight) / 32.0f);
    }
    dat->link(*polygon + 8, descriptors);
    dat->half(*polygon + 12, POBJ_ENVELOPE);
    dat->half(*polygon + 14, (uint16_t) (display.size() / 32));
    dat->link(*polygon + 16, commands);
    dat->link(*polygon + 20, table);
    return true;
}

struct ImportedSkinMesh {
    uint32_t descriptors = 0;
    uint32_t positions = 0;
    bool uv = false;
    std::vector<uint16_t> triangles;
    std::vector<SkinPalette> palettes;
    std::vector<uint8_t> used;
    std::vector<NativeImportedSkinVertex> vertices;
};

bool local_ref(const HSD_Dat& dat, uint32_t slot, uint32_t* target)
{
    const char* external;
    u32 resolved;
    if (HSD_DatResolve(&dat, slot, &resolved, &external) !=
            HSD_DAT_REF_LOCAL || resolved >= dat.data_size) return false;
    *target = (uint32_t) resolved;
    return true;
}

bool imported_skin_mesh(const NativeFighterModelEditor& model,
                        unsigned mesh, ImportedSkinMesh* out)
{
    if (mesh >= model.mesh_count) return false;
    const HSD_Dat& dat = model.dat;
    uint32_t polygon;
    if (!local_ref(dat, model.meshes[mesh].offset + 12, &polygon))
        return false;
    for (unsigned part = 0; part < model.meshes[mesh].polygon_count; ++part) {
        if (!HSD_DatBytes(&dat, polygon, 24)) return false;
        const uint8_t* header = dat.data + polygon;
        if ((read16(header + 12) & 0x3000) != POBJ_ENVELOPE)
            return false;
        uint32_t desc, display, table;
        if (!local_ref(dat, polygon + 8, &desc) ||
            !local_ref(dat, polygon + 16, &display) ||
            !local_ref(dat, polygon + 20, &table) ||
            !HSD_DatBytes(&dat, desc, 96) ||
            read32(dat.data + desc) != GX_VA_PNMTXIDX ||
            read32(dat.data + desc + 4) != GX_DIRECT ||
            read32(dat.data + desc + 24) != GX_VA_POS ||
            read32(dat.data + desc + 28) != 3 ||
            read32(dat.data + desc + 48) != GX_VA_NRM ||
            read32(dat.data + desc + 52) != 3) return false;
        const bool uv = read32(dat.data + desc + 72) == GX_VA_TEX0;
        if (uv && (!HSD_DatBytes(&dat, desc, 120) ||
                   read32(dat.data + desc + 76) != 3)) return false;
        uint32_t positions;
        if (!local_ref(dat, desc + 44, &positions)) return false;
        if (part == 0) {
            out->descriptors = desc;
            out->positions = positions;
            out->uv = uv;
        } else if (out->descriptors != desc || out->positions != positions ||
                   out->uv != uv) return false;
        const size_t bytes = size_t(read16(header + 14)) * 32;
        if (bytes < 3 || !HSD_DatBytes(&dat, display, bytes)) return false;
        const uint8_t* command = dat.data + display;
        if (command[0] != 0x90) return false;
        const unsigned count = read16(command + 1);
        const unsigned stride = uv ? 7 : 5;
        if (count == 0 || count % 3 ||
            3 + size_t(count) * stride > bytes) return false;
        SkinPalette groups[10] = {};
        bool loaded[10] = {};
        for (unsigned i = 0; i < count; ++i) {
            const uint8_t* entry = command + 3 + i * stride;
            const unsigned slot = entry[0] / 3;
            const unsigned index = read16(entry + 1);
            if (entry[0] % 3 || slot >= 10 ||
                read16(entry + 3) != index ||
                (uv && read16(entry + 5) != index) ||
                !HSD_DatBytes(&dat, positions + size_t(index) * 12, 12))
                return false;
            if (!loaded[slot]) {
                uint32_t influences;
                if (!local_ref(dat, table + slot * 4, &influences) ||
                    !HSD_DatBytes(&dat, influences, 24)) return false;
                uint32_t joint_offsets[2];
                float weights[2];
                for (unsigned b = 0; b < 2; ++b) {
                    if (!local_ref(dat, influences + b * 8,
                                   &joint_offsets[b])) return false;
                    weights[b] = readfloat(dat.data + influences + b * 8 + 4);
                    unsigned joint = 0;
                    while (joint < model.joint_count &&
                           model.joints[joint].offset != joint_offsets[b])
                        ++joint;
                    if (joint == model.joint_count) return false;
                    groups[slot].joint[b] = (uint16_t) joint;
                }
                if (!std::isfinite(weights[0]) ||
                    !std::isfinite(weights[1]) ||
                    std::fabs(weights[0] + weights[1] - 1.0f) > 0.01f)
                    return false;
                groups[slot].weight = groups[slot].joint[0] ==
                                      groups[slot].joint[1] ? 32 :
                    (uint8_t) std::clamp(std::lround(weights[0] * 32),
                                         0l, 32l);
                loaded[slot] = true;
            }
            if (index >= out->used.size()) {
                out->used.resize(index + 1, 0);
                out->palettes.resize(index + 1);
                out->vertices.resize(index + 1);
            }
            if (out->used[index] &&
                !(out->palettes[index] == groups[slot])) return false;
            out->used[index] = 1;
            out->palettes[index] = groups[slot];
            out->triangles.push_back((uint16_t) index);
        }
        uint32_t next;
        if (part + 1 < model.meshes[mesh].polygon_count) {
            if (!local_ref(dat, polygon + 4, &next)) return false;
            polygon = next;
        }
    }
    for (unsigned i = 0; i < out->used.size(); ++i) {
        if (!out->used[i]) continue;
        NativeImportedSkinVertex& vertex = out->vertices[i];
        vertex.used = true;
        for (unsigned axis = 0; axis < 3; ++axis) {
            vertex.position[axis] = readfloat(dat.data + out->positions +
                                               i * 12 + axis * 4);
            if (!std::isfinite(vertex.position[axis])) return false;
        }
        vertex.joints[0] = out->palettes[i].joint[0];
        vertex.joints[1] = out->palettes[i].joint[1];
        vertex.weight = out->palettes[i].weight / 32.0f;
    }
    return !out->triangles.empty();
}

struct TransferSkin {
    struct Node {
        unsigned vertex;
        int child[2];
        uint8_t axis;
    };
    const ImportedSkinMesh* source = nullptr;
    std::vector<Node> nodes;
    int root = -1;

    int build(std::vector<unsigned>& vertices, size_t begin, size_t end,
              unsigned axis)
    {
        if (begin == end) return -1;
        const size_t middle = begin + (end - begin) / 2;
        std::nth_element(vertices.begin() + begin,
                         vertices.begin() + middle,
                         vertices.begin() + end,
            [&](unsigned a, unsigned b) {
                const float x = source->vertices[a].position[axis];
                const float y = source->vertices[b].position[axis];
                return x == y ? a < b : x < y;
            });
        const int id = (int) nodes.size();
        nodes.push_back({ vertices[middle], { -1, -1 }, (uint8_t) axis });
        const unsigned next = (axis + 1) % 3;
        const int left = build(vertices, begin, middle, next);
        const int right = build(vertices, middle + 1, end, next);
        nodes[id].child[0] = left;
        nodes[id].child[1] = right;
        return id;
    }

    bool prepare(const ImportedSkinMesh* mesh)
    {
        source = mesh;
        std::vector<unsigned> vertices;
        for (unsigned i = 0; i < mesh->used.size(); ++i)
            if (mesh->used[i]) vertices.push_back(i);
        nodes.reserve(vertices.size());
        root = build(vertices, 0, vertices.size(), 0);
        return root >= 0;
    }

    void nearest(int id, const float point[3], unsigned* best,
                 float* distance_squared) const
    {
        if (id < 0) return;
        const Node& node = nodes[id];
        const float* source_point = source->vertices[node.vertex].position;
        float distance = 0.0f;
        for (unsigned axis = 0; axis < 3; ++axis) {
            const float delta = point[axis] - source_point[axis];
            distance += delta * delta;
        }
        if (distance < *distance_squared) {
            *distance_squared = distance;
            *best = node.vertex;
        }
        const float axis_delta = point[node.axis] - source_point[node.axis];
        const unsigned near_side = axis_delta >= 0 ? 1 : 0;
        nearest(node.child[near_side], point, best, distance_squared);
        if (axis_delta * axis_delta < *distance_squared)
            nearest(node.child[1 - near_side], point, best,
                    distance_squared);
    }

    SkinPalette palette(const float point[3]) const
    {
        unsigned best = 0;
        float distance = std::numeric_limits<float>::infinity();
        nearest(root, point, &best, &distance);
        return source->palettes[best];
    }
};

bool encode_geometry(const Primitive& primitive, NativeCostumeDat* dat,
                     float scale, uint32_t joint_offset,
                     const NativeFighterModelEditor& model,
                     const SkinBinding* skin,
                     const TransferSkin* transfer,
                     uint32_t* polygon, char* error, size_t error_size)
{
    const cgltf_primitive* mesh = primitive.mesh;
    const cgltf_accessor* position = attribute(mesh, cgltf_attribute_type_position);
    const cgltf_accessor* normal = attribute(mesh, cgltf_attribute_type_normal);
    const cgltf_accessor* uv = attribute(mesh, cgltf_attribute_type_texcoord);
    if (mesh->type != cgltf_primitive_type_triangles || !position || !normal ||
        position->count != normal->count ||
        (uv && uv->count != position->count) ||
        position->count == 0 || position->count > UINT16_MAX) {
        fail(error, error_size,
             "Import needs a triangle mesh with up to 65535 vertices and normals.");
        return false;
    }
    const cgltf_size indices = mesh->indices ? mesh->indices->count :
                                 position->count;
    if (indices == 0 || indices % 3 || indices > 1000000) {
        fail(error, error_size, "Mesh indices must contain triangles.");
        return false;
    }
    const unsigned vertices = (unsigned) position->count;
    const uint32_t positions = dat->allocate((size_t) vertices * 12, 32);
    const uint32_t normals = dat->allocate((size_t) vertices * 12, 32);
    const uint32_t uvs = uv ? dat->allocate((size_t) vertices * 8, 32) : 0;
    if (positions == UINT32_MAX || normals == UINT32_MAX ||
        uvs == UINT32_MAX) return false;
    float world[16] = { 1, 0, 0, 0, 0, 1, 0, 0,
                        0, 0, 1, 0, 0, 0, 0, 1 };
    if (primitive.node) cgltf_node_transform_world(primitive.node, world);
    const float cofactors[9] = {
        world[5] * world[10] - world[6] * world[9],
        world[6] * world[8] - world[4] * world[10],
        world[4] * world[9] - world[5] * world[8],
        world[9] * world[2] - world[10] * world[1],
        world[10] * world[0] - world[8] * world[2],
        world[8] * world[1] - world[9] * world[0],
        world[1] * world[6] - world[2] * world[5],
        world[2] * world[4] - world[0] * world[6],
        world[0] * world[5] - world[1] * world[4],
    };
    const float determinant = world[0] * cofactors[0] +
                              world[1] * cofactors[1] +
                              world[2] * cofactors[2];
    if (!std::isfinite(determinant) || std::fabs(determinant) < 1e-9f) {
        fail(error, error_size, "Mesh node has a non-invertible transform.");
        return false;
    }
    std::vector<SkinPalette> vertex_palette;
    if (skin) vertex_palette.resize(vertices);
    for (unsigned i = 0; i < vertices; ++i) {
        float xyz[3], n[3], tex[2];
        if (!cgltf_accessor_read_float(position, i, xyz, 3) ||
            !cgltf_accessor_read_float(normal, i, n, 3) ||
            (uv && !cgltf_accessor_read_float(uv, i, tex, 2)))
            return false;
        float world_position[3];
        for (unsigned axis = 0; axis < 3; ++axis) {
            float p = world[12 + axis];
            for (unsigned k = 0; k < 3; ++k)
                p += world[axis + k * 4] * xyz[k];
            world_position[axis] = p * scale;
            if (!std::isfinite(world_position[axis])) return false;
            befloat(dat->data.data() + positions + i * 12 + axis * 4,
                    world_position[axis]);
        }
        if (skin) vertex_palette[i] = transfer
            ? transfer->palette(world_position)
            : automatic_palette(*skin, world_position);
        float transformed[3];
        for (unsigned axis = 0; axis < 3; ++axis) {
            float value = 0.0f;
            for (unsigned k = 0; k < 3; ++k)
                value += cofactors[axis + k * 3] * n[k] / determinant;
            transformed[axis] = value;
        }
        const float length = std::sqrt(transformed[0] * transformed[0] +
                                       transformed[1] * transformed[1] +
                                       transformed[2] * transformed[2]);
        if (!std::isfinite(length) || length < 1e-9f) return false;
        for (unsigned axis = 0; axis < 3; ++axis)
            befloat(dat->data.data() + normals + i * 12 + axis * 4,
                    transformed[axis] / length);
        if (uv) for (unsigned axis = 0; axis < 2; ++axis)
            befloat(dat->data.data() + uvs + i * 8 + axis * 4, tex[axis]);
    }
    if (skin) {
        const uint32_t descriptors = dat->allocate((uv ? 5u : 4u) * 24);
        if (descriptors == UINT32_MAX) return false;
        dat->word(descriptors, GX_VA_PNMTXIDX);
        dat->word(descriptors + 4, GX_DIRECT);
        if (!encode_attribute(dat, descriptors + 24, positions, vertices,
                              3, GX_VA_POS, GX_POS_XYZ) ||
            !encode_attribute(dat, descriptors + 48, normals, vertices,
                              3, GX_VA_NRM, GX_NRM_XYZ)) return false;
        if (uv && !encode_attribute(dat, descriptors + 72, uvs, vertices,
                                    2, GX_VA_TEX0, GX_TEX_ST)) return false;
        dat->word(descriptors + (uv ? 96 : 72), GX_VA_NULL);
        SkinChunk chunk;
        uint32_t previous = 0;
        *polygon = 0;
        for (cgltf_size start = 0; start < indices; start += 3) {
            uint16_t triangle[3];
            for (unsigned c = 0; c < 3; ++c) {
                const unsigned flipped = determinant < 0.0f
                    ? (c == 1 ? 2 : c == 2 ? 1 : 0) : c;
                const cgltf_size index = mesh->indices
                    ? cgltf_accessor_read_index(mesh->indices, start + flipped)
                    : start + flipped;
                if (index >= vertices) {
                    fail(error, error_size, "Mesh has an out-of-range index.");
                    return false;
                }
                triangle[c] = (uint16_t) index;
            }
            unsigned additional = 0;
            for (unsigned c = 0; c < 3; ++c) {
                bool found = false;
                for (const SkinPalette& p : chunk.palettes)
                    if (p == vertex_palette[triangle[c]]) found = true;
                for (unsigned earlier = 0; earlier < c; ++earlier)
                    if (vertex_palette[triangle[earlier]] ==
                        vertex_palette[triangle[c]]) found = true;
                if (!found) ++additional;
            }
            if (!chunk.indices.empty() &&
                (chunk.palettes.size() + additional > 10 ||
                 chunk.indices.size() + 3 > 65532)) {
                uint32_t emitted;
                if (!write_skin_chunk(dat, chunk, descriptors, model, uv,
                                      &emitted)) return false;
                if (previous) dat->link(previous + 4, emitted);
                else *polygon = emitted;
                previous = emitted;
                chunk = {};
            }
            for (unsigned c = 0; c < 3; ++c) {
                const SkinPalette& palette = vertex_palette[triangle[c]];
                unsigned id = 0;
                for (; id < chunk.palettes.size(); ++id)
                    if (chunk.palettes[id] == palette) break;
                if (id == chunk.palettes.size()) chunk.palettes.push_back(palette);
                chunk.indices.push_back(triangle[c]);
                chunk.palette_index.push_back((uint8_t) id);
            }
        }
        if (!chunk.indices.empty()) {
            uint32_t emitted;
            if (!write_skin_chunk(dat, chunk, descriptors, model, uv,
                                  &emitted)) return false;
            if (previous) dat->link(previous + 4, emitted);
            else *polygon = emitted;
        }
        return *polygon != 0;
    }
    std::vector<uint8_t> display;
    for (cgltf_size start = 0; start < indices; ) {
        const cgltf_size count = std::min<cgltf_size>(indices - start, 65532);
        display.push_back(0x90); // GX_TRIANGLES / VAT 0.
        display.push_back((uint8_t) (count >> 8));
        display.push_back((uint8_t) count);
        for (cgltf_size i = 0; i < count; ++i) {
            const cgltf_size source_index = start +
                (determinant < 0.0f ? i / 3 * 3 +
                    (i % 3 == 1 ? 2 : i % 3 == 2 ? 1 : 0) : i);
            const cgltf_size index = mesh->indices
                ? cgltf_accessor_read_index(mesh->indices, source_index)
                : source_index;
            if (index >= vertices) {
                fail(error, error_size, "Mesh has an out-of-range index.");
                return false;
            }
            for (unsigned a = 0; a < (uv ? 3u : 2u); ++a) {
                display.push_back((uint8_t) (index >> 8));
                display.push_back((uint8_t) index);
            }
        }
        start += count;
    }
    display.resize((display.size() + 31) & ~size_t(31), 0);
    if (display.size() / 32 > UINT16_MAX) {
        fail(error, error_size, "Mesh display list exceeds the Melee limit.");
        return false;
    }
    const uint32_t commands = dat->allocate(display.size(), 32);
    const uint32_t descriptors = dat->allocate((uv ? 4u : 3u) * 24);
    *polygon = dat->allocate(24);
    if (commands == UINT32_MAX || descriptors == UINT32_MAX ||
        *polygon == UINT32_MAX) return false;
    std::memcpy(dat->data.data() + commands, display.data(), display.size());
    if (!encode_attribute(dat, descriptors, positions, vertices, 3, 9, 1) ||
        !encode_attribute(dat, descriptors + 24, normals, vertices, 3, 10, 0))
        return false;
    if (uv && !encode_attribute(dat, descriptors + 48, uvs, vertices, 2, 13, 1))
        return false;
    dat->word(descriptors + (uv ? 72 : 48), 0xFF); // GX_VA_NULL
    dat->link(*polygon + 8, descriptors);
    dat->half(*polygon + 12, 0); // POBJ_SKIN
    dat->half(*polygon + 14, (uint16_t) (display.size() / 32));
    dat->link(*polygon + 16, commands);
    dat->link(*polygon + 20, joint_offset);
    return true;
}

bool load_image_bytes(const char* source_path, const cgltf_image* image,
                      std::vector<uint8_t>* output,
                      char* error, size_t error_size)
{
    if (image->buffer_view) {
        const cgltf_buffer_view* view = image->buffer_view;
        if (!view->buffer || !view->buffer->data ||
            view->size > 8 * 1024 * 1024) return false;
        const uint8_t* bytes = static_cast<const uint8_t*>(
            view->buffer->data) + view->offset;
        output->assign(bytes, bytes + view->size);
        return true;
    }
    if (!image->uri) return false;
    if (std::strncmp(image->uri, "data:", 5) == 0) {
        const char* comma = std::strchr(image->uri, ',');
        if (!comma || std::string(image->uri, (size_t) (comma - image->uri))
                          .find(";base64") ==
                          std::string::npos) return false;
        const char* encoded = comma + 1;
        const size_t length = std::strlen(encoded);
        if (length % 4 || length > 12 * 1024 * 1024) return false;
        const size_t padding = (length && encoded[length - 1] == '=') +
            (length > 1 && encoded[length - 2] == '=');
        const size_t size = length / 4 * 3 - padding;
        void* decoded = nullptr;
        cgltf_options options = {};
        if (cgltf_load_buffer_base64(&options, size, encoded, &decoded) !=
            cgltf_result_success) return false;
        output->assign(static_cast<uint8_t*>(decoded),
                       static_cast<uint8_t*>(decoded) + size);
        std::free(decoded);
        return true;
    }
    std::string decoded_uri = image->uri;
    cgltf_decode_uri(decoded_uri.data());
    decoded_uri.resize(std::strlen(decoded_uri.c_str()));
    const std::filesystem::path path =
        std::filesystem::path(source_path).parent_path() / decoded_uri;
    FILE* file = std::fopen(path.string().c_str(), "rb");
    if (!file) {
        fail(error, error_size, "Could not open an external glTF texture.");
        return false;
    }
    std::fseek(file, 0, SEEK_END);
    const long size = std::ftell(file);
    std::rewind(file);
    if (size <= 0 || size > 8 * 1024 * 1024) {
        std::fclose(file);
        return false;
    }
    output->resize((size_t) size);
    const bool read = std::fread(output->data(), 1, output->size(), file) ==
                      output->size();
    std::fclose(file);
    return read;
}

bool encode_texture_bytes(const uint8_t* bytes, size_t byte_count,
                          cgltf_wrap_mode wrap_s, cgltf_wrap_mode wrap_t,
                          NativeCostumeDat* dat, uint32_t* tobj,
                          char* error, size_t error_size)
{
    unsigned short width = 0, height = 0;
    unsigned char* tiled = nativeGxImage_Decode(bytes,
        (unsigned) byte_count, &width, &height);
    if (!tiled) {
        fail(error, error_size, "Could not decode the glTF PNG/JPEG texture.");
        return false;
    }
    const size_t size = (size_t) ((width + 3) & ~3u) *
                        ((height + 3) & ~3u) * 4;
    const uint32_t image_pixels = dat->allocate(size, 32);
    const uint32_t image_desc = dat->allocate(24);
    *tobj = dat->allocate(92);
    if (image_pixels == UINT32_MAX || image_desc == UINT32_MAX ||
        *tobj == UINT32_MAX) {
        nativeGxImage_Free(tiled);
        return false;
    }
    std::memcpy(dat->data.data() + image_pixels, tiled, size);
    nativeGxImage_Free(tiled);
    dat->link(image_desc, image_pixels);
    dat->half(image_desc + 4, width);
    dat->half(image_desc + 6, height);
    dat->word(image_desc + 8, GX_TF_RGBA8);
    dat->word(*tobj + 8, GX_TEXMAP0);
    dat->word(*tobj + 12, GX_TG_TEX0);
    for (unsigned axis = 0; axis < 3; ++axis)
        befloat(dat->data.data() + *tobj + 28 + axis * 4, 1.0f);
    dat->word(*tobj + 52, wrap_s ==
              cgltf_wrap_mode_clamp_to_edge ? GX_CLAMP : GX_REPEAT);
    dat->word(*tobj + 56, wrap_t ==
              cgltf_wrap_mode_clamp_to_edge ? GX_CLAMP : GX_REPEAT);
    dat->data[*tobj + 60] = 1;
    dat->data[*tobj + 61] = 1;
    dat->word(*tobj + 64, TEX_LIGHTMAP_DIFFUSE |
              TEX_COLORMAP_MODULATE | TEX_ALPHAMAP_MODULATE);
    befloat(dat->data.data() + *tobj + 68, 1.0f);
    dat->word(*tobj + 72, GX_LINEAR);
    dat->link(*tobj + 76, image_desc);
    return true;
}

bool encode_texture(const char* source_path, const cgltf_texture* texture,
                    NativeCostumeDat* dat, uint32_t* tobj,
                    char* error, size_t error_size)
{
    if (!texture || !texture->image) return false;
    std::vector<uint8_t> encoded;
    if (!load_image_bytes(source_path, texture->image, &encoded,
                          error, error_size) || encoded.empty()) {
        fail(error, error_size, "Could not read the glTF base-color texture.");
        return false;
    }
    return encode_texture_bytes(encoded.data(), encoded.size(),
        texture->sampler ? texture->sampler->wrap_s : cgltf_wrap_mode_repeat,
        texture->sampler ? texture->sampler->wrap_t : cgltf_wrap_mode_repeat,
        dat, tobj, error, error_size);
}

bool validate_costume(const std::vector<uint8_t>& bytes,
                      const char* root, char* error, size_t error_size)
{
    std::vector<uint8_t> arena(FT_COSTUME_IMPORT_ARENA_SIZE);
    FtCostumeAssets imported;
    if (!ftCostumeAssets_Import(bytes.data(), bytes.size(), root, nullptr,
                                arena.data(), arena.size(),
                                FT_COSTUME_IMPORT_RECORD_CAPACITY,
                                &imported)) {
        if (error && error_size)
            std::snprintf(error, error_size,
                "The generated Melee model failed validation (DAT status %d at 0x%lX).",
                (int) imported.import.status,
                (unsigned long) imported.import.error_offset);
        return false;
    }
    return true;
}

} // namespace

bool nativeFighterGltf_Scan(const char* path, NativeGltfScan* scan,
                            char* error, size_t error_size)
{
    if (!scan) return false;
    *scan = {};
    Source source;
    if (!source.open(path, error, error_size)) return false;
    unsigned count = 0;
    find_primitive(source.data, 0, nullptr, &count);
    if (count > NATIVE_GLTF_MAX_PRIMITIVES) {
        fail(error, error_size, "This file has too many mesh primitives.");
        return false;
    }
    for (unsigned i = 0; i < count; ++i) {
        Primitive primitive;
        find_primitive(source.data, i, &primitive, nullptr);
        NativeGltfPrimitiveInfo* info = &scan->primitives[i];
        std::snprintf(info->name, sizeof(info->name), "%s%s%u",
                      primitive.name ? primitive.name : "Mesh ",
                      primitive.name ? " · " : "", i);
        const cgltf_accessor* position =
            attribute(primitive.mesh, cgltf_attribute_type_position);
        info->vertices = position ? (unsigned) position->count : 0;
        const cgltf_size index_count = primitive.mesh->indices
            ? primitive.mesh->indices->count : info->vertices;
        info->triangles = (unsigned) (index_count / 3);
        info->has_normals =
            attribute(primitive.mesh, cgltf_attribute_type_normal) != nullptr;
        info->has_uv =
            attribute(primitive.mesh, cgltf_attribute_type_texcoord) != nullptr;
        info->has_skin = primitive.node && primitive.node->skin;
        info->has_texture = primitive.mesh->material &&
            primitive.mesh->material->has_pbr_metallic_roughness &&
            primitive.mesh->material->pbr_metallic_roughness.base_color_texture.texture;
    }
    scan->count = count;
    return true;
}

bool nativeFighterGltf_Apply(NativeFighterProject* project, const char* path,
                             unsigned primitive_id, unsigned joint,
                             unsigned replacement_mesh, float scale,
                             NativeGltfBindingMode binding,
                             char* error, size_t error_size)
{
    if (!project || !std::isfinite(scale) || scale <= 0 || scale > 1000 ||
        (binding != NATIVE_GLTF_BIND_RIGID &&
         binding != NATIVE_GLTF_BIND_AUTOMATIC &&
         binding != NATIVE_GLTF_BIND_TRANSFER)) {
        fail(error, error_size, "Choose a positive model scale.");
        return false;
    }
    Source source;
    if (!source.open(path, error, error_size)) return false;
    Primitive primitive;
    if (!find_primitive(source.data, primitive_id, &primitive, nullptr)) {
        fail(error, error_size, "Choose a mesh from this file.");
        return false;
    }
    NativeFighterModelEditor existing = {};
    if (!nativeFighterModel_Open(&existing, project) ||
        joint >= existing.joint_count) {
        fail(error, error_size, "Choose a valid fighter joint.");
        return false;
    }
    if (binding != NATIVE_GLTF_BIND_RIGID) {
        while (joint < existing.joint_count &&
               !(existing.joints[joint].flags & JOBJ_SKELETON_ROOT)) {
            const int parent = existing.joints[joint].parent;
            if (parent < 0) break;
            joint = (unsigned) parent;
        }
    }
    if (replacement_mesh != UINT_MAX &&
        (replacement_mesh >= existing.mesh_count ||
         (binding == NATIVE_GLTF_BIND_RIGID &&
          joint != existing.meshes[replacement_mesh].joint))) {
        fail(error, error_size, "Choose a valid compatible fighter joint or mesh.");
        return false;
    }
    if (replacement_mesh != UINT_MAX &&
        existing.meshes[replacement_mesh].material < existing.material_count &&
        (existing.materials[existing.meshes[replacement_mesh].material]
             .render_mode & RENDER_TEX0) &&
        !attribute(primitive.mesh, cgltf_attribute_type_texcoord)) {
        fail(error, error_size,
             "This mesh keeps a textured Melee material; the glTF replacement needs UVs.");
        return false;
    }
    NativeCostumeDat dat;
    if (!dat.open(project)) return false;
    SkinBinding skin;
    if (binding != NATIVE_GLTF_BIND_RIGID &&
        !prepare_skin(existing, joint, &skin, error, error_size)) return false;
    ImportedSkinMesh old_skin;
    TransferSkin transfer;
    if (binding == NATIVE_GLTF_BIND_TRANSFER &&
        (replacement_mesh == UINT_MAX ||
         !imported_skin_mesh(existing, replacement_mesh, &old_skin) ||
         !transfer.prepare(&old_skin))) {
        fail(error, error_size,
             "Weight transfer needs an existing imported skinned mesh to replace.");
        return false;
    }
    uint32_t polygon;
    if (!encode_geometry(primitive, &dat, scale,
                         existing.joints[joint].offset, existing,
                         binding == NATIVE_GLTF_BIND_RIGID ? nullptr : &skin,
                         binding == NATIVE_GLTF_BIND_TRANSFER ? &transfer : nullptr,
                         &polygon, error, error_size)) return false;
    if (replacement_mesh != UINT_MAX) {
        const NativeModelMesh& destination = existing.meshes[replacement_mesh];
        if (destination.joint != joint) {
            const uint32_t old_head = existing.joints[destination.joint].offset + 16;
            uint32_t previous_slot = old_head;
            bool found = false;
            for (unsigned i = 0; i < existing.mesh_count; ++i) {
                const uint32_t current = read32(dat.data.data() + previous_slot);
                if (current == destination.offset) {
                    found = true;
                    break;
                }
                if (!current || current >= dat.data.size() - 16) break;
                previous_slot = current + 4;
            }
            if (!found) {
                fail(error, error_size, "Could not relink the replacement mesh.");
                return false;
            }
            const uint32_t old_next = read32(dat.data.data() + destination.offset + 4);
            dat.link(previous_slot, old_next);
            const uint32_t new_head_slot = existing.joints[joint].offset + 16;
            const uint32_t new_head = read32(dat.data.data() + new_head_slot);
            dat.link(destination.offset + 4, new_head);
            dat.link(new_head_slot, destination.offset);
        }
        dat.link(destination.offset + 12, polygon);
    } else {
        const uint32_t material = dat.allocate(20);
        const uint32_t mobj = dat.allocate(24);
        const uint32_t dobj = dat.allocate(16);
        if (material == UINT32_MAX || mobj == UINT32_MAX ||
            dobj == UINT32_MAX) return false;
        const cgltf_material* source_material = primitive.mesh->material;
        const cgltf_texture* texture = source_material &&
            source_material->has_pbr_metallic_roughness
            ? source_material->pbr_metallic_roughness.base_color_texture.texture
            : nullptr;
        if (texture && !attribute(primitive.mesh,
                                  cgltf_attribute_type_texcoord)) {
            fail(error, error_size, "Textured mesh needs TEXCOORD_0 UVs.");
            return false;
        }
        const float* rgba = source_material &&
            source_material->has_pbr_metallic_roughness
            ? source_material->pbr_metallic_roughness.base_color_factor
            : nullptr;
        for (unsigned c = 0; c < 4; ++c) {
            dat.data[material + c] = c == 3 ? 255 : 96;
            dat.data[material + 4 + c] = rgba
                ? (uint8_t) std::clamp(std::lround(rgba[c] * 255), 0l, 255l)
                : 255;
            dat.data[material + 8 + c] = c == 3 ? 255 : 0;
        }
        befloat(dat.data.data() + material + 12, rgba ? rgba[3] : 1.0f);
        befloat(dat.data.data() + material + 16, 0.0f);
        uint32_t tobj = 0;
        if (texture && !encode_texture(path, texture, &dat, &tobj,
                                        error, error_size)) return false;
        dat.word(mobj + 4, RENDER_CONSTANT |
                           (tobj ? RENDER_TEX0 : 0));
        if (tobj) dat.link(mobj + 8, tobj);
        dat.link(mobj + 12, material);
        const uint32_t joint_slot = existing.joints[joint].offset + 16;
        const uint32_t previous = read32(dat.data.data() + joint_slot);
        if (previous) dat.link(dobj + 4, previous);
        dat.link(dobj + 8, mobj);
        dat.link(dobj + 12, polygon);
        dat.link(joint_slot, dobj);
        // JObj loading trusts the descriptor's draw-pass bits. A mesh added
        // to a transform-only joint needs its own opaque pass and the subtree
        // pass on every ancestor, otherwise display traversal skips it.
        dat.word(existing.joints[joint].offset + 4,
                 read32(dat.data.data() + existing.joints[joint].offset + 4) |
                     JOBJ_OPA);
        for (int parent = existing.joints[joint].parent; parent >= 0;
             parent = existing.joints[parent].parent) {
            const uint32_t flags = existing.joints[parent].offset + 4;
            dat.word(flags, read32(dat.data.data() + flags) | JOBJ_ROOT_OPA);
        }
    }
    std::vector<uint8_t> converted;
    if (!dat.finish(&converted) ||
        !validate_costume(converted, existing.root_name, error, error_size))
        return false;
    uint8_t* owned = (uint8_t*) std::malloc(converted.size());
    if (!owned) return false;
    std::memcpy(owned, converted.data(), converted.size());
    std::free(project->costume_dat);
    project->costume_dat = owned;
    project->costume_size = converted.size();
    return true;
}

unsigned nativeFighterGltf_SkinVertices(const NativeFighterProject* project,
                                        unsigned mesh,
                                        NativeImportedSkinVertex* output,
                                        unsigned capacity)
{
    if (!project) return 0;
    NativeFighterModelEditor model = {};
    if (!nativeFighterModel_Open(&model,
            const_cast<NativeFighterProject*>(project))) return 0;
    ImportedSkinMesh skin;
    if (!imported_skin_mesh(model, mesh, &skin)) return 0;
    if (output && capacity >= skin.vertices.size())
        std::memcpy(output, skin.vertices.data(),
                    skin.vertices.size() * sizeof(*output));
    return (unsigned) skin.vertices.size();
}

bool nativeFighterGltf_PaintWeights(NativeFighterProject* project,
                                    unsigned mesh, unsigned joint,
                                    const NativeSkinBrushPoint* centers,
                                    unsigned center_count, float radius,
                                    float strength, bool subtract,
                                    unsigned* affected,
                                    char* error, size_t error_size)
{
    if (affected) *affected = 0;
    if (!project || !centers || center_count == 0 ||
        center_count > 4096 || !std::isfinite(radius) ||
        !std::isfinite(strength) || radius <= 0.0f ||
        radius > 1000.0f || strength <= 0.0f || strength > 1.0f) {
        fail(error, error_size, "Enter a valid brush radius and strength.");
        return false;
    }
    for (unsigned i = 0; i < center_count; ++i)
        for (unsigned axis = 0; axis < 3; ++axis)
            if (!std::isfinite(centers[i].position[axis])) return false;
    NativeFighterModelEditor model = {};
    if (!nativeFighterModel_Open(&model, project) ||
        joint >= model.joint_count ||
        !(model.joints[joint].flags & JOBJ_SKELETON)) {
        fail(error, error_size, "Select a deform bone on this fighter.");
        return false;
    }
    ImportedSkinMesh skin;
    if (!imported_skin_mesh(model, mesh, &skin)) {
        fail(error, error_size,
             "Per-vertex painting requires a mesh imported with generated weights.");
        return false;
    }
    unsigned changed = 0;
    for (unsigned i = 0; i < skin.used.size(); ++i) {
        if (!skin.used[i]) continue;
        const NativeImportedSkinVertex& vertex = skin.vertices[i];
        float distance_squared = radius * radius;
        for (unsigned center = 0; center < center_count; ++center) {
            float current = 0.0f;
            for (unsigned axis = 0; axis < 3; ++axis) {
                const float delta = vertex.position[axis] -
                                    centers[center].position[axis];
                current += delta * delta;
            }
            distance_squared = std::min(distance_squared, current);
        }
        if (distance_squared >= radius * radius) continue;
        SkinPalette& palette = skin.palettes[i];
        const uint8_t old_weight = palette.weight;
        const uint16_t old_joint[2] = {
            palette.joint[0], palette.joint[1]
        };
        const float influence = strength *
            (1.0f - std::sqrt(distance_squared) / radius);
        if (palette.joint[0] == joint && palette.joint[1] == joint) {
            if (!subtract) continue;
            // There is no other influence to receive the removed weight.
            continue;
        }
        if (palette.joint[0] == joint || palette.joint[1] == joint) {
            const bool first = palette.joint[0] == joint;
            const float current = first ? palette.weight / 32.0f :
                                          1.0f - palette.weight / 32.0f;
            const long target = std::clamp(std::lround(32.0f *
                std::clamp(current + (subtract ? -influence : influence),
                           0.0f, 1.0f)), 0l, 32l);
            palette.weight = (uint8_t) (first ? target : 32 - target);
            if (palette.weight == 0) {
                palette.joint[0] = palette.joint[1];
                palette.weight = 32;
            } else if (palette.weight == 32) {
                palette.joint[1] = palette.joint[0];
            }
        } else if (!subtract) {
            const uint16_t retained = palette.weight >= 16
                ? palette.joint[0] : palette.joint[1];
            const long target = std::clamp(std::lround(influence * 32.0f),
                                            0l, 32l);
            if (target == 0) continue;
            palette.joint[0] = (uint16_t) joint;
            palette.joint[1] = target == 32 ? (uint16_t) joint : retained;
            palette.weight = (uint8_t) target;
        }
        if (palette.weight != old_weight ||
            palette.joint[0] != old_joint[0] ||
            palette.joint[1] != old_joint[1]) ++changed;
    }
    if (affected) *affected = changed;
    if (!changed) return true;
    NativeCostumeDat dat;
    if (!dat.open(project)) return false;
    SkinChunk chunk;
    uint32_t first_polygon = 0, previous = 0;
    for (size_t start = 0; start < skin.triangles.size(); start += 3) {
        unsigned additional = 0;
        for (unsigned c = 0; c < 3; ++c) {
            const SkinPalette& palette = skin.palettes[skin.triangles[start + c]];
            bool found = false;
            for (const SkinPalette& existing : chunk.palettes)
                if (existing == palette) found = true;
            for (unsigned earlier = 0; earlier < c; ++earlier)
                if (skin.palettes[skin.triangles[start + earlier]] == palette)
                    found = true;
            if (!found) ++additional;
        }
        if (!chunk.indices.empty() &&
            (chunk.palettes.size() + additional > 10 ||
             chunk.indices.size() + 3 > 65532)) {
            uint32_t emitted;
            if (!write_skin_chunk(&dat, chunk, skin.descriptors, model,
                                  skin.uv, &emitted)) return false;
            if (previous) dat.link(previous + 4, emitted);
            else first_polygon = emitted;
            previous = emitted;
            chunk = {};
        }
        for (unsigned c = 0; c < 3; ++c) {
            const uint16_t index = skin.triangles[start + c];
            const SkinPalette& palette = skin.palettes[index];
            unsigned slot = 0;
            for (; slot < chunk.palettes.size(); ++slot)
                if (chunk.palettes[slot] == palette) break;
            if (slot == chunk.palettes.size())
                chunk.palettes.push_back(palette);
            chunk.indices.push_back(index);
            chunk.palette_index.push_back((uint8_t) slot);
        }
    }
    if (!chunk.indices.empty()) {
        uint32_t emitted;
        if (!write_skin_chunk(&dat, chunk, skin.descriptors, model,
                              skin.uv, &emitted)) return false;
        if (previous) dat.link(previous + 4, emitted);
        else first_polygon = emitted;
    }
    dat.link(model.meshes[mesh].offset + 12, first_polygon);
    std::vector<uint8_t> converted;
    if (!dat.finish(&converted) ||
        !validate_costume(converted, model.root_name, error, error_size))
        return false;
    uint8_t* owned = (uint8_t*) std::malloc(converted.size());
    if (!owned) return false;
    std::memcpy(owned, converted.data(), converted.size());
    std::free(project->costume_dat);
    project->costume_dat = owned;
    project->costume_size = converted.size();
    return true;
}

bool nativeFighterGltf_SetMaterialTexture(NativeFighterProject* project,
                                          unsigned material,
                                          const char* image_path,
                                          char* error, size_t error_size)
{
    NativeFighterModelEditor existing = {};
    if (!project || !image_path ||
        !nativeFighterModel_Open(&existing, project) ||
        material >= existing.material_count) {
        fail(error, error_size, "Choose an editable fighter material.");
        return false;
    }
    FILE* file = std::fopen(image_path, "rb");
    if (!file) {
        fail(error, error_size, "Could not open this image file.");
        return false;
    }
    std::fseek(file, 0, SEEK_END);
    const long length = std::ftell(file);
    std::rewind(file);
    if (length <= 0 || length > 8 * 1024 * 1024) {
        std::fclose(file);
        fail(error, error_size, "Image must be smaller than 8 MB.");
        return false;
    }
    std::vector<uint8_t> encoded((size_t) length);
    const bool read = std::fread(encoded.data(), 1, encoded.size(), file) ==
                      encoded.size();
    std::fclose(file);
    if (!read) {
        fail(error, error_size, "Could not read the image file.");
        return false;
    }
    NativeCostumeDat dat;
    if (!dat.open(project)) return false;
    uint32_t texture;
    if (!encode_texture_bytes(encoded.data(), encoded.size(),
                              cgltf_wrap_mode_repeat, cgltf_wrap_mode_repeat,
                              &dat, &texture, error, error_size)) return false;
    const uint32_t mobj = existing.materials[material].offset;
    dat.link(mobj + 8, texture);
    dat.word(mobj + 4, existing.materials[material].render_mode | RENDER_TEX0);
    std::vector<uint8_t> converted;
    if (!dat.finish(&converted) ||
        !validate_costume(converted, existing.root_name, error, error_size))
        return false;
    uint8_t* owned = (uint8_t*) std::malloc(converted.size());
    if (!owned) return false;
    std::memcpy(owned, converted.data(), converted.size());
    std::free(project->costume_dat);
    project->costume_dat = owned;
    project->costume_size = converted.size();
    return true;
}

#endif
