#include <sysdolphin/baselib/audio_mix.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void check(int ok) { if (!ok) { fputs("audio mixer mismatch\n", stderr); exit(1); } }
void __assert(char* file, u32 line, char* expression)
{ fprintf(stderr, "%s:%lu: %s\n", file, line, expression); abort(); }
static s16 clamp(s32 value) { return value > 32767 ? 32767 : value < -32768 ? -32768 : value; }
static HSD_AudioVoice* start(void)
{
    HSD_AudioVoice* v = HSD_AudioVoiceAcquire(1, NULL); check(v != NULL);
    v->sample.address.currentAddressLo = 2; v->sample.address.endAddressLo = 15;
    v->sample.address.loopAddressLo = 2; v->sample.address.loopFlag = 1;
    v->sample.adpcm.pred_scale = 12; v->sample.loop.loop_pred_scale = 12;
    v->source.ratioHi = 1;
    check(HSD_AudioVoiceStart(v)); return v;
}
int main(void)
{
    const u8 data[8] = {12, 0x78, 0xF2, 0x34, 0x56, 0x78, 0xAB, 0xCD};
    const u32 pair[9] = {0,1,7,2,3,8,4,5,6};
    HSD_AudioVoiceInit();
    HSD_AudioVoice* v = start();
    u16 words[18];
    for (u32 i = 0; i < 9; ++i) { words[i*2] = 0x8000 + i*100; words[i*2+1] = (u16)(-i-1); }
    AXPBMIX mix; memcpy(&mix, words, sizeof(mix));
    HSD_AudioVoiceConfigureMix(v, &mix);
    check(v->mix_mask == 0x1FF);
    v->envelope.currentVolume = 0x8001; v->envelope.currentDelta = -3;
    HSD_AudioVoice reference = *v;
    s16 samples[32]; HSD_AudioVoiceResample(&reference, data, sizeof(data), samples, 32);
    HSD_AudioBuses buses;
    HSD_AudioMixVoices(data, sizeof(data), &buses);
    for (u32 bus = 0; bus < 9; ++bus) {
        u16 gain = words[pair[bus]*2], delta = words[pair[bus]*2+1], envelope = 0x8001;
        for (u32 i = 0; i < 32; ++i) {
            s16 scaled = clamp(((s32)samples[i] * (s16)envelope) >> 15);
            s16 expected = clamp(((s32)scaled * gain) >> 15);
            check(buses.samples[bus][i] == expected);
            gain = (u16)(gain + delta); envelope = (u16)(envelope - 3);
        }
        check(v->last_mix[bus] == buses.samples[bus][31]);
        words[pair[bus]*2] = gain;
    }
    check(v->envelope.currentVolume == (u16)(0x8001 - 96));
    check(!memcmp(&v->mix, words, sizeof(words)));
    HSD_AudioVoiceRelease(v);
    // Disabled aux buses do not become enabled merely because their delta is nonzero.
    v = start(); memset(&mix, 0, sizeof(mix)); mix.vDeltaAuxAL = 100;
    HSD_AudioVoiceConfigureMix(v, &mix); check(v->mix_mask == 3);
    HSD_AudioMixVoices(data, sizeof(data), &buses);
    check(v->mix.vAuxAL == 0);
    HSD_AudioVoiceRelease(v);
    // Accumulation stays wide across all 64 voices; output clipping happens later.
    HSD_AudioVoice* all[64];
    memset(&mix, 0, sizeof(mix)); mix.vL = mix.vR = 0xFFFF;
    for (u32 i = 0; i < 64; ++i) {
        all[i] = start(); all[i]->envelope.currentVolume = 32767;
        HSD_AudioVoiceConfigureMix(all[i], &mix);
    }
    reference = *all[0]; HSD_AudioVoiceResample(&reference, data, sizeof(data), samples, 32);
    HSD_AudioMixVoices(data, sizeof(data), &buses);
    for (u32 i = 0; i < 32; ++i) {
        s16 scaled = clamp(((s32)samples[i] * 32767) >> 15);
        s32 expected = (s32)clamp(((s32)scaled * 65535) >> 15) * 64;
        check(buses.samples[0][i] == expected && buses.samples[1][i] == expected);
    }
    for (u32 i = 0; i < 64; ++i) HSD_AudioVoiceRelease(all[i]);
    HSD_AudioVoiceInit(); // A new session has no pending release tails.
    HSD_AudioMixVoices(data, sizeof(data), &buses);
    for (u32 bus = 0; bus < 9; ++bus) for (u32 i = 0; i < 32; ++i) check(buses.samples[bus][i] == 0);
    HSD_AudioVoiceInit();
    v = HSD_AudioVoiceAcquire(1, NULL);
    const s16 tails[9] = {1600,-1600,159,-159,32000,-32000,160,-160,0};
    for (u32 bus = 0; bus < 9; ++bus) v->last_mix[bus] = tails[bus];
    v->running = true; HSD_AudioVoiceRelease(v);
    s32 remaining[9];
    for (u32 bus = 0; bus < 9; ++bus) remaining[bus] = tails[bus];
    for (u32 frame = 0; frame < 11; ++frame) {
        s32 initial[9], slopes[9];
        for (u32 bus = 0; bus < 9; ++bus) {
            slopes[bus] = remaining[bus] / 160;
            if (slopes[bus] > 20) slopes[bus] = 20;
            if (slopes[bus] < -20) slopes[bus] = -20;
            initial[bus] = slopes[bus] ? remaining[bus] : 0;
            remaining[bus] = initial[bus] - slopes[bus] * 160;
        }
        for (u32 block = 0; block < 5; ++block) {
            HSD_AudioMixVoices(data, sizeof(data), &buses);
            for (u32 bus = 0; bus < 9; ++bus)
                for (u32 i = 0; i < 32; ++i)
                    check(buses.samples[bus][i] == initial[bus] - slopes[bus] * (block * 32 + i));
        }
    }
    HSD_AudioVoiceInit();
    for (u32 i = 0; i < 64; ++i) all[i] = HSD_AudioVoiceAcquire(1, NULL);
    all[0]->last_mix[0] = 1000; all[0]->running = true;
    v = HSD_AudioVoiceAcquire(2, NULL); check(v == all[0]);
    HSD_AudioVoiceRelease(v); // Releasing an unrendered replacement adds no tail.
    HSD_AudioMixVoices(data, sizeof(data), &buses);
    check(buses.samples[0][0] == 1000 && buses.samples[0][31] == 814);
    HSD_AudioVoiceInit();
    HSD_AudioMixVoices(data, sizeof(data), &buses); // First millisecond of a frame.
    v = HSD_AudioVoiceAcquire(1, NULL); v->running = true; v->last_mix[0] = 1600;
    HSD_AudioVoiceRelease(v);
    for (u32 block = 0; block < 4; ++block) {
        HSD_AudioMixVoices(data, sizeof(data), &buses);
        check(buses.samples[0][0] == 0 && buses.samples[0][31] == 0);
    }
    HSD_AudioMixVoices(data, sizeof(data), &buses);
    check(buses.samples[0][0] == 1600 && buses.samples[0][31] == 1290);
    puts("Nine-bus routing, signed envelope, ramps, saturation and 64-voice accumulation verified");
    // Independent local gains preserve voice playback and envelope advancement.
    const unsigned levels[3][2] = {{0,100},{100,0},{40,75}};
    for (unsigned music=0;music<2;++music) for (unsigned level=0;level<3;++level) {
        HSD_AudioVoiceInit(); v=start(); v->music=music!=0;
        v->envelope.currentVolume=32767;
        memset(&mix,0,sizeof(mix)); mix.vL=mix.vR=32768;
        HSD_AudioVoiceConfigureMix(v,&mix); reference=*v;
        HSD_AudioVoiceResample(&reference,data,sizeof(data),samples,32);
        HSD_AudioMixSetVolumes(levels[level][0],levels[level][1]);
        HSD_AudioMixVoices(data,sizeof(data),&buses);
        unsigned gain=levels[level][music]*32768/100;
        for (u32 i=0;i<32;++i) {
            s16 expected=clamp(((s32)samples[i]*32767)>>15);
            expected=(s16)(((s32)expected*gain)>>15);
            check(buses.samples[0][i]==expected && buses.samples[1][i]==expected);
        }
        check(v->envelope.currentVolume==32767);
        check(v->decoder.cursor==reference.decoder.cursor);
        check(!memcmp(v->decoder.history,reference.decoder.history,sizeof(v->decoder.history)));
    }
    HSD_AudioMixSetVolumes(100,100);
    return 0;
}
