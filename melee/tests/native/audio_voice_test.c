#include <sysdolphin/baselib/audio_voice.h>
#include <stdio.h>
#include <stdlib.h>
static unsigned dropped_count, dropped_index;
static void check(int ok) { if (!ok) { fputs("voice pool mismatch\n", stderr); exit(1); } }
void __assert(char* file, u32 line, char* expression)
{
    fprintf(stderr, "%s:%lu: %s\n", file, line, expression); abort();
}
static void dropped(void* ptr)
{
    HSD_AudioVoice* voice = ptr;
    ++dropped_count; dropped_index = voice->index;
    // A stealing callback cannot recursively steal its reserved slot.
    check(HSD_AudioVoiceAcquire(1, NULL) == NULL);
}
int main(void)
{
    HSD_AudioVoice* v[64];
    HSD_AudioVoiceInit();
    for (int i = 0; i < 64; ++i) {
        v[i] = HSD_AudioVoiceAcquire(10, dropped);
        check(v[i] && v[i]->index == 63 - i && !v[i]->running);
    }
    check(!HSD_AudioVoiceAcquire(10, dropped) && !dropped_count);
    HSD_AudioVoicePriority(v[0], 10); // Goes to the newest end of its priority.
    HSD_AudioVoice* stolen = HSD_AudioVoiceAcquire(11, dropped);
    check(stolen == v[1] && dropped_count == 1 && dropped_index == v[1]->index);
    HSD_AudioVoicePriority(v[32], 2);
    stolen = HSD_AudioVoiceAcquire(3, dropped);
    check(stolen == v[32] && dropped_count == 2);
    HSD_AudioVoiceRelease(v[12]);
    HSD_AudioVoice* voice = HSD_AudioVoiceAcquire(1, NULL);
    check(voice == v[12] && dropped_count == 2);
    voice->sample.address.currentAddressLo = 2;
    voice->sample.address.endAddressLo = 3;
    u8 data[2] = {0, 0x27}; s16 output[4];
    check(HSD_AudioVoiceStart(voice));
    check(HSD_AudioVoiceRead(voice, data, 2, output, 1) == 1 && output[0] == 2);
    check(voice->running && voice->sample.address.currentAddressLo == 3);
    check(HSD_AudioVoiceRead(voice, data, 2, output, 4) == 1 && output[0] == 7);
    check(!voice->running && voice->sample.adpcm.yn1 == 7);
    // Loop metadata may be retargeted while a voice is running.
    voice->sample.address.currentAddressLo = 2;
    voice->sample.address.loopFlag = 1;
    voice->sample.address.loopAddressLo = 2;
    check(HSD_AudioVoiceStart(voice));
    check(HSD_AudioVoiceRead(voice, data, 2, output, 1) == 1);
    voice->sample.address.loopAddressLo = 3;
    check(HSD_AudioVoiceRead(voice, data, 2, output, 3) == 3);
    check(output[0] == 7 && output[1] == 7 && output[2] == 7);
    for (int i = 0; i < 64; ++i) HSD_AudioVoiceRelease(v[i]);
    for (int i = 0; i < 64; ++i) check(HSD_AudioVoiceAcquire(1, NULL) != NULL);
    check(HSD_AudioVoiceAcquire(1, NULL) == NULL);
    puts("Native voice pool exhaustion, stealing, reuse and sample playback verified");
    return 0;
}
