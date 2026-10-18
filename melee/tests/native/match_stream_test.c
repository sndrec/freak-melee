#include <melee/gm/native_match_stream.h>
#include <melee/gm/native_pad_wire.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

enum { RECORDS=40 };
static uint8_t source_bytes[RECORDS*NATIVE_MATCH_STREAM_RECORD_BYTES];
static uint8_t receive_bytes[sizeof(source_bytes)],saved_bytes[sizeof(source_bytes)];

static NativeMatchStreamDescription description(void)
{
    return (NativeMatchStreamDescription){.initial_seed=0xdeadbeef,.match={
        .session_id=UINT64_C(0x123456789abcdef0),.stage_id=32,.peer_count=2,
        .mode=NativeLobbyMatchMode_Stock,.stock_count=1,.damage_ratio=10,.item_frequency=-1,
        .content_digest={1},
        .peers={{.peer_id="peer_one",.port_mask=0xD},
                {.peer_id="peer_two",.numeric_peer=1,.port_mask=0x2}},
        .slots={{.kind=NativeLobbyMatchSlot_Human,.character_id=1,.handicap=9},
                {.kind=NativeLobbyMatchSlot_Human,.character_id=2,.handicap=9}},
    }};
}
static void pads_for(PADStatus pads[4],unsigned frame)
{
    memset(pads,0xA5,sizeof(PADStatus)*4);
    for(unsigned p=0;p<4;++p) {
        pads[p].button=(uint16_t)(frame*13+p);
        pads[p].stickX=(int8_t)(frame*7-90+p);pads[p].stickY=(int8_t)(90-frame*9-p);
        pads[p].substickX=(int8_t)(p*10-50);pads[p].substickY=(int8_t)(50-p*7);
        pads[p].triggerLeft=(uint8_t)(frame*11+p);pads[p].triggerRight=(uint8_t)(frame*9+p);
        pads[p].analogA=(uint8_t)(frame*17+p);pads[p].analogB=(uint8_t)(frame*3+p);
        pads[p].err=p==3 ? PAD_ERR_NO_CONTROLLER : 0;pads[p].extButton=0x12345678+frame+p;
    }
}
int main(void)
{
    NativeMatchStreamDescription d=description(),decoded;
    uint8_t metadata[NATIVE_MATCH_STREAM_DESCRIPTION_BYTES];
    size_t size=nativeMatchStream_EncodeDescription(&d,metadata,sizeof(metadata));
    assert(size && nativeMatchStream_DecodeDescription(&decoded,metadata,size));
    assert(decoded.initial_seed==d.initial_seed && decoded.match.session_id==d.match.session_id);
    NativeMatchStreamDescription unchanged=decoded;
    metadata[0]^=1;
    assert(!nativeMatchStream_DecodeDescription(&decoded,metadata,size));
    assert(!memcmp(&decoded,&unchanged,sizeof(decoded)));metadata[0]^=1;
    assert(!nativeMatchStream_DecodeDescription(&decoded,metadata,size-1));
    NativeMatchStream sender,receiver;
    assert(nativeMatchStream_Bind(&sender,&d,source_bytes,RECORDS));
    assert(nativeMatchStream_Bind(&receiver,&d,receive_bytes,RECORDS));
    PADStatus pads[4];
    for(unsigned f=0;f<10;++f) {
        pads_for(pads,f);
        assert(nativeMatchStream_AppendFrame(&sender,f,pads,0x100000000ULL+f));
    }
    assert(nativeMatchStream_EndPhase(&sender,true,0x200000000ULL));
    for(unsigned f=0;f<7;++f) {
        pads_for(pads,f+10);
        assert(nativeMatchStream_AppendFrame(&sender,f,pads,0x300000000ULL+f));
    }
    assert(nativeMatchStream_EndPhase(&sender,false,0x400000000ULL));
    assert(sender.finished && sender.count==19 && sender.phase==1 && sender.next_frame==7);
    assert(!nativeMatchStream_AppendFrame(&sender,7,pads,0));
    assert(!nativeMatchStream_EndPhase(&sender,false,0));
    unsigned packets=0,corruptions=0;
    uint8_t packet[NATIVE_MATCH_STREAM_PACKET_BYTES],bad[sizeof(packet)];
    while(receiver.count<sender.count) {
        unsigned first=receiver.count>2 ? receiver.count-2 : 0;
        size=nativeMatchStream_EncodeRecords(&sender,first,packet,sizeof(packet));
        assert(size && nativeMatchStream_AcceptRecords(&receiver,packet,size));
        ++packets;
        NativeMatchStream saved=receiver;
        memcpy(saved_bytes,receive_bytes,sizeof(receive_bytes));
        assert(nativeMatchStream_AcceptRecords(&receiver,packet,size));
        assert(!memcmp(&saved,&receiver,sizeof(saved)));
        // Every changed byte contradicts an already accepted packet, including
        // its inputs, digest, phase markers, sequence and session identity.
        for(unsigned i=0;i<size;++i) {
            memcpy(bad,packet,size);bad[i]^=1;
            assert(!nativeMatchStream_AcceptRecords(&receiver,bad,size));
            assert(!memcmp(&saved,&receiver,sizeof(saved)));
            assert(!memcmp(saved_bytes,receive_bytes,sizeof(saved_bytes)));
            ++corruptions;
        }
    }
    assert(receiver.digest==sender.digest && receiver.finished);
    assert(!memcmp(source_bytes,receive_bytes,(size_t)sender.count*NATIVE_MATCH_STREAM_RECORD_BYTES));
    NativeMatchStreamSample sample;
    for(unsigned i=0;i<sender.count;++i) {
        assert(nativeMatchStream_Read(&receiver,i,&sample));
        if(i==10 || i==18) {
            assert(sample.kind==NativeMatchStream_PhaseEnd);
            assert(sample.continues==(i==10));
            PADStatus empty[4]={0};assert(!memcmp(sample.pads,empty,sizeof(empty)));
        } else {
            unsigned frame=i<10 ? i : i-11;
            assert(sample.kind==NativeMatchStream_Frame && sample.frame==frame);
            assert(sample.phase==(i<10 ? 0 : 1));
            pads_for(pads,i<10 ? i : frame+10);
            uint8_t a[NATIVE_PAD_WIRE_BYTES],b[sizeof(a)];
            for(unsigned p=0;p<4;++p) {
                nativePadWire_Write(a,&pads[p]);nativePadWire_Write(b,&sample.pads[p]);
                assert(!memcmp(a,b,sizeof(a)));
            }
        }
    }
    // A bad tail rejects the new prefix as well as its bad record.
    assert(nativeMatchStream_Bind(&receiver,&d,receive_bytes,RECORDS));
    size=nativeMatchStream_EncodeRecords(&sender,0,packet,sizeof(packet));
    memcpy(bad,packet,size);bad[NATIVE_MATCH_STREAM_PACKET_HEADER+5*NATIVE_MATCH_STREAM_RECORD_BYTES+4]=99;
    assert(!nativeMatchStream_AcceptRecords(&receiver,bad,size) && !receiver.count);
    size=nativeMatchStream_EncodeRecords(&sender,1,packet,sizeof(packet));
    assert(!nativeMatchStream_AcceptRecords(&receiver,packet,size) && !receiver.count);
    assert(nativeMatchStream_Bind(&receiver,&d,receive_bytes,2));
    pads_for(pads,0);
    assert(!nativeMatchStream_AppendFrame(&receiver,1,pads,1));
    assert(!receiver.count && !receiver.unavailable);
    assert(nativeMatchStream_AppendFrame(&receiver,0,pads,1));
    assert(nativeMatchStream_AppendFrame(&receiver,1,pads,2));
    memcpy(saved_bytes,receive_bytes,2*NATIVE_MATCH_STREAM_RECORD_BYTES);
    assert(!nativeMatchStream_AppendFrame(&receiver,2,pads,3));
    assert(receiver.unavailable && receiver.count==2);
    assert(!memcmp(saved_bytes,receive_bytes,2*NATIVE_MATCH_STREAM_RECORD_BYTES));
    assert(!nativeMatchStream_EncodeRecords(&receiver,0,packet,sizeof(packet)));
    printf("PASS %u packets, %u atomic corruption rejections, duplicate/overlap delivery, sudden-death phases, pads and bounded overflow\n",packets,corruptions);
    return 0;
}
