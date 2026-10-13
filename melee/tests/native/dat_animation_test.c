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

void test_dat_animation(void)
{
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
    put32(data + 72, 100);
    require(HSD_DatOpen(&dat, file, sizeof(file)) == HSD_DAT_OK &&
                HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 32) == HSD_DAT_OK &&
                HSD_DatImportGraph(&import, 0, DAT_MATANIM_JOINT) == NULL &&
                import.status == HSD_DAT_DESCRIPTOR,
            "Truncated keyframe data returned a partial graph");
    puts("Native material animation graphs and shared texture tables passed");
}
