#include <melee/it/native_article_resource_snapshot.h>
#include <sysdolphin/baselib/native_animation_pool.h>
#include <sysdolphin/baselib/native_objalloc_snapshot.h>
#include <melee/it/native_item_link_pool.h>
#include <melee/gm/native_match_snapshot_pool.h>

static unsigned resource_registry_checks,resource_registry_refusals;
static size_t resource_registry_bytes;
typedef struct RegistryComponent {
    bool(*save)(void*,size_t,size_t*);
    bool(*validate)(const void*,size_t);
    bool(*load)(const void*,size_t);
    uint8_t* bytes;
    uint8_t* after;
    size_t written;
} RegistryComponent;

static void resource_registry_snapshot(void)
{
    unsigned start=checks;
    HSD_GObj* previous_context=HSD_GObj_804D7814;
    HSD_GObj_804D7814=fighter_object;
    HSD_GObj_RenderCallbackDepth=1;
    CHECK(!nativeArticleResource_SnapshotBytes());
    HSD_GObj_RenderCallbackDepth=0;
    collection(objects[0],2);load(objects[0],0);load(objects[0],1);
    ResourceCapture before;CHECK(nativeArticleResource_Capture(objects[0],0,&before));
    // Actual animation application/interpretation, not hand-built decoder bytes.
    if(animated_graphs){
        CHECK(nativeArticleResource_AddAnimation(objects[0],0,NULL,0,0));
        CHECK(nativeArticleResource_RequestAnimation(objects[0],0,NULL,0,3));
        CHECK(nativeArticleResource_AdvanceAnimation(objects[0],0,NULL,0));
    }
    CHECK(nativeArticleResource_BindExternalPart(objects[0],0,(unsigned)before.collection->external_part));
    CHECK(fighter_data->parts[before.collection->external_part].joint==before.model);
    fighter_data->x914[0].jobj=before.model;fighter_data->x914[0].state=HitCapsule_Enabled;
    ResourceCollection expected=*before.collection;
    RegistryComponent components[]={
        {.save=nativeArticleResource_Save,.validate=nativeArticleResource_ValidateSnapshot,.load=nativeArticleResource_Load},
        {.save=nativeGObjPool_Save,.validate=nativeGObjPool_ValidateSnapshot,.load=nativeGObjPool_Load},
        {.save=nativeItemLinkPool_Save,.validate=nativeItemLinkPool_ValidateSnapshot,.load=nativeItemLinkPool_Load},
        {.save=nativeAnimationPool_Save,.validate=nativeAnimationPool_ValidateSnapshot,.load=nativeAnimationPool_Load},
        {.save=nativeHsdReservation_Save,.validate=nativeHsdReservation_ValidateSnapshot,.load=nativeHsdReservation_Load},
        {.save=nativeClass_Save,.validate=nativeClass_ValidateSnapshot,.load=nativeClass_Load},
        {.save=nativeObjAlloc_Save,.validate=nativeObjAlloc_ValidateSnapshot,.load=nativeObjAlloc_Load},
        {.save=nativeFighterPool_Save,.validate=nativeFighterPool_ValidateSnapshot,.load=nativeFighterPool_Load},
        {.save=nativeScriptFighter_Save,.validate=nativeScriptFighter_ValidateSnapshot,.load=nativeScriptFighter_Load},
        {.save=nativeScriptItem_Save,.validate=nativeScriptItem_ValidateSnapshot,.load=nativeScriptItem_Load},
        {.save=nativeID_Save,.validate=nativeID_ValidateSnapshot,.load=nativeID_Load}
    };
    enum { BUFFER_BYTES=2*1024*1024 };
    for(unsigned i=0;i<sizeof(components)/sizeof(components[0]);++i){
        RegistryComponent* c=&components[i];c->bytes=malloc(BUFFER_BYTES);c->after=malloc(BUFFER_BYTES);
        CHECK(c->bytes&&c->after);CHECK(c->save(c->bytes,BUFFER_BYTES,&c->written));
    }
    resource_registry_bytes=components[0].written;
    OSHeapHandle heap=HSD_GetHeap();u32 heap_bytes=OSHeapSnapshotBytes(heap),heap_written=0;
    uint8_t* heap_before=malloc(heap_bytes),*heap_after=malloc(heap_bytes);CHECK(heap_before&&heap_after);
    CHECK(OSSaveHeapSnapshot(heap,heap_before,heap_bytes,&heap_written)&&heap_written==heap_bytes);
    uint8_t* malformed=malloc(resource_registry_bytes+1);CHECK(malformed);
    for(unsigned n=0;n<12;++n){
        memcpy(malformed,components[0].bytes,resource_registry_bytes);
        NativeArticleResourceSnapshotHeader h;memcpy(&h,malformed,sizeof(h));
        uint8_t* prefix=malformed+sizeof(h)+sizeof(uint32_t);
        ResourceCollection row={0};memcpy(&row,prefix,offsetof(ResourceCollection,nodes));
        ResourceNode node;memcpy(&node,prefix+offsetof(ResourceCollection,nodes),sizeof(node));
        size_t bytes=resource_registry_bytes;
        if(n==0)--bytes;if(n==1)++bytes;
        if(n==2)++h.scene_identity;if(n==3)++h.model_identity;
        if(n==4)row.created=NATIVE_ARTICLE_RESOURCE_NODES+1;
        if(n==5)row.active_operations=1;
        if(n==6)row.owner.program=(const NativeScriptItemProgram*)(uintptr_t)1;
        if(n==7)node.payload.pool=(const NativeMatchPool*)(uintptr_t)1;
        if(n==8)node.graph.pool=(const NativeMatchPool*)(uintptr_t)1;
        if(n==9)node.generation=0;
        if(n==10)row.attachment.data=(Fighter*)(uintptr_t)1;
        if(n==11)row.parts=(FighterBone*)(uintptr_t)1;
        memcpy(malformed,&h,sizeof(h));memcpy(prefix,&row,offsetof(ResourceCollection,nodes));
        memcpy(prefix+offsetof(ResourceCollection,nodes),&node,sizeof(node));
        CHECK(!nativeArticleResource_ValidateSnapshot(malformed,bytes));
        CHECK(!nativeArticleResource_Load(malformed,bytes));
        CHECK(!memcmp(before.collection,&expected,sizeof(expected)));++resource_registry_refusals;
    }
    size_t untouched=123;CHECK(!nativeArticleResource_Save(before.collection,BUFFER_BYTES,&untouched)&&untouched==123);
    ++before.collection->active_operations;
    CHECK(!nativeArticleResource_SnapshotBytes()&&!nativeArticleResource_Load(components[0].bytes,resource_registry_bytes));
    --before.collection->active_operations;

    static NativeMatchArena registry_arena;
    static NativeMatchArenaFrame registry_frame;
    static uint8_t registry_arena_bytes[64] ATTRIBUTE_ALIGN(64);
    static uint8_t registry_frame_bytes[1024*1024];
    CHECK(nativeMatchArena_Begin(&registry_arena,registry_arena_bytes,sizeof(registry_arena_bytes),nativeGObjPool.lifetime));
    CHECK(nativeArticleResource_AdmitHistory(&registry_arena));
    CHECK(nativeMatchArena_BindFrame(&registry_arena,&registry_frame,registry_frame_bytes,sizeof(registry_frame_bytes)));
    CHECK(nativeMatchArena_Save(&registry_frame));

    // Real removal revokes bound Fighter parts/hitboxes, frees endpoints and
    // controllers, then the other actor creates future generations in that space.
    NativeScriptValue script_value={.type=NativeScriptType_I32,.as.i64=79};
    state(objects[0],"visits",&script_value);
    nativeScriptItem_BeginTick(objects[0]);
    CHECK(nativeScriptItem_RunPhase(objects[0],NativeScriptItemPhase_Animation,&script_value)==NativeScriptVM_Ok);
    nativeArticleResource_Release(objects[0]);CHECK(!fighter_data->parts[expected.external_part].joint);
    CHECK(fighter_data->x914[0].state==HitCapsule_Disabled&&!fighter_data->x914[0].jobj);
    collection(objects[1],2);load(objects[1],0);load(objects[1],1);
    ResourceCapture future;CHECK(nativeArticleResource_Capture(objects[1],0,&future));
    uint32_t future_generation=future.generation;void* future_object=future.gobj;
    void* future_payload=future.link;void* future_model=future.model;
    for(unsigned i=0;i<sizeof(components)/sizeof(components[0]);++i)
        CHECK(components[i].validate(components[i].bytes,components[i].written));
    u32 heap_again=0;CHECK(OSSaveHeapSnapshot(heap,heap_after,heap_bytes,&heap_again)&&heap_again==heap_written&&!memcmp(heap_before,heap_after,heap_written));
    // No OS allocations occur in this destruction/reuse window; Cell state is
    // unchanged. Payload/pool companions restore before registry/VM reads resume.
    const unsigned order[]={6,5,4,1,2,3,7,10,8,9};
    for(unsigned i=0;i<sizeof(order)/sizeof(order[0]);++i){RegistryComponent* c=&components[order[i]];CHECK(c->load(c->bytes,c->written));}
    CHECK(nativeMatchArena_Load(&registry_frame));
    CHECK(!memcmp(before.collection,&expected,sizeof(expected)));
    CHECK(nativeArticleResource_Capture(objects[0],0,&before)&&before.model==expected.nodes[0].model);
    CHECK(!nativeArticleResource_Collection(objects[1]));
    CHECK(fighter_data->parts[expected.external_part].joint==before.model);
    CHECK(fighter_data->x914[0].state==HitCapsule_Enabled&&fighter_data->x914[0].jobj==before.model);
    for(unsigned i=0;i<sizeof(components)/sizeof(components[0]);++i){RegistryComponent* c=&components[i];size_t bytes;
        CHECK(c->save(c->after,BUFFER_BYTES,&bytes)&&bytes==c->written&&!memcmp(c->bytes,c->after,bytes));}
    nativeArticleResource_Release(objects[0]);collection(objects[1],2);load(objects[1],0);load(objects[1],1);
    CHECK(nativeArticleResource_Capture(objects[1],0,&future));
    CHECK(future.generation==future_generation&&future.gobj==future_object&&future.link==future_payload&&future.model==future_model);
    nativeArticleResource_Release(objects[1]);
    registry_arena.lifetime=0;
    for(unsigned i=0;i<sizeof(components)/sizeof(components[0]);++i){free(components[i].after);free(components[i].bytes);}
    free(heap_after);free(heap_before);free(malformed);

    // Retiring/re-preparing the catalogs inside the same GObj epoch invalidates
    // an old registry even when all of its collection rows are empty.
    uint8_t empty[sizeof(NativeArticleResourceSnapshotHeader)];size_t empty_bytes;
    uint64_t epoch=nativeGObjPool.lifetime;
    CHECK(nativeArticleResource_Save(empty,sizeof(empty),&empty_bytes));
    CHECK(nativeArticleResourceModel_Retire()&&nativeArticleResourceModel_Prepare());
    CHECK(nativeGObjPool.lifetime==epoch);
    CHECK(!nativeArticleResource_ValidateSnapshot(empty,empty_bytes));
    CHECK(!nativeArticleResource_Load(empty,empty_bytes));++resource_registry_refusals;
    CHECK(nativeArticleResource_Save(empty,sizeof(empty),&empty_bytes));
    CHECK(nativeArticleResourceScene_Retire()&&nativeArticleResourceScene_Prepare());
    CHECK(nativeGObjPool.lifetime==epoch);
    CHECK(!nativeArticleResource_ValidateSnapshot(empty,empty_bytes));
    CHECK(!nativeArticleResource_Load(empty,empty_bytes));++resource_registry_refusals;
    HSD_GObj_804D7814=previous_context;
    resource_registry_checks=checks-start;++cases;
}
