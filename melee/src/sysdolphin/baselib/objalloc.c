#include "objalloc.h"

#include <string.h>

#include "memory.h"
#ifdef MELEE_NATIVE
#include "native_hsd_reservation.h"
#include "native_objalloc_snapshot.h"
#include "native_hsd_arena.h"
#endif
#include <dolphin/os/OSAlloc.h>

static objheap obj_heap = { 0, 0, -1, -1 };

static HSD_ObjAllocData* alloc_datas;
#ifdef MELEE_NATIVE
static uint64_t allocation_epoch;
static NativeObjAllocSnapshotBlock allocation_blocks[NATIVE_OBJALLOC_SNAPSHOT_BLOCKS];
static uint32_t allocation_block_count;
static void next_epoch(void)
{
    HSD_ASSERT(0, allocation_epoch != UINT64_MAX);
    ++allocation_epoch;
}
#endif

const HSD_ObjAllocData* HSD_ObjAllocRegistry(void)
{
    return alloc_datas;
}

void HSD_ObjSetHeap(u32 size, void* ptr)
{
#ifdef MELEE_NATIVE
    HSD_ASSERT(0, !nativeHsdReservation_Active());
    next_epoch();
#endif
    obj_heap.curr = (uintptr_t) ptr;
    obj_heap.top = (uintptr_t) ptr;
    obj_heap.remain = size;
    obj_heap.size = size;
}

s32 HSD_ObjAllocAddFree(HSD_ObjAllocData* data, u32 num)
{
    uintptr_t computed_start;
    uintptr_t pool_end;
    u32 pool_size;
    u8* pool_start;

    u8 _[4];

    HSD_ASSERT(0xEE, data);
#ifdef MELEE_NATIVE
    // A construction claim may only consume its reserved slots.
    if (nativeHsdReservation_RejectHeap()) return 0;
#endif
    if (num == 0 || num > U32_MAX / data->size) {
        return 0;
    }
    pool_size = data->size * num;
#ifdef MELEE_NATIVE
    HSD_ASSERT(0, allocation_block_count < NATIVE_OBJALLOC_SNAPSHOT_BLOCKS);
#endif
    if (obj_heap.top != 0) {
        pool_end = obj_heap.top + obj_heap.size;
        computed_start =
            (obj_heap.curr + data->align) & ~(uintptr_t) data->align;
        pool_start = (void*) computed_start;
        if (computed_start > pool_end) {
            return 0;
        }
        if (pool_end - computed_start < pool_size) {
            pool_size = (u32) (pool_end - computed_start);
            pool_size -= pool_size % data->size;
        }
        num = pool_size / data->size;
        if (num == 0) {
            return 0;
        }
        obj_heap.curr = computed_start + pool_size;
        obj_heap.remain = (u32) (pool_end - obj_heap.curr);
    } else {
#ifdef MELEE_NATIVE
        HSD_ASSERTREPORT(0,nativeHsdArena.lifetime || nativeHsdArena_BeginHeap(),
            "HSD object backing was not initialized.\n");
        if(!nativeHsdArena_Ensure(pool_size,(size_t)data->align+1))return 0;
        pool_start=nativeMatchArena_Allocate(&nativeHsdArena,pool_size,(size_t)data->align+1);
#else
        pool_start = HSD_MemAlloc(pool_size);
#endif
        if (pool_start == 0) {
            return 0;
        }
        obj_heap.remain -= pool_size;
    }

#ifdef MELEE_NATIVE
    // Record actual ordinary backing from either the fixed arena or OS heap.
    // Reuse/free does not traverse this directory. Construction claims return
    // above and keep their own reservation ownership instead.
    allocation_blocks[allocation_block_count++] =
        (NativeObjAllocSnapshotBlock){data, pool_start, pool_size};
#endif
    {
        int i;
        for (i = 0; (unsigned) i < num - 1; i++) {
            *(void**) (pool_start + data->size * i) =
                (void*) (pool_start + data->size * (i + 1));
        }
        *(void**) (pool_start + data->size * i) = data->freehead;
    }

    data->freehead = (HSD_ObjAllocLink*) pool_start;
    data->free += num;
    return num;
}

void* HSD_ObjAlloc(HSD_ObjAllocData* data)
{
#ifdef MELEE_NATIVE
    bool reserved;
    void* allocation = nativeHsdReservation_Allocate(data, data->size, &reserved);
    if (reserved) return allocation;
#endif
    HSD_ObjAllocLink* cur;
    u32 size;

    if (data->num_limit_flag && data->used >= data->num_limit) {
        return NULL;
    }
    if (data->heap_limit_flag) {
        if (data->heap_limit_num == (unsigned) -1) {
            if (obj_heap.top != 0) {
                size = obj_heap.remain;
            } else {
                size = OSCheckHeap(HSD_GetHeap());
            }
            if (size <= data->heap_limit_size) {
                data->heap_limit_num = data->used + data->free;
            }
        } else {
            if (obj_heap.top != 0) {
                size = obj_heap.remain;
            } else {
                size = OSCheckHeap(HSD_GetHeap());
            }
            if (size > data->heap_limit_size) {
                data->heap_limit_num = -1;
            }
        }
        if (data->used >= data->heap_limit_num) {
            return NULL;
        }
    }
    if (data->free == 0) {
        u32 count = 1;
#ifdef MELEE_NATIVE
        // Pack ordinary construction slots into bounded blocks. Large models
        // otherwise consume one snapshot-directory entry per envelope list.
        // Explicit limits and sealed match heaps retain single-slot growth.
        if (!nativeHsdArena.sealed && !data->num_limit_flag && !data->heap_limit_flag) {
            count = 4096 / data->size;
            if (count > 32) count = 32;
            if (!count) count = 1;
        }
#endif
        HSD_ObjAllocAddFree(data, count);
        if (data->free == 0) {
            return NULL;
        }
    }
    cur = data->freehead;
    data->freehead = cur->next;
    data->used += 1;
    data->free -= 1;
    if (data->used > data->peak) {
        data->peak = data->used;
    }
    return cur;
}

void HSD_ObjFree(HSD_ObjAllocData* data, void* obj)
{
#ifdef MELEE_NATIVE
    int reserved = nativeHsdReservation_Free(data, obj, data->size);
    HSD_ASSERT(0, reserved >= 0);
    if (reserved) return;
#endif
    HSD_ObjAllocLink* link = obj;
    link->next = data->freehead;
    data->freehead = link;
    data->free += 1;
    data->used -= 1;
}

static inline void removeAll(HSD_ObjAllocData* data)
{
    HSD_ObjAllocData** cur = &alloc_datas;
    while (*cur != NULL) {
        if (*cur == data) {
            *cur = (*cur)->next;
        } else {
            cur = &(*cur)->next;
        }
    }
}

void HSD_ObjAllocInit(HSD_ObjAllocData* data, size_t size, u32 align)
{
#ifdef MELEE_NATIVE
    HSD_ASSERT(0, !nativeHsdReservation_Active());
    HSD_ASSERTREPORT(0,!nativeHsdArena.sealed,
        "Object allocators must be registered before admitting match history.\n");
    next_epoch();
#endif
    HSD_ASSERT(0x185, data);
    HSD_ASSERT(0, align != 0 && (align & (align - 1)) == 0);
    // Free slots contain a native pointer, even when the payload is smaller.
    if (align < sizeof(HSD_ObjAllocLink)) {
        align = sizeof(HSD_ObjAllocLink);
    }
    if (size < sizeof(HSD_ObjAllocLink)) {
        size = sizeof(HSD_ObjAllocLink);
    }
    HSD_ASSERT(0, size <= U32_MAX - (align - 1));
    if (data != NULL) {
        removeAll(data);
    } else {
        alloc_datas = NULL;
    }
    memset(data, 0, sizeof(HSD_ObjAllocData));
    data->num_limit = -1;
    data->heap_limit_size = 0;
    data->heap_limit_num = -1;
    data->align = align - 1;
    data->size = (u32) ((size + data->align) & ~(size_t) data->align);
    data->next = alloc_datas;
    alloc_datas = data;
}

void _HSD_ObjAllocForgetMemory(void* low, void* high)
{
#ifdef MELEE_NATIVE
    HSD_ASSERT(0, !nativeHsdReservation_Active());
    next_epoch();
    allocation_block_count = 0;
#endif
    alloc_datas = NULL;
}

#ifdef MELEE_NATIVE
static bool snapshot_overlap(const void* a, size_t an, const void* b, size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y || (x<y+bn && y<x+an);
}

static bool descriptor_geometry(const HSD_ObjAllocData* data)
{
    size_t alignment=(size_t)data->align+1;
    return data->size>=sizeof(HSD_ObjAllocLink) && alignment>=sizeof(void*) &&
        !(alignment&(alignment-1)) && data->size%alignment==0;
}

static bool descriptor_state(const HSD_ObjAllocData* data)
{
    return data->peak>=data->used && data->free<=U32_MAX-data->used &&
        ((data->free==0)==(data->freehead==NULL)) &&
        (!data->freehead || !((uintptr_t)data->freehead & data->align));
}

static bool registry(HSD_ObjAllocData** descriptors, uint32_t* count)
{
    if (!allocation_epoch || nativeHsdReservation_Active()) return false;
    *count=0;
    for(HSD_ObjAllocData* data=alloc_datas;data;data=data->next){
        if(*count==NATIVE_OBJALLOC_SNAPSHOT_DESCRIPTORS || !descriptor_geometry(data))return false;
        for(unsigned i=0;i<*count;++i)if(descriptors[i]==data)return false;
        descriptors[(*count)++]=data;
    }
    return true;
}

static bool heap_geometry(const objheap* heap)
{
    if(!heap->top)return heap->curr==0;
    return heap->size<=UINTPTR_MAX-heap->top && heap->curr>=heap->top &&
        heap->curr-heap->top<=heap->size &&
        heap->remain==heap->size-(heap->curr-heap->top);
}

bool nativeObjAlloc_BufferIndependent(const void* buffer,size_t bytes)
{
    if(!buffer || snapshot_overlap(buffer,bytes,&obj_heap,sizeof(obj_heap)) ||
       snapshot_overlap(buffer,bytes,&nativeHsdArena,sizeof(nativeHsdArena)) ||
       snapshot_overlap(buffer,bytes,nativeHsdArena.memory,nativeHsdArena.capacity) ||
       snapshot_overlap(buffer,bytes,&alloc_datas,sizeof(alloc_datas)) ||
       snapshot_overlap(buffer,bytes,&allocation_epoch,sizeof(allocation_epoch)) ||
       snapshot_overlap(buffer,bytes,allocation_blocks,sizeof(allocation_blocks)) ||
       snapshot_overlap(buffer,bytes,&allocation_block_count,sizeof(allocation_block_count)) ||
       (obj_heap.top && snapshot_overlap(buffer,bytes,(void*)obj_heap.top,obj_heap.size)))return false;
    unsigned count=0;
    for(const HSD_ObjAllocData* data=alloc_datas;data;data=data->next)
        if(++count>NATIVE_OBJALLOC_SNAPSHOT_DESCRIPTORS ||
           snapshot_overlap(buffer,bytes,data,sizeof(*data)))return false;
    for(unsigned i=0;i<allocation_block_count;++i)
        if(snapshot_overlap(buffer,bytes,allocation_blocks[i].address,allocation_blocks[i].bytes))return false;
    return true;
}
bool nativeObjAlloc_AdmitArenaHistory(void)
{
    HSD_ObjAllocData* descriptors[NATIVE_OBJALLOC_SNAPSHOT_DESCRIPTORS];uint32_t count;
    if(!allocation_epoch && !alloc_datas)count=0;
    else if(!registry(descriptors,&count))return false;
    if(!heap_geometry(&obj_heap) || allocation_block_count>NATIVE_OBJALLOC_SNAPSHOT_BLOCKS)return false;
    uintptr_t begin=(uintptr_t)nativeHsdArena.memory;
    size_t used=nativeMatchArena_Used(&nativeHsdArena);
    for(unsigned i=0;i<count;++i)if(!descriptor_state(descriptors[i]))return false;
    // Production ordinary chunks come from the shared backing. Explicit fixed
    // ObjSetHeap backing is a separate admission contract and requires its own
    // root; reject earlier unclaimed external chunks rather than omit them.
    for(unsigned i=0;i<allocation_block_count;++i){
        uintptr_t at=(uintptr_t)allocation_blocks[i].address;
        size_t n=allocation_blocks[i].bytes;
        bool internal=at>=begin && at-begin<=used && n<=used-(at-begin);
        bool fixed=obj_heap.top && at>=obj_heap.top && at-obj_heap.top<=obj_heap.size &&
            n<=obj_heap.size-(at-obj_heap.top);
        if(!internal && !fixed)return false;
    }
#define ROOT(value) if(!nativeMatchArena_AddRoot(&nativeHsdArena,&(value),sizeof(value)))return false
    ROOT(obj_heap);ROOT(alloc_datas);ROOT(allocation_epoch);ROOT(allocation_block_count);
#undef ROOT
    if(!nativeMatchArena_AddCountedRoot(&nativeHsdArena,allocation_blocks,sizeof(*allocation_blocks),
        NATIVE_OBJALLOC_SNAPSHOT_BLOCKS,&allocation_block_count))return false;
    bool fixed_internal=obj_heap.top>=begin && obj_heap.top-begin<=used &&
        obj_heap.size<=used-(obj_heap.top-begin);
    if(obj_heap.top && !fixed_internal &&
       !nativeMatchArena_AddRoot(&nativeHsdArena,(void*)obj_heap.top,obj_heap.size))return false;
    for(unsigned i=0;i<count;++i){
        uintptr_t at=(uintptr_t)descriptors[i];
        if(at>=begin && at-begin<=used && sizeof(*descriptors[i])<=used-(at-begin))continue;
        if(!nativeMatchArena_AddRoot(&nativeHsdArena,descriptors[i],sizeof(*descriptors[i])))return false;
    }
    return true;
}

NativeObjAllocReference nativeObjAlloc_Reference(HSD_ObjAllocData* allocator,
                                                 const void* object,size_t bytes)
{
    NativeObjAllocReference invalid={0};
    if(!allocator || !object || !bytes || !allocation_epoch ||
        bytes>UINTPTR_MAX-(uintptr_t)object || nativeHsdReservation_Active())return invalid;
    uintptr_t pointer=(uintptr_t)object;
    for(uint32_t i=0;i<allocation_block_count;++i) {
        const NativeObjAllocSnapshotBlock* block=&allocation_blocks[i];
        if(block->allocator!=allocator || pointer<(uintptr_t)block->address)continue;
        // Descriptor identity is proven before dereferencing the caller's pointer.
        size_t stride=block->allocator->size,offset=pointer-(uintptr_t)block->address;
        if(!stride || block->bytes%stride || offset>=block->bytes || offset%stride ||
            offset/stride>UINT32_MAX || bytes>stride ||
            bytes>block->bytes-offset)continue;
        return (NativeObjAllocReference){allocation_epoch,i,(uint32_t)(offset/stride),allocator,bytes};
    }
    return invalid;
}
void* nativeObjAlloc_Resolve(NativeObjAllocReference reference)
{
    if(!reference.epoch || reference.epoch!=allocation_epoch || reference.block>=allocation_block_count ||
        !reference.allocator || !reference.bytes || nativeHsdReservation_Active())return NULL;
    const NativeObjAllocSnapshotBlock* block=&allocation_blocks[reference.block];
    if(reference.allocator!=block->allocator)return NULL;
    size_t stride=block->allocator->size;
    if(!stride || reference.bytes>stride || block->bytes%stride ||
        reference.slot>=block->bytes/stride)return NULL;
    return (u8*)block->address+(size_t)reference.slot*stride;
}

static size_t snapshot_bytes(uint32_t count)
{
    size_t bytes=sizeof(NativeObjAllocSnapshotHeader)+count*sizeof(NativeObjAllocSnapshotRecord);
    for(uint32_t i=0;i<allocation_block_count;++i){
        size_t n=allocation_blocks[i].bytes;
        if(!allocation_blocks[i].address || !n ||
           n>SIZE_MAX-sizeof(NativeObjAllocSnapshotBlock) ||
           n+sizeof(NativeObjAllocSnapshotBlock)>SIZE_MAX-bytes)return 0;
        bytes+=sizeof(NativeObjAllocSnapshotBlock)+n;
    }
    return bytes;
}

size_t nativeObjAlloc_SnapshotBytes(void)
{
    HSD_ObjAllocData* descriptors[NATIVE_OBJALLOC_SNAPSHOT_DESCRIPTORS];uint32_t count;
    if(!registry(descriptors,&count) || !heap_geometry(&obj_heap))return 0;
    for(unsigned i=0;i<count;++i)if(!descriptor_state(descriptors[i]))return 0;
    return snapshot_bytes(count);
}

bool nativeObjAlloc_Save(void* snapshot,size_t capacity,size_t* written)
{
    HSD_ObjAllocData* descriptors[NATIVE_OBJALLOC_SNAPSHOT_DESCRIPTORS];uint32_t count;
    if(!registry(descriptors,&count) || !heap_geometry(&obj_heap))return false;
    size_t bytes=snapshot_bytes(count);
    if(!bytes || capacity<bytes || !nativeObjAlloc_BufferIndependent(snapshot,capacity) ||
       !nativeObjAlloc_BufferIndependent(written,sizeof(*written)) ||
       snapshot_overlap(snapshot,capacity,written,sizeof(*written)))return false;
    for(unsigned i=0;i<count;++i)if(!descriptor_state(descriptors[i]))return false;
    NativeObjAllocSnapshotHeader h={0};h.registry=alloc_datas;h.heap=obj_heap;
    h.epoch=allocation_epoch;h.count=count;h.blocks=allocation_block_count;
    h.arena_used=nativeMatchArena_Used(&nativeHsdArena);
    if(h.arena_used && (h.arena_used<64 || h.arena_used>nativeHsdArena.capacity))return false;
    uint8_t* cursor=(uint8_t*)snapshot+sizeof(h);
    for(unsigned i=0;i<count;++i){
        NativeObjAllocSnapshotRecord r={0};r.descriptor=descriptors[i];r.state=*descriptors[i];
        memcpy(cursor,&r,sizeof(r));cursor+=sizeof(r);
    }
    for(uint32_t i=0;i<allocation_block_count;++i){
        uintptr_t at=(uintptr_t)allocation_blocks[i].address,base=(uintptr_t)nativeHsdArena.memory;
        if(at>=base && at-base<nativeHsdArena.capacity &&
           (at-base>h.arena_used || allocation_blocks[i].bytes>h.arena_used-(at-base)))return false;
        memcpy(cursor,&allocation_blocks[i],sizeof(allocation_blocks[i]));
        cursor+=sizeof(allocation_blocks[i]);
        memcpy(cursor,allocation_blocks[i].address,allocation_blocks[i].bytes);
        cursor+=allocation_blocks[i].bytes;
    }
    memcpy(snapshot,&h,sizeof(h));*written=bytes;return true;
}

bool nativeObjAlloc_ValidateSnapshot(const void* snapshot,size_t bytes)
{
    HSD_ObjAllocData* descriptors[NATIVE_OBJALLOC_SNAPSHOT_DESCRIPTORS];uint32_t count;
    if(!registry(descriptors,&count) || bytes<sizeof(NativeObjAllocSnapshotHeader) ||
       !nativeObjAlloc_BufferIndependent(snapshot,bytes))return false;
    NativeObjAllocSnapshotHeader h;memcpy(&h,snapshot,sizeof(h));
    if(h.registry!=alloc_datas || h.epoch!=allocation_epoch || h.count!=count ||
       (h.arena_used && (h.arena_used<64 || h.arena_used>nativeHsdArena.capacity)) ||
       h.blocks>allocation_block_count ||
       bytes<sizeof(h)+count*sizeof(NativeObjAllocSnapshotRecord) ||
       h.heap.top!=obj_heap.top || h.heap.size!=obj_heap.size || !heap_geometry(&h.heap))return false;
    const uint8_t* cursor=(const uint8_t*)snapshot+sizeof(h);
    bytes-=sizeof(h)+count*sizeof(NativeObjAllocSnapshotRecord);
    for(unsigned i=0;i<count;++i){
        NativeObjAllocSnapshotRecord r;memcpy(&r,cursor,sizeof(r));cursor+=sizeof(r);
        if(r.descriptor!=descriptors[i] || r.state.next!=descriptors[i]->next ||
           r.state.size!=descriptors[i]->size || r.state.align!=descriptors[i]->align ||
           !descriptor_state(&r.state))return false;
    }
    for(uint32_t i=0;i<h.blocks;++i){
        NativeObjAllocSnapshotBlock r;
        if(bytes<sizeof(r))return false;
        memcpy(&r,cursor,sizeof(r));cursor+=sizeof(r);bytes-=sizeof(r);
        if(r.allocator!=allocation_blocks[i].allocator || r.address!=allocation_blocks[i].address ||
           r.bytes!=allocation_blocks[i].bytes || bytes<r.bytes)return false;
        uintptr_t at=(uintptr_t)r.address,base=(uintptr_t)nativeHsdArena.memory;
        if(at>=base && at-base<nativeHsdArena.capacity &&
           (at-base>h.arena_used || r.bytes>h.arena_used-(at-base)))return false;
        cursor+=r.bytes;bytes-=r.bytes;
    }
    return bytes==0;
}

bool nativeObjAlloc_Load(const void* snapshot,size_t bytes)
{
    if(!nativeObjAlloc_ValidateSnapshot(snapshot,bytes))return false;
    NativeObjAllocSnapshotHeader h;memcpy(&h,snapshot,sizeof(h));
    const uint8_t* cursor=(const uint8_t*)snapshot+sizeof(h);
    for(unsigned i=0;i<h.count;++i){
        NativeObjAllocSnapshotRecord r;memcpy(&r,cursor,sizeof(r));cursor+=sizeof(r);
        *r.descriptor=r.state;
    }
    for(uint32_t i=0;i<h.blocks;++i){
        NativeObjAllocSnapshotBlock r;memcpy(&r,cursor,sizeof(r));cursor+=sizeof(r);
        memcpy(r.address,cursor,r.bytes);cursor+=r.bytes;
    }
    allocation_block_count=h.blocks;obj_heap=h.heap;
    if(h.arena_used)((NativeMatchArenaCursor*)nativeHsdArena.memory)->used=h.arena_used;
    return true;
}
#endif
