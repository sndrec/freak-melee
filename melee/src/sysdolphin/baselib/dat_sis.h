#ifndef SYSDOLPHIN_BASELIB_DAT_SIS_H
#define SYSDOLPHIN_BASELIB_DAT_SIS_H
#include "dat.h"

// SIS roots contain a pointer table followed by byte-encoded text/font data.
bool HSD_DatSisLayout(const HSD_Dat* dat, u32 root, u32* count);
bool HSD_DatImportSis(const HSD_Dat* dat, u32 root, void** table,
                      size_t capacity);
#endif
