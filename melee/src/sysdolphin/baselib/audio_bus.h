#ifndef SYSDOLPHIN_AUDIO_BUS_H
#define SYSDOLPHIN_AUDIO_BUS_H

#include <dolphin/types.h>

// One 5 ms auxiliary frame at 32 kHz. Channels need not be contiguous.
typedef struct HSD_AudioAuxBuffers {
    s32* left;
    s32* right;
    s32* surround;
} HSD_AudioAuxBuffers;

#endif
