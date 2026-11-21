#include "fixture_portrait.h"
// Actual gift, transfer and store code. Transport queues and engine validation
// are controlled here; native runtime/model validation has separate tests.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <melee/gm/native_netplay_room.h>
#include <melee/ft/native_costume_package.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_source_assets.h>
#include <melee/ft/native_fighter_pose.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codec_dependencies.inc"

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) {fprintf(stderr,"line %u: %s\n",__LINE__,#x);exit(1);} } while(0)
static NativeNetplayRoom rooms[2];
static bool reserved[2][2], started[2][2], validate_models = true;
static bool drop_receipt, corrupt_data, drop_final_ack;
static unsigned requests[2], receipts, sent_data;
static NativeContentControl last_request;
static NativeContentId parent_id, outfit_id;
static char roots[2][MAX_PATH];
typedef struct Packet {
    unsigned target;
    bool control;
    NativeContentDatagram data;
} Packet;
static Packet queue[128];
static unsigned read_index, write_index;

static unsigned index_of(NativeNetplayConnection* connection)
{
    CHECK(connection == &rooms[0].connection || connection == &rooms[1].connection);
    return connection == &rooms[1].connection;
}
bool nativeNetplayConnection_ReserveContent(NativeNetplayConnection* c, const char* peer, uint32_t serial, bool owner, NativeContentPurpose purpose)
{
    CHECK(purpose == NativeContentPurpose_Gift);
     (void)peer; (void)serial; reserved[index_of(c)][owner] = true; return true; }
bool nativeNetplayConnection_StartContent(NativeNetplayConnection* c, const char* peer, uint32_t serial, bool owner, NativeContentPurpose purpose)
{
    CHECK(purpose == NativeContentPurpose_Gift);
     (void)peer; (void)serial; unsigned i=index_of(c); CHECK(reserved[i][owner]); started[i][owner]=true; return true; }
void nativeNetplayConnection_ReleaseContent(NativeNetplayConnection* c, const char* peer, uint32_t serial, bool owner, NativeContentPurpose purpose)
{
    CHECK(purpose == NativeContentPurpose_Gift);
     (void)peer; if(serial) reserved[index_of(c)][owner]=started[index_of(c)][owner]=false; }
NativeContentChannelState nativeNetplayConnection_ContentState(NativeNetplayConnection* c, const char* peer, uint32_t serial, bool owner, NativeContentPurpose purpose)
{
    CHECK(purpose == NativeContentPurpose_Gift);
    (void)peer;(void)serial;unsigned i=index_of(c);
    if(!reserved[i][owner])return NativeContentChannel_None;
    if(!started[i][owner])return NativeContentChannel_Reserved;
    // Closing the receiver's route can beat the final signaling receipt.
    if(!reserved[1-i][!owner])return NativeContentChannel_Failed;
    return NativeContentChannel_Connected;
}
static Packet* enqueue(NativeNetplayConnection* c, bool control)
{
    CHECK(write_index-read_index<128);
    Packet* packet=&queue[write_index++%128];memset(packet,0,sizeof(*packet));
    unsigned i=index_of(c);packet->target=1-i;packet->control=control;
    strcpy(packet->data.message.peer_id,rooms[i].connection.local_peer_id);
    packet->data.message.join_order=rooms[1-i].connection.presence.participants[0].join_order;
    return packet;
}
bool nativeNetplayConnection_SendControl(NativeNetplayConnection* c, const char* peer, const uint8_t* bytes, size_t size)
{
    (void)peer;
    if(size==41 && bytes[4]==4) {++receipts;if(drop_receipt){drop_receipt=false;return true;}}
    if(size==73 && bytes[4]==2) {
        const NativeContentId* id=(const NativeContentId*)(bytes+41);
        if(!memcmp(id,&parent_id,sizeof(*id)))++requests[0];
        else if(!memcmp(id,&outfit_id,sizeof(*id)))++requests[1];
    }
    Packet* packet=enqueue(c,true);packet->data.message.datagram.size=size;
    memcpy(packet->data.message.datagram.bytes,bytes,size);
    if(size==73 && bytes[4]==2)last_request=packet->data.message;
    return true;
}
bool nativeNetplayConnection_SendContent(NativeNetplayConnection* c, const char* peer, uint32_t serial,
                                        bool owner, NativeContentPurpose purpose, const uint8_t* bytes, size_t size)
{
    CHECK(purpose == NativeContentPurpose_Gift);
    (void)peer;NativeContentId id;
    NativeContentTransferPacketType type=nativeContentTransfer_PacketType(bytes,size,&id);
    if(type==NativeContentTransferPacket_Ack && drop_final_ack && rooms[1].incoming_gift.receive.complete)
        return true;
    Packet* packet=enqueue(c,false);packet->data.serial=serial;packet->data.local_owner=!owner;
    packet->data.message.datagram.size=size;memcpy(packet->data.message.datagram.bytes,bytes,size);
    if(type==NativeContentTransferPacket_Data) {
        ++sent_data;
        if(corrupt_data && !memcmp(&id,&outfit_id,sizeof(id))) {
            packet->data.message.datagram.bytes[size-1]^=1;corrupt_data=false;
        }
    }
    return true;
}
bool nativeCostumePackage_Validate(const NativeCostumePackage* costume,
    const NativeCostumePackageData* data, const char* root, NativeCostumeValidation* result)
{(void)costume;(void)data;(void)root;(void)result;return validate_models;}
bool nativeFighterRuntime_ValidateCached(const NativeContentId* id,const char* root)
{(void)id;(void)root;return validate_models;}

static void deliver(void)
{
    unsigned end=write_index;
    while(read_index!=end) {
        Packet packet=queue[read_index++%128];
        if(packet.control)nativeNetplayRoom_GiftControl(&rooms[packet.target],&packet.data.message);
        else nativeNetplayRoom_GiftPacket(&rooms[packet.target],&packet.data);
    }
}
static void initialize(unsigned run)
{
    memset(rooms,0,sizeof(rooms));memset(reserved,0,sizeof(reserved));memset(started,0,sizeof(started));
    read_index=write_index=receipts=sent_data=requests[0]=requests[1]=0;
    validate_models=true;drop_receipt=corrupt_data=drop_final_ack=false;
    memset(&last_request,0,sizeof(last_request));
    char temp[MAX_PATH];CHECK(GetTempPathA(sizeof(temp),temp));
    for(unsigned i=0;i<2;++i) {
        snprintf(roots[i],MAX_PATH,"%sFreakGift-%lu-%u-%u",temp,GetCurrentProcessId(),run,i);
        rooms[i].active=true;rooms[i].content_root=roots[i];
        NativeNetplayConnection* c=&rooms[i].connection;c->has_presence=true;
        snprintf(c->local_peer_id,sizeof(c->local_peer_id),"peer%u",i);strcpy(c->local_build,"fixture");
        c->presence.participant_count=1;
        c->presence.participants[0].join_order=2-i;
        snprintf(c->presence.participants[0].peer_id,sizeof(c->presence.participants[0].peer_id),"peer%u",1-i);
        strcpy(c->presence.participants[0].build,"fixture");
    }
}
static void clean_store(const char* root)
{
    const char* folders[]={"cache","installed"};
    for(unsigned i=0;i<2;++i) {
        char pattern[MAX_PATH],path[MAX_PATH];snprintf(pattern,sizeof(pattern),"%s\\%s\\*",root,folders[i]);
        WIN32_FIND_DATAA found;HANDLE search=FindFirstFileA(pattern,&found);
        if(search!=INVALID_HANDLE_VALUE) {
            do {if(found.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)continue;
                snprintf(path,sizeof(path),"%s\\%s\\%s",root,folders[i],found.cFileName);CHECK(DeleteFileA(path));
            }while(FindNextFileA(search,&found));FindClose(search);
        }
        snprintf(path,sizeof(path),"%s\\%s",root,folders[i]);RemoveDirectoryA(path);
    }
    RemoveDirectoryA(root);
}
static void finish(void)
{
    for(unsigned i=0;i<2;++i) {nativeNetplayRoom_ClearGifts(&rooms[i]);clean_store(roots[i]);
        CHECK(!reserved[i][0]&&!reserved[i][1]&&!rooms[i].outgoing_gift.bytes&&!rooms[i].outgoing_gift.dependency_bytes);}
}
static void fixture(uint8_t** parent,size_t* parent_size,uint8_t** outfit,size_t* outfit_size)
{
    static uint8_t dat[1600]={1},record[65536];size_t record_size;
    NativeSourceAssetFacts facts={0};strcpy(facts.fighter_root,"ftDataFixture");strcpy(facts.costume_joint,"fixture_joint");
    facts.actions=facts.demo_count=1;facts.joints=7;facts.effect_bank=UINT8_MAX;
    facts.moves.common_count=facts.moves.first=ftCo_MS_Count;
    const unsigned roles[]={FtPart_TopN,FtPart_TransN,FtPart_XRotN,FtPart_YRotN,FtPart_HipN,FtPart_TransN2,FT_COMMON_PART_COUNT-1};
    for(unsigned i=0;i<7;++i){bool seen=false;for(unsigned j=0;j<facts.bone_count;++j)if((unsigned)facts.bones[j].part==roles[i])seen=true;
        if(!seen)facts.bones[facts.bone_count++]=(NativeFighterBone){(Fighter_Part)roles[i],(uint8_t)i};}
    NativeScriptDiagnostic diagnostic;CHECK(nativeSourceAssets_Write(&facts,record,sizeof(record),&record_size,&diagnostic));
    static const char source[]="fighter:\n    behavior independent\naction Main:\n    on load:\n        pass\n";
    NativeFighterPackage metadata={.member_count=1};strcpy(metadata.title,"Parent");strcpy(metadata.members[0].key,"main");
    NativeFighterPackageData data={.portrait_png=fixture_portrait,.portrait_size=sizeof(fixture_portrait),.members={{
        .fighter_dat=dat,.fighter_size=sizeof(dat),.animation_dat=dat,.animation_size=sizeof(dat),.costume_dat=dat,.costume_size=sizeof(dat),
        .asset_record=record,.asset_record_size=record_size,.script_text=(const uint8_t*)source,.script_size=sizeof(source)-1}}};
    CHECK(nativeContentStore_BuildFighter(&metadata,&data,parent,parent_size,&parent_id));
    NativeCostumePackage costume={.fighter_id=parent_id,.member_count=1};strcpy(costume.title,"Outfit");strcpy(costume.member_keys[0],"main");
    NativeCostumePackageData models={.members={{dat,sizeof(dat)}}};
    CHECK(nativeContentStore_BuildCostume(&costume,&models,outfit,outfit_size,&outfit_id));
}
static void cache(unsigned who,const void* bytes,size_t size)
{NativeContentId id;CHECK(nativeContentStore_Cache(roots[who],bytes,size,NULL,&id));}
static void offered(void)
{
    CHECK(nativeNetplayRoom_OfferGift(&rooms[0],"peer1",&outfit_id));
    nativeNetplayRoom_PumpGifts(&rooms[0]);deliver();
    CHECK(rooms[1].incoming_gift.state==NativeNetplayGift_Offered);
    CHECK(!nativeContentStore_IsInstalled(roots[1],&outfit_id));
    // Changing the dependency on a retried offer cannot change this delivery.
    NativeContentControl forged=queue[(write_index-1)%128].data.message;
    CHECK(forged.datagram.size==138);
    forged.datagram.bytes[106]^=1;
    nativeNetplayRoom_GiftControl(&rooms[1],&forged);
    CHECK(!memcmp(&rooms[1].incoming_gift.dependency_id,&parent_id,sizeof(parent_id)));
}
static bool drive(void)
{
    uint64_t deadline=GetTickCount64()+5000;bool awaiting=false, stale_checked=false;
    while(GetTickCount64()<deadline) {
        nativeNetplayRoom_PumpGifts(&rooms[0]);nativeNetplayRoom_PumpGifts(&rooms[1]);deliver();
        if(!stale_checked && rooms[0].outgoing_gift.state==NativeNetplayGift_Sending &&
            !memcmp(&rooms[0].outgoing_gift.send.id,&outfit_id,sizeof(outfit_id))) {
            NativeContentControl stale=last_request;
            CHECK(stale.datagram.size==73);
            stale.datagram.bytes[41]^=0x80;
            nativeNetplayRoom_GiftControl(&rooms[0],&stale);
            CHECK(!memcmp(&rooms[0].outgoing_gift.send.id,&outfit_id,sizeof(outfit_id)));
            memcpy(stale.datagram.bytes+41,&parent_id,sizeof(parent_id));
            nativeNetplayRoom_GiftControl(&rooms[0],&stale);
            CHECK(!memcmp(&rooms[0].outgoing_gift.send.id,&outfit_id,sizeof(outfit_id)));
            stale_checked=true;
        }
        awaiting|=rooms[0].outgoing_gift.state==NativeNetplayGift_AwaitingReceipt;
        NativeNetplayGiftState state=rooms[0].outgoing_gift.state;
        if(state==NativeNetplayGift_Sent || state==NativeNetplayGift_Declined || state==NativeNetplayGift_Failed)return awaiting;
        Sleep(2);
    }
    CHECK(false);return false;
}
int main(void)
{
    uint8_t *parent,*outfit;size_t parent_size,outfit_size;fixture(&parent,&parent_size,&outfit,&outfit_size);
    for(unsigned run=0;run<10;++run) {
        initialize(run);cache(0,parent,parent_size);cache(0,outfit,outfit_size);CHECK(nativeContentStore_Install(roots[0],&outfit_id));
        if((run&1) && run!=7)cache(1,parent,parent_size);
        if((run&2) && run!=7)cache(1,outfit,outfit_size);
        if(run==3)CHECK(nativeContentStore_Install(roots[1],&parent_id));
        offered();
        if(run==6) {nativeNetplayRoom_DeclineGift(&rooms[1]);deliver();CHECK(rooms[0].outgoing_gift.state==NativeNetplayGift_Declined);finish();continue;}
        if(run==8) {
            // A cached outfit proves the offered dependency is false before fetching it.
            cache(1,outfit,outfit_size);rooms[1].incoming_gift.dependency_id.bytes[0]^=1;
            CHECK(!nativeNetplayRoom_AcceptGift(&rooms[1]));deliver();
            CHECK(rooms[0].outgoing_gift.state==NativeNetplayGift_Declined && !sent_data);
            finish();continue;
        }
        if(run==9) {
            rooms[0].outgoing_gift.state=NativeNetplayGift_AwaitingReceipt;
            rooms[0].outgoing_gift.awaiting_since_ms=GetTickCount64()-120001;
            nativeNetplayRoom_PumpGifts(&rooms[0]);
            CHECK(rooms[0].outgoing_gift.state==NativeNetplayGift_Failed);
            CHECK(!rooms[0].outgoing_gift.bytes&&!rooms[0].outgoing_gift.dependency_bytes);
            finish();continue;
        }
        validate_models=run!=4;corrupt_data=run==5;drop_receipt=run<4;drop_final_ack=run<4;
        CHECK(nativeNetplayRoom_AcceptGift(&rooms[1]));
        if(run==7) {
            nativeNetplayRoom_PumpGifts(&rooms[0]);deliver();
            CHECK(rooms[1].incoming_gift.receive.active);
            finish();continue;
        }
        bool awaiting=drive();
        if(run<4) {
            CHECK(rooms[0].outgoing_gift.state==NativeNetplayGift_Sent);
            CHECK(rooms[1].incoming_gift.state==NativeNetplayGift_Installed);
            CHECK(nativeContentStore_IsInstalled(roots[1],&parent_id)&&nativeContentStore_IsInstalled(roots[1],&outfit_id));
            NativeNetplayGiftStatus status;CHECK(nativeNetplayRoom_GiftStatus(&rooms[1],true,&status));
            NativeContentId empty={0};CHECK(!memcmp(&status.added_fighter,run==3?&empty:&parent_id,sizeof(empty)));
            CHECK((requests[0]>0)==!(run&1));CHECK((requests[1]>0)==!(run&2));
            CHECK(receipts>=2);if(run!=3)CHECK(awaiting);
        } else {
            CHECK(rooms[1].incoming_gift.state==NativeNetplayGift_Failed);
            CHECK(rooms[0].outgoing_gift.state==NativeNetplayGift_Declined);
            CHECK(!nativeContentStore_IsInstalled(roots[1],&outfit_id)&&!nativeContentStore_IsInstalled(roots[1],&parent_id));
        }
        finish();
    }
    // Ordinary fighter gifts retain their single-artifact path.
    for(unsigned cached=0;cached<2;++cached) {
        initialize(10+cached);cache(0,parent,parent_size);
        CHECK(nativeContentStore_Install(roots[0],&parent_id));
        if(cached)cache(1,parent,parent_size);
        CHECK(nativeNetplayRoom_OfferGift(&rooms[0],"peer1",&parent_id));
        nativeNetplayRoom_PumpGifts(&rooms[0]);deliver();
        CHECK(rooms[1].incoming_gift.kind==NativeNetplayGift_Fighter);
        drop_receipt=true;CHECK(nativeNetplayRoom_AcceptGift(&rooms[1]));drive();
        CHECK(rooms[0].outgoing_gift.state==NativeNetplayGift_Sent &&
              nativeContentStore_IsInstalled(roots[1],&parent_id) &&
              !nativeContentStore_IsInstalled(roots[1],&outfit_id));
        CHECK((sent_data>0)==!cached);finish();
    }
    initialize(12);cache(0,parent,parent_size);cache(1,parent,parent_size);
    CHECK(nativeContentStore_Install(roots[0],&parent_id));
    CHECK(nativeNetplayRoom_OfferGift(&rooms[0],"peer1",&parent_id));
    nativeNetplayRoom_PumpGifts(&rooms[0]);deliver();
    ++rooms[0].connection.presence.participants[0].join_order;
    CHECK(nativeNetplayRoom_AcceptGift(&rooms[1]));deliver();
    CHECK(rooms[0].outgoing_gift.state==NativeNetplayGift_Offered);
    nativeNetplayRoom_PumpGifts(&rooms[0]);
    CHECK(rooms[0].outgoing_gift.state==NativeNetplayGift_Failed);
    finish();
    free(parent);free(outfit);printf("PASS gift dependency cases: %u checks\n",checks);return 0;
}
