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
    bool owns_bytes;
} NativeRoomCacheSend;

typedef struct NativeRoomCachedPackage {
    NativeContentId id;
    uint8_t* bytes;
    size_t size;
} NativeRoomCachedPackage;

typedef struct NativeRoomCache {
    NativeRoomCacheChoice choices[NATIVE_ROOM_CACHE_CHOICES];
    NativeRoomCacheSend sends[NATIVE_ROOM_CACHE_SENDS];
    NativeContentTransferReceive receive;
    NativeRoomCachedPackage retained[NATIVE_ROOM_CACHE_CHOICES];
    char peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
    uint64_t presence_revision, started_ms, last_request_ms;
    uint64_t retained_session, upload_ms, active_packets;
    size_t retained_bytes;
    uint32_t serial, revision, watch_serial, upload_cursor, file_reads, active_requests;
    uint8_t choice_count, retained_count;
    bool receive_cached;
} NativeRoomCache;

struct NativeNetplayRoom;
void nativeNetplayRoom_ClearCacheTransfers(struct NativeNetplayRoom*);
bool nativeNetplayRoom_RetainMatchPackages(struct NativeNetplayRoom*);
void nativeNetplayRoom_ClearRetainedPackages(struct NativeNetplayRoom*);
bool nativeNetplayRoom_WatchContentReady(const struct NativeNetplayRoom*);
void nativeNetplayRoom_CacheControl(struct NativeNetplayRoom*,
                                    const NativeContentControl*);
void nativeNetplayRoom_CachePacket(struct NativeNetplayRoom*,
                                   const NativeContentDatagram*);
void nativeNetplayRoom_PumpCache(struct NativeNetplayRoom*);
