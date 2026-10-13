#include "native_item_link_pool.h"
#ifdef MELEE_NATIVE
#include <melee/gm/native_match_snapshot_pool.h>
#include <melee/gm/native_match_arena.h>
#include <melee/ft/native_script_runtime.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <string.h>

// Payload lives in shared reserved actor storage. Scene history copies its
// initialized prefix plus full allocator metadata and free order. The broad
// oracle also tracks the backing while other match owners migrate.
NativeMatchPool nativeItemLinkPool;
static ItemLink* links;
static NativeMatchPoolSlot metadata[NATIVE_ITEM_LINK_CAPACITY];
static uint32_t free_order[NATIVE_ITEM_LINK_CAPACITY];
_Static_assert(NATIVE_ITEM_LINK_CAPACITY % 8 == 0 && NATIVE_GOBJ_CAPACITY % 8 == 0,
    "Snapshot occupancy bitsets require whole bytes");

static bool boundary(void)
{
    return !HSD_GObj_CurrentInvokedProcGObj && !HSD_GObj_CurrentInvokedProc &&
        !HSD_GObj_804D7818 && !HSD_GObj_DelayedProcInfo.flags && nativeScriptRuntime_Idle();
}
bool nativeItemLinkPool_Ready(void)
{
    return nativeItemLinkPool.payload == (uint8_t*)links &&
        nativeItemLinkPool.slots == metadata && nativeItemLinkPool.free_order == free_order &&
        nativeItemLinkPool.capacity == NATIVE_ITEM_LINK_CAPACITY &&
        nativeItemLinkPool.object_size == sizeof(ItemLink) && nativeItemLinkPool.stride == sizeof(ItemLink) &&
        nativeItemLinkPool.lifetime && nativeItemLinkPool.lifetime == nativeGObjPool.lifetime &&
        nativeItemLinkPool.used <= NATIVE_ITEM_LINK_CAPACITY &&
        nativeItemLinkPool.free_count == NATIVE_ITEM_LINK_CAPACITY - nativeItemLinkPool.used;
}
bool nativeItemLinkPool_CanBeginScene(uint64_t epoch)
{
    return boundary() && epoch && !nativeItemLinkPool.lifetime && !nativeItemLinkPool.used;
}
bool nativeItemLinkPool_BeginScene(uint64_t epoch)
{
    if (!nativeItemLinkPool_CanBeginScene(epoch)) return false;
    links=nativeMatchActorStorage.item_links;
    if(!links || !nativeMatchArena_AddRoot(&nativeMatchSceneArena,&nativeItemLinkPool,sizeof(nativeItemLinkPool)) ||
       !nativeMatchArena_AddRoot(&nativeMatchSceneArena,metadata,sizeof(metadata)) ||
       !nativeMatchArena_AddRoot(&nativeMatchSceneArena,free_order,sizeof(free_order)))return false;
    return nativeMatchPool_Bind(&nativeItemLinkPool, links, (NATIVE_ITEM_LINK_CAPACITY*sizeof(*links)), metadata, free_order,
        NATIVE_ITEM_LINK_CAPACITY, sizeof(ItemLink), _Alignof(ItemLink), epoch) &&
        nativeMatchArena_AddCountedRoot(&nativeMatchSceneArena,links,sizeof(*links),
            NATIVE_ITEM_LINK_CAPACITY,&nativeItemLinkPool.initialized_count);
}
bool nativeItemLinkPool_RetireScene(void)
{
    if (!boundary()) return false;
    nativeItemLinkPool.lifetime = 0;
    nativeItemLinkPool.used = nativeItemLinkPool.free_count = 0;
    return true;
}
static bool slot_of(const NativeMatchPool* pool, const void* pointer, uint32_t* slot)
{
    if (!pointer || !pool->payload || !pool->stride ||
        (uintptr_t)pointer < (uintptr_t)pool->payload) return false;
    uintptr_t offset = (uintptr_t)pointer - (uintptr_t)pool->payload;
    if (offset % pool->stride || offset / pool->stride >= pool->capacity) return false;
    *slot = (uint32_t)(offset / pool->stride);
    return true;
}
bool nativeItemLinkPool_Save(void* snapshot, size_t capacity, size_t* written)
{
    if (!boundary() || !nativeItemLinkPool_Ready() || !nativeMatchPool_Validate(&nativeItemLinkPool)) return false;
    for (unsigned i = 0; i < NATIVE_ITEM_LINK_CAPACITY; ++i) if (metadata[i].occupied) {
        ItemLink* link = &links[i];
        NativeMatchPoolReference reference;
        if (!nativeMatchPool_Reference(&nativeGObjPool, link->gobj, &reference) ||
            nativeMatchPool_Resolve(reference) != link->gobj ||
            link->gobj->classifier != HSD_GOBJ_CLASS_ITEMLINK || link->gobj->user_data_kind != 6 ||
            link->gobj->user_data != link || !link->gobj->user_data_remove_func) return false;
        const ItemLink* neighbors[] = { link->prev, link->next };
        for (unsigned n = 0; n < 2; ++n)
            if (neighbors[n] && (!nativeMatchPool_Reference(&nativeItemLinkPool, neighbors[n], &reference) ||
                                nativeMatchPool_Resolve(reference) != neighbors[n])) return false;
    }
    return nativeMatchSnapshotPool_Save(&nativeItemLinkPool, snapshot, capacity, written);
}
bool nativeItemLinkPool_ValidateSnapshot(const void* snapshot, size_t bytes)
{
    if (!boundary() || !nativeItemLinkPool_Ready() ||
        !nativeMatchSnapshotPool_View(&nativeItemLinkPool, snapshot, bytes).records) return false;
    NativeMatchPoolSnapshotHeader header;
    memcpy(&header, snapshot, sizeof(header));
    const uint8_t* records = (const uint8_t*)snapshot + sizeof(header) +
        (header.capacity + header.free_count) * sizeof(uint32_t);
    uint8_t occupied[NATIVE_ITEM_LINK_CAPACITY / 8] = { 0 };
    uint8_t roots[NATIVE_GOBJ_CAPACITY / 8] = { 0 };
    size_t record_bytes = sizeof(uint32_t) + sizeof(ItemLink);
    for (unsigned i = 0; i < header.used; ++i) {
        uint32_t slot;
        memcpy(&slot, records + i * record_bytes, sizeof(slot));
        occupied[slot / 8] |= (uint8_t)(1u << (slot % 8));
    }
    // Check stable pointer geometry without dereferencing candidate roots: their
    // occupancy may differ from the present timeline. GObj/model/registry state
    // must be restored by their owners in the eventual aggregate.
    for (unsigned i = 0; i < header.used; ++i) {
        ItemLink link;
        uint32_t slot;
        memcpy(&link, records + i * record_bytes + sizeof(uint32_t), sizeof(link));
        if (!slot_of(&nativeGObjPool, link.gobj, &slot) || (roots[slot / 8] & (1u << (slot % 8)))) return false;
        roots[slot / 8] |= (uint8_t)(1u << (slot % 8));
        const ItemLink* neighbors[] = { link.prev, link.next };
        for (unsigned n = 0; n < 2; ++n)
            if (neighbors[n] && (!slot_of(&nativeItemLinkPool, neighbors[n], &slot) ||
                                !(occupied[slot / 8] & (1u << (slot % 8))))) return false;
    }
    return true;
}
bool nativeItemLinkPool_Load(const void* snapshot, size_t bytes)
{
    if (!nativeItemLinkPool_ValidateSnapshot(snapshot, bytes)) return false;
    return nativeMatchSnapshotPool_Load(&nativeItemLinkPool, snapshot, bytes);
}
#endif
