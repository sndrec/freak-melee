#include "native_match_playback.h"
#ifdef MELEE_NATIVE
void nativeMatchPlayback_Bind(NativeMatchPlayback* p,const NativeMatchStream* s)
{*p=(NativeMatchPlayback){.stream=s,.failed=!s || !s->records || s->unavailable};}
NativeMatchPlaybackStep nativeMatchPlayback_Next(NativeMatchPlayback* p,NativeMatchStreamSample* sample)
{
    if(!p || !sample || p->failed || !p->stream || p->stream->unavailable)return NativeMatchPlayback_Failed;
    if(p->finished)return NativeMatchPlayback_Finished;
    if(p->index==p->stream->count)return p->stream->finished ? NativeMatchPlayback_Failed : NativeMatchPlayback_Waiting;
    if(!nativeMatchStream_Read(p->stream,p->index,sample) || sample->phase!=p->phase || sample->frame!=p->frame) {
        p->failed=true;return NativeMatchPlayback_Failed;
    }
    return sample->kind==NativeMatchStream_Frame ? NativeMatchPlayback_Frame : NativeMatchPlayback_PhaseEnd;
}
bool nativeMatchPlayback_CommitFrame(NativeMatchPlayback* p,uint64_t digest)
{
    if (!p) return false;
    NativeMatchStreamSample sample;
    if(nativeMatchPlayback_Next(p,&sample)!=NativeMatchPlayback_Frame || sample.state_digest!=digest) {
        p->failed=true;return false;
    }
    ++p->index;++p->frame;return true;
}
bool nativeMatchPlayback_EndPhase(NativeMatchPlayback* p,uint64_t digest,bool continues)
{
    if (!p) return false;
    NativeMatchStreamSample sample;
    if(nativeMatchPlayback_Next(p,&sample)!=NativeMatchPlayback_PhaseEnd ||
       sample.state_digest!=digest || sample.continues!=continues) {
        p->failed=true;return false;
    }
    ++p->index;
    if(continues){++p->phase;p->frame=0;}else p->finished=true;
    return true;
}
#endif
