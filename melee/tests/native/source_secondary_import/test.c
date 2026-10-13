#include <melee/ft/native_source_entity_assets.h>
#include <melee/ft/native_fighter_project.h>
#include <melee/it/native_article_model_pool.h>
#include <sysdolphin/baselib/dat_import_internal.h>
#include <sysdolphin/baselib/memory.h>
#include <sysdolphin/baselib/object.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/id.h>
#include <sysdolphin/baselib/mtx.h>
#include <dolphin/os/OSAlloc.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks, cases, imports, constructs, matrix_visits;
static NativeScriptDiagnostic diagnostic;
static NativeSourceImportedAssets assets[2];
static NativeSourceEntityAssets entities[2];
static NativeFighterProject projects[2];
static NativeSourceAssetInput inputs[2];
static NativeScriptSourceNode nodes[512];
static NativeScriptSourceTree tree;
static char source[4096];
static const HSD_Joint* roots[4];
static unsigned char heap_bytes[16 * 1024 * 1024] ATTRIBUTE_ALIGN(64);
static NativeArticleModelPool pool;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"secondary import %u: %s (%s)\n",__LINE__,#x,diagnostic.message); exit(1); } } while (0)
void test_native_article_model_roots_import(void);
static void parse(const char* text)
{
    snprintf(source, sizeof(source), "%s", text);
    tree = (NativeScriptSourceTree){ .source=source, .source_bytes=(uint32_t)strlen(source), .nodes=nodes, .capacity=512 };
    CHECK(nativeScriptSource_Parse(&tree, &diagnostic));
}
static void fresh(unsigned fighter)
{
    nativeSourceEntityAssets_Release(&entities[fighter]); nativeSourceAssets_Release(&assets[fighter]);
    CHECK(nativeSourceAssets_Import(&projects[fighter].facts,&inputs[fighter],Ft_Kind_CustomFirst,&assets[fighter],&diagnostic));
}
static const NativeScriptItemAsset* row(unsigned fighter, const char* name)
{
    for (unsigned i=0; i<entities[fighter].count; ++i)
        if (!strcmp(entities[fighter].rows[i].declaration,name)) return &entities[fighter].rows[i];
    CHECK(false); return NULL;
}
static void import(unsigned fighter)
{
    NativeSourceResources resources={0};
    CHECK(nativeSourceEntityAssets_Prepare(&entities[fighter],&assets[fighter],&inputs[fighter],&resources,&tree,&diagnostic));
    ++imports;
}
static void negative(const char* fields, const char* expected)
{
    fresh(0);
    char text[4096]; snprintf(text,sizeof(text),"fighter:\n    pass\narticle It_Kind_Link_Arrow states 0:\n%s",fields); parse(text);
    NativeSourceResources resources={0};
    CHECK(!nativeSourceEntityAssets_Prepare(&entities[0],&assets[0],&inputs[0],&resources,&tree,&diagnostic));
    CHECK(strstr(diagnostic.message,expected));
    static const NativeSourceEntityAssets vacant;
    CHECK(!memcmp(&entities[0],&vacant,sizeof(vacant)));
    ++cases;
}
static void source_cases(unsigned fighter)
{
    const char* declaration=fighter?"It_Kind_CLink_Arrow":"It_Kind_Link_Arrow";
    char text[4096]; snprintf(text,sizeof(text),
        "fighter:\n    pass\narticle %s states 5:\n    model_root First 36\n    model_root Second 0x28\n"
        "article Twin states 5:\n    model_root costarring 36\n    model_root liquid 40\n",declaration);
    parse(text);
    uint32_t* marker=HSD_DatImportAlloc(&assets[fighter].character.import,sizeof(*marker)); CHECK(marker); *marker=0xA1235678;
    size_t before=assets[fighter].character.import.arena_used;
    import(fighter);
    NativeScriptItemMetadataContext context={.assets=entities[fighter].rows,.asset_count=entities[fighter].count,.motions=&assets[fighter].facts.moves};
    for(unsigned n=1;n<tree.count;++n)if(nodes[n].kind==NativeScriptSource_Metadata&&nativeScriptSource_SpanEquals(&tree,nodes[n].name,"model_root")){
        CHECK(nativeScriptItemMetadata_Validate(&context,&tree,n,&diagnostic));
        NativeScriptItemAsset* binding=NULL;
        for(unsigned i=0;i<entities[fighter].count;++i)if(entities[fighter].rows[i].owner_node==nodes[n].parent)binding=&entities[fighter].rows[i];
        CHECK(binding);uint32_t owner=binding->owner_node,count=binding->model_root_count;
        binding->owner_node=UINT32_MAX;CHECK(!nativeScriptItemMetadata_Validate(&context,&tree,n,&diagnostic));binding->owner_node=owner;
        binding->model_root_count=count-1;CHECK(!nativeScriptItemMetadata_Validate(&context,&tree,n,&diagnostic));binding->model_root_count=count;
        NativeScriptItemModelRootAsset* roots=(NativeScriptItemModelRootAsset*)binding->model_roots;
        NativeScriptItemModelRootAsset saved=roots[0];roots[0].attribute_offset+=4;
        CHECK(!nativeScriptItemMetadata_Validate(&context,&tree,n,&diagnostic));roots[0]=saved;
        roots[0].name="different";CHECK(!nativeScriptItemMetadata_Validate(&context,&tree,n,&diagnostic));roots[0]=saved;
        roots[0].root=NULL;CHECK(!nativeScriptItemMetadata_Validate(&context,&tree,n,&diagnostic));roots[0]=saved;
        CHECK(nativeScriptItemMetadata_Validate(&context,&tree,n,&diagnostic));
    }
    const NativeScriptItemAsset* first=row(fighter,declaration),*second=row(fighter,"Twin");
    CHECK(first->article==second->article&&first->owner_node!=second->owner_node);
    CHECK(first->model_root_count==2&&second->model_root_count==2);
    CHECK(!strcmp(first->model_roots[0].name,"First")&&!strcmp(second->model_roots[0].name,"costarring"));
    CHECK(!strcmp(second->model_roots[1].name,"liquid"));
    CHECK(first->model_roots[0].root==second->model_roots[0].root&&first->model_roots[1].root==second->model_roots[1].root);
    CHECK(first->model_roots[0].root!=first->model_roots[1].root);
    CHECK(!first->article->native_model_root_count&&*marker==0xA1235678);
    CHECK(assets[fighter].character.import.arena_used==before); // Native attribute import already cached these exact graphs.
    for(unsigned r=0;r<2;++r){
        const HSD_Joint* root=first->model_roots[r].root;
        CHECK((uintptr_t)root>=(uintptr_t)assets[fighter].character_arena&&
              (uintptr_t)root-(uintptr_t)assets[fighter].character_arena<FT_CHARACTER_IMPORT_ARENA_SIZE);
        HSD_Joint* again=NULL;u32 records=assets[fighter].character.import.record_count;
        CHECK(itArticle_ImportModelRoot(&assets[fighter].character.import,first->article,first->model_roots[r].attribute_offset,&again));
        CHECK(again==root&&records==assets[fighter].character.import.record_count);
        roots[fighter*2+r]=root;
    }
    // Source table names are copied rather than borrowed from parser input.
    memset(source,'?',tree.source_bytes);
    CHECK(!strcmp(first->model_roots[0].name,"First")&&!strcmp(second->model_roots[1].name,"liquid"));
    ++cases;
}
static void matrices(HSD_JObj* graph)
{
    for(HSD_JObj* j=graph;j;j=j->next){HSD_JObjSetupMatrix(j);CHECK(j->scl);++matrix_visits;if(j->child)matrices(j->child);}
}
static void pool_cases(void)
{
    void* start=OSInitAlloc(heap_bytes,heap_bytes+sizeof(heap_bytes),1);
    OSHeapHandle heap=OSCreateHeap(start,heap_bytes+sizeof(heap_bytes));CHECK(heap>=0);
    HSD_SetHeap(heap);OSSetCurrentHeap(heap);HSD_ObjSetHeap(sizeof(heap_bytes),NULL);HSD_ObjInit();HSD_IDSetup();
    ClassInfoInit((HSD_ClassInfo*)&hsdJObj);ClassInfoInit((HSD_ClassInfo*)&hsdDObj);
    ClassInfoInit((HSD_ClassInfo*)&hsdPObj);ClassInfoInit((HSD_ClassInfo*)&hsdMObj);ClassInfoInit((HSD_ClassInfo*)&hsdTObj);
    size_t bytes=0,alignment=0;CHECK(nativeArticleModelPool_Size(roots,4,2,&bytes,&alignment));
    void* backing=_aligned_malloc(bytes,alignment);CHECK(backing);
    CHECK(nativeArticleModelPool_Bind(&pool,backing,bytes,roots,4,2,800));
    HSD_JObj* ordinary=HSD_JObjLoadJoint((HSD_Joint*)roots[3]);CHECK(ordinary);
    CHECK(!nativeArticleModelPool_BeginMatrices(&pool,ordinary));HSD_JObjRemoveAll(ordinary);
    HSD_JObj* graphs[3];
    for(unsigned i=0;i<3;++i){
        s32 available=OSCheckHeap(heap);graphs[i]=nativeArticleModelPool_Construct(&pool,roots[i]);CHECK(graphs[i]);++constructs;
        CHECK(OSCheckHeap(heap)==available&&nativeArticleModelPool_BeginMatrices(&pool,graphs[i]));
        matrices(graphs[i]);CHECK(nativeHsdReservation_End(&pool.reservation)&&OSCheckHeap(heap)==available);
    }
    void* unchanged=malloc(bytes);CHECK(unchanged);memcpy(unchanged,backing,bytes);NativeArticleModelPool metadata=pool;
    CHECK(!nativeArticleModelPool_Construct(&pool,roots[3]));
    CHECK(!memcmp(unchanged,backing,bytes)&&!memcmp(&metadata,&pool,sizeof(pool)));
    for(unsigned i=0;i<3;++i)CHECK(HSD_IDGetData((uintptr_t)roots[i],NULL)==graphs[i]);
    CHECK(!nativeArticleModelPool_Release(&pool));
    HSD_JObjRemoveAll(graphs[0]);
    HSD_JObj* replacement=nativeArticleModelPool_Construct(&pool,roots[3]);CHECK(replacement);++constructs;
    CHECK(nativeArticleModelPool_BeginMatrices(&pool,replacement));matrices(replacement);CHECK(nativeHsdReservation_End(&pool.reservation));
    HSD_JObjRemoveAll(graphs[1]);HSD_JObjRemoveAll(graphs[2]);HSD_JObjRemoveAll(replacement);
    CHECK(nativeArticleModelPool_Release(&pool));_aligned_free(backing);free(unchanged);
    CHECK(!HSD_IDGetAllocData()->used&&!HSD_VecGetAllocData()->used);
    printf("{\"pool_bytes\":%llu,\"root_alternatives\":4,\"live_limit\":2,\"replacement_headroom\":1}\n",(unsigned long long)bytes);++cases;
}
int main(int argc,char** argv)
{
    CHECK(argc==3);
    // Descriptor preparation intentionally precedes ALL HSD heap/class setup.
    for(unsigned f=0;f<2;++f){
        CHECK(nativeFighterProject_Open(argv[f+1],&projects[f]));
        const char* name=f?"It_Kind_CLink_Arrow":"It_Kind_Link_Arrow";unsigned selected=0;
        for(;selected<projects[f].facts.article_count;++selected)if(!strcmp(projects[f].facts.articles[selected].declaration,name))break;
        CHECK(selected<projects[f].facts.article_count&&projects[f].facts.article_count<NATIVE_SOURCE_ARTICLES);
        NativeSourceArticleAsset twin=projects[f].facts.articles[selected];strcpy(twin.declaration,"Twin");
        projects[f].facts.articles[projects[f].facts.article_count++]=twin;
        inputs[f]=(NativeSourceAssetInput){projects[f].fighter_dat,projects[f].fighter_size,projects[f].animation_dat,projects[f].animation_size,projects[f].costume_dat,projects[f].costume_size};
        fresh(f);
    }
    const char* errors[][2]={
        {"    model_root One 36\n    model_root One 40\n","Duplicate"},
        {"    model_root One 37\n","aligned"},
        {"    model_root One 4294967296\n","native32"},
        {"    model_root One 4294967292\n","offset"},
        {"    model_root One 36 extra\n","identifier"},
        {"    model_root bad-name 36\n","identifier"},
        {"    model_animation Other 4294967292\n","Animation"},
        {"    state 0 anim -1:\n        on physics:\n            pass\n    model_root One 36\n","precede"},
    };
    for(unsigned i=0;i<sizeof(errors)/sizeof(*errors);++i)negative(errors[i][0],errors[i][1]);
    char many[2048]={0};for(unsigned i=0;i<9;++i){char line[100];snprintf(line,sizeof(line),"    model_root Root%u 36\n",i);strcat(many,line);}negative(many,"table");
    // Controlled nullable attribute source over actual immutable DAT bytes.
    // The retail Arrow block itself has no NULL words; do not invent that fact.
    fresh(0);const Article* arrow=NULL;
    for(unsigned i=0;i<assets[0].facts.article_count;++i)if(!strcmp(assets[0].facts.articles[i].declaration,"It_Kind_Link_Arrow"))arrow=assets[0].character.fighter_items[assets[0].facts.articles[i].slot];
    CHECK(arrow);u32 null_source=UINT32_MAX;
    for(u32 at=0;at+4<=assets[0].character.dat.data_size;at+=4){u32 target;const char* external;
        if(HSD_DatResolve(&assets[0].character.dat,at,&target,&external)==HSD_DAT_REF_NULL&&HSD_DatNextTarget(&assets[0].character.dat,at)>=at+4){null_source=at;break;}}
    CHECK(null_source!=UINT32_MAX);Article nullable=*arrow;nullable.native_special_attribute_source=null_source;
    for(unsigned i=0;i<assets[0].facts.article_count;++i)if(!strcmp(assets[0].facts.articles[i].declaration,"It_Kind_Link_Arrow"))assets[0].character.fighter_items[assets[0].facts.articles[i].slot]=&nullable;
    parse("fighter:\n    pass\narticle It_Kind_Link_Arrow states 0:\n    model_root Null 0\n");NativeSourceResources resources={0};
    CHECK(!nativeSourceEntityAssets_Prepare(&entities[0],&assets[0],&inputs[0],&resources,&tree,&diagnostic));CHECK(strstr(diagnostic.message,"NULL"));++cases;
    fresh(0);parse("fighter:\n    pass\narticle Missing states 0:\n    model_root First 36\n");
    CHECK(!nativeSourceEntityAssets_Prepare(&entities[0],&assets[0],&inputs[0],&resources,&tree,&diagnostic));CHECK(strstr(diagnostic.message,"no exact"));++cases;
    fresh(0);parse("fighter:\n    pass\narticle It_Kind_Link_Arrow states 0:\n    on created:\n        pass\n");import(0);
    CHECK(!row(0,"It_Kind_Link_Arrow")->model_roots&&!row(0,"It_Kind_Link_Arrow")->model_root_count);++cases;
    fresh(0);source_cases(0);source_cases(1);
    test_native_article_model_roots_import();++cases;
    pool_cases();
    for(unsigned f=0;f<2;++f){nativeSourceEntityAssets_Release(&entities[f]);nativeSourceAssets_Release(&assets[f]);nativeFighterProject_Close(&projects[f]);}
    printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"entity_imports\":%u,\"pool_graphs\":%u,\"matrix_visits\":%u}\n",checks,cases,imports,constructs,matrix_visits);return 0;
}
