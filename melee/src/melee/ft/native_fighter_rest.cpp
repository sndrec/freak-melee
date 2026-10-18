#include "native_fighter_model_editor.h"
#include "native_asset_dat.h"

extern "C" {
#include <sysdolphin/baselib/mtx.h>
}
#include <cmath>
#include <cstdlib>
#include <memory>

namespace {
bool reference(const HSD_Dat* dat, u32 slot, u32* target)
{
    const char* external;
    return HSD_DatResolve(dat, slot, target, &external) == HSD_DAT_REF_LOCAL &&
           *target < dat->data_size;
}

static bool rest_world_matrices(const NativeFighterModelEditor* model,
                                Mtx* world)
{
    Vec3 accumulated_scale[NATIVE_MODEL_MAX_JOINTS];
    bool has_scale[NATIVE_MODEL_MAX_JOINTS] = { false };
    for (unsigned i = 0; i < model->joint_count; ++i) {
        const NativeModelJoint* bone = &model->joints[i];
        const int parent = bone->parent;
        if (bone->flags & 0x20000) return false;
        Vec3 scale = { bone->scale[0], bone->scale[1], bone->scale[2] };
        Vec3 rotation = { bone->rotation[0], bone->rotation[1],
                          bone->rotation[2] };
        Vec3 position = { bone->position[0], bone->position[1],
                          bone->position[2] };
        Vec3* parent_scale = parent >= 0 && has_scale[parent]
            ? &accumulated_scale[parent] : NULL;
        if (parent_scale != NULL &&
            (fabsf(parent_scale->x) < 0.000001f ||
             fabsf(parent_scale->y) < 0.000001f ||
             fabsf(parent_scale->z) < 0.000001f)) return false;
        if (bone->flags & 8) {
            if (parent_scale != NULL) {
                accumulated_scale[i] = *parent_scale;
                has_scale[i] = true;
            }
        } else {
            has_scale[i] = true;
            accumulated_scale[i].x = scale.x *
                (parent_scale ? parent_scale->x : 1.0f);
            accumulated_scale[i].y = scale.y *
                (parent_scale ? parent_scale->y : 1.0f);
            accumulated_scale[i].z = scale.z *
                (parent_scale ? parent_scale->z : 1.0f);
        }
        Mtx local;
        HSD_MtxSRT(local, &scale, &rotation, &position, parent_scale);
        if (parent >= 0) C_MTXConcat(world[parent], local, world[i]);
        else memcpy(world[i], local, sizeof(local));
        for (unsigned row = 0; row < 3; ++row)
            for (unsigned column = 0; column < 4; ++column)
                if (!isfinite(world[i][row][column])) return false;
    }
    return true;
}


void matrix_write(NativeAssetDat& dat, u32 offset, const Mtx matrix)
{
    for (unsigned r = 0; r < 3; ++r)
        for (unsigned c = 0; c < 4; ++c) {
            u32 bits;
            std::memcpy(&bits, &matrix[r][c], 4);
            dat.word(offset + (r * 4 + c) * 4, bits);
        }
}

struct Attribute {
    u32 offset, attr, type, count, format, array;
    unsigned width, bytes, fraction, stride;
};

bool attributes(const HSD_Dat& dat, u32 offset,
                std::vector<Attribute>& result)
{
    bool present[26] = {};
    for (unsigned i = 0; i < 27; ++i, offset += 24) {
        u32 attr;
        if (!HSD_DatReadU32(&dat, offset, &attr)) return false;
        if (attr == 255) {
            std::sort(result.begin(), result.end(), [](auto& a, auto& b) {
                return a.attr < b.attr;
            });
            return present[9];
        }
        if (attr == 25) attr = 10; // GX_VA_NBT shares the normal hardware slot.
        if (attr > 20 || present[attr] || !HSD_DatBytes(&dat, offset, 24)) return false;
        present[attr] = true;
        Attribute a = {};
        a.offset = offset; a.attr = attr;
        HSD_DatReadU32(&dat, offset + 4, &a.type);
        HSD_DatReadU32(&dat, offset + 8, &a.count);
        HSD_DatReadU32(&dat, offset + 12, &a.format);
        a.fraction = attr == 10 ? (a.format <= 1 ? 6 : a.format <= 3 ? 14 : 0) :
            HSD_DatBytes(&dat, offset + 16, 1)[0];
        u16 stride;
        HSD_DatReadU16(&dat, offset + 18, &stride); a.stride = stride;
        if (a.type == 0) continue;
        if (a.type > 3) return false;
        if (attr <= 8) {
            if (a.type != 1) return false;
            a.bytes = 1;
        } else if (attr == 11 || attr == 12) {
            const unsigned sizes[] = { 2, 3, 4, 2, 3, 4 };
            if (a.format > 5 || a.count > 1) return false;
            a.bytes = sizes[a.format];
        } else {
            if (a.format > 4 || a.fraction > 31 ||
                a.count > (attr == 10 ? 2u : 1u)) return false;
            a.width = a.format <= 1 ? 1 : a.format <= 3 ? 2 : 4;
            const unsigned components = attr == 9 ? (a.count ? 3 : 2) :
                attr == 10 ? (a.count == 1 ? 9 : 3) : (a.count ? 2 : 1);
            a.bytes = components * a.width;
        }
        if (a.type > 1 && (!a.stride || a.stride > 255 ||
                           !reference(&dat, offset + 20, &a.array))) return false;
        result.push_back(a);
    }
    return false;
}

float scalar(const u8* data, const Attribute& a)
{
    float value;
    if (a.format == 4) {
        u32 bits = NativeAssetDat::read32(data);
        std::memcpy(&value, &bits, 4);
        return value;
    }
    if (a.format == 0) value = data[0];
    else if (a.format == 1) value = (int8_t) data[0];
    else {
        const u16 word = u16(data[0]) << 8 | data[1];
        value = a.format == 3 ? (float) (int16_t) word : (float) word;
    }
    return std::ldexp(value, -(int) a.fraction);
}

bool append_vector(std::vector<u8>& bytes, const float* value)
{
    for (unsigned axis = 0; axis < 3; ++axis) {
        if (!std::isfinite(value[axis])) return false;
        const size_t at = bytes.size(); bytes.resize(at + 4);
        u32 bits; std::memcpy(&bits, value + axis, 4);
        NativeAssetDat::be32(bytes.data() + at, bits);
    }
    return true;
}

// Shape animation evaluates indexed base/target arrays before drawing. Give
// each polygon private float arrays, retaining its display-list indices.
bool compensate_shape_attribute(NativeAssetDat& output, const HSD_Dat& source,
                                u32 shape, bool normals, unsigned targets,
                                bool additive, Mtx adjustment,
                                u32 replacement_shape)
{
    const unsigned member = normals ? 16 : 4;
    u32 count, descriptor, table;
    if (!HSD_DatReadU32(&source, shape + member, &count) ||
        count > HSD_DEFAULT_MAX_SHAPE_VERTICES) return false;
    if (!count) return normals;
    if (!reference(&source, shape + member + 4, &descriptor) ||
        !reference(&source, shape + member + 8, &table) ||
        !HSD_DatBytes(&source, descriptor, 24) ||
        !HSD_DatBytes(&source, table, targets * 4)) return false;
    Attribute a = {};
    u16 stride;
    HSD_DatReadU32(&source, descriptor, &a.attr);
    HSD_DatReadU32(&source, descriptor + 4, &a.type);
    HSD_DatReadU32(&source, descriptor + 8, &a.count);
    HSD_DatReadU32(&source, descriptor + 12, &a.format);
    HSD_DatReadU16(&source, descriptor + 18, &stride);
    a.stride = stride;
    // Shape decoding uses the descriptor's fraction for normals too.
    a.fraction = HSD_DatBytes(&source, descriptor + 16, 1)[0];
    if ((a.type != 2 && a.type != 3) || a.format > 4 ||
        a.fraction > 31 || !a.stride ||
        (normals ? (a.attr != 10 && a.attr != 25) || a.count != 0 :
                   a.attr != 9 || a.count != 1) ||
        !reference(&source, descriptor + 20, &a.array)) return false;
    const unsigned vectors = a.attr == 25 ? 3 : 1;
    if (normals && count * vectors > HSD_DEFAULT_MAX_SHAPE_NORMALS) return false;
    a.width = a.format <= 1 ? 1 : a.format <= 3 ? 2 : 4;
    const u32 new_table = output.allocate(targets * 4);
    const u32 new_descriptor = output.allocate(48);
    if (new_table == UINT32_MAX || new_descriptor == UINT32_MAX) return false;
    // Retain sharing across targets. A position used as both an additive
    // offset and a base position needs two entries: only the base translates.
    const bool split_base = additive && !normals;
    std::vector<u32> remap((UINT16_MAX + size_t(1)) * (split_base ? 2 : 1), UINT32_MAX);
    std::vector<u8> values;
    for (unsigned target = 0; target < targets; ++target) {
        u32 indices;
        if (!reference(&source, table + target * 4, &indices)) return false;
        const unsigned index_width = a.type == 2 ? 1 : 2;
        const u8* list = HSD_DatBytes(&source, indices, count * index_width);
        if (!list) return false;
        const u32 new_list = output.allocate(count * 2);
        if (new_list == UINT32_MAX) return false;
        output.link(new_table + target * 4, new_list);
        for (unsigned i = 0; i < count; ++i) {
            const unsigned index = index_width == 1 ? list[i] :
                unsigned(list[i * 2]) << 8 | list[i * 2 + 1];
            u32& mapped = remap[index + (split_base && target ? UINT16_MAX + 1u : 0)];
            if (mapped != UINT32_MAX) {
                output.half(new_list + i * 2, (u16) mapped);
                continue;
            }
            mapped = (u32) (values.size() / (vectors * 12));
            if (mapped > UINT16_MAX) return false;
            output.half(new_list + i * 2, (u16) mapped);
            const size_t at = size_t(a.array) + size_t(index) * a.stride;
            if (at > UINT32_MAX) return false;
            const u8* value = HSD_DatBytes(&source, (u32) at, vectors * 3 * a.width);
            if (!value) return false;
            for (unsigned part = 0; part < vectors; ++part, value += 3 * a.width) {
                Vec3 original = { scalar(value, a), scalar(value + a.width, a),
                                  scalar(value + 2 * a.width, a) }, changed;
                if (normals || (additive && target))
                    C_MTXMultVecSR(adjustment, &original, &changed);
                else C_MTXMultVec(adjustment, &original, &changed);
                if (!append_vector(values, &changed.x)) return false;
            }
        }
    }
    const u32 new_array = output.allocate(values.size());
    if (new_array == UINT32_MAX) return false;
    std::memcpy(output.data.data() + new_array, values.data(), values.size());
    std::memcpy(output.data.data() + new_descriptor,
                HSD_DatBytes(&source, descriptor, 24), 24);
    output.word(new_descriptor + 4, 3); // GX_INDEX16
    output.word(new_descriptor + 12, 4); // GX_F32
    output.word(new_descriptor + 16, 0);
    output.half(new_descriptor + 18, (u16) (vectors * 12));
    output.link(new_descriptor + 20, new_array);
    output.word(new_descriptor + 24, 255); // GX_VA_NULL terminates the descriptor list.
    output.link(replacement_shape + member + 4, new_descriptor);
    output.link(replacement_shape + member + 8, new_table);
    return true;
}

bool compensate_shape(NativeAssetDat& output, const HSD_Dat& source,
                      u32 polygon, Mtx adjustment, u32* replacement)
{
    u32 shape;
    u16 flags, count;
    if (!reference(&source, polygon + 20, &shape) ||
        !HSD_DatBytes(&source, shape, 28) ||
        !HSD_DatReadU16(&source, shape, &flags) ||
        !HSD_DatReadU16(&source, shape + 2, &count) || !count ||
        ((flags & 3) != SHAPESET_AVERAGE && (flags & 3) != SHAPESET_ADDITIVE)) return false;
    const bool additive = (flags & SHAPESET_ADDITIVE) != 0;
    const unsigned targets = count + (additive ? 1 : 0);
    Mtx inverse, normal;
    if (!C_MTXInverse(adjustment, inverse)) return false;
    C_MTXTranspose(inverse, normal);
    const u32 new_shape = output.allocate(28);
    const u32 new_polygon = output.allocate(24);
    if (new_shape == UINT32_MAX || new_polygon == UINT32_MAX) return false;
    std::memcpy(output.data.data() + new_shape, HSD_DatBytes(&source, shape, 28), 28);
    for (unsigned member : {8u, 12u, 20u, 24u}) {
        u32 target = 0; const char* external;
        const auto kind = HSD_DatResolve(&source, shape + member, &target, &external);
        if (kind != HSD_DAT_REF_LOCAL && kind != HSD_DAT_REF_NULL) return false;
        output.link(new_shape + member, target);
    }
    if (!compensate_shape_attribute(output, source, shape, false, targets, additive, adjustment, new_shape) ||
        !compensate_shape_attribute(output, source, shape, true, targets, additive, normal, new_shape)) return false;
    std::memcpy(output.data.data() + new_polygon, HSD_DatBytes(&source, polygon, 24), 24);
    for (unsigned member : {0u, 4u, 8u, 16u}) {
        u32 target = 0; const char* external;
        const auto kind = HSD_DatResolve(&source, polygon + member, &target, &external);
        if (kind != HSD_DAT_REF_LOCAL && kind != HSD_DAT_REF_NULL) return false;
        output.link(new_polygon + member, target);
    }
    output.link(new_polygon + 20, new_shape);
    *replacement = new_polygon;
    return true;
}

// Positions/normals become direct floats, so aliases, quantization and a
// vertex used with both rigid matrix slots cannot corrupt another polygon.
// Other attributes retain their original bytes and arrays.
bool compensate_polygon(NativeAssetDat& output, const HSD_Dat& source,
                    u32 polygon, Mtx* adjustment, unsigned matrix_count, u32* replacement)
{
    u32 descriptor, display;
    u16 blocks;
    if (!reference(&source, polygon + 8, &descriptor) ||
        !reference(&source, polygon + 16, &display) ||
        !HSD_DatReadU16(&source, polygon + 14, &blocks)) return false;
    std::vector<Attribute> attrs;
    if (!attributes(source, descriptor, attrs)) return false;
    const size_t length = size_t(blocks) * 32;
    const u8* data = HSD_DatBytes(&source, display, length);
    if (!data) return false;
    Mtx normal[10], inverse;
    for (unsigned slot = 0; slot < matrix_count; ++slot) {
        if (!C_MTXInverse(adjustment[slot], inverse)) return false;
        C_MTXTranspose(inverse, normal[slot]);
    }
    std::vector<u8> commands;
    size_t cursor = 0;
    while (cursor < length) {
        const unsigned command = data[cursor++];
        if (!command) continue;
        if ((command & 7) || command < 0x80 || command > 0xB8 ||
            command == 0x88 || length - cursor < 2) return false;
        const unsigned count = unsigned(data[cursor]) << 8 | data[cursor + 1];
        commands.insert(commands.end(), { (u8) command, data[cursor], data[cursor + 1] });
        cursor += 2;
        for (unsigned vertex = 0; vertex < count; ++vertex) {
            unsigned matrix = 0;
            for (const Attribute& a : attrs) {
                const bool nbt3 = a.attr == 10 && a.count == 2;
                const unsigned repeats = nbt3 ? 3 : 1;
                const unsigned width = a.type == 1 ? a.bytes : a.type == 2 ? 1 : 2;
                const size_t consumed = size_t(width) * repeats;
                if (length - cursor < consumed) return false;
                const u8* value = data + cursor; cursor += consumed;
                if (a.attr == 0) {
                    if (value[0] % 3 || value[0] / 3 >= matrix_count) return false;
                    matrix = value[0] / 3;
                }
                if (a.attr != 9 && a.attr != 10) {
                    commands.insert(commands.end(), value, value + consumed);
                    continue;
                }
                for (unsigned part = 0; part < repeats; ++part) {
                    const u8* vector = value + part * width;
                    if (a.type > 1) {
                        const unsigned index = a.type == 2 ? vector[0] :
                            unsigned(vector[0]) << 8 | vector[1];
                        const size_t at = size_t(a.array) + size_t(index) * a.stride;
                        if (at > UINT32_MAX || !(vector = HSD_DatBytes(&source, (u32) at, a.bytes))) return false;
                    }
                    const unsigned vectors = a.attr == 10 && a.count == 1 ? 3 : 1;
                    for (unsigned k = 0; k < vectors; ++k) {
                        Vec3 original = { scalar(vector, a), scalar(vector + a.width, a),
                            a.attr == 9 && a.count == 0 ? 0 : scalar(vector + 2 * a.width, a) };
                        Vec3 changed;
                        if (a.attr == 9) C_MTXMultVec(adjustment[matrix], &original, &changed);
                        else C_MTXMultVecSR(normal[matrix], &original, &changed);
                        if (!append_vector(commands, &changed.x)) return false;
                        vector += a.width * 3;
                    }
                }
            }
            if (commands.size() > size_t(UINT16_MAX) * 32) return false;
        }
    }
    commands.resize((commands.size() + 31) & ~size_t(31), 0);
    const u32 new_display = output.allocate(commands.size(), 32);
    const u32 new_attrs = output.allocate((attrs.size() + 1) * 24);
    const u32 new_polygon = output.allocate(24);
    if (new_display == UINT32_MAX || new_attrs == UINT32_MAX || new_polygon == UINT32_MAX) return false;
    std::memcpy(output.data.data() + new_display, commands.data(), commands.size());
    for (unsigned i = 0; i < attrs.size(); ++i) {
        const Attribute& a = attrs[i]; const u32 at = new_attrs + i * 24;
        std::memcpy(output.data.data() + at, HSD_DatBytes(&source, a.offset, 24), 24);
        if (a.attr == 9 || a.attr == 10) {
            output.word(at, a.attr);
            output.word(at + 4, 1); // GX_DIRECT
            output.word(at + 8, a.attr == 9 ? 1 : a.count ? 1 : 0);
            output.word(at + 12, 4); // GX_F32
            output.word(at + 16, 0);
            output.link(at + 20, 0);
        } else if (a.type > 1) output.link(at + 20, a.array);
        else output.link(at + 20, 0);
    }
    output.word(new_attrs + attrs.size() * 24, 255);
    std::memcpy(output.data.data() + new_polygon, HSD_DatBytes(&source, polygon, 24), 24);
    for (unsigned member : {0u, 4u, 20u}) {
        u32 target = 0;
        const char* external;
        const auto kind = HSD_DatResolve(&source, polygon + member, &target, &external);
        if (kind != HSD_DAT_REF_LOCAL && kind != HSD_DAT_REF_NULL) return false;
        output.link(new_polygon + member, target);
    }
    output.link(new_polygon + 8, new_attrs);
    output.link(new_polygon + 16, new_display);
    output.half(new_polygon + 14, (u16) (commands.size() / 32));
    *replacement = new_polygon;
    return true;
}


bool read_bind(const u8* data, size_t size, u32 joint, Mtx bind)
{
    if (joint > size || size - joint < 60) return false;
    const u32 at = NativeAssetDat::read32(data + joint + 56);
    if (!at || at > size || size - at < sizeof(Mtx)) return false;
    for (unsigned r = 0; r < 3; ++r) for (unsigned c = 0; c < 4; ++c) {
        const u32 bits = NativeAssetDat::read32(data + at + (r * 4 + c) * 4);
        std::memcpy(&bind[r][c], &bits, 4);
        if (!std::isfinite(bind[r][c])) return false;
    }
    return true;
}

// Evaluate the same rest-space palette as HSD: root-owned single-bone
// envelopes bypass inverse binds; non-root owners add a model-node matrix.
bool envelope_matrices(const NativeFighterModelEditor& model, unsigned owner,
                        u32 polygon, Mtx* world, const u8* bind_data,
                        size_t bind_size, Mtx* matrices, unsigned* count)
{
    const bool has_right = !(model.joints[owner].flags & 2);
    Mtx right;
    if (has_right) {
        int skeleton = (int) owner;
        while (skeleton >= 0 && !(model.joints[skeleton].flags & 3)) skeleton = model.joints[skeleton].parent;
        if (skeleton < 0) return false;
        Mtx bind, base, inverse;
        if (skeleton == (int) owner) {
            if (!read_bind(bind_data, bind_size, model.joints[owner].offset, bind) ||
                !C_MTXInverse(bind, right)) return false;
        } else {
            if (model.joints[skeleton].flags & 2) C_MTXCopy(world[skeleton], base);
            else {
                if (!read_bind(bind_data, bind_size, model.joints[skeleton].offset, bind)) return false;
                C_MTXConcat(world[skeleton], bind, base);
            }
            if (!C_MTXInverse(base, inverse)) return false;
            C_MTXConcat(inverse, world[owner], right);
        }
    }
    u32 table;
    if (!reference(&model.dat, polygon + 20, &table)) return false;
    *count = 0;
    for (unsigned slot = 0; slot < 10; ++slot) {
        u32 entry;
        if (!reference(&model.dat, table + slot * 4, &entry)) break;
        float first;
        if (!HSD_DatReadF32(&model.dat, entry + 4, &first)) return false;
        // The engine's float subtraction rounds its threshold to 1.0f.
        const bool single = first >= 1.0f - 1.00000001335e-10F;
        std::memset(matrices[slot], 0, sizeof(Mtx));
        bool terminated = false;
        for (unsigned influence = 0; influence < NATIVE_MODEL_MAX_JOINTS; ++influence, entry += 8) {
            u32 bone_offset;
            if (!reference(&model.dat, entry, &bone_offset)) {
                u32 value;
                if (!HSD_DatReadU32(&model.dat, entry, &value) || value) return false;
                terminated = true; break;
            }
            unsigned bone = 0;
            while (bone < model.joint_count && model.joints[bone].offset != bone_offset) ++bone;
            float weight;
            if (bone == model.joint_count || !HSD_DatReadF32(&model.dat, entry + 4, &weight) ||
                !std::isfinite(weight) || weight < 0) return false;
            Mtx effective, bind;
            if (single && !has_right) C_MTXCopy(world[bone], effective);
            else {
                if (!read_bind(bind_data, bind_size, bone_offset, bind)) return false;
                C_MTXConcat(world[bone], bind, effective);
            }
            for (unsigned r = 0; r < 3; ++r) for (unsigned c = 0; c < 4; ++c)
                matrices[slot][r][c] += effective[r][c] * (single ? 1.0f : weight);
            if (single) { terminated = true; break; }
        }
        if (!terminated) return false;
        if (has_right) {
            Mtx temporary; C_MTXConcat(matrices[slot], right, temporary);
            C_MTXCopy(temporary, matrices[slot]);
        }
        ++*count;
    }
    return *count != 0;
}

bool compensate_meshes(NativeAssetDat& dat, const NativeFighterModelEditor& editor,
                        const NativeFighterModelEditor& old,
                   Mtx* old_world, Mtx* new_world,
                   const bool* changed)
{
    for (unsigned mesh = 0; mesh < editor.mesh_count; ++mesh) {
        const auto& entry = editor.meshes[mesh];
        u32 polygon;
        if (!reference(&editor.dat, entry.offset + 12, &polygon)) continue;
        struct Polygon { u32 source; Mtx adjustment[10]; unsigned slots; bool compensate; };
        std::vector<Polygon> chain;
        bool affected = false;
        while (polygon) {
            if (chain.size() == 4096 || !HSD_DatBytes(&editor.dat, polygon, 24)) return false;
            for (auto& p : chain) if (p.source == polygon) return false;
            u16 flags; HSD_DatReadU16(&editor.dat, polygon + 12, &flags);
            Polygon p = {}; p.source = polygon; p.slots = 2;
            unsigned joints[2] = { entry.joint, entry.joint };
            const unsigned type = flags & 0x3000;
            if (type == 0) {
                u32 other;
                if (reference(&editor.dat, polygon + 20, &other)) {
                    unsigned i = 0;
                    for (; i < editor.joint_count && editor.joints[i].offset != other; ++i) {}
                    if (i == editor.joint_count) return false;
                    joints[1] = i;
                }
            }
            if (type == 0x2000) {
                Mtx old_palette[10], new_palette[10]; unsigned old_count, new_count;
                if (!envelope_matrices(old, entry.joint, polygon, old_world,
                        old.dat.data, old.dat.data_size, old_palette, &old_count) ||
                    !envelope_matrices(editor, entry.joint, polygon, new_world,
                        dat.data.data(), dat.data.size(), new_palette, &new_count) ||
                    old_count != new_count) return false;
                p.slots = old_count;
                for (unsigned slot = 0; slot < p.slots; ++slot) {
                    bool different = false;
                    for (unsigned r = 0; r < 3; ++r) for (unsigned c = 0; c < 4; ++c)
                        different |= std::fabs(old_palette[slot][r][c] - new_palette[slot][r][c]) >
                            .00002f * std::fmax(1.0f, std::fabs(old_palette[slot][r][c]));
                    if (!different) { C_MTXIdentity(p.adjustment[slot]); continue; }
                    Mtx inverse;
                    if (!C_MTXInverse(new_palette[slot], inverse)) return false;
                    C_MTXConcat(inverse, old_palette[slot], p.adjustment[slot]);
                    p.compensate |= different;
                }
                affected |= p.compensate;
            } else if (changed[joints[0]] || changed[joints[1]]) {
                if (type != 0 && type != POBJ_SHAPEANIM) return false;
                if (type == POBJ_SHAPEANIM) p.slots = 1;
                for (unsigned slot = 0; slot < p.slots; ++slot) {
                    Mtx inverse;
                    if (!C_MTXInverse(new_world[joints[slot]], inverse)) return false;
                    C_MTXConcat(inverse, old_world[joints[slot]], p.adjustment[slot]);
                }
                p.compensate = affected = true;
            }
            chain.push_back(p);
            u32 next = 0; reference(&editor.dat, polygon + 4, &next); polygon = next;
        }
        if (!affected) continue;
        // Clone the whole chain: modifying an unchanged predecessor's next
        // pointer would otherwise affect another mesh sharing that polygon.
        u32 link = entry.offset + 12;
        for (auto& p : chain) {
            u32 replacement;
            if (p.compensate) {
                u16 flags; HSD_DatReadU16(&editor.dat, p.source + 12, &flags);
                if ((flags & 0x3000) == POBJ_SHAPEANIM) {
                    if (!compensate_shape(dat, editor.dat, p.source, p.adjustment[0], &replacement)) return false;
                } else if (!compensate_polygon(dat, editor.dat, p.source, p.adjustment, p.slots, &replacement)) return false;
            } else {
                replacement = dat.allocate(24);
                if (replacement == UINT32_MAX) return false;
                std::memcpy(dat.data.data() + replacement, HSD_DatBytes(&editor.dat, p.source, 24), 24);
                for (unsigned member : {0u, 8u, 16u, 20u}) {
                    u32 target = 0; const char* external;
                    const auto kind = HSD_DatResolve(&editor.dat, p.source + member, &target, &external);
                    if (kind != HSD_DAT_REF_LOCAL && kind != HSD_DAT_REF_NULL) return false;
                    dat.link(replacement + member, target);
                }
            }
            dat.link(link, replacement);
            link = replacement + 4;
        }
        dat.link(link, 0);
    }
    return true;
}
} // namespace


bool nativeFighterModel_CompensateRest(NativeFighterModelEditor* editor,
                                      const uint8_t* previous,
                                      size_t previous_size,
                                      unsigned changed_joint)
{
    if (!editor || !previous || changed_joint >= editor->joint_count ||
        previous_size != editor->project->costume_size) return false;
    NativeFighterProject old_project = *editor->project;
    old_project.costume_dat = const_cast<uint8_t*>(previous);
    auto old = std::make_unique<NativeFighterModelEditor>();
    if (!nativeFighterModel_Open(old.get(), &old_project) ||
        old->joint_count != editor->joint_count) return false;
    auto old_world = std::make_unique<Mtx[]>(editor->joint_count);
    auto new_world = std::make_unique<Mtx[]>(editor->joint_count);
    if (!rest_world_matrices(old.get(), old_world.get()) ||
        !rest_world_matrices(editor, new_world.get())) return false;
    NativeAssetDat dat;
    if (!dat.open(editor->project->costume_dat, editor->project->costume_size)) return false;
    bool changed[NATIVE_MODEL_MAX_JOINTS] = {};
    for (unsigned i = 0; i < editor->joint_count; ++i) {
        int cursor = (int) i;
        while (cursor >= 0 && cursor != (int) changed_joint) cursor = editor->joints[cursor].parent;
        if (cursor < 0 || !std::memcmp(old_world[i], new_world[i], sizeof(Mtx))) continue;
        changed[i] = true;
        u32 bind_offset;
        if (!reference(&old->dat, old->joints[i].offset + 56, &bind_offset)) continue;
        Mtx inverse, old_bind, adjustment, new_bind;
        if (!C_MTXInverse(new_world[i], inverse)) return false;
        for (unsigned r = 0; r < 3; ++r)
            for (unsigned c = 0; c < 4; ++c)
                if (!HSD_DatReadF32(&old->dat, bind_offset + (r * 4 + c) * 4,
                                    &old_bind[r][c])) return false;
        C_MTXConcat(inverse, old_world[i], adjustment);
        C_MTXConcat(adjustment, old_bind, new_bind);
        Mtx old_rest, new_rest;
        C_MTXConcat(old_world[i], old_bind, old_rest);
        C_MTXConcat(new_world[i], new_bind, new_rest);
        for (unsigned r = 0; r < 3; ++r)
            for (unsigned c = 0; c < 4; ++c)
                if (!std::isfinite(new_bind[r][c]) || !std::isfinite(new_rest[r][c]) ||
                    std::fabs(new_rest[r][c] - old_rest[r][c]) > 0.002f) return false;
        // A shared old bind matrix may still be used by an unchanged bone.
        const u32 replacement = dat.allocate(sizeof(Mtx));
        if (replacement == UINT32_MAX) return false;
        matrix_write(dat, replacement, new_bind);
        dat.link(editor->joints[i].offset + 56, replacement);
    }
    if (!compensate_meshes(dat, *editor, *old, old_world.get(), new_world.get(), changed)) return false;
    std::vector<u8> result;
    if (!dat.finish(&result)) return false;
    auto owned = std::unique_ptr<u8, decltype(&std::free)>((u8*) std::malloc(result.size()), &std::free);
    if (!owned) return false;
    std::memcpy(owned.get(), result.data(), result.size());
    NativeFighterProject candidate = *editor->project;
    candidate.costume_dat = owned.get(); candidate.costume_size = result.size();
    auto verified = std::make_unique<NativeFighterModelEditor>();
    if (!nativeFighterModel_Open(verified.get(), &candidate)) return false;
    NativeFighterProject* project = editor->project;
    std::free(project->costume_dat);
    project->costume_dat = owned.release(); project->costume_size = result.size();
    *editor = *verified;
    editor->project = project;
    return true;
}
