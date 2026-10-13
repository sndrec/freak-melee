#include <melee/ft/native_source_declarations.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_fighter_project.h>
#include <melee/ft/native_fighter_sound_policy.h>
#include <melee/ft/native_source_motion_policy.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/types.h>
#include <aurora/dvd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static NativeSourceAssetFacts original,normalized,unchanged;
static NativeSourcePackage accepted,candidate,saved;
static NativeScriptSourceNode nodes[1024];
static uint8_t encoded[65536];
static NativeScriptDiagnostic diagnostic;
static unsigned checks,cases;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"owned declarations %u %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
static const char climber[]="fighter:\n    behavior independent\nsound_mute_channels 0x44\nsurface_friction ignore\nsound_remap 0xFFFFFFFC 13 0x1FBFD 0x1FC62 0x66\nsound_remap 0x3 13 0x1FC63 0x1FCC8 -0x66\nsound_ko_cue 0xFFFFFFFC 0x1FBD1\naction Probe:\n    on load:\n        pass\n";
static const char gamewatch[]="fighter:\n    behavior independent\nvisibility_asset 10\naction Probe:\n    on load:\n        pass\n";
static const char empty[]="fighter:\n    behavior independent\naction Probe:\n    on load:\n        pass\n";
static NativeScriptSourceTree parse(const char* source)
{NativeScriptSourceTree tree={.source=source,.source_bytes=(uint32_t)strlen(source),.nodes=nodes,.capacity=ARRAY_SIZE(nodes)};CHECK(nativeScriptSource_Parse(&tree,&diagnostic));return tree;}
static void rejected(const char* text)
{
 NativeScriptSourceTree tree=parse(text);memset(&normalized,0xAC,sizeof(normalized));unchanged=normalized;
 CHECK(!nativeSourceDeclarations_Prepare(&tree,&original,&normalized,&diagnostic));CHECK(!memcmp(&normalized,&unchanged,sizeof(normalized)));++cases;
}
static void recipes(FighterKind kind)
{
 const char* source=kind==Ft_Kind_GameWatch?gamewatch:kind==Ft_Kind_Popo||kind==Ft_Kind_Nana?climber:empty;
 NativeScriptSourceTree tree=parse(source);CHECK(nativeSourceDeclarations_Prepare(&tree,&original,&normalized,&diagnostic));
 CHECK(!strcmp(normalized.fighter_root,original.fighter_root)&&!memcmp(&normalized.moves,&original.moves,sizeof(original.moves)));
 CHECK(!memcmp(normalized.motions,original.motions,sizeof(original.motions))&&normalized.joints==original.joints);
 for(uint32_t i=0;i<tree.count;++i)if(nativeSourceDeclarations_Owns(&tree,i)){CHECK(nativeSourceDeclarations_Validate(&tree,i,&normalized,&diagnostic));CHECK(!nativeSourceDeclarations_Validate(&tree,i,NULL,&diagnostic));}
 if(kind==Ft_Kind_Popo||kind==Ft_Kind_Nana){
  CHECK(!original.ignore_surface_friction&&original.sound_rule_count==0&&original.ko_cue_count==0);
  CHECK(normalized.ignore_surface_friction&&normalized.sound_mute_channels==0x44&&normalized.sound_rule_count==2&&normalized.ko_cue_count==1);
  NativeFighterSoundPolicy sound={normalized.sound_rules,normalized.ko_cues,2,1};
  CHECK(nativeFighterSoundPolicy_MapId(&sound,2,13,0x1FBFD)==0x1FC63);
  CHECK(nativeFighterSoundPolicy_MapId(&sound,0,13,0x1FC63)==0x1FBFD);
  CHECK(nativeFighterSoundPolicy_MapId(&sound,0,12,0x1FC63)==0x1FC63);
  CHECK(nativeFighterSoundPolicy_KoCue(&sound,2,-1)==0x1FBD1&&nativeFighterSoundPolicy_KoCue(&sound,0,-1)==-1);
 }
 if(kind==Ft_Kind_GameWatch)CHECK(normalized.layout.visibility_mask==0x400);
 ++cases;
 const char* bad[]={"sound_mute_channels 128\n","sound_mute_channels 1\nsound_mute_channels 1\n","surface_friction guessed\n","surface_friction ignore\nsurface_friction stage\n","visibility_asset 16\n","visibility_asset 10\nvisibility_asset 10\n","sound_remap 1 64 0 1 0\n","sound_remap 1 0 0 1 -1\n","sound_remap 1 0 2147483647 2147483647 1\n","sound_ko_cue 1 1\nsound_ko_cue 1 2\n","sound_remap 1 0 0 1 0 garbage\n","action Probe:\n    on load:\n        pass\nsurface_friction ignore\n"};
 for(unsigned i=0;i<ARRAY_SIZE(bad);++i)rejected(bad[i]);
}
static void packages(const NativeSourceAssetInput* assets,FighterKind kind)
{
 const char* source=kind==Ft_Kind_GameWatch?gamewatch:kind==Ft_Kind_Popo||kind==Ft_Kind_Nana?climber:empty;
 size_t bytes=0;CHECK(nativeSourceAssets_Write(&original,encoded,sizeof(encoded),&bytes,&diagnostic));
 NativeSourcePackageInput input={.family=NativeSourcePackage_Single,.facts=encoded,.facts_bytes=bytes,.assets=*assets,.source=source,.source_bytes=strlen(source)};
 CHECK(nativeSourcePackage_Prepare(&accepted,&input,Ft_Kind_CustomFirst,NULL,&diagnostic));
 CHECK(accepted.ready&&accepted.program.bank==&accepted.compiler.bank&&accepted.metadata.source_facts==&accepted.assets.facts);
 CHECK(accepted.compiler.bank.declarations.function_count==1&&!memcmp(&accepted.assets.facts,&normalized,sizeof(normalized)));
 CHECK(accepted.assets.character.data.x0==&accepted.assets.character.common_attributes);
 for(unsigned i=0;i<accepted.assets.facts.moves.count;++i)CHECK(!accepted.assets.motions[i].anim_cb&&!accepted.assets.motions[i].input_cb&&!accepted.assets.motions[i].phys_cb&&!accepted.assets.motions[i].coll_cb);
 saved=accepted;++cases;
 const char* bad[]={"sound_mute_channels 128\n", "base ftDataMario 341 1\n", "motion_policy 341 common 44\n"};
 for(unsigned i=0;i<ARRAY_SIZE(bad);++i){input.source=bad[i];input.source_bytes=strlen(bad[i]);CHECK(!nativeSourcePackage_Prepare(&candidate,&input,Ft_Kind_CustomFirst,&accepted,&diagnostic));CHECK(!memcmp(&saved,&accepted,sizeof(saved)));CHECK(!candidate.ready&&!candidate.input_memory&&!candidate.compiler.arena&&!candidate.assets.character_arena);++cases;}
 if(kind==Ft_Kind_GameWatch){input.source="visibility_asset 11\n";input.source_bytes=strlen(input.source);CHECK(!nativeSourcePackage_Prepare(&candidate,&input,Ft_Kind_CustomFirst,&accepted,&diagnostic));CHECK(!memcmp(&saved,&accepted,sizeof(saved)));++cases;}
 CHECK(nativeSourcePackage_Release(&accepted)&&nativeSourcePackage_Release(&candidate));
 // Actual imported rows retain numeric assets/camera while borrowing only the
 // canonical common engine phases. Authored phases remain prepared overrides.
 const char* policy="fighter:\n    behavior independent\nmotion_policy 341 common 44\naction Override:\n    on state_341_animation:\n        pass\n";
 input.source=policy;input.source_bytes=strlen(policy);
 CHECK(nativeSourcePackage_Prepare(&accepted,&input,Ft_Kind_CustomFirst,NULL,&diagnostic));
 const MotionState* common=&ftData_MotionStateList[44];MotionState* owned=&accepted.assets.motions[0];
 CHECK(owned->anim_cb==common->anim_cb&&owned->input_cb==common->input_cb&&owned->phys_cb==common->phys_cb&&owned->coll_cb==common->coll_cb);
 CHECK(owned->anim_id==original.motions[0].animation&&owned->x4_flags==(enum_t)original.motions[0].flags&&owned->_==original.motions[0].attack);
 CHECK(accepted.metadata.source_assets==&accepted.assets);
 CHECK(accepted.program.motion_count==1&&accepted.program.motions[0].motion==341&&accepted.program.motions[0].functions[NativeScriptMotion_Animation]!=UINT32_MAX);
 uint32_t policy_node=0;for(uint32_t i=0;i<accepted.compiler.bank.tree.count;++i)if(nativeSourceMotionPolicy_Owns(&accepted.compiler.bank.tree,i))policy_node=i;
 CHECK(policy_node&&nativeScriptItemMetadata_Validate(&accepted.metadata,&accepted.compiler.bank.tree,policy_node,&diagnostic));
 HSD_GObjEvent saved_input=owned->input_cb;owned->input_cb=NULL;
 CHECK(saved_input&&!nativeScriptItemMetadata_Validate(&accepted.metadata,&accepted.compiler.bank.tree,policy_node,&diagnostic));owned->input_cb=saved_input;
 MotionState unchanged_motion=*owned;
 const char* invalid[]={"motion_policy 341 common 44\nmotion_policy 341 common 48\n","motion_policy 0 common 44\n","motion_policy 341 common 341\n","motion_policy 341 common 4294967296\n","motion_policy -1 common 44\n","motion_policy 341 common 44 trailing\n"};
 for(unsigned i=0;i<ARRAY_SIZE(invalid);++i){NativeScriptSourceTree bad_tree=parse(invalid[i]);CHECK(!nativeSourceMotionPolicy_Prepare(&bad_tree,ftData_MotionStateList,&accepted.assets,&diagnostic));CHECK(!memcmp(owned,&unchanged_motion,sizeof(*owned)));++cases;}
 CHECK(nativeSourcePackage_Release(&accepted));++cases;
}
int main(int argc,char** argv)
{
 CHECK(argc==3&&aurora_dvd_open(argv[1]));const FighterKind kinds[]={Ft_Kind_Peach,Ft_Kind_GameWatch,Ft_Kind_Popo,Ft_Kind_Nana};
 for(unsigned i=0;i<ARRAY_SIZE(kinds);++i){const NativeFighterDefinition* donor=nativeFighterRuntime_FindStockDefinition(kinds[i]);CHECK(donor&&nativeSourceAssets_Copy(donor,&original,&diagnostic));recipes(kinds[i]);
  // Reject helpers overwrite scratch; restore the accepted normalized recipe.
  NativeScriptSourceTree tree=parse(kinds[i]==Ft_Kind_GameWatch?gamewatch:kinds[i]==Ft_Kind_Popo||kinds[i]==Ft_Kind_Nana?climber:empty);CHECK(nativeSourceDeclarations_Prepare(&tree,&original,&normalized,&diagnostic));
  char directory[1024];CHECK(snprintf(directory,sizeof(directory),"%s/member%u",argv[2],i)>0);CHECK(nativeFighterProject_CreateStockFromDisc(directory,"Owned declaration fixture",kinds[i]));NativeFighterProject project;CHECK(nativeFighterProject_Open(directory,&project));
  NativeSourceAssetInput assets={project.fighter_dat,project.fighter_size,project.animation_dat,project.animation_size,project.costume_dat,project.costume_size};packages(&assets,kinds[i]);nativeFighterProject_Close(&project);
 }
 printf("{\"status\":\"PASS owned declarations\",\"checks\":%u,\"cases\":%u,\"actual_dat_imports\":4,\"game_acceptance\":false}\n",checks,cases);return 0;
}
