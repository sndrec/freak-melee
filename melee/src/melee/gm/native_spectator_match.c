#include "native_spectator_match.h"
#ifdef MELEE_NATIVE
#include "native_match_playback.h"
#include "native_match_launch.h"
#include "native_online_session.h"
#include "native_replay.h"
#include "gm_1A3F.h"
#include "gm_1A45.h"
#include "gmvsmode.h"
#include "gmmain.h"
#include <melee/db/dbnativecontrol.h>
#include <melee/ui/native_hub.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/rumble.h>
#include <sysdolphin/baselib/rumble_output.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/random.h>
#include <sysdolphin/baselib/audio_output.h>
#include <aurora/aurora.h>
#include <aurora/event.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

static struct {
    NativeMatchPlayback playback;
    NativeMatchStreamSample sample;
    NativeSpectatorState state;
    uint64_t waiting_ms, display_ms, verified, hidden, last_digest;
    bool frame_active, sampled, catching_up, hide_frame, stop, last_b, quiet;
} spectator;

static uint64_t digest(uint32_t frame)
{return (nativeReplay_StateDigest(frame)^*seed_ptr)*UINT64_C(1099511628211);}
static void quiet(bool value)
{
    if(spectator.quiet!=value) {
        spectator.quiet=value;HSD_AudioOutputSetMuted(value || nativeMatchLaunch_Muted());
    }
}
static void fail(const char* reason)
{
    OSReport("[watch-playback] FAILED: %s phase=%u frame=%u index=%u.\n",reason,
             spectator.playback.phase,spectator.playback.frame,spectator.playback.index);
    nativeHub_Notice(reason);spectator.state=NativeSpectator_Failed;quiet(false);
}
static void local_buttons(const PADStatus pads[4])
{
    bool b=(pads[0].button&PAD_BUTTON_B)!=0;
    if(b && !spectator.last_b)spectator.stop=true;
    spectator.last_b=b;
}
static void inputs(PADStatus pads[4])
{
    local_buttons(pads);
    if(!spectator.frame_active) {fail("Viewing input sampled outside its frame.");return;}
    memcpy(pads,spectator.sample.pads,sizeof(spectator.sample.pads));
    spectator.sampled=true;
}
bool nativeSpectatorMatch_Request(void)
{
    if(spectator.state>=NativeSpectator_Requested && spectator.state<=NativeSpectator_Results)
        return false;
    if(nativeReplay_IsPlayback() || !nativeOnlineSession_WatchMatch())return false;
    spectator=(typeof(spectator)){.state=NativeSpectator_Requested};return true;
}
void nativeSpectatorMatch_Stop(void)
{
    spectator.stop=true;
    if(spectator.state==NativeSpectator_Requested)nativeSpectatorMatch_Finish();
}
bool nativeSpectatorMatch_QueueScene(void)
{
    if(spectator.state!=NativeSpectator_Requested)return false;
    const NativeRoomWatchReceive* watch=nativeOnlineSession_Watch();
    if(!watch || watch->state==NativeRoomWatch_Failed) {
        fail("Could not receive this match.");nativeSpectatorMatch_Finish();return false;
    }
    if(!nativeOnlineSession_WatchContentReady() || !watch->stream.count)return false;
    spectator.state=NativeSpectator_Queued;return true;
}
void nativeSpectatorMatch_LoadScene(void* unused)
{
    (void)unused;
    const NativeRoomWatchReceive* watch=nativeOnlineSession_ClaimWatch();
    if(spectator.state!=NativeSpectator_Queued || !watch ||
       !nativeMatchLaunch_AdoptSpectatorMatch(&watch->stream.description.match)) {
        fail("The watched fighter assets could not load.");nativeSpectatorMatch_Finish();
        gm_SetNextGameModeStateId(gmVsMode_State_Css);
    } else {
        nativeMatchPlayback_Bind(&spectator.playback,&watch->stream);
        spectator.state=NativeSpectator_Armed;
        gm_SetNextGameModeStateId(gmVsMode_State_Vs);
    }
}
void nativeSpectatorMatch_Prepare(bool sudden_death)
{
    if(sudden_death)return;
    if(spectator.state==NativeSpectator_Armed) {
        *seed_ptr=spectator.playback.stream->description.initial_seed;
        OSReport("[watch-playback] Seed %08x session=%016llx.\n",(unsigned)*seed_ptr,
                 (unsigned long long)spectator.playback.stream->description.match.session_id);
    }
}
bool nativeSpectatorMatch_Init(void)
{
    if(spectator.state!=NativeSpectator_Armed && spectator.state!=NativeSpectator_Playing)return false;
    spectator.state=NativeSpectator_Playing;spectator.frame_active=false;spectator.waiting_ms=SDL_GetTicks();
    const s8 silent[4]={-1,-1,-1,-1};HSD_RumbleOutputSetPortMapping(silent);
    HSD_PadSetRawFilter(inputs);nativeReplay_SetStateTraceEnabled(true);
    return true;
}
int nativeSpectatorMatch_PollFrame(void)
{
    if(spectator.state!=NativeSpectator_Playing)return -1;
    nativeOnlineSession_Pump();
    const NativeRoomWatchReceive* watch=nativeOnlineSession_Watch();
    if(!watch || watch->state==NativeRoomWatch_Failed) {fail("The viewing connection ended.");return -1;}
    if(spectator.stop)return -1;
    if(spectator.frame_active)return 1;
    NativeMatchPlaybackStep step=nativeMatchPlayback_Next(&spectator.playback,&spectator.sample);
    if(step==NativeMatchPlayback_Waiting) {
        PADStatus pads[4];PADRead(pads);local_buttons(pads);
        if(SDL_GetTicks()-spectator.waiting_ms>30000) {fail("The match input stream timed out.");return -1;}
        return spectator.stop ? -1 : 0;
    }
    if(step!=NativeMatchPlayback_Frame) {fail("The watched match ended at a different frame.");return -1;}
    spectator.waiting_ms=SDL_GetTicks();spectator.frame_active=true;spectator.sampled=false;
    spectator.catching_up=spectator.playback.stream->count-spectator.playback.index>4;
    spectator.hide_frame=spectator.catching_up && SDL_GetTicks()-spectator.display_ms<100;
    if(!spectator.hide_frame)spectator.display_ms=SDL_GetTicks();
    quiet(spectator.catching_up);
    return 1;
}
bool nativeSpectatorMatch_EndFrame(void)
{
    if(spectator.state!=NativeSpectator_Playing || !spectator.frame_active || !spectator.sampled) {
        fail("The watched input frame was not sampled.");return false;
    }
    spectator.frame_active=false;
    uint64_t actual=digest(spectator.playback.frame);spectator.last_digest=actual;
    if(!nativeMatchPlayback_CommitFrame(&spectator.playback,actual)) {
        OSReport("[watch-playback] expected=%016llx actual=%016llx rng=%08x\n",
            (unsigned long long)spectator.sample.state_digest,(unsigned long long)actual,(unsigned)*seed_ptr);
        nativeReplay_LogStateDigestSegments(spectator.playback.frame);
        fail("The watched game state diverged.");return false;
    }
    ++spectator.verified;if(spectator.hide_frame)++spectator.hidden;
    return !spectator.stop;
}
bool nativeSpectatorMatch_EndPhase(bool continues)
{
    HSD_PadSetRawFilter(NULL);nativeReplay_SetStateTraceEnabled(false);quiet(false);
    if(spectator.stop || spectator.state==NativeSpectator_Failed || gmMain_ExitRequested) {
        if(spectator.state!=NativeSpectator_Failed)spectator.state=NativeSpectator_Stopped;
        return false;
    }
    uint64_t began=SDL_GetTicks();NativeMatchStreamSample sample;
    while(nativeMatchPlayback_Next(&spectator.playback,&sample)==NativeMatchPlayback_Waiting &&
          SDL_GetTicks()-began<30000) {
        nativeOnlineSession_Pump();dbNativeControl_Poll();
        const AuroraEvent* event=aurora_update();
        for(;event && event->type!=AURORA_NONE;++event)
            if(event->type==AURORA_EXIT)gmMain_ExitRequested=true;
        PADStatus pads[4];PADRead(pads);local_buttons(pads);
        if(gmMain_ExitRequested || spectator.stop)break;
        SDL_Delay(1);
    }
    if(gmMain_ExitRequested || spectator.stop) {
        spectator.state=NativeSpectator_Stopped;return false;
    }
    uint64_t actual=digest(spectator.playback.frame);spectator.last_digest=actual;
    if(!nativeMatchPlayback_EndPhase(&spectator.playback,actual,continues)) {
        fail("The watched match result diverged.");return false;
    }
    OSReport("[watch-playback] Phase %u complete, verified=%llu hidden=%llu digest=%016llx rng=%08x.\n",
        sample.phase,(unsigned long long)spectator.verified,(unsigned long long)spectator.hidden,
        (unsigned long long)actual,(unsigned)*seed_ptr);
    if(!continues)spectator.state=NativeSpectator_Results;
    return true;
}
void nativeSpectatorMatch_Finish(void)
{
    HSD_PadSetRawFilter(NULL);nativeReplay_SetStateTraceEnabled(false);quiet(false);
    nativeOnlineSession_ReleaseWatch();
    spectator.frame_active=false;spectator.playback.stream=NULL;
    if(spectator.state!=NativeSpectator_Failed)spectator.state=NativeSpectator_Stopped;
}
NativeSpectatorState nativeSpectatorMatch_State(void){return spectator.state;}
bool nativeSpectatorMatch_HideFrame(void){return spectator.hide_frame;}
bool nativeSpectatorMatch_HasFailed(void){return spectator.state==NativeSpectator_Failed || spectator.stop;}
void nativeSpectatorMatch_Inspect(void)
{
    fprintf(stderr,"[watch-playback] state=%u phase=%u frame=%u index=%u verified=%llu hidden=%llu digest=%016llx failed=%u\n",
        spectator.state,spectator.playback.phase,spectator.playback.frame,spectator.playback.index,
        (unsigned long long)spectator.verified,(unsigned long long)spectator.hidden,
        (unsigned long long)spectator.last_digest,nativeSpectatorMatch_HasFailed());
}
#endif
