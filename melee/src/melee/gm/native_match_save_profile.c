#include "native_match_save_profile.h"
#if defined(MELEE_PROFILE_MATCH_SAVE)
#include <string.h>

// Diagnostic host state: never restored, and absent from normal builds.
#pragma clang section data = "" bss = ""
NativeMatchSaveProfile nativeMatchSaveProfile;
static volatile uint64_t observed;
static void* (*volatile copy_bytes)(void*,const void*,size_t)=memcpy;

void nativeMatchSaveProfile_Begin(void)
{
    if(!nativeMatchSaveProfile.enabled)return;
    bool detailed=nativeMatchSaveProfile.detailed;
    memset(&nativeMatchSaveProfile,0,sizeof(nativeMatchSaveProfile));
    nativeMatchSaveProfile.enabled=true;
    nativeMatchSaveProfile.active=true;
    nativeMatchSaveProfile.detailed=detailed;
}

static double empty_interval_ns(void)
{
    uint64_t total=0;
    for(unsigned i=0;i<4096;++i) {
        uint64_t start=SDL_GetTicksNS();
        total+=SDL_GetTicksNS()-start;
    }
    return total/4096.0;
}

typedef struct CopySegment {const void* source;size_t bytes,offset;} CopySegment;
enum { Segments=NativeMatchHistory_Arenas*(NATIVE_MATCH_ARENA_ROOTS+1),Trials=5,Copies=64 };

static void sort(double* values)
{
    for(unsigned i=1;i<Trials;++i) {
        double value=values[i];unsigned j=i;
        while(j && values[j-1]>value){values[j]=values[j-1];--j;}
        values[j]=value;
    }
}

// Controlled replay-boundary experiment. Reads live sources, writes only the
// caller's preallocated scratch. It neither publishes history nor changes the
// game. The source is warmed by repetition: this does not replace tick timings.
static void copy_experiment(FILE* output,const NativeMatchHistory* history,
                            void* scratch,size_t scratch_bytes)
{
    CopySegment segments[Segments];unsigned count=0;size_t bytes=0;
    for(unsigned a=0;a<history->arena_count;++a) {
        const NativeMatchArena* arena=history->frames[0].spans[a].arena;
        size_t prefix=nativeMatchArena_Used(arena);
        if(prefix>SIZE_MAX-bytes)return;
        segments[count++]=(CopySegment){arena->memory,prefix,bytes};bytes+=prefix;
        for(unsigned i=0;i<arena->root_count;++i) {
            const NativeMatchArenaRoot* root=&arena->roots[i];
            size_t n=root->count?(size_t)*root->count*root->stride:root->bytes;
            if(n>SIZE_MAX-bytes)return;
            segments[count++]=(CopySegment){root->address,n,bytes};bytes+=n;
        }
    }
    size_t stride=(bytes+7)&~(size_t)7;
    if(!stride || stride>scratch_bytes/17) {
        fprintf(output,"[save-profile] experiment skipped=1 reason=scratch-capacity bytes=%zu\n",bytes);
        return;
    }
    uint8_t* packed=scratch;uint8_t* ring=packed+stride;
    for(unsigned i=0;i<count;++i)memcpy(packed+segments[i].offset,segments[i].source,segments[i].bytes);
    const char* names[]={"live-gather","packed-chunks","packed-single"};
    for(unsigned variant=0;variant<3;++variant) {
        double times[Trials];
        for(unsigned trial=0;trial<Trials;++trial) {
            uint64_t start=SDL_GetTicksNS();
            for(unsigned iteration=0;iteration<Copies;++iteration) {
                uint8_t* destination=ring+(iteration%16)*stride;
                if(variant==2)copy_bytes(destination,packed,bytes);
                else for(unsigned i=0;i<count;++i) {
                    const void* source=variant==0?segments[i].source:packed+segments[i].offset;
                    copy_bytes(destination+segments[i].offset,source,segments[i].bytes);
                }
            }
            times[trial]=(SDL_GetTicksNS()-start)/(Copies*1000.0);
            for(unsigned i=0;i<16;++i)observed+=ring[i*stride+bytes-1];
        }
        // Check complete output, outside timing, against the gathered source.
        bool exact=true;
        for(unsigned i=0;i<16;++i)exact=exact && !memcmp(ring+i*stride,packed,bytes);
        sort(times);
        fprintf(output,"[save-profile] experiment mode=%s bytes=%zu segments=%u slots=16 copies=%u median_us=%.3f min_us=%.3f max_us=%.3f exact=%u scope=repeated-warm-source\n",
            names[variant],bytes,count,Trials*Copies,times[Trials/2],times[0],times[Trials-1],exact);
    }
    // Same length calculations/offset accumulation, without any data copies.
    // Observable checksums keep the metadata reads and results live.
    uint64_t start=SDL_GetTicksNS(),sum=0;
    for(unsigned repeat=0;repeat<4096;++repeat) {
        for(unsigned a=0;a<history->arena_count;++a) {
            const NativeMatchArena* arena=history->frames[0].spans[a].arena;
            size_t cursor=nativeMatchArena_Used(arena);
            for(unsigned i=0;i<arena->root_count;++i) {
                const NativeMatchArenaRoot* root=&arena->roots[i];
                size_t n=root->count?(size_t)*root->count*root->stride:root->bytes;
                cursor+=n;
                sum+=(uintptr_t)root->address+n;
            }
            sum+=cursor;
        }
        observed=sum;
    }
    double walk_us=(SDL_GetTicksNS()-start)/4096000.0;
    fprintf(output,"[save-profile] experiment mode=span-walk bytes=%zu segments=%u iterations=4096 mean_us=%.3f checksum=%llu scope=lengths-and-offsets-only\n",
        bytes,count,walk_us,(unsigned long long)observed);
}

void nativeMatchSaveProfile_Report(FILE* output,uint64_t call_site_ns,
    const NativeMatchHistory* history,void* scratch,size_t scratch_bytes)
{
    NativeMatchSaveProfile* profile=&nativeMatchSaveProfile;
    if(!profile->active)return;
    profile->active=false;profile->current=NULL;
    if(!profile->count)return;
    unsigned frames=profile->count;
    uint64_t total=0,checks=0,publish=0;
    for(unsigned i=0;i<frames;++i) {
        total+=profile->frames[i].total_ns;
        checks+=profile->frames[i].checks_ns;
        publish+=profile->frames[i].publish_ns;
        fprintf(output,"[save-profile] frame=%u total_us=%.3f checks_us=%.3f publication_us=%.3f\n",
            i,profile->frames[i].total_ns/1000.0,profile->frames[i].checks_ns/1000.0,profile->frames[i].publish_ns/1000.0);
    }
    double divisor=frames*1000.0;
    uint64_t arena_total=0;
    for(unsigned a=0;a<history->arena_count;++a) {
        NativeMatchSaveArenaProfile totals={0};
        for(unsigned i=0;i<frames;++i) {
            const NativeMatchSaveArenaProfile* frame=&profile->frames[i].arenas[a];
            totals.checks_ns+=frame->checks_ns;totals.prefix_ns+=frame->prefix_ns;
            totals.prepare_ns+=frame->prepare_ns;totals.copy_ns+=frame->copy_ns;
            totals.publish_ns+=frame->publish_ns;totals.total_ns+=frame->total_ns;
            totals.prefix_bytes+=frame->prefix_bytes;totals.root_bytes+=frame->root_bytes;
            totals.roots+=frame->roots;totals.copies+=frame->copies;
        }
        arena_total+=totals.total_ns;
        uint64_t assigned=totals.checks_ns+totals.prefix_ns+totals.prepare_ns+totals.copy_ns+totals.publish_ns;
        fprintf(output,"[save-profile] arena=%u frames=%u total_us=%.3f checks_us=%.3f prefix_us=%.3f root_prepare_us=%.3f root_copy_us=%.3f publication_us=%.3f loop_and_profiler_us=%.3f prefix_bytes=%zu root_bytes=%zu roots=%u nonempty_roots=%u clock_scope=inclusive root_scope=%s\n",
            a,frames,totals.total_ns/divisor,totals.checks_ns/divisor,totals.prefix_ns/divisor,
            totals.prepare_ns/divisor,totals.copy_ns/divisor,totals.publish_ns/divisor,
            (totals.total_ns-assigned)/divisor,totals.prefix_bytes/frames,totals.root_bytes/frames,
            totals.roots/frames,totals.copies/frames,profile->detailed?"copy-only":"whole-loop");
        const NativeMatchArena* arena=history->frames[0].spans[a].arena;
        for(unsigned i=0;profile->detailed && i<arena->root_count;++i) {
            const NativeMatchSaveRootProfile* root=&profile->roots[a][i];
            fprintf(output,"[save-profile] root arena=%u index=%u source=%p calls=%u nonempty=%u mean_bytes=%llu prepare_us=%.3f copy_us=%.3f\n",
                a,i,arena->roots[i].address,root->calls,root->nonempty,
                (unsigned long long)(root->calls?root->bytes/root->calls:0),
                root->calls?root->prepare_ns/(root->calls*1000.0):0,
                root->calls?root->copy_ns/(root->calls*1000.0):0);
        }
    }
    fprintf(output,"[save-profile] summary frames=%u mean_us=%.3f history_checks_us=%.3f history_publication_us=%.3f history_dispatch_and_profiler_us=%.3f call_site_us=%.3f empty_clock_interval_ns=%.3f detailed=%u\n",
        frames,total/divisor,checks/divisor,publish/divisor,(total-checks-publish-arena_total)/divisor,
        (call_site_ns-total)/divisor,empty_interval_ns(),profile->detailed);
    copy_experiment(output,history,scratch,scratch_bytes);
}
#endif
