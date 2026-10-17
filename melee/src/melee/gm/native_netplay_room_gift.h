#ifndef MELEE_GM_NATIVE_NETPLAY_ROOM_GIFT_H
#define MELEE_GM_NATIVE_NETPLAY_ROOM_GIFT_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "native_content_transfer.h"
#include "native_netplay_content_channel.h"
#include "native_signaling_client.h"

typedef enum NativeNetplayGiftKind {
    NativeNetplayGift_Costume = 1,
    NativeNetplayGift_Fighter = 2,
} NativeNetplayGiftKind;

typedef enum NativeNetplayGiftState {
    NativeNetplayGift_None,
    NativeNetplayGift_Offered,
    NativeNetplayGift_Receiving,
    NativeNetplayGift_Sending,
    NativeNetplayGift_Installed,
    NativeNetplayGift_Sent,
    NativeNetplayGift_Declined,
    NativeNetplayGift_Failed,
} NativeNetplayGiftState;

typedef struct NativeNetplayGift {
    NativeContentId id;
    char peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
    char title[NATIVE_CONTENT_TITLE_BYTES];
    uint8_t kind;
    uint32_t serial;
    NativeNetplayGiftState state;
    uint64_t last_control_ms;
    uint8_t* bytes;
    size_t size;
    NativeContentTransferSend send;
    NativeContentTransferReceive receive;
} NativeNetplayGift;

typedef struct NativeNetplayGiftStatus {
    NativeContentId id;
    char peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
    char title[NATIVE_CONTENT_TITLE_BYTES];
    uint8_t kind;
    NativeNetplayGiftState state;
    uint32_t bytes_done;
    uint32_t bytes_total;
} NativeNetplayGiftStatus;

struct NativeNetplayRoom;
bool nativeNetplayRoom_OfferGift(struct NativeNetplayRoom* room,
                                 const char* peer_id,
                                 const NativeContentId* id);
bool nativeNetplayRoom_AcceptGift(struct NativeNetplayRoom* room);
void nativeNetplayRoom_DeclineGift(struct NativeNetplayRoom* room);
bool nativeNetplayRoom_GiftStatus(const struct NativeNetplayRoom* room,
                                  bool incoming,
                                  NativeNetplayGiftStatus* status);
void nativeNetplayRoom_GiftControl(struct NativeNetplayRoom*, const NativeContentControl*);
void nativeNetplayRoom_GiftPacket(struct NativeNetplayRoom*, const NativeContentDatagram*);
bool nativeNetplayRoom_PumpGifts(struct NativeNetplayRoom* room);
void nativeNetplayRoom_ClearGifts(struct NativeNetplayRoom* room);

#endif
#endif
