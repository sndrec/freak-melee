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

static void test_scalar_joint_binding(void)
{
    // Joint at 0, controller at 20, track at 36, stream at 56.
    u8 file[32 + 64 + 12] = {0}, arena[4096];
    u8* data = file + 32;
    put32(file, sizeof(file)); put32(file + 4, 64); put32(file + 8, 3);
    put32(data + 64, 8); put32(data + 68, 28); put32(data + 72, 52);
    put32(data + 8, 20); put32(data + 28, 36);
    put32(data + 40, 6); put32(data + 52, 56);
    memcpy(data + 56, (u8[]){1, 0, 0, 0, 0x40, 1}, 6);
    const u8 channels[] = {HSD_A_J_TRAX, 0x32, 0x33, 0x34, 0x35};
    for (size_t i = 0; i < sizeof(channels); ++i) {
        HSD_Dat dat;
        HSD_DatImport import;
        data[48] = channels[i];
        require(HSD_DatOpen(&dat, file, sizeof(file)) == HSD_DAT_OK &&
                    HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 16) == HSD_DAT_OK,
                "Cannot open joint animation fixture");
        // A later joint can bind a controller already decoded in this arena.
        require(HSD_DatImportGraph(&import, 20, DAT_AOBJ) != NULL,
                "Could not decode scalar controller");
        void* root = HSD_DatImportGraph(&import, 0, DAT_ANIMJOINT);
        require(i == 0 ? root != NULL :
                    (root == NULL && import.status == HSD_DAT_DESCRIPTOR),
                "Joint track must supply the value its channel consumes");
    }
}

void test_dat_animation(void)
{
    test_scalar_joint_binding();
    static const u32 slots[] = { 8, 16, 20, 36, 52, 56, 84, 88, 92, 96 };
    static const u8 stream[] = { 1, 0, 0, 0, 0x40, 1 };
    u8 file[264] = { 0 };
    u8 original[sizeof(file)];
    u8 arena[8192];
    u8* data = file + 32;
    HSD_Dat dat;
    HSD_DatImport import;
    put32(file, sizeof(file)); put32(file + 4, 192); put32(file + 8, 10);
    for (int i = 0; i < 10; ++i) put32(file + 224 + i * 4, slots[i]);
    put32(data + 8, 12); put32(data + 16, 28); put32(data + 20, 44);
    put32(data + 32, 0x41200000); put32(data + 36, 68);
    put32(data + 52, 28); put32(data + 56, 88); put32(data + 64, 2 << 16);
    put32(data + 72, sizeof(stream)); data[80] = 12;
    put32(data + 84, 180); put32(data + 88, 96); put32(data + 92, 96);
    put32(data + 96, 128); put32(data + 100, (4 << 16) | 4);
    memset(data + 128, 0xAD, 32);
    memcpy(data + 180, stream, sizeof(stream));
    memcpy(original, file, sizeof(file));
    require(HSD_DatOpen(&dat, file, sizeof(file)) == HSD_DAT_OK &&
                HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 32) == HSD_DAT_OK,
            "Animation fixture rejected");
    HSD_MatAnimJoint* root = HSD_DatImportGraph(&import, 0, DAT_MATANIM_JOINT);
    require(root && root->matanim && root->matanim->texanim,
            "Material animation graph failed to import");
    HSD_MatAnim* material = root->matanim;
    HSD_TexAnim* texture = material->texanim;
    require(texture->needs_tev && !texture->selects_indexed_images,
            "An animated texture color needs a TEV color register");
    require(material->aobjdesc == texture->aobjdesc &&
                material->aobjdesc->end_frame == 10 && texture->n_imagetbl == 2 &&
                texture->n_tluttbl == 0 && texture->tluttbl == NULL,
            "Animation sharing, frame count or texture-table counts changed");
    require(texture->imagetbl[0] == texture->imagetbl[1] &&
                texture->imagetbl[0]->image_ptr == data + 128,
            "Animation image table did not preserve shared descriptors");
    HSD_FObjDesc* track = material->aobjdesc->fobjdesc;
    require(track && track->type == 12 && track->length == sizeof(stream) &&
                track->ad == data + 180 && memcmp(track->ad, stream, sizeof(stream)) == 0,
            "Keyframe stream was swapped, copied incorrectly, or disconnected");
    require(memcmp(file, original, sizeof(file)) == 0, "Animation source data changed");
    data[80] = HSD_A_T_TIMG;
    // This track selects one of two images. Index 1 is the last valid image.
    data[183] = 0x80; data[184] = 0x3f;
    put32(data + 104, GX_TF_C4);
    require(HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 32) == HSD_DAT_OK &&
                HSD_DatImportGraph(&import, 0, DAT_MATANIM_JOINT) != NULL,
            "The final image in a texture animation table was rejected");
    data[183] = 0; data[184] = 0x40;
    require(HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 32) == HSD_DAT_OK,
            "Could not initialize endpoint fixture");
    root = HSD_DatImportGraph(&import, 0, DAT_MATANIM_JOINT);
    require(root && !root->matanim->texanim->needs_tev &&
                root->matanim->texanim->selects_indexed_images &&
                root->matanim->texanim->n_imagetbl == 3 &&
                root->matanim->texanim->imagetbl[2] == root->matanim->texanim->imagetbl[1],
            "Stock animation endpoint did not hold the final image");
    data[183] = 0x40; // 3.0, beyond the single endpoint
    require(HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 32) == HSD_DAT_OK &&
                HSD_DatImportGraph(&import, 0, DAT_MATANIM_JOINT) == NULL &&
                import.status == HSD_DAT_DESCRIPTOR,
            "An animation selecting an absent image was accepted");
    data[80] = 12;
    put32(data + 72, 100);
    require(HSD_DatOpen(&dat, file, sizeof(file)) == HSD_DAT_OK &&
                HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 32) == HSD_DAT_OK &&
                HSD_DatImportGraph(&import, 0, DAT_MATANIM_JOINT) == NULL &&
                import.status == HSD_DAT_DESCRIPTOR,
            "Truncated keyframe data returned a partial graph");
    puts("Native material animation graphs and shared texture tables passed");
}
