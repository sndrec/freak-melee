#include <melee/it/native_item_link_pool.h>
// Complete production userdata destructor extracted from itlinkhookshot.c.
#include "link_body.inc"

static ItemLink* create_link(void)
{
    HSD_GObj* root = GObj_Create(HSD_GOBJ_CLASS_ITEMLINK, 10, 0);
    if (!root) return NULL;
    NativeMatchPoolReference reference;
    ItemLink* link = nativeMatchPool_Allocate(&nativeItemLinkPool, &reference);
    if (!link) { HSD_GObjFree(root); return NULL; }
    ItemLink zero = { 0 };
    CHECK(memcmp(link, &zero, sizeof(zero)) == 0);
    link->gobj = root;
    GObj_InitUserData(root, 6, it_802A2474, link);
    return link;
}
static void deferred_link_remove(HSD_GObj* root)
{
    size_t bytes = 654;
    CHECK(!nativeItemLinkPool_Save(after, buffer_bytes, &bytes) && bytes == 654);
    CHECK(!nativeItemLinkPool_RetireScene());
    unsigned used = nativeItemLinkPool.used;
    HSD_GObjFree(root);
    CHECK(nativeItemLinkPool.used == used && HSD_GObj_DelayedProcInfo.delay_remove_gobj);
}
static void link_lifecycle(void)
{
    initialize();
    CHECK(nativeItemLinkPool_Ready() && nativeItemLinkPool.used == 0);
    ItemLink* link = create_link();
    NativeMatchPoolReference old, current;
    CHECK(nativeMatchPool_Reference(&nativeItemLinkPool, link, &old));
    HSD_GObj* root = link->gobj;
    memset(&link->coll_data, 0xAB, sizeof(link->coll_data));
    HSD_GObjFree(root);
    CHECK(nativeMatchPool_Resolve(old) == NULL && nativeItemLinkPool.used == 0);
    link = create_link();
    CHECK(link->gobj == root && nativeMatchPool_Reference(&nativeItemLinkPool, link, &current));
    CHECK(current.slot == old.slot && current.generation == old.generation + 1);
    GObj_RemoveUserData(root);
    CHECK(nativeItemLinkPool.used == 0 && root->user_data == NULL);
    HSD_GObjFree(root);
    link = create_link();
    HSD_GObj_SetupProc(link->gobj, deferred_link_remove, 0);
    HSD_GObj_80390CFC();
    CHECK(nativeItemLinkPool.used == 0 && nativeGObjProcPool.used == 0);
    CHECK(nativeMatchPool_Validate(&nativeItemLinkPool));
    ++cases;
}
static void reject_link(const void* candidate, size_t bytes)
{
    size_t before_bytes, after_bytes;
    CHECK(nativeItemLinkPool_Save(after, buffer_bytes, &before_bytes));
    CHECK(!nativeItemLinkPool_ValidateSnapshot(candidate, bytes));
    CHECK(!nativeItemLinkPool_Load(candidate, bytes));
    CHECK(nativeItemLinkPool_Save(invalid, buffer_bytes, &after_bytes));
    CHECK(before_bytes == after_bytes && !memcmp(after, invalid, after_bytes));
}
static size_t link_sparse_bytes, link_full_bytes;
static void link_restore(void)
{
    initialize();
    ItemLink* a = create_link();
    ItemLink* b = create_link();
    ItemLink* c = create_link();
    a->prev = c; c->next = a;
    a->pos = (Vec3){ 21, -9, 8 }; a->vel = (Vec3){ 1, 2, 3 };
    a->coll_data.contact = (Vec3){ 55, 7, 2 }; a->x2C_b7 = 1; a->x1CC = -2;
    a->jobj = (HSD_JObj*)c->gobj; // Borrowed graph token; this family preserves it.
    HSD_GObjFree(b->gobj);
    ItemLink expected = *a;
    NativeMatchPoolReference reference, future;
    CHECK(nativeMatchPool_Reference(&nativeItemLinkPool, a, &reference));
    size_t object_bytes;
    CHECK(nativeItemLinkPool_Save(saved, buffer_bytes, &link_sparse_bytes));
    CHECK(nativeMatchSnapshotPool_Save(&nativeGObjPool, object_saved, sizeof(object_saved), &object_bytes));
    CHECK(link_sparse_bytes == sizeof(NativeMatchPoolSnapshotHeader) +
        (2 * NATIVE_ITEM_LINK_CAPACITY - 2) * sizeof(uint32_t) + 2 * (sizeof(uint32_t) + sizeof(ItemLink)));
    NativeRollbackStateRegion region;
    CHECK(nativeRollbackState_GetGlobalRegions(&region, 1) == 1);
    oracle = malloc(region.size); CHECK(oracle); memcpy(oracle, region.memory, region.size);
    HSD_GObj* root = a->gobj; HSD_GObjFree(root);
    a = create_link(); CHECK(a->gobj == root);
    CHECK(nativeMatchPool_Reference(&nativeItemLinkPool, a, &future));
    a->pos.x = -1;
    CHECK(nativeItemLinkPool_ValidateSnapshot(saved, link_sparse_bytes));
    CHECK(nativeItemLinkPool_Load(saved, link_sparse_bytes));
    CHECK(nativeMatchPool_Resolve(reference) == a && nativeMatchPool_Resolve(future) == NULL);
    CHECK(!memcmp(a, &expected, sizeof(expected)) && a->prev == c && c->next == a);
    CHECK(nativeMatchSnapshotPool_Load(&nativeGObjPool, object_saved, object_bytes));
    CHECK(a->gobj->user_data == a);
    // Restore scheduler tables with the independent broad oracle until aggregate
    // ownership closes. This is record acceptance, not whole-match rollback.
    memcpy(region.memory, oracle, region.size); free(oracle); oracle = NULL;
    NativeMatchPoolSnapshotHeader header;
    memcpy(&header, saved, sizeof(header));
    size_t payload_offset = sizeof(header) + (header.capacity + header.free_count) * sizeof(uint32_t) + sizeof(uint32_t);
    uint8_t* candidate = malloc(link_sparse_bytes); CHECK(candidate);
    ItemLink* bad_neighbors[] = { (ItemLink*)1, (ItemLink*)(nativeItemLinkPool.payload + 1), b,
        (ItemLink*)(nativeItemLinkPool.payload + nativeItemLinkPool.capacity * nativeItemLinkPool.stride) };
    for (unsigned i = 0; i < 4; ++i) {
        memcpy(candidate, saved, link_sparse_bytes);
        memcpy(candidate + payload_offset + offsetof(ItemLink, prev), &bad_neighbors[i], sizeof(bad_neighbors[i]));
        reject_link(candidate, link_sparse_bytes); ++cases;
    }
    HSD_GObj* bad_root = (HSD_GObj*)1;
    memcpy(candidate, saved, link_sparse_bytes);
    memcpy(candidate + payload_offset + offsetof(ItemLink, gobj), &bad_root, sizeof(bad_root));
    reject_link(candidate, link_sparse_bytes); ++cases;
    // Two occupied records cannot claim the same root.
    memcpy(candidate, saved, link_sparse_bytes);
    memcpy(candidate + payload_offset + sizeof(ItemLink) + sizeof(uint32_t) + offsetof(ItemLink, gobj), &a->gobj, sizeof(a->gobj));
    reject_link(candidate, link_sparse_bytes); ++cases;
    reject_link(saved, link_sparse_bytes - 1); ++cases;
    free(candidate);
    initialize();
    CHECK(!nativeItemLinkPool_Load(saved, link_sparse_bytes) && nativeMatchPool_Resolve(reference) == NULL);
    ++cases;
}
static void link_exhaustion(void)
{
    initialize();
    for (unsigned i = 0; i < NATIVE_ITEM_LINK_CAPACITY; ++i) CHECK(create_link());
    CHECK(nativeItemLinkPool.used == NATIVE_GOBJ_CAPACITY && create_link() == NULL);
    CHECK(nativeItemLinkPool_Save(saved, buffer_bytes, &link_full_bytes));
    CHECK(link_full_bytes == sizeof(NativeMatchPoolSnapshotHeader) +
        NATIVE_ITEM_LINK_CAPACITY * (sizeof(ItemLink) + 2 * sizeof(uint32_t)));
    while (HSD_GObjPLinkHead[10]) HSD_GObjFree(HSD_GObjPLinkHead[10]);
    size_t bytes;
    CHECK(nativeItemLinkPool.used == 0 && nativeItemLinkPool_Save(saved, buffer_bytes, &bytes));
    CHECK(bytes == sizeof(NativeMatchPoolSnapshotHeader) + 2 * NATIVE_ITEM_LINK_CAPACITY * sizeof(uint32_t));
    CHECK(nativeItemLinkPool_Load(saved, bytes)); ++cases;
}
