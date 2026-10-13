#include "native_netplay_udp.h"

#ifdef MELEE_NATIVE

#define WIN32_LEAN_AND_MEAN
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)

enum {
    MAX_RECEIVE_PACKETS_PER_POLL = 64,
};

static SOCKET transport_socket(const NativeNetplayUdp* transport)
{
    return (SOCKET) transport->socket_handle;
}

static bool socket_address(const NativeNetplayUdpPeer* peer,
                           struct sockaddr_in* address)
{
    if (peer == NULL || address == NULL || !peer->configured) {
        return false;
    }
    *address = (struct sockaddr_in){
        .sin_family = AF_INET,
        .sin_port = htons(peer->port),
    };
    address->sin_addr.s_addr = peer->ipv4_address;
    return true;
}

bool nativeNetplayUdp_Init(NativeNetplayUdp* transport, uint16_t bind_port,
                           uint64_t session_id, uint8_t local_peer,
                           uint8_t local_port_mask)
{
    if (transport == NULL) {
        return false;
    }
    *transport = (NativeNetplayUdp){
        .socket_handle = (uintptr_t) INVALID_SOCKET,
    };
    if (!nativeNetplayExchange_Init(&transport->exchange, session_id,
                                    local_peer, local_port_mask)) {
        return false;
    }
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        return false;
    }
    SOCKET socket_handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_handle == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }
    BOOL report_udp_resets = FALSE;
    DWORD reset_bytes;
    WSAIoctl(socket_handle, SIO_UDP_CONNRESET, &report_udp_resets,
             sizeof(report_udp_resets), NULL, 0, &reset_bytes, NULL, NULL);
    u_long nonblocking = 1;
    struct sockaddr_in local = {
        .sin_family = AF_INET,
        .sin_port = htons(bind_port),
    };
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    if (ioctlsocket(socket_handle, FIONBIO, &nonblocking) != 0 ||
        bind(socket_handle, (const struct sockaddr*) &local, sizeof(local)) !=
            0)
    {
        closesocket(socket_handle);
        WSACleanup();
        return false;
    }
    int local_size = sizeof(local);
    if (getsockname(socket_handle, (struct sockaddr*) &local, &local_size) !=
        0)
    {
        closesocket(socket_handle);
        WSACleanup();
        return false;
    }
    transport->socket_handle = (uintptr_t) socket_handle;
    transport->bound_port = ntohs(local.sin_port);
    transport->active = true;
    return true;
}

void nativeNetplayUdp_Shutdown(NativeNetplayUdp* transport)
{
    if (transport == NULL || !transport->active) {
        return;
    }
    closesocket(transport_socket(transport));
    WSACleanup();
    transport->socket_handle = (uintptr_t) INVALID_SOCKET;
    transport->active = false;
}

bool nativeNetplayUdp_AddPeer(NativeNetplayUdp* transport, uint8_t peer,
                              const char* ipv4_address, uint16_t port,
                              uint8_t port_mask)
{
    if (transport == NULL || !transport->active || peer >= 4 ||
        ipv4_address == NULL || port == 0 ||
        transport->peers[peer].configured)
    {
        return false;
    }
    struct in_addr parsed;
    if (InetPtonA(AF_INET, ipv4_address, &parsed) != 1) {
        return false;
    }
    if (!nativeNetplayExchange_AddPeer(&transport->exchange, peer,
                                       port_mask)) {
        return false;
    }
    transport->peers[peer] = (NativeNetplayUdpPeer){
        .ipv4_address = parsed.s_addr,
        .port = port,
        .configured = true,
    };
    return true;
}

bool nativeNetplayUdp_Send(NativeNetplayUdp* transport)
{
    if (transport == NULL || !transport->active)
    {
        return false;
    }
    uint8_t packet[NATIVE_NETPLAY_INPUT_PACKET_CAPACITY];
    size_t packet_size;
    if (!nativeNetplayExchange_PreparePacket(
            &transport->exchange, packet, sizeof(packet), &packet_size)) {
        return false;
    }
    if (packet_size == 0) {
        return true;
    }
    for (uint32_t peer = 0; peer < 4; ++peer) {
        if (!transport->peers[peer].configured) {
            continue;
        }
        struct sockaddr_in address;
        socket_address(&transport->peers[peer], &address);
        int sent = sendto(transport_socket(transport), (const char*) packet,
                          (int) packet_size, 0,
                          (const struct sockaddr*) &address, sizeof(address));
        if (sent != (int) packet_size) {
            return false;
        }
    }
    return true;
}

static bool source_matches(const NativeNetplayUdpPeer* peer,
                           const struct sockaddr_in* source)
{
    return peer->configured && source->sin_family == AF_INET &&
           source->sin_port == htons(peer->port) &&
           source->sin_addr.s_addr == peer->ipv4_address;
}

bool nativeNetplayUdp_Receive(NativeNetplayUdp* transport,
                              NativeRollbackInputHistory* inputs)
{
    if (transport == NULL || inputs == NULL || !transport->active) {
        return false;
    }
    uint8_t packet[NATIVE_NETPLAY_INPUT_PACKET_CAPACITY];
    for (uint32_t received = 0; received < MAX_RECEIVE_PACKETS_PER_POLL;
         ++received)
    {
        struct sockaddr_in source;
        int source_size = sizeof(source);
        int size = recvfrom(transport_socket(transport), (char*) packet,
                            sizeof(packet), 0, (struct sockaddr*) &source,
                            &source_size);
        if (size == SOCKET_ERROR) {
            int error = WSAGetLastError();
            return error == WSAEWOULDBLOCK;
        }
        NativeNetplayInputBatch batch;
        if (!nativeNetplayInput_Decode(&batch, packet, (size_t) size) ||
            batch.sender_peer >= 4) {
            ++transport->exchange.rejected_packets;
            ++transport->exchange.rejected_header_packets;
            continue;
        }
        if (!source_matches(&transport->peers[batch.sender_peer], &source))
        {
            ++transport->exchange.rejected_packets;
            ++transport->exchange.rejected_header_packets;
            continue;
        }
        nativeNetplayExchange_AcceptPacket(&transport->exchange, inputs,
                                           batch.sender_peer, packet,
                                           (size_t) size);
    }
    return true;
}

uint16_t nativeNetplayUdp_BoundPort(const NativeNetplayUdp* transport)
{
    return transport != NULL && transport->active ? transport->bound_port : 0;
}

#endif
