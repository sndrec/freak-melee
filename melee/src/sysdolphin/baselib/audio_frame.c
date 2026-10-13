#include "audio_frame.h"
#include "audio_aux.h"

#include <string.h>

// The SDK rotates independent DSP write/read and CPU effect-processing slots.
static s32 auxiliary[3][2][3][HSD_AUDIO_FRAME_SAMPLES];
static u32 write_slot;

void HSD_AudioFrameReset(void)
{
    memset(auxiliary, 0, sizeof(auxiliary));
    write_slot = 0;
}

void HSD_AudioRenderFrame(const u8* storage, size_t size,
                          s16 output[HSD_AUDIO_FRAME_SAMPLES * 2])
{
    u32 read_slot = (write_slot + 1) % 3;
    u32 process_slot = (write_slot + 2) % 3;
    bool active[2] = { HSD_AudioAuxIsActive(0), HSD_AudioAuxIsActive(1) };
    for (u32 block = 0; block < 5; ++block) {
        HSD_AudioBuses buses;
        HSD_AudioMixVoices(storage, size, &buses);
        for (u32 i = 0; i < HSD_AUDIO_MS_SAMPLES; ++i) {
            u32 sample = block * HSD_AUDIO_MS_SAMPLES + i;
            for (u32 bus = 0; bus < 2; ++bus) {
                for (u32 channel = 0; channel < 3; ++channel) {
                    auxiliary[write_slot][bus][channel][sample] =
                        buses.samples[3 + bus * 3 + channel][i];
                }
            }
            for (u32 channel = 0; channel < 2; ++channel) {
                u32 mixed = (u32) buses.samples[channel][i];
                for (u32 bus = 0; bus < 2; ++bus) {
                    if (active[bus]) {
                        mixed += (u32) auxiliary[read_slot][bus][channel][sample];
                    }
                }
                s32 value = (s32) mixed;
                if (value > 32767) value = 32767;
                if (value < -32768) value = -32768;
                // SDL expects native-endian LR; DSP DMA used big-endian RL.
                output[sample * 2 + channel] = value;
            }
        }
    }
    for (u32 bus = 0; bus < 2; ++bus) {
        HSD_AudioAuxBuffers buffers = {
            auxiliary[process_slot][bus][0], auxiliary[process_slot][bus][1],
            auxiliary[process_slot][bus][2]
        };
        HSD_AudioAuxProcess(bus, &buffers);
    }
    write_slot = (write_slot + 1) % 3;
}
