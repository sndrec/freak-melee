// Explicit transition fixture inputs can stage an unpublished candidate in the
// actual runtime slot/heap tables. Getter/heap kernels below are unchanged.
#include <melee/ft/native_fighter_runtime.c>
#include <melee/lb/lbheap.c>
#include <melee/it/native_article_model_scene.h>
#include <melee/it/native_article_model.h>
#include <melee/it/native_article_model_demand.h>
#include <melee/it/native_item_lifetime.h>
#include <melee/it/itarticle_data.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/object.h>
#include <sysdolphin/baselib/id.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/native_hsd_reservation.h>
#include <melee/ft/native_fighter_project.h>
#include <melee/ft/native_source_motion_policy.h>
#include <stdio.h>

static unsigned checks, graphs, matrices, cases;
static unsigned char heap_bytes[64 * 1024 * 1024] ATTRIBUTE_ALIGN(64);
static NativeSourcePackage candidate;
static NativeFighterProject project;
static NativeScriptDiagnostic diagnostic;
static Item data[NATIVE_ARTICLE_MODEL_SCENE_CAPACITY/3+4];
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"scene %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)

static void destroy_item(void* opaque)
{
    Item* item=opaque;
    for(unsigned slot=1;slot<=3;++slot)nativeArticleModel_Destroy(item->entity,slot);
    nativeScriptItem_Detach(item->entity);
}
static void stage_candidate(const char* path,const char* declaration)
{
    CHECK(nativeFighterProject_Open(path,&project));
    candidate.address=&candidate;candidate.target=Ft_Kind_CustomFirst;
    candidate.owned_input.assets=(NativeSourceAssetInput){project.fighter_dat,project.fighter_size,
        project.animation_dat,project.animation_size,project.costume_dat,project.costume_size};
    CHECK(nativeSourceAssets_Import(&project.facts,&candidate.owned_input.assets,
        Ft_Kind_CustomFirst,&candidate.assets,&diagnostic));
    CHECK(nativeSourceBank_Allocate(&candidate.compiler,&diagnostic));
    bool named_root=!strcmp(declaration,"It_Kind_Link_Arrow");
    static char source[1024];
    int length=snprintf(source,sizeof(source),"fighter:\n    behavior independent\n"
        "article %s states 1:\n%s    state int visits\n"
        "    state bool cloned\n    on signal CloneModel:\n        item.state.cloned = item.clone_model(3)\n"
        "    state 0 anim -1:\n        on animation:\n            item.state.visits = item.state.visits + 1\n            return false\n",
        declaration,named_root?"    model_root ArrowIdle 36\n":"");
    CHECK(length>0&&(size_t)length<sizeof(source));
    NativeScriptSourceTree* tree=&candidate.compiler.bank.tree;
    tree->source=source;tree->source_bytes=(uint32_t)strlen(source);
    CHECK(nativeScriptSource_Parse(tree,&diagnostic));
    CHECK(nativeSourceMotionPolicy_Prepare(tree,ftData_MotionStateList,&candidate.assets,&diagnostic));
    CHECK(nativeSourceEntityAssets_Prepare(&candidate.entities,&candidate.assets,
        &candidate.resources,tree,&diagnostic));
    unsigned row_index=UINT32_MAX;
    for(unsigned i=0;i<candidate.entities.count;++i)
        if(!strcmp(candidate.entities.rows[i].declaration,declaration))row_index=i;
    CHECK(row_index!=UINT32_MAX);
    NativeScriptItemAsset* row=&candidate.entities.rows[row_index];
    // Current source metadata imports the actual owned root before publication.
    if(named_root) {
        CHECK(row->model_root_count==1&&row->model_roots&&row->model_roots[0].root);
        CHECK(!strcmp(row->model_roots[0].name,"ArrowIdle")&&row->model_roots[0].attribute_offset==36);
    } else CHECK(!row->model_root_count);
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
    CHECK(candidate.program.items.programs[0].clones_primary_model);
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
#include "clone_cases.h"


#include <melee/gm/native_match_control_history.h>
#include <sysdolphin/baselib/native_hsd_arena.h>
static void copy_model_history(Item_GObj* object)
{
    static NativeMatchArena arena;
    static NativeMatchArenaFrame frame,hsd_frame;
    static uint8_t arena_bytes[64] ATTRIBUTE_ALIGN(64);
    static uint8_t saved_bytes[64*1024*1024],hsd_bytes[32*1024*1024];
    NativeArticleModelSceneSlot retained;
    CHECK(nativeArticleModelScene_CaptureSlot(object,1,&retained));
    HSD_JObj expected=*retained.root;
    CHECK(nativeMatchArena_Begin(&arena,arena_bytes,sizeof(arena_bytes),nativeGObjPool.lifetime));
    CHECK(nativeArticleModelScene_AdmitHistory(&arena));
    CHECK(nativeHsdReservation_AdmitHistory(&arena));
    CHECK(nativeID_AdmitHistory(&arena));
    CHECK(nativeMatchArena_AddRoot(&arena,data,sizeof(data)));
    CHECK(nativeHsdArena_AdmitHistory());
    CHECK(nativeMatchArena_BindFrame(&nativeHsdArena,&hsd_frame,hsd_bytes,sizeof(hsd_bytes)));
    CHECK(nativeMatchArena_BindFrame(&arena,&frame,saved_bytes,sizeof(saved_bytes)));
    CHECK(nativeMatchArena_Save(&hsd_frame) && nativeMatchArena_Save(&frame));
    HSD_JObj* future=NULL;
    for(unsigned repeat=0;repeat<2;++repeat) {
        nativeArticleModel_Destroy(object,1);
        CHECK(!nativeArticleModelScene_SlotCurrent(&retained));
        CHECK(nativeArticleModelScene_Load(object,1,0));
        HSD_JObj* next=((Item*)object->user_data)->native_models[0];CHECK(next);
        if(future)CHECK(next==future);else future=next;
        CHECK(nativeMatchArena_Load(&hsd_frame) && nativeMatchArena_Load(&frame));
        CHECK(nativeArticleModelScene_SlotCurrent(&retained));
        CHECK(((Item*)object->user_data)->native_models[0]==retained.root);
        CHECK(!memcmp(retained.root,&expected,sizeof(expected)));
        CHECK(HSD_IDGetData(retained.root->id,NULL)==retained.root);
    }
    arena.lifetime=0;
    ++cases;
}

int main(int argc,char** argv)
{
    CHECK(argc==2||argc==3);
    void* start=OSInitAlloc(heap_bytes,heap_bytes+sizeof(heap_bytes),1);
    OSHeapHandle heap=OSCreateHeap(start,heap_bytes+sizeof(heap_bytes));CHECK(heap>=0);
    HSD_SetHeap(heap);OSSetCurrentHeap(heap);HSD_ObjSetHeap(sizeof(heap_bytes),NULL);HSD_ObjInit();HSD_IDSetup();
    lbHeap_80431FA0.heap_array[0]=(struct Heap){.id=heap,.type=LbHeapType_Hsd,.status=LbHeapStatus_Create};
    HSD_GObjLibInitDataType config={.p_link_max=63,.gx_link_max=63,.gproc_pri_max=24};
    HSD_GObj_80391304(&config);
    ssize_t vacant=OSCheckHeap(heap);
    CHECK(nativeArticleModelScene_Prepare());CHECK(OSCheckHeap(heap)==vacant);
    CHECK(nativeArticleModelScene_Retire());++cases;
    stage_candidate(argv[1],argc==3?argv[2]:"It_Kind_Link_Arrow");
    if(!nativeArticleModelScene_Prepare()) {
        const NativeScriptItemProgram* p=&candidate.program.items.programs[0];
        const HSD_Joint* roots[2]={p->article->x10_modelDesc->x0_joint,
            p->model_root_count?p->model_roots[0].root:NULL};
        for(unsigned i=0;i<(p->model_root_count?2u:1u);++i) {
            NativeArticleModelDemand demand={0};bool census=nativeArticleModel_Census(roots[i],&demand);
            fprintf(stderr,"model census %u: success=%u uncovered=%x joints=%u materials=%u ready=%u\n",
                i,census,demand.uncovered,demand.joints,demand.materials,nativeArticleModel_EnvironmentReady(&demand));
        }
        CHECK(false);
    }
    CHECK(candidate.leases==2);
    if(argc==3) {
        clone_cases(item(0),heap);
        CHECK(nativeArticleModelScene_Retire()&&candidate.leases==1);
        goto cleanup;
    }
    CHECK(!nativeArticleModelScene_Prepare());
    Item_GObj* first=item(0);Item_GObj* second=item(1);
    clone_cases(item(2),heap);
    capacity_cases(heap);
    ssize_t before=OSCheckHeap(heap);
    CHECK(!nativeArticleModelScene_Load(first,0,0));CHECK(!nativeArticleModelScene_Load(first,4,0));
    CHECK(!nativeArticleModelScene_Load(first,1,1));CHECK(OSCheckHeap(heap)==before);++cases;
    CHECK(nativeArticleModelScene_Load(first,1,0));++graphs;
    HSD_JObj* old=data[0].native_models[0];CHECK(old&&HSD_IDGetData(old->id,NULL)==old);
    CHECK(nativeArticleModelScene_Load(second,1,0));++graphs;
    CHECK(data[1].native_models[0]!=old);
    CHECK(nativeArticleModelScene_Load(first,1,0));++graphs;
    CHECK(data[0].native_models[0]!=old&&HSD_IDGetData(data[0].native_models[0]->id,NULL)==data[0].native_models[0]);
    CHECK(OSCheckHeap(heap)==before);++cases;
    HSD_JObj* root=data[0].native_models[0];Vec3 scaling={2,3,4};HSD_JObjSetScale(root,&scaling);
    CHECK(nativeArticleModelScene_BeginMatrices(first,1));CHECK(!nativeArticleModelScene_BeginMatrices(second,1));
    CHECK(!nativeArticleModelScene_Revoke(first,1,root));CHECK(!nativeArticleModelScene_Retire());
    matrix(root);CHECK(nativeArticleModelScene_EndMatrices());CHECK(!nativeArticleModelScene_EndMatrices());
    CHECK(OSCheckHeap(heap)==before);++cases;
    copy_model_history(first);
    data[0].native_removal_in_progress=true;
    CHECK(nativeArticleModelScene_BeginMatrices(first,1));matrix(root);
    CHECK(nativeArticleModelScene_EndMatrices());
    CHECK(nativeArticleModelScene_BeginMatrices(first,1));
    data[0].native_removal_in_progress=false;
    CHECK(!nativeArticleModelScene_EndMatrices());CHECK(!nativeHsdReservation_Active());
    CHECK(nativeArticleModelScene_BeginMatrices(first,1));CHECK(nativeArticleModelScene_EndMatrices());++cases;
    CHECK(!nativeArticleModelScene_Revoke(second,1,root));
    data[1].native_models[1]=root;CHECK(!nativeArticleModelScene_Load(second,2,0));data[1].native_models[1]=NULL;
    uint64_t identity=candidate.program.items.programs[0].identity;
    candidate.program.items.programs[0].identity^=1;CHECK(!nativeArticleModelScene_Load(first,2,0));
    candidate.program.items.programs[0].identity=identity;
    CHECK(nativeArticleModelScene_Load(first,2,0));++graphs;
    nativeArticleModel_Destroy(first,1);CHECK(!data[0].native_models[0]);
    CHECK(!nativeArticleModelScene_BeginMatrices(first,1));++cases;
    HSD_GObjFree(second);CHECK(!data[1].native_models[0]);
    CHECK(nativeArticleModelScene_Retire());CHECK(!data[0].native_models[1]&&candidate.leases==1);
    CHECK(!nativeArticleModelScene_Load(first,3,0));CHECK(nativeArticleModelScene_Retire());
    HSD_GObjFree(first);++cases;
cleanup:
    CHECK(nativeSourcePackage_Unpin(&candidate));
    slots[0].active=slots[1].active=false;
    CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));
    NativeScriptFighterProgram* empty=calloc(1,sizeof(*empty));CHECK(empty);
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,empty);free(empty);
    CHECK(nativeSourcePackage_Release(&candidate));nativeFighterProject_Close(&project);
    CHECK(nativeGObjPool_RetireScene());
    printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"graphs\":%u,\"matrix_visits\":%u}\n",checks,cases,graphs,matrices);
    return 0;
}
