#include <melee/gm/native_match_control_history.h>
#include "random.h"
#include "native_random_snapshot.h"
#include <stdint.h>
#include <string.h>
#ifdef MELEE_NATIVE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#endif

u32 seed = 1;
u32* seed_ptr = &seed;

#ifdef MELEE_NATIVE
#pragma clang section data = "" bss = ""
static uint64_t trace_frame;
static bool trace_enabled,trace_replaying;
static uintptr_t trace_base;
enum { TraceFrames=16, TraceRecords=4096, TraceBytes=256 };
struct TraceRecord {
    const char* name;
    const void* address;
    size_t bytes;
    unsigned char data[TraceBytes];
};
static struct TraceHistory {
    struct TraceRecord records[TraceFrames][TraceRecords];
    uint64_t frames[TraceFrames];
    unsigned counts[TraceFrames];
} *trace_history;
static unsigned trace_cursor,trace_differences;
void nativeRandom_TraceFrame(uint64_t frame, bool enabled, bool replaying)
{
    // Scene boundary only: normal netplay reserves no diagnostic payload and
    // recording/resimulation never allocates while advancing a tick.
    if(frame==0 && !replaying) {
        if(!enabled) {free(trace_history);trace_history=NULL;}
        else if(!trace_history)trace_history=calloc(1,sizeof(*trace_history));
    }
    trace_frame=frame;trace_enabled=enabled && frame>=700 && trace_history;trace_replaying=replaying;
    if(trace_enabled && !trace_base)trace_base=(uintptr_t)GetModuleHandleW(NULL);
    trace_cursor=trace_differences=0;
    if(trace_enabled && !replaying) {
        trace_history->frames[frame%TraceFrames]=frame;
        trace_history->counts[frame%TraceFrames]=0;
    }
}
static void trace_random(const char* operation,void* caller,u32 before,u32 after,int argument)
{
    if(trace_enabled)fprintf(stderr,"[rng-call] frame=%llu replay=%u op=%s caller=%zx before=%08x after=%08x argument=%d\n",
        (unsigned long long)trace_frame,trace_replaying,operation,
        (uintptr_t)caller-trace_base,(unsigned)before,(unsigned)after,argument);
}
void nativeRandom_TraceBytes(const char* name,const void* data,size_t bytes)
{
    if (!trace_enabled || bytes>TraceBytes) return;
    unsigned slot=(unsigned)(trace_frame%TraceFrames),record=trace_cursor++;
    if(record>=TraceRecords)return;
    struct TraceRecord* baseline=&trace_history->records[slot][record];
    if(!trace_replaying) {
        baseline->name=name;baseline->address=data;baseline->bytes=bytes;
        memcpy(baseline->data,data,bytes);trace_history->counts[slot]=record+1;return;
    }
    if(trace_history->frames[slot]!=trace_frame)return;
    // A stack address is not an identity: scalar controls may be called from
    // different native stack depths on ordinary versus restored ticks.
    bool same=record<trace_history->counts[slot] && baseline->name==name &&
        baseline->bytes==bytes;
    size_t offset=0;
    if(same)while(offset<bytes && baseline->data[offset]==((const unsigned char*)data)[offset])++offset;
    if(same && offset==bytes)return;
    if(trace_differences++>=10)return;
    fprintf(stderr,"[rng-state-diff] frame=%llu record=%u name=%s address=%p baseline_name=%s baseline_address=%p offset=%zu before=%u after=%u bytes=",
        (unsigned long long)trace_frame,record,name,data,
        record<trace_history->counts[slot]?baseline->name:"missing",
        record<trace_history->counts[slot]?baseline->address:NULL,offset,
        record<trace_history->counts[slot]?baseline->data[offset]:0,((const unsigned char*)data)[offset]);
    const unsigned char* at=data;
    char hex[4096];const char digits[]="0123456789abcdef";
    if(bytes>sizeof(hex)/2)bytes=sizeof(hex)/2;
    for(size_t i=0;i<bytes;++i) {hex[2*i]=digits[at[i]>>4];hex[2*i+1]=digits[at[i]&15];}
    fwrite(hex,2,bytes,stderr);
    fprintf(stderr,"\n");
    if(record<trace_history->counts[slot]) {
        fprintf(stderr,"[rng-state-baseline] frame=%llu record=%u bytes=",(unsigned long long)trace_frame,record);
        for(size_t i=0;i<baseline->bytes;++i) {hex[2*i]=digits[baseline->data[i]>>4];hex[2*i+1]=digits[baseline->data[i]&15];}
        fwrite(hex,2,baseline->bytes,stderr);fprintf(stderr,"\n");
    }
}
void nativeRandom_TraceEvent(const char* name,void* caller,const void* object,int value)
{
    if(!trace_enabled)return;
    struct {uintptr_t caller,object;int value,reserved;} event={
        (uintptr_t)caller-trace_base,(uintptr_t)object,value,0};
    nativeRandom_TraceBytes(name,&event,sizeof(event));
}
#endif

s32 HSD_Rand(void)
{
    u32 before=*seed_ptr;
    *seed_ptr = *seed_ptr * 214013 + 2531011;
#ifdef MELEE_NATIVE
    trace_random("rand",__builtin_return_address(0),before,*seed_ptr,0);
#endif
    return *seed_ptr >> 0x10;
}

f32 HSD_Randf(void)
{
    u32 before=*seed_ptr;
    *seed_ptr = *seed_ptr * 214013 + 2531011;
#ifdef MELEE_NATIVE
    trace_random("float",__builtin_return_address(0),before,*seed_ptr,0);
#endif
    return (f32) (*seed_ptr >> 0x10) / (1 << 16);
}

s32 HSD_Randi(s32 max_val)
{
    u32 before=*seed_ptr;
    s32 value=max_val * HSD_Rand() / (1 << 16);
#ifdef MELEE_NATIVE
    trace_random("int",__builtin_return_address(0),before,*seed_ptr,max_val);
#endif
    return value;
}

void _HSD_RandForgetMemory(void* low, void* high)
{
    if (low <= (void*) seed_ptr && (void*) seed_ptr < high) {
        seed_ptr = &seed;
    }
    return;
}

static bool random_buffer(const void* data, size_t bytes)
{
    uintptr_t at = (uintptr_t)data;
    if (!data || !bytes || bytes > UINTPTR_MAX - at) return false;
    uintptr_t value = (uintptr_t)&seed, binding = (uintptr_t)&seed_ptr;
    return !(at < value + sizeof(seed) && value < at + bytes) &&
        !(at < binding + sizeof(seed_ptr) && binding < at + bytes);
}

bool nativeRandom_Save(void* data, size_t capacity, size_t* written)
{
    const size_t bytes = sizeof(NativeRandomSnapshot);
    if (seed_ptr != &seed || capacity < bytes || !random_buffer(data, bytes) ||
        !random_buffer(written, sizeof(*written))) return false;
    uintptr_t a = (uintptr_t)data, b = (uintptr_t)written;
    if (a < b + sizeof(*written) && b < a + bytes) return false;
    NativeRandomSnapshot state = {seed, 0};
    memcpy(data, &state, bytes);
    *written = bytes;
    return true;
}

bool nativeRandom_ValidateSnapshot(const void* data, size_t bytes)
{
    if (seed_ptr != &seed || bytes != sizeof(NativeRandomSnapshot) ||
        !random_buffer(data, bytes)) return false;
    NativeRandomSnapshot state;
    memcpy(&state, data, sizeof(state));
    return state.reserved == 0;
}

bool nativeRandom_Load(const void* data, size_t bytes)
{
    if (!nativeRandom_ValidateSnapshot(data, bytes)) return false;
    NativeRandomSnapshot state;
    memcpy(&state, data, sizeof(state));
    seed = state.seed;
    return true;
}

#ifdef MELEE_NATIVE
bool nativeRandom_AdmitHistory(NativeMatchArena* arena)
{
    if(!arena || !arena->lifetime)return false;
#define ROOT(v) do { if(!nativeMatchArena_AddRoot(arena,&(v),sizeof(v)))return false; } while(0)
    ROOT(seed);
#undef ROOT
    return true;
}
#endif
