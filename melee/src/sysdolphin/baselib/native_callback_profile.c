#include "native_callback_profile.h"
#include "debug.h"
#include <SDL3/SDL_timer.h>
#include <stdlib.h>
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma clang section data = "" bss = ""

enum { ProfileBuckets = 512, WarmupFrame = 120 };
typedef struct CallbackProfileEntry {
    uintptr_t callback;
    NativeCallbackProfileKind kind;
    uint64_t calls[2], elapsed[2], maximum[2];
} CallbackProfileEntry;
bool nativeCallbackProfileEnabled;
static CallbackProfileEntry entries[ProfileBuckets];
static bool requested, hidden;
static unsigned overflow;

void nativeCallbackProfile_Init(void)
{
    const char* value = getenv("FREAK_MELEE_PROFILE_CALLBACKS");
    requested = value && !strcmp(value, "1");
    nativeCallbackProfileEnabled = false;
    memset(entries, 0, sizeof(entries));
    overflow = 0;
}
void nativeCallbackProfile_SetFrame(uint64_t frame, bool is_hidden)
{
    nativeCallbackProfileEnabled = requested && frame >= WarmupFrame;
    hidden = is_hidden;
}
NativeCallbackProfileStamp nativeCallbackProfile_Begin(
    uintptr_t callback, NativeCallbackProfileKind kind)
{
    unsigned bucket = ((callback >> 4) * 2654435761U + kind) & (ProfileBuckets - 1);
    for (unsigned scanned = 0; scanned < ProfileBuckets; ++scanned) {
        CallbackProfileEntry* entry = &entries[bucket];
        if (!entry->callback) {
            entry->callback = callback;
            entry->kind = kind;
        }
        if (entry->callback == callback && entry->kind == kind) {
            return (NativeCallbackProfileStamp) { SDL_GetTicksNS(), bucket, hidden };
        }
        bucket = (bucket + 1) & (ProfileBuckets - 1);
    }
    ++overflow;
    return (NativeCallbackProfileStamp) { 0, ProfileBuckets, 0 };
}
void nativeCallbackProfile_End(NativeCallbackProfileStamp stamp)
{
    if (stamp.bucket == ProfileBuckets) return;
    uint64_t elapsed = SDL_GetTicksNS() - stamp.started;
    CallbackProfileEntry* entry = &entries[stamp.bucket];
    ++entry->calls[stamp.lane];
    entry->elapsed[stamp.lane] += elapsed;
    if (elapsed > entry->maximum[stamp.lane]) entry->maximum[stamp.lane] = elapsed;
}
void nativeCallbackProfile_Report(void)
{
    if (!requested) return;
    uintptr_t base = (uintptr_t) GetModuleHandleW(NULL);
    OSReport("[callback-profile] warmup_frame=%u overflow=%u scope=inclusive-callbacks "
             "lanes=visible,hidden camera_includes_display=1\n", WarmupFrame, overflow);
    for (unsigned i = 0; i < ProfileBuckets; ++i) {
        const CallbackProfileEntry* entry = &entries[i];
        if (!entry->callback) continue;
        for (unsigned lane = 0; lane < 2; ++lane) {
            if (!entry->calls[lane]) continue;
            OSReport("[callback-profile] kind=%u rva=%llx hidden=%u calls=%llu "
                     "total_us=%.3f max_us=%.3f\n", entry->kind,
                     (unsigned long long)(entry->callback-base),lane,
                     (unsigned long long)entry->calls[lane],entry->elapsed[lane]/1000.0,
                     entry->maximum[lane]/1000.0);
        }
    }
    requested = nativeCallbackProfileEnabled = false;
}
