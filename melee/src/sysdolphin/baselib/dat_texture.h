#ifndef SYSDOLPHIN_BASELIB_DAT_TEXTURE_H
#define SYSDOLPHIN_BASELIB_DAT_TEXTURE_H

#include "dat.h"
#include "tobj.h"

// Outputs are native descriptors. Pixel/palette storage still belongs to the
// immutable DAT view and must outlive their use; no payload copying is needed.
HSD_DatResult HSD_DatDecodeImage(const HSD_Dat* dat, u32 offset,
                                HSD_ImageDesc* image, size_t* data_size);
HSD_DatResult HSD_DatDecodePalette(const HSD_Dat* dat, u32 offset,
                                  HSD_TlutDesc* palette);
HSD_DatResult HSD_DatDecodeLOD(const HSD_Dat* dat, u32 offset,
                              HSD_TexLODDesc* lod);

#endif
