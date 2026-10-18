#include "native_netplay_room.h"
#ifdef MELEE_NATIVE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

enum {
    WATCH_CONTROL_BYTES=17, WATCH_METADATA_HEADER=20, WATCH_METADATA_CHUNK=512,
    WATCH_PROGRESS_BYTES=28, WATCH_SEND_WINDOW=96, WATCH_BURST=8,
    WATCH_PUMP_MS=16, WATCH_RETRY_MS=250, WATCH_CONTROL_RETRY_MS=500,
    WATCH_TIMEOUT_MS=30000,
};
enum { Watch_Request=1, Watch_Accepted, Watch_Pending, Watch_Unavailable, Watch_Cancel };
enum { Watch_Metadata=1, Watch_Ack, Watch_Status };

static void put(uint8_t* out,uint64_t value,unsigned bytes)
{for(unsigned i=0;i<bytes;++i)out[i]=(uint8_t)(value>>(8*i));}
static uint64_t get(const uint8_t* in,unsigned bytes)
{uint64_t value=0;for(unsigned i=0;i<bytes;++i)value|=(uint64_t)in[i]<<(8*i);return value;}
static uint8_t metadata_bits(size_t bytes)
{return (uint8_t)((1u<<((bytes+WATCH_METADATA_CHUNK-1)/WATCH_METADATA_CHUNK))-1);}

static const NativeSignalingParticipant* member(NativeNetplayRoom* room,const char* peer)
{
    NativeNetplayConnection* c=&room->connection;
    for(unsigned i=0;c->has_presence && i<c->presence.participant_count;++i) {
        const NativeSignalingParticipant* p=&c->presence.participants[i];
        if(!strcmp(p->peer_id,peer) && !strcmp(p->build,c->local_build))return p;
    }
    return NULL;
}
static bool control(NativeNetplayRoom* room,const char* peer,uint32_t serial,
                    uint64_t session,uint8_t type)
{
    uint8_t bytes[WATCH_CONTROL_BYTES];memcpy(bytes,"FMVW",4);bytes[4]=type;
    put(bytes+5,serial,4);put(bytes+9,session,8);
    return nativeNetplayConnection_SendControl(&room->connection,peer,bytes,sizeof(bytes));
}
static void stop_send(NativeNetplayRoom* room,NativeRoomWatchSend* send,bool notify)
{
    if(!send->serial)return;
    if(notify)control(room,send->peer_id,send->serial,send->session,Watch_Unavailable);
    nativeNetplayConnection_ReleaseContent(&room->connection,send->peer_id,send->serial,false);
    --room->watch.sender_count;
    *send=(NativeRoomWatchSend){0};
}
static void release_receive(NativeNetplayRoom* room)
{
    NativeRoomWatchReceive* r=&room->watch.receive;
    if(!r->serial)return;
    control(room,r->peer_id,r->serial,r->session,Watch_Cancel);
    nativeNetplayConnection_ReleaseContent(&room->connection,r->peer_id,r->serial,true);
}
static void fail_receive(NativeNetplayRoom* room,const char* reason)
{
    NativeRoomWatchReceive* r=&room->watch.receive;
    release_receive(room);r->state=NativeRoomWatch_Failed;
    snprintf(r->failure,sizeof(r->failure),"%s",reason);
}
void nativeNetplayRoom_StopWatching(NativeNetplayRoom* room)
{
    release_receive(room);free(room->watch.receive.storage);
    room->watch.receive=(NativeRoomWatchReceive){0};
}
void nativeNetplayRoom_ClearWatch(NativeNetplayRoom* room)
{
    nativeNetplayRoom_StopWatching(room);
    for(unsigned i=0;i<NATIVE_SIGNALING_MAX_PARTICIPANTS-1;++i)
        stop_send(room,&room->watch.sends[i],true);
    room->watch.cursor=0;room->watch.pump_ms=0;
}
bool nativeNetplayRoom_WatchMatch(NativeNetplayRoom* room)
{
    if(!room || !room->active || room->match_configured ||
       room->watch.receive.state==NativeRoomWatch_Requesting ||
       room->watch.receive.state==NativeRoomWatch_Receiving)return false;
    const NativeSignalingPresence* p=&room->connection.presence;
    const NativeSignalingParticipant* local=member(room,room->connection.local_peer_id);
    NativeLobbyMatch match;
    if(!local || local->active_mask || !p->match_size ||
       !nativeLobbyMatch_Decode(&match,p->match,p->match_size))return false;
    const NativeLobbyMatchPeer* authority=NULL;
    for(unsigned i=0;i<match.peer_count;++i)
        if(match.peers[i].numeric_peer==0)authority=&match.peers[i];
    if(!authority || !member(room,authority->peer_id) ||
       !strcmp(authority->peer_id,room->connection.local_peer_id))return false;
    nativeNetplayRoom_StopWatching(room);
    NativeRoomWatchReceive* r=&room->watch.receive;
    r->storage=malloc((size_t)NATIVE_MATCH_STREAM_RECORD_CAPACITY*NATIVE_MATCH_STREAM_RECORD_BYTES);
    if(!r->storage) {r->state=NativeRoomWatch_Failed;snprintf(r->failure,sizeof(r->failure),"Could not reserve match history.");return false;}
    r->session=match.session_id;r->stream.description.match=match;
    r->serial=++room->content_serial;if(!r->serial)r->serial=++room->content_serial;
    strcpy(r->peer_id,authority->peer_id);r->started_ms=r->data_ms=GetTickCount64();
    if(!nativeNetplayConnection_ReserveContent(&room->connection,r->peer_id,r->serial,true)) {
        fail_receive(room,"No viewing connection is available.");return false;
    }
    r->state=NativeRoomWatch_Requesting;
    return true;
}
void nativeNetplayRoom_WatchControl(NativeNetplayRoom* room,const NativeContentControl* message)
{
    const uint8_t* bytes=message->datagram.bytes;
    if(message->datagram.size!=WATCH_CONTROL_BYTES || memcmp(bytes,"FMVW",4))return;
    uint32_t serial=(uint32_t)get(bytes+5,4);uint64_t session=get(bytes+9,8);
    const NativeSignalingParticipant* person=member(room,message->peer_id);
    if(!serial || !session || !person)return;
    if(bytes[4]==Watch_Request) {
        if(person->active_mask) {control(room,message->peer_id,serial,session,Watch_Unavailable);return;}
        NativeMatchStream* stream=&room->stream;
        if(!stream->records || stream->description.match.session_id!=session) {
            NativeLobbyMatch pending;
            bool preparing=room->connection.presence.match_size &&
                nativeLobbyMatch_Decode(&pending,room->connection.presence.match,
                    room->connection.presence.match_size) && pending.session_id==session;
            control(room,message->peer_id,serial,session,preparing ? Watch_Pending : Watch_Unavailable);
            return;
        }
        if(stream->unavailable) {control(room,message->peer_id,serial,session,Watch_Unavailable);return;}
        NativeRoomWatchSend* available=NULL;
        for(unsigned i=0;i<NATIVE_SIGNALING_MAX_PARTICIPANTS-1;++i) {
            NativeRoomWatchSend* s=&room->watch.sends[i];
            if(s->serial && !strcmp(s->peer_id,message->peer_id)) {
                if(s->serial==serial && s->session==session) {
                    control(room,s->peer_id,serial,session,Watch_Accepted);return;
                }
                stop_send(room,s,false);
            }
            if(!s->serial && !available)available=s;
        }
        if(!available || !nativeNetplayConnection_ReserveContent(&room->connection,message->peer_id,serial,false)) {
            control(room,message->peer_id,serial,session,Watch_Unavailable);return;
        }
        *available=(NativeRoomWatchSend){.serial=serial,.session=session,.last_ack_ms=GetTickCount64()};
        ++room->watch.sender_count;
        strcpy(available->peer_id,message->peer_id);
        control(room,available->peer_id,serial,session,Watch_Accepted);
    } else if(bytes[4]==Watch_Cancel) {
        for(unsigned i=0;i<NATIVE_SIGNALING_MAX_PARTICIPANTS-1;++i) {
            NativeRoomWatchSend* s=&room->watch.sends[i];
            if(s->serial==serial && s->session==session && !strcmp(s->peer_id,message->peer_id))stop_send(room,s,false);
        }
    } else {
        NativeRoomWatchReceive* r=&room->watch.receive;
        if((r->state!=NativeRoomWatch_Requesting && r->state!=NativeRoomWatch_Receiving) ||
           r->serial!=serial || r->session!=session || strcmp(r->peer_id,message->peer_id))return;
        if(bytes[4]==Watch_Accepted) {
            if(!nativeNetplayConnection_StartContent(&room->connection,r->peer_id,r->serial,true))
                fail_receive(room,"Could not connect to the match.");
            else {r->state=NativeRoomWatch_Receiving;r->data_ms=GetTickCount64();}
        } else if(bytes[4]==Watch_Unavailable)fail_receive(room,"This match is no longer available.");
        else if(bytes[4]==Watch_Pending)r->data_ms=GetTickCount64();
    }
}
static bool same_match(const NativeLobbyMatch* a,const NativeLobbyMatch* b)
{
    uint8_t left[NATIVE_LOBBY_MATCH_MAX_WIRE_BYTES],right[sizeof(left)];
    size_t bytes=nativeLobbyMatch_Encode(a,left,sizeof(left));
    return bytes && nativeLobbyMatch_Encode(b,right,sizeof(right))==bytes && !memcmp(left,right,bytes);
}
static void metadata(NativeNetplayRoom* room,const uint8_t* bytes,size_t size)
{
    NativeRoomWatchReceive* r=&room->watch.receive;
    if(size<=WATCH_METADATA_HEADER || get(bytes+17,3))return;
    unsigned total=(unsigned)get(bytes+13,2),offset=(unsigned)get(bytes+15,2);
    if(!total || total>sizeof(r->metadata) || offset>=total || offset%WATCH_METADATA_CHUNK ||
       (r->metadata_size && total!=r->metadata_size))return;
    unsigned chunk=total-offset;if(chunk>WATCH_METADATA_CHUNK)chunk=WATCH_METADATA_CHUNK;
    if(size!=WATCH_METADATA_HEADER+chunk)return;
    uint8_t bit=1u<<(offset/WATCH_METADATA_CHUNK);
    if((r->metadata_mask&bit) && memcmp(r->metadata+offset,bytes+WATCH_METADATA_HEADER,chunk)) {
        fail_receive(room,"Conflicting match descriptions.");return;
    }
    memcpy(r->metadata+offset,bytes+WATCH_METADATA_HEADER,chunk);
    r->metadata_size=(uint16_t)total;r->metadata_mask|=bit;
    if(r->metadata_mask==metadata_bits(total) && !r->stream.records) {
        NativeMatchStreamDescription description;
        if(!nativeMatchStream_DecodeDescription(&description,r->metadata,total) ||
           !same_match(&description.match,&r->stream.description.match) ||
           !nativeMatchStream_Bind(&r->stream,&description,r->storage,NATIVE_MATCH_STREAM_RECORD_CAPACITY))
            fail_receive(room,"The match description changed.");
    }
}
void nativeNetplayRoom_WatchPacket(NativeNetplayRoom* room,const NativeContentDatagram* message)
{
    const uint8_t* bytes=message->message.datagram.bytes;size_t size=message->message.datagram.size;
    if(size<4)return;
    uint64_t now=GetTickCount64();
    if(!message->local_owner) {
        if(size!=WATCH_PROGRESS_BYTES || memcmp(bytes,"FMWV",4) || bytes[4]!=Watch_Ack || get(bytes+18,2))return;
        for(unsigned i=0;i<NATIVE_SIGNALING_MAX_PARTICIPANTS-1;++i) {
            NativeRoomWatchSend* s=&room->watch.sends[i];
            if(!s->serial || s->serial!=message->serial || s->session!=get(bytes+5,8) ||
               strcmp(s->peer_id,message->message.peer_id))continue;
            uint32_t count=(uint32_t)get(bytes+13,4);
            if(count>s->sent || count<s->acknowledged || bytes[17]&~3u)return;
            s->acknowledged=count;s->metadata_mask|=bytes[17];s->last_ack_ms=now;
            return;
        }
        return;
    }
    NativeRoomWatchReceive* r=&room->watch.receive;
    if(r->state!=NativeRoomWatch_Receiving || r->serial!=message->serial ||
       strcmp(r->peer_id,message->message.peer_id))return;
    if(!memcmp(bytes,"FMWD",4)) {
        if(r->stream.records && nativeMatchStream_AcceptRecords(&r->stream,bytes,size))r->data_ms=now;
        return;
    }
    if(size<13 || memcmp(bytes,"FMWV",4) || get(bytes+5,8)!=r->session)return;
    if(bytes[4]==Watch_Metadata)metadata(room,bytes,size);
    else if(bytes[4]==Watch_Status && size==WATCH_PROGRESS_BYTES && !get(bytes+18,2) && !(bytes[17]&~3u)) {
        uint32_t count=(uint32_t)get(bytes+13,4);uint64_t digest=get(bytes+20,8);
        if(count>NATIVE_MATCH_STREAM_RECORD_CAPACITY) {fail_receive(room,"The match history is too large.");return;}
        if(count<r->remote_count)return;
        if(r->status_received && count==r->remote_count && digest!=r->remote_digest) {
            fail_receive(room,"Conflicting match history.");return;
        }
        r->remote_count=count;r->remote_digest=digest;r->remote_finished=bytes[17]&1;r->status_received=true;
        if(bytes[17]&2) {fail_receive(room,"The match history is unavailable.");return;}
    } else return;
    r->data_ms=now;
}
static bool progress(NativeNetplayRoom* room,const char* peer,uint32_t serial,bool owner,
                     uint64_t session,uint8_t type,uint32_t count,uint8_t flags,uint64_t digest)
{
    uint8_t bytes[WATCH_PROGRESS_BYTES]={0};memcpy(bytes,"FMWV",4);bytes[4]=type;
    put(bytes+5,session,8);put(bytes+13,count,4);bytes[17]=flags;put(bytes+20,digest,8);
    return nativeNetplayConnection_SendContent(&room->connection,peer,serial,owner,bytes,sizeof(bytes));
}
void nativeNetplayRoom_PumpWatch(NativeNetplayRoom* room)
{
    NativeRoomWatch* watch=&room->watch;uint64_t now=GetTickCount64();
    if(now-watch->pump_ms<WATCH_PUMP_MS)return;
    watch->pump_ms=now;
    NativeRoomWatchReceive* r=&watch->receive;
    if(r->state==NativeRoomWatch_Requesting || r->state==NativeRoomWatch_Receiving) {
        NativeContentChannelState state=nativeNetplayConnection_ContentState(&room->connection,r->peer_id,r->serial,true);
        if(state==NativeContentChannel_Failed || !member(room,r->peer_id) || now-r->data_ms>=WATCH_TIMEOUT_MS)
            fail_receive(room,"The viewing connection timed out.");
        else if(r->state==NativeRoomWatch_Requesting && now-r->control_ms>=WATCH_CONTROL_RETRY_MS) {
            control(room,r->peer_id,r->serial,r->session,Watch_Request);r->control_ms=now;
        } else if(state==NativeContentChannel_Connected) {
            if(now-r->ack_ms>=WATCH_PUMP_MS) {
                progress(room,r->peer_id,r->serial,true,r->session,Watch_Ack,r->stream.count,r->metadata_mask,r->stream.digest);
                r->ack_ms=now;
            }
            if(r->status_received && r->remote_finished && r->stream.finished && r->stream.count==r->remote_count) {
                if(r->stream.digest!=r->remote_digest)fail_receive(room,"The received history does not match.");
                else {release_receive(room);r->state=NativeRoomWatch_Complete;}
            }
        }
    }
    if(!watch->sender_count)return;
    uint8_t description[NATIVE_MATCH_STREAM_DESCRIPTION_BYTES];
    size_t description_size=0;unsigned budget=WATCH_BURST, starts=1;
    for(unsigned visited=0;visited<(NATIVE_SIGNALING_MAX_PARTICIPANTS-1)*WATCH_BURST && budget;++visited) {
        unsigned index=watch->cursor++%(NATIVE_SIGNALING_MAX_PARTICIPANTS-1);
        NativeRoomWatchSend* s=&watch->sends[index];if(!s->serial)continue;
        NativeMatchStream* stream=&room->stream;
        NativeContentChannelState state=nativeNetplayConnection_ContentState(&room->connection,s->peer_id,s->serial,false);
        if(!stream->records || stream->description.match.session_id!=s->session || stream->unavailable ||
           !member(room,s->peer_id) || state==NativeContentChannel_Failed || now-s->last_ack_ms>=WATCH_TIMEOUT_MS) {
            stop_send(room,s,true);continue;
        }
        if(state==NativeContentChannel_Reserved) {
            if(!starts)continue;
            --starts;
            if(!nativeNetplayConnection_StartContent(&room->connection,s->peer_id,s->serial,false))
                stop_send(room,s,true);
            continue;
        }
        if(state!=NativeContentChannel_Connected)continue;
        if(!description_size)description_size=nativeMatchStream_EncodeDescription(&stream->description,description,sizeof(description));
        if(!description_size) {stop_send(room,s,true);continue;}
        uint8_t needed=metadata_bits(description_size)&~s->metadata_mask;
        if(needed) {
            unsigned offset=(needed&1) ? 0 : WATCH_METADATA_CHUNK;
            size_t bytes=description_size-offset;if(bytes>WATCH_METADATA_CHUNK)bytes=WATCH_METADATA_CHUNK;
            uint8_t packet[WATCH_METADATA_HEADER+WATCH_METADATA_CHUNK]={0};
            memcpy(packet,"FMWV",4);packet[4]=Watch_Metadata;put(packet+5,s->session,8);
            put(packet+13,description_size,2);put(packet+15,offset,2);
            memcpy(packet+WATCH_METADATA_HEADER,description+offset,bytes);
            nativeNetplayConnection_SendContent(&room->connection,s->peer_id,s->serial,false,packet,WATCH_METADATA_HEADER+bytes);
            --budget;continue;
        }
        if(now-s->status_ms>=WATCH_RETRY_MS) {
            progress(room,s->peer_id,s->serial,false,s->session,Watch_Status,stream->count,
                     (stream->finished ? 1 : 0)|(stream->unavailable ? 2 : 0),stream->digest);
            s->status_ms=now;--budget;continue;
        }
        if(s->sent>s->acknowledged && now-s->retry_ms>=WATCH_RETRY_MS) {
            s->sent=s->acknowledged;s->retry_ms=now;
        }
        if(s->sent>=stream->count || s->sent-s->acknowledged>=WATCH_SEND_WINDOW)continue;
        uint8_t packet[NATIVE_MATCH_STREAM_PACKET_BYTES];
        size_t bytes=nativeMatchStream_EncodeRecords(stream,s->sent,packet,sizeof(packet));
        if(!bytes) {stop_send(room,s,true);continue;}
        if(nativeNetplayConnection_SendContent(&room->connection,s->peer_id,s->serial,false,packet,bytes)) {
            if(s->sent==s->acknowledged)s->retry_ms=now;
            s->sent+=packet[16];
        }
        --budget;
    }
}
#endif
