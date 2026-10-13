#ifndef HSD_NATIVE_CLASS_SNAPSHOT_H
#define HSD_NATIVE_CLASS_SNAPSHOT_H

#include "class.h"
#include <stdint.h>

enum {
    NATIVE_CLASS_SNAPSHOT_BLOCKS = 65536,
    NATIVE_CLASS_SNAPSHOT_TYPES = 256
};

// Private in-memory records. Code, class definitions and allocation addresses
// remain resident throughout history. No persistent format or compatibility.
typedef struct NativeClassSnapshotHeader {
    uint64_t epoch;
    HSD_MemoryEntry** memory_list;
    int32_t entries;
    uint32_t blocks, types;
    size_t arena_used;
} NativeClassSnapshotHeader;

typedef struct NativeClassSnapshotType {
    HSD_ClassInfo* info;
    HSD_ClassInfoHead state;
} NativeClassSnapshotType;

typedef struct NativeClassSnapshotBlock {
    void* address;
    size_t bytes;
} NativeClassSnapshotBlock;

// Reference to retained ordinary class backing, not proof that a live object
// occupies it. Consumers validate restored type/role after the class companion
// loads; aggregate validation must prove this block exists in that candidate.
typedef struct NativeClassReference {
    uint64_t epoch;
    uint32_t block, reserved;
    size_t offset, bytes;
} NativeClassReference;
// Zero bytes means the address is not retained ordinary backing. Epoch zero
// itself is valid before the first heap retirement. Returning by
// value avoids rescanning every allocation just to validate an output slot.
// This checks retained backing geometry only, not the initialized type tree or
// class counters. Type queries and class snapshot writers validate those owners.
NativeClassReference nativeClass_Reference(const void*,size_t bytes);
void* nativeClass_Resolve(NativeClassReference);
// Numeric identity lookup in the initialized class tree. Never follows an
// unrecognized caller-supplied info pointer. The class tree remains resident.
bool nativeClass_TypeKnown(const HSD_ClassInfo*, size_t minimum_object_bytes);

// Owns ordinary class-piece chunks (including free links), the size-class
// table/entries, and initialized class counts. Reserved pieces are separate.
// Call only at an idle tick boundary with no HSD construction claim. The
// initialized class tree must remain fixed while history exists. Ordinary class
// and ObjAlloc chunks share a cursor: their diagnostic components must come from
// the same boundary. Copy-only HSD history restores both families together.
size_t nativeClass_SnapshotBytes(void);
// Successful save guarantees a valid own candidate, including class tree/counter
// and retained size-class table checks before publication of the written count.
bool nativeClass_Save(void*, size_t capacity, size_t* written);
bool nativeClass_ValidateSnapshot(const void*, size_t bytes);
bool nativeClass_Load(const void*, size_t bytes);

// Caller-reserved index, prepared once per candidate. Snapshot/type definitions
// stay immutable throughout inspection. There is no tick allocation or hidden
// single-candidate cache; independent views can coexist with separate indexes.
typedef struct NativeClassSnapshotView {
    NativeClassSnapshotHeader header;
    const uint8_t* types;
    const uint8_t* const* payloads;
} NativeClassSnapshotView;
NativeClassSnapshotView nativeClass_ViewSnapshot(const void*,size_t,
                                                 const uint8_t** index,uint32_t capacity);
const void* nativeClass_ReadSnapshot(NativeClassSnapshotView,NativeClassReference);
void* nativeClass_ResolveSnapshot(NativeClassSnapshotView,NativeClassReference);
bool nativeClass_SnapshotTypeKnown(NativeClassSnapshotView,const HSD_ClassInfo*,size_t minimum_bytes);
// Matches the saved HSD_Class header and frozen type size; not proof of liveness
// or complete graph topology. Never follows an unrecognized type pointer.
bool nativeClass_SnapshotObjectType(NativeClassSnapshotView,NativeClassReference,const HSD_ClassInfo*);

#endif
