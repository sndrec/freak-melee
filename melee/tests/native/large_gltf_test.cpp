#include <melee/ft/native_fighter_gltf_import.h>
#include <melee/ft/native_fighter_authoring.h>
#include <melee/ft/native_fighter_core_asset_state.h>
#include <melee/ft/native_fighter_skin_mesh.h>
#include <melee/ft/native_mesh_vertex_ids.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>

namespace {
unsigned checks;
char error[256];
constexpr unsigned vertex_count=70002,corner_count=70005;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s [%s]\n",__LINE__,#x,error);std::exit(1);}}while(0)

unsigned ordinal(const NativeFighterModelEditor& model,uint32_t offset)
{
    for(unsigned i=0;i<model.mesh_count;++i)if(model.meshes[i].offset==offset)return i;
    CHECK(false);return 0;
}
NativeSkinMesh read(NativeFighterProject& project,uint32_t offset,bool deformed)
{
    auto model=std::make_unique<NativeFighterModelEditor>();CHECK(nativeFighterModel_Open(model.get(),&project));
    const unsigned mesh=ordinal(*model,offset);
    NativeSkinMesh skin;CHECK(nativeSkinMesh_Read(*model,mesh,&skin,error,sizeof(error)));
    CHECK(skin.vertices.size()==vertex_count&&skin.corner_vertices.size()==corner_count);
    for(unsigned i=0;i<vertex_count;++i) {
        const unsigned from=i==vertex_count-1?i-1:i;
        const unsigned triangle=from/3;
        const float expected[3]={(triangle%157)*.02f+(from%3==1?.015f:0),
            (triangle/157)*.02f+(from%3==2?.015f:0),(triangle%7)*.001f+(deformed?.25f:0)};
        for(unsigned axis=0;axis<3;++axis)CHECK(std::fabs(skin.vertices[i].position[axis]-expected[axis])<.000002f);
    }
    for(const auto& part:skin.parts) for(const auto& corner:part.corners) {
        CHECK(corner.normal[0]==0&&corner.normal[1]==0&&corner.normal[2]==1);
        u32 cursor=corner.source;
        for(const auto& attr:part.attributes) {
            if(attr.attr==13) {
                const unsigned index=attr.type==2?model->dat.data[cursor]:
                    (unsigned(model->dat.data[cursor])<<8)|model->dat.data[cursor+1];
                float u,v;CHECK(HSD_DatReadF32(&model->dat,attr.array+index*attr.stride,&u));
                CHECK(HSD_DatReadF32(&model->dat,attr.array+index*attr.stride+4,&v));
                CHECK(std::fabs(u-float(corner.vertex%17)/16)<.0000001f);
                CHECK(std::fabs(v-float(corner.vertex%23)/22)<.0000001f);
            }
            cursor+=attr.display_bytes;
        }
    }
    CHECK(skin.corner_vertices[vertex_count]==0&&skin.corner_vertices[vertex_count+1]==65536&&
        skin.corner_vertices[vertex_count+2]==vertex_count-1);
    return skin;
}
void run(const std::filesystem::path& base,const std::filesystem::path& fixtures,
         const char* name,NativeGltfBindingMode binding)
{
    const auto folder=fixtures/(std::string("case-")+name);
    std::filesystem::copy(base,folder,std::filesystem::copy_options::recursive);
    NativeFighterProject project={};CHECK(nativeFighterProject_Open(folder.string().c_str(),&project));
    const auto source=fixtures/(std::string(name)+".glb");
    NativeGltfImportRequest request={0,0,UINT_MAX,binding,1,false};
    CHECK(nativeFighterGltf_ApplyBatch(&project,source.string().c_str(),&request,1,error,sizeof(error)));
    auto model=std::make_unique<NativeFighterModelEditor>();CHECK(nativeFighterModel_Open(model.get(),&project));
    uint32_t offset=0;
    for(unsigned i=0;i<model->mesh_count;++i) {
        const auto* link=nativeFighterMeshSource_Get(&project,(uint32_t)model->meshes[i].offset);
        if(link&&link->vertices==vertex_count)offset=link->mesh;
    }
    CHECK(offset);
    auto skin=read(project,offset,false);
    const unsigned sections=model->meshes[ordinal(*model,offset)].polygon_count;
    CHECK(sections>=2);
    if(!std::strcmp(name,"gradient"))CHECK(sections>4096);
    // Paint one vertex used in both distant draw sections.
    NativeSkinBrushPoint center;std::memcpy(center.position,skin.vertices[0].position,sizeof(center.position));
    unsigned affected=0;
    CHECK(nativeFighterSkin_Paint(&project,ordinal(*model,offset),3,&center,1,.001f,.1f,false,&affected,error,sizeof(error)));
    CHECK(affected==1);skin=read(project,offset,false);
    const auto painted=skin.vertices;
    CHECK(nativeFighterProject_SaveAssets(folder.string().c_str(),&project,error,sizeof(error)));
    nativeFighterProject_Close(&project);CHECK(nativeFighterProject_Open(folder.string().c_str(),&project));
    const auto* link=nativeFighterMeshSource_Get(&project,offset);
    CHECK(link&&link->vertices==vertex_count&&link->corners==corner_count);
    skin=read(project,offset,false);
    for(unsigned i=0;i<vertex_count;++i)CHECK(nativeSkinWeights_Equal(&skin.vertices[i].skin,&painted[i].skin));
    CHECK(nativeFighterModel_Open(model.get(),&project));
    request.replacement_mesh=ordinal(*model,offset);request.binding=NATIVE_GLTF_BIND_KEEP;
    CHECK(nativeFighterGltf_ApplyBatch(&project,(fixtures/"deformed.glb").string().c_str(),&request,1,error,sizeof(error)));
    skin=read(project,offset,true);
    for(unsigned i=0;i<vertex_count;++i)CHECK(nativeSkinWeights_Equal(&skin.vertices[i].skin,&painted[i].skin));
    NativeFighterCoreAssetState before;CHECK(nativeFighterCoreAssets_Capture(project,before));
    auto names=std::make_unique<NativeFighterAuthoring>(*project.authoring);
    CHECK(!nativeFighterGltf_ApplyBatch(&project,(fixtures/"bad.glb").string().c_str(),&request,1,error,sizeof(error)));
    CHECK(nativeFighterCoreAssets_Matches(project,before)&&!std::memcmp(names.get(),project.authoring,sizeof(*names)));
    if(binding==NATIVE_GLTF_BIND_RIGID) {
        request.binding=NATIVE_GLTF_BIND_TRANSFER;
        CHECK(nativeFighterGltf_ApplyBatch(&project,(fixtures/"deformed.glb").string().c_str(),&request,1,error,sizeof(error)));
        skin=read(project,offset,true);
        for(unsigned i=0;i<vertex_count;++i)CHECK(nativeSkinWeights_Equal(&skin.vertices[i].skin,&painted[i].skin));
    }
    CHECK(nativeFighterProject_SaveAssets(folder.string().c_str(),&project,error,sizeof(error)));
    nativeFighterProject_Close(&project);CHECK(nativeFighterProject_Open(folder.string().c_str(),&project));
    skin=read(project,offset,true);
    CHECK(nativeFighterProject_ValidateAssets(&project,error,sizeof(error)));
    std::printf("case=%s vertices=%zu corners=%zu sections=%u bytes=%zu\n",name,skin.vertices.size(),skin.corner_vertices.size(),sections,project.costume_size);
    nativeFighterProject_Close(&project);
}
}
int main(int argc,char** argv)
{
    CHECK(argc==3);
    const auto base=std::filesystem::absolute(argv[1]),fixtures=std::filesystem::absolute(argv[2]);
    run(base,fixtures,"rigid",NATIVE_GLTF_BIND_RIGID);
    run(base,fixtures,"auto",NATIVE_GLTF_BIND_AUTOMATIC);
    run(base,fixtures,"source",NATIVE_GLTF_BIND_SOURCE);
    run(base,fixtures,"gradient",NATIVE_GLTF_BIND_SOURCE);
    std::printf("PASS: %u large import, painting, save and reimport checks\n",checks);
}
