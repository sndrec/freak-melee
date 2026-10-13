#include <melee/gm/native_match_control_history.h>
#include "memory.h"

#include <Runtime/platform.h>

#include "debug.h"
#ifdef MELEE_NATIVE
#include "native_hsd_reservation.h"
#include "native_memory_snapshot.h"
#include "native_objalloc_snapshot.h"
#include <dolphin/os/OSHeapSnapshot.h>
#include <string.h>
#endif
#include <dolphin/os/OSAlloc.h>

static volatile OSHeapHandle current_heap = -1;

OSHeapHandle HSD_GetHeap(void)
{
    return current_heap;
}

void HSD_SetHeap(OSHeapHandle handle)
{
    current_heap = handle;
}

void HSD_Free(void* ptr)
{
    OSFreeToHeap(HSD_GetHeap(), ptr);
}

void* HSD_MemAlloc(ssize_t size)
{
    void* adr;
#ifdef MELEE_NATIVE
    if (nativeHsdReservation_RejectHeap()) return NULL;
#endif

    if (size <= 0) {
        return NULL;
    }

    adr = OSAllocFromHeap(HSD_GetHeap(), size);
    HSD_ASSERT(52, adr);

    return adr;
}

#ifdef MELEE_NATIVE
static bool memory_snapshot_buffer(const void* data, size_t bytes)
{
    uintptr_t at = (uintptr_t)data, root = (uintptr_t)&current_heap;
    return bytes && bytes <= UINT32_MAX && bytes <= UINTPTR_MAX - at &&
        !(at < root + sizeof(current_heap) && root < at + bytes) &&
        OSHeapSnapshotBufferIndependent(data, (u32)bytes) &&
        nativeObjAlloc_BufferIndependent(data, bytes) &&
        nativeHsdReservation_BufferIndependent(data, bytes);
}

static bool memory_snapshot_heap(OSHeapHandle heap)
{
    OSAllocStateRegion region;
    return heap == -1 || OSGetHeapStateRegion(heap, &region);
}

bool nativeMemory_Save(void* data, size_t capacity, size_t* written)
{
    const size_t bytes = sizeof(NativeMemorySnapshot);
    if (!OSHeapSnapshotEpoch() || !memory_snapshot_heap(current_heap) ||
        nativeHsdReservation_Active() || capacity < bytes ||
        !memory_snapshot_buffer(data, bytes) ||
        !memory_snapshot_buffer(written, sizeof(*written))) return false;
    uintptr_t a = (uintptr_t)data, b = (uintptr_t)written;
    if (a < b + sizeof(*written) && b < a + bytes) return false;
    NativeMemorySnapshot state = {OSHeapSnapshotEpoch(), current_heap, 0};
    memcpy(data, &state, bytes);
    *written = bytes;
    return true;
}

bool nativeMemory_ValidateSnapshot(const void* data, size_t bytes)
{
    if (bytes != sizeof(NativeMemorySnapshot) || nativeHsdReservation_Active() ||
        !memory_snapshot_buffer(data, bytes)) return false;
    NativeMemorySnapshot state;
    memcpy(&state, data, sizeof(state));
    return state.epoch && state.epoch == OSHeapSnapshotEpoch() &&
        !state.reserved && memory_snapshot_heap(state.heap);
}

bool nativeMemory_Load(const void* data, size_t bytes)
{
    if (!nativeMemory_ValidateSnapshot(data, bytes)) return false;
    NativeMemorySnapshot state;
    memcpy(&state, data, sizeof(state));
    current_heap = state.heap;
    return true;
}
#endif

#ifdef MELEE_NATIVE
bool nativeMemory_AdmitHistory(NativeMatchArena* arena)
{
    if(!arena || !arena->lifetime)return false;
#define ROOT(v) do { if(!nativeMatchArena_AddRoot(arena,&(v),sizeof(v)))return false; } while(0)
    // Ownership is quiescent here; the source declaration retains volatile.
    if(!nativeMatchArena_AddRoot(arena,(void*)&current_heap,sizeof(current_heap)))return false;
#undef ROOT
    return true;
}
#endif
