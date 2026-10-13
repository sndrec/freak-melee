#ifndef SYSDOLPHIN_BASELIB_SISLIB_STATIC_H
#define SYSDOLPHIN_BASELIB_SISLIB_STATIC_H

#include <Runtime/platform.h>

/// Shared by the sislib translation units.
static inline u8* HSD_SisLib_BytePtr(void* ptr)
{
    return ptr;
}

// SIS operands can begin at any byte alignment and remain big-endian.
static inline u16 HSD_SisLib_ReadU16(const void* bytes)
{
    const u8* p = bytes;
    return (u16) ((u16) p[0] << 8 | p[1]);
}

#endif
