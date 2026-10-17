#pragma once
#include "native_netplay_ice.h"
#include "native_signaling_wire.h"

enum {
    NATIVE_CONTENT_CHANNELS = 2,
    NATIVE_CONTENT_CONTROL_QUEUE = 16
};
typedef enum NativeContentChannelState {
    NativeContentChannel_None,
    NativeContentChannel_Reserved,
    NativeContentChannel_Connecting,
    NativeContentChannel_Connected,
    NativeContentChannel_Failed
} NativeContentChannelState;
typedef struct NativeContentChannel {
    NativeNetplayIcePeer ice;
    char peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
    char remote_description[NATIVE_NETPLAY_ICE_DESCRIPTION_CAPACITY];
    uint64_t join_order, started_ms;
    uint32_t serial;
    bool local_owner, started, local_description_sent,
        remote_description_applied;
    bool failed;
} NativeContentChannel;
typedef struct NativeContentControl {
    NativeNetplayIceDatagram datagram;
    char peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
} NativeContentControl;
typedef struct NativeContentDatagram {
    NativeContentControl message;
    uint32_t serial;
    bool local_owner;
} NativeContentDatagram;
typedef struct NativeNetplayContentChannels {
    NativeContentChannel routes[NATIVE_CONTENT_CHANNELS];
    NativeContentControl controls[NATIVE_CONTENT_CONTROL_QUEUE];
    unsigned read, write, poll_cursor;
} NativeNetplayContentChannels;

struct NativeNetplayConnection;
bool nativeNetplayConnection_ReserveContent(struct NativeNetplayConnection*,
                                            const char* peer, uint32_t serial,
                                            bool local_owner);
bool nativeNetplayConnection_StartContent(struct NativeNetplayConnection*,
                                          const char* peer, uint32_t serial,
                                          bool local_owner);
void nativeNetplayConnection_ReleaseContent(struct NativeNetplayConnection*,
                                            const char* peer, uint32_t serial,
                                            bool local_owner);
NativeContentChannelState
nativeNetplayConnection_ContentState(struct NativeNetplayConnection*,
                                     const char* peer, uint32_t serial,
                                     bool local_owner);
bool nativeNetplayConnection_SendContent(struct NativeNetplayConnection*,
                                         const char* peer, uint32_t serial,
                                         bool local_owner, const uint8_t*,
                                         size_t);
bool nativeNetplayConnection_PollContent(struct NativeNetplayConnection*,
                                         NativeContentDatagram*);
bool nativeNetplayConnection_SendControl(struct NativeNetplayConnection*,
                                         const char* peer, const uint8_t*,
                                         size_t);
bool nativeNetplayConnection_PollControl(struct NativeNetplayConnection*,
                                         NativeContentControl*);

// Connection-module lifecycle; all mutations stay on the calling game thread.
void nativeNetplayContentChannels_Reconcile(struct NativeNetplayConnection*);
void nativeNetplayContentChannels_Pump(struct NativeNetplayConnection*);
void nativeNetplayContentChannels_Clear(struct NativeNetplayConnection*);
void nativeNetplayContentChannels_Control(struct NativeNetplayConnection*,
                                          const char* peer, const uint8_t*,
                                          size_t);
void nativeNetplayContentChannels_Description(struct NativeNetplayConnection*,
                                              const char* peer,
                                              uint32_t serial,
                                              bool sender_owner,
                                              const uint8_t*, size_t);
