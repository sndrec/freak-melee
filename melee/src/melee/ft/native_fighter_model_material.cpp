#include "native_fighter_model_editor.h"
#include "native_fighter_model_references.h"
#include "native_asset_dat.h"
#include <cstdlib>
#include <cstring>
#include <memory>

bool nativeFighterModel_SetMeshMaterial(NativeFighterModelEditor* editor,
                                       unsigned mesh, unsigned material)
{
    if(!editor || !editor->project || mesh>=editor->mesh_count || material>=editor->material_count ||
       !nativeFighterModel_MaterialInputs(editor,mesh,material,&editor->dat,editor->materials[material].render_mode)) return false;
    if(editor->meshes[mesh].material==material) return true;
    auto& project=*editor->project;
    NativeFighterProject candidate=project;
    std::vector<uint8_t> fighter(project.fighter_dat,project.fighter_dat+project.fighter_size);
    std::vector<uint8_t> costume(project.costume_dat,project.costume_dat+project.costume_size);
    candidate.fighter_dat=fighter.data();candidate.costume_dat=costume.data();
    NativeAssetDat::be32(costume.data()+32+editor->meshes[mesh].offset+8,editor->materials[material].offset);
    auto after=std::make_unique<NativeFighterModelEditor>();
    if(!nativeFighterModel_Open(after.get(),&candidate)) return false;
    const unsigned selected=after->meshes[mesh].material;
    u32 flags[NATIVE_MODEL_MAX_JOINTS];
    nativeFighterModel_MaterialJointFlags(after.get(),selected,after->materials[selected].render_mode,flags);
    for(unsigned joint=0;joint<after->joint_count;++joint)
        NativeAssetDat::be32(costume.data()+32+after->joints[joint].offset+4,flags[joint]);
    NativeAssetDat dat;
    std::vector<uint8_t> converted;
    if(!dat.open(costume.data(),costume.size()) ||
       !nativeFighterModel_RemapMaterialAnimations(*editor,*after,dat) || !dat.finish(&converted)) return false;
    // Serialization still reads the source archive's footer. Publish the new
    // vector only after that read completes, keeping its source storage alive.
    costume.swap(converted);
    candidate.costume_dat=costume.data();candidate.costume_size=costume.size();
    if(!nativeFighterModel_Open(after.get(),&candidate) ||
       !nativeFighterModel_RemapSelectors(editor,after.get(),&candidate)) return false;
    char error[256];
    if(!nativeFighterProject_ValidateAssets(&candidate,error,sizeof(error))) return false;
    std::unique_ptr<uint8_t,decltype(&std::free)> saved_fighter((uint8_t*)std::malloc(fighter.size()),std::free);
    std::unique_ptr<uint8_t,decltype(&std::free)> saved_costume((uint8_t*)std::malloc(costume.size()),std::free);
    if(!saved_fighter || !saved_costume) return false;
    std::memcpy(saved_fighter.get(),fighter.data(),fighter.size());std::memcpy(saved_costume.get(),costume.data(),costume.size());
    std::free(project.fighter_dat);std::free(project.costume_dat);
    project.fighter_dat=saved_fighter.release();project.costume_dat=saved_costume.release();project.costume_size=costume.size();
    return nativeFighterModel_Open(editor,&project);
}
