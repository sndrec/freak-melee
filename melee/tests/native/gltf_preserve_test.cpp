#undef NDEBUG
#include <melee/ft/native_fighter_gltf_import.h>
#include <melee/ft/native_fighter_authoring.h>
#include <melee/ft/native_fighter_core_asset_state.h>
#include <melee/ft/native_fighter_skin_mesh.h>
#include <melee/ft/native_asset_dat.h>
#include <sysdolphin/baselib/jobj.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace {
unsigned checks;
char error[256];
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"PRESERVE FAIL %d: %s [%s]\n",__LINE__,#x,error);std::exit(1);}}while(0)

void publish(NativeFighterProject& project,NativeAssetDat& dat)
{
    std::vector<uint8_t> bytes;CHECK(dat.finish(&bytes,UINT32_MAX));
    auto* memory=(uint8_t*)std::malloc(bytes.size());CHECK(memory);
    std::memcpy(memory,bytes.data(),bytes.size());std::free(project.costume_dat);
    project.costume_dat=memory;project.costume_size=bytes.size();
}
unsigned ordinal(const NativeFighterModelEditor& model,uint32_t offset)
{
    for(unsigned i=0;i<model.mesh_count;++i) if(model.meshes[i].offset==offset)return i;
    CHECK(false);return 0;
}
std::array<NativeSkinVertex,4> vertices(NativeFighterProject& project,uint32_t offset)
{
    auto model=std::make_unique<NativeFighterModelEditor>();CHECK(nativeFighterModel_Open(model.get(),&project));
    NativeSkinMesh skin;CHECK(nativeSkinMesh_Read(*model,ordinal(*model,offset),&skin,error,sizeof(error)));
    const auto* source=nativeFighterMeshSource_Get(&project,offset);CHECK(source);
    const unsigned normal[6]={0,1,2,0,2,3},mirrored[6]={0,2,1,0,3,2};
    std::array<NativeSkinVertex,4> result={};unsigned at=0;
    for(const auto& part:skin.parts) for(unsigned triangle:part.triangles) {
        CHECK(at<6);
        const unsigned index=source->mirrored?mirrored[at]:normal[at];
        const auto& vertex=skin.vertices[part.corners[triangle].vertex];
        if(result[index].skin.count) CHECK(nativeSkinWeights_Equal(&result[index].skin,&vertex.skin));
        result[index]=vertex;++at;
    }
    CHECK(at==6);return result;
}

void run_case(const char* draft,const char* original,const char* updated,const char* topology,
              bool output_mirrored)
{
    NativeFighterProject project={};CHECK(nativeFighterProject_Open(draft,&project));
    NativeGltfImportRequest add[2]={{0,0,UINT_MAX,NATIVE_GLTF_BIND_AUTOMATIC,1,false},
                                  {1,0,UINT_MAX,NATIVE_GLTF_BIND_AUTOMATIC,2,false}};
    CHECK(nativeFighterGltf_ApplyBatch(&project,original,add,2,error,sizeof(error)));
    auto model=std::make_unique<NativeFighterModelEditor>();CHECK(nativeFighterModel_Open(model.get(),&project));
    uint32_t hull=0,trim=0;
    for(unsigned i=0;i<model->mesh_count;++i) {
        const auto* source=nativeFighterMeshSource_Get(&project,(uint32_t)model->meshes[i].offset);
        if(!source)continue;
        if(!std::strcmp(source->identity,"mesh:4:Hull"))hull=source->mesh;
        if(!std::strcmp(source->identity,"mesh:4:Trim"))trim=source->mesh;
    }
    CHECK(hull&&trim);
    unsigned mesh=ordinal(*model,hull);
    // Author the maximum influence count without normalizing untouched data.
    NativeSkinMesh skin;CHECK(nativeSkinMesh_Read(*model,mesh,&skin,error,sizeof(error)));
    NativeSkinWeights weights={};
    for(unsigned joint=0;joint<model->joint_count && weights.count<16;++joint) {
        float inverse[3][4];
        if((model->joints[joint].flags&JOBJ_SKELETON)&&nativeSkinMesh_InverseBind(*model,joint,inverse)) {
            const unsigned i=weights.count++;weights.joints[i]=(uint16_t)joint;weights.weights[i]=(float)(i+1)/136;
        }
    }
    CHECK(weights.count==16);CHECK(nativeSkinWeights_Normalize(&weights));
    skin.vertices[skin.parts[0].corners[0].vertex].skin=weights;
    NativeAssetDat dat;CHECK(dat.open(project.costume_dat,project.costume_size));
    uint32_t polygon;CHECK(nativeSkinMesh_Encode(dat,*model,skin,&polygon));
    dat.link(model->meshes[mesh].offset+12,polygon);publish(project,dat);
    CHECK(nativeFighterModel_Open(model.get(),&project));mesh=ordinal(*model,hull);
    const unsigned material=model->meshes[mesh].material;
    const uint32_t material_offset=(uint32_t)model->materials[material].offset;
    CHECK(dat.open(project.costume_dat,project.costume_size));
    dat.link(trim+8,material_offset);publish(project,dat);
    CHECK(nativeFighterModel_Open(model.get(),&project));
    const unsigned shared_material=model->meshes[ordinal(*model,hull)].material;
    CHECK(nativeFighterModel_SetMaterialColor(model.get(),shared_material,1,0,181));
    CHECK(nativeFighterModel_SetMaterialFloat(model.get(),shared_material,true,19.25f));
    CHECK(nativeFighterModel_Open(model.get(),&project));
    const NativeModelMaterial edited=model->materials[model->meshes[ordinal(*model,hull)].material];
    const auto before=vertices(project,hull),before_trim=vertices(project,trim);
    NativeGltfReimportPlan plan;CHECK(nativeFighterGltf_PlanReimport(&project,ordinal(*model,hull),updated,true,&plan,error,sizeof(error)));
    CHECK(plan.count==2);
    NativeGltfImportRequest requests[2];
    for(unsigned i=0;i<2;++i)requests[i]={plan.matches[i].primitive,plan.matches[i].joint,
        plan.matches[i].mesh,NATIVE_GLTF_BIND_KEEP,plan.matches[i].scale,false};
    CHECK(nativeFighterGltf_ApplyBatch(&project,updated,requests,2,error,sizeof(error)));
    const auto after=vertices(project,hull),after_trim=vertices(project,trim);
    for(unsigned vertex=0;vertex<4;++vertex) {
        CHECK(nativeSkinWeights_Equal(&before[vertex].skin,&after[vertex].skin));
        CHECK(nativeSkinWeights_Equal(&before_trim[vertex].skin,&after_trim[vertex].skin));
        // Updated fixtures add (1.25,2,0) before their node transform.
        const float original_x[4]={-12,-9.6f,-8,-11};
        CHECK(std::fabs(after[vertex].position[0]-(original_x[vertex]+1.25f)*(output_mirrored?-1:1))<3e-6f);
        CHECK(std::fabs(after[vertex].position[1]-before[vertex].position[1]-2)<3e-6f);
        CHECK(std::fabs(after_trim[vertex].position[1]-before_trim[vertex].position[1]-4)<5e-6f);
    }
    CHECK(after[0].skin.count==16);
    CHECK(nativeFighterModel_Open(model.get(),&project));
    const auto& retained=model->materials[model->meshes[ordinal(*model,hull)].material];
    CHECK(retained.offset==material_offset && retained.shininess==edited.shininess);
    CHECK(!std::memcmp(retained.diffuse,edited.diffuse,sizeof(edited.diffuse)));
    CHECK(model->materials[model->meshes[ordinal(*model,trim)].material].offset==material_offset);
    NativeFighterCoreAssetState snapshot;CHECK(nativeFighterCoreAssets_Capture(project,snapshot));
    const auto* names=project.authoring;
    auto saved=std::make_unique<NativeFighterAuthoring>(*names);
    NativeGltfImportRequest changed={0,0,ordinal(*model,hull),NATIVE_GLTF_BIND_KEEP,1,false};
    CHECK(!nativeFighterGltf_ApplyBatch(&project,topology,&changed,1,error,sizeof(error)));
    CHECK(nativeFighterCoreAssets_Matches(project,snapshot)&&project.authoring==names);
    CHECK(!std::memcmp(names,saved.get(),sizeof(*names)));
    // Explicit source material replacement keeps current weights and leaves a
    // second mesh using the old shared material unchanged.
    changed.replace_material=true;
    CHECK(nativeFighterGltf_ApplyBatch(&project,updated,&changed,1,error,sizeof(error)));
    CHECK(nativeFighterModel_Open(model.get(),&project));
    const auto& replacement=model->materials[model->meshes[ordinal(*model,hull)].material];
    CHECK(replacement.offset!=material_offset);
    CHECK(replacement.diffuse[0]==51&&replacement.diffuse[1]==102&&replacement.diffuse[2]==153);
    const auto& shared=model->materials[model->meshes[ordinal(*model,trim)].material];
    CHECK(shared.offset==material_offset&&shared.shininess==19.25f&&shared.diffuse[0]==181);
    const auto material_changed=vertices(project,hull);
    for(unsigned vertex=0;vertex<4;++vertex) CHECK(nativeSkinWeights_Equal(&after[vertex].skin,&material_changed[vertex].skin));
    // Changed topology requires an explicit replacement/transfer choice.
    changed.binding=NATIVE_GLTF_BIND_TRANSFER;changed.replace_material=false;
    CHECK(nativeFighterGltf_ApplyBatch(&project,topology,&changed,1,error,sizeof(error)));
    CHECK(nativeFighterProject_ValidateAssets(&project,error,sizeof(error)));
    nativeFighterProject_Close(&project);
}
}

unsigned nativeGltfPreserveTests(const char* draft,const char* original,const char* updated,
    const char* topology,const char* mirrored,const char* mirrored_updated)
{
    run_case(draft,original,updated,topology,false);
    run_case(draft,mirrored,mirrored_updated,topology,true);
    run_case(draft,original,mirrored_updated,topology,true);
    return checks;
}
