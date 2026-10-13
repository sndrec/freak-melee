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

void test_dat_model(void)
{
    static const u32 slots[] = { 8, 16, 56, 60, 140, 152, 160, 164,
                                188, 216, 224, 232, 304 };
    u8 file[500] = { 0 };
    u8 original[sizeof(file)];
    u8 arena[16384];
    u8* data = file + 32;
    HSD_Dat dat;
    HSD_DatImport import;
    put32(file, sizeof(file)); put32(file + 4, 416); put32(file + 8, 13);
    for (int i = 0; i < 13; ++i) put32(file + 448 + i * 4, slots[i]);
    put32(data + 8, 64); put32(data + 16, 128);
    put32(data + 56, 248); put32(data + 60, 296);
    for (int j = 0; j < 2; ++j) {
        for (int i = 0; i < 3; ++i) put32(data + j * 64 + 32 + i * 4, 0x3F800000);
    }
    put32(data + 108, 0x40000000); // Child translation X = 2.
    put32(data + 140, 144); put32(data + 152, 168);
    put32(data + 156, ((u32) POBJ_ENVELOPE << 16) | 1);
    put32(data + 160, 320); put32(data + 164, 216);
    put32(data + 168, GX_VA_POS); put32(data + 172, GX_INDEX8);
    put32(data + 176, GX_POS_XYZ); put32(data + 180, GX_F32);
    put32(data + 184, 12); put32(data + 188, 352);
    put32(data + 192, GX_VA_NULL);
    put32(data + 216, 224);
    put32(data + 228, 0x3E800000); // Root weight .25, pointer offset zero.
    put32(data + 232, 64); put32(data + 236, 0x3F400000);
    for (int i = 0; i < 3; ++i) put32(data + 248 + (i * 4 + i) * 4, 0x3F800000);
    put32(data + 300, REFTYPE_JOBJ); put32(data + 304, 64);
    data[320] = 0x90; data[322] = 3; data[324] = 1; data[325] = 2;
    for (int i = 0; i < 9; ++i) put32(data + 352 + i * 4, 0x3F800000);
    memcpy(original, file, sizeof(file));
    require(HSD_DatOpen(&dat, file, sizeof(file)) == HSD_DAT_OK &&
                HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 64) == HSD_DAT_OK,
            "Model fixture rejected");
    HSD_Joint* root = HSD_DatImportGraph(&import, 0, DAT_JOINT);
    require(root && root->child && root->child->position.x == 2 && root->scale.y == 1,
            "Joint links or transforms changed");
    require(root->mtx && root->mtx[0][0] == 1 && root->mtx[1][1] == 1 &&
                root->mtx[2][2] == 1 && root->mtx[0][1] == 0,
            "Joint matrix was not converted to native floats");
    require(root->robjdesc->u.joint == root->child, "Reference object duplicated its target joint");
    HSD_PObjDesc* polygon = root->u.dobjdesc->pobjdesc;
    require(polygon && polygon->n_display == 1 && polygon->display != data + 320 && memcmp(polygon->display, data + 320, 32) == 0 &&
                polygon->verts[0].attr == GX_VA_POS && polygon->verts[0].stride == 12 &&
                polygon->verts[0].vertex == data + 352 && polygon->verts[1].attr == GX_VA_NULL,
            "Geometry descriptor or original payload references changed");
    HSD_EnvelopeDesc** envelopes = polygon->u.envelope_p;
    require(envelopes && envelopes[1] == NULL && envelopes[0][0].joint == root &&
                envelopes[0][0].weight == 0.25f && envelopes[0][1].joint == root->child &&
                envelopes[0][1].weight == 0.75f && envelopes[0][2].joint == NULL,
            "Skinning weights, shared joints, or terminators changed");
    require(memcmp(file, original, sizeof(file)) == 0, "Model conversion modified original bytes");
    require(polygon->verts[0].vertex_size == 36 &&
                !polygon->verts[0].vertex_little_endian,
            "Indexed vertex extent or byte order is incorrect");
    put32(data + 156, ((u32) (POBJ_ENVELOPE | 0x2000) << 16) | 1);
    require(HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 64) == HSD_DAT_OK,
            "Cannot reset refraction model import");
    root = HSD_DatImportGraph(&import, 0, DAT_JOINT);
    require(root != NULL, "Cannot import refraction model");
    polygon = root->u.dobjdesc->pobjdesc;
    require(polygon->display != data + 320 &&
                memcmp(polygon->display, data + 320, 32) == 0 &&
                (u8*) polygon->display >= arena && (u8*) polygon->display + 32 <= arena + sizeof(arena),
            "Refraction display list is not owned by the model arena");
    ((u8*) polygon->display)[324 - 320] = 7;
    require(data[324] == 1, "Writable refraction payload modified source DAT");
    put32(data + 156, ((u32) POBJ_ENVELOPE << 16) | 1);
    data[325] = 255;
    require(HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 64) == HSD_DAT_OK &&
                HSD_DatImportGraph(&import, 0, DAT_JOINT) == NULL,
            "Out-of-range vertex index was accepted");
    data[325] = 2;
    data[322] = 255;
    require(HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 64) == HSD_DAT_OK &&
                HSD_DatImportGraph(&import, 0, DAT_JOINT) == NULL,
            "Truncated primitive payload was accepted");
    data[322] = 3;
    data[320] = 0x08;
    require(HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 64) == HSD_DAT_OK &&
                HSD_DatImportGraph(&import, 0, DAT_JOINT) == NULL,
            "Register write was accepted as a geometry primitive");
    data[320] = 0x90;
    put32(data + 156, ((u32) POBJ_SHAPEANIM << 16) | 1);
    require(HSD_DatOpen(&dat, file, sizeof(file)) == HSD_DAT_OK &&
                HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 64) == HSD_DAT_OK &&
                HSD_DatImportGraph(&import, 0, DAT_JOINT) == NULL,
            "Malformed morph targets were accepted");
    puts("Native skeleton, geometry descriptors, matrix and skinning references passed");
}
