#ifndef HSD_NATIVE_ANIMATION_POOL_H
#define HSD_NATIVE_ANIMATION_POOL_H
#ifdef MELEE_NATIVE
#include <melee/gm/native_match_pool.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/fobj.h>
#include <sysdolphin/baselib/list.h>

// Eight-fighter Green Greens peaks at 786 controllers / 2482 channels.
// Channels cannot be cut eightfold without failing that stock workload.
enum { NATIVE_AOBJ_CAPACITY = 2048, NATIVE_FOBJ_CAPACITY = 4096 };
typedef struct NativeAnimationGlobals {
    HSD_SList* end_callbacks;
    uint64_t heap_epoch;
    int32_t finished, active;
    uint32_t callback_depth;
} NativeAnimationGlobals;

// Backing identity follows the main heap. Logical counters and allocator
// metadata rewind; immutable storage identity is excluded from game state.
typedef struct NativeAnimationStorage { void* memory; size_t bytes; } NativeAnimationStorage;
extern NativeAnimationStorage nativeAnimationStorage;
extern NativeMatchPool nativeAObjPool, nativeFObjPool;
extern NativeAnimationGlobals nativeAnimationGlobals;

// Called once by actual HSD_ObjInit at boot/main-heap recreation, not GObj bind.
// Bulk heap retirement deliberately does not run graph destructors/callbacks.
bool nativeAnimationPool_CanBeginHeap(void);
bool nativeAnimationPool_BeginHeap(void);
struct NativeMatchArena;
bool nativeAnimationPool_AdmitHistory(struct NativeMatchArena*);
size_t nativeAnimationPool_SnapshotBytes(void);
bool nativeAnimationPool_Save(void* snapshot, size_t capacity, size_t* written);
// Successful idle Save validates both pool writers and the saved graph before
// publishing its byte count. Its immutable result needs no second capture check.
bool nativeAnimationPool_ValidateSnapshot(const void* snapshot, size_t bytes);
bool nativeAnimationPool_Load(const void* snapshot, size_t bytes);
#endif
#endif
