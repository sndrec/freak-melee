#ifndef SYSDOLPHIN_BASELIB_DAT_AUDIO_H
#define SYSDOLPHIN_BASELIB_DAT_AUDIO_H

#include "dat.h"

enum {
    HSD_AUDIO_LOAD_TABLES = 4,
    HSD_AUDIO_LOAD_LISTS = 30,
};

typedef struct HSD_AudioLoadData {
    u32** tables[HSD_AUDIO_LOAD_TABLES];
} HSD_AudioLoadData;

bool HSD_DatAudioLoadLayout(const HSD_Dat* dat, u32 root,
                            u32* value_count);
bool HSD_DatImportAudioLoad(const HSD_Dat* dat, u32 root,
                            HSD_AudioLoadData* output,
                            u32** list_storage, size_t list_capacity,
                            u32* value_storage, size_t value_capacity);

#endif
