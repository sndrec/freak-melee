// Explicit transition fixture inputs can stage an unpublished candidate in the
// actual runtime slot/heap tables. Getter/heap kernels below are unchanged.
#include <melee/ft/native_fighter_runtime.c>
#include <melee/lb/lbheap.c>
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
#include <sysdolphin/baselib/gobjobject.h>
#include <melee/lb/native_model_joint.h>
#include <melee/lb/lb_00B0.h>
#include <math.h>

static unsigned checks, graphs, matrices, cases;
static unsigned char heap_bytes[64 * 1024 * 1024] ATTRIBUTE_ALIGN(64);
static NativeSourcePackage candidate;
static NativeFighterProject project;
static NativeScriptDiagnostic diagnostic;
static Item data[3];
static const char* declaration;
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
    const char* source_template=
        "fighter:\n    behavior independent\n"
        "article It_Kind_Link_Arrow states 5:\n"
        "    model_root ArrowIdle 36\n    model_root ArrowFire 40\n"
        "    state float observed\n    state float wx\n    state float wy\n    state float wz\n"
        "    state bool exists\n    state bool loaded\n"
        "    state 0 anim -1:\n        on physics:\n"
        "            item.state.exists = item.joint(1, \"\").exists\n"
        "            item.state.observed = item.joint(1, \"\").quaternion.w\n"
        "            item.state.loaded = item.load_model(1, ArrowIdle)\n"
        "            item.state.loaded = item.load_model(2, ArrowFire)\n"
        "            item.state.loaded = item.load_model(3, ArrowIdle)\n"
        "            item.set_model_draw_enabled(2, false)\n"
        "    state 1 anim -1:\n        on physics:\n"
        "            item.joint(0, \"\").translation.x = 7.0\n"
        "            item.joint(1, \"\").translation.x = 8.0\n"
        "            item.joint(2, \"\").scale.y = 3.0\n"
        "            item.joint(3, \"\").local_visible = false\n"
        "            item.joint(1, \"\").quaternion.x = 0.25\n"
        "            item.joint(1, \"\").quaternion.y = 0.5\n"
        "            item.joint(1, \"\").quaternion.z = 0.75\n"
        "            item.joint(1, \"\").quaternion.w = 1.0\n"
        "            item.state.observed = item.joint(1, \"0/255\").scale.z\n"
        "            item.state.exists = item.joint(1, \"0/255\").exists\n"
        "            item.joint(1, \"0/255\").translation.z = 99.0\n"
        "    state 2 anim -1:\n        on physics:\n"
        "            item.set_joint_transform(1, \"\", 2.0, 3.0, 4.0, 0.0, 0.0, 0.0, 1.0, 2.0, 3.0, 4.0, false)\n"
        "            item.joint_world_position(1, \"\", item.state.wx, item.state.wy, item.state.wz)\n"
        "    state 3 anim -1:\n        on physics:\n"
        "            item.set_joint_transform(1, \"\", 5.0, 6.0, 7.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, true)\n"
        "            item.set_joint_transform(2, \"\", 1.0 / 0.0, 8.0, 9.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, true)\n"
        "            item.joint_world_position(1, \"0/255\", item.state.wx, item.state.wy, item.state.wz)\n"
        "    state 4 anim -1:\n        on physics:\n"
        "            item.destroy_model(1)\n            item.destroy_model(2)\n            item.destroy_model(3)\n"
        "            item.state.exists = item.joint(1, \"\").exists\n"
        "            item.state.observed = item.joint(1, \"\").quaternion.w\n"
        "        on collision:\n"
        "            item.joint_world_position(1, \"\", item.state.wx, item.state.wy, item.state.wz)\n";
    static char source[8192];strcpy(source,source_template);
    char* name=strstr(source,"It_Kind_Link_Arrow");CHECK(name);
    size_t before_name=(size_t)(name-source),old_length=strlen("It_Kind_Link_Arrow");
    memmove(source+before_name+strlen(declaration),name+old_length,strlen(name+old_length)+1);
    memcpy(name,declaration,strlen(declaration));
    NativeScriptSourceTree* tree=&candidate.compiler.bank.tree;
    tree->source=source;tree->source_bytes=(uint32_t)strlen(source);
    CHECK(nativeScriptSource_Parse(tree,&diagnostic));
    CHECK(nativeSourceMotionPolicy_Prepare(tree,ftData_MotionStateList,&candidate.assets,&diagnostic));
    CHECK(nativeSourceEntityAssets_Prepare(&candidate.entities,&candidate.assets,
        &candidate.resources,tree,&diagnostic));
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
    HSD_JObj* primary=HSD_JObjLoadJoint(program->article->x10_modelDesc->x0_joint);CHECK(primary);
    HSD_GObjObject_80390A70(object,HSD_GObj_JObjKind,primary);

    return object;
}
static NativeScriptValue field(Item_GObj* object,const char* name)
{
    uint32_t generation;NativeScriptReceiverView view;
    CHECK(nativeCallbackSource_Snapshot(object,NativeCallbackSource_Item,&generation));
    CHECK(nativeScriptItem_View(object,generation,&view));
    const NativeScriptProgramIndex* index=view.program;
    for(unsigned i=0;i<index->value_count;++i)if(index->values[i].owner==view.storage->owner&&
        nativeScriptSource_SpanEquals(index->tree,index->values[i].name,name)){
        NativeScriptValue value;CHECK(nativeScriptStorage_Read(view.storage,UINT32_C(0x10000000)|i,NULL,0,&value,&diagnostic));return value;
    }
    CHECK(false);return (NativeScriptValue){0};
}
static void compiler_negatives(void)
{
    static const char* const bodies[]={
        "            item.load_model(0, ArrowIdle)\n",
        "            item.load_model(4, ArrowIdle)\n",
        "            item.load_model(1, MissingRoot)\n",
        "            item.load_model(1, 0)\n",
        "            item.joint(4, \"\").translation.x = 1.0\n",
        "            item.joint(1, \"bad\").visible = false\n",
        "            item.joint(1, \"\").quaternion = 1.0\n",
        "            item.set_joint_transform(1, \"\", 1.0, 2.0, 3.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1)\n"
    };
    static const char* const expected[]={"literal","literal","name or context",
        "same-owner declaration","Item joint needs","Item joint needs","not writable","argument type"};
    NativeSourceBank scratch={0};CHECK(nativeSourceBank_Allocate(&scratch,&diagnostic));
    uint64_t identity=candidate.program.identity;
    bool matched=true;
    for(unsigned i=0;i<sizeof(bodies)/sizeof(*bodies);++i){
        char source[2048];snprintf(source,sizeof(source),
            "fighter:\n    behavior independent\narticle %s states 5:\n"
            "    model_root ArrowIdle 36\n    model_root ArrowFire 40\n"
            "    state 0 anim -1:\n        on physics:\n%s",declaration,bodies[i]);
        NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),
            .metadata=nativeScriptItemMetadata_Validate,.metadata_context=&candidate.metadata,
            .motion_layout=&candidate.assets.facts.moves};
        const NativeScriptCompileBank* accepted=&candidate.compiler.bank;
        memset(&diagnostic,0,sizeof(diagnostic));
        CHECK(!nativeScriptCompile_Build(&scratch.bank,&request,&accepted,&diagnostic));
        CHECK(diagnostic.message[0]);
        if(!strstr(diagnostic.message,expected[i]))fprintf(stderr,"negative %u: %s\n",i,diagnostic.message);
        matched=matched&&strstr(diagnostic.message,expected[i]);CHECK(accepted==&candidate.compiler.bank);
        CHECK(candidate.compiler.bank.valid&&candidate.program.identity==identity);
    }
    CHECK(matched);
    static const char* const declarations[]={
        "    model_root ArrowIdle 36\n    model_root ArrowIdle 40\n",
        "    model_root ArrowIdle 37\n    model_root ArrowFire 40\n",
        "    model_root state 36\n    model_root ArrowFire 40\n",
        "    model_root and 36\n    model_root ArrowFire 40\n",
        "    model_root or 36\n    model_root ArrowFire 40\n",
        "    model_root not 36\n    model_root ArrowFire 40\n",
        "    model_root ArrowIdle 36\n    model_root ArrowFire 4294967296\n"
    };
    for(unsigned i=0;i<sizeof(declarations)/sizeof(*declarations);++i){
        char source[2048];snprintf(source,sizeof(source),
            "fighter:\n    behavior independent\narticle %s states 5:\n%s"
            "    state 0 anim -1:\n        on physics:\n            pass\n",declaration,declarations[i]);
        NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),
            .metadata=nativeScriptItemMetadata_Validate,.metadata_context=&candidate.metadata,
            .motion_layout=&candidate.assets.facts.moves};
        const NativeScriptCompileBank* accepted=&candidate.compiler.bank;
        memset(&diagnostic,0,sizeof(diagnostic));
        CHECK(!nativeScriptCompile_Build(&scratch.bank,&request,&accepted,&diagnostic));
        CHECK(diagnostic.message[0]);CHECK(accepted==&candidate.compiler.bank);
        CHECK(candidate.compiler.bank.valid&&candidate.program.identity==identity);
    }
    nativeSourceBank_Release(&scratch);++cases;
}
static void run(Item_GObj* object,int state)
{
    CHECK(nativeScriptItem_SelectState(object,state));NativeScriptValue result={.type=NativeScriptType_I32,.as.i64=73};
    CHECK(nativeScriptItem_RunPhase(object,NativeScriptItemPhase_Physics,&result)==NativeScriptVM_Ok);
    CHECK(result.type==NativeScriptType_Void);++cases;
}
int main(int argc,char** argv)
{
    CHECK(argc==3);declaration=argv[2];
    void* start=OSInitAlloc(heap_bytes,heap_bytes+sizeof(heap_bytes),1);
    OSHeapHandle heap=OSCreateHeap(start,heap_bytes+sizeof(heap_bytes));CHECK(heap>=0);
    HSD_SetHeap(heap);OSSetCurrentHeap(heap);HSD_ObjSetHeap(sizeof(heap_bytes),NULL);HSD_ObjInit();HSD_IDSetup();
    lbHeap_80431FA0.heap_array[0]=(struct Heap){.id=heap,.type=LbHeapType_Hsd,.status=LbHeapStatus_Create};
    HSD_GObjLibInitDataType config={.p_link_max=63,.gx_link_max=63,.gproc_pri_max=24};
    HSD_GObj_80391304(&config);
    ssize_t vacant=OSCheckHeap(heap);
    CHECK(nativeArticleModelScene_Prepare());CHECK(OSCheckHeap(heap)==vacant);
    CHECK(nativeArticleModelScene_Retire());++cases;
    stage_candidate(argv[1]);
    compiler_negatives();
    CHECK(nativeArticleModelScene_Prepare());CHECK(candidate.leases==2);
    CHECK(!nativeArticleModelScene_Prepare());
    Item_GObj* first=item(0);Item_GObj* second=item(1);
    ssize_t before=OSCheckHeap(heap);
    run(first,0);graphs+=3;
    CHECK(!field(first,"exists").as.u64);CHECK(field(first,"observed").as.f32==1.0f);
    CHECK(field(first,"loaded").as.u64);CHECK(data[0].native_model_draw_disabled_mask==2);
    for(unsigned i=0;i<3;++i)CHECK(data[0].native_models[i]);
    CHECK(data[0].native_models[0]!=data[0].native_models[2]);CHECK(OSCheckHeap(heap)==before);
    run(second,0);graphs+=3;CHECK(data[0].native_models[0]!=data[1].native_models[0]);
    Item_GObj* borrowed=item(2);
    data[2].native_models[0]=data[0].native_models[0];
    CHECK(nativeScriptItem_SelectState(borrowed,4));
    NativeScriptValue failed_result={.type=NativeScriptType_I32,.as.i64=73};
    CHECK(nativeScriptItem_RunPhase(borrowed,NativeScriptItemPhase_Collision,&failed_result)==NativeScriptVM_Error);
    CHECK(failed_result.type==NativeScriptType_I32&&failed_result.as.i64==73);
    CHECK(nativeScriptItem_Status(borrowed)==NativeScriptVM_Error);
    CHECK(!nativeHsdReservation_Active());
    data[2].native_models[0]=NULL;HSD_GObjFree(borrowed);++cases;
    ssize_t before_matrices=OSCheckHeap(heap);
    run(first,1);
    CHECK(((HSD_JObj*)first->hsd_obj)->translate.x==7.0f);CHECK(data[0].native_models[0]->translate.x==8.0f);
    CHECK(data[0].native_models[1]->scale.y==3.0f);CHECK(data[0].native_models[2]->flags&JOBJ_HIDDEN);
    Quaternion rotation;HSD_JObjGetRotation(data[0].native_models[0],&rotation);
    CHECK(rotation.x==0.25f&&rotation.y==0.5f&&rotation.z==0.75f&&rotation.w==1.0f);
    CHECK(field(first,"observed").as.f32==1.0f&&!field(first,"exists").as.u64);
    HSD_JObj* reference=data[1].native_models[0];
    Vec3 position={2,3,4},scale={2,3,4};Quaternion identity_rotation={0,0,0,1};
    HSD_JObjSetTranslate(reference,&position);HSD_JObjSetRotation(reference,&identity_rotation);
    HSD_JObjSetScale(reference,&scale);
    run(first,2);++matrices;
    HSD_JObj* root=data[0].native_models[0];
    CHECK(!memcmp(&root->translate,&reference->translate,sizeof(Vec3)));
    CHECK(!memcmp(&root->rotate,&reference->rotate,sizeof(Quaternion)));
    CHECK(!memcmp(&root->scale,&reference->scale,sizeof(Vec3)));
    CHECK(root->translate.x==2&&root->translate.y==3&&root->translate.z==4);
    CHECK(root->scale.x==2&&root->scale.y==3&&root->scale.z==4);
    CHECK(field(first,"wx").as.f32==2&&field(first,"wy").as.f32==3&&field(first,"wz").as.f32==4);
    CHECK(!nativeHsdReservation_Active());CHECK(OSCheckHeap(heap)==before_matrices);
    Vec3 previous=data[0].native_models[1]->translate;
    position=(Vec3){5,6,7};scale=(Vec3){1,1,1};
    HSD_JObjSetTranslateWithMtxDirty(reference,&position);
    HSD_JObjSetRotationWithMtxDirty(reference,&identity_rotation);
    HSD_JObjSetScaleWithMtxDirty(reference,&scale);
    run(first,3);
    CHECK(!memcmp(&root->translate,&reference->translate,sizeof(Vec3)));
    CHECK(!memcmp(&root->rotate,&reference->rotate,sizeof(Quaternion)));
    CHECK(!memcmp(&root->scale,&reference->scale,sizeof(Vec3)));
    CHECK(root->translate.x==5&&root->translate.y==6&&root->translate.z==7);
    CHECK(!memcmp(&previous,&data[0].native_models[1]->translate,sizeof(previous)));
    CHECK(field(first,"wx").as.f32==0&&field(first,"wy").as.f32==0&&field(first,"wz").as.f32==0);
    NativeArticleModelSceneSlot retained;
    CHECK(nativeArticleModelScene_CaptureSlot(first,1,&retained));
    run(first,4);CHECK(!nativeArticleModelScene_SlotCurrent(&retained));
    CHECK(!field(first,"exists").as.u64&&field(first,"observed").as.f32==1);
    for(unsigned i=0;i<3;++i)CHECK(!data[0].native_models[i]);
    run(first,0);graphs+=3;CHECK(!nativeArticleModelScene_SlotCurrent(&retained));
    HSD_GObjFree(second);
    CHECK(nativeArticleModelScene_Retire());CHECK(candidate.leases==1);
    for(unsigned i=0;i<3;++i)CHECK(!data[0].native_models[i]);
    HSD_GObjFree(first);CHECK(nativeSourcePackage_Unpin(&candidate));
    slots[0].active=slots[1].active=false;
    CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));
    NativeScriptFighterProgram* empty=calloc(1,sizeof(*empty));CHECK(empty);
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,empty);free(empty);
    CHECK(nativeSourcePackage_Release(&candidate));nativeFighterProject_Close(&project);
    CHECK(nativeGObjPool_RetireScene());
    printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"graphs\":%u,\"matrix_visits\":%u}\n",checks,cases,graphs,matrices);
    return 0;
}
