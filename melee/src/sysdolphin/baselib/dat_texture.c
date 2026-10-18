#include "dat_texture.h"

#include <math.h>
#include <string.h>

static HSD_DatResult texture_data(const HSD_Dat* dat, u32 slot, size_t size,
                                   void** output)
{
    u32 offset;
    const char* external;
    HSD_DatReferenceKind kind = HSD_DatResolve(dat, slot, &offset, &external);
    if (kind == HSD_DAT_REF_EXTERNAL) {
        return HSD_DAT_EXTERNAL;
    }
    if (kind != HSD_DAT_REF_LOCAL) {
        return HSD_DAT_RELOCATION;
    }
    *output = (void*) HSD_DatBytes(dat, offset, size);
    return *output != NULL ? HSD_DAT_OK : HSD_DAT_DESCRIPTOR;
}

HSD_DatResult HSD_DatDecodeImage(const HSD_Dat* dat, u32 offset,
                                HSD_ImageDesc* image, size_t* data_size)
{
    HSD_ImageDesc result = { 0 };
    u32 format;
    unsigned tile_width, tile_height, tile_bytes = 32;
    size_t bytes = 0;
    unsigned width, height, levels;
    HSD_DatResult status;
    memset(image, 0, sizeof(*image));
    if (data_size) *data_size = 0;
    if (HSD_DatBytes(dat, offset, 24) == NULL) {
        return HSD_DAT_DESCRIPTOR;
    }
    HSD_DatReadU16(dat, offset + 4, &result.width);
    HSD_DatReadU16(dat, offset + 6, &result.height);
    HSD_DatReadU32(dat, offset + 8, &format);
    result.format = (GXTexFmt) format;
    HSD_DatReadU32(dat, offset + 12, &result.mipmap);
    HSD_DatReadF32(dat, offset + 16, &result.minLOD);
    HSD_DatReadF32(dat, offset + 20, &result.maxLOD);
    if (!result.width || !result.height || !isfinite(result.minLOD) ||
        !isfinite(result.maxLOD) || result.minLOD < 0 ||
        result.maxLOD < result.minLOD || result.maxLOD > 16) {
        return HSD_DAT_DESCRIPTOR;
    }
    switch (format) {
    case GX_TF_I4: case GX_TF_C4: case GX_TF_CMPR:
        tile_width = 8; tile_height = 8; break;
    case GX_TF_I8: case GX_TF_IA4: case GX_TF_C8:
        tile_width = 8; tile_height = 4; break;
    case GX_TF_RGBA8:
        tile_bytes = 64;
        tile_width = 4; tile_height = 4; break;
    case GX_TF_IA8: case GX_TF_RGB565: case GX_TF_RGB5A3: case GX_TF_C14X2:
        tile_width = 4; tile_height = 4; break;
    default:
        return HSD_DAT_DESCRIPTOR;
    }
    width = result.width;
    height = result.height;
    // Fractional LOD can blend with the next mip level.
    levels = result.mipmap ? (unsigned) ceilf(result.maxLOD) + 1 : 1;
    while (levels--) {
        bytes += (size_t) ((width + tile_width - 1) / tile_width) *
                 ((height + tile_height - 1) / tile_height) * tile_bytes;
        if (width == 1 && height == 1) {
            break;
        }
        width = width > 1 ? width / 2 : 1;
        height = height > 1 ? height / 2 : 1;
    }
    status = texture_data(dat, offset, bytes, &result.image_ptr);
    if (status == HSD_DAT_OK) {
        *image = result;
        if (data_size) *data_size = bytes;
    }
    return status;
}

HSD_DatResult HSD_DatDecodePalette(const HSD_Dat* dat, u32 offset,
                                  HSD_TlutDesc* palette)
{
    HSD_TlutDesc result = { 0 };
    u32 format;
    HSD_DatResult status;
    memset(palette, 0, sizeof(*palette));
    if (HSD_DatBytes(dat, offset, 16) == NULL) {
        return HSD_DAT_DESCRIPTOR;
    }
    HSD_DatReadU32(dat, offset + 4, &format);
    HSD_DatReadU32(dat, offset + 8, &result.tlut_name);
    HSD_DatReadU16(dat, offset + 12, &result.n_entries);
    if (format > GX_TL_RGB5A3 || result.n_entries == 0) {
        return HSD_DAT_DESCRIPTOR;
    }
    result.fmt = (GXTlutFmt) format;
    status = texture_data(dat, offset, (size_t) result.n_entries * 2, &result.lut);
    if (status == HSD_DAT_OK) {
        *palette = result;
    }
    return status;
}

HSD_DatResult HSD_DatDecodeLOD(const HSD_Dat* dat, u32 offset,
                              HSD_TexLODDesc* lod)
{
    HSD_TexLODDesc result = { 0 };
    u32 filter, anisotropy;
    const u8* record = HSD_DatBytes(dat, offset, 16);
    memset(lod, 0, sizeof(*lod));
    if (record == NULL) {
        return HSD_DAT_DESCRIPTOR;
    }
    HSD_DatReadU32(dat, offset, &filter);
    HSD_DatReadF32(dat, offset + 4, &result.LODBias);
    HSD_DatReadU32(dat, offset + 12, &anisotropy);
    if (filter > GX_LIN_MIP_LIN || anisotropy > GX_ANISO_4 ||
        !isfinite(result.LODBias)) {
        return HSD_DAT_DESCRIPTOR;
    }
    result.minFilt = (GXTexFilter) filter;
    result.bias_clamp = record[8];
    result.edgeLODEnable = record[9];
    result.max_anisotropy = (GXAnisotropy) anisotropy;
    *lod = result;
    return HSD_DAT_OK;
}
