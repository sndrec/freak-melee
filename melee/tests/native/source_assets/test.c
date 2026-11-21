#include <melee/ft/native_source_assets.h>
#include <melee/ft/native_source_rig_validation.h>
#include <melee/ft/native_fighter_project.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <aurora/dvd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks,cases;
static NativeScriptDiagnostic diagnostic;
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"source assets %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
static NativeSourceAssetFacts facts,decoded,poison,bad;
static NativeSourceImportedAssets accepted,candidate;
static NativeSourceImportedAssets original;
static uint8_t encoded[65536],again[65536],unchanged[65536];
static NativeSourcePackage package,next_package;
static NativeScriptFighterProgram no_program;
static void package_cases(const NativeSourceAssetInput* assets,const char* source_path)
{
    FILE* file=fopen(source_path,"rb");CHECK(file&&fseek(file,0,SEEK_END)==0);long count=ftell(file);
    CHECK(count>0&&fseek(file,0,SEEK_SET)==0);char* source=malloc((size_t)count);CHECK(source);
    CHECK(fread(source,1,(size_t)count,file)==(size_t)count&&fclose(file)==0);
    size_t size=0;CHECK(nativeSourceAssets_Write(&facts,encoded,sizeof(encoded),&size,&diagnostic));
    const char overrides[]="float cpu_fireball_range = 33\nfloat It_Kind_Mario_Fire.minimum_bounce_speed = 1.75\n";
    NativeSourcePackageInput input={.family=NativeSourcePackage_Single,.facts=encoded,.facts_bytes=size,
        .assets=*assets,.source=source,.source_bytes=(size_t)count,.parameters=overrides,.parameter_bytes=sizeof(overrides)-1};
    CHECK(nativeSourcePackage_Prepare(&package,&input,Ft_Kind_CustomFirst,NULL,&diagnostic));
    CHECK(package.ready&&package.compiler.bank.declarations.function_count==77&&package.program.items.count==2&&package.program.effects.count==1);
    CHECK(package.parameters.root_imports==31&&package.parameters.article_imports==4&&package.parameters.override_count==2);
    CHECK(package.owned_input.assets.fighter!=assets->fighter&&package.owned_input.source!=source);
    bool root_seen=false,item_seen=false;
    for(unsigned i=0;i<package.compiler.bank.declarations.value_count;++i){
        const NativeScriptValueDeclaration* field=&package.compiler.bank.declarations.values[i];
        const NativeScriptParameterSlot* slot=&package.compiler.bank.storage_parameters.slots[i];
        if(field->parameter&&nativeScriptSource_SpanEquals(&package.compiler.bank.tree,field->name,"cpu_fireball_range")){
            CHECK(slot->resolved&&slot->value.as.f32==33);root_seen=true;}
        if(field->parameter&&nativeScriptSource_SpanEquals(&package.compiler.bank.tree,field->name,"minimum_bounce_speed")){
            CHECK(slot->resolved&&slot->value.as.f32==1.75f);item_seen=true;}
    }
    CHECK(root_seen&&item_seen&&nativeSourcePackage_CanActivate(&package));++cases;
    NativeSourcePackage* saved=malloc(sizeof(*saved));CHECK(saved);memcpy(saved,&package,sizeof(*saved));
    const char* failures[]={"float unknown = 1\n","int cpu_fireball_range = 1\n","float cpu_fireball_range = 1\nfloat cpu_fireball_range = 2\n"};
    for(unsigned i=0;i<3;++i){input.parameters=failures[i];input.parameter_bytes=strlen(failures[i]);
        CHECK(!nativeSourcePackage_Prepare(&next_package,&input,Ft_Kind_CustomFirst,&package,&diagnostic));
        CHECK(!next_package.ready&&!next_package.input_memory&&!next_package.compiler.arena&&!next_package.assets.character_arena);
        CHECK(!memcmp(saved,&package,sizeof(*saved)));++cases;}
    input.parameters=NULL;input.parameter_bytes=0;input.family=(NativeSourcePackageFamily)1;
    CHECK(!nativeSourcePackage_Prepare(&next_package,&input,Ft_Kind_CustomFirst,&package,&diagnostic));CHECK(!memcmp(saved,&package,sizeof(*saved)));++cases;
    CHECK(nativeSourcePackage_Pin(&package)&&!nativeSourcePackage_CanRelease(&package));CHECK(!nativeSourcePackage_Release(&package));
    CHECK(nativeSourcePackage_Unpin(&package)&&nativeSourcePackage_CanRelease(&package));++cases;
    memcpy(saved,&package,sizeof(*saved));CHECK(!nativeSourcePackage_CanActivate(saved)&&!nativeSourcePackage_CanRelease(saved));++cases;
    CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&package.program);
    CHECK(!nativeSourcePackage_CanRelease(&package)&&!nativeSourcePackage_Release(&package));
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&no_program);CHECK(nativeSourcePackage_CanRelease(&package));++cases;
    printf("{\"package_reserved_bytes\":%llu,\"package_active_buffer_bytes\":%llu,\"instructions\":%u,\"max_call_registers\":%u,\"imports\":35}\n",
        (unsigned long long)package.stats.reserved_bytes,(unsigned long long)package.stats.active_buffer_bytes,
        package.stats.instructions,package.stats.maximum_call_registers);
    CHECK(nativeSourcePackage_Release(&package));CHECK(nativeSourcePackage_Release(&next_package));free(saved);free(source);
}
static void forbidden(HSD_GObj* object){(void)object;abort();}
static void record_cases(const NativeFighterDefinition* donor)
{
    CHECK(nativeSourceAssets_Copy(donor,&facts,&diagnostic));size_t bytes=0;
    CHECK(nativeSourceAssets_Write(&facts,encoded,sizeof(encoded),&bytes,&diagnostic));
    CHECK(nativeSourceAssets_Read(encoded,bytes,&decoded,&diagnostic));size_t next=0;
    CHECK(nativeSourceAssets_Write(&decoded,again,sizeof(again),&next,&diagnostic));
    CHECK(next==bytes&&!memcmp(encoded,again,bytes));++cases;
    NativeFighterDefinition changed=*donor;
    changed.on_load=changed.on_tick=changed.on_death=changed.special_n=changed.special_lw=forbidden;
    changed.code_key="no_runtime_dependency";changed.source_kind=Ft_Kind_None;
    CHECK(nativeSourceAssets_Copy(&changed,&decoded,&diagnostic));
    CHECK(nativeSourceAssets_Write(&decoded,again,sizeof(again),&next,&diagnostic));
    CHECK(next==bytes&&!memcmp(encoded,again,bytes));++cases;
    // All nonempty truncated prefixes reject without publishing a partial owner.
    memset(&poison,0xA5,sizeof(poison));
    for(size_t count=0;count<bytes;++count){decoded=poison;CHECK(!nativeSourceAssets_Read(encoded,count,&decoded,&diagnostic));CHECK(!memcmp(&decoded,&poison,sizeof(decoded)));}
    encoded[bytes]=0;decoded=poison;CHECK(!nativeSourceAssets_Read(encoded,bytes+1,&decoded,&diagnostic));CHECK(!memcmp(&decoded,&poison,sizeof(decoded)));++cases;
    memset(again,0xA5,sizeof(again));memcpy(unchanged,again,sizeof(again));next=123;
    CHECK(!nativeSourceAssets_Write(&facts,again,bytes-1,&next,&diagnostic));CHECK(next==123&&!memcmp(again,unchanged,sizeof(again)));++cases;
    bad=facts;bad.bones[0].part=bad.bones[1].part;decoded=poison;
    CHECK(!nativeSourceAssets_Write(&bad,again,sizeof(again),&next,&diagnostic));CHECK(!memcmp(again,unchanged,sizeof(again)));++cases;
    bad=facts;bad.bone_count=0;CHECK(!nativeSourceAssets_Validate(&bad,&diagnostic));++cases;
    bad=facts;bad.motions[0].camera=2;CHECK(!nativeSourceAssets_Validate(&bad,&diagnostic));++cases;
    bad=facts;bad.article_count=3;bad.articles[2]=facts.articles[0];strcpy(bad.articles[2].declaration,"CustomBolt");
    CHECK(nativeSourceAssets_Validate(&bad,&diagnostic));CHECK(nativeSourceAssets_Write(&bad,again,sizeof(again),&next,&diagnostic));
    CHECK(nativeSourceAssets_Read(again,next,&decoded,&diagnostic));CHECK(decoded.articles[2].slot==decoded.articles[0].slot);++cases;
    strcpy(bad.articles[2].declaration,bad.articles[0].declaration);CHECK(!nativeSourceAssets_Validate(&bad,&diagnostic));++cases;
    bad=facts;bad.sound_banks=UINT64_C(1)<<55;CHECK(!nativeSourceAssets_Validate(&bad,&diagnostic));++cases;
    bad=facts;bad.layout.byte_range_count=1;bad.layout.byte_ranges[0]=(FtCharacterByteRange){20,1};
    CHECK(!nativeSourceAssets_Validate(&bad,&diagnostic));++cases;
    decoded=poison;CHECK(!nativeSourceAssets_Read(&decoded,sizeof(decoded),&decoded,&diagnostic));CHECK(!memcmp(&decoded,&poison,sizeof(decoded)));++cases;
    next=123;CHECK(!nativeSourceAssets_Write(&facts,&facts,sizeof(facts),&next,&diagnostic));CHECK(next==123);++cases;
}
int main(int argc,char** argv)
{
    CHECK(argc==4);CHECK(aurora_dvd_open(argv[1]));
    const NativeFighterDefinition* donor=nativeFighterRuntime_FindStockDefinition(Ft_Kind_Mario);CHECK(donor);
    record_cases(donor);
    CHECK(nativeFighterProject_CreateStockFromDisc(argv[2],"Source assets fixture",Ft_Kind_Mario));
    NativeFighterProject project;CHECK(nativeFighterProject_Open(argv[2],&project));
    NativeSourceAssetInput input={project.fighter_dat,project.fighter_size,project.animation_dat,project.animation_size,project.costume_dat,project.costume_size};
    CHECK(nativeSourceAssets_Import(&facts,&input,Ft_Kind_CustomFirst,&accepted,&diagnostic));
    CHECK(accepted.ready&&accepted.costume.joint_count==61&&accepted.character.data.x0==&accepted.character.common_attributes);
    CHECK(accepted.parts.part_to_joint==accepted.part_to_joint&&accepted.parts.joint_to_part==accepted.joint_to_part);
    CHECK(accepted.demos.result_filename==accepted.facts.demos[0]);
    CHECK(nativeSourceAssets_Import(&facts,&input,Ft_Kind_Mario,&original,&diagnostic));
    const unsigned external = MAX_FT_PARTS - 1;
    CHECK(external >= original.parts.parts_num);
    original.external_hitbox_parts[external] = true;
    CHECK(!nativeSourceAssets_ValidateExternalParts(&original, &no_program.items, &diagnostic));
    no_program.items.count = 1;
    no_program.items.programs[0].resource_reserved_part = external;
    CHECK(nativeSourceAssets_ValidateExternalParts(&original, &no_program.items, &diagnostic));
    no_program.items.programs[0].resource_reserved_part = external - 1;
    CHECK(!nativeSourceAssets_ValidateExternalParts(&original, &no_program.items, &diagnostic));
    memset(&no_program.items, 0, sizeof(no_program.items));
    const int camera_joint = original.character.common_attributes.camera_zoom_target_bone;
    original.character.common_attributes.camera_zoom_target_bone = original.parts.parts_num;
    CHECK(!nativeSourceAssets_ValidateRig(&original, &diagnostic));
    CHECK(strstr(diagnostic.message, "Camera") != NULL);
    // A slot for a future attachment is in range but empty when the camera starts.
    CHECK(original.facts.omission_count == 0 && original.parts.parts_num < MAX_FT_PARTS);
    original.facts.omissions[0].joint = original.parts.parts_num;
    original.facts.omission_count = 1;
    ++original.parts.parts_num;
    CHECK(!nativeSourceAssets_ValidateRig(&original, &diagnostic));
    CHECK(strstr(diagnostic.message, "Camera") != NULL);
    --original.parts.parts_num;
    original.facts.omission_count = 0;
    original.character.common_attributes.camera_zoom_target_bone = camera_joint;
    CHECK(nativeSourceAssets_ValidateRig(&original, &diagnostic));
    ++cases;
    struct Fighter_WaitAnimData* stock_banks[]={original.character.data.xC,original.character.data.x14};
    struct Fighter_WaitAnimData* custom_banks[]={accepted.character.data.xC,accepted.character.data.x14};
    const unsigned bank_counts[]={facts.actions,facts.demo_count};
    for(unsigned b=0;b<2;++b)for(unsigned i=0;i<bank_counts[b];++i){
        uint32_t stock=(uint32_t)stock_banks[b][i].x10_animCurrFlags;
        uint32_t custom=(uint32_t)custom_banks[b][i].x10_animCurrFlags;
        CHECK((stock&UINT32_C(0x03ffffff))==(custom&UINT32_C(0x03ffffff)));
        if(stock_banks[b][i].x14&&(stock>>26)==facts.animation_tag)
            CHECK((custom>>26)==Ft_Kind_CustomFirst);
        else CHECK(custom==stock);
    }
    nativeSourceAssets_Release(&original);++cases;
    for(unsigned i=0;i<facts.moves.count;++i){CHECK(!accepted.motions[i].anim_cb&&!accepted.motions[i].input_cb&&!accepted.motions[i].phys_cb&&!accepted.motions[i].coll_cb);
        CHECK(accepted.motions[i].anim_id==facts.motions[i].animation&&accepted.motions[i].x4_flags==(enum_t)facts.motions[i].flags&&accepted.motions[i]._==facts.motions[i].attack);}
    CHECK(accepted.character.fighter_items[0]&&accepted.character.fighter_items[2]);++cases;
    NativeSourceImportedAssets* saved=malloc(sizeof(*saved));CHECK(saved);memcpy(saved,&accepted,sizeof(*saved));
    bad=facts;++bad.joints;CHECK(!nativeSourceAssets_Import(&bad,&input,Ft_Kind_CustomFirst,&candidate,&diagnostic));
    CHECK(!candidate.ready&&!candidate.motions&&!candidate.character_arena&&!candidate.animation_arena&&!candidate.costume.arena);
    CHECK(!memcmp(saved,&accepted,sizeof(*saved)));++cases;
    bad=facts;bad.articles[1].kind_attribute_offset=FT_CHARACTER_EXT_ATTR_CAPACITY-4;
    CHECK(nativeSourceAssets_Validate(&bad,&diagnostic));CHECK(!nativeSourceAssets_Import(&bad,&input,Ft_Kind_CustomFirst,&candidate,&diagnostic));
    CHECK(!candidate.ready&&!candidate.character_arena&&!memcmp(saved,&accepted,sizeof(*saved)));++cases;
    CHECK(!nativeSourceAssets_Import(&facts,&input,Ft_Kind_CustomFirst,&accepted,&diagnostic));CHECK(!memcmp(saved,&accepted,sizeof(*saved)));++cases;
    free(saved);nativeSourceAssets_Release(&candidate);nativeSourceAssets_Release(&accepted);CHECK(!accepted.ready&&!accepted.character_arena);
    package_cases(&input,argv[3]);nativeFighterProject_Close(&project);
    printf("{\"status\":\"PASS source assets\",\"checks\":%u,\"cases\":%u,\"disc_import\":true,\"authored_native_callbacks\":false}\n",checks,cases);return 0;
}
