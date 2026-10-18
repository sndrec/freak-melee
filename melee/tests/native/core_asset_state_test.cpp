#undef NDEBUG
#include <melee/ft/native_fighter_core_asset_state.h>
#include <melee/ft/native_fighter_rig_edit.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static unsigned checks;
static char error[256];
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s [%s]\n",__LINE__,#x,error);std::exit(1);}}while(0)
static void unchanged(const NativeFighterProject& p,const NativeFighterCoreAssetState& state,
                      const uint8_t* const pointers[4],uint64_t serial)
{
    CHECK(nativeFighterCoreAssets_Matches(p,state)&&p.rig_edit_serial==serial);
    CHECK(p.fighter_dat==pointers[0]&&p.animation_dat==pointers[1]&&p.costume_dat==pointers[2]&&p.asset_record==pointers[3]);
}
int main(int argc,char** argv)
{
    CHECK(argc==2);NativeFighterProject project={};CHECK(nativeFighterProject_Open(argv[1],&project));
    const auto* script=project.script_text;const auto* parameters=project.parameter_text;
    const auto* resources=project.resources;const auto* labels=project.authoring;
    NativeFighterCoreAssetState before,after;
    CHECK(nativeFighterCoreAssets_Capture(project,before));
    NativeFighterRigEdit edit={NativeRigEdit_Add,0,0,{}};std::snprintf(edit.name,sizeof(edit.name),"HistoryExtra");
    int32_t added=-1;CHECK(nativeFighterRig_Edit(&project,&edit,&added,error,sizeof(error)));
    CHECK(project.rig_edit_revision==1&&project.rig_edit_serial==1);
    CHECK(nativeFighterCoreAssets_Capture(project,after));
    CHECK(!nativeFighterCoreAssets_Matches(project,before));
    CHECK(nativeFighterCoreAssets_Restore(project,after,before,error,sizeof(error)));
    CHECK(nativeFighterCoreAssets_Matches(project,before)&&project.rig_edit_serial==1);
    CHECK(nativeFighterCoreAssets_Restore(project,before,after,error,sizeof(error)));
    CHECK(nativeFighterCoreAssets_Matches(project,after)&&project.rig_edit_serial==1);
    // Each independently owned buffer participates in the expected-state guard.
    uint8_t* buffers[]={project.fighter_dat,project.animation_dat,project.costume_dat,project.asset_record};
    const uint8_t* pointers[]={project.fighter_dat,project.animation_dat,project.costume_dat,project.asset_record};
    for(unsigned n=0;n<4;++n){
        buffers[n][0]^=1;NativeFighterCoreAssetState modified;CHECK(nativeFighterCoreAssets_Capture(project,modified));
        CHECK(!nativeFighterCoreAssets_Restore(project,after,before,error,sizeof(error))&&error[0]);
        unchanged(project,modified,pointers,1);buffers[n][0]^=1;
    }
    // Facts and history identities also belong to the guarded state.
    project.facts.effect_bank^=1;NativeFighterCoreAssetState modified;
    CHECK(nativeFighterCoreAssets_Capture(project,modified));
    CHECK(!nativeFighterCoreAssets_Restore(project,after,before,error,sizeof(error)));
    unchanged(project,modified,pointers,1);project.facts.effect_bank^=1;
    // Admit all replacements privately, including rejection of a later asset.
    NativeFighterCoreAssetState invalid=before;invalid.bytes[3][0]^=1;
    CHECK(!nativeFighterCoreAssets_Restore(project,after,invalid,error,sizeof(error)));
    unchanged(project,after,pointers,1);
    invalid=before;invalid.bytes[2].resize(20);
    CHECK(!nativeFighterCoreAssets_Restore(project,after,invalid,error,sizeof(error)));
    unchanged(project,after,pointers,1);
    CHECK(nativeFighterCoreAssets_Restore(project,after,before,error,sizeof(error)));
    CHECK(nativeFighterRig_Edit(&project,&edit,&added,error,sizeof(error)));
    NativeFighterCoreAssetState branch;CHECK(nativeFighterCoreAssets_Capture(project,branch));
    CHECK(project.rig_edit_revision==2&&project.rig_edit_serial==2&&branch.bytes==after.bytes);
    CHECK(!nativeFighterCoreAssets_Matches(project,after));
    CHECK(!nativeFighterCoreAssets_Restore(project,after,before,error,sizeof(error)));
    CHECK(nativeFighterCoreAssets_Matches(project,branch));
    CHECK(nativeFighterCoreAssets_Restore(project,branch,before,error,sizeof(error)));
    CHECK(project.rig_edit_revision==0&&project.rig_edit_serial==2);
    CHECK(project.script_text==script&&project.parameter_text==parameters&&project.resources==resources&&project.authoring==labels);
    CHECK(nativeFighterProject_ValidateAssets(&project,error,sizeof(error)));
    std::printf("PASS %u complete asset history checks\n",checks);nativeFighterProject_Close(&project);
}
