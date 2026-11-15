#ifndef SYSDOLPHIN_AUDIO_VOICE_H
#define SYSDOLPHIN_AUDIO_VOICE_H
#include "audio_sample.h"
#define HSD_AUDIO_VOICES 64

typedef struct HSD_AudioVoice {
    u32 index, priority;
    u64 priority_order;
    void (*dropped)(void*);
    bool allocated, running, streaming, music;
    HSD_SfxVoice sample;
    AXPBSRC source;
    AXPBMIX mix;
    u16 mix_mask;
    s16 last_mix[9];
    AXPBVE envelope;
    AXPBITD itd;
    HSD_AudioSample decoder;
} HSD_AudioVoice;

// All voice ownership and control operations run on the game/audio owner thread.
void HSD_AudioVoiceInit(void);
void HSD_AudioVoiceConfigureMix(HSD_AudioVoice* voice, const AXPBMIX* mix);
HSD_AudioVoice* HSD_AudioVoiceAcquire(u32 priority, void (*dropped)(void*));
void HSD_AudioVoiceRelease(HSD_AudioVoice* voice);
void HSD_AudioVoicePriority(HSD_AudioVoice* voice, u32 priority);
bool HSD_AudioVoiceStart(HSD_AudioVoice* voice);
size_t HSD_AudioVoiceRead(HSD_AudioVoice* voice, const u8* storage,
                         size_t size, s16* output, size_t count);
// Original default four-tap polyphase filter; persists fractional phase/history.
void HSD_AudioVoiceResample(HSD_AudioVoice* voice, const u8* storage,
                            size_t storage_size, s16* output, size_t count);
#endif
