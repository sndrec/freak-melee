#include "native_hsd_reservation.h"
#include "native_hsd_reservation_snapshot.h"
#include <melee/gm/native_match_snapshot_pool.h>

#include <limits.h>
#include <string.h>

static NativeHsdReservation* owners[NATIVE_HSD_RESERVATION_OWNERS];
static NativeHsdReservation* active;
static uint32_t remaining[NATIVE_HSD_RESERVATION_BINS];
static bool failed;
// Host lease: bind/release are scene transitions, never rollback-tick work.
static NativeMatchArena* history_arena;
static uint64_t history_lifetime;
static bool history_sealed(void)
{
    return history_arena && history_arena->lifetime == history_lifetime &&
        history_arena->sealed;
}

static bool span_overlap(const void* a, size_t an, const void* b, size_t bn)
{
    uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return an > UINTPTR_MAX - x || bn > UINTPTR_MAX - y ||
           (x < y + bn && y < x + an);
}

static bool shape(const NativeHsdDemand* d, size_t* size, size_t* alignment)
{
    if (!d || !d->count || d->count > NATIVE_MATCH_POOL_MAX_SLOTS) return false;
    if (!d->allocator) {
        if (!d->size || d->size > INT32_MAX - 31) return false;
        *alignment = 32;
        *size = ((size_t)d->size + 31) & ~(size_t)31;
    } else {
        HSD_ObjAllocData* a = d->allocator;
        *alignment = (size_t)a->align + 1;
        *size = a->size;
        // Native heap-limit policy cannot be bypassed by external backing.
        if (a->heap_limit_flag || d->size != a->size || !a->size ||
            *alignment < sizeof(void*) ||
            (*alignment & (*alignment - 1)) || a->size % *alignment) return false;
    }
    return true;
}

static bool append(size_t* offset, size_t alignment, size_t size,
                   uint32_t count, size_t* start)
{
    size_t at = nativeMatchPool_Stride(*offset ? *offset : 1, alignment);
    if (*offset == 0) at = 0;
    if ((!at && *offset) || size > SIZE_MAX / count ||
        size * count > SIZE_MAX - at) return false;
    *start = at;
    *offset = at + size * count;
    return true;
}

size_t nativeHsdReservation_Bytes(const NativeHsdDemand* demand, uint32_t count,
                                 size_t* alignment)
{
    if (!demand || !count || count > NATIVE_HSD_RESERVATION_BINS || !alignment)
        return 0;
    size_t bytes = 0, max_align = 32, ignored;
    for (uint32_t i = 0; i < count; ++i) {
        size_t size, align;
        if (!shape(&demand[i], &size, &align)) return 0;
        if (align > max_align) max_align = align;
        for (uint32_t j = 0; j < i; ++j) {
            size_t other_size, other_align;
            if (!shape(&demand[j], &other_size, &other_align) ||
                (demand[i].allocator == demand[j].allocator &&
                 (demand[i].allocator || size == other_size))) return 0;
        }
        if (!append(&bytes, _Alignof(NativeMatchPoolSlot),
                    sizeof(NativeMatchPoolSlot), demand[i].count, &ignored) ||
            !append(&bytes, _Alignof(uint32_t), sizeof(uint32_t),
                    demand[i].count, &ignored) ||
            !append(&bytes, align, size, demand[i].count, &ignored)) return 0;
    }
    *alignment = max_align;
    return bytes;
}

bool nativeHsdReservation_Bind(NativeHsdReservation* reservation, void* backing,
                              size_t bytes, const NativeHsdDemand* demand,
                              uint32_t count, uint64_t epoch)
{
    size_t align;
    size_t required = nativeHsdReservation_Bytes(demand, count, &align);
    if (history_sealed() || active || !reservation || !backing || !required || bytes < required || !epoch ||
        ((uintptr_t)backing & (align - 1)) ||
        span_overlap(reservation, sizeof(*reservation), backing, bytes) ||
        span_overlap(demand, count * sizeof(*demand), backing, bytes) ||
        span_overlap(demand, count * sizeof(*demand), reservation,
                     sizeof(*reservation))) return false;
    for (uint32_t i = 0; i < count; ++i)
        if (demand[i].allocator &&
            (span_overlap(demand[i].allocator, sizeof(HSD_ObjAllocData), backing, bytes) ||
             span_overlap(demand[i].allocator, sizeof(HSD_ObjAllocData),
                          reservation, sizeof(*reservation)))) return false;
    unsigned free_owner = NATIVE_HSD_RESERVATION_OWNERS;
    for (unsigned i = 0; i < NATIVE_HSD_RESERVATION_OWNERS; ++i) {
        if (!owners[i]) { free_owner = i; continue; }
        if (owners[i] == reservation ||
            span_overlap(backing, bytes, owners[i]->backing,
                         owners[i]->backing_bytes) ||
            span_overlap(reservation, sizeof(*reservation), owners[i],
                         sizeof(*reservation)) ||
            span_overlap(backing, bytes, owners[i], sizeof(*reservation)) ||
            span_overlap(reservation, sizeof(*reservation), owners[i]->backing,
                         owners[i]->backing_bytes)) return false;
    }
    if (free_owner == NATIVE_HSD_RESERVATION_OWNERS) return false;
    // All layout/overlap checks precede writes into caller memory.
    memset(reservation, 0, sizeof(*reservation));
    size_t offset = 0;
    for (uint32_t i = 0; i < count; ++i) {
        size_t size, bin_align, slots, order, payload;
        shape(&demand[i], &size, &bin_align);
        append(&offset, _Alignof(NativeMatchPoolSlot), sizeof(NativeMatchPoolSlot),
               demand[i].count, &slots);
        append(&offset, _Alignof(uint32_t), sizeof(uint32_t), demand[i].count,
               &order);
        append(&offset, bin_align, size, demand[i].count, &payload);
        NativeHsdReservationBin* bin = &reservation->bins[i];
        bin->allocator = demand[i].allocator;
        bin->alignment = bin_align;
        // Guaranteed by the preceding complete sizing and disjoint layout pass.
        if (!nativeMatchPool_Bind(&bin->pool, (uint8_t*)backing + payload,
                                 size * demand[i].count,
                                 (void*)((uint8_t*)backing + slots),
                                 (void*)((uint8_t*)backing + order),
                                 demand[i].count, size, bin_align, epoch)) return false;
    }
    reservation->backing = backing;
    reservation->backing_bytes = bytes;
    reservation->count = count;
    owners[free_owner] = reservation;
    return true;
}

static int registered(NativeHsdReservation* reservation)
{
    for (unsigned i = 0; i < NATIVE_HSD_RESERVATION_OWNERS; ++i)
        if (owners[i] && owners[i] == reservation) return (int)i;
    return -1;
}

bool nativeHsdReservation_Release(NativeHsdReservation* reservation)
{
    int i = registered(reservation);
    if (history_sealed() || i < 0 || active == reservation) return false;
    for (uint32_t j = 0; j < reservation->count; ++j)
        if (reservation->bins[j].pool.used) return false;
    owners[i] = NULL;
    memset(reservation, 0, sizeof(*reservation));
    return true;
}

static int find_bin(NativeHsdReservation* reservation, HSD_ObjAllocData* allocator,
                    size_t size)
{
    for (uint32_t i = 0; i < reservation->count; ++i) {
        NativeHsdReservationBin* bin = &reservation->bins[i];
        if (bin->allocator == allocator && bin->pool.object_size == size)
            return (int)i;
    }
    return -1;
}

bool nativeHsdReservation_Begin(NativeHsdReservation* reservation,
                               const NativeHsdDemand* demand, uint32_t count)
{
    uint32_t quotas[NATIVE_HSD_RESERVATION_BINS] = { 0 };
    if (active || registered(reservation) < 0 || !demand || !count ||
        count > NATIVE_HSD_RESERVATION_BINS) return false;
    for (uint32_t i = 0; i < count; ++i) {
        size_t size, align;
        if (!shape(&demand[i], &size, &align)) return false;
        int bin = find_bin(reservation, demand[i].allocator, size);
        if (bin < 0 || quotas[bin] ||
            reservation->bins[bin].pool.free_count < demand[i].count) return false;
        HSD_ObjAllocData* a = demand[i].allocator;
        if (a && (reservation->bins[bin].alignment != align ||
                  reservation->bins[bin].pool.stride != a->size ||
                  (a->num_limit_flag &&
                   (a->used > a->num_limit ||
                    demand[i].count > a->num_limit - a->used)))) return false;
        quotas[bin] = demand[i].count;
    }
    memcpy(remaining, quotas, sizeof(remaining));
    failed = false;
    active = reservation;
    return true;
}

bool nativeHsdReservation_End(NativeHsdReservation* reservation)
{
    if (!active || active != reservation) return false;
    bool ok = !failed;
    active = NULL;
    memset(remaining, 0, sizeof(remaining));
    return ok;
}

bool nativeHsdReservation_Active(void) { return active != NULL; }
bool nativeHsdReservation_Failed(void) { return failed; }
bool nativeHsdReservation_RejectHeap(void)
{
    if (!active) return false;
    failed = true;
    return true;
}

void* nativeHsdReservation_Allocate(HSD_ObjAllocData* allocator, uint32_t size,
                                    bool* handled)
{
    *handled = active != NULL;
    if (!active) return NULL;
    size_t rounded = allocator ? size : ((size_t)size + 31) & ~(size_t)31;
    int i = find_bin(active, allocator, rounded);
    if (failed || !size || i < 0 || !remaining[i] ||
        (allocator && (allocator->heap_limit_flag ||
                       allocator->size != size ||
                       allocator->align + (size_t)1 != active->bins[i].alignment ||
                       (allocator->num_limit_flag &&
                        allocator->used >= allocator->num_limit)))) {
        failed = true;
        return NULL;
    }
    NativeMatchPoolReference reference;
    void* result = nativeMatchPool_Allocate(&active->bins[i].pool, &reference);
    if (!result) { failed = true; return NULL; }
    --remaining[i];
    if (allocator) {
        ++allocator->used;
        if (allocator->used > allocator->peak) allocator->peak = allocator->used;
    }
    return result;
}

int nativeHsdReservation_Free(HSD_ObjAllocData* allocator, void* object,
                               uint32_t size)
{
    uintptr_t address = (uintptr_t)object;
    for (unsigned o = 0; o < NATIVE_HSD_RESERVATION_OWNERS; ++o) {
        NativeHsdReservation* reservation = owners[o];
        if (!reservation || address < (uintptr_t)reservation->backing ||
            address - (uintptr_t)reservation->backing >=
                reservation->backing_bytes) continue;
        size_t rounded = allocator ? size : ((size_t)size + 31) & ~(size_t)31;
        int i = find_bin(reservation, allocator, rounded);
        NativeMatchPoolReference reference;
        if (i < 0 || (allocator && !allocator->used) ||
            !nativeMatchPool_Reference(&reservation->bins[i].pool, object,
                                        &reference) ||
            !nativeMatchPool_Free(&reservation->bins[i].pool, reference)) return -1;
        if (allocator) --allocator->used;
        return 1;
    }
    return 0;
}

static bool contained(const NativeHsdReservation* owner, const void* p, size_t n)
{
    uintptr_t base = (uintptr_t)owner->backing, at = (uintptr_t)p;
    return p && at >= base && owner->backing_bytes <= UINTPTR_MAX - base &&
           at - base <= owner->backing_bytes &&
           n <= owner->backing_bytes - (at - base);
}

// Only immutable geometry is required for load: mutable metadata may have
// changed since saving, including generation reuse and occupancy. Never follow
// candidate payload pointers here; graph closure belongs to aggregate owners.
static bool snapshot_boundary(void)
{
    if (active) return false;
    for (unsigned i = 0; i < NATIVE_HSD_RESERVATION_BINS; ++i)
        if (remaining[i]) return false;
    for (unsigned o = 0; o < NATIVE_HSD_RESERVATION_OWNERS; ++o) {
        const NativeHsdReservation* owner = owners[o];
        if (!owner) continue;
        if (!owner->backing || !owner->count ||
            owner->count > NATIVE_HSD_RESERVATION_BINS) return false;
        for (unsigned i = 0; i < owner->count; ++i) {
            const NativeHsdReservationBin* bin = &owner->bins[i];
            const NativeMatchPool* p = &bin->pool;
            if (!p->capacity || p->capacity > NATIVE_MATCH_POOL_MAX_SLOTS ||
                !p->lifetime || !bin->alignment ||
                (bin->alignment & (bin->alignment - 1)) ||
                !p->object_size || p->stride != nativeMatchPool_Stride(p->object_size,bin->alignment) ||
                p->stride > SIZE_MAX / p->capacity ||
                !contained(owner,p->payload,p->stride * p->capacity) ||
                !contained(owner,p->slots,sizeof(*p->slots) * p->capacity) ||
                !contained(owner,p->free_order,sizeof(*p->free_order) * p->capacity) ||
                ((uintptr_t)p->payload & (bin->alignment - 1)) ||
                (uintptr_t)p->slots % _Alignof(NativeMatchPoolSlot) ||
                (uintptr_t)p->free_order % _Alignof(uint32_t)) return false;
            if (bin->allocator && (bin->allocator->size != p->object_size ||
                (size_t)bin->allocator->align + 1 != bin->alignment)) return false;
        }
    }
    return true;
}

bool nativeHsdReservation_BufferIndependent(const void* buffer, size_t bytes)
{
    if (!buffer || span_overlap(buffer,bytes,owners,sizeof(owners)) ||
        span_overlap(buffer,bytes,&active,sizeof(active)) ||
        span_overlap(buffer,bytes,remaining,sizeof(remaining)) ||
        span_overlap(buffer,bytes,&failed,sizeof(failed))) return false;
    for (unsigned o = 0; o < NATIVE_HSD_RESERVATION_OWNERS; ++o) {
        const NativeHsdReservation* owner = owners[o];
        if (!owner) continue;
        if (span_overlap(buffer,bytes,owner,sizeof(*owner)) ||
            span_overlap(buffer,bytes,owner->backing,owner->backing_bytes)) return false;
        for (unsigned i = 0; i < owner->count; ++i)
            if (owner->bins[i].allocator && span_overlap(buffer,bytes,
                    owner->bins[i].allocator,sizeof(HSD_ObjAllocData))) return false;
    }
    return true;
}

size_t nativeHsdReservation_SnapshotBytes(void)
{
    if (!snapshot_boundary()) return 0;
    size_t bytes = sizeof(NativeHsdReservationSnapshotHeader);
    for (unsigned o = 0; o < NATIVE_HSD_RESERVATION_OWNERS; ++o) {
        const NativeHsdReservation* owner = owners[o];
        if (!owner) continue;
        if (sizeof(NativeHsdReservationSnapshotOwner) > SIZE_MAX - bytes) return 0;
        bytes += sizeof(NativeHsdReservationSnapshotOwner);
        for (unsigned i = 0; i < owner->count; ++i) {
            size_t n = nativeMatchSnapshotPool_Bytes(&owner->bins[i].pool);
            if (!n || sizeof(NativeHsdReservationSnapshotBin) > SIZE_MAX - bytes ||
                n > SIZE_MAX - bytes - sizeof(NativeHsdReservationSnapshotBin)) return 0;
            bytes += sizeof(NativeHsdReservationSnapshotBin) + n;
        }
    }
    return bytes;
}

bool nativeHsdReservation_Save(void* snapshot, size_t capacity, size_t* written)
{
    if (!snapshot_boundary() || capacity < sizeof(NativeHsdReservationSnapshotHeader) ||
        !nativeHsdReservation_BufferIndependent(snapshot,capacity) ||
        !nativeHsdReservation_BufferIndependent(written,sizeof(*written)) ||
        span_overlap(snapshot,capacity,written,sizeof(*written))) return false;
    NativeHsdReservationSnapshotHeader h = {0,failed ? 1u : 0u};
    uint8_t* cursor = (uint8_t*)snapshot + sizeof(h);
    size_t left = capacity - sizeof(h);
    for (unsigned o = 0; o < NATIVE_HSD_RESERVATION_OWNERS; ++o) {
        const NativeHsdReservation* owner = owners[o];
        if (!owner) continue;
        NativeHsdReservationSnapshotOwner r = {0};
        r.owner = owner; r.backing = owner->backing;
        r.backing_bytes = owner->backing_bytes; r.slot = o; r.bins = owner->count;
        if (left < sizeof(r)) return false;
        memcpy(cursor,&r,sizeof(r)); cursor += sizeof(r); left -= sizeof(r); ++h.owners;
        for (unsigned i = 0; i < owner->count; ++i) {
            const NativeHsdReservationBin* bin = &owner->bins[i];
            NativeHsdReservationSnapshotBin b = {0};
            b.allocator = bin->allocator; b.alignment = bin->alignment;
            if (left < sizeof(b)) return false;
            uint8_t* bin_header = cursor;
            cursor += sizeof(b); left -= sizeof(b);
            // Each pool writer validates its live metadata and copies it once.
            // Avoid sizing the whole registry and then sizing every bin again.
            if (!nativeMatchSnapshotPool_Save(&bin->pool,cursor,left,&b.bytes)) return false;
            memcpy(bin_header,&b,sizeof(b));
            cursor += b.bytes; left -= b.bytes;
        }
    }
    // Publish only after every bin succeeds. A refused late bin may overwrite
    // scratch bytes, but never publishes a size or changes live pool state.
    memcpy(snapshot,&h,sizeof(h)); *written = capacity - left;
    return true;
}

bool nativeHsdReservation_ValidateSnapshot(const void* snapshot, size_t bytes)
{
    if (!snapshot_boundary() || bytes < sizeof(NativeHsdReservationSnapshotHeader) ||
        !nativeHsdReservation_BufferIndependent(snapshot,bytes)) return false;
    NativeHsdReservationSnapshotHeader h; memcpy(&h,snapshot,sizeof(h));
    if (h.owners > NATIVE_HSD_RESERVATION_OWNERS || h.failed > 1) return false;
    uint32_t count = 0;
    const uint8_t* cursor = (const uint8_t*)snapshot + sizeof(h);
    size_t left = bytes - sizeof(h);
    for (unsigned o = 0; o < NATIVE_HSD_RESERVATION_OWNERS; ++o) {
        const NativeHsdReservation* owner = owners[o];
        if (!owner) continue;
        if (left < sizeof(NativeHsdReservationSnapshotOwner)) return false;
        NativeHsdReservationSnapshotOwner r; memcpy(&r,cursor,sizeof(r));
        cursor += sizeof(r); left -= sizeof(r); ++count;
        if (r.slot != o || r.owner != owner || r.backing != owner->backing ||
            r.backing_bytes != owner->backing_bytes || r.bins != owner->count) return false;
        for (unsigned i = 0; i < owner->count; ++i) {
            if (left < sizeof(NativeHsdReservationSnapshotBin)) return false;
            NativeHsdReservationSnapshotBin b; memcpy(&b,cursor,sizeof(b));
            cursor += sizeof(b); left -= sizeof(b);
            const NativeHsdReservationBin* bin = &owner->bins[i];
            if (b.allocator != bin->allocator || b.alignment != bin->alignment ||
                b.bytes > left || !nativeMatchSnapshotPool_View(&bin->pool,cursor,b.bytes).records) return false;
            cursor += b.bytes; left -= b.bytes;
        }
    }
    return count == h.owners && left == 0;
}

bool nativeHsdReservation_Load(const void* snapshot, size_t bytes)
{
    // Validate all owners before writing any. No heap, constructor, destructor,
    // script callback or reference-count changes during this direct restore.
    if (!nativeHsdReservation_ValidateSnapshot(snapshot,bytes)) return false;
    NativeHsdReservationSnapshotHeader h; memcpy(&h,snapshot,sizeof(h));
    const uint8_t* cursor = (const uint8_t*)snapshot + sizeof(h);
    for (unsigned o = 0; o < NATIVE_HSD_RESERVATION_OWNERS; ++o) {
        NativeHsdReservation* owner = owners[o];
        if (!owner) continue;
        cursor += sizeof(NativeHsdReservationSnapshotOwner);
        for (unsigned i = 0; i < owner->count; ++i) {
            NativeHsdReservationSnapshotBin b; memcpy(&b,cursor,sizeof(b));
            cursor += sizeof(b);
            if (!nativeMatchSnapshotPool_Load(&owner->bins[i].pool,cursor,b.bytes)) return false;
            cursor += b.bytes;
        }
    }
    failed = h.failed != 0;
    return true;
}

// Copy actual pool descriptors/free order and initialized payload prefixes.
// Shared ObjAlloc descriptors are admitted by their own allocator owner.
bool nativeHsdReservation_AdmitHistory(NativeMatchArena* arena)
{
    if (!snapshot_boundary() || !arena || !arena->lifetime || arena->sealed)
        return false;
    for (unsigned o=0;o<NATIVE_HSD_RESERVATION_OWNERS;++o) {
        NativeHsdReservation* owner=owners[o];
        if (!owner) continue;
        for (unsigned i=0;i<owner->count;++i)
            if (!nativeMatchPool_Validate(&owner->bins[i].pool)) return false;
    }
#define ROOT(value) do { if (!nativeMatchArena_AddRoot(arena,&(value),sizeof(value))) return false; } while (0)
    ROOT(owners); ROOT(active); ROOT(remaining); ROOT(failed);
#undef ROOT
    for (unsigned o=0;o<NATIVE_HSD_RESERVATION_OWNERS;++o) {
        NativeHsdReservation* owner=owners[o];
        if (!owner) continue;
        if (!nativeMatchArena_AddRoot(arena,owner,sizeof(*owner))) return false;
        for (unsigned i=0;i<owner->count;++i) {
            NativeMatchPool* pool=&owner->bins[i].pool;
            if (!nativeMatchArena_AddRoot(arena,pool->slots,sizeof(*pool->slots)*pool->capacity) ||
                !nativeMatchArena_AddRoot(arena,pool->free_order,sizeof(*pool->free_order)*pool->capacity) ||
                !nativeMatchArena_AddCountedRoot(arena,pool->payload,pool->stride,
                    pool->capacity,&pool->initialized_count)) return false;
        }
    }
    history_arena=arena; history_lifetime=arena->lifetime;
    return true;
}
