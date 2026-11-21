#include <sysdolphin/baselib/dat_texture.h>
#include <sysdolphin/baselib/dat_import.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

static void put32(u8* bytes, u32 value)
{
    bytes[0] = (u8) (value >> 24); bytes[1] = (u8) (value >> 16);
    bytes[2] = (u8) (value >> 8); bytes[3] = (u8) value;
}

void test_dat_textures(void)
{
    u8 file[296] = { 0 };
    u8 original[sizeof(file)];
    u8* data = file + 32;
    HSD_Dat dat;
    HSD_ImageDesc image;
    HSD_TlutDesc palette;
    HSD_TexLODDesc lod;
    put32(file, sizeof(file));
    put32(file + 4, 256);
    put32(file + 8, 2);
    put32(file + 288, 0);
    put32(file + 292, 32);
    put32(data, 64);
    put32(data + 4, (4 << 16) | 4);
    put32(data + 8, GX_TF_RGBA8);
    put32(data + 12, 1);
    put32(data + 20, 0x3F800000); // maxLOD = 1
    put32(data + 32, 192);
    put32(data + 36, GX_TL_RGB565);
    put32(data + 40, 7);
    put32(data + 44, 16 << 16);
    put32(data + 48, GX_LIN_MIP_LIN);
    put32(data + 52, 0xBF000000); // LODBias = -0.5
    data[56] = 1;
    put32(data + 60, GX_ANISO_4);
    memset(data + 64, 0xC3, 128);
    memset(data + 192, 0x6A, 32);
    memcpy(original, file, sizeof(file));
    require(HSD_DatOpen(&dat, file, sizeof(file)) == HSD_DAT_OK, "Texture fixture rejected");
    HSD_DatImageSize size;
    require(HSD_DatDecodeImage(&dat, 0, &image, &size) == HSD_DAT_OK && size.source_bytes == 128 && size.texels == 20 &&
                image.image_ptr == data + 64 && image.width == 4 && image.height == 4 &&
                image.format == GX_TF_RGBA8 && image.mipmap == 1 && image.maxLOD == 1,
            "Native image descriptor conversion failed");
    require(HSD_DatDecodePalette(&dat, 32, &palette) == HSD_DAT_OK &&
                palette.lut == data + 192 && palette.fmt == GX_TL_RGB565 &&
                palette.tlut_name == 7 && palette.n_entries == 16,
            "Native palette descriptor conversion failed");
    require(HSD_DatDecodeLOD(&dat, 48, &lod) == HSD_DAT_OK &&
                lod.minFilt == GX_LIN_MIP_LIN && lod.LODBias == -0.5f &&
                lod.bias_clamp == 1 && lod.edgeLODEnable == 0 &&
                lod.max_anisotropy == GX_ANISO_4,
            "Native LOD descriptor conversion failed");
    require(memcmp(file, original, sizeof(file)) == 0, "Texture bytes were modified");
    HSD_DatImport graph;
    void* arena[128];
    require(HSD_DatImportInit(&graph, &dat, arena, sizeof(arena), 4) == HSD_DAT_OK, "Texture graph setup");
    graph.texture_byte_limit = 80 + 64;
    require(HSD_DatImportGraph(&graph, 0, DAT_IMAGE) &&
            HSD_DatImportGraph(&graph, 0, DAT_IMAGE) &&
            HSD_DatImportGraph(&graph, 32, DAT_PALETTE) &&
            graph.decoded_texture_bytes == graph.texture_byte_limit,
            "Repeated image references share one charge and the palette fits at the limit");
    require(HSD_DatImportInit(&graph, &dat, arena, sizeof(arena), 4) == HSD_DAT_OK, "Reset texture graph");
    graph.texture_byte_limit = 143;
    require(HSD_DatImportGraph(&graph, 0, DAT_IMAGE) &&
            HSD_DatImportGraph(&graph, 32, DAT_PALETTE) == NULL &&
            graph.status == HSD_DAT_RESOURCE_LIMIT && graph.error_offset == 32,
            "The next texture allocation must fit the remaining budget");
    put32(data + 4, (2 << 16) | 1);
    put32(data + 16, 0x40000000); // minLOD = 2
    put32(data + 20, 0x41000000); // maxLOD = 8
    require(HSD_DatDecodeImage(&dat, 0, &image, &size) == HSD_DAT_OK &&
            image.minLOD == 1 && image.maxLOD == 1 && size.texels == 3 && size.source_bytes == 128,
            "Renderer LODs end at the last physical mip of a rectangular texture");
    memcpy(file, original, sizeof(file));
    put32(data, 240);
    require(HSD_DatOpen(&dat, file, sizeof(file)) == HSD_DAT_OK &&
                HSD_DatDecodeImage(&dat, 0, &image, &size) == HSD_DAT_DESCRIPTOR &&
                image.image_ptr == NULL && size.source_bytes == 0 && size.texels == 0,
            "Truncated mip chain accepted or partial native output retained");
    memcpy(file, original, sizeof(file));
    put32(data, 96);
    put32(data + 20, 0x3FC00000); // Fractional LOD 1.5 needs the third level.
    require(HSD_DatOpen(&dat, file, sizeof(file)) == HSD_DAT_OK &&
                HSD_DatDecodeImage(&dat, 0, &image, NULL) == HSD_DAT_DESCRIPTOR,
            "Fractional LOD did not include the upper mip level");
    memcpy(file, original, sizeof(file));
    put32(data + 8, 0xFFFFFFFF);
    require(HSD_DatOpen(&dat, file, sizeof(file)) == HSD_DAT_OK &&
                HSD_DatDecodeImage(&dat, 0, &image, NULL) == HSD_DAT_DESCRIPTOR,
            "Unknown texture format accepted");
    puts("Native image, palette and LOD conversion passed");
}
