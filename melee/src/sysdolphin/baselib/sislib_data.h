#ifndef SYSDOLPHIN_BASELIB_SISLIB_DATA_H
#define SYSDOLPHIN_BASELIB_SISLIB_DATA_H
#include <Runtime/platform.h>

void HSD_SisLib_ResetData(void);
void HSD_SisLib_FreeData(s32 font_idx);
const u8* HSD_SisLib_Jump(s32 font_idx, const u8* operand);
#endif
