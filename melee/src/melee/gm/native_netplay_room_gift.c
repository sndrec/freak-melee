#include "native_netplay_room_gift.h"

#ifdef MELEE_NATIVE

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <melee/ft/native_costume_package.h>
#include <melee/ft/native_fighter_runtime.h>

#include "native_netplay_room.h"

#include <stdlib.h>
#include <string.h>

enum {
    GIFT_OFFER_BYTES = 106,
    GIFT_REPLY_BYTES = 41,
    GIFT_RETRY_MS = 250,
    GIFT_SEND_BURST = 8,
    GIFT_RECEIVE_LIMIT = 64,
};

static const uint8_t gift_magic[4] = { 'F', 'M', 'G', '1' };
enum { GiftPacket_Offer = 1, GiftPacket_Accept = 2,
       GiftPacket_Decline = 3, GiftPacket_Installed = 4 };

static bool same_id(const NativeContentId* left, const NativeContentId* right)
{
    return memcmp(left, right, sizeof(*left)) == 0;
}

static void write_u32(uint8_t* bytes, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) {
        bytes[i] = (uint8_t) (value >> (i * 8));
    }
}

static uint32_t read_u32(const uint8_t* bytes)
{
    return (uint32_t) bytes[0] | (uint32_t) bytes[1] << 8 |
           (uint32_t) bytes[2] << 16 | (uint32_t) bytes[3] << 24;
}

static bool has_id(const NativeContentId* id)
{
    const NativeContentId empty = { 0 };
    return !same_id(id, &empty);
}

static const char* content_root(const NativeNetplayRoom* room, char path[512])
{
    if (room->content_root != NULL) {
        return room->content_root;
    }
    return nativeContentStore_DefaultRoot(path, 512) ? path : NULL;
}

static bool connected_peer(const NativeNetplayRoom* room, const char* peer_id)
{
    for (unsigned i = 0; i < NATIVE_NETPLAY_CONNECTION_MAX_REMOTE_PEERS; ++i) {
        const NativeNetplayConnectionPeer* peer = &room->connection.peers[i];
        if (!peer->active || strcmp(peer->peer_id, peer_id) != 0) {
            continue;
        }
        NativeNetplayIceState state = nativeNetplayIce_State(&peer->ice);
        return state == NativeNetplayIceState_Connected ||
               state == NativeNetplayIceState_Completed;
    }
    return false;
}

static size_t control_packet(const NativeNetplayGift* gift, uint8_t type,
                             uint8_t output[GIFT_OFFER_BYTES])
{
    memcpy(output, gift_magic, sizeof(gift_magic));
    output[4] = type;
    memcpy(output + 5, gift->id.bytes, NATIVE_CONTENT_ID_BYTES);
    write_u32(output + 37, gift->serial);
    if (type == GiftPacket_Offer) {
        output[41] = gift->kind;
        memcpy(output + 42, gift->title, NATIVE_CONTENT_TITLE_BYTES);
        return GIFT_OFFER_BYTES;
    }
    return GIFT_REPLY_BYTES;
}

static void send_reply(NativeNetplayRoom* room,
                       const NativeNetplayGift* gift, uint8_t type)
{
    uint8_t packet[GIFT_OFFER_BYTES];
    size_t size = control_packet(gift, type, packet);
    nativeNetplayConnection_Send(&room->connection, gift->peer_id,
                                 packet, size);
}

static bool install_cached(NativeNetplayRoom* room);

void nativeNetplayRoom_ClearGifts(NativeNetplayRoom* room)
{
    if (room == NULL) {
        return;
    }
    free(room->outgoing_gift.bytes);
    nativeContentTransfer_StopReceive(&room->incoming_gift.receive);
    room->outgoing_gift = (NativeNetplayGift){ 0 };
    room->incoming_gift = (NativeNetplayGift){ 0 };
}

bool nativeNetplayRoom_OfferGift(NativeNetplayRoom* room,
                                 const char* peer_id,
                                 const NativeContentId* id)
{
    if (room == NULL || !room->active || room->match_configured ||
        peer_id == NULL || id == NULL || !has_id(id) ||
        !connected_peer(room, peer_id) ||
        (room->outgoing_gift.state != NativeNetplayGift_None &&
         room->outgoing_gift.state != NativeNetplayGift_Sent &&
         room->outgoing_gift.state != NativeNetplayGift_Declined &&
         room->outgoing_gift.state != NativeNetplayGift_Failed)) {
        return false;
    }
    char default_path[512];
    const char* root = content_root(room, default_path);
    uint8_t* bytes = NULL;
    size_t size = 0;
    if (root == NULL || !nativeContentStore_IsInstalled(root, id) ||
        !nativeContentStore_Read(root, id, &bytes, &size)) {
        return false;
    }
    NativeNetplayGift next = { 0 };
    NativeCostumePackage costume;
    NativeFighterPackage fighter;
    NativeFighterPackageData fighter_data;
    const uint8_t* costume_dat;
    size_t costume_size;
    if (nativeContentStore_ParseCostume(bytes, size, &costume,
                                        &costume_dat, &costume_size)) {
        next.kind = NativeNetplayGift_Costume;
        memcpy(next.title, costume.title, sizeof(next.title));
    } else if (nativeContentStore_ParseFighter(bytes, size, &fighter,
                                               &fighter_data)) {
        next.kind = NativeNetplayGift_Fighter;
        memcpy(next.title, fighter.title, sizeof(next.title));
    } else {
        free(bytes);
        return false;
    }
    if (memchr(next.title, '\0', sizeof(next.title)) == NULL) {
        free(bytes);
        return false;
    }
    next.id = *id;
    next.serial = ++room->gift_serial;
    if (next.serial == 0) {
        next.serial = ++room->gift_serial;
    }
    memcpy(next.peer_id, peer_id, strlen(peer_id) + 1);
    next.bytes = bytes;
    next.size = size;
    next.state = NativeNetplayGift_Offered;
    free(room->outgoing_gift.bytes);
    room->outgoing_gift = next;
    return true;
}

bool nativeNetplayRoom_AcceptGift(NativeNetplayRoom* room)
{
    if (room == NULL || room->match_configured ||
        room->incoming_gift.state != NativeNetplayGift_Offered) {
        return false;
    }
    NativeNetplayGift* gift = &room->incoming_gift;
    char default_path[512];
    const char* root = content_root(room, default_path);
    if (root == NULL) {
        gift->state = NativeNetplayGift_Failed;
        return false;
    }
    if (nativeContentStore_IsCached(root, &gift->id)) {
        bool installed = install_cached(room);
        gift->state = installed ? NativeNetplayGift_Installed
                                : NativeNetplayGift_Failed;
        if (installed) {
            send_reply(room, gift, GiftPacket_Installed);
        }
        return installed;
    }
    nativeContentTransfer_StartReceive(&gift->receive, &gift->id);
    gift->state = NativeNetplayGift_Receiving;
    gift->last_control_ms = 0;
    send_reply(room, gift, GiftPacket_Accept);
    return true;
}

void nativeNetplayRoom_DeclineGift(NativeNetplayRoom* room)
{
    if (room != NULL && !room->match_configured &&
        room->incoming_gift.state == NativeNetplayGift_Offered) {
        room->incoming_gift.state = NativeNetplayGift_Declined;
        room->incoming_gift.last_control_ms = 0;
        send_reply(room, &room->incoming_gift, GiftPacket_Decline);
    }
}

bool nativeNetplayRoom_GiftStatus(const NativeNetplayRoom* room,
                                  bool incoming,
                                  NativeNetplayGiftStatus* status)
{
    if (room == NULL || status == NULL) {
        return false;
    }
    const NativeNetplayGift* gift = incoming ? &room->incoming_gift
                                             : &room->outgoing_gift;
    if (gift->state == NativeNetplayGift_None) {
        return false;
    }
    *status = (NativeNetplayGiftStatus){ 0 };
    status->id = gift->id;
    status->kind = gift->kind;
    status->state = gift->state;
    memcpy(status->peer_id, gift->peer_id, sizeof(status->peer_id));
    memcpy(status->title, gift->title, sizeof(status->title));
    if (incoming && gift->state == NativeNetplayGift_Receiving) {
        status->bytes_done = (uint32_t) gift->receive.received;
        status->bytes_total = (uint32_t) gift->receive.size;
    } else if (!incoming && gift->state == NativeNetplayGift_Sending) {
        status->bytes_done = (uint32_t) gift->send.acknowledged;
        status->bytes_total = (uint32_t) gift->send.size;
    }
    return true;
}

static void receive_offer(NativeNetplayRoom* room,
                          const NativeNetplayConnectionDatagram* datagram)
{
    const uint8_t* packet = datagram->datagram.bytes;
    NativeContentId id;
    memcpy(id.bytes, packet + 5, sizeof(id.bytes));
    uint32_t serial = read_u32(packet + 37);
    if (!has_id(&id) || serial == 0 ||
        (packet[41] != NativeNetplayGift_Costume &&
         packet[41] != NativeNetplayGift_Fighter) ||
        memchr(packet + 42, '\0', NATIVE_CONTENT_TITLE_BYTES) == NULL) {
        return;
    }
    NativeNetplayGift* gift = &room->incoming_gift;
    if (gift->state != NativeNetplayGift_None &&
        gift->state != NativeNetplayGift_Installed &&
        gift->state != NativeNetplayGift_Failed &&
        gift->state != NativeNetplayGift_Declined &&
        !(same_id(&gift->id, &id) && gift->serial == serial &&
          strcmp(gift->peer_id, datagram->peer_id) == 0)) {
        return;
    }
    if (same_id(&gift->id, &id) && gift->serial == serial &&
        strcmp(gift->peer_id, datagram->peer_id) == 0) {
        if (gift->state == NativeNetplayGift_Receiving) {
            send_reply(room, gift, GiftPacket_Accept);
        } else if (gift->state == NativeNetplayGift_Declined) {
            send_reply(room, gift, GiftPacket_Decline);
        } else if (gift->state == NativeNetplayGift_Installed) {
            send_reply(room, gift, GiftPacket_Installed);
        }
        return;
    }
    nativeContentTransfer_StopReceive(&gift->receive);
    *gift = (NativeNetplayGift){ 0 };
    gift->id = id;
    gift->serial = serial;
    gift->kind = packet[41];
    memcpy(gift->title, packet + 42, sizeof(gift->title));
    memcpy(gift->peer_id, datagram->peer_id,
           strlen(datagram->peer_id) + 1);
    gift->state = NativeNetplayGift_Offered;
}

static void receive_reply(NativeNetplayRoom* room,
                          const NativeNetplayConnectionDatagram* datagram,
                          uint8_t type)
{
    NativeNetplayGift* gift = &room->outgoing_gift;
    if ((gift->state != NativeNetplayGift_Offered &&
         gift->state != NativeNetplayGift_Sending) ||
        strcmp(gift->peer_id, datagram->peer_id) != 0 ||
        memcmp(gift->id.bytes, datagram->datagram.bytes + 5,
               NATIVE_CONTENT_ID_BYTES) != 0 ||
        gift->serial != read_u32(datagram->datagram.bytes + 37)) {
        return;
    }
    if (type == GiftPacket_Decline || type == GiftPacket_Installed) {
        gift->state = type == GiftPacket_Decline ? NativeNetplayGift_Declined
                                                 : NativeNetplayGift_Sent;
        free(gift->bytes);
        gift->bytes = NULL;
        return;
    }
    if (gift->state == NativeNetplayGift_Sending) {
        return;
    }
    char default_path[512];
    const char* root = content_root(room, default_path);
    if (root == NULL ||
        !nativeContentStore_IsInstalled(root, &gift->id) ||
        !nativeContentTransfer_StartSend(&gift->send, &gift->id,
                                          gift->bytes, gift->size)) {
        gift->state = NativeNetplayGift_Failed;
        return;
    }
    gift->state = NativeNetplayGift_Sending;
}

static bool install_cached(NativeNetplayRoom* room)
{
    NativeNetplayGift* gift = &room->incoming_gift;
    char default_path[512];
    const char* root = content_root(room, default_path);
    uint8_t* bytes = NULL;
    size_t size = 0;
    if (root == NULL ||
        !nativeContentStore_Read(root, &gift->id, &bytes, &size)) {
        return false;
    }
    bool valid = false;
    if (gift->kind == NativeNetplayGift_Costume) {
        NativeCostumePackage costume;
        const uint8_t* dat;
        size_t dat_size;
        valid = nativeContentStore_ParseCostume(bytes, size, &costume,
                                                &dat, &dat_size) &&
                nativeCostumePackage_Validate(&costume, dat, dat_size,
                                              root, NULL);
    } else if (gift->kind == NativeNetplayGift_Fighter) {
        NativeFighterPackage fighter;
        NativeFighterPackageData data;
        valid = nativeContentStore_ParseFighter(bytes, size, &fighter,
                                                &data) &&
                nativeFighterRuntime_ValidateCached(&gift->id, root);
    }
    free(bytes);
    return valid && nativeContentStore_Install(root, &gift->id);
}

static bool accept_download(NativeNetplayRoom* room)
{
    NativeNetplayGift* gift = &room->incoming_gift;
    char default_path[512];
    const char* root = content_root(room, default_path);
    NativeContentId cached;
    return root != NULL &&
           nativeContentStore_Cache(root, gift->receive.bytes,
                                    gift->receive.size, &gift->id, &cached) &&
           same_id(&cached, &gift->id) && install_cached(room);
}

bool nativeNetplayRoom_PumpGifts(NativeNetplayRoom* room)
{
    if (room == NULL || !room->active || room->match_configured) {
        return false;
    }
    const uint64_t now = GetTickCount64();
    for (unsigned received = 0; received < GIFT_RECEIVE_LIMIT; ++received) {
        NativeNetplayConnectionDatagram datagram;
        if (!nativeNetplayConnection_PollDatagram(&room->connection,
                                                  &datagram)) {
            break;
        }
        const uint8_t* packet = datagram.datagram.bytes;
        const size_t size = datagram.datagram.size;
        if (size >= GIFT_REPLY_BYTES &&
            memcmp(packet, gift_magic, sizeof(gift_magic)) == 0) {
            if (packet[4] == GiftPacket_Offer && size == GIFT_OFFER_BYTES) {
                receive_offer(room, &datagram);
            } else if ((packet[4] == GiftPacket_Accept ||
                        packet[4] == GiftPacket_Decline ||
                        packet[4] == GiftPacket_Installed) &&
                       size == GIFT_REPLY_BYTES) {
                receive_reply(room, &datagram, packet[4]);
            }
            continue;
        }
        NativeContentId id;
        NativeContentTransferPacketType type =
            nativeContentTransfer_PacketType(packet, size, &id);
        if (type == NativeContentTransferPacket_Ack &&
            room->outgoing_gift.state == NativeNetplayGift_Sending &&
            strcmp(room->outgoing_gift.peer_id, datagram.peer_id) == 0 &&
            same_id(&room->outgoing_gift.id, &id)) {
            nativeContentTransfer_SendPacket(&room->outgoing_gift.send,
                                              packet, size, now);
        } else if ((type == NativeContentTransferPacket_Begin ||
                    type == NativeContentTransferPacket_Data ||
                    type == NativeContentTransferPacket_Missing) &&
                   room->incoming_gift.state == NativeNetplayGift_Receiving &&
                   strcmp(room->incoming_gift.peer_id, datagram.peer_id) == 0 &&
                   same_id(&room->incoming_gift.id, &id)) {
            nativeContentTransfer_ReceivePacket(&room->incoming_gift.receive,
                                                 packet, size, now);
        }
    }
    NativeNetplayGift* outgoing = &room->outgoing_gift;
    if (outgoing->state == NativeNetplayGift_Offered &&
        connected_peer(room, outgoing->peer_id) &&
        (outgoing->last_control_ms == 0 ||
         now - outgoing->last_control_ms >= GIFT_RETRY_MS)) {
        uint8_t packet[GIFT_OFFER_BYTES];
        size_t size = control_packet(outgoing, GiftPacket_Offer, packet);
        nativeNetplayConnection_Send(&room->connection, outgoing->peer_id,
                                     packet, size);
        outgoing->last_control_ms = now;
    } else if (outgoing->state == NativeNetplayGift_Sending) {
        uint8_t packet[NATIVE_CONTENT_TRANSFER_PACKET_CAPACITY];
        for (unsigned burst = 0; burst < GIFT_SEND_BURST; ++burst) {
            size_t size = nativeContentTransfer_NextSend(&outgoing->send,
                                                          now, packet);
            if (size == 0) {
                break;
            }
            nativeNetplayConnection_Send(&room->connection, outgoing->peer_id,
                                         packet, size);
        }
        if (outgoing->send.complete) {
            free(outgoing->bytes);
            outgoing->bytes = NULL;
            outgoing->state = NativeNetplayGift_Sent;
        }
    }
    NativeNetplayGift* incoming = &room->incoming_gift;
    if (incoming->state == NativeNetplayGift_Receiving &&
        connected_peer(room, incoming->peer_id) &&
        (incoming->last_control_ms == 0 ||
         now - incoming->last_control_ms >= GIFT_RETRY_MS)) {
        uint8_t packet[GIFT_OFFER_BYTES];
        size_t size = control_packet(incoming, GiftPacket_Accept, packet);
        nativeNetplayConnection_Send(&room->connection, incoming->peer_id,
                                     packet, size);
        incoming->last_control_ms = now;
    }
    if (incoming->state == NativeNetplayGift_Receiving) {
        uint8_t packet[NATIVE_CONTENT_TRANSFER_PACKET_CAPACITY];
        size_t size = nativeContentTransfer_NextReceive(&incoming->receive,
                                                        now, packet);
        if (size != 0) {
            nativeNetplayConnection_Send(&room->connection, incoming->peer_id,
                                         packet, size);
        }
        if (incoming->receive.failed) {
            incoming->state = NativeNetplayGift_Failed;
        } else if (incoming->receive.complete) {
            bool installed = accept_download(room);
            nativeContentTransfer_StopReceive(&incoming->receive);
            incoming->state = installed ? NativeNetplayGift_Installed
                                        : NativeNetplayGift_Failed;
            if (installed) {
                send_reply(room, incoming, GiftPacket_Installed);
            }
        }
    }
    return true;
}

#endif
