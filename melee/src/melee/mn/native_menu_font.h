#pragma once
#ifdef MELEE_NATIVE
#include <Runtime/platform.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum NativeMenuFontFace {
    NativeMenuFont_Heading, NativeMenuFont_ResultName,
    NativeMenuFont_ResultWinner, NativeMenuFont_Notice, NativeMenuFont_Tile,
    NativeMenuFont_Count
} NativeMenuFontFace;
void nativeMenuFont_Acquire(void);
void nativeMenuFont_Release(void);
float nativeMenuFont_Width(NativeMenuFontFace face,const char* text,float height);
void nativeMenuFont_Draw(NativeMenuFontFace face,const char* text,
                        float x,float y,float z,float width,float height,
                        u32 rgba, bool centered);
#ifdef __cplusplus
}
#endif
#endif
