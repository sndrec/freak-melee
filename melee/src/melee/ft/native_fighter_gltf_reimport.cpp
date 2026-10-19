#include "native_fighter_gltf_import.h"
#include "native_fighter_mesh_source.h"
#include "native_fighter_model_editor.h"
#include <cstdio>
#include <cstring>
#include <memory>
#include <new>

bool nativeFighterGltf_PlanReimport(const NativeFighterProject* project,unsigned selected,
    const char* path,bool whole_source,NativeGltfReimportPlan* output,char* error,size_t capacity)
{
    auto fail=[&](const char* message) {
        if(error&&capacity)std::snprintf(error,capacity,"%s",message);return false;
    };
    if(!project||!path||!output) return fail("Choose an imported mesh and a source file.");
    std::unique_ptr<NativeFighterModelEditor> model(new(std::nothrow) NativeFighterModelEditor{});
    if(!model||!nativeFighterModel_Open(model.get(),const_cast<NativeFighterProject*>(project))||selected>=model->mesh_count)
        return fail("Choose an imported mesh.");
    const auto* anchor=nativeFighterMeshSource_Get(project,(uint32_t)model->meshes[selected].offset);
    if(!anchor) return fail("This mesh has no unique source identity. Import a uniquely named source mesh first.");
    NativeGltfScan scan;
    if(!nativeFighterGltf_Scan(path,&scan,error,capacity)) return false;
    NativeGltfReimportPlan candidate={};
    for(unsigned mesh=0;mesh<model->mesh_count;++mesh) {
        const auto* source=nativeFighterMeshSource_Get(project,(uint32_t)model->meshes[mesh].offset);
        if(!source || (whole_source ? std::strcmp(source->path,anchor->path)!=0 : mesh!=selected)) continue;
        unsigned found=UINT_MAX;
        for(unsigned primitive=0;primitive<scan.count;++primitive)
            if(scan.primitives[primitive].identity[0] &&
               !std::strcmp(source->identity,scan.primitives[primitive].identity)) {found=primitive;break;}
        if(found==UINT_MAX) {
            if(error&&capacity)std::snprintf(error,capacity,
                "Source mesh '%s' is missing or ambiguous. Keep mesh and material-slot names unique.",model->meshes[mesh].name);
            return false;
        }
        if(candidate.count==NATIVE_GLTF_MAX_PRIMITIVES) return fail("This source has too many imported mesh instances for one reimport.");
        candidate.matches[candidate.count++]={found,mesh,model->meshes[mesh].joint,source->scale};
    }
    if(!candidate.count) return fail("No imported meshes match this source.");
    *output=candidate;if(error&&capacity)error[0]=0;return true;
}
