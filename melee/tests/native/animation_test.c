#include <sysdolphin/baselib/dat_import.h>
#include <sysdolphin/baselib/memory.h>
#include <sysdolphin/baselib/native_animation_pool.h>

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

static void observe(void* output, enum_t type, HSD_ObjData* value)
{
    require(type == 7, "Imported animation channel changed");
    *(f32*) output = value->fv;
}

static void run_track(const u8* stream, size_t length, u8 fraction, f32 first_value)
{
    u8 file[104] = { 0 };
    u8 arena[2048];
    u8* data = file + 32;
    HSD_Dat dat;
    HSD_DatImport import;
    put32(file, sizeof(file)); put32(file + 4, 64); put32(file + 8, 2);
    put32(file + 96, 8); put32(file + 100, 32);
    put32(data + 4, 0x40800000); put32(data + 8, 16);
    put32(data + 20, (u32) length); data[28] = 7; data[29] = fraction;
    put32(data + 32, 36); memcpy(data + 36, stream, length);
    require(HSD_DatOpen(&dat, file, sizeof(file)) == HSD_DAT_OK &&
                HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 4) == HSD_DAT_OK,
            "Cannot import animation playback fixture");
    HSD_AObjDesc* animation = HSD_DatImportGraph(&import, 0, DAT_AOBJ);
    require(animation != NULL, "Native animation descriptor import failed");
    HSD_FObj* track = HSD_FObjLoadDesc(animation->fobjdesc);
    require(track != NULL, "Original interpreter could not load converted track");
    long available = OSCheckHeap(HSD_GetHeap());
    for (int repeat = 0; repeat < 2; ++repeat) {
        HSD_FObjReqAnimAll(track, 0);
        for (int frame = 0; frame <= 4; ++frame) {
            f32 value = -999;
            HSD_FObjInterpretAnim(track, &value, observe, frame ? 1 : 0);
            f32 expected = first_value + (6 - first_value) * frame / 4;
            require(value == expected, "Converted linear track produced an incorrect frame value");
        }
    }
    require(OSCheckHeap(HSD_GetHeap()) == available,
            "Animation playback allocated additional heap memory");
    HSD_FObjRemoveAll(track);
    require(nativeFObjPool.used == 0, "Animation track was not returned to its pool");
}

void test_native_animation(void)
{
    // Two linear keys with a four-frame segment. Stream scalars are little-endian.
    static const u8 floats[] = { 0x12, 0, 0, 0, 0x40, 4, 0, 0, 0xC0, 0x40 };
    static const u8 signed_values[] = { 0x12, 0xFE, 0xFF, 4, 6, 0 };
    static const u8 negative_floats[] = { 0x12, 0, 0, 0, 0xC0, 4, 0, 0, 0xC0, 0x40 };
    require(nativeAnimationPool_BeginHeap(), "Cannot initialize native animation ownership");
    HSD_FObj waiting = { .op_intrp = 0, .obj_type = 7 };
    f32 value = 123.0F;
    FObjUpdateAnim(&waiting, &value, observe);
    require(value == 123.0F, "Animation without an interpolation opcode updated the pose");
    run_track(floats, sizeof(floats), HSD_A_FRAC_FLOAT, 2);
    run_track(signed_values, sizeof(signed_values), HSD_A_FRAC_S16, -2);
    run_track(negative_floats, sizeof(negative_floats), HSD_A_FRAC_FLOAT, -2);
    puts("Converted DAT tracks passed original FObj interpolation and restart");
}
