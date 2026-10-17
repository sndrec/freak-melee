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
// Keep ordinary fighter names on one compressed line. Very long names use
// two balanced lines; the caller expands the name bar for those two lines.
unsigned nativeMenuFont_LayoutTileName(const char* text, float width,
                                      float height, char lines[2][128]);
void nativeMenuFont_Draw(NativeMenuFontFace face,const char* text,
                        float x,float y,float z,float width,float height,
                        u32 rgba, bool centered);
#ifdef __cplusplus
}
#endif
#endif
