#ifndef HSD_NATIVE_RESERVATION_H
#define HSD_NATIVE_RESERVATION_H

#include <melee/gm/native_match_pool.h>
#include <melee/gm/native_match_arena.h>
#include "objalloc.h"

enum { NATIVE_HSD_RESERVATION_BINS = 96, NATIVE_HSD_RESERVATION_OWNERS = 16 };

// NULL allocator identifies the class-piece allocator, rounded to 32 bytes.
// ObjAlloc entries freeze the actual descriptor's size/alignment at binding.
typedef struct NativeHsdDemand {
    HSD_ObjAllocData* allocator;
    uint32_t size, count;
} NativeHsdDemand;

typedef struct NativeHsdReservationBin {
    HSD_ObjAllocData* allocator;
    NativeMatchPool pool;
    size_t alignment;
} NativeHsdReservationBin;

typedef struct NativeHsdReservation {
    NativeHsdReservationBin bins[NATIVE_HSD_RESERVATION_BINS];
    void* backing;
    size_t backing_bytes;
    uint32_t count;
} NativeHsdReservation;

// Explicit transition only. Caller owns both struct and backing; neither may
// move or be freed while registered, occupied, claimed, or retained by history.
// These APIs do not register broad snapshot regions. The explicit snapshot below
// owns reserved records and pool metadata; shared HSD allocator/class counters
// and graph references require companion owners in an aggregate snapshot.
size_t nativeHsdReservation_Bytes(const NativeHsdDemand*, uint32_t count,
                                 size_t* alignment);
bool nativeHsdReservation_Bind(NativeHsdReservation*, void*, size_t,
                              const NativeHsdDemand*, uint32_t, uint64_t epoch);
bool nativeHsdReservation_Release(NativeHsdReservation*);
// A construction claim consumes total allocation counts, including allocations
// freed during construction. Unknown/exhausted allocation never falls back.
bool nativeHsdReservation_Begin(NativeHsdReservation*, const NativeHsdDemand*,
                               uint32_t count);
bool nativeHsdReservation_End(NativeHsdReservation*);
bool nativeHsdReservation_Active(void);
bool nativeHsdReservation_Failed(void);
// Direct heap calls are unaccounted construction demand, never a growth escape.
bool nativeHsdReservation_RejectHeap(void);

// Direct allocator entry points; handled=false means the ordinary allocator.
void* nativeHsdReservation_Allocate(HSD_ObjAllocData*, uint32_t size,
                                    bool* handled);
// 0: ordinary address; 1: freed reserved address; -1: invalid reserved free.
int nativeHsdReservation_Free(HSD_ObjAllocData*, void*, uint32_t size);

// Private scene-local snapshot at an idle construction boundary. Reservations,
// descriptors and backing remain bound at stable addresses for history's life.
// Copies occupied records, generations and exact free order for ALL reservations.
// Does not own shared ObjAlloc descriptors, class counts, ordinary HSD heaps or
// borrowed graph references. Restore those companion owners before callbacks.
size_t nativeHsdReservation_SnapshotBytes(void);
bool nativeHsdReservation_BufferIndependent(const void*, size_t bytes);
// Success guarantees a valid own capture. Failure can change scratch bytes but
// leaves written and live state unchanged; history must not publish that buffer.
bool nativeHsdReservation_Save(void*, size_t capacity, size_t* written);
bool nativeHsdReservation_ValidateSnapshot(const void*, size_t bytes);
bool nativeHsdReservation_Load(const void*, size_t bytes);

// Freeze registry ownership for private copy-only scene history.
bool nativeHsdReservation_AdmitHistory(NativeMatchArena*);

#endif
