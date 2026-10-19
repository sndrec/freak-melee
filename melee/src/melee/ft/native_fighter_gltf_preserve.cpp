#include "native_fighter_gltf_preserve.h"
#include "native_fighter_gltf_identity.h"
#include "native_fighter_skin_mesh.h"
#include <sysdolphin/baselib/displayfunc.h>
#include <sysdolphin/baselib/jobj.h>
#include <cmath>
#include <cstdio>
#include <cstring>

bool nativeFighterGltf_Preserve(const NativeFighterModelEditor& model,unsigned mesh,
    const cgltf_node* node,const cgltf_primitive* primitive,const NativeFighterMeshSource& source,
    float scale,NativeGltfSourceSkin* output,bool* rigid,char* error,size_t capacity)
{
    auto fail=[&](const char* message) {
        if(error&&capacity)std::snprintf(error,capacity,"%s",message);return false;
    };
    if(mesh>=model.mesh_count || !output || !rigid) return fail("Choose an imported mesh to preserve.");
    const unsigned owner=model.meshes[mesh].joint;
    const char* external;u32 polygon;
    if(HSD_DatResolve(&model.dat,model.meshes[mesh].offset+12,&polygon,&external)!=HSD_DAT_REF_LOCAL)
        return fail("Could not read the mesh's attachment.");
    bool attached=true;
    for(unsigned p=0;p<model.meshes[mesh].polygon_count;++p) {
        const u8* flags=HSD_DatBytes(&model.dat,polygon+12,2);u32 joint;
        if(!flags) return fail("Could not read the mesh's geometry.");
        const unsigned type = ((unsigned)flags[0]<<8|flags[1])&0x3000;
        if(type==POBJ_SHAPEANIM)
            return fail("Keeping this mesh's morph targets requires target-aware reimport.");
        const auto reference=HSD_DatResolve(&model.dat,polygon+20,&joint,&external);
        attached &= type==POBJ_SKIN &&
            reference==HSD_DAT_REF_LOCAL && joint==model.joints[owner].offset;
        if(p+1<model.meshes[mesh].polygon_count &&
           HSD_DatResolve(&model.dat,polygon+4,&polygon,&external)!=HSD_DAT_REF_LOCAL)
            return fail("Could not read all mesh parts.");
    }
    if(attached) {*rigid=true;*output={};return true;}
    if(source.topology!=nativeGltfTopology(primitive))
        return fail("The source topology changed. Choose weight transfer or a new binding explicitly.");
    NativeSkinMesh current;
    if(!nativeSkinMesh_Read(model,mesh,&current,error,capacity)) return false;
    size_t corners=0;
    for(const auto& part:current.parts) corners+=part.triangles.size();
    if(corners!=source.corners) return fail("The edited mesh no longer has its imported corner order. Choose weight transfer.");
    NativeGltfSourceSkin kept;
    kept.vertices.resize(source.vertices);
    size_t at=0;
    for(const auto& part:current.parts) for(unsigned triangle:part.triangles) {
        if(triangle>=part.corners.size() || part.corners[triangle].vertex>=current.vertices.size())
            return fail("The edited mesh has an invalid corner.");
        const auto& weights=current.vertices[part.corners[triangle].vertex].skin;
        const size_t index=at/3*3+(source.mirrored?(at%3==1?2:at%3==2?1:0):at%3);
        const size_t vertex=primitive->indices?cgltf_accessor_read_index(primitive->indices,index):index;
        if(vertex>=kept.vertices.size()) return fail("The source has an invalid vertex index.");
        auto& destination=kept.vertices[vertex];
        if(destination.count && !nativeSkinWeights_Equal(&destination,&weights))
            return fail("A source vertex now has conflicting corner weights. Choose weight transfer.");
        destination=weights;++at;
    }
    // Unreferenced source vertices have no rendered weights in the DAT. They
    // remain unreferenced because the ordered topology is unchanged.
    for(auto& weights:kept.vertices) if(!weights.count) {
        weights.count=1;weights.joints[0]=(uint16_t)owner;weights.weights[0]=1;
    }
    if(source.uses_bind_shape) {
        std::memcpy(kept.bind_shape,source.transform,sizeof(kept.bind_shape));
        for(unsigned axis=0;axis<3;++axis) kept.bind_shape[12+axis]*=scale/source.scale;
    } else {
        kept.bind_shape[0]=kept.bind_shape[5]=kept.bind_shape[10]=kept.bind_shape[15]=1;
        if(node) cgltf_node_transform_world(node,kept.bind_shape);
        for(unsigned axis=0;axis<3;++axis) kept.bind_shape[12+axis]*=scale;
    }
    *output=std::move(kept);*rigid=false;return true;
}
