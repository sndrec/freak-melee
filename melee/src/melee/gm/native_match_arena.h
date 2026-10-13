#ifndef MELEE_GM_NATIVE_MATCH_ARENA_H
#define MELEE_GM_NATIVE_MATCH_ARENA_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { NATIVE_MATCH_ARENA_ROOTS = 512, NATIVE_MATCH_SCENE_BYTES = 2 * 1024 * 1024 };
typedef struct NativeMatchArenaCursor { size_t used; } NativeMatchArenaCursor;
typedef struct NativeMatchArenaRoot {
    void* address;
    size_t bytes, stride;
    const uint32_t* count;
} NativeMatchArenaRoot;
// The descriptor is scene construction state. The allocation cursor lives in
// backing and rewinds with the payload. Root addresses remain fixed in a scene.
typedef struct NativeMatchArena {
    uint8_t* memory;
    size_t capacity, root_bytes;
    uint64_t lifetime;
    unsigned root_count;
    bool sealed;
    NativeMatchArenaRoot roots[NATIVE_MATCH_ARENA_ROOTS];
} NativeMatchArena;
// A private, prebound history slot, never an imported file/network savestate.
typedef struct NativeMatchArenaFrame {
    NativeMatchArena* arena;
    uint8_t* memory;
    size_t capacity, bytes, total_bytes;
    uint64_t lifetime;
    bool valid;
    size_t root_bytes[NATIVE_MATCH_ARENA_ROOTS];
} NativeMatchArenaFrame;
bool nativeMatchArena_Begin(NativeMatchArena*, void*, size_t, uint64_t);
void* nativeMatchArena_Allocate(NativeMatchArena*, size_t, size_t alignment);
bool nativeMatchArena_AddRoot(NativeMatchArena*, void*, size_t);
// A bounded append-only directory can copy its initialized prefix instead of
// its maximum capacity. Its count must also belong to captured mutable state;
// the owner enforces the admitted capacity whenever it appends entries.
bool nativeMatchArena_AddCountedRoot(NativeMatchArena*, void*, size_t stride,
                                    uint32_t capacity, const uint32_t* count);
size_t nativeMatchArena_Used(const NativeMatchArena*);
// Checks backing/alias/layout once, seals root admission. Subsequent save/load
// copy the known prefix and root spans; no object graph or allocator validation.
bool nativeMatchArena_BindFrame(NativeMatchArena*, NativeMatchArenaFrame*, void*, size_t);
bool nativeMatchArena_Save(NativeMatchArenaFrame*);
bool nativeMatchArena_Load(const NativeMatchArenaFrame*);
// Fixed payload windows share one process reservation. Only initialized slot
// prefixes are admitted to scene history; identity stays constant in a scene.
typedef struct NativeMatchActorStorage {
    void* memory;
    size_t bytes;
    void *fighters, *ground, *items, *item_links, *objects, *procedures;
} NativeMatchActorStorage;
extern NativeMatchActorStorage nativeMatchActorStorage;
extern NativeMatchArena nativeMatchSceneArena;
bool nativeMatchScene_Begin(void);
bool nativeMatchScene_ResetHistory(void);
void nativeMatchScene_Retire(void);
#endif
