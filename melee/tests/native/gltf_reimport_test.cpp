#undef NDEBUG
#include <melee/ft/native_fighter_gltf_import.h>
#include <melee/ft/native_fighter_authoring.h>
#include <melee/ft/native_fighter_core_asset_state.h>
#include <melee/ft/native_fighter_model_editor.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

static unsigned checks;
static char error[256];
unsigned nativeGltfPreserveTests(const char*,const char*,const char*,const char*,const char*,const char*);
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s [%s]\n",__LINE__,#x,error);std::exit(1);}}while(0)

int main(int argc,char** argv)
{
    CHECK(argc==8);
    checks+=nativeGltfPreserveTests(argv[1],argv[2],argv[4],argv[5],argv[6],argv[7]);
    NativeFighterProject project={};CHECK(nativeFighterProject_Open(argv[1],&project));
    auto model=std::make_unique<NativeFighterModelEditor>();
    CHECK(nativeFighterModel_Open(model.get(),&project));
    const unsigned original=model->mesh_count;
    // Imports on different owners, then replaces them in the order which moves
    // an earlier destination ordinal. Targets must follow DObj identity.
    NativeGltfImportRequest add[2]={{0,5,UINT_MAX,NATIVE_GLTF_BIND_RIGID,1,false},
                                   {1,10,UINT_MAX,NATIVE_GLTF_BIND_RIGID,1,false}};
    CHECK(nativeFighterGltf_ApplyBatch(&project,argv[2],add,2,error,sizeof(error)));
    CHECK(nativeFighterModel_Open(model.get(),&project));CHECK(model->mesh_count==original+2);
    uint32_t offsets[2]={};unsigned ordinals[2]={};
    for(unsigned mesh=0;mesh<model->mesh_count;++mesh) {
        const auto* source=nativeFighterMeshSource_Get(&project,(uint32_t)model->meshes[mesh].offset);
        if(!source) continue;
        if(!std::strcmp(source->identity,"mesh:4:Hull")) {offsets[0]=source->mesh;ordinals[0]=mesh;}
        if(!std::strcmp(source->identity,"mesh:4:Trim")) {offsets[1]=source->mesh;ordinals[1]=mesh;}
    }
    CHECK(offsets[0]&&offsets[1]&&offsets[0]!=offsets[1]);
    NativeGltfImportRequest replace[2]={{1,10,ordinals[1],NATIVE_GLTF_BIND_AUTOMATIC,1,false},
                                       {0,5,ordinals[0],NATIVE_GLTF_BIND_AUTOMATIC,1,false}};
    CHECK(nativeFighterGltf_ApplyBatch(&project,argv[2],replace,2,error,sizeof(error)));
    CHECK(nativeFighterModel_Open(model.get(),&project));CHECK(model->mesh_count==original+2);
    for(unsigned n=0;n<2;++n) {
        const auto* source=nativeFighterMeshSource_Get(&project,offsets[n]);CHECK(source);
        CHECK(!std::strcmp(source->identity,n?"mesh:4:Trim":"mesh:4:Hull"));
        bool found=false;
        for(unsigned mesh=0;mesh<model->mesh_count;++mesh) if(model->meshes[mesh].offset==offsets[n]) {
            found=true;CHECK(model->meshes[mesh].joint==0);
        }
        CHECK(found);
    }
    NativeFighterCoreAssetState before;CHECK(nativeFighterCoreAssets_Capture(project,before));
    const auto* fighter=project.fighter_dat;const auto* costume=project.costume_dat;
    const auto* names=project.authoring;
    auto saved=std::make_unique<NativeFighterAuthoring>(*names);
    // The first mesh converts successfully; the later one lacks normals.
    CHECK(!nativeFighterGltf_ApplyBatch(&project,argv[3],add,2,error,sizeof(error)));
    CHECK(nativeFighterCoreAssets_Matches(project,before));
    CHECK(project.fighter_dat==fighter&&project.costume_dat==costume&&project.authoring==names);
    CHECK(!std::memcmp(project.authoring,saved.get(),sizeof(*names)));
    // Duplicate replacement destinations fail before doing any conversion.
    replace[0].replacement_mesh=replace[1].replacement_mesh=0;
    CHECK(!nativeFighterGltf_ApplyBatch(&project,argv[2],replace,2,error,sizeof(error)));
    CHECK(nativeFighterCoreAssets_Matches(project,before));
    CHECK(!std::memcmp(project.authoring,saved.get(),sizeof(*names)));

    // Exercise serialization with escaped percent text, long keys and a
    // subnormal matrix entry, preserving field values and exact matrix bits.
    NativeFighterMeshSource source=*nativeFighterMeshSource_Get(&project,offsets[0]);
    std::snprintf(source.path,sizeof(source.path),"C:/source 100%%/mesh.glb");
    std::snprintf(source.identity,sizeof(source.identity),"mesh:200:");
    std::memset(source.identity+9,'Q',200);source.identity[209]=0;
    source.transform[1]=1.40129846e-45f;
    CHECK(nativeFighterMeshSource_Set(&project,&source));
    CHECK(nativeFighterProject_SaveAssets(argv[1],&project,error,sizeof(error)));
    NativeFighterProject loaded={};CHECK(nativeFighterAuthoring_Load(argv[1],&loaded));
    const auto* roundtrip=nativeFighterMeshSource_Get(&loaded,offsets[0]);CHECK(roundtrip);
    CHECK(roundtrip->mesh==source.mesh && roundtrip->vertices==source.vertices && roundtrip->corners==source.corners);
    CHECK(roundtrip->topology==source.topology && roundtrip->binding==source.binding && roundtrip->scale==source.scale);
    CHECK(roundtrip->used==source.used && roundtrip->mirrored==source.mirrored && roundtrip->uses_bind_shape==source.uses_bind_shape);
    CHECK(!std::strcmp(roundtrip->path,source.path) && !std::strcmp(roundtrip->identity,source.identity));
    CHECK(!std::memcmp(roundtrip->transform,source.transform,sizeof(source.transform)));
    // A bad record never changes the destination metadata owner or contents.
    auto prior=std::make_unique<NativeFighterAuthoring>(*loaded.authoring);
    const auto* owner=loaded.authoring;
    for(const char* row:{"0\t4\t6\t0\t0\t0\t0\t1\t0\tpath\tkey",
                        "1\t4\t6\tzz\t0\t0\t0\t1\t0\tpath\tkey",
                        "1\t4\t6\t0\t0\t0\t0\t1\t0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,nan\tpath\tkey",
                        "1\t4\t6\t0\t0\t0\t0\t1\t0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1\tpath%00\tkey"}) {
        std::vector<char> mutable_row(row,row+std::strlen(row)+1);
        CHECK(!nativeFighterMeshSource_Parse(&loaded,mutable_row.data()));
        CHECK(loaded.authoring==owner&&!std::memcmp(owner,prior.get(),sizeof(*owner)));
    }
    std::free(loaded.authoring);nativeFighterProject_Close(&project);
    std::printf("PASS %u source metadata and import transaction checks\n",checks);
}
