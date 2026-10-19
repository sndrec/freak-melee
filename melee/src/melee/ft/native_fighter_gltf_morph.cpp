#include "native_fighter_gltf_morph.h"
#include "native_fighter_skin_morph.h"
#include "native_asset_dat.h"
extern "C" {
#include <melee/ft/ftcostume_assets.h>
#include <sysdolphin/baselib/shape.h>
#include <sysdolphin/baselib/mtx.h>
}
#include <cmath>
#include <cstdio>
#include <numeric>
#include <set>

namespace {
bool target_order(const NativeGltfMorphImport& input,std::vector<unsigned>* order,
                    std::vector<std::string>* names,char* error,size_t size)
{
    auto fail=[&](const char* message){if(error&&size)std::snprintf(error,size,"%s",message);return false;};
    const unsigned count=(unsigned)input.primitive->targets_count;
    const bool named=input.mesh->target_names_count!=0;
    if(named && input.mesh->target_names_count!=count)return fail("Morph target names must match this primitive's target count.");
    std::set<std::string> unique;
    if(named)for(unsigned i=0;i<count;++i) {
        const char* name=input.mesh->target_names[i];
        if(!name || !*name || std::strlen(name)>127 || !unique.emplace(name).second)
            return fail("Morph target names must be unique and contain at most 127 bytes.");
    }
    const auto* previous=input.previous;
    if(previous && previous->morph_count) {
        if(previous->morph_order.empty()) {
            if(previous->morph_count!=count)return fail("An unnamed morph mesh needs the same target count and order on reimport.");
        } else {
            if(!named)return fail("The source export is missing its morph target names.");
            for(const auto& name:previous->morph_order) {
                unsigned found=0;while(found<count && name!=input.mesh->target_names[found])++found;
                if(found==count) {
                    if(error&&size)std::snprintf(error,size,"The source export is missing morph target '%s'.",name.c_str());
                    return false;
                }
                order->push_back(found);
            }
        }
    }
    for(unsigned i=0;i<count;++i)
        if(std::find(order->begin(),order->end(),i)==order->end())order->push_back(i);
    if(named)for(unsigned i:*order)names->emplace_back(input.mesh->target_names[i]);
    return true;
}

void transform(Mtx matrix,const float* point,float* output,bool offset)
{
    Vec3 a={point[0],point[1],point[2]},b;
    if(offset)C_MTXMultVecSR(matrix,&a,&b);else C_MTXMultVec(matrix,&a,&b);
    std::memcpy(output,&b,sizeof(b));
}
}

bool nativeFighterGltf_EncodeMorph(NativeAssetDat& dat,const NativeFighterModelEditor& model,
    unsigned owner,const NativeGltfMorphImport& input,uint32_t* polygon,
    NativeFighterMeshSource* metadata,char* error,size_t error_size)
{
    auto fail=[&](const char* message){if(error&&error_size)std::snprintf(error,error_size,"%s",message);return false;};
    const auto* primitive=input.primitive;
    const unsigned vertices=(unsigned)(input.positions.size()/3);
    const size_t targets=primitive->targets_count;
    if(!vertices || vertices>HSD_DEFAULT_MAX_SHAPE_VERTICES || vertices>HSD_DEFAULT_MAX_SHAPE_NORMALS ||
        !targets || targets>UINT16_MAX || (targets+1)*vertices*3>FT_COSTUME_IMPORT_MAX_ARENA_SIZE/sizeof(float))
        return fail("This morph mesh exceeds the current position, normal or target storage capacity.");
    if((input.node && input.node->weights_count && input.node->weights_count!=targets) ||
       (input.mesh->weights_count && input.mesh->weights_count!=targets))
        return fail("Morph default weights must match this primitive's target count.");
    NativeSkinMesh mesh;NativeSkinPart part={};part.source=UINT32_MAX;part.flags=POBJ_SHAPEANIM;
    auto& morph=part.morph;morph.flags=SHAPESET_ADDITIVE|SHAPESET_SIGNED_WEIGHTS;
    morph.shape_count=(u16)targets;morph.position_count=morph.normal_count=vertices;morph.normal_components=3;
    std::vector<unsigned> order;
    if(!target_order(input,&order,&morph.target_names,error,error_size))return false;
    morph.default_blend.resize(targets);
    const float* defaults=input.node && input.node->weights_count?input.node->weights:input.mesh->weights;
    for(unsigned i=0;i<targets;++i) {
        const float weight=defaults?defaults[order[i]]:0;
        if(!std::isfinite(weight))return fail("Morph target weights must be finite.");
        morph.default_blend[i]=weight;
    }
    Mtx position_domain,normal_domain;C_MTXIdentity(position_domain);C_MTXIdentity(normal_domain);
    int rigid=-1;
    if(!input.weights) {
        rigid=nativeSkinMorph_RigidDomain(model,owner,position_domain,normal_domain);
        if(rigid<0)return fail("This attachment has no supported morph skin coordinate domain.");
        // A later Keep binding reimport needs this same canonical domain,
        // including the rigid owner's inverse-bind conversion.
        float placed[16]={};placed[15]=1;
        for(unsigned r=0;r<3;++r) {
            for(unsigned c=0;c<3;++c)for(unsigned k=0;k<3;++k)
                placed[r+c*4]+=position_domain[r][k]*input.transform[k+c*4];
            placed[r+12]=position_domain[r][3];
            for(unsigned k=0;k<3;++k)placed[r+12]+=position_domain[r][k]*input.transform[k+12]*input.scale;
        }
        std::memcpy(metadata->transform,placed,sizeof(placed));metadata->uses_bind_shape=true;
    }
    morph.positions.resize((targets+1)*vertices*3);morph.normals.resize((targets+1)*vertices*3);
    for(unsigned i=0;i<vertices;++i) {
        NativeSkinVertex vertex={};transform(position_domain,input.positions.data()+i*3,vertex.position,false);
        std::copy(vertex.position,vertex.position+3,morph.positions.data()+i*3);
        transform(normal_domain,input.normals.data()+i*3,morph.normals.data()+i*3,true);
        if(input.weights)vertex.skin=(*input.weights)[i];
        else {vertex.skin.count=1;vertex.skin.joints[0]=(u16)rigid;vertex.skin.weights[0]=1;}
        mesh.vertices.push_back(vertex);morph.vertices.push_back(i);
    }
    for(unsigned target=0;target<targets;++target) {
        const auto& source=primitive->targets[order[target]];
        const cgltf_accessor* position=nullptr;const cgltf_accessor* normal=nullptr;
        for(cgltf_size a=0;a<source.attributes_count;++a) {
            const auto& attr=source.attributes[a];
            if(attr.type==cgltf_attribute_type_custom)continue;
            if(attr.index || (attr.type!=cgltf_attribute_type_position && attr.type!=cgltf_attribute_type_normal))
                return fail("Morph import currently supports position and normal target attributes.");
            const auto*& output=attr.type==cgltf_attribute_type_position?position:normal;
            if(output || attr.data->type!=cgltf_type_vec3 || attr.data->count!=vertices)
                return fail("Each morph attribute needs one vector per source vertex.");
            output=attr.data;
        }
        for(unsigned i=0;i<vertices;++i) {
            float delta[3]={},direction[3]={},placed[3]={},lit[3]={};
            if((position && !cgltf_accessor_read_float(position,i,delta,3)) ||
               (normal && !cgltf_accessor_read_float(normal,i,direction,3)))return fail("Could not read a morph target.");
            for(unsigned r=0;r<3;++r)for(unsigned k=0;k<3;++k) {
                placed[r]+=input.transform[r+k*4]*delta[k]*input.scale;
                lit[r]+=input.normal_transform[r+k*3]*direction[k]*input.normal_scale[i];
            }
            const size_t at=(size_t(target+1)*vertices+i)*3;
            transform(position_domain,placed,morph.positions.data()+at,true);
            transform(normal_domain,lit,morph.normals.data()+at,true);
            for(unsigned r=0;r<3;++r)if(!std::isfinite(morph.positions[at+r]) || !std::isfinite(morph.normals[at+r]))
                return fail("Morph targets must contain finite vectors.");
        }
    }
    const unsigned attributes=input.texcoords?3:2;
    part.attribute_bytes.resize(attributes*24);
    for(unsigned i=0;i<attributes;++i) {
        NativeSkinAttribute attr={};attr.source=i*24;
        attr.attr=i==0?GX_VA_POS:i==1?GX_VA_NRM:GX_VA_TEX0;
        attr.type=GX_INDEX16;attr.count=i==1?0:1;attr.format=GX_F32;
        attr.components=i==2?2:3;attr.stride=attr.width=attr.components*4;attr.display_bytes=2;
        attr.array=i==2?input.texcoords:0;
        auto* bytes=part.attribute_bytes.data()+attr.source;
        NativeAssetDat::be32(bytes,attr.attr);NativeAssetDat::be32(bytes+4,attr.type);
        NativeAssetDat::be32(bytes+8,attr.count);NativeAssetDat::be32(bytes+12,attr.format);
        NativeAssetDat::be16(bytes+18,(u16)attr.stride);part.attributes.push_back(attr);
    }
    const size_t corners=primitive->indices?primitive->indices->count:vertices;
    for(size_t start=0;start<corners;start+=3)for(unsigned c=0;c<3;++c) {
        const unsigned corner=metadata->mirrored?(c==1?2:c==2?1:0):c;
        const size_t vertex=primitive->indices?cgltf_accessor_read_index(primitive->indices,start+corner):start+corner;
        if(vertex>=vertices)return fail("Morph mesh has an invalid vertex index.");
        NativeSkinCorner value={};value.vertex=(unsigned)vertex;value.normal_index=(unsigned)vertex;
        value.source=(uint32_t)part.attribute_bytes.size();
        for(unsigned a=0;a<attributes;++a) {
            part.attribute_bytes.push_back((u8)(vertex>>8));part.attribute_bytes.push_back((u8)vertex);
        }
        part.triangles.push_back((unsigned)part.corners.size());part.corners.push_back(value);
    }
    mesh.parts.push_back(std::move(part));
    if(!nativeSkinMesh_Encode(dat,model,mesh,polygon))return fail("Could not encode the morph mesh and its skin batches.");
    return true;
}
