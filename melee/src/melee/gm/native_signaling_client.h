#ifndef MELEE_GM_NATIVE_SIGNALING_CLIENT_H
#define MELEE_GM_NATIVE_SIGNALING_CLIENT_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stdatomic.h>
#include <stdint.h>

#include "native_signaling_wire.h"

enum {
    NATIVE_SIGNALING_COMMAND_CAPACITY = 16,
    NATIVE_SIGNALING_EVENT_CAPACITY = 16,
    NATIVE_SIGNALING_CHAT_CAPACITY = 128,
    NATIVE_SIGNALING_ENDPOINT_BYTES = 512,
    NATIVE_SIGNALING_ROOM_ID_BYTES = 12,
};

typedef struct NativeSignalingClientConfig {
    const char* endpoint;
    const char* room_id;
    const char* peer_id;
    const char* name;
    const char* build;
    const char* admission;
    uint8_t local_players;
} NativeSignalingClientConfig;

typedef enum NativeSignalingClientEventType {
    NativeSignalingClientEvent_Connected,
    NativeSignalingClientEvent_Message,
    NativeSignalingClientEvent_Disconnected,
} NativeSignalingClientEventType;

typedef struct NativeSignalingClientEvent {
    NativeSignalingServerMessage message;
    NativeSignalingClientEventType type;
    uint32_t error_code;
    char detail[96];
} NativeSignalingClientEvent;

typedef struct NativeSignalingCommandSlot {
    uint8_t bytes[NATIVE_SIGNALING_MAX_MESSAGE_BYTES];
    uint16_t size;
} NativeSignalingCommandSlot;

typedef struct NativeSignalingCommandQueue {
    NativeSignalingCommandSlot slots[NATIVE_SIGNALING_COMMAND_CAPACITY];
    _Atomic uint32_t read;
    _Atomic uint32_t write;
} NativeSignalingCommandQueue;

typedef struct NativeSignalingEventQueue {
    NativeSignalingClientEvent slots[NATIVE_SIGNALING_EVENT_CAPACITY];
    _Atomic uint32_t read;
    _Atomic uint32_t write;
} NativeSignalingEventQueue;

typedef struct NativeSignalingChatQueue {
    NativeRoomChatEvent slots[NATIVE_SIGNALING_CHAT_CAPACITY];
    _Atomic uint32_t read;
    _Atomic uint32_t write;
    _Atomic bool overflow;
} NativeSignalingChatQueue;

typedef struct NativeSignalingClient {
    NativeSignalingCommandQueue commands;
    NativeSignalingEventQueue events;
    NativeSignalingChatQueue chat;
    uintptr_t thread_handle;
    uintptr_t wake_event;
    _Atomic bool stop_requested;
    _Atomic bool running;
    _Atomic uint32_t worker_error;
    char endpoint[NATIVE_SIGNALING_ENDPOINT_BYTES];
    char room_id[NATIVE_SIGNALING_ROOM_ID_BYTES + 1];
    char peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
    char name[NATIVE_SIGNALING_MAX_NAME_BYTES + 1];
    char build[NATIVE_SIGNALING_MAX_BUILD_BYTES + 1];
    char admission[33];
    uint8_t local_players;
} NativeSignalingClient;

// The caller owns this transition-lifetime storage and zero-initializes it.
bool nativeSignalingClient_Start(NativeSignalingClient* client,
                                 const NativeSignalingClientConfig* config);
void nativeSignalingClient_Stop(NativeSignalingClient* client);
bool nativeSignalingClient_QueueUpdate(NativeSignalingClient* client,
                                       const NativeSignalingUpdate* update);
bool nativeSignalingClient_QueueSignal(NativeSignalingClient* client,
                                       const char* target_peer_id,
                                       const uint8_t* payload,
                                       uint16_t payload_size);
bool nativeSignalingClient_RequestPresence(NativeSignalingClient* client);
bool nativeSignalingClient_SetMatch(NativeSignalingClient* client,
                                    const uint8_t* payload,
                                    uint16_t payload_size);
bool nativeSignalingClient_AssignSeats(NativeSignalingClient* client,
                                       const char* peer_id,
                                       uint8_t active_mask,
                                       uint8_t waiting_mask);
bool nativeSignalingClient_SetRotation(NativeSignalingClient* client,
                                       uint8_t policy);
bool nativeSignalingClient_ReportMatch(NativeSignalingClient* client,
                                       uint64_t epoch,
                                       const char* winner_peer_id,
                                       uint8_t winner_seat,
                                       bool eligible);
bool nativeSignalingClient_RequestChat(NativeSignalingClient* client);
bool nativeSignalingClient_ConfigureQuick(NativeSignalingClient* client, uint8_t mode);
bool nativeSignalingClient_QueueQuick(NativeSignalingClient* client, bool searching);
bool nativeSignalingClient_RequestQuick(NativeSignalingClient* client);
bool nativeSignalingClient_PostChat(NativeSignalingClient* client, uint32_t nonce, const char* text);
bool nativeSignalingClient_PollChat(NativeSignalingClient* client, NativeRoomChatEvent* event);
bool nativeSignalingClient_Poll(NativeSignalingClient* client,
                                NativeSignalingClientEvent* event);
bool nativeSignalingClient_IsRunning(const NativeSignalingClient* client);

#endif
#endif
