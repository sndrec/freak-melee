#ifndef DOLPHIN_OS_HEAP_SNAPSHOT_H
#define DOLPHIN_OS_HEAP_SNAPSHOT_H

#include <dolphin/os/OSAlloc.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { OS_HEAP_SNAPSHOT_MAX_CELLS = 65536 };

// Private native process-memory format. Payload bytes belong to their owners.
typedef struct OSHeapSnapshotHeader {
  void* descriptor;
  void* arena_start;
  void* arena_end;
  void* free_head;
  void* allocated_head;
  uint64_t epoch;
  int32_t heap, current_heap, heap_bytes;
  uint32_t cells;
} OSHeapSnapshotHeader;

typedef struct OSHeapSnapshotCell {
  void* address;
  unsigned char state[32];
} OSHeapSnapshotCell;

// Idle allocator/game-thread boundary only. Fixed scratch; no allocation,
// payload copying, page protection or constructors. Geometry/lifetime changes
// retire history; normal allocation/free and current-heap selection do not.
u32 OSHeapSnapshotBytes(OSHeapHandle heap);
BOOL OSSaveHeapSnapshot(OSHeapHandle heap, void* data, u32 capacity, u32* written);
// A successful idle Save fully validates live topology and frozen geometry,
// producing a valid immutable candidate. Restore still validates that candidate.
BOOL OSValidateHeapSnapshot(OSHeapHandle heap, const void* data, u32 bytes);
BOOL OSLoadHeapSnapshot(OSHeapHandle heap, const void* data, u32 bytes);

// Explicit transition lease for a frozen selected-heap topology. Runtime
// allocation/free and geometry changes abort until the scene releases history.
BOOL OSLockHeapForHistory(OSHeapHandle heap);
BOOL OSUnlockHeapForHistory(OSHeapHandle heap);

// Payload owners use the same allocator lifetime as the Cell snapshots.
// Range ownership survives allocation/free/coalescing, but not heap geometry
// changes. The fixed range catalog is rebuilt once per geometry epoch; callers
// may prepare it before starting history to avoid the first-query cost.
uint64_t OSHeapSnapshotEpoch(void);
BOOL OSPrepareHeapSnapshotOwnership(void);
BOOL OSHeapSnapshotOwnsRange(OSHeapHandle heap, const void* data, u32 bytes);
// After allocator restoration, a payload writer additionally requires a live
// Cell header at this exact payload address with sufficient allocated capacity.
BOOL OSHeapSnapshotOwnsAllocation(OSHeapHandle heap, const void* data, u32 bytes);
BOOL OSHeapSnapshotBufferIndependent(const void* data, u32 bytes);

#ifdef __cplusplus
}
#endif
#endif
