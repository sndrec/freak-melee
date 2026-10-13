#include "native_green_grid.h"
#include "native_ground_pool.h"
#include <melee/ft/native_script_runtime.h>
#include <melee/it/native_item_pool.h>
#include <melee/it/item.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <stdlib.h>
#include <string.h>

// Host lease; the actual arrays, occupancy and lifetime are simulation storage.
enum { JOINT_CACHE_SIZE=64 };
static struct {
    NativeMatchPool objects,ground;
    uint64_t lifetime;
    bool active;
    struct {
        HSD_JObj* pointer;
        NativeClassReference backing;
        HSD_ClassInfo* type;
    } joints[JOINT_CACHE_SIZE];
} scene;

static bool idle(void)
{
    return !HSD_GObj_CurrentInvokedProcGObj && !HSD_GObj_CurrentInvokedProc &&
        !HSD_GObj_804D7818 && !HSD_GObj_DelayedProcInfo.flags && nativeScriptRuntime_Idle();
}
bool nativeGreenGrid_CanBeginScene(uint64_t epoch)
{
    if (!idle() || !epoch || nativeGreenGridLifetime || scene.active) return false;
    for (unsigned i=0;i<GREEN_GRID_WORDS;++i) if(nativeGreenGridOccupied[i])return false;
    return true;
}
bool nativeGreenGrid_BeginScene(uint64_t epoch)
{
    if (!nativeGreenGrid_CanBeginScene(epoch) || nativeGroundPool.lifetime!=epoch ||
        nativeGObjPool.lifetime!=epoch) return false;
    scene.objects=nativeGObjPool;scene.ground=nativeGroundPool;scene.lifetime=epoch;
    memset(scene.joints,0,sizeof(scene.joints));
    nativeGreenGridInitializedCount=0;nativeGreenGridLifetime=epoch;scene.active=true;return true;
}
bool nativeGreenGrid_RetireScene(void)
{
    if (!idle())return false;
    memset(nativeGreenGridOccupied,0,sizeof(nativeGreenGridOccupied));
    nativeGreenGridInitializedCount=0;nativeGreenGridLifetime=0;scene.active=false;return true;
}
static bool geometry(const NativeMatchPool* a,const NativeMatchPool* b)
{
    return a->payload==b->payload && a->slots==b->slots && a->free_order==b->free_order &&
        a->capacity==b->capacity && a->stride==b->stride && a->object_size==b->object_size && a->lifetime==b->lifetime;
}
static bool boundary(void)
{
    return scene.active && idle() && nativeGreenGridLifetime==scene.lifetime &&
        geometry(&nativeGObjPool,&scene.objects) && geometry(&nativeGroundPool,&scene.ground);
}
static bool overlap(const void* first,size_t an,const void* second,size_t bn)
{
    uintptr_t a=(uintptr_t)first,b=(uintptr_t)second;
    return an>UINTPTR_MAX-a || bn>UINTPTR_MAX-b || (a<b+bn && b<a+an);
}
static bool independent(const void* data,size_t bytes)
{
    if (!data || !bytes || bytes>UINTPTR_MAX-(uintptr_t)data)return false;
#define EXCLUDE(value) if(overlap(data,bytes,&(value),sizeof(value)))return false
    EXCLUDE(scene);EXCLUDE(nativeGreenGrids);EXCLUDE(nativeGreenGridOccupied);EXCLUDE(nativeGreenGridLifetime);EXCLUDE(nativeGreenGridInitializedCount);
    EXCLUDE(nativeGroundPool);EXCLUDE(nativeGObjPool);EXCLUDE(nativeItemPool);
#undef EXCLUDE
    const NativeMatchPool* pools[]={&scene.objects,&scene.ground,&nativeItemPool};
    for(unsigned i=0;i<3;++i) {
        const NativeMatchPool* pool=pools[i];
        if(overlap(data,bytes,pool->payload,pool->stride*pool->capacity) ||
            overlap(data,bytes,pool->slots,sizeof(*pool->slots)*pool->capacity) ||
            overlap(data,bytes,pool->free_order,sizeof(*pool->free_order)*pool->capacity))return false;
    }
    return true;
}
static uint32_t occupied(void)
{
    uint32_t count=0;
    for(unsigned i=0;i<GREEN_GRID_WORDS;++i)count+=(uint32_t)__builtin_popcountll(nativeGreenGridOccupied[i]);
    return count;
}
size_t nativeGreenGrid_SnapshotBytes(void)
{
    return boundary() ? sizeof(NativeGreenGridSnapshotHeader)+occupied()*sizeof(NativeGreenGridSnapshotEntry) : 0;
}
static bool claim(NativeMatchPoolReference ref,const NativeMatchPool* pool,const NativeMatchPool* fixed)
{
    return ref.pool==pool && ref.lifetime==scene.lifetime && ref.slot<fixed->capacity && ref.generation;
}
static bool live(const NativeGreenGrid* grid,uint32_t slot)
{
    if (!claim(grid->ground,&nativeGroundPool,&scene.ground) ||
        !claim(grid->actor,&nativeGObjPool,&scene.objects) || grid->ground.slot!=slot) return false;
    Ground* owner=nativeMatchPool_Resolve(grid->ground);
    HSD_GObj* object=nativeMatchPool_Resolve(grid->actor);
    return owner && object && grid->ground.slot==slot && object->classifier==HSD_GOBJ_CLASS_STAGE &&
        object->user_data_kind==3 && object->user_data==owner && object->user_data_remove_func==nativeGroundPool_Release &&
        owner->gobj==object && owner->map_id==6 &&
        owner->u.greens.block_positions==nativeGreenGrids[slot].positions &&
        owner->u.greens.blocks==nativeGreenGrids[slot].blocks;
}
static bool zero_cell(const NativeGreenGridCellReferences* refs)
{
    static const NativeGreenGridCellReferences zero={0};
    return !memcmp(refs,&zero,sizeof(zero));
}
static bool pointer_claim(const void* pointer,NativeMatchPoolReference ref,
                          const NativeMatchPool* pool,const NativeMatchPool* fixed)
{
    return pointer && claim(ref,pool,fixed) && fixed->payload && fixed->stride &&
        pointer==fixed->payload+(size_t)ref.slot*fixed->stride;
}
static bool cell_shape(const struct grGreens_BlockVars* block,
                       const NativeGreenGridCellReferences* refs,const void* data,size_t bytes)
{
    if (block->status>3 || (block->status && block->index>=30))return false;
    // Original cleanup destroys both actors, hides the joint and retains these
    // pointer bits. Creation overwrites them before reuse; do not follow them.
    if (!block->status)return zero_cell(refs);
    return pointer_claim(block->xC,refs->block_actor,&nativeGObjPool,&scene.objects) &&
        claim(refs->block_ground,&nativeGroundPool,&scene.ground) &&
        nativeItemPool.lifetime==scene.lifetime &&
        pointer_claim(block->x10,refs->material_actor,&nativeGObjPool,&scene.objects) &&
        claim(refs->material_item,&nativeItemPool,&nativeItemPool) &&
        block->x14 && (uintptr_t)block->x14%_Alignof(HSD_JObj)==0 &&
        refs->joint.bytes==sizeof(HSD_JObj) && nativeClass_Resolve(refs->joint)==block->x14 &&
        nativeClass_TypeKnown(refs->joint_type,sizeof(HSD_JObj)) &&
        !overlap(data,bytes,block->x14,sizeof(HSD_JObj)) &&
        !overlap(data,bytes,refs->joint_type,refs->joint_type->head.info_size);
}
static bool cell_live(const struct grGreens_BlockVars* block,
                      const NativeGreenGridCellReferences* refs,const Ground* grid_owner)
{
    if (!block->status)return zero_cell(refs);
    HSD_GObj* actor=nativeMatchPool_Resolve(refs->block_actor);
    Ground* ground=nativeMatchPool_Resolve(refs->block_ground);
    HSD_GObj* material=nativeMatchPool_Resolve(refs->material_actor);
    NativeItemPoolSlot* item=nativeMatchPool_Resolve(refs->material_item);
    // Every dereference is preceded by a numeric backing/generation proof.
    // The material's own borrower must agree with this grid and its joint.
    return actor && actor==block->xC && ground && actor->classifier==HSD_GOBJ_CLASS_STAGE &&
        actor->user_data_kind==3 && actor->user_data_remove_func==nativeGroundPool_Release &&
        actor->user_data==ground && ground->gobj==actor && ground->map_id==(block->x1_1 ? 3 : 1) &&
        material && material==block->x10 && item && material->classifier==HSD_GOBJ_CLASS_ITEM &&
        material->user_data_kind==6 && material->user_data_remove_func==Item_OnUserDataRemove &&
        material->user_data==&item->actor && item->actor.entity==material &&
        item->actor.xDD4_itemVar.yaku.x0==8 && item->actor.xDD4_itemVar.yaku.x2==1 &&
        item->actor.xDD4_itemVar.yaku.x10==grid_owner && item->actor.xDD4_itemVar.yaku.x4==block->x14 &&
        nativeClass_Resolve(refs->joint)==block->x14 && HSD_CLASS_METHOD(block->x14)==refs->joint_type;
}
static bool capture_cell(const struct grGreens_BlockVars* block,
                         NativeGreenGridCellReferences* refs,const Ground* grid_owner)
{
    memset(refs,0,sizeof(*refs));
    if (block->status>3 || (block->status && block->index>=30))return false;
    if (!block->status)return true;
    if (!nativeMatchPool_Reference(&nativeGObjPool,block->xC,&refs->block_actor) ||
        !nativeMatchPool_Reference(&nativeGObjPool,block->x10,&refs->material_actor) ||
        !nativeMatchPool_Reference(&nativeGroundPool,block->xC->user_data,&refs->block_ground) ||
        !nativeItemPool_Slot(block->x10->user_data) || nativeItemPool.lifetime!=scene.lifetime ||
        !nativeMatchPool_Reference(&nativeItemPool,block->x10->user_data,&refs->material_item) ||
        !block->x14 || (uintptr_t)block->x14%_Alignof(HSD_JObj))return false;
    unsigned cache=((uintptr_t)block->x14>>4)%JOINT_CACHE_SIZE;
    if (scene.joints[cache].pointer!=block->x14 ||
        nativeClass_Resolve(scene.joints[cache].backing)!=block->x14) {
        NativeClassReference backing;
        if (!(backing=nativeClass_Reference(block->x14,sizeof(HSD_JObj))).bytes)return false;
        HSD_ClassInfo* type=HSD_CLASS_METHOD(block->x14);
        if (!nativeClass_TypeKnown(type,sizeof(HSD_JObj)))return false;
        scene.joints[cache].pointer=block->x14;scene.joints[cache].backing=backing;scene.joints[cache].type=type;
    }
    refs->joint=scene.joints[cache].backing;refs->joint_type=scene.joints[cache].type;
    return cell_live(block,refs,grid_owner);
}
bool nativeGreenGrid_Validate(const void* data,size_t bytes)
{
    if (!boundary() || bytes<sizeof(NativeGreenGridSnapshotHeader) || !independent(data,bytes))return false;
    NativeGreenGridSnapshotHeader h;memcpy(&h,data,sizeof(h));
    if(h.lifetime!=scene.lifetime || h.initialized_count>GREEN_GRID_CAPACITY || h.count>h.initialized_count ||
        bytes!=sizeof(h)+h.count*sizeof(NativeGreenGridSnapshotEntry))return false;
    uint32_t previous=0;const u8* cursor=(const u8*)data+sizeof(h);
    for(uint32_t i=0;i<h.count;++i,cursor+=sizeof(NativeGreenGridSnapshotEntry)) {
        NativeGreenGridSnapshotEntry entry;memcpy(&entry,cursor,sizeof(entry));
        if(entry.reserved || entry.slot>=h.initialized_count || (i && entry.slot<=previous) ||
            !claim(entry.grid.ground,&nativeGroundPool,&scene.ground) || entry.grid.ground.slot!=entry.slot ||
            !claim(entry.grid.actor,&nativeGObjPool,&scene.objects))return false;
        previous=entry.slot;
        for(unsigned r=0;r<GREEN_GRID_ROWS;++r)for(unsigned c=0;c<GREEN_GRID_COLUMNS;++c)
            if(!cell_shape(&entry.grid.blocks[r][c],&entry.cells[r][c],data,bytes))return false;
    }
    return true;
}
bool nativeGreenGrid_Save(void* data,size_t capacity,size_t* written)
{
    size_t bytes=nativeGreenGrid_SnapshotBytes();
    if(!bytes || capacity<bytes || !independent(data,bytes) || !independent(written,sizeof(*written)) ||
        overlap(data,bytes,written,sizeof(*written)))return false;
    // Complete live-owner/borrower checks before modifying caller output. The
    // second pass cannot fail at this idle boundary: it uses the same records.
    for(unsigned w=0;w<GREEN_GRID_WORDS;++w) {
        uint64_t bits=nativeGreenGridOccupied[w];
        while(bits) {
            unsigned slot=w*64+(unsigned)__builtin_ctzll(bits);bits&=bits-1;
            if(slot>=GREEN_GRID_CAPACITY || !live(&nativeGreenGrids[slot],slot))return false;
            Ground* owner=nativeMatchPool_Resolve(nativeGreenGrids[slot].ground);
            for(unsigned r=0;r<GREEN_GRID_ROWS;++r)for(unsigned c=0;c<GREEN_GRID_COLUMNS;++c) {
                const struct grGreens_BlockVars* block=&nativeGreenGrids[slot].blocks[r][c];
                NativeGreenGridCellReferences refs;
                if(!capture_cell(block,&refs,owner) || (block->status &&
                    (overlap(data,bytes,block->x14,sizeof(HSD_JObj)) ||
                     overlap(written,sizeof(*written),block->x14,sizeof(HSD_JObj)) ||
                     overlap(data,bytes,refs.joint_type,refs.joint_type->head.info_size) ||
                     overlap(written,sizeof(*written),refs.joint_type,refs.joint_type->head.info_size))))return false;
            }
        }
    }
    NativeGreenGridSnapshotHeader h={scene.lifetime,occupied(),nativeGreenGridInitializedCount};memcpy(data,&h,sizeof(h));
    u8* cursor=(u8*)data+sizeof(h);
    for(unsigned w=0;w<GREEN_GRID_WORDS;++w) {
        uint64_t bits=nativeGreenGridOccupied[w];
        while(bits) {
            unsigned slot=w*64+(unsigned)__builtin_ctzll(bits);bits&=bits-1;
            NativeGreenGridSnapshotEntry entry;memset(&entry,0,sizeof(entry));entry.slot=slot;
            memcpy(&entry.grid,&nativeGreenGrids[slot],sizeof(entry.grid));
            Ground* owner=nativeMatchPool_Resolve(entry.grid.ground);
            for(unsigned r=0;r<GREEN_GRID_ROWS;++r)for(unsigned c=0;c<GREEN_GRID_COLUMNS;++c)
                if(!capture_cell(&entry.grid.blocks[r][c],&entry.cells[r][c],owner))abort();
            memcpy(cursor,&entry,sizeof(entry));cursor+=sizeof(entry);
        }
    }
    *written=bytes;return true;
}
bool nativeGreenGrid_Load(const void* data,size_t bytes)
{
    if(!nativeGreenGrid_Validate(data,bytes))return false;
    NativeGreenGridSnapshotHeader h;memcpy(&h,data,sizeof(h));
    const u8* cursor=(const u8*)data+sizeof(h);
    // Actual GObj, Ground, Item and class companion payloads restore first.
    // Aggregate prevalidation must still establish candidate-wide graph closure.
    for(uint32_t i=0;i<h.count;++i,cursor+=sizeof(NativeGreenGridSnapshotEntry)) {
        NativeGreenGridSnapshotEntry entry;memcpy(&entry,cursor,sizeof(entry));
        if(!live(&entry.grid,entry.slot))return false;
        Ground* owner=nativeMatchPool_Resolve(entry.grid.ground);
        for(unsigned r=0;r<GREEN_GRID_ROWS;++r)for(unsigned c=0;c<GREEN_GRID_COLUMNS;++c)
            if(!cell_live(&entry.grid.blocks[r][c],&entry.cells[r][c],owner))return false;
    }
    nativeGreenGridInitializedCount=h.initialized_count;
    memset(nativeGreenGridOccupied,0,sizeof(nativeGreenGridOccupied));cursor=(const u8*)data+sizeof(h);
    for(uint32_t i=0;i<h.count;++i,cursor+=sizeof(NativeGreenGridSnapshotEntry)) {
        NativeGreenGridSnapshotEntry entry;memcpy(&entry,cursor,sizeof(entry));
        memcpy(&nativeGreenGrids[entry.slot],&entry.grid,sizeof(entry.grid));
        nativeGreenGridOccupied[entry.slot/64]|=UINT64_C(1)<<(entry.slot%64);
    }
    return true;
}

// Occupancy is the only publication mechanism. Restored occupancy hides any
// discarded-future tail; Bind clears a full grid before publishing it again.
bool nativeGreenGrid_AdmitHistory(NativeMatchArena* arena)
{
    if (!boundary() || !arena || arena->lifetime!=scene.lifetime ||
        nativeGreenGridInitializedCount>GREEN_GRID_CAPACITY) return false;
    for (unsigned w=0;w<GREEN_GRID_WORDS;++w) {
        uint64_t bits=nativeGreenGridOccupied[w];
        while (bits) {
            unsigned slot=w*64+(unsigned)__builtin_ctzll(bits);bits&=bits-1;
            if (slot>=nativeGreenGridInitializedCount || !live(&nativeGreenGrids[slot],slot)) return false;
        }
    }
    return nativeMatchArena_AddRoot(arena,nativeGreenGridOccupied,sizeof(nativeGreenGridOccupied)) &&
        nativeMatchArena_AddRoot(arena,&nativeGreenGridLifetime,sizeof(nativeGreenGridLifetime)) &&
        nativeMatchArena_AddRoot(arena,&nativeGreenGridInitializedCount,sizeof(nativeGreenGridInitializedCount)) &&
        nativeMatchArena_AddCountedRoot(arena,nativeGreenGrids,sizeof(*nativeGreenGrids),
            GREEN_GRID_CAPACITY,&nativeGreenGridInitializedCount);
}
