#include <sysdolphin/baselib/audio_sample.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void check(int ok) { if (!ok) { fputs("ADPCM state mismatch\n", stderr); exit(1); } }
int main(void)
{
    HSD_SfxVoice v = {0};
    v.address.currentAddressLo = 2; v.address.endAddressLo = 31;
    v.adpcm.pred_scale = 0;
    u8 data[16] = {0, 0x17, 0x8F, 0x23, 0x45, 0x67, 0x89, 0xAB,
                   1, 0x17, 0x8F, 0x23, 0x45, 0x67, 0x89, 0xAB};
    HSD_AudioSample a, b;
    s16 whole[28], split[28];
    check(HSD_AudioSampleStart(&a, &v, false)); b = a;
    check(HSD_AudioSampleRead(&a, data, 16, whole, 28) == 28 && !a.active);
    check(whole[0] == 1 && whole[1] == 7 && whole[2] == -8 && whole[3] == -1);
    check(whole[14] == 2 && whole[15] == 14 && whole[16] == -16);
    for (int i = 0; i < 28; ++i) check(HSD_AudioSampleRead(&b, data, 16, split + i, 1) == 1);
    check(!memcmp(whole, split, sizeof(whole)) && !memcmp(&a, &b, sizeof(a)));
    check(HSD_AudioSampleRead(&a, data, 16, whole, 28) == 0);
    v.address.endAddressLo = 2; v.address.loopAddressLo = 2; v.address.loopFlag = 1;
    v.adpcm.a[0][0] = 2048; v.adpcm.yn1 = 100; v.loop.loop_yn1 = 200;
    check(HSD_AudioSampleStart(&a, &v, false));
    check(HSD_AudioSampleRead(&a, data, 16, whole, 3) == 3);
    check(whole[0] == 101 && whole[1] == 201 && whole[2] == 201);
    check(HSD_AudioSampleStart(&a, &v, true));
    check(HSD_AudioSampleRead(&a, data, 16, whole, 3) == 3);
    check(whole[0] == 101 && whole[1] == 102 && whole[2] == 103);
    v.adpcm.yn1 = 32767;
    check(HSD_AudioSampleStart(&a, &v, true));
    check(HSD_AudioSampleRead(&a, data, 16, whole, 1) == 1 && whole[0] == 32767);
    v.adpcm.yn1 = (u16)-32768; data[1] = 0xF0;
    check(HSD_AudioSampleStart(&a, &v, true));
    check(HSD_AudioSampleRead(&a, data, 16, whole, 1) == 1 && whole[0] == -32768);
    check(HSD_AudioSampleStart(&a, &v, false));
    check(HSD_AudioSampleRead(&a, data, 1, whole, 1) == 0 && !a.active);
    v.address.loopFlag = 0; v.address.endAddressLo = 15;
    v.adpcm.a[0][0] = 0; v.adpcm.yn1 = 0;
    check(HSD_AudioSampleStart(&a, &v, false));
    check(HSD_AudioSampleRead(&a, data, 8, whole, 28) == 14 && !a.active);
    v.address.endAddressLo = 2; v.adpcm.a[0][0] = 1024;
    v.adpcm.yn1 = 1; data[1] = 0;
    check(HSD_AudioSampleStart(&a, &v, false));
    check(HSD_AudioSampleRead(&a, data, 8, whole, 1) == 1 && whole[0] == 1);
    v.adpcm.yn1 = (u16)-1;
    check(HSD_AudioSampleStart(&a, &v, false));
    check(HSD_AudioSampleRead(&a, data, 8, whole, 1) == 1 && whole[0] == 0);
    v.address.currentAddressLo = 0; check(!HSD_AudioSampleStart(&a, &v, false));
    puts("ADPCM signed samples, frame headers, split reads, loops and saturation verified");
    return 0;
}
