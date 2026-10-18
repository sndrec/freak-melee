#include <melee/gm/native_match_playback.h>
#include <assert.h>
#include <stdio.h>

static uint8_t records[8*NATIVE_MATCH_STREAM_RECORD_BYTES];
int main(void)
{
    NativeMatchStreamDescription d={.initial_seed=123,.match={
        .session_id=123,.stage_id=32,.peer_count=2,
        .mode=NativeLobbyMatchMode_Stock,.stock_count=1,.damage_ratio=10,
        .item_frequency=-1,.content_digest={1},
        .peers={{.peer_id="peer_one",.port_mask=13},
                {.peer_id="peer_two",.numeric_peer=1,.port_mask=2}},
        .slots={{.kind=NativeLobbyMatchSlot_Human,.character_id=1,.handicap=9},
                {.kind=NativeLobbyMatchSlot_Human,.character_id=2,.handicap=9}}}};
    NativeMatchStream stream;NativeMatchPlayback p;NativeMatchStreamSample sample;
    PADStatus pads[4]={{.stickX=-80,.button=PAD_BUTTON_A}};
    assert(nativeMatchStream_Bind(&stream,&d,records,8));
    nativeMatchPlayback_Bind(&p,&stream);
    assert(nativeMatchPlayback_Next(&p,&sample)==NativeMatchPlayback_Waiting);
    assert(nativeMatchStream_AppendFrame(&stream,0,pads,10));
    assert(nativeMatchPlayback_Next(&p,&sample)==NativeMatchPlayback_Frame);
    assert(sample.pads[0].stickX==-80 && sample.pads[0].button==PAD_BUTTON_A);
    assert(p.index==0 && nativeMatchPlayback_Next(&p,&sample)==NativeMatchPlayback_Frame);
    assert(nativeMatchPlayback_CommitFrame(&p,10));
    assert(nativeMatchPlayback_Next(&p,&sample)==NativeMatchPlayback_Waiting);
    assert(nativeMatchStream_EndPhase(&stream,true,20));
    assert(nativeMatchPlayback_EndPhase(&p,20,true));
    assert(p.phase==1 && p.frame==0 && p.index==2);
    assert(nativeMatchStream_AppendFrame(&stream,0,pads,30));
    assert(nativeMatchStream_EndPhase(&stream,false,40));
    NativeMatchPlayback bad=p;
    assert(!nativeMatchPlayback_CommitFrame(&bad,31) && bad.failed && bad.index==2);
    assert(nativeMatchPlayback_CommitFrame(&p,30));
    bad=p;assert(!nativeMatchPlayback_EndPhase(&bad,40,true) && bad.failed);
    assert(nativeMatchPlayback_EndPhase(&p,40,false));
    assert(nativeMatchPlayback_Next(&p,&sample)==NativeMatchPlayback_Finished);
    nativeMatchPlayback_Bind(&p,&stream);p.index=stream.count;
    assert(nativeMatchPlayback_Next(&p,&sample)==NativeMatchPlayback_Failed);
    nativeMatchPlayback_Bind(&p,NULL);
    assert(nativeMatchPlayback_Next(&p,&sample)==NativeMatchPlayback_Failed);
    assert(!nativeMatchPlayback_CommitFrame(NULL,0));
    puts("match playback: live wait, exact input, sudden death and divergence passed");
    return 0;
}
