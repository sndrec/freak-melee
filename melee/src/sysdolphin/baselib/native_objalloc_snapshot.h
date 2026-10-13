#ifndef HSD_NATIVE_OBJALLOC_SNAPSHOT_H
#define HSD_NATIVE_OBJALLOC_SNAPSHOT_H

#include "objalloc.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    NATIVE_OBJALLOC_SNAPSHOT_DESCRIPTORS = 256,
    NATIVE_OBJALLOC_SNAPSHOT_BLOCKS = 65536
};

// Private in-memory component, not a persistent format. Descriptors and backing
// stay resident throughout history. No format compatibility or constructors.
typedef struct NativeObjAllocSnapshotHeader {
    const HSD_ObjAllocData* registry;
    objheap heap;
    uint64_t epoch;
    uint32_t count, blocks;
    size_t arena_used;
} NativeObjAllocSnapshotHeader;

typedef struct NativeObjAllocSnapshotRecord {
    HSD_ObjAllocData* descriptor;
    HSD_ObjAllocData state;
} NativeObjAllocSnapshotRecord;

typedef struct NativeObjAllocSnapshotBlock {
    HSD_ObjAllocData* allocator;
    void* address;
    size_t bytes;
} NativeObjAllocSnapshotBlock;

// Physical ordinary allocation slot. These allocators do not assign object
// generations: occupancy/list/ID agreement belongs to the consuming owner.
// Reserved construction slots have a separate reservation owner.
typedef struct NativeObjAllocReference {
    uint64_t epoch;
    uint32_t block, slot;
    HSD_ObjAllocData* allocator;
    size_t bytes;
} NativeObjAllocReference;
// Returns a zero-epoch reference on failure; never writes through caller storage.
NativeObjAllocReference nativeObjAlloc_Reference(HSD_ObjAllocData*,const void*,size_t);
void* nativeObjAlloc_Resolve(NativeObjAllocReference);

// Owns shared descriptors/cursor and ordinary allocation blocks, including
// payload and free-link bytes. Reserved payload remains its own component.
// Class state and roots are aggregate companions. Ordinary chunks share the HSD
// arena cursor; explicit fixed ObjSetHeap backing retains its separate contract.
// Caller must use an idle game/script/draw boundary with no construction claim.
size_t nativeObjAlloc_SnapshotBytes(void);
bool nativeObjAlloc_BufferIndependent(const void*, size_t bytes);
bool nativeObjAlloc_Save(void*, size_t capacity, size_t* written);
bool nativeObjAlloc_ValidateSnapshot(const void*, size_t bytes);
bool nativeObjAlloc_Load(const void*, size_t bytes);
bool nativeObjAlloc_AdmitArenaHistory(void);

#endif
