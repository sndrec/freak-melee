#ifndef MELEE_GM_NATIVE_MATCH_HISTORY_H
#define MELEE_GM_NATIVE_MATCH_HISTORY_H
#include "native_match_arena.h"
enum { NativeMatchHistory_Capacity=16, NativeMatchHistory_Arenas=2 };
typedef struct NativeMatchHistoryFrame {
    uint64_t frame;
    size_t bytes;
    bool valid;
    NativeMatchArenaFrame spans[NativeMatchHistory_Arenas];
} NativeMatchHistoryFrame;
typedef struct NativeMatchHistory {
    uint8_t* backing;
    size_t slot_bytes,peak_bytes;
    uint64_t epoch,newest_frame,captures,restores;
    unsigned capacity,arena_count;
    bool has_frames,failed;
    NativeMatchHistoryFrame frames[NativeMatchHistory_Capacity];
} NativeMatchHistory;
// Construction only: bounded backing supports each arena's full admitted capacity.
// Counted roots copy only initialized prefixes during ordinary capture/restore.
size_t nativeMatchHistory_SlotBytes(NativeMatchArena* const*,unsigned count);
bool nativeMatchHistory_Bind(NativeMatchHistory*,NativeMatchArena* const*,unsigned count,
    void*,size_t backing_bytes,unsigned capacity,uint64_t scene_epoch);
bool nativeMatchHistory_Save(NativeMatchHistory*,uint64_t frame,uint64_t scene_epoch);
const NativeMatchHistoryFrame* nativeMatchHistory_Find(const NativeMatchHistory*,uint64_t frame);
// Private own frames, with all arena lifetimes checked before any copy. No graph
// walk, allocator reconstruction, validation or callbacks run during restore.
bool nativeMatchHistory_Load(NativeMatchHistory*,uint64_t frame,uint64_t scene_epoch);
void nativeMatchHistory_Discard(NativeMatchHistory*);
#endif
