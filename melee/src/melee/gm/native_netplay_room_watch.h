#pragma once
#include "native_match_stream.h"
#include "native_netplay_content_channel.h"

typedef enum NativeRoomWatchState {
    NativeRoomWatch_None,
    NativeRoomWatch_Requesting,
    NativeRoomWatch_Receiving,
    NativeRoomWatch_Complete,
    NativeRoomWatch_Failed,
} NativeRoomWatchState;

typedef struct NativeRoomWatchSend {
    char peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES+1];
    uint64_t session, last_ack_ms, retry_ms, status_ms;
    uint32_t serial, acknowledged, sent;
    uint8_t metadata_mask;
} NativeRoomWatchSend;

typedef struct NativeRoomWatchReceive {
    NativeMatchStream stream;
    uint8_t* storage;
    char peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES+1];
    char failure[128];
    uint8_t metadata[NATIVE_MATCH_STREAM_DESCRIPTION_BYTES];
    uint64_t session, started_ms, control_ms, data_ms, ack_ms, remote_digest;
    uint32_t serial, remote_count;
    uint16_t metadata_size;
    uint8_t metadata_mask;
    bool remote_finished, status_received;
    NativeRoomWatchState state;
} NativeRoomWatchReceive;

typedef struct NativeRoomWatch {
    NativeRoomWatchSend sends[NATIVE_SIGNALING_MAX_PARTICIPANTS-1];
    NativeRoomWatchReceive receive;
    uint64_t pump_ms;
    unsigned cursor, sender_count;
} NativeRoomWatch;

struct NativeNetplayRoom;
bool nativeNetplayRoom_WatchMatch(struct NativeNetplayRoom*);
void nativeNetplayRoom_StopWatching(struct NativeNetplayRoom*);
void nativeNetplayRoom_ClearWatch(struct NativeNetplayRoom*);
void nativeNetplayRoom_WatchControl(struct NativeNetplayRoom*,const NativeContentControl*);
void nativeNetplayRoom_WatchPacket(struct NativeNetplayRoom*,const NativeContentDatagram*);
void nativeNetplayRoom_PumpWatch(struct NativeNetplayRoom*);
