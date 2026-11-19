#include "native_netplay_room_content.h"

#ifdef MELEE_NATIVE

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

enum { CONTENT_SEND_BURST = 8 };

static bool has_id(const NativeContentId* id)
{
    const NativeContentId empty = { 0 };
    return memcmp(id, &empty, sizeof(empty)) != 0;
}

static bool content_fail(NativeNetplayRoom* room, const char* reason)
{
    snprintf(room->content_failure, sizeof(room->content_failure), "%s",
             reason);
    return false;
}

static bool same_id(const NativeContentId* a, const NativeContentId* b)
{
    return memcmp(a, b, sizeof(*a)) == 0;
}

static const char* content_root(const NativeNetplayRoom* room, char path[512])
{
    if (room->content_root != NULL) {
        return room->content_root;
    }
    return nativeContentStore_DefaultRoot(path, 512) ? path : NULL;
}

static int route_index(const NativeNetplayRoom* room, const char* peer_id)
{
    for (unsigned i = 0; i < room->peer_count; ++i) {
        if (strcmp(room->peers[i].peer_id, peer_id) == 0) {
            return (int) i;
        }
    }
    return -1;
}

static const char* slot_owner(const NativeNetplayRoom* room, unsigned port)
{
    for (unsigned i = 0; i < room->match.peer_count; ++i) {
        const NativeLobbyMatchPeer* peer = &room->match.peers[i];
        if ((peer->port_mask & (1U << port)) != 0) {
            return peer->peer_id;
        }
    }
    return NULL;
}

static bool locally_selected(const NativeNetplayRoom* room,
                             const NativeContentId* id)
{
    for (unsigned port = 0; port < 4; ++port) {
        const NativeLobbyMatchSlot* slot = &room->match.slots[port];
        const char* owner = slot_owner(room, port);
        if (slot->kind != NativeLobbyMatchSlot_Empty && owner != NULL &&
            strcmp(owner, room->connection.local_peer_id) == 0 &&
            (same_id(&slot->character_package_id, id) ||
             same_id(&slot->costume_package_id, id))) {
            return true;
        }
    }
    return false;
}

void nativeNetplayRoom_ClearContent(NativeNetplayRoom* room)
{
    if (room == NULL) {
        return;
    }
    for (unsigned i = 0; i < NATIVE_NETPLAY_CONNECTION_MAX_REMOTE_PEERS;
         ++i) {
        free(room->content_send_bytes[i]);
        room->content_send_bytes[i] = NULL;
        room->content_sends[i] = (NativeContentTransferSend){ 0 };
    }
    nativeContentTransfer_StopReceive(&room->content_receive);
    room->needed_count = 0;
    room->needed_index = 0;
    room->content_prepared = false;
    room->content_failure[0] = '\0';
}

bool nativeNetplayRoom_PrepareContent(NativeNetplayRoom* room)
{
    if (room == NULL || !room->match_configured) {
        return false;
    }
    if (room->content_prepared) {
        return true;
    }
    char default_path[512];
    const char* root = content_root(room, default_path);
    if (root == NULL) {
        return content_fail(room, "Could not open the content collection.");
    }
    room->needed_count = 0;
    for (unsigned port = 0; port < 4; ++port) {
        const NativeLobbyMatchSlot* slot = &room->match.slots[port];
        const NativeContentId* packages[] = {
            &slot->character_package_id,
            &slot->costume_package_id,
        };
        for (unsigned package = 0; package < 2; ++package) {
            const NativeContentId* id = packages[package];
            if (!has_id(id)) {
                continue;
            }
            const char* source = slot_owner(room, port);
            if (source == NULL) {
                return content_fail(room, "Package has no owning player.");
            }
            if (strcmp(source, room->connection.local_peer_id) == 0) {
                if (!nativeContentStore_IsInstalled(root, id)) {
                    return content_fail(room, "Selected local package is missing.");
                }
                continue;
            }
            if (nativeContentStore_IsCached(root, id)) {
                continue;
            }
            bool already_needed = false;
            for (unsigned i = 0; i < room->needed_count; ++i) {
                already_needed |= same_id(id, &room->needed_content[i]);
            }
            if (already_needed) {
                continue;
            }
            unsigned index = room->needed_count++;
            room->needed_content[index] = *id;
            memcpy(room->needed_sources[index], source,
                   strlen(source) + 1);
        }
    }
    room->needed_index = 0;
    if (room->needed_count != 0) {
        nativeContentTransfer_StartReceive(&room->content_receive,
                                            &room->needed_content[0]);
    }
    room->content_prepared = true;
    return true;
}

bool nativeNetplayRoom_ContentPacket(
    NativeNetplayRoom* room,
    const NativeNetplayConnectionDatagram* datagram)
{
    if (room == NULL || datagram == NULL) {
        return false;
    }
    int route = route_index(room, datagram->peer_id);
    if (route < 0) {
        return true;
    }
    NativeContentId id;
    NativeContentTransferPacketType type = nativeContentTransfer_PacketType(
        datagram->datagram.bytes, datagram->datagram.size, &id);
    if (type == NativeContentTransferPacket_Invalid ||
        type == NativeContentTransferPacket_Other) {
        return true;
    }
    if (type == NativeContentTransferPacket_Request) {
        NativeContentTransferSend* send = &room->content_sends[route];
        if (send->active && same_id(&send->id, &id)) {
            return true;
        }
        free(room->content_send_bytes[route]);
        room->content_send_bytes[route] = NULL;
        *send = (NativeContentTransferSend){ 0 };
        char default_path[512];
        const char* root = content_root(room, default_path);
        size_t size = 0;
        if (root != NULL && locally_selected(room, &id) &&
            nativeContentStore_IsInstalled(root, &id) &&
            nativeContentStore_Read(root, &id,
                                    &room->content_send_bytes[route],
                                    &size) &&
            nativeContentTransfer_StartSend(
                send, &id, room->content_send_bytes[route], size)) {
            return true;
        }
        free(room->content_send_bytes[route]);
        room->content_send_bytes[route] = NULL;
        uint8_t packet[NATIVE_CONTENT_TRANSFER_PACKET_CAPACITY];
        size_t length = nativeContentTransfer_Missing(&id, packet);
        return nativeNetplayConnection_Send(&room->connection,
                                             datagram->peer_id, packet,
                                             length) ||
               content_fail(room, "Could not reject unavailable package.");
    }
    if (type == NativeContentTransferPacket_Ack) {
        nativeContentTransfer_SendPacket(&room->content_sends[route],
                                          datagram->datagram.bytes,
                                          datagram->datagram.size,
                                          GetTickCount64());
        return true;
    }
    if (room->needed_index < room->needed_count &&
        strcmp(room->needed_sources[room->needed_index],
               datagram->peer_id) == 0) {
        nativeContentTransfer_ReceivePacket(&room->content_receive,
                                             datagram->datagram.bytes,
                                             datagram->datagram.size,
                                             GetTickCount64());
    }
    return true;
}

bool nativeNetplayRoom_PumpContent(NativeNetplayRoom* room, bool* ready)
{
    if (room == NULL || ready == NULL || !room->content_prepared) {
        return false;
    }
    *ready = false;
    uint64_t now = GetTickCount64();
    uint8_t packet[NATIVE_CONTENT_TRANSFER_PACKET_CAPACITY];
    for (unsigned i = 0; i < room->peer_count; ++i) {
        NativeContentTransferSend* send = &room->content_sends[i];
        for (unsigned burst = 0; burst < CONTENT_SEND_BURST; ++burst) {
            size_t length = nativeContentTransfer_NextSend(send, now, packet);
            if (length == 0) {
                break;
            }
            if (!nativeNetplayConnection_Send(&room->connection,
                                               room->peers[i].peer_id,
                                               packet, length)) {
                return content_fail(room, "Could not send content data.");
            }
        }
    }
    if (room->needed_index < room->needed_count) {
        NativeContentTransferReceive* receive = &room->content_receive;
        if (receive->failed) {
            return content_fail(room, "Peer could not provide selected content.");
        }
        if (receive->timer_started &&
            now - receive->last_progress_ms > 120000) {
            return content_fail(room, "Content transfer timed out.");
        }
        if (nativeNetplayConnection_ConnectedCount(&room->connection) ==
            nativeNetplayConnection_RemoteCount(&room->connection)) {
            size_t length =
                nativeContentTransfer_NextReceive(receive, now, packet);
            if (length != 0 &&
                !nativeNetplayConnection_Send(
                    &room->connection,
                    room->needed_sources[room->needed_index], packet, length)) {
                return content_fail(room, "Could not request selected content.");
            }
        }
        if (receive->complete) {
            char default_path[512];
            const char* root = content_root(room, default_path);
            NativeContentId cached;
            if (root == NULL ||
                !nativeContentStore_Cache(root, receive->bytes,
                                          receive->size, &receive->id,
                                          &cached) ||
                !same_id(&cached, &receive->id)) {
                return content_fail(room, "Received content failed verification.");
            }
            nativeContentTransfer_StopReceive(receive);
            ++room->needed_index;
            if (room->needed_index < room->needed_count) {
                nativeContentTransfer_StartReceive(
                    receive, &room->needed_content[room->needed_index]);
            }
        }
    }
    *ready = room->needed_index == room->needed_count;
    return true;
}

bool nativeNetplayRoom_ContentProgress(
    const NativeNetplayRoom* room, NativeNetplayContentProgress* progress)
{
    if (room == NULL || progress == NULL || !room->content_prepared ||
        room->ready) {
        return false;
    }
    if (room->needed_index < room->needed_count) {
        const NativeContentTransferReceive* receive =
            &room->content_receive;
        *progress = (NativeNetplayContentProgress){
            .id = receive->id,
            .bytes_done = (uint32_t) receive->received,
            .bytes_total = (uint32_t) receive->size,
            .package_index = room->needed_index + 1,
            .package_count = room->needed_count,
            .direction = NativeNetplayContent_Receiving,
        };
        nativeContentStore_ReadPreview(receive->bytes, receive->received,
                                       &progress->preview);
        return true;
    }
    for (unsigned i = 0; i < room->peer_count; ++i) {
        const NativeContentTransferSend* send = &room->content_sends[i];
        if (send->active && !send->complete) {
            *progress = (NativeNetplayContentProgress){
                .bytes_done = (uint32_t) send->acknowledged,
                .bytes_total = (uint32_t) send->size,
                .package_index = 1,
                .package_count = 1,
                .direction = NativeNetplayContent_Sending,
            };
            return true;
        }
    }
    return false;
}

#endif
