#ifndef SYSDOLPHIN_BASELIB_AUDIO_ROLLBACK_H
#define SYSDOLPHIN_BASELIB_AUDIO_ROLLBACK_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stdint.h>

typedef struct HSD_AudioRollbackStats {
    uint32_t reused_starts;
    uint32_t new_starts;
    uint32_t canceled_starts;
} HSD_AudioRollbackStats;

void HSD_AudioRollbackReset(void);
bool HSD_AudioRollbackBeginFrame(uint64_t frame, bool resimulating);
void HSD_AudioRollbackAbortFrame(void);
bool HSD_AudioRollbackReuseStart(int sound_id, uint8_t volume, uint8_t pan,
                                 int track, int channel, int* instance);
void HSD_AudioRollbackRecordStart(int sound_id, uint8_t volume, uint8_t pan,
                                  int track, int channel, int instance);
bool HSD_AudioRollbackEndFrame(void);
bool HSD_AudioRollbackFrameHasStarts(uint64_t frame);
HSD_AudioRollbackStats HSD_AudioRollbackGetStats(void);

#endif
#endif
