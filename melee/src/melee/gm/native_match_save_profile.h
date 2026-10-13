#ifndef MELEE_GM_NATIVE_MATCH_SAVE_PROFILE_H
#define MELEE_GM_NATIVE_MATCH_SAVE_PROFILE_H

#if defined(MELEE_PROFILE_MATCH_SAVE)
#include "native_match_history.h"
#include <stdio.h>
#include <SDL3/SDL_timer.h>

enum { NativeMatchSaveProfile_Frames=64 };
typedef struct NativeMatchSaveArenaProfile {
    uint64_t checks_ns,prefix_ns,prepare_ns,copy_ns,publish_ns,total_ns;
    size_t prefix_bytes,root_bytes;
    unsigned roots,copies;
} NativeMatchSaveArenaProfile;
typedef struct NativeMatchSaveFrameProfile {
    uint64_t checks_ns,publish_ns,total_ns;
    NativeMatchSaveArenaProfile arenas[NativeMatchHistory_Arenas];
} NativeMatchSaveFrameProfile;
typedef struct NativeMatchSaveRootProfile {
    uint64_t prepare_ns,copy_ns,bytes;
    unsigned calls,nonempty;
} NativeMatchSaveRootProfile;
typedef struct NativeMatchSaveProfile {
    NativeMatchSaveFrameProfile frames[NativeMatchSaveProfile_Frames];
    NativeMatchSaveRootProfile roots[NativeMatchHistory_Arenas][NATIVE_MATCH_ARENA_ROOTS];
    NativeMatchSaveFrameProfile* current;
    unsigned count,arena;
    bool enabled,active,detailed;
} NativeMatchSaveProfile;

extern NativeMatchSaveProfile nativeMatchSaveProfile;
void nativeMatchSaveProfile_Begin(void);
void nativeMatchSaveProfile_Report(FILE*,uint64_t call_site_ns,
    const NativeMatchHistory*,void* scratch,size_t scratch_bytes);
#endif
#endif
