#ifndef MELEE_GM_NATIVE_NETPLAY_UDP_H
#define MELEE_GM_NATIVE_NETPLAY_UDP_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stdint.h>

#include "native_netplay_exchange.h"

typedef struct NativeNetplayUdpPeer {
    uint32_t ipv4_address;
    uint16_t port;
    bool configured;
} NativeNetplayUdpPeer;

typedef struct NativeNetplayUdp {
    NativeNetplayExchange exchange;
    NativeNetplayUdpPeer peers[4];
    uintptr_t socket_handle;
    uint16_t bound_port;
    bool active;
} NativeNetplayUdp;

bool nativeNetplayUdp_Init(NativeNetplayUdp* transport, uint16_t bind_port,
                           uint64_t session_id, uint8_t local_peer,
                           uint8_t local_port_mask);
void nativeNetplayUdp_Shutdown(NativeNetplayUdp* transport);
bool nativeNetplayUdp_AddPeer(NativeNetplayUdp* transport, uint8_t peer,
                              const char* ipv4_address, uint16_t port,
                              uint8_t port_mask);
bool nativeNetplayUdp_Send(NativeNetplayUdp* transport);
bool nativeNetplayUdp_Receive(NativeNetplayUdp* transport,
                              NativeRollbackInputHistory* inputs);
uint16_t nativeNetplayUdp_BoundPort(const NativeNetplayUdp* transport);

#endif
#endif
