#pragma once
#include <melee/gm/native_content_store.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct NativeMenuPortrait {
    unsigned char* pixels;
    unsigned short width, height;
    char title[65];
} NativeMenuPortrait;
// Load at menu construction or a fighter-choice change. The caller owns
// pixels.
typedef enum NativeMenuPortraitUse { NativeMenuPortrait_Tile, NativeMenuPortrait_PlayerCard } NativeMenuPortraitUse;
bool nativeMenuPortrait_Load(NativeMenuPortrait* result,
                             const NativeContentId* package,
                             uint64_t stock_id, NativeMenuPortraitUse use);
void nativeMenuPortrait_Free(NativeMenuPortrait* portrait);
#ifdef __cplusplus
}
#endif
