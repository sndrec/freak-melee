#pragma once
#include <stdbool.h>
#include <dolphin/types.h>
// Game-thread recording of mixed PCM, before the playback device's mute gain.
bool HSD_AudioCaptureSetFile(const char* path);
void HSD_AudioCaptureWrite(const s16* samples, unsigned frames);
