#ifndef SYSDOLPHIN_AUDIO_FRAME_H
#define SYSDOLPHIN_AUDIO_FRAME_H

#include "audio_mix.h"

enum { HSD_AUDIO_FRAME_SAMPLES = 160 };
void HSD_AudioFrameReset(void);
void HSD_AudioRenderFrame(const u8* storage, size_t size,
                          s16 output[HSD_AUDIO_FRAME_SAMPLES * 2]);

#endif
