#pragma once
#include "native_content_transfer.h"
#include "native_netplay_content_channel.h"

enum {
    NATIVE_ROOM_CACHE_SENDS = 3,
    NATIVE_ROOM_CACHE_CHOICES = 8
};
typedef struct NativeRoomCacheChoice {
    NativeContentId id;
    char peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
    uint64_t retry_after_ms;
    bool cached;
} NativeRoomCacheChoice;

typedef struct NativeRoomCacheSend {
    NativeContentTransferSend transfer;
    char peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
    uint8_t* bytes;
    uint64_t last_progress_ms;
    uint32_t serial;
} NativeRoomCacheSend;

typedef struct NativeRoomCache {
    NativeRoomCacheChoice choices[NATIVE_ROOM_CACHE_CHOICES];
    NativeRoomCacheSend sends[NATIVE_ROOM_CACHE_SENDS];
    NativeContentTransferReceive receive;
    char peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
    uint64_t presence_revision, started_ms, last_request_ms;
    uint32_t serial, revision;
    uint8_t choice_count;
    bool receive_cached;
} NativeRoomCache;

struct NativeNetplayRoom;
void nativeNetplayRoom_ClearCacheTransfers(struct NativeNetplayRoom*);
void nativeNetplayRoom_CacheControl(struct NativeNetplayRoom*,
                                    const NativeContentControl*);
void nativeNetplayRoom_CachePacket(struct NativeNetplayRoom*,
                                   const NativeContentDatagram*);
void nativeNetplayRoom_PumpCache(struct NativeNetplayRoom*);
