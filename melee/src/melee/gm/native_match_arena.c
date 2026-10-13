#include "native_match_arena.h"
#include "native_match_save_profile.h"
#include <string.h>

enum { ArenaAlignment = 64 };
static bool overlap(const void* a, size_t an, const void* b, size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y || (x<y+bn && y<x+an);
}
bool nativeMatchArena_Begin(NativeMatchArena* arena,void* memory,size_t bytes,uint64_t lifetime)
{
    if(!arena || !memory || !lifetime || bytes<ArenaAlignment ||
       (uintptr_t)memory%ArenaAlignment || overlap(arena,sizeof(*arena),memory,bytes))return false;
    *arena=(NativeMatchArena){.memory=memory,.capacity=bytes,.lifetime=lifetime};
    memset(memory,0,ArenaAlignment);
    ((NativeMatchArenaCursor*)memory)->used=ArenaAlignment;
    return true;
}
size_t nativeMatchArena_Used(const NativeMatchArena* arena)
{
    return arena && arena->lifetime ? ((const NativeMatchArenaCursor*)arena->memory)->used : 0;
}
void* nativeMatchArena_Allocate(NativeMatchArena* arena,size_t bytes,size_t alignment)
{
    if(!arena || !arena->lifetime || !bytes || !alignment ||
       (alignment&(alignment-1)))return NULL;
    NativeMatchArenaCursor* cursor=(NativeMatchArenaCursor*)arena->memory;
    size_t used=cursor->used;
    uintptr_t begin=(uintptr_t)arena->memory;
    if(used>arena->capacity || begin+used>UINTPTR_MAX-(alignment-1))return NULL;
    size_t at=((begin+used+alignment-1)&~(uintptr_t)(alignment-1))-begin;
    if(at>arena->capacity || bytes>arena->capacity-at)return NULL;
    // Clear padding too: every byte in the copied prefix has a defined value.
    memset(arena->memory+used,0,at+bytes-used);
    cursor->used=at+bytes;
    return arena->memory+at;
}
bool nativeMatchArena_AddRoot(NativeMatchArena* arena,void* address,size_t bytes)
{
    if(!arena || !arena->lifetime || arena->sealed || !address || !bytes ||
       arena->root_count==NATIVE_MATCH_ARENA_ROOTS || bytes>SIZE_MAX-arena->root_bytes ||
       overlap(address,bytes,arena,sizeof(*arena)) ||
       overlap(address,bytes,arena->memory,arena->capacity))return false;
    for(unsigned i=0;i<arena->root_count;++i)
        if(overlap(address,bytes,arena->roots[i].address,arena->roots[i].bytes))return false;
    arena->roots[arena->root_count++]=(NativeMatchArenaRoot){address,bytes};
    arena->root_bytes+=bytes;
    return true;
}
bool nativeMatchArena_AddCountedRoot(NativeMatchArena* arena,void* address,size_t stride,
                                    uint32_t capacity,const uint32_t* count)
{
    if(!arena || !arena->lifetime || !stride || !capacity || stride>SIZE_MAX/capacity || !count ||
       (uintptr_t)count%_Alignof(uint32_t))return false;
    uintptr_t at=(uintptr_t)count,begin=(uintptr_t)arena->memory;
    size_t used=nativeMatchArena_Used(arena);
    bool captured=at>=begin && at-begin<=used && sizeof(*count)<=used-(at-begin);
    for(unsigned i=0;!captured && i<arena->root_count;++i){
        const NativeMatchArenaRoot* root=&arena->roots[i];
        begin=(uintptr_t)root->address;
        size_t n=root->count?(size_t)*root->count*root->stride:root->bytes;
        captured=at>=begin && at-begin<=n && sizeof(*count)<=n-(at-begin);
    }
    if(!captured || *count>capacity)return false;
    if(!nativeMatchArena_AddRoot(arena,address,stride*capacity))return false;
    NativeMatchArenaRoot* root=&arena->roots[arena->root_count-1];
    root->stride=stride;root->count=count;
    return true;
}
bool nativeMatchArena_BindFrame(NativeMatchArena* arena,NativeMatchArenaFrame* frame,void* memory,size_t bytes)
{
    if(!arena || !arena->lifetime || !frame || !memory ||
       arena->capacity>SIZE_MAX-arena->root_bytes || bytes<arena->capacity+arena->root_bytes ||
       overlap(memory,bytes,arena->memory,arena->capacity) ||
       overlap(memory,bytes,arena,sizeof(*arena)) || overlap(memory,bytes,frame,sizeof(*frame)) ||
       overlap(frame,sizeof(*frame),arena->memory,arena->capacity) ||
       overlap(frame,sizeof(*frame),arena,sizeof(*arena)))return false;
    for(unsigned i=0;i<arena->root_count;++i)
        if(overlap(memory,bytes,arena->roots[i].address,arena->roots[i].bytes) ||
           overlap(frame,sizeof(*frame),arena->roots[i].address,arena->roots[i].bytes))return false;
    *frame=(NativeMatchArenaFrame){.arena=arena,.memory=memory,.capacity=bytes,.lifetime=arena->lifetime};
    arena->sealed=true;
    return true;
}
bool nativeMatchArena_Save(NativeMatchArenaFrame* frame)
{
#if defined(MELEE_PROFILE_MATCH_SAVE)
    NativeMatchSaveFrameProfile* measured=nativeMatchSaveProfile.current;
    NativeMatchSaveArenaProfile* profile=measured?&measured->arenas[nativeMatchSaveProfile.arena]:NULL;
    bool detailed=profile && nativeMatchSaveProfile.detailed;
    uint64_t total_start=profile?SDL_GetTicksNS():0;
#endif
    if(!frame || !frame->arena || frame->lifetime!=frame->arena->lifetime)return false;
    NativeMatchArena* arena=frame->arena;
    size_t bytes=nativeMatchArena_Used(arena);
    if(bytes<ArenaAlignment || bytes>arena->capacity)return false;
#if defined(MELEE_PROFILE_MATCH_SAVE)
    uint64_t copy_start=profile?SDL_GetTicksNS():0;
    if(profile)profile->checks_ns=copy_start-total_start;
#endif
    memcpy(frame->memory,arena->memory,bytes);
#if defined(MELEE_PROFILE_MATCH_SAVE)
    if(profile)profile->prefix_ns=SDL_GetTicksNS()-copy_start;
#endif
    uint8_t* cursor=frame->memory+bytes;
#if defined(MELEE_PROFILE_MATCH_SAVE)
    uint64_t roots_start=profile?SDL_GetTicksNS():0;
#endif
    for(unsigned i=0;i<arena->root_count;++i){
#if defined(MELEE_PROFILE_MATCH_SAVE)
        uint64_t prepare_start=detailed?SDL_GetTicksNS():0;
#endif
        const NativeMatchArenaRoot* root=&arena->roots[i];
        size_t n=root->count?(size_t)*root->count*root->stride:root->bytes;
        frame->root_bytes[i]=n;
#if defined(MELEE_PROFILE_MATCH_SAVE)
        uint64_t root_copy_start=detailed?SDL_GetTicksNS():0;
#endif
        memcpy(cursor,root->address,n);
#if defined(MELEE_PROFILE_MATCH_SAVE)
        if(detailed) {
            uint64_t copy_ns=SDL_GetTicksNS()-root_copy_start;
            uint64_t prepare_ns=root_copy_start-prepare_start;
            profile->prepare_ns+=prepare_ns;profile->copy_ns+=copy_ns;
            NativeMatchSaveRootProfile* entry=&nativeMatchSaveProfile.roots[nativeMatchSaveProfile.arena][i];
            entry->prepare_ns+=prepare_ns;entry->copy_ns+=copy_ns;entry->bytes+=n;
            ++entry->calls;entry->nonempty+=n!=0;
        }
#endif
        cursor+=n;
    }
#if defined(MELEE_PROFILE_MATCH_SAVE)
    uint64_t publish_start=profile?SDL_GetTicksNS():0;
    if(profile && !detailed)profile->copy_ns=publish_start-roots_start;
#endif
    frame->bytes=bytes;frame->total_bytes=(size_t)(cursor-frame->memory);frame->valid=true;
#if defined(MELEE_PROFILE_MATCH_SAVE)
    if(profile) {
        uint64_t end=SDL_GetTicksNS();
        profile->publish_ns=end-publish_start;profile->total_ns=end-total_start;
        profile->prefix_bytes=bytes;profile->roots=arena->root_count;
        profile->root_bytes=frame->total_bytes-bytes;
        for(unsigned i=0;i<arena->root_count;++i)profile->copies+=frame->root_bytes[i]!=0;
    }
#endif
    return true;
}
bool nativeMatchArena_Load(const NativeMatchArenaFrame* frame)
{
    if(!frame || !frame->valid || !frame->arena || frame->lifetime!=frame->arena->lifetime)return false;
    NativeMatchArena* arena=frame->arena;
    memcpy(arena->memory,frame->memory,frame->bytes);
    const uint8_t* cursor=frame->memory+frame->bytes;
    for(unsigned i=0;i<arena->root_count;++i){
        size_t n=frame->root_bytes[i];
        memcpy(arena->roots[i].address,cursor,n);cursor+=n;
    }
    return true;
}
