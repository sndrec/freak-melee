#include <melee/lb/lbanim_dat.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void require(bool ok, const char* text)
{
    if (!ok) { fprintf(stderr, "%s\n", text); exit(1); }
}
static void put32(u8* p, u32 x)
{
    p[0] = x >> 24; p[1] = x >> 16; p[2] = x >> 8; p[3] = x;
}
void test_native_figatree(void)
{
    u8 bytes[108] = { 0 }, original[108];
    u8* data = bytes + 32;
    const u8 stream[] = { 0x12, 0, 0, 0, 0x40, 4, 0, 0, 0xC0, 0x40 };
    put32(bytes, sizeof(bytes)); put32(bytes + 4, 64); put32(bytes + 8, 3);
    put32(bytes + 96, 12); put32(bytes + 100, 16); put32(bytes + 104, 28);
    put32(data, 1); put32(data + 8, 0x40800000);
    put32(data + 12, 32); put32(data + 16, 20);
    data[21] = sizeof(stream); data[24] = 5; // Joint translation X.
    put32(data + 28, 40); data[32] = 1; data[33] = 255;
    memcpy(data + 40, stream, sizeof(stream));
    memcpy(original, bytes, sizeof(bytes));
    HSD_Dat dat;
    FigaTree tree;
    FigaTrack tracks[1];
    u32 nodes;
    require(HSD_DatOpen(&dat, bytes, sizeof(bytes)) == HSD_DAT_OK &&
        lbAnim_DecodeFigaTree(&dat, 0, &tree, tracks, 1, &nodes) == HSD_DAT_OK,
        "Cannot decode FigaTree fixture");
    require(nodes == 1 && tree.frames == 4 && tree.type == 1 &&
        tracks[0].length == sizeof(stream) && tracks[0].ad_head == data + 40 &&
        !memcmp(bytes, original, sizeof(bytes)), "FigaTree fields or stream bytes changed");
    HSD_Joint desc = { .scale = { 1, 1, 1 } };
    HSD_JObj* joint = HSD_JObjLoadJoint(&desc);
    lbAnim_8001E6D8(joint, &tree, tracks, tree.nodes[0]);
    long free_bytes = OSCheckHeap(HSD_GetHeap());
    for (int repeat = 0; repeat < 2; ++repeat) {
        HSD_JObjReqAnimAll(joint, 0);
        for (int frame = 0; frame <= 4; ++frame) {
            HSD_JObjAnimAll(joint);
            require(joint->translate.x == 2 + frame, "FigaTree playback changed translation samples");
        }
    }
    require(OSCheckHeap(HSD_GetHeap()) == free_bytes, "FigaTree playback allocated heap memory");
    HSD_JObjUnref(joint);
    require(lbAnim_DecodeFigaTree(&dat, 0, &tree, tracks, 0, &nodes) == HSD_DAT_CAPACITY,
            "FigaTree track capacity was not enforced");
    data[32] = 128;
    require(lbAnim_DecodeFigaTree(&dat, 0, &tree, tracks, 1, &nodes) == HSD_DAT_DESCRIPTOR,
            "Invalid negative node count was accepted");
    data[32] = 1; data[21] = 255;
    require(lbAnim_DecodeFigaTree(&dat, 0, &tree, tracks, 1, &nodes) == HSD_DAT_DESCRIPTOR,
            "Truncated animation stream was accepted");
    data[21] = sizeof(stream) - 1;
    require(lbAnim_DecodeFigaTree(&dat, 0, &tree, tracks, 1, &nodes) == HSD_DAT_DESCRIPTOR,
            "A scalar extending beyond its declared track length was accepted");
    data[21] = 6;
    data[45] = 0x80;
    require(lbAnim_DecodeFigaTree(&dat, 0, &tree, tracks, 1, &nodes) == HSD_DAT_DESCRIPTOR,
            "A frame interval ending inside a packed integer was accepted");
    puts("FigaTree import, original joint playback, restart and bounds checks passed");
}
