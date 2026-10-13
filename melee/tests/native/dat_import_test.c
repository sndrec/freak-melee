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

static void put32(u8* p, u32 n)
{
    p[0] = (u8) (n >> 24); p[1] = (u8) (n >> 16);
    p[2] = (u8) (n >> 8); p[3] = (u8) n;
}

void test_dat_import(void)
{
    static const u32 slots[] = { 8, 12, 20, 28, 100, 104, 108, 112,
                                192, 196, 200, 204, 208, 312 };
    u8 file[568] = { 0 };
    u8 original[sizeof(file)];
    u8 arena[8193];
    u8* data = file + 32;
    HSD_Dat dat;
    HSD_DatImport import;
    HSD_MObjDesc* material;
    put32(file, sizeof(file)); put32(file + 4, 480); put32(file + 8, 14);
    for (int i = 0; i < 14; ++i) {
        put32(file + 512 + i * 4, slots[i]);
    }
    put32(data + 4, 0x20); put32(data + 8, 24);
    put32(data + 12, 232); put32(data + 20, 252);
    put32(data + 28, 116);
    for (int i = 0; i < 2; ++i) {
        u32 t = 24 + i * 92;
        put32(data + t + 28, 0x3F800000);
        put32(data + t + 32, 0x3F800000);
        put32(data + t + 36, 0x3F800000);
        put32(data + t + 68, 0x3F000000);
        put32(data + t + 76, 208); put32(data + t + 80, 312);
        put32(data + t + 84, 296); put32(data + t + 88, 264);
    }
    put32(data + 208, 352); put32(data + 212, (4 << 16) | 4);
    put32(data + 216, GX_TF_RGBA8);
    memset(data + 232, 0x55, 12);
    put32(data + 244, 0x3F000000); put32(data + 248, 0x41800000);
    for (int i = 0; i < 12; ++i) data[252 + i] = (u8) i;
    memset(data + 264, 0x27, 28); put32(data + 292, 0x12345678);
    put32(data + 312, 416); put32(data + 316, GX_TL_RGB565);
    put32(data + 324, 16 << 16);
    memset(data + 352, 0xE3, 64); memset(data + 416, 0x4B, 32);
    memcpy(original, file, sizeof(file));
    require(HSD_DatOpen(&dat, file, sizeof(file)) == HSD_DAT_OK, "Material fixture rejected");
    require(HSD_DatImportInit(&import, &dat, arena + 1, sizeof(arena) - 1, 32) == HSD_DAT_OK,
            "Caller-owned import arena could not initialize");
    material = HSD_DatImportGraph(&import, 0, DAT_MOBJ);
    require(material && material->rendermode == 0x20 && material->texdesc->next,
            "Native material graph was not assembled");
    HSD_TObjDesc* texture = material->texdesc;
    HSD_TObjDesc* next = texture->next;
    require(next->next == NULL && texture->imagedesc == next->imagedesc &&
                texture->tlutdesc == next->tlutdesc && texture->lod == next->lod &&
                texture->tev == next->tev,
            "Shared descriptors were duplicated or NULL links changed");
    require(texture->scale.x == 1 && texture->scale.y == 1 && texture->scale.z == 1 &&
                texture->blending == 0.5f && texture->imagedesc->image_ptr == data + 352 &&
                texture->tlutdesc->lut == data + 416 && texture->tev->active == 0x12345678,
            "Material graph fields were decoded incorrectly");
    require(material->mat->ambient.r == 0x55 && material->mat->alpha == 0.5f &&
                material->mat->shininess == 16 && material->pedesc->alpha_comp1 == 11,
            "Material color or pixel-engine fields changed");
    u32 count = import.record_count;
    size_t used = import.arena_used;
    require(HSD_DatImportGraph(&import, 0, DAT_MOBJ) == material &&
                import.record_count == count && import.arena_used == used,
            "Repeated root import allocated another graph");
    require(memcmp(file, original, sizeof(file)) == 0, "Graph conversion modified the DAT");

    require(HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 1) == HSD_DAT_OK &&
                HSD_DatImportGraph(&import, 0, DAT_MOBJ) == NULL && import.status == HSD_DAT_CAPACITY,
            "Record limit returned a partial material graph");
    require(HSD_DatImportInit(&import, &dat, arena, 16, 32) == HSD_DAT_CAPACITY,
            "Undersized import arena was accepted");
    puts("Native material graph sharing, fields, identity and capacity tests passed");
}
