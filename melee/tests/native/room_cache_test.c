// Actual cache/transfer state machines with controlled transport and storage.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/gm/native_netplay_room.h>
#include <melee/gm/native_lobby_selection.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static NativeNetplayRoom rooms[2];
static uint8_t data[2][16384];
static const size_t sizes[2]={16384,1536};
static NativeContentId ids[2]={{{1}},{{2}}};
static bool cached[2],disk_available=true;
static unsigned reads,checks;
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"line %u: %s\n",__LINE__,#x);exit(1);}}while(0)
static unsigned id_index(const NativeContentId* id)
{unsigned i=id->bytes[0]-1;CHECK(i<2 && !memcmp(id,&ids[i],sizeof(*id)));return i;}
bool nativeContentStore_DefaultRoot(char* out,size_t size)
{(void)out;(void)size;return false;}
void nativeContentStore_UpdatePreview(const void* prefix, size_t received, NativeContentPreview* preview)
{ (void)prefix; (void)received; (void)preview; }
bool nativeContentStore_IsCached(const char* root,const NativeContentId* id)
{unsigned i=id_index(id);CHECK(strcmp(root,"source") || !rooms[0].match_configured);return !strcmp(root,"source") || cached[i];}
bool nativeContentStore_IsInstalled(const char* root,const NativeContentId* id)
{(void)id;CHECK(strcmp(root,"source") || !rooms[0].match_configured);return !strcmp(root,"source") && disk_available;}
bool nativeContentStore_Read(const char* root,const NativeContentId* id,uint8_t** bytes,size_t* size)
{
    CHECK(!strcmp(root,"source"));++reads;if(!disk_available)return false;
    unsigned i=id_index(id);*bytes=malloc(sizes[i]);CHECK(*bytes);*size=sizes[i];memcpy(*bytes,data[i],*size);return true;
}
static unsigned active_leases;
void nativeContentStore_Unpin(void** lease)
{
    if (*lease) {
        CHECK(active_leases > 0);
        --active_leases;
        *lease = NULL;
    }
}
bool nativeContentStore_Pin(const char* root,const NativeContentId* id,void** lease)
{
    CHECK(!*lease);
    if (!nativeContentStore_IsCached(root, id)) return false;
    ++active_leases;
    *lease = &data[id_index(id)];
    return true;
}
bool nativeContentStore_CachePeer(const char* root,const void* bytes,size_t size,const NativeContentId* expected,void** lease)
{
    CHECK(!strcmp(root,"viewer"));unsigned i=id_index(expected);
    CHECK(size==sizes[i] && !memcmp(bytes,data[i],size));cached[i]=true;
    CHECK(!*lease);++active_leases;*lease=&data[i];return true;
}
typedef struct Route {uint32_t serial;bool reserved,started;} Route;
static Route routes[2];
typedef struct Packet {NativeContentDatagram data;unsigned target;bool control;} Packet;
static Packet queue[256];static unsigned read_index,write_index;
static unsigned index_of(NativeNetplayConnection* c)
{CHECK(c==&rooms[0].connection || c==&rooms[1].connection);return c==&rooms[1].connection;}
bool nativeNetplayConnection_ReserveContent(NativeNetplayConnection* c,const char* peer,uint32_t serial,bool owner, NativeContentPurpose purpose)
{
    CHECK(purpose == NativeContentPurpose_Cache);
    (void)peer;unsigned i=index_of(c);CHECK(owner==(i==1));routes[i]=(Route){.serial=serial,.reserved=true};return true;}
bool nativeNetplayConnection_StartContent(NativeNetplayConnection* c,const char* peer,uint32_t serial,bool owner, NativeContentPurpose purpose)
{
    CHECK(purpose == NativeContentPurpose_Cache);
    (void)peer;(void)owner;unsigned i=index_of(c);CHECK(routes[i].serial==serial && routes[i].reserved);routes[i].started=true;return true;}
void nativeNetplayConnection_ReleaseContent(NativeNetplayConnection* c,const char* peer,uint32_t serial,bool owner, NativeContentPurpose purpose)
{
    CHECK(purpose == NativeContentPurpose_Cache);
    (void)peer;(void)owner;unsigned i=index_of(c);if(routes[i].serial==serial)routes[i]=(Route){0};}
NativeContentChannelState nativeNetplayConnection_ContentState(NativeNetplayConnection* c,const char* peer,uint32_t serial,bool owner, NativeContentPurpose purpose)
{
    CHECK(purpose == NativeContentPurpose_Cache);
    (void)peer;(void)owner;unsigned i=index_of(c);
    if(!routes[i].reserved || routes[i].serial!=serial)return NativeContentChannel_None;
    if(!routes[i].started)return NativeContentChannel_Reserved;
    return routes[1-i].started ? NativeContentChannel_Connected : NativeContentChannel_Connecting;
}
static Packet* enqueue(NativeNetplayConnection* c,const uint8_t* bytes,size_t size,bool control)
{
    CHECK(write_index-read_index<256 && size<=NATIVE_CONTENT_TRANSFER_PACKET_CAPACITY);
    unsigned i=index_of(c);
    Packet* p=&queue[write_index++%256];*p=(Packet){.target=1-i,.control=control};
    strcpy(p->data.message.peer_id,rooms[i].connection.local_peer_id);
    p->data.message.datagram.size=(uint16_t)size;memcpy(p->data.message.datagram.bytes,bytes,size);return p;
}
bool nativeNetplayConnection_SendControl(NativeNetplayConnection* c,const char* peer,const uint8_t* bytes,size_t size)
{(void)peer;enqueue(c,bytes,size,true);return true;}
bool nativeNetplayConnection_SendContent(NativeNetplayConnection* c,const char* peer,uint32_t serial,bool owner, NativeContentPurpose purpose,const uint8_t* bytes,size_t size)
{
    CHECK(purpose == NativeContentPurpose_Cache);
    (void)peer;Packet* p=enqueue(c,bytes,size,false);p->data.serial=serial;p->data.local_owner=!owner;return true;}
static bool checked_upload_limit;
static void deliver(void)
{
    while(read_index!=write_index) {
        Packet p=queue[read_index++%256];
        if(p.control) {
            nativeNetplayRoom_CacheControl(&rooms[p.target],&p.data.message);
            if(!checked_upload_limit && p.target==0 && p.data.message.datagram.bytes[4]==1) {
                unsigned requests=rooms[0].cache.active_requests;
                CHECK(requests==1);
                nativeNetplayRoom_CacheControl(&rooms[0],&p.data.message);
                ++p.data.message.datagram.bytes[37];
                nativeNetplayRoom_CacheControl(&rooms[0],&p.data.message);
                CHECK(rooms[0].cache.active_requests==requests);
                checked_upload_limit=true;
            }
        } else nativeNetplayRoom_CachePacket(&rooms[p.target],&p.data);
    }
}
int main(void)
{
    for(unsigned i=0;i<2;++i) {
        for(size_t n=0;n<sizes[i];++n)data[i][n]=(uint8_t)(n*13+i);
        rooms[i].active=true;rooms[i].content_root=i ? "viewer" : "source";
        NativeNetplayConnection* c=&rooms[i].connection;c->active=c->has_presence=true;
        strcpy(c->local_peer_id,i ? "peer_viewer" : "peer_source");strcpy(c->local_build,"fixture");
        c->presence.participant_count=2;c->presence.revision=1;
        for(unsigned p=0;p<2;++p) {
            strcpy(c->presence.participants[p].peer_id,p ? "peer_viewer" : "peer_source");
            strcpy(c->presence.participants[p].build,"fixture");
        }
        c->presence.participants[0].active_mask=1;
        NativeLobbySelection s={.player_count=1,.confirmed=true,
            .players={{.selected=true,.character_package_id=ids[0],.costume_package_id=ids[1]}}};
        c->presence.participants[0].selection_size=(uint16_t)nativeLobbySelection_Encode(&s,
            c->presence.participants[0].selection,sizeof(c->presence.participants[0].selection));
        CHECK(c->presence.participants[0].selection_size);
    }
    rooms[0].match_configured=true;
    NativeLobbyMatch* match=&rooms[0].match;match->session_id=42;match->peer_count=1;
    strcpy(match->peers[0].peer_id,"peer_source");match->peers[0].port_mask=0xF;
    for(unsigned p=0;p<2;++p) {
        match->slots[p].kind=NativeLobbyMatchSlot_Human;
        match->slots[p].character_package_id=ids[0];match->slots[p].costume_package_id=ids[1];
    }
    CHECK(nativeNetplayRoom_RetainMatchPackages(&rooms[0]));
    CHECK(reads==2 && rooms[0].cache.retained_count==2 &&
          rooms[0].cache.retained_bytes==sizes[0]+sizes[1]);
    rooms[0].ready=true;
    disk_available=false;
    rooms[1].watch.receive.state=NativeRoomWatch_Requesting;
    rooms[1].watch.receive.serial=90;rooms[1].watch.receive.stream.description.match=*match;
    CHECK(!nativeNetplayRoom_WatchContentReady(&rooms[1]));
    bool ended=false,rate_limited=false;uint64_t deadline=GetTickCount64()+5000;
    while(GetTickCount64()<deadline && (!cached[0] || !cached[1] || rooms[1].cache.serial)) {
        unsigned sent_before=rooms[0].cache.active_packets;
        nativeNetplayRoom_PumpCache(&rooms[0]);nativeNetplayRoom_PumpCache(&rooms[1]);deliver();
        if(!ended) {
            unsigned sent=rooms[0].cache.active_packets-sent_before;
            CHECK(sent<=4);
            rate_limited|=sent==4;
        }
        if(cached[0] && !ended) {
            // A completed match clears active selections. The requested exact
            // outfit must stay pinned and its retained sender must survive.
            ended=true;rooms[0].match_configured=false;
            for(unsigned i=0;i<2;++i) {
                NativeSignalingParticipant* p=&rooms[i].connection.presence.participants[0];
                p->active_mask=0;p->selection_size=0;++rooms[i].connection.presence.revision;
            }
        }
        Sleep(17);
    }
    CHECK(cached[0] && cached[1] && ended && !rooms[1].cache.serial);
    CHECK(reads==2 && rooms[0].cache.file_reads==2);
    CHECK(checked_upload_limit && rooms[0].cache.active_requests>0 && rate_limited);
    CHECK(!memcmp(rooms[0].cache.retained[0].bytes,data[0],sizes[0]));
    CHECK(!memcmp(rooms[0].cache.retained[1].bytes,data[1],sizes[1]));
    CHECK(!nativeNetplayRoom_WatchContentReady(&rooms[1]));
    rooms[1].watch.receive.state=NativeRoomWatch_Receiving;
    rooms[1].watch.receive.stream.records=data[0]; // Metadata validation is tested by room_watch.
    CHECK(nativeNetplayRoom_WatchContentReady(&rooms[1]));
    nativeNetplayRoom_ClearCacheTransfers(&rooms[0]);nativeNetplayRoom_ClearCacheTransfers(&rooms[1]);deliver();
    CHECK(!active_leases);
    nativeNetplayRoom_ClearRetainedPackages(&rooms[0]);
    CHECK(!rooms[0].cache.retained_count && !rooms[0].cache.retained_bytes);
    printf("PASS %u checks: immutable shared uploads, zero active file reads, exact outfit dependency after Results, readiness and release\n",checks);
    return 0;
}
