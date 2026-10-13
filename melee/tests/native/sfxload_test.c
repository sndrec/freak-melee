// Exercise the production loader's private state without an audio device.
#include <sysdolphin/baselib/synth.c>
#include <stdio.h>
#include <stdlib.h>

static unsigned allocations, completions;
static int next_request, pending_request, canceled_request;
static uintptr_t pending_dest, pending_offset;
static size_t pending_size;
static int pending_type;
static HSD_DevComCallback pending_callback;
static void* pending_context;
static u8 fixture[192];

static void check(int ok, const char* message)
{
    if (!ok) { fprintf(stderr, "%s\n", message); exit(1); }
}
void __assert(char* file, u32 line, char* expression)
{
    fprintf(stderr, "%s:%u: %s\n", file, (unsigned) line, expression); abort();
}
void OSReport(char* format, ...) {}
void* OSAllocFromHeap(int heap, unsigned long size)
{
    ++allocations; return calloc(1, size);
}
void OSFreeToHeap(int heap, void* memory)
{
    check(allocations != 0, "allocation lifetime"); --allocations; free(memory);
}
s32 DVDConvertPathToEntrynum(const char* name) { return atoi(name); }
void HSD_DevComPoll(void) {}
int HSD_DevComRequest(int file, uintptr_t src, uintptr_t dest, size_t size,
                      int type, int pri, HSD_DevComCallback callback, void* args)
{
    check(!pending_request, "only one in-flight bank request");
    pending_request = ++next_request;
    pending_offset = src; pending_dest = dest; pending_size = size;
    pending_type = type; pending_callback = callback; pending_context = args;
    return pending_request;
}
int HSD_DevComCancelEx(int request, u32 flags, HSD_DevComCallback cb, void* args)
{
    check(request == pending_request, "cancel current stage");
    canceled_request = request; return 0;
}
static void finish(void)
{
    int request = pending_request;
    bool canceled = canceled_request == request;
    HSD_DevComCallback callback = pending_callback;
    void* context = pending_context;
    void* destination = (void*) pending_dest;
    check(request != 0, "pending transfer");
    if (pending_type == 0x21) {
        // The read owns its buffer until completion, even when canceled.
        check(pending_offset + pending_size <= sizeof(fixture), "fixture range");
        memcpy(destination, fixture + pending_offset, pending_size);
    }
    pending_request = 0;
    if (callback) callback(request, context, destination, canceled);
}
static void loaded(int file, int mode) { ++completions; }
static void put32(u32 offset, u32 value)
{
    fixture[offset] = value >> 24; fixture[offset + 1] = value >> 16;
    fixture[offset + 2] = value >> 8; fixture[offset + 3] = value;
}
int main(void)
{
    put32(0, 72); put32(4, 32); put32(8, 1); put32(12, 100);
    put32(16, 1); put32(20, 32000);
    put32(24 + 8, 31); put32(24 + 12, 2);
    hsd_SynthSFXBankNum = 2;
    hsd_SynthSFXBankHead[0] = 0x10000;
    hsd_SynthSFXBankHead[1] = 0x20000;
    hsd_SynthSFXBankHead[2] = 0x30000;
    hsd_SynthSFXBank[0] = 0x10000; hsd_SynthSFXBank[1] = 0x20000;
    for (int stage = 0; stage < 3; ++stage) {
        HSD_SynthSFXLoad("1", 0, loaded, 0);
        for (int i = 0; i < stage; ++i) finish();
        unsigned before = allocations;
        check(HSD_SynthSFXCancelLoad(1), "cancel load");
        check(allocations == before, "cancel retains in-flight destination");
        finish();
        check(!allocations && !pending_request && !completions &&
              !HSD_SynthSFXGetPendingLoadCount(), "canceled stage cleanup");
    }
    HSD_SynthSFXLoad("1", 0, loaded, 0);
    HSD_SynthSFXLoad("2", 1, loaded, 0);
    for (int i = 0; i < 6; ++i) finish();
    check(completions == 2 && allocations == 2 && !pending_request, "queued load completion");
    check(HSD_Synth_804C29E0[100 & 31] == &HSD_Synth_804C2AE0[1]->entries[0], "duplicate ID newest first");
    HSD_AudioVoiceInit();
    HSD_Synth_804C28E0_1784[0].x1784 = 1.0f;
    int sound = HSD_Synth_80389334(100, 255, 255, 128, 10, 0,
                                  1.0f, 1.0f, 1.0f, 0.0f, 0.0f);
    check(sound > 0, "original synth starts native voice");
    struct HSD_SynthSFXNode* node = getNode(sound);
    check(node && node->voice[0]->running && node->voice[0]->source.ratioHi == 1 &&
          node->voice[0]->source.ratioLo == 0, "native playback parameters");
    HSD_SynthSFXStopNode(node);
    check(!node->voice[0]->allocated && !node->x0, "original synth releases native voice");
    sound = HSD_SynthSFXPlayWithGroup(100, 255, 255, 128, 10, 0, 0,
                                      1.0f, 1.0f, 1.0f, 0.0f, 0.0f);
    check(sound > 1 && getNode(sound), "group playback preserves exact sound handle");
    HSD_SynthSFXStopNode(getNode(sound));
    HSD_SfxEntry* stereo = HSD_Synth_804C29E0[100 & 31];
    stereo->voice_count = 2; stereo->voices[1] = stereo->voices[0];
    int sounds[132];
    for (int i = 0; i < 132; ++i) {
        sounds[i] = HSD_Synth_80389334(100, 255, 255, 128, 10, 0,
                                      1.0f, 1.0f, 1.0f, 0.0f, 0.0f);
        check(sounds[i] > 0, "stereo voice stealing");
        check(HSD_Synth_804D7720 <= 64, "bounded partner release queue");
    }
    check(!getNode(sounds[0]), "stolen stereo sound invalidated");
    HSD_SynthCallback();
    check(HSD_Synth_804D7720 == 0, "partner queue drained");
    for (int i = 100; i < 132; ++i) {
        struct HSD_SynthSFXNode* live = getNode(sounds[i]);
        check(live && live->voice[0]->allocated && live->voice[1]->allocated,
              "reused stereo voices survive partner cleanup");
        HSD_SynthSFXStopNode(live);
    }
    int paired = HSD_Synth_80389334(100, 255, 255, 128, 10, 0,
                                   1.0f, 1.0f, 1.0f, 0.0f, 0.0f);
    HSD_AudioVoice* partner = getNode(paired)->voice[1];
    HSD_AudioVoice* held[62];
    for (int i = 0; i < 62; ++i) held[i] = HSD_AudioVoiceAcquire(31, NULL);
    HSD_AudioVoice* borrowed = HSD_AudioVoiceAcquire(11, dropcallback);
    check(borrowed && !getNode(paired) && HSD_Synth_804D7720 == 1,
          "one stolen voice queues its unstolen partner");
    HSD_SynthCallback();
    check(!partner->allocated && borrowed->allocated && !HSD_Synth_804D7720,
          "partner cleanup preserves borrowed voice");
    HSD_AudioVoiceRelease(borrowed);
    for (int i = 0; i < 62; ++i) HSD_AudioVoiceRelease(held[i]);
    stereo->voice_count = 1;
    HSD_SynthSFXUnloadBank(0);
    check(allocations == 1 && HSD_Synth_804C29E0[100 & 31] == &HSD_Synth_804C2AE0[1]->entries[0], "unload older duplicate by identity");
    HSD_Synth_80388E08(2);
    check(!allocations && !HSD_Synth_804C29E0[100 & 31], "unload file group");
    HSD_SynthSFXBankDeflag(1);
    check(hsd_SynthSFXBank[1] == 0x20000, "compaction restores correct bank cursor");
    HSD_SynthSFXLoad("3", 0, loaded, 0);
    HSD_SynthSFXLoad("4", 0, loaded, 0);
    for (int i = 0; i < 6; ++i) finish();
    HSD_Synth_80388E08(3);
    check(HSD_Synth_804C2AE0[0]->aram_base == 0x10020, "hole before remaining bank");
    HSD_SynthSFXBankDeflag(0);
    check(pending_type == 0x1B && pending_offset == 0x10020 &&
          pending_dest == 0x10000 && pending_size == 32, "compaction transfer");
    check(hsd_SynthSFXBank[0] == 0x10020 &&
          HSD_Synth_804C2AE0[0]->aram_base == 0x10000, "compaction metadata/cursor");
    finish();
    check(!sfxGroupDataReaddressCounter, "compaction completion");
    HSD_SynthSFXUnloadBank(0);
    check(!allocations, "final bank allocation cleanup");
    puts("Bank loader stages, cancellation, duplicate IDs and unload verified");
    return 0;
}

// These device calls must never execute in the loader-only test.
void AISetDSPSampleRate(u32 rate) { check(0, "unexpected AISetDSPSampleRate"); }
void AISetStreamVolLeft(u8 vol) { check(0, "unexpected AISetStreamVolLeft"); }
void AISetStreamVolRight(u8 vol) { check(0, "unexpected AISetStreamVolRight"); }
u32 ARAlloc(u32 length) { check(0, "unexpected ARAlloc"); return 0; }
