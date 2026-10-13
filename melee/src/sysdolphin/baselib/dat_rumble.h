#ifndef SYSDOLPHIN_DAT_RUMBLE_H
#define SYSDOLPHIN_DAT_RUMBLE_H
#include "dat.h"

typedef struct HSD_RumblePattern {
    u16* commands;
    u8 priority;
} HSD_RumblePattern;

bool HSD_DatRumbleLayout(const HSD_Dat* dat, u32* patterns, u32* words);
bool HSD_DatImportRumble(const HSD_Dat* dat, HSD_RumblePattern* patterns,
                         size_t pattern_capacity, u16* words, size_t word_capacity);
#endif
