// Explicit transition fixture inputs can stage an unpublished candidate in the
// actual runtime slot/heap tables. Getter/heap kernels below are unchanged.
#pragma clang section data = ".data"
#include <melee/ft/native_fighter_runtime.c>
#pragma clang section data = ".mstate$m"
#include <melee/lb/lbheap.c>
#include <melee/it/native_article_model_scene.c>
#pragma clang section data = ".data"
#include <melee/it/native_article_model_scene.h>
#include <melee/it/native_article_model.h>
#include <melee/it/native_item_lifetime.h>
#include <melee/it/itarticle_data.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/object.h>
#include <sysdolphin/baselib/id.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/native_hsd_reservation.h>
#include <melee/ft/native_fighter_project.h>
#include <melee/ft/native_source_motion_policy.h>
#include <stdio.h>
#include <melee/gm/native_rollback_regions.h>
#include <melee/it/native_script_item_actor_internal.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static unsigned checks, graphs, matrices, cases, frames, restores;
enum { HEAP_BYTES = 64 * 1024 * 1024 };
static unsigned char* heap_bytes;
static NativeSourcePackage candidate;
static NativeFighterProject project;
static NativeScriptDiagnostic diagnostic;
static Item* data;
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"scene %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)

static void destroy_item(void* opaque)
{
    Item* item=opaque;
    for(unsigned slot=1;slot<=3;++slot)nativeArticleModel_Destroy(item->entity,slot);
    nativeScriptItem_Detach(item->entity);
}
static void stage_candidate(const char* path)
{
    CHECK(nativeFighterProject_Open(path,&project));
    candidate.address=&candidate;candidate.target=Ft_Kind_CustomFirst;
    candidate.owned_input.assets=(NativeSourceAssetInput){project.fighter_dat,project.fighter_size,
        project.animation_dat,project.animation_size,project.costume_dat,project.costume_size};
    CHECK(nativeSourceAssets_Import(&project.facts,&candidate.owned_input.assets,
        Ft_Kind_CustomFirst,&candidate.assets,&diagnostic));
    CHECK(nativeSourceBank_Allocate(&candidate.compiler,&diagnostic));
    const char* source="fighter:\n    behavior independent\n"
        "article It_Kind_Link_Arrow states 1:\n    state int visits\n"
        "    state 0 anim -1:\n        on animation:\n            item.state.visits = item.state.visits + 1\n            return false\n";
    NativeScriptSourceTree* tree=&candidate.compiler.bank.tree;
    tree->source=source;tree->source_bytes=(uint32_t)strlen(source);
    CHECK(nativeScriptSource_Parse(tree,&diagnostic));
    CHECK(nativeSourceMotionPolicy_Prepare(tree,ftData_MotionStateList,&candidate.assets,&diagnostic));
    CHECK(nativeSourceEntityAssets_Prepare(&candidate.entities,&candidate.assets,
        &candidate.resources,tree,&diagnostic));
    unsigned row_index=UINT32_MAX;
    for(unsigned i=0;i<candidate.entities.count;++i)
        if(!strcmp(candidate.entities.rows[i].declaration,"It_Kind_Link_Arrow"))row_index=i;
    CHECK(row_index!=UINT32_MAX);
    NativeScriptItemAsset* row=&candidate.entities.rows[row_index];
    // A typed, explicitly staged secondary table before program publication.
    // Public model_root metadata is intentionally NOT accepted by this fixture.
    HSD_Joint* root=NULL;
    CHECK(itArticle_ImportModelRoot(&candidate.assets.character.import,row->article,36,&root));
    candidate.entities.model_roots[row_index][0]=(NativeScriptItemModelRootAsset){"ArrowIdle",36,root};
    row->model_roots=candidate.entities.model_roots[row_index];row->model_root_count=1;
    candidate.metadata=(NativeScriptItemMetadataContext){.motions=&candidate.assets.facts.moves,
        .assets=candidate.entities.rows,.asset_count=candidate.entities.count,
        .source_facts=&candidate.assets.facts,.source_assets=&candidate.assets};
    NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),
        .metadata=nativeScriptItemMetadata_Validate,.metadata_context=&candidate.metadata,
        .motion_layout=&candidate.assets.facts.moves};
    const NativeScriptCompileBank* accepted=NULL;
    CHECK(nativeScriptCompile_Build(&candidate.compiler.bank,&request,&accepted,&diagnostic));
    CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,accepted,
        &candidate.assets.facts.moves,&candidate.metadata,&candidate.program,&diagnostic));
    CHECK(nativeSourceItemVisuals_Prepare(&candidate.visuals,&candidate.program.items,&diagnostic));
    candidate.parameters.items=&candidate.metadata;
    candidate.parameters.fighter.bytes=project.fighter_dat;
    candidate.parameters.fighter.fighter_root=candidate.assets.facts.fighter_root;
    candidate.input_memory=malloc(1);CHECK(candidate.input_memory);
    candidate.ready=true;
    CHECK(nativeSourcePackage_Pin(&candidate));
    CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&candidate.program);
    slots[0]=(NativeFighterSlot){.source=&candidate,.fighter_kind=Ft_Kind_CustomFirst,.active=true};
    // Same-kind duplicate slot rows retain the actual getter's first-match
    // behavior; this is not a distinct-kind package deduplication proof.
    slots[1]=slots[0];
    CHECK(nativeFighterRuntime_SourcePackage(Ft_Kind_CustomFirst)==&candidate);
}
static Item_GObj* item(unsigned at)
{
    const NativeScriptItemProgram* program=nativeScriptItem_Program(Ft_Kind_CustomFirst,
        candidate.program.items.programs[0].owner);CHECK(program);
    CHECK(program!=&candidate.program.items.programs[0]);
    CHECK(program->identity!=candidate.program.identity);
    Item_GObj* object=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(object);
    data[at]=(Item){.entity=object,.kind=program->kind,.x1C=(s32)(at+1),.msid=-1,
        .native_article_author_kind=program->author_kind,.native_script_owner=program->owner,
        .xC4_article_data=(Article*)program->article,.xCC_item_attr=program->article->x0_common_attr};
    GObj_InitUserData(object,6,destroy_item,&data[at]);
    CHECK(nativeScriptItem_Attach(object,program));
    CHECK(nativeScriptItem_SelectState(object,0));
    NativeScriptValue result={0};
    CHECK(nativeScriptItem_RunPhase(object,NativeScriptItemPhase_Animation,&result)==NativeScriptVM_Ok);
    CHECK(result.type==NativeScriptType_Bool&&!result.as.u64);
    return object;
}
static void matrix(HSD_JObj* root)
{
    for(HSD_JObj* j=root;j;j=j->next){HSD_JObjSetupMatrix(j);++matrices;if(j->child)matrix(j->child);}
}

typedef struct ItemObservation {
    NativeMatchPoolReference object, graph;
    HSD_JObj* root;
    Vec3 scale, translation, accumulated;
    Mtx transform;
    uint32_t generation, visits, drawables, polygons, tev;
    bool live, has_scale;
} ItemObservation;
typedef struct Observation {
    uint64_t hash;
    ItemObservation items[3];
    HSD_JObj* descriptor_id;
    uint32_t registry_used, bin_used[NATIVE_HSD_RESERVATION_BINS];
    uint32_t bin_free[NATIVE_HSD_RESERVATION_BINS], bin_peak[NATIVE_HSD_RESERVATION_BINS];
} Observation;
static NativeRollbackRegionSet regions;
static Observation baseline, expected[3], repeated;
static Item_GObj* objects[3];
static NativeMatchPoolReference original_second, recycled_second;

static bool captured(const void* pointer,size_t bytes)
{
    for(uint32_t i=0;i<regions.count;++i){
        uintptr_t p=(uintptr_t)pointer,b=(uintptr_t)regions.regions[i].memory;
        if(p>=b&&p-b<=regions.regions[i].size&&bytes<=regions.regions[i].size-(p-b))return true;
    }
    return false;
}
static uint32_t visits(Item_GObj* object)
{
    NativeScriptReceiverView view;NativeScriptValue value={0};Item* payload=object->user_data;
    CHECK(nativeScriptItem_View(object,(uint32_t)payload->x1C,&view));
    CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC,NULL,0,&value,&diagnostic));
    CHECK(value.type==NativeScriptType_I32);return (uint32_t)value.as.i64;
}
static void observe(Observation* output)
{
    memset(output,0,sizeof(*output));
    for(unsigned i=0;i<3;++i){
        ItemObservation* o=&output->items[i];Item_GObj* object=data[i].entity;
        NativeMatchPoolReference reference;
        if(!nativeMatchPool_Reference(&nativeGObjPool,object,&reference)||object->user_data!=&data[i])continue;
        o->live=true;o->object=reference;o->generation=(uint32_t)data[i].x1C;
        o->visits=visits(object);o->root=data[i].native_models[0];
        if(!o->root)continue;
        CHECK(graph_reference(o->root,&o->graph));
        CHECK(captured(o->root,sizeof(*o->root)));
        o->scale=o->root->scale;o->translation=o->root->translate;
        memcpy(o->transform,o->root->mtx,sizeof(o->transform));
        o->has_scale=o->root->scl!=NULL;if(o->has_scale)o->accumulated=*o->root->scl;
        for(HSD_DObj* d=o->root->u.dobj;d;d=d->next){
            ++o->drawables;for(HSD_PObj* p=d->pobj;p;p=p->next)++o->polygons;
            for(HSD_TExpTevDesc* t=d->mobj->tevdesc;t;t=(HSD_TExpTevDesc*)t->desc.next)++o->tev;
        }
    }
    output->descriptor_id=HSD_IDGetData((uintptr_t)candidate.entities.model_roots[0][0].root,NULL);
    for(unsigned i=0;i<MODEL_SLOTS;++i)if(scene.models[i].root)++output->registry_used;
    for(unsigned i=0;i<scene.pool.reservation.count;++i){
        output->bin_used[i]=scene.pool.reservation.bins[i].pool.used;
        output->bin_free[i]=scene.pool.reservation.bins[i].pool.free_count;
        output->bin_peak[i]=scene.pool.reservation.bins[i].pool.peak;
    }
    output->hash=nativeRollbackRegions_Hash(&regions);
}
static void animate(Item_GObj* object)
{
    nativeScriptItem_BeginTick(object);NativeScriptValue value={0};
    CHECK(nativeScriptItem_RunPhase(object,NativeScriptItemPhase_Animation,&value)==NativeScriptVM_Ok);
    CHECK(value.type==NativeScriptType_Bool&&!value.as.u64);
}
static void load_matrix(Item_GObj* object,float factor)
{
    ssize_t before=OSCheckHeap(HSD_GetHeap());
    CHECK(nativeArticleModelScene_Load(object,1,0));++graphs;
    Item* payload=object->user_data;HSD_JObj* root=payload->native_models[0];
    Vec3 scale={factor,factor+1,factor+2};HSD_JObjSetScale(root,&scale);
    CHECK(nativeArticleModelScene_BeginMatrices(object,1));matrix(root);
    CHECK(nativeArticleModelScene_EndMatrices());CHECK(OSCheckHeap(HSD_GetHeap())==before);
}
static void step(unsigned frame)
{
    CHECK(nativeRollbackPages_BeginFrame(frame));
    if(frame==0){load_matrix(objects[0],2);animate(objects[0]);}
    if(frame==1){load_matrix(objects[1],4);load_matrix(objects[0],6);animate(objects[0]);animate(objects[1]);}
    if(frame==2){
        nativeArticleModel_Destroy(objects[0],1);CHECK(!data[0].native_models[0]);
        HSD_GObjFree(objects[1]);CHECK(nativeMatchPool_Resolve(original_second)==NULL);
        objects[2]=item(2);CHECK(nativeMatchPool_Reference(&nativeGObjPool,objects[2],&recycled_second));
        CHECK(recycled_second.slot==original_second.slot&&recycled_second.generation>original_second.generation);
        load_matrix(objects[2],8);animate(objects[0]);
    }
    CHECK(nativeRollbackPages_EndFrame());++frames;
}
int main(int argc,char** argv)
{
    CHECK(argc==2);heap_bytes=VirtualAlloc(NULL,HEAP_BYTES,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);CHECK(heap_bytes);
    void* start=OSInitAlloc(heap_bytes,heap_bytes+HEAP_BYTES,1);
    OSHeapHandle heap=OSCreateHeap(start,heap_bytes+HEAP_BYTES);CHECK(heap>=0);
    HSD_SetHeap(heap);OSSetCurrentHeap(heap);HSD_ObjSetHeap(HEAP_BYTES,NULL);HSD_ObjInit();HSD_IDSetup();
    lbHeap_80431FA0.heap_array[0]=(struct Heap){.id=heap,.type=LbHeapType_Hsd,.status=LbHeapStatus_Create,
        .start=(uintptr_t)heap_bytes,.size=HEAP_BYTES};
    data=lbHeap_80015BD0(0,sizeof(*data)*3);CHECK(data);memset(data,0,sizeof(*data)*3);
    HSD_GObjLibInitDataType config={.p_link_max=63,.gx_link_max=63,.gproc_pri_max=24};HSD_GObj_80391304(&config);
    stage_candidate(argv[1]);CHECK(nativeArticleModelScene_Prepare());CHECK(candidate.leases==2);
    objects[0]=item(0);objects[1]=item(1);
    CHECK(nativeMatchPool_Reference(&nativeGObjPool,objects[1],&original_second));
    CHECK(nativeRollbackRegions_Build(&regions));
    CHECK(captured(&scene,sizeof(scene))&&captured(&nativeGObjPool,sizeof(nativeGObjPool)));
    CHECK(captured(nativeScriptItemActors,sizeof(nativeScriptItemActors)));
    CHECK(captured(scene.pool.backing,scene.pool.bytes)&&captured(data,sizeof(*data)*3));
    CHECK(!captured(&candidate,sizeof(candidate))&&!captured(&checks,sizeof(checks)));
    CHECK(captured(&hsdJObj,sizeof(hsdJObj))&&captured(HSD_IDGetAllocData(),sizeof(*HSD_IDGetAllocData())));
    observe(&baseline);
    CHECK(nativeRollbackPages_Init(regions.regions,regions.count,regions.exact_regions,regions.exact_count,4,512));
    CHECK(!nativeArticleModelScene_Retire()&&candidate.leases==2);++cases;
    for(unsigned i=0;i<3;++i){step(i);observe(&expected[i]);}
    CHECK(!nativeArticleModelScene_Retire()&&candidate.leases==2);
    CHECK(nativeRollbackPages_ToFrame(1,3));++restores;observe(&repeated);
    CHECK(!memcmp(&repeated,&expected[0],sizeof(repeated)));
    CHECK(nativeMatchPool_Resolve(original_second)==objects[1]);
    CHECK(nativeMatchPool_Resolve(recycled_second)==NULL);++cases;
    for(unsigned i=1;i<3;++i){step(i);observe(&repeated);CHECK(!memcmp(&repeated,&expected[i],sizeof(repeated)));}++cases;
    CHECK(nativeRollbackPages_ToFrame(0,3));++restores;observe(&repeated);
    CHECK(!memcmp(&repeated,&baseline,sizeof(repeated)));
    for(unsigned i=0;i<3;++i){step(i);observe(&repeated);CHECK(!memcmp(&repeated,&expected[i],sizeof(repeated)));}++cases;
    CHECK(nativeRollbackPages_IsInitialized()&&!nativeArticleModelScene_Retire());
    nativeRollbackPages_Shutdown();CHECK(!nativeRollbackPages_IsInitialized());
    CHECK(nativeArticleModelScene_Retire()&&candidate.leases==1);++cases;
    HSD_GObjFree(objects[0]);HSD_GObjFree(objects[2]);CHECK(nativeSourcePackage_Unpin(&candidate));
    slots[0].active=slots[1].active=false;CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));
    NativeScriptFighterProgram* empty=calloc(1,sizeof(*empty));CHECK(empty);
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,empty);free(empty);
    CHECK(nativeSourcePackage_Release(&candidate));nativeFighterProject_Close(&project);CHECK(nativeGObjPool_RetireScene());
    lbHeap_80015CA8(0,data);OSDestroyHeap(heap);CHECK(VirtualFree(heap_bytes,0,MEM_RELEASE));
    printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"frames\":%u,\"restores\":%u,\"graphs\":%u,\"matrix_visits\":%u,\"regions\":%u,\"exact_regions\":%u,\"tracked_bytes\":%llu}\n",
        checks,cases,frames,restores,graphs,matrices,regions.count,regions.exact_count,(unsigned long long)regions.tracked_bytes);
    return 0;
}
