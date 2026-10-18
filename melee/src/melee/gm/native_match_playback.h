#pragma once
#include "native_match_stream.h"

typedef enum NativeMatchPlaybackStep {
    NativeMatchPlayback_Waiting,
    NativeMatchPlayback_Frame,
    NativeMatchPlayback_PhaseEnd,
    NativeMatchPlayback_Finished,
    NativeMatchPlayback_Failed,
} NativeMatchPlaybackStep;
typedef struct NativeMatchPlayback {
    const NativeMatchStream* stream;
    uint32_t index, frame;
    uint16_t phase;
    bool finished, failed;
} NativeMatchPlayback;
void nativeMatchPlayback_Bind(NativeMatchPlayback*,const NativeMatchStream*);
NativeMatchPlaybackStep nativeMatchPlayback_Next(NativeMatchPlayback*,NativeMatchStreamSample*);
bool nativeMatchPlayback_CommitFrame(NativeMatchPlayback*,uint64_t digest);
bool nativeMatchPlayback_EndPhase(NativeMatchPlayback*,uint64_t digest,bool continues);
