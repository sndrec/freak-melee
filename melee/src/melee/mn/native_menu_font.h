#pragma once
#ifdef MELEE_NATIVE
#include <Runtime/platform.h>
typedef enum NativeMenuFontFace {
    NativeMenuFont_Heading, NativeMenuFont_ResultName,
    NativeMenuFont_ResultWinner, NativeMenuFont_Notice, NativeMenuFont_Tile,
    NativeMenuFont_Count
} NativeMenuFontFace;
void nativeMenuFont_Acquire(void);
void nativeMenuFont_Release(void);
void nativeMenuFont_Draw(NativeMenuFontFace face,const char* text,
                        float x,float y,float z,float width,float height);
#endif
