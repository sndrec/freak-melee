#include "native_fighter_gltf_identity.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

namespace {
std::string mesh_name(const cgltf_data& data,const cgltf_mesh& mesh)
{
    if(mesh.name && *mesh.name) return mesh.name;
    for(cgltf_size i=0;i<data.nodes_count;++i)
        if(data.nodes[i].mesh==&mesh && data.nodes[i].name && *data.nodes[i].name)
            return data.nodes[i].name;
    return {};
}
std::string key(const cgltf_data& data,const cgltf_mesh& mesh,const cgltf_primitive& primitive)
{
    const auto name=mesh_name(data,mesh);
    if(name.empty()) return {};
    std::string identity="mesh:"+std::to_string(name.size())+":"+name;
    if(mesh.primitives_count>1) {
        if(!primitive.material || !primitive.material->name || !*primitive.material->name) return {};
        std::string material=primitive.material->name;
        identity+=";slot:"+std::to_string(material.size())+":"+material;
    }
    if(identity.size()>=256) return {};
    for(unsigned char c:identity) if(c<32 || c==127) return {};
    return identity;
}
}

bool nativeGltfIdentity(const cgltf_data* data,const cgltf_mesh* mesh,
                        const cgltf_primitive* primitive,char identity[256])
{
    identity[0]=0;
    const auto wanted=key(*data,*mesh,*primitive);
    if(wanted.empty()) return false;
    unsigned matches=0;
    for(cgltf_size m=0;m<data->meshes_count;++m)
        for(cgltf_size p=0;p<data->meshes[m].primitives_count;++p)
            matches+=key(*data,data->meshes[m],data->meshes[m].primitives[p])==wanted;
    if(matches!=1) return false;
    std::memcpy(identity,wanted.c_str(),wanted.size()+1);return true;
}
uint64_t nativeGltfTopology(const cgltf_primitive* primitive)
{
    cgltf_size vertices=0;
    for(cgltf_size a=0;a<primitive->attributes_count;++a)
        if(primitive->attributes[a].type==cgltf_attribute_type_position && primitive->attributes[a].index==0)
            vertices=primitive->attributes[a].data->count;
    const cgltf_size count=primitive->indices?primitive->indices->count:vertices;
    uint64_t hash=UINT64_C(14695981039346656037);
    auto word=[&](uint64_t value) {
        for(unsigned b=0;b<8;++b) {hash^=value&255;hash*=UINT64_C(1099511628211);value>>=8;}
    };
    word(primitive->type);word(vertices);word(count);
    for(cgltf_size i=0;i<count;++i) word(primitive->indices?cgltf_accessor_read_index(primitive->indices,i):i);
    return hash;
}
bool nativeGltfSourcePath(const char* source,char path[512])
{
    std::error_code error;
    const auto resolved=std::filesystem::weakly_canonical(std::filesystem::u8path(source),error);
    if(error) return false;
    const auto text=resolved.generic_u8string();
    if(text.empty()||text.size()>=512) return false;
    std::memcpy(path,text.data(),text.size());path[text.size()]=0;return true;
}
