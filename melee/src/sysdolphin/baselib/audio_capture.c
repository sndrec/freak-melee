#include "audio_capture.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>

static FILE* recording;
static u32 recorded_bytes;

static void le32(u8* output,u32 value)
{
    for (unsigned i=0;i<4;++i) output[i]=(u8)(value>>(i*8));
}

bool HSD_AudioCaptureSetFile(const char* path)
{
    bool ok=true;
    if (recording) {
        u8 header[44]={0};
        memcpy(header,"RIFF",4); le32(header+4,recorded_bytes+36);
        memcpy(header+8,"WAVEfmt ",8); le32(header+16,16);
        header[20]=1; header[22]=2; le32(header+24,32000); le32(header+28,128000);
        header[32]=4; header[34]=16; memcpy(header+36,"data",4); le32(header+40,recorded_bytes);
        ok=fseek(recording,0,SEEK_SET)==0 && fwrite(header,1,44,recording)==44;
        ok=fclose(recording)==0 && ok; recording=NULL;
        OSReport("[audio-capture] Closed %u bytes; ok=%u\n",recorded_bytes,ok);
    }
    recorded_bytes=0;
    if (path && *path) {
        recording=fopen(path,"wb");
        u8 empty[44]={0};
        if (!recording || fwrite(empty,1,44,recording)!=44) {
            if (recording) fclose(recording);
            recording=NULL; return false;
        }
        OSReport("[audio-capture] Started %s\n",path);
    }
    return ok;
}

void HSD_AudioCaptureWrite(const s16* samples,unsigned frames)
{
    if (!recording) return;
    unsigned bytes=frames*4;
    if (bytes>0xFFFFFFD0u-recorded_bytes || fwrite(samples,1,bytes,recording)!=bytes) {
        OSReport("[audio-capture] Recording write failed or exceeded WAV size limit.\n");
        HSD_AudioCaptureSetFile(NULL); return;
    }
    recorded_bytes+=bytes;
}

void HSD_AudioCapturePackageCue(unsigned owner, unsigned clip)
{
    if (recording) {
        OSReport("[audio-capture] Fighter %u clip %u at sample %u\n",
                 owner, clip, recorded_bytes / 4);
    }
}
