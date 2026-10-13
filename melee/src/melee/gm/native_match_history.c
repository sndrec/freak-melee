#include "native_match_history.h"
#include "native_match_save_profile.h"
#include <string.h>
static bool overlap(const void* a,size_t an,const void* b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y || (x<y+bn && y<x+an);
}

size_t nativeMatchHistory_SlotBytes(NativeMatchArena* const* arenas,unsigned count)
{
    if (!arenas || !count || count>NativeMatchHistory_Arenas) return 0;
    size_t bytes=0;
    for(unsigned i=0;i<count;++i) {
        const NativeMatchArena* arena=arenas[i];
        if(!arena || !arena->lifetime || arena->capacity>SIZE_MAX-arena->root_bytes) return 0;
        size_t span=arena->capacity+arena->root_bytes;
        if(span>SIZE_MAX-7 || ((span+7)&~(size_t)7)>SIZE_MAX-bytes) return 0;
        bytes+=(span+7)&~(size_t)7;
        for(unsigned j=0;j<i;++j)if(arena==arenas[j])return 0;
    }
    return bytes;
}
bool nativeMatchHistory_Bind(NativeMatchHistory* history,NativeMatchArena* const* arenas,
    unsigned count,void* backing,size_t backing_bytes,unsigned capacity,uint64_t epoch)
{
    size_t slot_bytes=nativeMatchHistory_SlotBytes(arenas,count);
    if(!history || !backing || !epoch || !capacity || capacity>NativeMatchHistory_Capacity ||
       !slot_bytes || ((uintptr_t)backing&7) || slot_bytes>SIZE_MAX/capacity ||
       backing_bytes<slot_bytes*capacity || backing_bytes>UINTPTR_MAX-(uintptr_t)backing ||
       sizeof(*history)>UINTPTR_MAX-(uintptr_t)history)return false;
    uintptr_t start=(uintptr_t)backing,end=start+backing_bytes,metadata=(uintptr_t)history;
    if(start<metadata+sizeof(*history) && metadata<end)return false;
    if(overlap(history,sizeof(*history),arenas,count*sizeof(*arenas)) ||
       overlap(backing,backing_bytes,arenas,count*sizeof(*arenas)))return false;
    // Verify all output spans/metadata against every owner before any write.
    for(unsigned i=0;i<count;++i) {
        NativeMatchArena* arena=arenas[i];
        if(overlap(history,sizeof(*history),arena,sizeof(*arena)) ||
           overlap(backing,backing_bytes,arena,sizeof(*arena)) ||
           overlap(history,sizeof(*history),arena->memory,arena->capacity) ||
           overlap(backing,backing_bytes,arena->memory,arena->capacity))return false;
        for(unsigned j=0;j<arena->root_count;++j) {
            const NativeMatchArenaRoot* root=&arena->roots[j];
            if(overlap(history,sizeof(*history),root->address,root->bytes) ||
               overlap(backing,backing_bytes,root->address,root->bytes))return false;
        }
    }
    memset(history,0,sizeof(*history));
    history->backing=backing;history->slot_bytes=slot_bytes;
    history->capacity=capacity;history->arena_count=count;history->epoch=epoch;
    for(unsigned slot=0;slot<capacity;++slot) {
        size_t offset=slot*slot_bytes;
        for(unsigned i=0;i<count;++i) {
            size_t span=(arenas[i]->capacity+arenas[i]->root_bytes+7)&~(size_t)7;
            if(!nativeMatchArena_BindFrame(arenas[i],&history->frames[slot].spans[i],
                    (uint8_t*)backing+offset,span)) {
                history->failed=true;return false;
            }
            offset+=span;
        }
    }
    return true;
}
static bool current(const NativeMatchHistory* history)
{
    if(!history || !history->capacity || history->capacity>NativeMatchHistory_Capacity ||
       !history->arena_count || history->arena_count>NativeMatchHistory_Arenas || history->failed)return false;
    for(unsigned i=0;i<history->arena_count;++i) {
        const NativeMatchArenaFrame* span=&history->frames[0].spans[i];
        if(!span->arena || span->lifetime!=span->arena->lifetime)return false;
    }
    return true;
}
const NativeMatchHistoryFrame* nativeMatchHistory_Find(const NativeMatchHistory* history,uint64_t frame)
{
    if(!current(history))return NULL;
    const NativeMatchHistoryFrame* slot=&history->frames[frame%history->capacity];
    return slot->valid && slot->frame==frame ? slot : NULL;
}
bool nativeMatchHistory_Save(NativeMatchHistory* history,uint64_t frame,uint64_t epoch)
{
#if defined(MELEE_PROFILE_MATCH_SAVE)
    NativeMatchSaveFrameProfile* profile=nativeMatchSaveProfile.active &&
        nativeMatchSaveProfile.count<NativeMatchSaveProfile_Frames ?
        &nativeMatchSaveProfile.frames[nativeMatchSaveProfile.count]:NULL;
    nativeMatchSaveProfile.current=profile;
    uint64_t total_start=profile?SDL_GetTicksNS():0;
#endif
    if(!current(history) || !history->backing || !epoch || epoch!=history->epoch || frame==UINT64_MAX ||
       (history->has_frames && frame!=history->newest_frame && frame!=history->newest_frame+1))return false;
    NativeMatchHistoryFrame* slot=&history->frames[frame%history->capacity];
    slot->valid=false;slot->bytes=0;
#if defined(MELEE_PROFILE_MATCH_SAVE)
    if(profile)profile->checks_ns=SDL_GetTicksNS()-total_start;
#endif
    for(unsigned i=0;i<history->arena_count;++i) {
#if defined(MELEE_PROFILE_MATCH_SAVE)
        nativeMatchSaveProfile.arena=i;
#endif
        if(!nativeMatchArena_Save(&slot->spans[i])){history->failed=true;return false;}
        slot->bytes+=slot->spans[i].total_bytes;
    }
#if defined(MELEE_PROFILE_MATCH_SAVE)
    uint64_t publish_start=profile?SDL_GetTicksNS():0;
#endif
    slot->frame=frame;slot->valid=true;
    history->newest_frame=frame;history->has_frames=true;++history->captures;
    if(slot->bytes>history->peak_bytes)history->peak_bytes=slot->bytes;
#if defined(MELEE_PROFILE_MATCH_SAVE)
    if(profile) {
        uint64_t end=SDL_GetTicksNS();
        profile->publish_ns=end-publish_start;profile->total_ns=end-total_start;
        ++nativeMatchSaveProfile.count;
    }
    nativeMatchSaveProfile.current=NULL;
#endif
    return true;
}
bool nativeMatchHistory_Load(NativeMatchHistory* history,uint64_t frame,uint64_t epoch)
{
    if(!history || !epoch || epoch!=history->epoch)return false;
    const NativeMatchHistoryFrame* slot=nativeMatchHistory_Find(history,frame);
    if(!slot)return false;
    // Validate constant-sized private leases before either arena is mutated.
    for(unsigned i=0;i<history->arena_count;++i)
        if(!slot->spans[i].valid || slot->spans[i].lifetime!=slot->spans[i].arena->lifetime)return false;
    for(unsigned i=0;i<history->arena_count;++i)
        if(!nativeMatchArena_Load(&slot->spans[i])){history->failed=true;return false;}
    for(unsigned i=0;i<history->capacity;++i)
        if(history->frames[i].valid && history->frames[i].frame>frame)history->frames[i].valid=false;
    history->newest_frame=frame;++history->restores;
    return true;
}
void nativeMatchHistory_Discard(NativeMatchHistory* history)
{
    if(!history)return;
    for(unsigned i=0;i<NativeMatchHistory_Capacity;++i)history->frames[i].valid=false;
    history->has_frames=false;
}
