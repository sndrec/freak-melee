#include <melee/gr/native_green_grid.h>
#include <melee/it/native_item_pool.h>
#include <melee/it/item.h>
// Controlled callback endpoint: use the actual item pool, no asset teardown.
void Item_OnUserDataRemove(void* actor)
{
    NativeMatchPoolReference ref;CHECK(nativeItemPool_Slot(actor));
    CHECK(nativeMatchPool_Reference(&nativeItemPool,actor,&ref));
    CHECK(nativeMatchPool_Free(&nativeItemPool,ref));
}
static u8 grid_items_saved[131072];
static unsigned grid_checks,grid_rejections;
static size_t grid_snapshot_bytes;
static u8 grid_saved[16384],grid_bad[16384],grid_after[16384];
static void test_grid_state(void)
{
    unsigned first=checks;camera_idle=true;
    CHECK(nativeMatchPool_Bind(&nativeGObjPool,camera_objects,sizeof(camera_objects),
        camera_slots,camera_free,8,sizeof(HSD_GObj),_Alignof(HSD_GObj),105));
    CHECK(nativeMatchScene_Begin());CHECK(nativeGroundPool_BeginScene(105));CHECK(nativeItemPool_BeginScene(105));
    size_t written,ob,gb,ib,cb;
    HSD_GObj* blocks[2];HSD_GObj* materials[2];
    Ground* grounds[2];NativeItemPoolSlot* items[2];
    NativeMatchPoolReference block_handles[2],block_userdata[2],material_handles[2],material_userdata[2];
    HSD_JObj* joints[2];
    CHECK(nativeGreenGrid_SnapshotBytes()==sizeof(NativeGreenGridSnapshotHeader));
    CHECK(nativeGreenGrid_Save(grid_saved,sizeof(grid_saved),&written));CHECK(written==16);
    HSD_GObj* objects[2];Ground* owners[2];NativeMatchPoolReference handles[2],userdata[2];
    for(unsigned i=0;i<2;++i) {
        objects[i]=nativeMatchPool_Allocate(&nativeGObjPool,&handles[i]);
        owners[i]=nativeMatchPool_Allocate(&nativeGroundPool,&userdata[i]);CHECK(objects[i] && owners[i]);
        memset(objects[i],0,sizeof(*objects[i]));memset(owners[i],0,sizeof(*owners[i]));
        objects[i]->classifier=HSD_GOBJ_CLASS_STAGE;objects[i]->user_data_kind=3;
        objects[i]->user_data=owners[i];objects[i]->user_data_remove_func=nativeGroundPool_Release;
        owners[i]->gobj=objects[i];owners[i]->map_id=6;CHECK(nativeGreenGrid_Bind(objects[i]));
        CHECK(!nativeGreenGrid_Bind(objects[i]));
        CHECK(owners[i]->u.greens.block_positions==nativeGreenGrids[userdata[i].slot].positions);
        CHECK(owners[i]->u.greens.blocks==nativeGreenGrids[userdata[i].slot].blocks);
        for(unsigned r=0;r<5;++r)for(unsigned c=0;c<6;++c) {
            struct grGreens_BlockVars* block=&owners[i]->u.greens.blocks[r][c];
            CHECK(!block->status && !block->xC && !block->x14);
            block->index=r*6+c;block->x8=(float)(r*6+c);
            owners[i]->u.greens.block_positions[r*6+c].x=(float)(i+r+c);
        }
    }
    for (unsigned i=0;i<2;++i) {
        blocks[i]=nativeMatchPool_Allocate(&nativeGObjPool,&block_handles[i]);
        materials[i]=nativeMatchPool_Allocate(&nativeGObjPool,&material_handles[i]);
        grounds[i]=nativeMatchPool_Allocate(&nativeGroundPool,&block_userdata[i]);
        items[i]=nativeMatchPool_Allocate(&nativeItemPool,&material_userdata[i]);
        joints[i]=hsdNew(&stage_joint_class);
        CHECK(blocks[i] && materials[i] && grounds[i] && items[i] && joints[i]);
        memset(blocks[i],0,sizeof(*blocks[i]));memset(materials[i],0,sizeof(*materials[i]));
        memset(grounds[i],0,sizeof(*grounds[i]));memset(items[i],0,sizeof(*items[i]));
        memset((u8*)joints[i]+sizeof(HSD_Class),0,sizeof(*joints[i])-sizeof(HSD_Class));
        blocks[i]->classifier=HSD_GOBJ_CLASS_STAGE;blocks[i]->user_data_kind=3;
        blocks[i]->user_data=grounds[i];blocks[i]->user_data_remove_func=nativeGroundPool_Release;
        grounds[i]->gobj=blocks[i];grounds[i]->map_id=i ? 3 : 1;
        materials[i]->classifier=HSD_GOBJ_CLASS_ITEM;materials[i]->user_data_kind=6;
        materials[i]->user_data=&items[i]->actor;materials[i]->user_data_remove_func=Item_OnUserDataRemove;
        items[i]->actor.entity=materials[i];items[i]->actor.xDD4_itemVar.yaku.x0=8;
        items[i]->actor.xDD4_itemVar.yaku.x2=1;items[i]->actor.xDD4_itemVar.yaku.x10=owners[i];
        items[i]->actor.xDD4_itemVar.yaku.x4=joints[i];
        struct grGreens_BlockVars* block=&owners[i]->u.greens.blocks[4][5];
        block->status=3;block->x1_1=i;block->xC=blocks[i];block->x10=materials[i];block->x14=joints[i];
        // Dormant pointer bits are not live references and may be invalid.
        owners[i]->u.greens.blocks[0][0].xC=(void*)42;
        owners[i]->u.greens.blocks[0][0].x10=(void*)43;
        owners[i]->u.greens.blocks[0][0].x14=(void*)44;
        owners[i]->u.greens.blocks[0][0].hit_source=(void*)45;
    }
    CHECK(nativeGreenGrid_Save(grid_saved,sizeof(grid_saved),&written));grid_snapshot_bytes=written;
    CHECK(written==sizeof(NativeGreenGridSnapshotHeader)+2*sizeof(NativeGreenGridSnapshotEntry));
    CHECK(nativeMatchSnapshotPool_Save(&nativeGObjPool,stage_objects_saved,sizeof(stage_objects_saved),&ob));
    CHECK(nativeGroundPool_Save(stage_ground_saved,sizeof(stage_ground_saved),&gb));
    CHECK(nativeItemPool_Save(grid_items_saved,sizeof(grid_items_saved),&ib));
    CHECK(nativeClass_Save(stage_graph_saved,sizeof(stage_graph_saved),&cb));

    static NativeMatchArena grid_arena;
    static NativeMatchArenaFrame grid_frame;
    static u8 grid_arena_bytes[64] ATTRIBUTE_ALIGN(64);
    static u8 grid_frame_bytes[16*1024*1024];
    CHECK(nativeMatchArena_Begin(&grid_arena,grid_arena_bytes,sizeof(grid_arena_bytes),105));
    CHECK(nativeGreenGrid_AdmitHistory(&grid_arena));
    CHECK(nativeMatchArena_BindFrame(&grid_arena,&grid_frame,grid_frame_bytes,sizeof(grid_frame_bytes)));
    CHECK(nativeMatchArena_Save(&grid_frame));
    NativeMatchPoolReference future_actor_ref,future_ground_ref;
    HSD_GObj* future_actor=nativeMatchPool_Allocate(&nativeGObjPool,&future_actor_ref);
    Ground* future_ground=nativeMatchPool_Allocate(&nativeGroundPool,&future_ground_ref);
    CHECK(future_actor && future_ground);
    future_actor->classifier=HSD_GOBJ_CLASS_STAGE;future_actor->user_data_kind=3;
    future_actor->user_data=future_ground;future_actor->user_data_remove_func=nativeGroundPool_Release;
    future_ground->gobj=future_actor;future_ground->map_id=6;
    CHECK(nativeGreenGrid_Bind(future_actor));
    CHECK(nativeGreenGridInitializedCount>2);
    owners[0]->u.greens.blocks[4][5].x8=101;
    owners[1]->u.greens.block_positions[29].y=202;
    CHECK(nativeMatchArena_Load(&grid_frame));
    CHECK(nativeGreenGridInitializedCount==2);
    size_t copied;
    CHECK(nativeGreenGrid_Save(grid_after,sizeof(grid_after),&copied));
    CHECK(copied==written && !memcmp(grid_saved,grid_after,copied));
    CHECK(!(nativeGreenGridOccupied[future_ground_ref.slot/64] & (UINT64_C(1)<<(future_ground_ref.slot%64))));
    nativeGroundPool_Release(future_ground);
    CHECK(nativeMatchPool_Free(&nativeGObjPool,future_actor_ref));
    grid_arena.lifetime=0;

    owners[0]->u.greens.blocks[4][5].x8=91;owners[1]->u.greens.block_positions[29].y=17;
    CHECK(nativeGreenGrid_Load(grid_saved,written));CHECK(owners[0]->u.greens.blocks[4][5].x8==29);
    CHECK(owners[1]->u.greens.block_positions[29].y==0);
    u8 endpoint[16384],repeat[16384];size_t after;
    for(unsigned pass=0;pass<2;++pass) {
        if(pass)CHECK(nativeGreenGrid_Load(grid_saved,written));
        for(unsigned frame=0;frame<16;++frame) {
            owners[0]->u.greens.block_positions[frame].y+=(float)frame;
            owners[1]->u.greens.blocks[frame%5][frame%6].x4+=(float)frame/7;
        }
        CHECK(nativeGreenGrid_Save(pass ? repeat : endpoint,sizeof(endpoint),&after));CHECK(after==written);
    }
    CHECK(!memcmp(endpoint,repeat,written));CHECK(nativeGreenGrid_Load(grid_saved,written));
    NativeGreenGridSnapshotHeader* h=(void*)grid_bad;
    NativeGreenGridSnapshotEntry* entry=(void*)(grid_bad+sizeof(*h));
#define REJECT(change) do { memcpy(grid_bad,grid_saved,written);change; \
        CHECK(!nativeGreenGrid_Validate(grid_bad,written));CHECK(!nativeGreenGrid_Load(grid_bad,written)); \
        CHECK(nativeGreenGrid_Save(grid_after,sizeof(grid_after),&after));CHECK(after==written); \
        CHECK(!memcmp(grid_after,grid_saved,written));++grid_rejections; } while(0)
    REJECT(++h->lifetime);REJECT(h->initialized_count=GREEN_GRID_CAPACITY+1);REJECT(h->initialized_count=0);REJECT(h->count=3);
    REJECT(entry[1].slot=entry[0].slot);REJECT(entry[1].reserved=1);
    REJECT(entry[1].grid.actor.pool=NULL);REJECT(entry[1].grid.ground.generation=0);
    REJECT(entry[1].grid.blocks[4][5].status=4);REJECT(entry[1].grid.blocks[4][5].index=31);
    REJECT(entry[1].cells[4][5].block_actor.pool=NULL);
    REJECT(entry[1].cells[4][5].block_ground.generation=0);
    REJECT(entry[1].cells[4][5].material_actor.slot=8);
    REJECT(entry[1].cells[4][5].material_item.pool=&nativeGroundPool);
    REJECT(entry[1].grid.blocks[4][5].xC=(void*)42);
    REJECT(entry[1].grid.blocks[4][5].x10=(void*)43);
    REJECT(entry[1].grid.blocks[4][5].x14=(void*)44);
    REJECT(entry[1].cells[4][5].joint.reserved=1);
    REJECT(entry[1].cells[4][5].joint.bytes=1);
    REJECT(entry[1].cells[4][5].joint_type=(void*)1);
    REJECT(entry[1].cells[0][0].block_actor=entry[1].cells[4][5].block_actor);
#undef REJECT
    // Numeric candidate references pass; wrong late generation fails before
    // the valid first record can overwrite its current payload.
    memcpy(grid_bad,grid_saved,written);++entry[1].grid.ground.generation;
    owners[0]->u.greens.blocks[0][0].x8=777;
    CHECK(nativeGreenGrid_Validate(grid_bad,written));CHECK(!nativeGreenGrid_Load(grid_bad,written));
    CHECK(owners[0]->u.greens.blocks[0][0].x8==777);CHECK(nativeGreenGrid_Load(grid_saved,written));
    // Valid numeric shape is not proof of restored occupancy/role. Refuse
    // late borrower disagreement before copying the first owner's payload.
    memcpy(grid_bad,grid_saved,written);++entry[1].cells[4][5].material_item.generation;
    owners[0]->u.greens.blocks[0][0].x8=888;
    CHECK(nativeGreenGrid_Validate(grid_bad,written));CHECK(!nativeGreenGrid_Load(grid_bad,written));
    CHECK(owners[0]->u.greens.blocks[0][0].x8==888);CHECK(nativeGreenGrid_Load(grid_saved,written));
    items[1]->actor.xDD4_itemVar.yaku.x10=owners[0];
    CHECK(!nativeGreenGrid_Load(grid_saved,written));
    memset(grid_after,0xA5,sizeof(grid_after));after=313;
    CHECK(!nativeGreenGrid_Save(grid_after,sizeof(grid_after),&after));CHECK(after==313);
    for(unsigned i=0;i<sizeof(grid_after);++i)CHECK(grid_after[i]==0xA5);
    CHECK(nativeItemPool_Load(grid_items_saved,ib));
    HSD_CLASS_METHOD(joints[1])=(void*)1;CHECK(!nativeGreenGrid_Load(grid_saved,written));
    CHECK(nativeClass_Load(stage_graph_saved,cb));CHECK(nativeGreenGrid_Load(grid_saved,written));
    CHECK(!nativeGreenGrid_Save(joints[1],sizeof(grid_saved),&after));
    CHECK(!nativeGreenGrid_Save(grid_after,sizeof(grid_after),(void*)joints[1]));
    CHECK(!nativeGreenGrid_Save(&stage_joint_class,sizeof(grid_saved),&after));
    // Retire/reuse each companion at the same physical address, then restore
    // its actual metadata/payload. Neither current address nor shape suffices.
    CHECK(nativeMatchPool_Free(&nativeGObjPool,material_handles[1]));
    NativeMatchPoolReference replacement;
    CHECK(nativeMatchPool_Allocate(&nativeGObjPool,&replacement)==materials[1]);
    CHECK(nativeGreenGrid_Validate(grid_saved,written));CHECK(!nativeGreenGrid_Load(grid_saved,written));
    CHECK(nativeMatchSnapshotPool_Load(&nativeGObjPool,stage_objects_saved,ob));
    Item_OnUserDataRemove(&items[1]->actor);
    CHECK(nativeMatchPool_Allocate(&nativeItemPool,&replacement)==items[1]);
    CHECK(nativeGreenGrid_Validate(grid_saved,written));CHECK(!nativeGreenGrid_Load(grid_saved,written));
    CHECK(nativeItemPool_Load(grid_items_saved,ib));CHECK(nativeGreenGrid_Load(grid_saved,written));
    nativeGroundPool_Release(grounds[1]);
    CHECK(nativeMatchPool_Allocate(&nativeGroundPool,&replacement)==grounds[1]);
    CHECK(nativeGreenGrid_Validate(grid_saved,written));CHECK(!nativeGreenGrid_Load(grid_saved,written));
    CHECK(nativeGroundPool_Load(stage_ground_saved,gb));CHECK(nativeGreenGrid_Load(grid_saved,written));
    // Column compaction moves whole cell records. Claims follow contents, not
    // a cached row/column location.
    owners[0]->u.greens.blocks[0][1]=owners[0]->u.greens.blocks[4][5];
    memset(&owners[0]->u.greens.blocks[4][5],0,sizeof(struct grGreens_BlockVars));
    CHECK(nativeGreenGrid_Save(grid_after,sizeof(grid_after),&after));
    CHECK(nativeGreenGrid_Validate(grid_after,after));CHECK(nativeGreenGrid_Load(grid_saved,written));
    CHECK(!nativeGreenGrid_Save(&nativeGreenGrids[0],sizeof(grid_saved),&after));
    CHECK(!nativeGreenGrid_Save(grid_after,sizeof(grid_after),(size_t*)&nativeGreenGridLifetime));
    CHECK(nativeGreenGrid_DropOwner(userdata[1]));nativeGroundPool_Release(owners[1]);
    CHECK(nativeMatchPool_Free(&nativeGObjPool,handles[1]));
    CHECK(nativeGreenGrid_Validate(grid_saved,written));CHECK(!nativeGreenGrid_Load(grid_saved,written));
    CHECK(nativeGreenGrid_SnapshotBytes()==sizeof(*h)+sizeof(*entry));
    CHECK(nativeMatchSnapshotPool_Load(&nativeGObjPool,stage_objects_saved,ob));
    CHECK(!nativeGreenGrid_Load(grid_saved,written));CHECK(nativeGroundPool_Load(stage_ground_saved,gb));
    CHECK(nativeGreenGrid_Load(grid_saved,written));CHECK(nativeGreenGrid_SnapshotBytes()==written);
    camera_idle=false;CHECK(!nativeGreenGrid_Load(grid_saved,written));camera_idle=true;
    CHECK(nativeItemPool_RetireScene());
    CHECK(nativeGroundPool_RetireScene());nativeMatchScene_Retire();CHECK(!nativeGreenGrid_Load(grid_saved,written));
    camera_idle=false;grid_checks=checks-first;
}
