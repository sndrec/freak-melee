// Actual Item scheduler + actor state/storage snapshots. Geometry, graph and
// native workshop holdable protocol leaves remain controlled by boundary.c.
#include <melee/it/native_script_item_snapshot.h>

static unsigned capture_cases;
static bool unexpected_capture_animation(HSD_GObj* object)
{ (void)object;CHECK(false);return true; }
static void unexpected_capture_physics(HSD_GObj* object)
{ (void)object;CHECK(false); }
static bool unexpected_capture_collision(HSD_GObj* object)
{ (void)object;CHECK(false);return true; }

static void capture_inert_cases(void)
{
    HSD_GObj* object=create();Item* item=object->user_data;
    item->native_capture_inert=true;
    scheduler_state(object,0);
    CHECK(nativeScriptItem_IsAttached(object));
    const NativeScriptItemProgram* retained=nativeScriptItem_Spec(object);
    NativeScriptReceiverView view;
    CHECK(nativeScriptItem_View(object,item->x1C,&view));
    NativeScriptStorageInstance* storage=view.storage;
    NativeScriptValue result;
    for(unsigned phase=0;phase<NativeScriptItemPhase_Count;++phase){
        result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=42};
        CHECK(nativeScriptItem_RunPhase(object,phase,&result)==NativeScriptVM_Ok);
        CHECK(state(object,"count").as.i64==1);
    }
    // Normal native callbacks would retire/move this object if mistakenly run.
    item->animated=unexpected_capture_animation;
    item->physics_updated=unexpected_capture_physics;
    item->collided=unexpected_capture_collision;
    unsigned tail=scheduler_tail;
    for(unsigned frame=0;frame<30;++frame){
        scheduler_animation(object);scheduler_physics(object);scheduler_collision(object);
        CHECK(nativeScriptItem_IsAttached(object));
        CHECK(state(object,"count").as.i64==1);
        CHECK(nativeScriptItem_Spec(object)==retained);
        CHECK(nativeScriptItem_View(object,item->x1C,&view)&&view.storage==storage&&storage->live);
    }
    CHECK(scheduler_tail>tail); // shared graph/position scheduler tail still runs
    ++capture_cases;

    static unsigned char snapshot[65536];size_t written=0;
    CHECK(nativeScriptItem_Save(snapshot,sizeof(snapshot),&written));
    CHECK(nativeScriptItem_ValidateSnapshot(snapshot,written));
    const NativeScriptItemSnapshotHeader* header=(const void*)snapshot;
    CHECK(header->count==1);
    const NativeScriptItemSnapshotRecord* record=(const void*)(snapshot+sizeof(*header));
    CHECK(record->selected_state==0);
    for(unsigned phase=0;phase<NativeScriptItemPhase_Count;++phase)CHECK(record->phases[phase]==UINT32_MAX);
    // Actor snapshots restore phases/storage; the native Item field belongs to
    // the separate native heap state and is restored explicitly in this probe.
    item->native_capture_inert=false;
    CHECK(nativeScriptItem_SelectState(object,0));
    CHECK(nativeScriptItem_RunPhase(object,NativeScriptItemPhase_Physics,&result)==NativeScriptVM_Ok);
    CHECK(state(object,"count").as.i64==4);
    item->native_capture_inert=true;
    CHECK(nativeScriptItem_Load(snapshot,written));
    CHECK(state(object,"count").as.i64==1);
    CHECK(nativeScriptItem_RunPhase(object,NativeScriptItemPhase_Physics,&result)==NativeScriptVM_Ok);
    CHECK(state(object,"count").as.i64==1);
    ++capture_cases;

    // Capture suppresses normal phases, not the authored destruction hook.
    NativeMatchPoolReference reference;
    CHECK(nativeMatchPool_Reference(&nativeGObjPool,object,&reference));
    Item_8026A8EC(object);
    CHECK(nativeMatchPool_Resolve(reference)==NULL&&!nativeScriptItem_IsAttached(object));
    CHECK(scheduler_destroyed==101);
    ++capture_cases;

    object=create();item=object->user_data;
    nativeScriptItem_Detach(object);
    item->native_script_owner=0;
    item->native_capture_inert=true;
    item->animated=unexpected_capture_animation;
    item->physics_updated=unexpected_capture_physics;
    item->collided=unexpected_capture_collision;
    scheduler_animation(object);scheduler_physics(object);scheduler_collision(object);
    CHECK(nativeMatchPool_Reference(&nativeGObjPool,object,&reference));
    item->xB8_itemLogicTable=&unowned_logic;
    Item_8026A8EC(object);
    CHECK(nativeMatchPool_Resolve(reference)==NULL);
    ++capture_cases;
    cases+=capture_cases;
}
