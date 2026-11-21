#include <melee/ft/native_source_entity_assets.h>
#include <melee/ft/native_fighter_project.h>
#include <melee/ft/native_script_model_metadata.h>
#include <melee/it/itarticle_data.h>
#include <melee/it/native_article_animation_demand.h>
#include <sysdolphin/baselib/dat_import_internal.h>
#include <sysdolphin/baselib/native_animation_pool.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks,cases,imports,scalar_comparisons;
static NativeScriptDiagnostic diagnostic;
static NativeSourceImportedAssets assets[3];
static NativeSourceEntityAssets entities[3];
static NativeFighterProject projects[3];
static NativeSourceAssetInput inputs[3];
static NativeScriptSourceNode nodes[512];
static NativeScriptSourceTree tree;
static char source[8192];
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"animation import %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
void test_native_article_animation_array_import(void);
void test_native_article_model_roots_import(void);
static void parse(const char* text)
{
    snprintf(source,sizeof(source),"%s",text);
    tree=(NativeScriptSourceTree){.source=source,.source_bytes=(uint32_t)strlen(source),.nodes=nodes,.capacity=512};
    CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
}
static void fresh(unsigned fighter)
{
    nativeSourceEntityAssets_Release(&entities[fighter]);nativeSourceAssets_Release(&assets[fighter]);
    CHECK(nativeSourceAssets_Import(&projects[fighter].facts,&inputs[fighter],Ft_Kind_CustomFirst,&assets[fighter],&diagnostic));
}
static const NativeScriptItemAsset* row(unsigned fighter,const char* name)
{
    for(unsigned i=0;i<entities[fighter].count;++i)if(!strcmp(entities[fighter].rows[i].declaration,name))return &entities[fighter].rows[i];
    CHECK(false);return NULL;
}
static void prepare(unsigned fighter)
{
    NativeSourceResources resources={0};
    CHECK(nativeSourceEntityAssets_Prepare(&entities[fighter],&assets[fighter],&resources,&tree,&diagnostic));++imports;
}
static void negative(const char* fields)
{
    fresh(0);char text[8192];snprintf(text,sizeof(text),
        "fighter:\n    pass\narticle It_Kind_Link_Boomerang states 4:\n%s",fields);parse(text);
    NativeSourceResources resources={0};memset(&diagnostic,0,sizeof(diagnostic));
    bool accepted=nativeSourceEntityAssets_Prepare(&entities[0],&assets[0],&resources,&tree,&diagnostic);
    if(accepted)fprintf(stderr,"Unexpected accepted metadata:\n%s",fields);
    CHECK(!accepted);
    CHECK(diagnostic.message[0]);
    static const NativeSourceEntityAssets empty;CHECK(!memcmp(&entities[0],&empty,sizeof(empty)));++cases;
}
static void source_cases(unsigned fighter)
{
    const char* declaration=fighter?"It_Kind_CLink_Boomerang":"It_Kind_Link_Boomerang";
    char text[8192];snprintf(text,sizeof(text),
        "fighter:\n    pass\narticle %s states 4:\n"
        "    model_root TrailOne 68\n    model_root TrailTwo 72\n"
        "    model_animation FlightOne 76\n    model_animation FlightTwo 88\n"
        "article Twin states 4:\n"
        "    model_root TwinOne 68\n    model_root TwinTwo 72\n"
        "    model_animation FlightOne 76\n    model_animation Independent 88\n",declaration);
    parse(text);
    uint32_t* marker=HSD_DatImportAlloc(&assets[fighter].character.import,sizeof(*marker));CHECK(marker);*marker=0x1234ABCD;
    size_t before=assets[fighter].character.import.arena_used;
    uint32_t records_before=assets[fighter].character.import.record_count;
    prepare(fighter);
    const NativeScriptItemAsset* original=row(fighter,declaration),*twin=row(fighter,"Twin");
    CHECK(original->article==twin->article&&original->owner_node!=twin->owner_node);
    CHECK(original->model_root_count==2&&twin->model_root_count==2);
    CHECK(original->model_animation_count==2&&twin->model_animation_count==2);
    CHECK(original->model_animations!=twin->model_animations);
    CHECK(!strcmp(original->model_animations[0].name,"FlightOne")&&!strcmp(twin->model_animations[0].name,"FlightOne"));
    CHECK(!strcmp(original->model_animations[1].name,"FlightTwo")&&!strcmp(twin->model_animations[1].name,"Independent"));
    CHECK(original->model_roots[0].root==twin->model_roots[0].root&&original->model_roots[1].root==twin->model_roots[1].root);
    for(unsigned i=0;i<2;++i){
        const NativeScriptItemModelAnimationAsset* record=&original->model_animations[i];
        for(unsigned channel=0;channel<3;++channel){
            CHECK(record->components[channel].attribute_offset==(i?88u:76u)+4*channel);
            CHECK(record->components[channel].array_index==UINT32_MAX);
        }
        CHECK(record->anim&&!record->matanim&&!record->shapeanim);
        CHECK(record->anim==twin->model_animations[i].anim);
        NativeArticleAnimationDemand demand;
        CHECK(nativeArticleAnimation_Census(original->model_roots[i].root,record->anim,record->matanim,record->shapeanim,&demand));
        printf("{\"observation\":\"bundle_census\",\"owner\":\"%s\",\"offset\":%u,\"controllers\":%u,\"tracks\":%u,\"uncovered\":%u}\n",
            declaration,record->components[0].attribute_offset,demand.controllers,demand.tracks,demand.uncovered);
        CHECK((uintptr_t)record->anim>=(uintptr_t)assets[fighter].character_arena&&
            (uintptr_t)record->anim-(uintptr_t)assets[fighter].character_arena<FT_CHARACTER_IMPORT_ARENA_SIZE);
        size_t used=assets[fighter].character.import.arena_used;uint32_t count=assets[fighter].character.import.record_count;
        ItArticleModelAnimationBundle bundle;
        CHECK(itArticle_ImportModelAnimation(&assets[fighter].character.import,original->article,record->components,&bundle));
        CHECK(bundle.anim==record->anim&&!bundle.matanim&&!bundle.shapeanim);
        CHECK(used==assets[fighter].character.import.arena_used&&count==assets[fighter].character.import.record_count);
    }
    // Current native character import may already have decoded these exact
    // component graphs; source preparation must neither rewind nor detach them.
    CHECK(assets[fighter].character.import.arena_used==before&&assets[fighter].character.import.record_count==records_before);
    CHECK(*marker==0x1234ABCD&&!original->article->native_model_animation_count);
    NativeScriptItemMetadataContext context={.assets=entities[fighter].rows,.asset_count=entities[fighter].count,
        .motions=&assets[fighter].facts.moves,.source_facts=&assets[fighter].facts,.source_assets=&assets[fighter]};
    for(unsigned n=1;n<tree.count;++n)if(nodes[n].kind==NativeScriptSource_Metadata&&nativeScriptSource_SpanEquals(&tree,nodes[n].name,"model_animation")){
        CHECK(nativeScriptItemMetadata_Validate(&context,&tree,n,&diagnostic));
    }
    char saved[128];snprintf(saved,sizeof(saved),"%s",original->model_animations[0].name);
    memset(source,'X',strlen(source));
    CHECK(!strcmp(saved,original->model_animations[0].name));
    nativeSourceEntityAssets_Release(&entities[fighter]);
    CHECK(*marker==0x1234ABCD); // Owning DAT arena survives the table release.
    ++cases;
}

static NativeScriptModelMetadata animation_metadata(const char* text)
{
    NativeScriptSourceTree input={.source=text,.source_bytes=(uint32_t)strlen(text)};
    NativeScriptModelMetadata parsed;
    CHECK(nativeScriptModelMetadata_Parse(&input,(NativeScriptSourceSpan){.length=input.source_bytes},true,&parsed));
    return parsed;
}
static void metadata_cases(void)
{
    static const struct {const char* text;uint32_t offsets[3],indices[3];} valid[]={
        {"Clip 76",{76,80,84},{UINT32_MAX,UINT32_MAX,UINT32_MAX}},
        {"Clip 0x4c",{76,80,84},{UINT32_MAX,UINT32_MAX,UINT32_MAX}},
        {"Spin material 88",{UINT32_MAX,88,UINT32_MAX},{UINT32_MAX,UINT32_MAX,UINT32_MAX}},
        {"Clip shape 24 joint 8 material None",{8,UINT32_MAX,24},{UINT32_MAX,UINT32_MAX,UINT32_MAX}},
        {"Clip joint 16[0] material 20[1] shape 24[0]",{16,20,24},{0,1,0}},
        {"Clip material 0x14[0x1]",{UINT32_MAX,20,UINT32_MAX},{UINT32_MAX,1,UINT32_MAX}},
        {"Clip joint None material None shape None",{UINT32_MAX,UINT32_MAX,UINT32_MAX},{UINT32_MAX,UINT32_MAX,UINT32_MAX}}
    };
    for(unsigned i=0;i<sizeof(valid)/sizeof(*valid);++i){
        NativeScriptModelMetadata parsed=animation_metadata(valid[i].text);
        CHECK(parsed.animation&&parsed.name.length>0);
        for(unsigned channel=0;channel<3;++channel){
            CHECK(parsed.components[channel].attribute_offset==valid[i].offsets[channel]);
            CHECK(parsed.components[channel].array_index==valid[i].indices[channel]);
        }
        ++cases;
    }
    static const char* const invalid[]={
        "Clip", "Clip joint", "Clip joint 4 material", "Clip joint 4 joint 8",
        "Clip unknown 4", "Clip material 7", "Clip material -4", "Clip material +4",
        "Clip material 4294967296", "Clip 4294967292", "Clip material 0x",
        "Clip material 8[]", "Clip material 8[0", "Clip material 8[0]extra",
        "Clip material 8[-1]", "Clip material 8[1073741823]", "Clip material None[0]",
        "Clip material 8[1][2]", "Clip material 8 joint 0 shape 4 extra 8"
    };
    for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);++i){
        NativeScriptSourceTree input={.source=invalid[i],.source_bytes=(uint32_t)strlen(invalid[i])};
        NativeScriptModelMetadata parsed,old;memset(&parsed,0xA5,sizeof(parsed));old=parsed;
        CHECK(!nativeScriptModelMetadata_Parse(&input,(NativeScriptSourceSpan){.length=input.source_bytes},true,&parsed));
        CHECK(!memcmp(&parsed,&old,sizeof(old)));++cases;
    }
}
static void explicit_source_cases(void)
{
    fresh(0);
    parse("fighter:\n    pass\narticle It_Kind_Link_Boomerang states 4:\n"
          "    model_root TrailOne 68\n    model_animation Flight joint 76 material None shape None\n");
    prepare(0);
    const NativeScriptItemAsset* asset=row(0,"It_Kind_Link_Boomerang");
    CHECK(asset->model_animation_count==1&&asset->model_animations[0].anim);
    CHECK(!asset->model_animations[0].matanim&&!asset->model_animations[0].shapeanim);
    NativeScriptItemMetadataContext context={.assets=entities[0].rows,.asset_count=entities[0].count,
        .motions=&assets[0].facts.moves,.source_facts=&assets[0].facts,.source_assets=&assets[0]};
    unsigned node=0;
    for(unsigned n=1;n<tree.count;++n)if(nativeScriptSource_SpanEquals(&tree,nodes[n].name,"model_animation"))node=n;
    CHECK(node&&nativeScriptItemMetadata_Validate(&context,&tree,node,&diagnostic));
    // Pointer selection and array indexing are part of exact asset identity.
    char* offset=strstr(source,"joint 76");CHECK(offset);offset[6]='8';offset[7]='8';
    CHECK(!nativeScriptItemMetadata_Validate(&context,&tree,node,&diagnostic));
    CHECK(strstr(diagnostic.message,"components differ"));offset[6]='7';offset[7]='6';
    NativeScriptItemModelAnimationAsset* record=(NativeScriptItemModelAnimationAsset*)asset->model_animations;
    record->components[0].array_index=0;
    CHECK(!nativeScriptItemMetadata_Validate(&context,&tree,node,&diagnostic));
    CHECK(strstr(diagnostic.message,"components differ"));record->components[0].array_index=UINT32_MAX;
    CHECK(nativeScriptItemMetadata_Validate(&context,&tree,node,&diagnostic));++cases;
}
static void ness_source_case(void)
{
    parse("fighter:\n    pass\narticle It_Kind_Ness_Yoyo states 4:\n"
          "    model_root String 80\n    model_root Tip 84\n    model_animation Spin material 88\n");
    prepare(2);
    const NativeScriptItemAsset* asset=row(2,"It_Kind_Ness_Yoyo");
    CHECK(asset->model_root_count==2&&asset->model_animation_count==1);
    const NativeScriptItemModelAnimationAsset* record=&asset->model_animations[0];
    CHECK(!record->anim&&record->matanim&&!record->shapeanim);
    CHECK(record->components[0].attribute_offset==UINT32_MAX&&record->components[1].attribute_offset==88&&record->components[2].attribute_offset==UINT32_MAX);
    NativeFighterArticleAnimationComponent expected[3]={{UINT32_MAX,UINT32_MAX},{88,UINT32_MAX},{UINT32_MAX,UINT32_MAX}};
    ItArticleModelAnimationBundle bundle;
    size_t used=assets[2].character.import.arena_used;uint32_t count=assets[2].character.import.record_count;
    CHECK(itArticle_ImportModelAnimation(&assets[2].character.import,asset->article,expected,&bundle));
    CHECK(!bundle.anim&&bundle.matanim==record->matanim&&!bundle.shapeanim);
    CHECK(used==assets[2].character.import.arena_used&&count==assets[2].character.import.record_count);
    NativeScriptItemMetadataContext context={.assets=entities[2].rows,.asset_count=entities[2].count,
        .motions=&assets[2].facts.moves,.source_facts=&assets[2].facts,.source_assets=&assets[2]};
    for(unsigned n=1;n<tree.count;++n)if(nativeScriptSource_SpanEquals(&tree,nodes[n].name,"model_animation"))
        CHECK(nativeScriptItemMetadata_Validate(&context,&tree,n,&diagnostic));
    printf("{\"observation\":\"ness_material_only\",\"offset\":88,\"joint\":false,\"material\":true,\"shape\":false}\n");++cases;
}

static void put32(uint8_t* p,uint32_t value)
{p[0]=(uint8_t)(value>>24);p[1]=(uint8_t)(value>>16);p[2]=(uint8_t)(value>>8);p[3]=(uint8_t)value;}
static size_t synthetic(uint8_t bytes[1024],uint32_t external_slot,uint32_t invalid_slot)
{
    static const uint32_t slots[]={0,48,52,56,60,64,68,96,100,112,116,128,132,160,192,224};
    memset(bytes,0,1024);uint8_t* data=bytes+32;put32(data,32);
    put32(data+48,96);put32(data+52,112);put32(data+56,128);
    for(unsigned i=0;i<3;++i){put32(data+60+i*4,160+i*32);put32(data+96+i*16,160+i*32);put32(data+100+i*16,160+i*32);put32(data+160+i*32,256+i*32);}
    put32(data+176,0x123);put32(data+272,0x456);
    unsigned count=0;
    for(unsigned i=0;i<sizeof(slots)/sizeof(*slots);++i)if(slots[i]!=external_slot&&slots[i]!=invalid_slot)put32(data+384+4*count++,slots[i]);
    size_t end=32+384+4*count;
    if(external_slot!=UINT32_MAX){put32(data+external_slot,UINT32_MAX);put32(bytes+end,external_slot);put32(bytes+end+4,0);end+=8;
        memcpy(bytes+end,"ExternalAnimation",sizeof("ExternalAnimation"));end+=sizeof("ExternalAnimation");put32(bytes+16,1);}
    put32(bytes,(uint32_t)end);put32(bytes+4,384);put32(bytes+8,count);return end;
}
static void typed_synthetic_cases(void)
{
    uint8_t bytes[1024],arena[8192];HSD_Dat dat;HSD_DatImport import;
    NativeScriptModelMetadata parsed=animation_metadata("Clip joint 16[0] material 20[1] shape 24[0]");
    NativeFighterArticleAnimationComponent* components=parsed.components;
    Article article={.native_special_attribute_source=32};
    size_t size=synthetic(bytes,UINT32_MAX,UINT32_MAX);
    CHECK(HSD_DatOpen(&dat,bytes,size)==HSD_DAT_OK);
    CHECK(HSD_DatImportInit(&import,&dat,arena,sizeof(arena),32)==HSD_DAT_OK);
    ItArticleModelAnimationBundle first;
    CHECK(itArticle_ImportModelAnimation(&import,&article,components,&first));
    CHECK(first.anim&&first.matanim&&first.shapeanim&&first.anim->flags==0x123&&first.anim->child->flags==0x456);
    NativeFighterArticleAnimationComponent direct[3]={{28,UINT32_MAX},{32,UINT32_MAX},{36,UINT32_MAX}};
    uint32_t count=import.record_count;size_t used=import.arena_used;ItArticleModelAnimationBundle again;
    CHECK(itArticle_ImportModelAnimation(&import,&article,direct,&again));
    CHECK(!memcmp(&first,&again,sizeof(first))&&import.record_count==count&&import.arena_used==used);++cases;
    for(unsigned scenario=0;scenario<10;++scenario){
        size=synthetic(bytes,scenario==5?52:scenario==6?116:UINT32_MAX,scenario==7?52:scenario==8?116:UINT32_MAX);
        NativeFighterArticleAnimationComponent selected[3];memcpy(selected,components,sizeof(selected));
        if(scenario==0)selected[1].array_index=4;
        if(scenario==1)selected[1].array_index=1073741823;
        if(scenario==2)selected[1].attribute_offset=UINT32_MAX;
        if(scenario==3)selected[1].attribute_offset=21;
        if(scenario==4)selected[1].attribute_offset=64;
        CHECK(HSD_DatOpen(&dat,bytes,size)==HSD_DAT_OK);CHECK(HSD_DatImportInit(&import,&dat,arena,sizeof(arena),32)==HSD_DAT_OK);
        if(scenario==9)import.arena_size=import.arena_used;
        ItArticleModelAnimationBundle saved=first,output=first;
        CHECK(!itArticle_ImportModelAnimation(&import,&article,selected,&output));CHECK(!memcmp(&saved,&output,sizeof(saved)));
        CHECK(import.status==(scenario==5||scenario==6?HSD_DAT_EXTERNAL:scenario==7||scenario==8?HSD_DAT_RELOCATION:scenario==9?HSD_DAT_CAPACITY:HSD_DAT_DESCRIPTOR));
    }++cases;
    for(unsigned slot=52;slot<=116;slot+=64){
        size=synthetic(bytes,UINT32_MAX,slot);put32(bytes+32+slot,0);
        CHECK(HSD_DatOpen(&dat,bytes,size)==HSD_DAT_OK);CHECK(HSD_DatImportInit(&import,&dat,arena,sizeof(arena),32)==HSD_DAT_OK);
        CHECK(itArticle_ImportModelAnimation(&import,&article,components,&again));CHECK(again.anim&&!again.matanim&&again.shapeanim);
    }
    NativeFighterArticleAnimationComponent omitted[3]={{UINT32_MAX,UINT32_MAX},{UINT32_MAX,UINT32_MAX},{UINT32_MAX,UINT32_MAX}};
    CHECK(itArticle_ImportModelAnimation(&import,&article,omitted,&again));CHECK(!again.anim&&!again.matanim&&!again.shapeanim);++cases;
}
static void synthetic_census_cases(void)
{
    // This is deliberately synthetic descriptor evidence, separate from the
    // retail bundles. Positional children sharing one immutable controller
    // description each create a native controller application.
    uint8_t stream[]={HSD_A_OP_CON,0x3f,0x80,0,0,0};
    HSD_FObjDesc track={.type=HSD_A_J_TRAX,.length=sizeof(stream),.ad=stream};
    HSD_AObjDesc controller={.fobjdesc=&track};
    HSD_Joint children[2]={{0},{0}},root={.child=&children[0]};
    children[0].next=&children[1];
    HSD_AnimJoint branches[2]={{.aobjdesc=&controller},{.aobjdesc=&controller}};
    branches[0].next=&branches[1];
    HSD_AnimJoint animation={.child=&branches[0],.aobjdesc=&controller};
    NativeArticleAnimationDemand output;
    CHECK(nativeArticleAnimation_Census(&root,&animation,NULL,NULL,&output));
    CHECK(output.controllers==3&&output.tracks==3&&!output.uncovered);
    branches[0].aobjdesc=NULL;
    CHECK(nativeArticleAnimation_Census(&root,&animation,NULL,NULL,&output));
    CHECK(output.controllers==2&&output.tracks==2&&!output.uncovered);
    NativeArticleAnimationDemand saved=output;
    track.length=4; // CON's four-byte operand is truncated.
    CHECK(!nativeArticleAnimation_Census(&root,&animation,NULL,NULL,&output));
    CHECK(!memcmp(&saved,&output,sizeof(output)));
    track.length=sizeof(stream);track.type=0xff;
    CHECK(nativeArticleAnimation_Census(&root,&animation,NULL,NULL,&output));
    CHECK(output.uncovered==NativeArticleAnimation_Channel);
    track.type=HSD_A_J_TRAX;track.startframe=INFINITY;saved=output;
    CHECK(!nativeArticleAnimation_Census(&root,&animation,NULL,NULL,&output));
    CHECK(!memcmp(&saved,&output,sizeof(output)));
    track.startframe=0;children[1].next=&children[0];
    CHECK(!nativeArticleAnimation_Census(&root,&animation,NULL,NULL,&output));
    CHECK(!memcmp(&saved,&output,sizeof(output)));
    children[1].next=NULL;branches[1].aobjdesc=NULL;
    CHECK(nativeArticleAnimation_Census(&root,&animation,NULL,NULL,&output));
    CHECK(output.controllers==1&&output.tracks==1&&!output.uncovered);++cases;
}
typedef struct ScalarTrace { float value; unsigned calls; } ScalarTrace;
static void record_scalar(void* object,enum_t type,HSD_ObjData* value)
{
    CHECK(type==HSD_A_J_TRAX);
    ScalarTrace* trace=object;trace->value=value->fv;++trace->calls;
}
static void evaluate_stream(uint8_t* bytes,uint32_t length,
    const float expected[5],const unsigned calls[5])
{
    HSD_FObjDesc descriptor={.type=HSD_A_J_TRAX,.length=length,.ad=bytes,
        .frac_value=HSD_A_FRAC_S8,.frac_slope=HSD_A_FRAC_S8};
    HSD_AObjDesc controller={.fobjdesc=&descriptor};
    HSD_Joint joint={0};HSD_AnimJoint animation={.aobjdesc=&controller};
    NativeArticleAnimationDemand demand;
    CHECK(nativeArticleAnimation_Census(&joint,&animation,NULL,NULL,&demand));
    CHECK(demand.controllers==1&&demand.tracks==1&&!demand.uncovered);
    // Use the actual public evaluator on a stack leaf with the same initial
    // fields as HSD_FObjLoadDesc. No pool object/controller/graph is built.
    HSD_FObj leaf={.obj_type=descriptor.type,.frac_value=descriptor.frac_value,
        .frac_slope=descriptor.frac_slope,.ad_head=descriptor.ad,.length=descriptor.length};
    ScalarTrace trace={0};HSD_FObjReqAnimAll(&leaf,0);
    for(unsigned frame=0;frame<5;++frame){
        HSD_FObjInterpretAnim(&leaf,&trace,record_scalar,frame?1:0);
        CHECK(trace.calls==calls[frame]);CHECK(trace.value==expected[frame]);
        CHECK(leaf.ad>=bytes&&leaf.ad<=bytes+length);++scalar_comparisons;
    }
    CHECK(!nativeAnimationGlobals.callback_depth&&!nativeAnimationGlobals.heap_epoch);
}
static void native_stream_cases(void)
{
    static const unsigned every[]={1,2,3,4,5},keys[]={1,1,1,1,2};
    static const float held[]={2,2,2,2,6},linear[]={2,3,4,5,6};
    static const float spline[]={2,2.625f,4,5.375f,6};
    static const float slope[]={2,3.1875f,4.5f,5.5625f,6};
    uint8_t constant[]={HSD_A_OP_CON,2,4,HSD_A_OP_CON,6,4};
    uint8_t lin[]={HSD_A_OP_LIN,2,4,HSD_A_OP_LIN,6,4};
    uint8_t spl0[]={HSD_A_OP_SPL0,2,4,HSD_A_OP_SPL0,6,4};
    uint8_t spl[]={HSD_A_OP_SPL,2,0,4,HSD_A_OP_SPL,6,0,4};
    uint8_t slp[]={HSD_A_OP_SPL,2,0,4,HSD_A_OP_SLP,1,HSD_A_OP_SPL,6,0,4};
    uint8_t key[]={HSD_A_OP_KEY,2,4,HSD_A_OP_KEY,6,4};
    uint8_t terminal[]={HSD_A_OP_LIN,2,4,HSD_A_OP_LIN,6};
    uint8_t key_terminal[]={HSD_A_OP_KEY,2,4,HSD_A_OP_KEY,6};
    evaluate_stream(constant,sizeof(constant),held,every);
    evaluate_stream(lin,sizeof(lin),linear,every);
    evaluate_stream(spl0,sizeof(spl0),spline,every);
    evaluate_stream(spl,sizeof(spl),spline,every);
    evaluate_stream(slp,sizeof(slp),slope,every);
    evaluate_stream(key,sizeof(key),held,keys);
    evaluate_stream(terminal,sizeof(terminal),linear,every);
    evaluate_stream(key_terminal,sizeof(key_terminal),held,keys);++cases;
}
int main(int argc,char** argv)
{
    CHECK(argc==4);
    metadata_cases();
    // No OS/HSD heap bootstrap, class initialization, AObj construction or live
    // graph publication occurs anywhere in this descriptor-only process.
    for(unsigned f=0;f<3;++f){
        CHECK(nativeFighterProject_Open(argv[f+1],&projects[f]));
        const char* name=f==2?"It_Kind_Ness_Yoyo":f?"It_Kind_CLink_Boomerang":"It_Kind_Link_Boomerang";unsigned selected=0;
        for(;selected<projects[f].facts.article_count;++selected)if(!strcmp(projects[f].facts.articles[selected].declaration,name))break;
        CHECK(selected<projects[f].facts.article_count&&projects[f].facts.article_count<NATIVE_SOURCE_ARTICLES);
        NativeSourceArticleAsset twin=projects[f].facts.articles[selected];strcpy(twin.declaration,"Twin");
        projects[f].facts.articles[projects[f].facts.article_count++]=twin;
        inputs[f]=(NativeSourceAssetInput){projects[f].fighter_dat,projects[f].fighter_size,projects[f].animation_dat,projects[f].animation_size,projects[f].costume_dat,projects[f].costume_size};fresh(f);
    }
    static const char* const bad[]={
        "    model_animation Same 76\n    model_animation Same 88\n",
        "    model_root Same 68\n    model_animation Same 76\n",
        "    model_animation Wrong 77\n",
        "    model_animation Wrong 4294967296\n",
        "    model_animation Wrong 4294967292\n",
        "    model_animation Wrong 10000\n",
        "    model_animation bad-name 76\n",
        "    model_animation 36 76\n",
        "    model_animation Wrong 76 extra\n",
        "    model_animation Wrong joint 77\n",
        "    state 0 anim -1:\n        on physics:\n            pass\n    model_animation Late 76\n"
    };
    for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i)negative(bad[i]);
    char many[4096]={0};for(unsigned i=0;i<9;++i){char line[100];snprintf(line,sizeof(line),"    model_animation Clip%u 76\n",i);strcat(many,line);}negative(many);
    fresh(0);many[0]=0;strcat(many,"fighter:\n    pass\narticle It_Kind_Link_Boomerang states 4:\n");
    for(unsigned i=0;i<8;++i){char line[100];snprintf(line,sizeof(line),"    model_animation Clip%u 76\n",i);strcat(many,line);}
    parse(many);prepare(0);CHECK(row(0,"It_Kind_Link_Boomerang")->model_animation_count==8);++cases;
    // The importer admits identifier shape, not compiler operator/name scope.
    // These names import but do not bypass compiler namespace validation.
    fresh(0);parse("fighter:\n    pass\narticle It_Kind_Link_Boomerang states 4:\n    model_animation and 76\n    model_animation not 88\n");prepare(0);
    const NativeScriptItemAsset* keywords=row(0,"It_Kind_Link_Boomerang");
    CHECK(keywords->model_animation_count==2&&!strcmp(keywords->model_animations[0].name,"and")&&!strcmp(keywords->model_animations[1].name,"not"));
    NativeScriptItemMetadataContext keyword_context={.assets=entities[0].rows,.asset_count=entities[0].count,
        .motions=&assets[0].facts.moves,.source_facts=&assets[0].facts,.source_assets=&assets[0]};
    for(unsigned n=1;n<tree.count;++n)if(nodes[n].kind==NativeScriptSource_Metadata&&nativeScriptSource_SpanEquals(&tree,nodes[n].name,"model_animation")){
        CHECK(!nativeScriptItemMetadata_Validate(&keyword_context,&tree,n,&diagnostic));
        CHECK(strstr(diagnostic.message,"conflicts with a source/native namespace"));
    }
    ++cases;
    fresh(0);parse("fighter:\n    pass\narticle Missing states 0:\n    model_animation One 76\n");
    NativeSourceResources resources={0};CHECK(!nativeSourceEntityAssets_Prepare(&entities[0],&assets[0],&resources,&tree,&diagnostic));++cases;
    fresh(0);parse("fighter:\n    pass\narticle It_Kind_Link_Boomerang states 4:\n    on created:\n        pass\n");prepare(0);
    CHECK(!row(0,"It_Kind_Link_Boomerang")->model_animations&&!row(0,"It_Kind_Link_Boomerang")->model_animation_count);++cases;
    fresh(0);source_cases(0);source_cases(1);
    explicit_source_cases();
    ness_source_case();
    typed_synthetic_cases();
    synthetic_census_cases();
    native_stream_cases();
    test_native_article_animation_array_import();test_native_article_model_roots_import();++cases;
    for(unsigned f=0;f<3;++f){nativeSourceEntityAssets_Release(&entities[f]);nativeSourceAssets_Release(&assets[f]);nativeFighterProject_Close(&projects[f]);}
    printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"entity_imports\":%u,\"native_scalar_comparisons\":%u}\n",checks,cases,imports,scalar_comparisons);return 0;
}
