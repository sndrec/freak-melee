#include "native_hsd_arena.h"
#ifdef MELEE_NATIVE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#pragma clang section data = "" bss = ""
NativeMatchArena nativeHsdArena;
static void* backing;
static uint64_t owner_epoch;
bool nativeHsdArena_BeginHeap(void)
{
    if(owner_epoch==UINT64_MAX)return false;
    if(!backing){
        backing=VirtualAlloc(NULL,NATIVE_HSD_ARENA_BYTES,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
        if(!backing)return false;
        memset(backing,0,NATIVE_HSD_ARENA_BYTES);
    }
    return nativeMatchArena_Begin(&nativeHsdArena,backing,NATIVE_HSD_ARENA_BYTES,++owner_epoch);
}
bool nativeHsdArena_ResetHistory(void)
{
    if(!nativeHsdArena.lifetime || owner_epoch==UINT64_MAX)return false;
    // Storage may survive a scene change; private history and its root admission
    // do not. Advance the same counter used by heap recreation to avoid reuse.
    nativeHsdArena.lifetime=++owner_epoch;
    nativeHsdArena.root_count=0;nativeHsdArena.root_bytes=0;nativeHsdArena.sealed=false;
    return true;
}
#endif
