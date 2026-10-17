#include "native_netplay_room_gift.h"

#ifdef MELEE_NATIVE

#define WIN32_LEAN_AND_MEAN
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "native_netplay_room.h"
#include <melee/ft/native_costume_package.h>
#include <melee/ft/native_fighter_runtime.h>

enum {
    GIFT_OFFER_BYTES = 106,
    GIFT_REPLY_BYTES = 41,
    GIFT_RETRY_MS = 250,
    GIFT_SEND_BURST = 8,
};

static const uint8_t gift_magic[4] = { 'F', 'M', 'G', '1' };
enum {
    GiftPacket_Offer = 1,
    GiftPacket_Accept = 2,
    GiftPacket_Decline = 3,
    GiftPacket_Installed = 4
};

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

static bool room_member(const NativeNetplayRoom* room, const char* peer_id)
{
    const NativeNetplayConnection* c = &room->connection;
    if (!c->has_presence || !peer_id || !strcmp(peer_id, c->local_peer_id)) {
        return false;
    }
    for (unsigned i = 0; i < c->presence.participant_count; ++i) {
        const NativeSignalingParticipant* peer = &c->presence.participants[i];
        if (!strcmp(peer->peer_id, peer_id) &&
            !strcmp(peer->build, c->local_build))
        {
            return true;
        }
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

static void send_reply(NativeNetplayRoom* room, const NativeNetplayGift* gift,
                       uint8_t type)
{
    uint8_t packet[GIFT_OFFER_BYTES];
    size_t size = control_packet(gift, type, packet);
    nativeNetplayConnection_SendControl(&room->connection, gift->peer_id,
                                        packet, size);
}

static bool install_cached(NativeNetplayRoom* room);

void nativeNetplayRoom_ClearGifts(NativeNetplayRoom* room)
{
    if (room == NULL) {
        return;
    }
    nativeNetplayConnection_ReleaseContent(&room->connection,
                                           room->outgoing_gift.peer_id,
                                           room->outgoing_gift.serial, true);
    nativeNetplayConnection_ReleaseContent(&room->connection,
                                           room->incoming_gift.peer_id,
                                           room->incoming_gift.serial, false);
    free(room->outgoing_gift.bytes);
    nativeContentTransfer_StopReceive(&room->incoming_gift.receive);
    room->outgoing_gift = (NativeNetplayGift){ 0 };
    room->incoming_gift = (NativeNetplayGift){ 0 };
}

bool nativeNetplayRoom_OfferGift(NativeNetplayRoom* room, const char* peer_id,
                                 const NativeContentId* id)
{
    if (room == NULL || !room->active || room->match_configured ||
        peer_id == NULL || id == NULL || !has_id(id) ||
        !room_member(room, peer_id) ||
        (room->outgoing_gift.state != NativeNetplayGift_None &&
         room->outgoing_gift.state != NativeNetplayGift_Sent &&
         room->outgoing_gift.state != NativeNetplayGift_Declined &&
         room->outgoing_gift.state != NativeNetplayGift_Failed))
    {
        return false;
    }
    char default_path[512];
    const char* root = content_root(room, default_path);
    uint8_t* bytes = NULL;
    size_t size = 0;
    if (root == NULL || !nativeContentStore_IsInstalled(root, id) ||
        !nativeContentStore_Read(root, id, &bytes, &size))
    {
        return false;
    }
    NativeNetplayGift next = { 0 };
    NativeCostumePackage costume;
    NativeFighterPackage fighter;
    NativeFighterPackageData fighter_data;
    const uint8_t* costume_dat;
    size_t costume_size;
    if (nativeContentStore_ParseCostume(bytes, size, &costume, &costume_dat,
                                        &costume_size))
    {
        next.kind = NativeNetplayGift_Costume;
        memcpy(next.title, costume.title, sizeof(next.title));
    } else if (nativeContentStore_ParseFighter(bytes, size, &fighter,
                                               &fighter_data))
    {
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
    next.serial = ++room->content_serial;
    if (next.serial == 0) {
        next.serial = ++room->content_serial;
    }
    memcpy(next.peer_id, peer_id, strlen(peer_id) + 1);
    next.bytes = bytes;
    next.size = size;
    next.state = NativeNetplayGift_Offered;
    nativeNetplayConnection_ReleaseContent(&room->connection,
                                           room->outgoing_gift.peer_id,
                                           room->outgoing_gift.serial, true);
    if (!nativeNetplayConnection_ReserveContent(&room->connection, peer_id,
                                                next.serial, true))
    {
        free(bytes);
        return false;
    }
    free(room->outgoing_gift.bytes);
    room->outgoing_gift = next;
    return true;
}

bool nativeNetplayRoom_AcceptGift(NativeNetplayRoom* room)
{
    if (room == NULL || room->match_configured ||
        room->incoming_gift.state != NativeNetplayGift_Offered)
    {
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
        gift->state =
            installed ? NativeNetplayGift_Installed : NativeNetplayGift_Failed;
        if (installed) {
            send_reply(room, gift, GiftPacket_Installed);
        } else {
            send_reply(room, gift, GiftPacket_Decline);
        }
        nativeNetplayConnection_ReleaseContent(
            &room->connection, gift->peer_id, gift->serial, false);
        return installed;
    }
    if (!nativeNetplayConnection_StartContent(&room->connection, gift->peer_id,
                                              gift->serial, false))
    {
        gift->state = NativeNetplayGift_Failed;
        send_reply(room, gift, GiftPacket_Decline);
        nativeNetplayConnection_ReleaseContent(
            &room->connection, gift->peer_id, gift->serial, false);
        return false;
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
        room->incoming_gift.state == NativeNetplayGift_Offered)
    {
        room->incoming_gift.state = NativeNetplayGift_Declined;
        room->incoming_gift.last_control_ms = 0;
        send_reply(room, &room->incoming_gift, GiftPacket_Decline);
        nativeNetplayConnection_ReleaseContent(
            &room->connection, room->incoming_gift.peer_id,
            room->incoming_gift.serial, false);
    }
}

bool nativeNetplayRoom_GiftStatus(const NativeNetplayRoom* room, bool incoming,
                                  NativeNetplayGiftStatus* status)
{
    if (room == NULL || status == NULL) {
        return false;
    }
    const NativeNetplayGift* gift =
        incoming ? &room->incoming_gift : &room->outgoing_gift;
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
                          const NativeContentControl* datagram)
{
    const uint8_t* packet = datagram->datagram.bytes;
    NativeContentId id;
    memcpy(id.bytes, packet + 5, sizeof(id.bytes));
    uint32_t serial = read_u32(packet + 37);
    if (!has_id(&id) || serial == 0 ||
        (packet[41] != NativeNetplayGift_Costume &&
         packet[41] != NativeNetplayGift_Fighter) ||
        memchr(packet + 42, '\0', NATIVE_CONTENT_TITLE_BYTES) == NULL)
    {
        return;
    }
    NativeNetplayGift* gift = &room->incoming_gift;
    if (gift->state != NativeNetplayGift_None &&
        gift->state != NativeNetplayGift_Installed &&
        gift->state != NativeNetplayGift_Failed &&
        gift->state != NativeNetplayGift_Declined &&
        !(same_id(&gift->id, &id) && gift->serial == serial &&
          strcmp(gift->peer_id, datagram->peer_id) == 0))
    {
        return;
    }
    if (same_id(&gift->id, &id) && gift->serial == serial &&
        strcmp(gift->peer_id, datagram->peer_id) == 0)
    {
        if (gift->state == NativeNetplayGift_Receiving) {
            send_reply(room, gift, GiftPacket_Accept);
        } else if (gift->state == NativeNetplayGift_Declined) {
            send_reply(room, gift, GiftPacket_Decline);
        } else if (gift->state == NativeNetplayGift_Installed) {
            send_reply(room, gift, GiftPacket_Installed);
        }
        return;
    }
    nativeNetplayConnection_ReleaseContent(&room->connection, gift->peer_id,
                                           gift->serial, false);
    if (!nativeNetplayConnection_ReserveContent(
            &room->connection, datagram->peer_id, serial, false))
    {
        return;
    }
    nativeContentTransfer_StopReceive(&gift->receive);
    *gift = (NativeNetplayGift){ 0 };
    gift->id = id;
    gift->serial = serial;
    gift->kind = packet[41];
    memcpy(gift->title, packet + 42, sizeof(gift->title));
    memcpy(gift->peer_id, datagram->peer_id, strlen(datagram->peer_id) + 1);
    gift->state = NativeNetplayGift_Offered;
}

static void receive_reply(NativeNetplayRoom* room,
                          const NativeContentControl* datagram, uint8_t type)
{
    NativeNetplayGift* gift = &room->outgoing_gift;
    if ((gift->state != NativeNetplayGift_Offered &&
         gift->state != NativeNetplayGift_Sending) ||
        strcmp(gift->peer_id, datagram->peer_id) != 0 ||
        memcmp(gift->id.bytes, datagram->datagram.bytes + 5,
               NATIVE_CONTENT_ID_BYTES) != 0 ||
        gift->serial != read_u32(datagram->datagram.bytes + 37))
    {
        return;
    }
    if (type == GiftPacket_Decline || type == GiftPacket_Installed) {
        gift->state = type == GiftPacket_Decline ? NativeNetplayGift_Declined
                                                 : NativeNetplayGift_Sent;
        free(gift->bytes);
        gift->bytes = NULL;
        nativeNetplayConnection_ReleaseContent(
            &room->connection, gift->peer_id, gift->serial, true);
        return;
    }
    if (gift->state == NativeNetplayGift_Sending) {
        return;
    }
    if (!nativeNetplayConnection_StartContent(&room->connection, gift->peer_id,
                                              gift->serial, true) ||
        !nativeContentTransfer_StartSend(&gift->send, &gift->id, gift->bytes,
                                         gift->size))
    {
        gift->state = NativeNetplayGift_Failed;
        free(gift->bytes);
        gift->bytes = NULL;
        nativeNetplayConnection_ReleaseContent(
            &room->connection, gift->peer_id, gift->serial, true);
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
        !nativeContentStore_Read(root, &gift->id, &bytes, &size))
    {
        return false;
    }
    bool valid = false;
    if (gift->kind == NativeNetplayGift_Costume) {
        NativeCostumePackage costume;
        const uint8_t* dat;
        size_t dat_size;
        valid =
            nativeContentStore_ParseCostume(bytes, size, &costume, &dat,
                                            &dat_size) &&
            nativeCostumePackage_Validate(&costume, dat, dat_size, root, NULL);
    } else if (gift->kind == NativeNetplayGift_Fighter) {
        NativeFighterPackage fighter;
        NativeFighterPackageData data;
        valid =
            nativeContentStore_ParseFighter(bytes, size, &fighter, &data) &&
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

static void fail_gift(NativeNetplayRoom* room, NativeNetplayGift* gift,
                      bool outgoing)
{
    if (!outgoing) {
        send_reply(room, gift, GiftPacket_Decline);
        nativeContentTransfer_StopReceive(&gift->receive);
    }
    free(gift->bytes);
    gift->bytes = NULL;
    gift->state = NativeNetplayGift_Failed;
    nativeNetplayConnection_ReleaseContent(&room->connection, gift->peer_id,
                                           gift->serial, outgoing);
}

void nativeNetplayRoom_GiftControl(NativeNetplayRoom* room,
                                   const NativeContentControl* message)
{
    const uint8_t* packet = message->datagram.bytes;
    size_t size = message->datagram.size;
    if (size < GIFT_REPLY_BYTES ||
        memcmp(packet, gift_magic, sizeof(gift_magic)))
    {
        return;
    }
    if (packet[4] == GiftPacket_Offer && size == GIFT_OFFER_BYTES) {
        receive_offer(room, message);
    } else if ((packet[4] == GiftPacket_Accept ||
                packet[4] == GiftPacket_Decline ||
                packet[4] == GiftPacket_Installed) &&
               size == GIFT_REPLY_BYTES)
    {
        receive_reply(room, message, packet[4]);
    }
}

void nativeNetplayRoom_GiftPacket(NativeNetplayRoom* room,
                                  const NativeContentDatagram* message)
{
    const uint64_t now = GetTickCount64();
    const uint8_t* packet = message->message.datagram.bytes;
    size_t size = message->message.datagram.size;
    NativeContentId id;
    NativeContentTransferPacketType type =
        nativeContentTransfer_PacketType(packet, size, &id);
    NativeNetplayGift* outgoing = &room->outgoing_gift;
    NativeNetplayGift* incoming = &room->incoming_gift;
    if (message->local_owner && message->serial == outgoing->serial &&
        type == NativeContentTransferPacket_Ack &&
        outgoing->state == NativeNetplayGift_Sending &&
        !strcmp(outgoing->peer_id, message->message.peer_id) &&
        same_id(&outgoing->id, &id))
    {
        nativeContentTransfer_SendPacket(&outgoing->send, packet, size, now);
    } else if (!message->local_owner && message->serial == incoming->serial &&
               (type == NativeContentTransferPacket_Begin ||
                type == NativeContentTransferPacket_Data ||
                type == NativeContentTransferPacket_Missing) &&
               incoming->state == NativeNetplayGift_Receiving &&
               !strcmp(incoming->peer_id, message->message.peer_id) &&
               same_id(&incoming->id, &id))
    {
        nativeContentTransfer_ReceivePacket(&incoming->receive, packet, size,
                                            now);
    }
}

bool nativeNetplayRoom_PumpGifts(NativeNetplayRoom* room)
{
    if (!room || !room->active || room->match_configured) {
        return false;
    }
    NativeNetplayConnection* connection = &room->connection;
    const uint64_t now = GetTickCount64();
    NativeNetplayGift* outgoing = &room->outgoing_gift;
    if (outgoing->state == NativeNetplayGift_Offered ||
        outgoing->state == NativeNetplayGift_Sending)
    {
        NativeContentChannelState state = nativeNetplayConnection_ContentState(
            connection, outgoing->peer_id, outgoing->serial, true);
        if (state == NativeContentChannel_None ||
            state == NativeContentChannel_Failed)
        {
            fail_gift(room, outgoing, true);
        } else if (outgoing->state == NativeNetplayGift_Offered &&
                   (!outgoing->last_control_ms ||
                    now - outgoing->last_control_ms >= GIFT_RETRY_MS))
        {
            uint8_t packet[GIFT_OFFER_BYTES];
            size_t size = control_packet(outgoing, GiftPacket_Offer, packet);
            if (nativeNetplayConnection_SendControl(
                    connection, outgoing->peer_id, packet, size))
            {
                outgoing->last_control_ms = now;
            }
        } else if (outgoing->state == NativeNetplayGift_Sending &&
                   state == NativeContentChannel_Connected)
        {
            uint8_t packet[NATIVE_CONTENT_TRANSFER_PACKET_CAPACITY];
            for (unsigned burst = 0; burst < GIFT_SEND_BURST; ++burst) {
                size_t size = nativeContentTransfer_NextSend(&outgoing->send,
                                                             now, packet);
                if (!size) {
                    break;
                }
                nativeNetplayConnection_SendContent(
                    connection, outgoing->peer_id, outgoing->serial, true,
                    packet, size);
            }
            if (outgoing->send.complete) {
                free(outgoing->bytes);
                outgoing->bytes = NULL;
                outgoing->state = NativeNetplayGift_Sent;
                nativeNetplayConnection_ReleaseContent(
                    connection, outgoing->peer_id, outgoing->serial, true);
            }
        }
    }
    NativeNetplayGift* incoming = &room->incoming_gift;
    if (incoming->state == NativeNetplayGift_Offered ||
        incoming->state == NativeNetplayGift_Receiving)
    {
        NativeContentChannelState state = nativeNetplayConnection_ContentState(
            connection, incoming->peer_id, incoming->serial, false);
        if (state == NativeContentChannel_None ||
            state == NativeContentChannel_Failed)
        {
            fail_gift(room, incoming, false);
        } else if (incoming->state == NativeNetplayGift_Receiving) {
            if (!incoming->last_control_ms ||
                now - incoming->last_control_ms >= GIFT_RETRY_MS)
            {
                send_reply(room, incoming, GiftPacket_Accept);
                incoming->last_control_ms = now;
            }
            if (state == NativeContentChannel_Connected) {
                uint8_t packet[NATIVE_CONTENT_TRANSFER_PACKET_CAPACITY];
                size_t size = nativeContentTransfer_NextReceive(
                    &incoming->receive, now, packet);
                if (size) {
                    nativeNetplayConnection_SendContent(
                        connection, incoming->peer_id, incoming->serial, false,
                        packet, size);
                }
            }
            if (incoming->receive.failed) {
                fail_gift(room, incoming, false);
            } else if (incoming->receive.complete) {
                bool installed = accept_download(room);
                nativeContentTransfer_StopReceive(&incoming->receive);
                incoming->state = installed ? NativeNetplayGift_Installed
                                            : NativeNetplayGift_Failed;
                send_reply(room, incoming,
                           installed ? GiftPacket_Installed
                                     : GiftPacket_Decline);
                nativeNetplayConnection_ReleaseContent(
                    connection, incoming->peer_id, incoming->serial, false);
            }
        }
    }
    return true;
}

#endif
