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
    GIFT_OFFER_BYTES = 138,
    GIFT_ACCEPT_BYTES = 73,
    GIFT_RECEIPT_TIMEOUT_MS = 120000,
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
        memcpy(output + 106, gift->dependency_id.bytes, NATIVE_CONTENT_ID_BYTES);
        return GIFT_OFFER_BYTES;
    }
    if (type == GiftPacket_Accept) {
        memcpy(output + GIFT_REPLY_BYTES, gift->receive.id.bytes, NATIVE_CONTENT_ID_BYTES);
        return GIFT_ACCEPT_BYTES;
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

static bool advance_incoming(NativeNetplayRoom* room);
static void fail_gift(NativeNetplayRoom* room, NativeNetplayGift* gift, bool outgoing);

static void release_bytes(NativeNetplayGift* gift)
{
    free(gift->bytes);
    free(gift->dependency_bytes);
    gift->bytes = gift->dependency_bytes = NULL;
    gift->send = (NativeContentTransferSend){0};
}

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
    release_bytes(&room->outgoing_gift);
    nativeContentTransfer_StopReceive(&room->incoming_gift.receive);
    release_bytes(&room->incoming_gift);
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
    NativeCostumePackageData costume_data;
    NativeFighterPackage fighter;
    NativeFighterPackageData fighter_data;
    if (nativeContentStore_ParseCostume(bytes, size, &costume, &costume_data))
    {
        next.kind = NativeNetplayGift_Costume;
        next.dependency_id = costume.fighter_id;
        if (has_id(&next.dependency_id) &&
            (!nativeContentStore_Read(root, &next.dependency_id, &next.dependency_bytes, &next.dependency_size) ||
             !nativeContentStore_ParseFighter(next.dependency_bytes, next.dependency_size, &fighter, &fighter_data))) {
            free(next.dependency_bytes);
            free(bytes);
            return false;
        }
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
        free(next.dependency_bytes);
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
        free(next.dependency_bytes);
        free(bytes);
        return false;
    }
    release_bytes(&room->outgoing_gift);
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
    return advance_incoming(room);
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
    status->dependency_id = gift->dependency_id;
    status->added_fighter = gift->added_fighter;
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
    NativeContentId dependency;
    memcpy(dependency.bytes, packet + 106, sizeof(dependency.bytes));
    uint32_t serial = read_u32(packet + 37);
    if (!room_member(room, datagram->peer_id) || !has_id(&id) || serial == 0 ||
        same_id(&id, &dependency) ||
        (packet[41] == NativeNetplayGift_Fighter && has_id(&dependency)) ||
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
        if (gift->kind != packet[41] || !same_id(&gift->dependency_id, &dependency)) return;
        if (gift->state == NativeNetplayGift_Receiving) {
            send_reply(room, gift, GiftPacket_Accept);
        } else if (gift->state == NativeNetplayGift_Declined || gift->state == NativeNetplayGift_Failed) {
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
    gift->dependency_id = dependency;
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
         gift->state != NativeNetplayGift_Sending &&
         gift->state != NativeNetplayGift_AwaitingReceipt) ||
        strcmp(gift->peer_id, datagram->peer_id) ||
        memcmp(gift->id.bytes, datagram->datagram.bytes + 5, NATIVE_CONTENT_ID_BYTES) ||
        gift->serial != read_u32(datagram->datagram.bytes + 37)) return;
    if (type == GiftPacket_Decline || type == GiftPacket_Installed) {
        gift->state = type == GiftPacket_Decline ? NativeNetplayGift_Declined : NativeNetplayGift_Sent;
        release_bytes(gift);
        nativeNetplayConnection_ReleaseContent(&room->connection, gift->peer_id, gift->serial, true);
        return;
    }
    NativeContentId requested;
    memcpy(requested.bytes, datagram->datagram.bytes + GIFT_REPLY_BYTES, sizeof(requested.bytes));
    bool parent = has_id(&gift->dependency_id) && same_id(&requested, &gift->dependency_id);
    if (!parent && !same_id(&requested, &gift->id)) return;
    // Parent then outfit is the only multi-package order. Late parent requests
    // cannot replace a transfer of the outfit already in progress.
    if (gift->send.active && (same_id(&gift->send.id, &requested) ||
                             (parent && same_id(&gift->send.id, &gift->id)))) return;
    if (!nativeNetplayConnection_StartContent(&room->connection, gift->peer_id, gift->serial, true) ||
        !nativeContentTransfer_StartSend(&gift->send, &requested,
            parent ? gift->dependency_bytes : gift->bytes,
            parent ? gift->dependency_size : gift->size)) {
        fail_gift(room, gift, true);
        return;
    }
    gift->state = NativeNetplayGift_Sending;
    gift->awaiting_since_ms = 0;
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
        NativeCostumePackageData costume_data;
        valid =
            nativeContentStore_ParseCostume(bytes, size, &costume, &costume_data) &&
            same_id(&costume.fighter_id, &gift->dependency_id) &&
            nativeCostumePackage_Validate(&costume, &costume_data, root, NULL);
    } else if (gift->kind == NativeNetplayGift_Fighter) {
        NativeFighterPackage fighter;
        NativeFighterPackageData data;
        valid =
            nativeContentStore_ParseFighter(bytes, size, &fighter, &data) &&
            nativeFighterRuntime_ValidateCached(&gift->id, root);
    }
    free(bytes);
    bool had_parent = has_id(&gift->dependency_id) &&
                      nativeContentStore_IsInstalled(root, &gift->dependency_id);
    bool installed = valid && nativeContentStore_Install(root, &gift->id);
    if (installed && has_id(&gift->dependency_id) && !had_parent)
        gift->added_fighter = gift->dependency_id;
    return installed;
}

static bool advance_incoming(NativeNetplayRoom* room)
{
    NativeNetplayGift* gift = &room->incoming_gift;
    char default_path[512];
    const char* root = content_root(room, default_path);
    if (!root) { fail_gift(room, gift, false); return false; }
    bool cached = nativeContentStore_IsCached(root, &gift->id);
    if (cached) {
        uint8_t* bytes = NULL;
        size_t size;
        NativeCostumePackage costume;
        NativeCostumePackageData data;
        NativeFighterPackage fighter;
        NativeFighterPackageData fighter_data;
        bool valid = nativeContentStore_Read(root, &gift->id, &bytes, &size) &&
            (gift->kind == NativeNetplayGift_Costume
             ? nativeContentStore_ParseCostume(bytes, size, &costume, &data) &&
               same_id(&costume.fighter_id, &gift->dependency_id)
             : nativeContentStore_ParseFighter(bytes, size, &fighter, &fighter_data));
        free(bytes);
        if (!valid) { fail_gift(room, gift, false); return false; }
    }
    const NativeContentId* wanted = NULL;
    if (has_id(&gift->dependency_id) && !nativeContentStore_IsCached(root, &gift->dependency_id))
        wanted = &gift->dependency_id;
    else if (!cached) wanted = &gift->id;
    if (wanted) {
        if (!nativeNetplayConnection_StartContent(&room->connection, gift->peer_id, gift->serial, false)) {
            fail_gift(room, gift, false);
            return false;
        }
        nativeContentTransfer_StopReceive(&gift->receive);
        nativeContentTransfer_StartReceive(&gift->receive, wanted);
        gift->state = NativeNetplayGift_Receiving;
        gift->last_control_ms = 0;
        send_reply(room, gift, GiftPacket_Accept);
        return true;
    }
    bool installed = install_cached(room);
    nativeContentTransfer_StopReceive(&gift->receive);
    gift->state = installed ? NativeNetplayGift_Installed : NativeNetplayGift_Failed;
    send_reply(room, gift, installed ? GiftPacket_Installed : GiftPacket_Decline);
    nativeNetplayConnection_ReleaseContent(&room->connection, gift->peer_id, gift->serial, false);
    return installed;
}

static bool accept_download(NativeNetplayRoom* room)
{
    NativeNetplayGift* gift = &room->incoming_gift;
    char default_path[512];
    const char* root = content_root(room, default_path);
    NativeContentId cached;
    if (!root || !nativeContentStore_Cache(root, gift->receive.bytes, gift->receive.size,
                                          &gift->receive.id, &cached)) return false;
    if (same_id(&cached, &gift->dependency_id)) {
        NativeFighterPackage fighter;
        NativeFighterPackageData data;
        if (!nativeContentStore_ParseFighter(gift->receive.bytes, gift->receive.size, &fighter, &data)) return false;
    }
    return advance_incoming(room);
}

static void fail_gift(NativeNetplayRoom* room, NativeNetplayGift* gift,
                      bool outgoing)
{
    if (!outgoing) {
        send_reply(room, gift, GiftPacket_Decline);
        nativeContentTransfer_StopReceive(&gift->receive);
    }
    release_bytes(gift);
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
    } else if ((packet[4] == GiftPacket_Accept && size == GIFT_ACCEPT_BYTES) ||
               ((packet[4] == GiftPacket_Decline || packet[4] == GiftPacket_Installed) &&
                size == GIFT_REPLY_BYTES))
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
        same_id(&outgoing->send.id, &id))
    {
        nativeContentTransfer_SendPacket(&outgoing->send, packet, size, now);
    } else if (!message->local_owner && message->serial == incoming->serial &&
               (type == NativeContentTransferPacket_Begin ||
                type == NativeContentTransferPacket_Data ||
                type == NativeContentTransferPacket_Missing) &&
               incoming->state == NativeNetplayGift_Receiving &&
               !strcmp(incoming->peer_id, message->message.peer_id) &&
               same_id(&incoming->receive.id, &id))
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
        outgoing->state == NativeNetplayGift_Sending ||
        outgoing->state == NativeNetplayGift_AwaitingReceipt)
    {
        NativeContentChannelState state = nativeNetplayConnection_ContentState(
            connection, outgoing->peer_id, outgoing->serial, true);
        // Installation can close the byte channel before its signaling receipt
        // arrives, including when the final data ACK was lost.
        if (outgoing->state == NativeNetplayGift_Sending &&
            (state == NativeContentChannel_None || state == NativeContentChannel_Failed)) {
            outgoing->state = NativeNetplayGift_AwaitingReceipt;
            outgoing->awaiting_since_ms = now;
        }
        if (!room_member(room, outgoing->peer_id) ||
            (outgoing->state != NativeNetplayGift_AwaitingReceipt &&
             (state == NativeContentChannel_None || state == NativeContentChannel_Failed)) ||
            (outgoing->state == NativeNetplayGift_AwaitingReceipt &&
             now - outgoing->awaiting_since_ms >= GIFT_RECEIPT_TIMEOUT_MS)) {
            fail_gift(room, outgoing, true);
        } else {
            if (!outgoing->last_control_ms || now - outgoing->last_control_ms >= GIFT_RETRY_MS) {
                uint8_t packet[GIFT_OFFER_BYTES];
                size_t size = control_packet(outgoing, GiftPacket_Offer, packet);
                if (nativeNetplayConnection_SendControl(connection, outgoing->peer_id, packet, size))
                    outgoing->last_control_ms = now;
            }
            if (outgoing->state == NativeNetplayGift_Sending && state == NativeContentChannel_Connected) {
                uint8_t packet[NATIVE_CONTENT_TRANSFER_PACKET_CAPACITY];
                for (unsigned burst = 0; burst < GIFT_SEND_BURST; ++burst) {
                    size_t size = nativeContentTransfer_NextSend(&outgoing->send, now, packet);
                    if (!size) break;
                    nativeNetplayConnection_SendContent(connection, outgoing->peer_id,
                        outgoing->serial, true, packet, size);
                }
                if (outgoing->send.complete) {
                    outgoing->state = NativeNetplayGift_AwaitingReceipt;
                    outgoing->awaiting_since_ms = now;
                }
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
                if (!accept_download(room) && incoming->state != NativeNetplayGift_Failed)
                    fail_gift(room, incoming, false);
            }
        }
    }
    return true;
}

#endif
