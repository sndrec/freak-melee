#include "native_lobby_selection.h"
#include "native_netplay_room.h"
#ifdef MELEE_NATIVE
#define WIN32_LEAN_AND_MEAN
#include <stdlib.h>
#include <string.h>
#include <windows.h>

enum {
    CACHE_CONTROL_BYTES = 41,
    CACHE_RETRY_MS = 2000,
    CACHE_TIMEOUT_MS = 120000,
    CACHE_SEND_BUDGET = 64,
    CACHE_ROUTE_BURST = 8,
    CACHE_UPLOAD_PUMP_MS = 4
};
enum {
    Cache_Request = 1,
    Cache_Ready,
    Cache_Missing,
    Cache_Cancel,
    Cache_Done
};
static const uint8_t cache_magic[4] = { 'F', 'M', 'Q', '1' };

static bool same_id(const NativeContentId* a, const NativeContentId* b)
{
    return !memcmp(a, b, sizeof(*a));
}
static bool has_id(const NativeContentId* id)
{
    const NativeContentId zero = { 0 };
    return !same_id(id, &zero);
}
static const char* root_path(NativeNetplayRoom* room, char path[512])
{
    return room->content_root                          ? room->content_root
           : nativeContentStore_DefaultRoot(path, 512) ? path
                                                       : NULL;
}
static bool reply(NativeNetplayRoom* room, const char* peer,
                  const NativeContentId* id, uint32_t serial, uint8_t type)
{
    uint8_t bytes[CACHE_CONTROL_BYTES];
    memcpy(bytes, cache_magic, 4);
    bytes[4] = type;
    memcpy(bytes + 5, id->bytes, sizeof(id->bytes));
    for (unsigned i = 0; i < 4; ++i) {
        bytes[37 + i] = (uint8_t) (serial >> (i * 8));
    }
    return nativeNetplayConnection_SendControl(&room->connection, peer, bytes,
                                               sizeof(bytes));
}
static const NativeSignalingParticipant* participant(NativeNetplayRoom* room,
                                                     const char* peer)
{
    const NativeNetplayConnection* c = &room->connection;
    for (unsigned i = 0; c->has_presence && i < c->presence.participant_count;
         ++i)
    {
        const NativeSignalingParticipant* p = &c->presence.participants[i];
        if (!strcmp(p->peer_id, peer) && !strcmp(p->build, c->local_build)) {
            return p;
        }
    }
    return NULL;
}
static bool selected_by(NativeNetplayRoom* room, const char* peer,
                        const NativeContentId* id)
{
    const NativeSignalingParticipant* p = participant(room, peer);
    NativeLobbySelection selection;
    if (!p || !nativeLobbySelection_Decode(&selection, p->selection,
                                           p->selection_size))
    {
        return false;
    }
    for (unsigned i = 0; i < selection.player_count; ++i) {
        const NativeLobbyPlayerSelection* s = &selection.players[i];
        if ((p->active_mask & (1U << i)) && s->selected &&
            (same_id(id, &s->character_package_id) ||
             same_id(id, &s->costume_package_id)))
        {
            return true;
        }
    }
    return false;
}
static void stop_send(NativeNetplayRoom* room, NativeRoomCacheSend* send,
                      bool notify)
{
    if (!send->serial) {
        return;
    }
    if (notify) {
        reply(room, send->peer_id, &send->transfer.id, send->serial,
              Cache_Missing);
    }
    nativeNetplayConnection_ReleaseContent(&room->connection, send->peer_id,
                                           send->serial, false);
    if(send->owns_bytes)free(send->bytes);
    memset(send, 0, sizeof(*send));
}
static void stop_receive(NativeNetplayRoom* room, bool cancel, bool failed)
{
    NativeRoomCache* cache = &room->cache;
    if (!cache->serial) {
        return;
    }
    if (cancel) {
        reply(room, cache->peer_id, &cache->receive.id, cache->serial,
              Cache_Cancel);
    }
    for (unsigned i = 0; failed && i < cache->choice_count; ++i) {
        if (same_id(&cache->choices[i].id, &cache->receive.id)) {
            cache->choices[i].retry_after_ms =
                GetTickCount64() + CACHE_RETRY_MS;
        }
    }
    nativeNetplayConnection_ReleaseContent(&room->connection, cache->peer_id,
                                           cache->serial, true);
    nativeContentTransfer_StopReceive(&cache->receive);
    cache->serial = 0;
    cache->peer_id[0] = 0;
    cache->receive_cached = false;
}
void nativeNetplayRoom_ClearCacheTransfers(NativeNetplayRoom* room)
{
    stop_receive(room, true, false);
    for (unsigned i = 0; i < NATIVE_ROOM_CACHE_SENDS; ++i) {
        stop_send(room, &room->cache.sends[i], true);
    }
    room->cache.choice_count = 0;
    room->cache.presence_revision = 0;
    room->cache.watch_serial = 0;
}

void nativeNetplayRoom_ClearRetainedPackages(NativeNetplayRoom* room)
{
    NativeRoomCache* cache=&room->cache;
    for(unsigned i=0;i<NATIVE_ROOM_CACHE_SENDS;++i)
        if(cache->sends[i].serial && !cache->sends[i].owns_bytes)
            stop_send(room,&cache->sends[i],true);
    for(unsigned i=0;i<cache->retained_count;++i)free(cache->retained[i].bytes);
    memset(cache->retained,0,sizeof(cache->retained));
    cache->retained_count=0;cache->retained_bytes=0;cache->retained_session=0;
}
static NativeRoomCachedPackage* retained(NativeNetplayRoom* room,const NativeContentId* id)
{
    for(unsigned i=0;i<room->cache.retained_count;++i)
        if(same_id(id,&room->cache.retained[i].id))return &room->cache.retained[i];
    return NULL;
}
bool nativeNetplayRoom_RetainMatchPackages(NativeNetplayRoom* room)
{
    if(!room || !room->match_configured)return false;
    nativeNetplayRoom_ClearRetainedPackages(room);
    NativeRoomCache* cache=&room->cache;
    uint8_t ports=0;
    for(unsigned p=0;p<room->match.peer_count;++p)
        if(!strcmp(room->match.peers[p].peer_id,room->connection.local_peer_id))
            ports=room->match.peers[p].port_mask;
    char path[512];const char* root=root_path(room,path);
    if(!ports || !root)return false;
    for(unsigned p=0;p<4;++p) {
        const NativeLobbyMatchSlot* slot=&room->match.slots[p];
        if(!(ports&(1u<<p)) || slot->kind==NativeLobbyMatchSlot_Empty)continue;
        const NativeContentId* ids[]={&slot->character_package_id,&slot->costume_package_id};
        for(unsigned k=0;k<2;++k) {
            if(!has_id(ids[k]) || retained(room,ids[k]))continue;
            NativeRoomCachedPackage* package=&cache->retained[cache->retained_count];
            ++cache->file_reads;
            if(!nativeContentStore_Read(root,ids[k],&package->bytes,&package->size)) {
                nativeNetplayRoom_ClearRetainedPackages(room);return false;
            }
            package->id=*ids[k];cache->retained_bytes+=package->size;++cache->retained_count;
        }
    }
    cache->retained_session=room->match.session_id;
    return true;
}

static void add_choice(NativeNetplayRoom* room,NativeRoomCacheChoice desired[8],
                        unsigned* count,const NativeContentId* id,const char* peer,const char* root)
{
    if(!has_id(id) || *count==NATIVE_ROOM_CACHE_CHOICES ||
       !strcmp(peer,room->connection.local_peer_id))return;
    for(unsigned n=0;n<*count;++n)if(same_id(&desired[n].id,id))return;
    NativeRoomCacheChoice* choice=&desired[(*count)++];choice->id=*id;
    strcpy(choice->peer_id,peer);
    NativeRoomCache* cache=&room->cache;
    for(unsigned n=0;n<cache->choice_count;++n) {
        const NativeRoomCacheChoice* previous=&cache->choices[n];
        if(same_id(id,&previous->id)) {
            choice->cached=previous->cached;
            if(!strcmp(peer,previous->peer_id))choice->retry_after_ms=previous->retry_after_ms;
            return;
        }
    }
    choice->cached=root && nativeContentStore_IsCached(root,id);
}

static void reconcile(NativeNetplayRoom* room)
{
    NativeRoomCache* cache = &room->cache;
    const NativeNetplayConnection* c = &room->connection;
    const NativeRoomWatchReceive* watch=&room->watch.receive;
    uint32_t watch_serial=watch->state==NativeRoomWatch_Requesting ||
        watch->state==NativeRoomWatch_Receiving || watch->state==NativeRoomWatch_Complete
        ? watch->serial : 0;
    if (!c->has_presence || (cache->presence_revision == c->presence.revision &&
                            cache->watch_serial==watch_serial)) {
        return;
    }
    NativeRoomCacheChoice desired[NATIVE_ROOM_CACHE_CHOICES] = { 0 };
    unsigned count = 0;
    char path[512];
    const char* root = root_path(room, path);
    if(watch_serial) {
        const NativeLobbyMatch* match=&watch->stream.description.match;
        for(unsigned p=0;p<4;++p) {
            if(match->slots[p].kind==NativeLobbyMatchSlot_Empty)continue;
            const char* owner=NULL;
            for(unsigned n=0;n<match->peer_count;++n)
                if(match->peers[n].port_mask&(1u<<p))owner=match->peers[n].peer_id;
            if(!owner)continue;
            add_choice(room,desired,&count,&match->slots[p].character_package_id,owner,root);
            add_choice(room,desired,&count,&match->slots[p].costume_package_id,owner,root);
        }
    }
    for (unsigned i = 0; i < c->presence.participant_count; ++i) {
        const NativeSignalingParticipant* p = &c->presence.participants[i];
        NativeLobbySelection selection;
        if (!p->active_mask || !strcmp(p->peer_id, c->local_peer_id) ||
            strcmp(p->build, c->local_build) ||
            !nativeLobbySelection_Decode(&selection, p->selection,
                                         p->selection_size))
        {
            continue;
        }
        for (unsigned seat = 0; seat < selection.player_count; ++seat) {
            const NativeLobbyPlayerSelection* s = &selection.players[seat];
            if (!(p->active_mask & (1U << seat)) || !s->selected) {
                continue;
            }
            const NativeContentId* ids[] = { &s->character_package_id,
                                             &s->costume_package_id };
            for (unsigned kind = 0;
                 kind < 2 && count < NATIVE_ROOM_CACHE_CHOICES; ++kind)
            {
                add_choice(room,desired,&count,ids[kind],p->peer_id,root);
            }
        }
    }
    memcpy(cache->choices, desired, sizeof(desired));
    cache->choice_count = (uint8_t) count;
    cache->presence_revision = c->presence.revision;
    cache->watch_serial=watch_serial;
    bool wanted = false;
    for (unsigned i = 0; cache->serial && i < count; ++i) {
        wanted |= same_id(&desired[i].id, &cache->receive.id) &&
                  !strcmp(desired[i].peer_id, cache->peer_id);
    }
    if (cache->serial && !wanted) {
        stop_receive(room, true, false);
    }
}

void nativeNetplayRoom_CacheControl(NativeNetplayRoom* room,
                                    const NativeContentControl* message)
{
    const uint8_t* bytes = message->datagram.bytes;
    if (message->datagram.size != CACHE_CONTROL_BYTES ||
        memcmp(bytes, cache_magic, 4) ||
        !participant(room, message->peer_id))
    {
        return;
    }
    NativeContentId id;
    memcpy(id.bytes, bytes + 5, sizeof(id.bytes));
    uint32_t serial = (uint32_t) bytes[37] | (uint32_t) bytes[38] << 8 |
                      (uint32_t) bytes[39] << 16 | (uint32_t) bytes[40] << 24;
    if (!serial || !has_id(&id)) {
        return;
    }
    NativeRoomCache* cache = &room->cache;
    if (bytes[4] == Cache_Request) {
        NativeRoomCacheSend* available = NULL;
        for (unsigned i = 0; i < NATIVE_ROOM_CACHE_SENDS; ++i) {
            NativeRoomCacheSend* send = &cache->sends[i];
            if (send->serial == serial &&
                !strcmp(send->peer_id, message->peer_id))
            {
                if (same_id(&send->transfer.id, &id)) {
                    reply(room, message->peer_id, &id, serial, Cache_Ready);
                }
                return;
            }
            if (!send->serial && !available) {
                available = send;
            }
        }
        NativeRoomCachedPackage* package=retained(room,&id);
        if (!available || (!package &&
            (room->match_configured || !selected_by(room, room->connection.local_peer_id, &id))))
        {
            reply(room, message->peer_id, &id, serial, Cache_Missing);
            return;
        }
        uint8_t* data = package ? package->bytes : NULL;
        size_t size = package ? package->size : 0;
        bool owned=!package;
        if(owned) {
            char path[512];const char* root=root_path(room,path);
            ++cache->file_reads;
            if(!root || !nativeContentStore_IsInstalled(root,&id) ||
               !nativeContentStore_Read(root,&id,&data,&size)) {
                reply(room,message->peer_id,&id,serial,Cache_Missing);return;
            }
        }
        NativeContentTransferSend transfer;
        if (!nativeContentTransfer_StartSend(&transfer, &id, data, size) ||
            !nativeNetplayConnection_ReserveContent(
                &room->connection, message->peer_id, serial, false))
        {
            if(owned)free(data);
            reply(room, message->peer_id, &id, serial, Cache_Missing);
            return;
        }
        available->transfer = transfer;
        available->bytes = data;
        available->owns_bytes=owned;
        available->serial = serial;
        available->last_progress_ms = GetTickCount64();
        strcpy(available->peer_id, message->peer_id);
        if(room->match_configured)++cache->active_requests;
        reply(room, message->peer_id, &id, serial, Cache_Ready);
    } else if ((bytes[4] == Cache_Ready || bytes[4] == Cache_Missing) &&
               cache->serial == serial && same_id(&cache->receive.id, &id) &&
               !strcmp(cache->peer_id, message->peer_id))
    {
        if (bytes[4] == Cache_Missing ||
            !nativeNetplayConnection_StartContent(
                &room->connection, cache->peer_id, serial, true))
        {
            stop_receive(room, true, true);
        }
    } else if (bytes[4] == Cache_Done || bytes[4] == Cache_Cancel) {
        for (unsigned i = 0; i < NATIVE_ROOM_CACHE_SENDS; ++i) {
            NativeRoomCacheSend* send = &cache->sends[i];
            if (send->serial == serial && same_id(&send->transfer.id, &id) &&
                !strcmp(send->peer_id, message->peer_id))
            {
                stop_send(room, send, false);
            }
        }
    }
}

void nativeNetplayRoom_CachePacket(NativeNetplayRoom* room,
                                   const NativeContentDatagram* message)
{
    NativeContentId id = { 0 };
    const uint8_t* packet = message->message.datagram.bytes;
    size_t size = message->message.datagram.size;
    NativeContentTransferPacketType type =
        nativeContentTransfer_PacketType(packet, size, &id);
    if (type == NativeContentTransferPacket_Invalid ||
        type == NativeContentTransferPacket_Other)
    {
        return;
    }
    NativeRoomCache* cache = &room->cache;
    uint64_t now = GetTickCount64();
    if (message->local_owner && message->serial == cache->serial &&
        same_id(&id, &cache->receive.id) &&
        !strcmp(message->message.peer_id, cache->peer_id))
    {
        nativeContentTransfer_ReceivePacket(&cache->receive, packet, size,
                                            now);
    } else if (!message->local_owner &&
               type == NativeContentTransferPacket_Ack)
    {
        for (unsigned i = 0; i < NATIVE_ROOM_CACHE_SENDS; ++i) {
            NativeRoomCacheSend* send = &cache->sends[i];
            if (send->serial == message->serial &&
                same_id(&send->transfer.id, &id) &&
                !strcmp(send->peer_id, message->message.peer_id))
            {
                size_t acknowledged = send->transfer.acknowledged;
                nativeContentTransfer_SendPacket(&send->transfer, packet, size,
                                                 now);
                if (send->transfer.acknowledged > acknowledged) {
                    send->last_progress_ms = now;
                }
            }
        }
    }
}

static void pump_uploads(NativeNetplayRoom* room,uint64_t now)
{
    NativeRoomCache* cache=&room->cache;
    if(now-cache->upload_ms<CACHE_UPLOAD_PUMP_MS)return;
    cache->upload_ms=now;unsigned starts=1, active=0;
    for (unsigned i = 0; i < NATIVE_ROOM_CACHE_SENDS; ++i) {
        NativeRoomCacheSend* send = &cache->sends[i];
        if (!send->serial) {
            continue;
        }
        NativeContentChannelState state = nativeNetplayConnection_ContentState(
            &room->connection, send->peer_id, send->serial, false);
        if (state == NativeContentChannel_None ||
            state == NativeContentChannel_Failed ||
            now - send->last_progress_ms > CACHE_TIMEOUT_MS ||
            (!retained(room,&send->transfer.id) &&
             !selected_by(room,room->connection.local_peer_id,&send->transfer.id)))
        {
            stop_send(room, send, true);
            continue;
        }
        if(state==NativeContentChannel_Reserved && starts) {
            --starts;
            if(!nativeNetplayConnection_StartContent(&room->connection,send->peer_id,send->serial,false))
                stop_send(room,send,true);
        }
        active+=send->serial!=0;
    }
    if(!active)return;
    unsigned budget=CACHE_SEND_BUDGET;
    unsigned bursts[NATIVE_ROOM_CACHE_SENDS]={0};
    for(unsigned visited=0;visited<NATIVE_ROOM_CACHE_SENDS*CACHE_SEND_BUDGET && budget;++visited) {
        unsigned index=cache->upload_cursor++%NATIVE_ROOM_CACHE_SENDS;
        if(bursts[index]>=CACHE_ROUTE_BURST)continue;
        NativeRoomCacheSend* send=&cache->sends[index];
        if(!send->serial || nativeNetplayConnection_ContentState(&room->connection,send->peer_id,send->serial,false)!=NativeContentChannel_Connected)continue;
        uint8_t packet[NATIVE_CONTENT_TRANSFER_PACKET_CAPACITY];
        size_t size=nativeContentTransfer_NextSend(&send->transfer,now,packet);
        if(!size)continue;
        nativeNetplayConnection_SendContent(&room->connection,send->peer_id,send->serial,false,packet,size);
        --budget;++bursts[index];if(room->match_configured)++cache->active_packets;
    }
}

bool nativeNetplayRoom_WatchContentReady(const NativeNetplayRoom* room)
{
    const NativeRoomWatchReceive* r=&room->watch.receive;
    if(!r->stream.records || (r->state!=NativeRoomWatch_Receiving && r->state!=NativeRoomWatch_Complete))return false;
    for(unsigned p=0;p<4;++p) {
        const NativeLobbyMatchSlot* slot=&r->stream.description.match.slots[p];
        if(slot->kind==NativeLobbyMatchSlot_Empty)continue;
        const NativeContentId* ids[]={&slot->character_package_id,&slot->costume_package_id};
        for(unsigned k=0;k<2;++k) {
            if(!has_id(ids[k]))continue;
            bool found=false;
            for(unsigned i=0;i<room->cache.choice_count;++i)
                found|=room->cache.choices[i].cached && same_id(ids[k],&room->cache.choices[i].id);
            if(!found)return false;
        }
    }
    return true;
}

void nativeNetplayRoom_PumpCache(NativeNetplayRoom* room)
{
    if(!room->active)return;
    uint64_t now=GetTickCount64();pump_uploads(room,now);
    if(room->match_configured)return;
    reconcile(room);
    NativeRoomCache* cache=&room->cache;
    if (cache->serial) {
        NativeContentChannelState state = nativeNetplayConnection_ContentState(
            &room->connection, cache->peer_id, cache->serial, true);
        if (state == NativeContentChannel_None ||
            state == NativeContentChannel_Failed || cache->receive.failed ||
            now - (cache->receive.begun ? cache->receive.last_progress_ms
                                        : cache->started_ms) >
                CACHE_TIMEOUT_MS)
        {
            stop_receive(room, true, true);
        } else {
            if (!cache->receive.begun &&
                (!cache->last_request_ms ||
                 now - cache->last_request_ms >= 500) &&
                reply(room, cache->peer_id, &cache->receive.id, cache->serial,
                      Cache_Request))
            {
                cache->last_request_ms = now;
            }
            if (state == NativeContentChannel_Connected) {
                uint8_t packet[NATIVE_CONTENT_TRANSFER_PACKET_CAPACITY];
                size_t size = nativeContentTransfer_NextReceive(
                    &cache->receive, now, packet);
                if (size) {
                    nativeNetplayConnection_SendContent(
                        &room->connection, cache->peer_id, cache->serial, true,
                        packet, size);
                }
            }
            if (cache->receive.complete && !cache->receive_cached) {
                char path[512];
                const char* root = root_path(room, path);
                NativeContentId id;
                if (root && nativeContentStore_Cache(
                                root, cache->receive.bytes,
                                cache->receive.size, &cache->receive.id, &id))
                {
                    cache->receive_cached = true;
                    ++cache->revision;
                    for (unsigned i = 0; i < cache->choice_count; ++i) {
                        if (same_id(&cache->choices[i].id, &id)) {
                            cache->choices[i].cached = true;
                        }
                    }
                } else {
                    stop_receive(room, true, true);
                }
            }
            if (cache->serial && cache->receive_cached &&
                reply(room, cache->peer_id, &cache->receive.id, cache->serial,
                      Cache_Done))
            {
                stop_receive(room, false, false);
            }
        }
    }
    if (!cache->serial) {
        for (unsigned i = 0; i < cache->choice_count; ++i) {
            NativeRoomCacheChoice* choice = &cache->choices[i];
            if (choice->cached || now < choice->retry_after_ms) {
                continue;
            }
            // Explicit gift acceptance may have populated the cache meanwhile.
            char path[512];
            const char* root = root_path(room, path);
            if (root && nativeContentStore_IsCached(root, &choice->id)) {
                choice->cached = true;
                ++cache->revision;
                continue;
            }
            uint32_t serial = ++room->content_serial;
            if (!serial) {
                serial = ++room->content_serial;
            }
            if (!nativeNetplayConnection_ReserveContent(
                    &room->connection, choice->peer_id, serial, true))
            {
                break;
            }
            cache->serial = serial;
            strcpy(cache->peer_id, choice->peer_id);
            cache->started_ms = now;
            cache->last_request_ms = 0;
            cache->receive_cached = false;
            nativeContentTransfer_StartReceive(&cache->receive, &choice->id);
            break;
        }
    }
}
#endif
