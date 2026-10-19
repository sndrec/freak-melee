#include "native_fighter_skin_morph.h"
#include "native_asset_dat.h"
extern "C" {
#include <melee/ft/ftcostume_assets.h>
#include <sysdolphin/baselib/shape.h>
#include <sysdolphin/baselib/mtx.h>
}
#include <cmath>
#include <unordered_map>

namespace {
bool reference(const HSD_Dat& dat, u32 slot, u32* target)
{
    const char* external;
    return HSD_DatResolve(&dat, slot, target, &external) == HSD_DAT_REF_LOCAL;
}
unsigned index(const u8* bytes, unsigned type)
{
    return type == GX_INDEX8 ? bytes[0] : unsigned(bytes[0]) << 8 | bytes[1];
}
bool targets(const HSD_ShapeSetDesc& shape, bool normal, Mtx conversion,
               std::vector<float>* values)
{
    const unsigned count = normal ? shape.nb_normal_index : shape.nb_vertex_index;
    if (!count) return true;
    const unsigned components = normal && shape.normal_desc->attr == GX_VA_NBT ? 9 : 3;
    const unsigned total = shape.nb_shape + ((shape.flags & SHAPESET_ADDITIVE) != 0);
    const size_t floats = size_t(total) * count * components;
    if (floats > FT_COSTUME_IMPORT_MAX_ARENA_SIZE / sizeof(float)) return false;
    values->resize(floats);
    for (unsigned target = 0; target < total; ++target) {
        for (unsigned vertex = 0; vertex < count; ++vertex) {
            float decoded[9];
            HSD_ShapeSetReadVector(normal ? shape.normal_desc : shape.vertex_desc,
                normal ? shape.normal_idx_list[target] : shape.vertex_idx_list[target],
                vertex, components, decoded);
            for (unsigned c = 0; c < components; c += 3) {
                Vec3 point, output;
                std::memcpy(&point, decoded + c, sizeof(point));
                if (normal || (target && (shape.flags & SHAPESET_ADDITIVE)))
                    C_MTXMultVecSR(conversion, &point, &output);
                else C_MTXMultVec(conversion, &point, &output);
                if (!std::isfinite(output.x) || !std::isfinite(output.y) || !std::isfinite(output.z)) return false;
                std::memcpy(values->data() + (size_t(target)*count+vertex)*components+c, &output, sizeof(output));
            }
        }
    }
    return true;
}

bool envelope(const HSD_EnvelopeDesc* input,
                const std::unordered_map<const HSD_Joint*, unsigned>& joints,
                NativeSkinWeights* weights)
{
    double sums[NATIVE_MODEL_MAX_JOINTS] = {};
    for (; input->joint; ++input) {
        const auto found = joints.find(input->joint);
        if (found == joints.end()) return false;
        sums[found->second] += input->weight;
    }
    *weights = {};
    for (unsigned i = 0; i < NATIVE_MODEL_MAX_JOINTS; ++i) {
        if (!sums[i]) continue;
        if (weights->count == NATIVE_SKIN_MAX_INFLUENCES) return false;
        weights->joints[weights->count] = (u16)i;
        weights->weights[weights->count++] = (float)sums[i];
    }
    return weights->count != 0;
}
}

int nativeSkinMorph_RigidDomain(const NativeFighterModelEditor& model,unsigned owner,
                                 Mtx conversion,Mtx normal_conversion)
{
    if(owner>=model.joint_count)return -1;
    int rigid=(int)owner;
    while(rigid>=0 && !(model.joints[rigid].flags&(JOBJ_SKELETON|JOBJ_SKELETON_ROOT)))rigid=model.joints[rigid].parent;
    if(rigid<0)return -1;
    Mtx inverse;C_MTXIdentity(conversion);C_MTXIdentity(normal_conversion);
    if(!nativeSkinMesh_InverseBind(model,rigid,inverse))return -1;
    if(model.joints[owner].flags&JOBJ_SKELETON_ROOT) {
        if(!C_MTXInverse(inverse,conversion))return -1;
        C_MTXTranspose(inverse,normal_conversion);
    } else if(model.joints[rigid].flags&JOBJ_SKELETON_ROOT) {
        for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)
            if(inverse[r][c]!=(r==c?1.f:0.f))return -1;
    }
    return rigid;
}

bool nativeSkinMorph_Read(const NativeFighterModelEditor& model, unsigned owner,
                           const FtCostumeAssets& imported, NativeSkinPart* part,
                           NativeSkinMesh* mesh)
{
    const HSD_PObjDesc* polygon = nullptr;
    std::unordered_map<const HSD_Joint*, unsigned> joints;
    for (unsigned i = 0; i < imported.import.record_count; ++i) {
        const auto& record = imported.import.records[i];
        if (record.type == DAT_POBJ && record.offset == part->source)
            polygon = (const HSD_PObjDesc*)record.object;
        if (record.type == DAT_JOINT) {
            for (unsigned bone = 0; bone < model.joint_count; ++bone)
                if (model.joints[bone].offset == record.offset)
                    joints.emplace((const HSD_Joint*)record.object, bone);
        }
    }
    if (!polygon || !polygon->u.shape_set || !polygon->display || !polygon->n_display) return false;
    const auto& shape = *polygon->u.shape_set;
    auto& morph = part->morph;
    morph.flags = shape.flags;
    morph.shape_count = shape.nb_shape;
    if(shape.default_blend)morph.default_blend.assign(shape.default_blend,
        shape.default_blend+(shape.flags&SHAPESET_ADDITIVE?shape.nb_shape:1));
    if(shape.target_names)for(unsigned i=0;i<shape.nb_shape;++i)morph.target_names.emplace_back(shape.target_names[i]);
    morph.position_count = shape.nb_vertex_index;
    morph.normal_count = shape.nb_normal_index;
    morph.normal_components = morph.normal_count ? shape.normal_desc->attr == GX_VA_NBT ? 9 : 3 : 0;
    if (!morph.position_count || mesh->vertices.size() > UINT32_MAX - morph.position_count) return false;
    Mtx conversion, normal_conversion;
    C_MTXIdentity(conversion); C_MTXIdentity(normal_conversion);
    int rigid=(int)owner;
    if(morph.flags&SHAPESET_SKIN)
        while(rigid>=0 && !(model.joints[rigid].flags&(JOBJ_SKELETON|JOBJ_SKELETON_ROOT)))rigid=model.joints[rigid].parent;
    else rigid=nativeSkinMorph_RigidDomain(model,owner,conversion,normal_conversion);
    if(rigid<0)return false;
    if (!targets(shape, false, conversion, &morph.positions) ||
        !targets(shape, true, normal_conversion, &morph.normals)) return false;
    const unsigned first_vertex = (unsigned)mesh->vertices.size();
    for (unsigned i = 0; i < morph.position_count; ++i) {
        NativeSkinVertex vertex = {};
        std::memcpy(vertex.position, morph.positions.data()+i*3, sizeof(vertex.position));
        vertex.skin.count = 1; vertex.skin.joints[0] = (u16)rigid; vertex.skin.weights[0] = 1;
        morph.vertices.push_back(first_vertex+i); mesh->vertices.push_back(vertex);
    }
    u32 descriptors;
    if (!reference(model.dat, part->source+8, &descriptors) ||
        !nativeSkinMesh_ReadAttributes(model.dat, descriptors, &part->attributes)) return false;
    const u8* cursor = polygon->display;
    const u8* end = cursor + size_t(polygon->n_display)*32;
    while (cursor < end) {
        const unsigned command = *cursor++;
        if (command == GX_NOP) continue;
        if (end-cursor < 2) return false;
        const unsigned count = unsigned(cursor[0]) << 8 | cursor[1]; cursor += 2;
        const unsigned start = (unsigned)part->corners.size();
        for (unsigned i = 0; i < count; ++i) {
            NativeSkinCorner corner = {};
            corner.source = (u32)(cursor-model.dat.data);
            unsigned vertex = UINT32_MAX;
            for (const auto& attr : part->attributes) {
                if (size_t(end-cursor) < attr.display_bytes) return false;
                if (attr.attr == GX_VA_POS) vertex = index(cursor, attr.type);
                else if (attr.attr == GX_VA_NRM || attr.attr == GX_VA_NBT)
                    corner.normal_index = index(cursor, attr.type);
                cursor += attr.display_bytes;
            }
            if (vertex >= morph.position_count || (morph.normal_count && corner.normal_index >= morph.normal_count)) return false;
            corner.vertex = first_vertex+vertex;
            if (morph.normal_count) std::memcpy(corner.normal,
                morph.normals.data()+corner.normal_index*morph.normal_components, morph.normal_components*sizeof(float));
            part->corners.push_back(corner); mesh->corner_vertices.push_back(corner.vertex);
        }
        auto triangle = [&](unsigned a, unsigned b, unsigned c) {
            part->triangles.insert(part->triangles.end(), {start+a, start+b, start+c});
        };
        if (command == GX_TRIANGLES) for (unsigned i = 0; i < count; i += 3) triangle(i, i+1, i+2);
        else if (command == GX_QUADS) for (unsigned i = 0; i < count; i += 4) {
            triangle(i, i+1, i+2); triangle(i, i+2, i+3);
        } else for (unsigned i = 2; i < count; ++i) {
            if (command == GX_TRIANGLEFAN) triangle(0, i-1, i);
            else if (command == GX_TRIANGLESTRIP) triangle(i%2 ? i-1 : i-2, i%2 ? i-2 : i-1, i);
            else return false;
        }
    }
    if (!(morph.flags & SHAPESET_SKIN)) return !part->corners.empty();
    std::vector<bool> assigned(morph.position_count);
    size_t corner_index = 0;
    for (const HSD_PObjDesc* batch = shape.skin; batch; batch = batch->next) {
        std::vector<NativeSkinWeights> bindings;
        for (unsigned i = 0; batch->u.envelope_p[i]; ++i) {
            NativeSkinWeights weights;
            if (!envelope(batch->u.envelope_p[i], joints, &weights)) return false;
            bindings.push_back(weights);
        }
        cursor = batch->display; end = cursor + size_t(batch->n_display)*32;
        while (cursor < end) {
            if (*cursor++ == GX_NOP) continue;
            const unsigned count = unsigned(cursor[0]) << 8 | cursor[1]; cursor += 2;
            for (unsigned i = 0; i < count; ++i, ++corner_index) {
                unsigned matrix = 0;
                for (unsigned hardware = 0; hardware <= GX_VA_TEX7; ++hardware) {
                    for (const HSD_VtxDescList* attr = batch->verts; attr->attr != GX_VA_NULL; ++attr) {
                        if ((attr->attr == GX_VA_NBT ? GX_VA_NRM : attr->attr) != hardware || attr->attr_type == GX_NONE) continue;
                        if (hardware == GX_VA_PNMTXIDX) matrix = cursor[0]/3;
                        const unsigned width = attr->attr_type == GX_INDEX8 ? 1 : attr->attr_type == GX_INDEX16 ? 2 :
                            hardware <= GX_VA_TEX7MTXIDX ? 1 : 0;
                        // Direct color/UV widths are already known from the
                        // matching, validated parent attribute stream.
                        unsigned direct = width;
                        if (!direct) for (const auto& a : part->attributes)
                            if ((a.attr == GX_VA_NBT ? GX_VA_NRM : a.attr) == hardware) direct = a.display_bytes;
                        if (!direct || size_t(end-cursor) < direct) return false;
                        cursor += direct;
                    }
                }
                if (corner_index >= part->corners.size() || matrix >= bindings.size()) return false;
                const unsigned vertex = part->corners[corner_index].vertex-first_vertex;
                auto& weights = mesh->vertices[first_vertex+vertex].skin;
                if (assigned[vertex] && !nativeSkinWeights_Equal(&weights, &bindings[matrix])) return false;
                weights = bindings[matrix]; assigned[vertex] = true;
            }
        }
    }
    return corner_index == part->corners.size();
}
