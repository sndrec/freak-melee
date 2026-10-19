#include "native_fighter_gltf_skin.h"
#include "native_fighter_mesh_source.h"
#include "native_fighter_authoring.h"

extern "C" {
#include <dolphin/mtx.h>
#include <sysdolphin/baselib/jobj.h>
}
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <utility>

namespace {
struct AttributeSet {
    int index;
    const cgltf_accessor* joints = nullptr;
    const cgltf_accessor* weights = nullptr;
};
struct Joint {
    unsigned target;
    bool used = false;
};

bool problem(char* error, size_t size, const char* message, const char* name = "")
{
    if (error && size) std::snprintf(error, size, message, name);
    return false;
}

bool read_matrix(const float* input, float scale, Mtx out)
{
    for (unsigned i = 0; i < 16; ++i)
        if (!std::isfinite(input[i])) return false;
    if (std::fabs(input[3]) > 1e-6f || std::fabs(input[7]) > 1e-6f ||
        std::fabs(input[11]) > 1e-6f || std::fabs(input[15]-1) > 1e-6f)
        return false;
    for (unsigned r = 0; r < 3; ++r)
        for (unsigned c = 0; c < 4; ++c) {
            out[r][c] = input[c*4+r] * (c == 3 ? scale : 1);
            if (!std::isfinite(out[r][c])) return false;
        }
    return true;
}

bool inverse_bind(const NativeFighterModelEditor& model, unsigned target, Mtx out)
{
    u32 offset;
    const char* external;
    if (!(model.joints[target].flags & JOBJ_SKELETON) ||
        HSD_DatResolve(&model.dat, model.joints[target].offset+56,
                       &offset, &external) != HSD_DAT_REF_LOCAL ||
        !HSD_DatBytes(&model.dat, offset, sizeof(Mtx))) return false;
    for (unsigned r = 0; r < 3; ++r)
        for (unsigned c = 0; c < 4; ++c)
            if (!HSD_DatReadF32(&model.dat, offset+(r*4+c)*4, &out[r][c]) ||
                !std::isfinite(out[r][c])) return false;
    return true;
}

bool same_bind_shape(const Mtx a, const Mtx b)
{
    for (unsigned r = 0; r < 3; ++r)
        for (unsigned c = 0; c < 4; ++c) {
            const float magnitude = std::max(std::fabs(a[r][c]),std::fabs(b[r][c]));
            const float tolerance = c == 3 ? .002f+1e-5f*magnitude : .0001f*(1+magnitude);
            if (!std::isfinite(a[r][c]) || !std::isfinite(b[r][c]) ||
                std::fabs(a[r][c]-b[r][c]) > tolerance) return false;
        }
    return true;
}

bool map_joints(const NativeFighterModelEditor& model, unsigned owner,
    const cgltf_skin& skin, std::vector<Joint>& mapping,
    char* error, size_t size)
{
    int source_for_target[NATIVE_MODEL_MAX_JOINTS];
    std::fill_n(source_for_target,NATIVE_MODEL_MAX_JOINTS,-1);
    mapping.resize(skin.joints_count);
    for (unsigned i = 0; i < skin.joints_count; ++i) {
        const char* name = skin.joints[i]->name;
        if (!name || !*name)
            return problem(error,size,"Every source skin bone needs a name matching the fighter rig.");
        unsigned match = model.joint_count, count = 0;
        for (unsigned j = 0; j < model.joint_count; ++j) {
            const char* role = nativeFighterAuthoring_BoneRole(model.project,j);
            if (!std::strcmp(name,model.joints[j].name) ||
                (role && !std::strcmp(name,role))) {
                match = j;
                ++count;
            }
        }
        if (count != 1)
            return problem(error,size,count ? "Source bone '%s' matches more than one fighter bone. Give it a unique name."
                : "Source bone '%s' has no matching fighter bone. Rename it or choose generated weights.",name);
        if (source_for_target[match] >= 0)
            return problem(error,size,"More than one source bone maps to '%s'.",name);
        int parent = (int) match;
        while (parent >= 0 && parent != (int) owner)
            parent = model.joints[parent].parent;
        if (parent < 0)
            return problem(error,size,"Source bone '%s' belongs to another fighter skeleton.",name);
        source_for_target[match] = (int) i;
        mapping[i].target = match;
    }
    for (unsigned i = 0; i < skin.joints_count; ++i) {
        int source_parent = -1;
        for (const cgltf_node* parent = skin.joints[i]->parent; parent; parent = parent->parent) {
            for (unsigned j = 0; j < skin.joints_count; ++j)
                if (skin.joints[j] == parent) { source_parent = (int) j; break; }
            if (source_parent >= 0) break;
        }
        int target_parent = model.joints[mapping[i].target].parent;
        while (target_parent >= 0 && source_for_target[target_parent] < 0)
            target_parent = model.joints[target_parent].parent;
        const int expected = target_parent >= 0 ? source_for_target[target_parent] : -1;
        if (source_parent != expected)
            return problem(error,size,"Source bone '%s' has a different parent hierarchy from the fighter rig.",skin.joints[i]->name);
    }
    return true;
}

bool read_sets(const cgltf_primitive& primitive, cgltf_size vertices,
    std::vector<AttributeSet>& sets, char* error, size_t size)
{
    for (cgltf_size i = 0; i < primitive.attributes_count; ++i) {
        const cgltf_attribute& a = primitive.attributes[i];
        if (a.type != cgltf_attribute_type_joints && a.type != cgltf_attribute_type_weights)
            continue;
        if (a.index < 0) return problem(error,size,"Skin attribute indices must be nonnegative.");
        auto found = std::find_if(sets.begin(),sets.end(),[&](const AttributeSet& s){return s.index==a.index;});
        if (found == sets.end()) { sets.push_back({a.index}); found = sets.end()-1; }
        const cgltf_accessor*& slot = a.type == cgltf_attribute_type_joints ? found->joints : found->weights;
        if (slot) return problem(error,size,"The mesh has duplicate skin attributes.");
        slot = a.data;
    }
    std::sort(sets.begin(),sets.end(),[](const AttributeSet& a,const AttributeSet& b){return a.index<b.index;});
    if (sets.empty() || sets[0].index != 0)
        return problem(error,size,"Source weights need JOINTS_0 and WEIGHTS_0 attributes.");
    for (const AttributeSet& set : sets) {
        if (!set.joints || !set.weights || set.joints->type != cgltf_type_vec4 ||
            set.weights->type != cgltf_type_vec4 || set.joints->count != vertices ||
            set.weights->count != vertices || set.joints->normalized ||
            (set.joints->component_type != cgltf_component_type_r_8u &&
             set.joints->component_type != cgltf_component_type_r_16u) ||
            !((set.weights->component_type == cgltf_component_type_r_32f && !set.weights->normalized) ||
              ((set.weights->component_type == cgltf_component_type_r_8u ||
                set.weights->component_type == cgltf_component_type_r_16u) && set.weights->normalized)))
            return problem(error,size,"Each JOINTS/WEIGHTS pair needs matching VEC4 vertices and valid skin component types.");
    }
    return true;
}
} // namespace

bool nativeFighterGltf_ReadSkin(const NativeFighterModelEditor& model,
    unsigned owner, const cgltf_node* node, const cgltf_primitive* primitive,
    float scale, NativeGltfSourceSkin* output, char* error, size_t size)
{
    if (!node || !node->skin || !primitive || !output ||
        owner >= model.joint_count || !std::isfinite(scale) || scale <= 0)
        return problem(error,size,"Choose a mesh with a source skin and a compatible fighter skeleton.");
    const cgltf_skin& skin = *node->skin;
    if (!skin.joints_count || skin.joints_count > NATIVE_MODEL_MAX_JOINTS)
        return problem(error,size,"The source skin has an unsupported number of bones.");
    const cgltf_accessor* positions = nullptr;
    for (cgltf_size i = 0; i < primitive->attributes_count; ++i)
        if (primitive->attributes[i].type == cgltf_attribute_type_position && primitive->attributes[i].index == 0)
            positions = primitive->attributes[i].data;
    if (!positions || !positions->count || positions->count > NATIVE_MESH_SOURCE_MAX_VERTICES)
        return problem(error,size,"Source skin import needs 1 to 1000000 mesh vertices.");
    std::vector<Joint> mapping;
    std::vector<AttributeSet> sets;
    if (!map_joints(model,owner,skin,mapping,error,size) ||
        !read_sets(*primitive,positions->count,sets,error,size)) return false;
    const cgltf_accessor* inverse = skin.inverse_bind_matrices;
    if (inverse && (inverse->type != cgltf_type_mat4 || inverse->count != skin.joints_count ||
                    inverse->component_type != cgltf_component_type_r_32f || inverse->normalized))
        return problem(error,size,"The source inverse-bind table must contain one floating MAT4 per skin bone.");
    NativeGltfSourceSkin result;
    result.vertices.resize(positions->count);
    for (unsigned v = 0; v < positions->count; ++v) {
        NativeSkinWeights& weights = result.vertices[v];
        for (const AttributeSet& set : sets) {
            cgltf_uint indices[4];
            float values[4];
            if (!cgltf_accessor_read_uint(set.joints,v,indices,4) ||
                !cgltf_accessor_read_float(set.weights,v,values,4))
                return problem(error,size,"Could not read a source skin vertex.");
            for (unsigned b = 0; b < 4; ++b) {
                if (indices[b] >= mapping.size() || !std::isfinite(values[b]) ||
                    values[b] < 0 || values[b] > 1)
                    return problem(error,size,"A source vertex has an invalid bone index or weight.");
                if (values[b] == 0) continue;
                Joint& joint = mapping[indices[b]];
                joint.used = true;
                unsigned at = 0;
                while (at < weights.count && weights.joints[at] != joint.target) ++at;
                if (at == weights.count) {
                    if (weights.count == NATIVE_SKIN_MAX_INFLUENCES)
                        return problem(error,size,"A source vertex has more than 16 bone influences.");
                    weights.joints[weights.count++] = (uint16_t) joint.target;
                }
                weights.weights[at] += values[b];
            }
        }
        if (!nativeSkinWeights_Normalize(&weights))
            return problem(error,size,"Every source vertex needs a positive skin weight.");
    }
    Mtx common;
    bool have_shape = false;
    for (unsigned i = 0; i < mapping.size(); ++i) {
        if (!mapping[i].used) continue;
        float input[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
        Mtx source_inverse, source_bind, target_inverse, target_bind, shape;
        if ((inverse && !cgltf_accessor_read_float(inverse,i,input,16)) ||
            !read_matrix(input,scale,source_inverse) || !C_MTXInverse(source_inverse,source_bind) ||
            !inverse_bind(model,mapping[i].target,target_inverse) ||
            !C_MTXInverse(target_inverse,target_bind))
            return problem(error,size,"Source bone '%s' needs a finite, invertible inverse bind and a target deform bone.",skin.joints[i]->name);
        C_MTXConcat(target_bind,source_inverse,shape);
        if (have_shape && !same_bind_shape(common,shape))
            return problem(error,size,"Source bone '%s' has a different bind pose. Check the rig, proportions and import scale.",skin.joints[i]->name);
        if (!have_shape) { C_MTXCopy(shape,common); have_shape = true; }
    }
    if (!have_shape) return problem(error,size,"The source skin has no weighted bones.");
    for (unsigned r = 0; r < 3; ++r)
        for (unsigned c = 0; c < 4; ++c) result.bind_shape[c*4+r] = common[r][c];
    result.bind_shape[15] = 1;
    *output = std::move(result);
    return true;
}
