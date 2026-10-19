#include "native_hsd_arena.h"
#ifdef MELEE_NATIVE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma clang section data = "" bss = ""
NativeMatchArena nativeHsdArena;
static void* backing;
static size_t committed;
static uint64_t owner_epoch;
bool nativeHsdArena_BeginHeap(void)
{
    if(owner_epoch==UINT64_MAX)return false;
    if(!backing){
        backing=VirtualAlloc(NULL,NATIVE_HSD_ARENA_MAX_BYTES,MEM_RESERVE,PAGE_READWRITE);
        if(!backing)return false;
        if(!VirtualAlloc(backing,NATIVE_HSD_ARENA_BYTES,MEM_COMMIT,PAGE_READWRITE)) {
            VirtualFree(backing,0,MEM_RELEASE);backing=NULL;return false;
        }
        committed=NATIVE_HSD_ARENA_BYTES;
    }
    if(committed>NATIVE_HSD_ARENA_BYTES) {
        if(!VirtualFree((uint8_t*)backing+NATIVE_HSD_ARENA_BYTES,
                        committed-NATIVE_HSD_ARENA_BYTES,MEM_DECOMMIT))return false;
        committed=NATIVE_HSD_ARENA_BYTES;
    }
    return nativeMatchArena_Begin(&nativeHsdArena,backing,committed,++owner_epoch);
}
bool nativeHsdArena_Ensure(size_t bytes,size_t alignment)
{
    if(!nativeHsdArena.lifetime || !bytes || !alignment || (alignment&(alignment-1)))return false;
    size_t used=nativeMatchArena_Used(&nativeHsdArena);
    uintptr_t begin=(uintptr_t)nativeHsdArena.memory;
    if(used>nativeHsdArena.capacity || begin+used>UINTPTR_MAX-(alignment-1))return false;
    size_t at=((begin+used+alignment-1)&~(uintptr_t)(alignment-1))-begin;
    if(bytes>SIZE_MAX-at)return false;
    size_t needed=at+bytes;
    if(needed<=nativeHsdArena.capacity)return true;
    if(nativeHsdArena.sealed || needed>NATIVE_HSD_ARENA_MAX_BYTES ||
       !committed || nativeHsdArena.memory!=backing || nativeHsdArena.capacity!=committed)return false;
    size_t grown=committed;
    while(grown<needed)grown*=2;
    if(!VirtualAlloc((uint8_t*)backing+committed,grown-committed,MEM_COMMIT,PAGE_READWRITE))return false;
    committed=grown;nativeHsdArena.capacity=grown;
    return true;
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
