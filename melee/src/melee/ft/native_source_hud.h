#ifndef MELEE_FT_NATIVE_SOURCE_HUD_H
#define MELEE_FT_NATIVE_SOURCE_HUD_H
#include "native_source_resources.h"
#include <sysdolphin/baselib/tobj.h>

typedef enum NativeSourceHudTarget {
    NativeSourceHud_Stock, NativeSourceHud_Emblem, NativeSourceHud_Count
} NativeSourceHudTarget;
typedef struct NativeSourceHudImage {
    HSD_ImageDesc image;
} NativeSourceHudImage;
typedef struct NativeSourceHud {
    NativeSourceHudImage images[NativeSourceHud_Count];
} NativeSourceHud;

// Decoded once with the immutable package; no decoding or allocation in HUD draw.
bool nativeSourceHud_Prepare(NativeSourceHud*, const NativeSourceResources*,
                             char* error, size_t error_capacity);
void nativeSourceHud_Release(NativeSourceHud*);
void nativeSourceHud_Apply(unsigned player, HSD_TObj*, NativeSourceHudTarget);
#endif
