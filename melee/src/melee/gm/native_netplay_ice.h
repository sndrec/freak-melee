#ifndef MELEE_GM_NATIVE_NETPLAY_ICE_H
#define MELEE_GM_NATIVE_NETPLAY_ICE_H

#ifdef MELEE_NATIVE

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "native_content_transfer.h"
#include "native_netplay_input.h"

enum {
    NATIVE_NETPLAY_ICE_DATAGRAM_CAPACITY = 64,
    NATIVE_NETPLAY_ICE_DATAGRAM_BYTES = NATIVE_CONTENT_TRANSFER_PACKET_CAPACITY,
    NATIVE_NETPLAY_ICE_DESCRIPTION_CAPACITY = 4096,
    NATIVE_NETPLAY_ICE_ADDRESS_CAPACITY = 64,
};

typedef enum NativeNetplayIceState {
    NativeNetplayIceState_Disconnected,
    NativeNetplayIceState_Gathering,
    NativeNetplayIceState_Connecting,
    NativeNetplayIceState_Connected,
    NativeNetplayIceState_Completed,
    NativeNetplayIceState_Failed,
} NativeNetplayIceState;

typedef enum NativeNetplayIceSend {
    NativeNetplayIceSend_Failed,
    NativeNetplayIceSend_Sent,
    NativeNetplayIceSend_Retry,
} NativeNetplayIceSend;

typedef struct NativeNetplayIceConfig {
    const char* stun_host;
    uint16_t stun_port;
    const char* turn_host;
    uint16_t turn_port;
    const char* turn_username;
    const char* turn_password;
    uint16_t local_port_begin;
    uint16_t local_port_end;
} NativeNetplayIceConfig;

typedef struct NativeNetplayIceDatagram {
    uint16_t size;
    uint8_t bytes[NATIVE_NETPLAY_ICE_DATAGRAM_BYTES];
} NativeNetplayIceDatagram;

typedef struct NativeNetplayIcePeer {
    NativeNetplayIceDatagram datagrams[NATIVE_NETPLAY_ICE_DATAGRAM_CAPACITY];
    uintptr_t agent;
    _Atomic uint32_t read_position;
    _Atomic uint32_t write_position;
    _Atomic uint32_t dropped_datagrams;
    _Atomic int state;
    _Atomic bool gathering_done;
    bool gathering_started;
} NativeNetplayIcePeer;

bool nativeNetplayIce_Init(NativeNetplayIcePeer* peer,
                           const NativeNetplayIceConfig* config);
void nativeNetplayIce_Shutdown(NativeNetplayIcePeer* peer);
bool nativeNetplayIce_StartGathering(NativeNetplayIcePeer* peer);
bool nativeNetplayIce_GatheringDone(const NativeNetplayIcePeer* peer);
bool nativeNetplayIce_GetLocalDescription(const NativeNetplayIcePeer* peer,
                                          char* description,
                                          size_t description_capacity);
bool nativeNetplayIce_SetRemoteDescription(NativeNetplayIcePeer* peer,
                                           const char* description);
bool nativeNetplayIce_SetRemoteGatheringDone(NativeNetplayIcePeer* peer);
NativeNetplayIceState nativeNetplayIce_State(const NativeNetplayIcePeer* peer);
NativeNetplayIceSend nativeNetplayIce_Send(NativeNetplayIcePeer* peer,
                                          const uint8_t* bytes, size_t size);
bool nativeNetplayIce_PollDatagram(NativeNetplayIcePeer* peer,
                                   NativeNetplayIceDatagram* datagram);
uint32_t nativeNetplayIce_DroppedDatagrams(const NativeNetplayIcePeer* peer);
const char* nativeNetplayIce_SelectedRoute(const NativeNetplayIcePeer* peer);
bool nativeNetplayIce_GetSelectedAddresses(const NativeNetplayIcePeer* peer,
                                           char* local, size_t local_capacity,
                                           char* remote,
                                           size_t remote_capacity);

#endif
#endif
