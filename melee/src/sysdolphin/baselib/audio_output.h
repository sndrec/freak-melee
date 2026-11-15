#ifndef SYSDOLPHIN_AUDIO_OUTPUT_H
#define SYSDOLPHIN_AUDIO_OUTPUT_H

#include <stdbool.h>

// All calls belong to the game thread. SDL consumes queued PCM only.
bool HSD_AudioOutputInit(void);
void HSD_AudioOutputSetMuted(bool muted);
void HSD_AudioOutputSetMasterVolume(unsigned percent);
void HSD_AudioOutputPoll(void);
void HSD_AudioOutputShutdown(void);

#endif
