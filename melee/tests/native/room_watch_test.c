#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/gm/native_netplay_room.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static NativeNetplayRoom rooms[2];
static uint8_t records[400*NATIVE_MATCH_STREAM_RECORD_BYTES];
static char peer_ids[5][65];
static unsigned checks,dropped_metadata,dropped_records,duplicates;
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"line %u: %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct Route {uint32_t serial;bool reserved,started;} Route;
static Route routes[2];
typedef struct Packet {NativeContentDatagram data;unsigned target;bool control;} Packet;
static Packet queue[256];
static unsigned read_index,write_index;
static bool lose_metadata,lose_records;

static unsigned index_of(NativeNetplayConnection* c)
{CHECK(c==&rooms[0].connection || c==&rooms[1].connection);return c==&rooms[1].connection;}
bool nativeNetplayConnection_ReserveContent(NativeNetplayConnection* c,const char* peer,uint32_t serial,bool owner, NativeContentPurpose purpose)
{
    CHECK(purpose == NativeContentPurpose_Watch);
    unsigned i=index_of(c);CHECK(!strcmp(peer,rooms[1-i].connection.local_peer_id));CHECK(owner==(i==1));routes[i]=(Route){.serial=serial,.reserved=true};return true;}
bool nativeNetplayConnection_StartContent(NativeNetplayConnection* c,const char* peer,uint32_t serial,bool owner, NativeContentPurpose purpose)
{
    CHECK(purpose == NativeContentPurpose_Watch);
    (void)peer;(void)owner;unsigned i=index_of(c);CHECK(routes[i].reserved && routes[i].serial==serial);routes[i].started=true;return true;}
void nativeNetplayConnection_ReleaseContent(NativeNetplayConnection* c,const char* peer,uint32_t serial,bool owner, NativeContentPurpose purpose)
{
    CHECK(purpose == NativeContentPurpose_Watch);
    (void)peer;(void)owner;unsigned i=index_of(c);if(routes[i].serial==serial)routes[i]=(Route){0};}
NativeContentChannelState nativeNetplayConnection_ContentState(NativeNetplayConnection* c,const char* peer,uint32_t serial,bool owner, NativeContentPurpose purpose)
{
    CHECK(purpose == NativeContentPurpose_Watch);
    (void)peer;(void)owner;unsigned i=index_of(c);
    if(!routes[i].reserved || routes[i].serial!=serial)return NativeContentChannel_None;
    if(!routes[i].started)return NativeContentChannel_Reserved;
    return routes[1-i].started ? NativeContentChannel_Connected : NativeContentChannel_Connecting;
}
static Packet* enqueue(NativeNetplayConnection* c,const uint8_t* bytes,size_t size,bool control)
{
    CHECK(write_index-read_index<256 && size<=NATIVE_NETPLAY_INPUT_PACKET_CAPACITY);
    unsigned i=index_of(c);Packet* p=&queue[write_index++%256];*p=(Packet){.target=1-i,.control=control};
    strcpy(p->data.message.peer_id,rooms[i].connection.local_peer_id);
    p->data.message.datagram.size=(uint16_t)size;memcpy(p->data.message.datagram.bytes,bytes,size);
    return p;
}
bool nativeNetplayConnection_SendControl(NativeNetplayConnection* c,const char* peer,const uint8_t* bytes,size_t size)
{(void)peer;enqueue(c,bytes,size,true);return true;}
bool nativeNetplayConnection_SendContent(NativeNetplayConnection* c,const char* peer,uint32_t serial,bool owner, NativeContentPurpose purpose,const uint8_t* bytes,size_t size)
{
    CHECK(purpose == NativeContentPurpose_Watch);
    (void)peer;
    if(!memcmp(bytes,"FMWV",4) && bytes[4]==1 && lose_metadata) {
        lose_metadata=false;++dropped_metadata;return true;
    }
    if(!memcmp(bytes,"FMWD",4) && lose_records) {
        lose_records=false;++dropped_records;return true;
    }
    Packet* p=enqueue(c,bytes,size,false);p->data.serial=serial;p->data.local_owner=!owner;
    // Every third input packet is duplicated. A lost early packet also causes
    // later packets to arrive beyond the receiver's contiguous prefix.
    if(!memcmp(bytes,"FMWD",4) && ++duplicates%3==0) {
        Packet saved=*p;p=enqueue(c,bytes,size,false);*p=saved;
    }
    return true;
}
static void deliver(void)
{
    while(read_index!=write_index) {
        Packet p=queue[read_index++%256];
        if(p.control)nativeNetplayRoom_WatchControl(&rooms[p.target],&p.data.message);
        else nativeNetplayRoom_WatchPacket(&rooms[p.target],&p.data);
    }
}
static void append(unsigned from,unsigned to)
{
    PADStatus pads[4]={0};
    for(unsigned f=from;f<to;++f) {
        for(unsigned p=0;p<4;++p){pads[p].stickX=(int8_t)(f+p);pads[p].extButton=f*17+p;}
        CHECK(nativeMatchStream_AppendFrame(&rooms[0].stream,f,pads,0x12340000+f));
    }
}
int main(void)
{
    NativeMatchStreamDescription d={.initial_seed=42,.match={.session_id=123456789,
        .stage_id=32,.peer_count=4,.mode=NativeLobbyMatchMode_Stock,.stock_count=1,
        .damage_ratio=10,.item_frequency=-1,.content_digest={1}}};
    for(unsigned p=0;p<5;++p) {memset(peer_ids[p],'a'+p,64);peer_ids[p][64]=0;}
    for(unsigned p=0;p<4;++p) {
        strcpy(d.match.peers[p].peer_id,peer_ids[p]);
        d.match.peers[p].numeric_peer=(uint8_t)p;d.match.peers[p].port_mask=1u<<p;
        d.match.slots[p].kind=NativeLobbyMatchSlot_Human;
        d.match.slots[p].character_id=p+1;d.match.slots[p].handicap=9;
    }
    for(unsigned i=0;i<2;++i) {
        rooms[i].active=true;NativeNetplayConnection* c=&rooms[i].connection;
        c->active=c->has_presence=true;c->topology_locked=i==0;
        strcpy(c->local_peer_id,peer_ids[i==0 ? 0 : 4]);strcpy(c->local_build,"fixture");
        c->presence.participant_count=5;
        for(unsigned p=0;p<5;++p) {
            strcpy(c->presence.participants[p].peer_id,peer_ids[p]);
            strcpy(c->presence.participants[p].build,"fixture");
            c->presence.participants[p].active_mask=p<4 ? 1 : 0;
        }
        c->presence.match_size=(uint16_t)nativeLobbyMatch_Encode(&d.match,c->presence.match,sizeof(c->presence.match));
        CHECK(c->presence.match_size>512);
    }
    CHECK(nativeMatchStream_Bind(&rooms[0].stream,&d,records,400));
    append(0,240);
    const uint8_t other_build[32] = {2};
    CHECK(!nativeNetplayRoom_WatchMatch(&rooms[1], other_build));
    CHECK(!rooms[1].watch.receive.storage);
    CHECK(nativeNetplayRoom_WatchMatch(&rooms[1], d.match.content_digest));
    CHECK(!nativeNetplayRoom_WatchMatch(&rooms[1], d.match.content_digest));
    lose_metadata=lose_records=true;
    uint64_t deadline=GetTickCount64()+5000;bool extended=false,closed_after_final=false;
    while(GetTickCount64()<deadline) {
        nativeNetplayRoom_PumpWatch(&rooms[0]);nativeNetplayRoom_PumpWatch(&rooms[1]);deliver();
        if(!extended && rooms[1].watch.receive.stream.count>100) {
            append(240,270);CHECK(nativeMatchStream_EndPhase(&rooms[0].stream,true,0x7788));
            append(0,20);CHECK(nativeMatchStream_EndPhase(&rooms[0].stream,false,0x9988));extended=true;
        }
        NativeRoomWatchReceive* received=&rooms[1].watch.receive;
        if(!closed_after_final && received->stream.finished && received->remote_finished &&
           received->stream.count==received->remote_count && received->state==NativeRoomWatch_Receiving) {
            routes[1].started=false;closed_after_final=true;
        }
        if(rooms[1].watch.receive.state==NativeRoomWatch_Complete)break;
        CHECK(rooms[1].watch.receive.state!=NativeRoomWatch_Failed);
        Sleep(17);
    }
    NativeRoomWatchReceive* r=&rooms[1].watch.receive;
    CHECK(r->state==NativeRoomWatch_Complete && extended && closed_after_final);
    CHECK(r->stream.finished && r->stream.count==rooms[0].stream.count && r->stream.phase==1);
    CHECK(r->stream.digest==rooms[0].stream.digest);
    CHECK(!memcmp(records,r->stream.records,(size_t)r->stream.count*NATIVE_MATCH_STREAM_RECORD_BYTES));
    CHECK(dropped_metadata==1 && dropped_records==1 && duplicates>3);
    CHECK(!rooms[0].watch.sender_count);
    // The receiver retains the complete history after closing the connection.
    CHECK(!routes[0].reserved && !routes[1].reserved && r->storage);
    nativeNetplayRoom_StopWatching(&rooms[1]);CHECK(!r->storage && r->state==NativeRoomWatch_None);
    deliver();
    CHECK(nativeNetplayRoom_WatchMatch(&rooms[1], d.match.content_digest));
    r->started_ms=GetTickCount64()-90001;
    rooms[1].watch.pump_ms=0;
    nativeNetplayRoom_PumpWatch(&rooms[1]);
    CHECK(r->state==NativeRoomWatch_Failed && !routes[1].reserved);
    nativeNetplayRoom_StopWatching(&rooms[1]);deliver();

    CHECK(nativeNetplayRoom_WatchMatch(&rooms[1], d.match.content_digest));
    routes[1].reserved=false;
    rooms[1].watch.pump_ms=0;
    nativeNetplayRoom_PumpWatch(&rooms[1]);
    CHECK(r->state==NativeRoomWatch_Failed);
    nativeNetplayRoom_StopWatching(&rooms[1]);deliver();

    CHECK(nativeMatchStream_Bind(&rooms[0].stream,&d,records,400));
    CHECK(nativeNetplayRoom_WatchMatch(&rooms[1], d.match.content_digest));
    rooms[1].watch.pump_ms=0;
    nativeNetplayRoom_PumpWatch(&rooms[1]);deliver();
    CHECK(rooms[0].watch.sender_count==1 && r->state==NativeRoomWatch_Receiving);
    rooms[0].connection.presence.participants[4].active_mask=1;
    rooms[0].watch.pump_ms=0;
    nativeNetplayRoom_PumpWatch(&rooms[0]);deliver();
    CHECK(!rooms[0].watch.sender_count && r->state==NativeRoomWatch_Failed);
    nativeNetplayRoom_StopWatching(&rooms[1]);deliver();
    // Active fighters cannot request a second match-viewing role.
    rooms[1].connection.presence.participants[4].active_mask=1;
    CHECK(!nativeNetplayRoom_WatchMatch(&rooms[1], d.match.content_digest));
    nativeNetplayRoom_ClearWatch(&rooms[0]);nativeNetplayRoom_ClearWatch(&rooms[1]);
    printf("PASS %u checks: fragmented metadata, lost/gapped/duplicate delivery, live extension, sudden death, final digest and role isolation\n",checks);
    return 0;
}
