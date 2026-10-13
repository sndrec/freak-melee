#ifndef MELEE_GM_NATIVE_LOBBY_MATCH_H
#define MELEE_GM_NATIVE_LOBBY_MATCH_H

#ifdef MELEE_NATIVE

#include "native_content_store.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    NATIVE_LOBBY_MATCH_MAX_PEERS = 4,
    NATIVE_LOBBY_MATCH_PEER_ID_BYTES = 64,
    NATIVE_LOBBY_MATCH_CONTENT_DIGEST_BYTES = 32,
    NATIVE_LOBBY_MATCH_MAX_WIRE_BYTES = 768,
};

typedef enum NativeLobbyMatchMode {
    NativeLobbyMatchMode_Time,
    NativeLobbyMatchMode_Stock,
    NativeLobbyMatchMode_Coin,
} NativeLobbyMatchMode;

typedef enum NativeLobbyMatchSlotKind {
    NativeLobbyMatchSlot_Empty,
    NativeLobbyMatchSlot_Human,
    NativeLobbyMatchSlot_Cpu,
} NativeLobbyMatchSlotKind;

typedef struct NativeLobbyMatchPeer {
    char peer_id[NATIVE_LOBBY_MATCH_PEER_ID_BYTES + 1];
    uint8_t numeric_peer;
    uint8_t port_mask;
} NativeLobbyMatchPeer;

typedef struct NativeLobbyMatchSlot {
    uint64_t character_id;
    uint64_t costume_id;
    NativeContentId character_package_id;
    NativeContentId costume_package_id;
    uint8_t kind;
    uint8_t local_seat;
    uint8_t cpu_level;
    uint8_t team;
    uint8_t handicap;
} NativeLobbyMatchSlot;

typedef struct NativeLobbyMatch {
    NativeLobbyMatchPeer peers[NATIVE_LOBBY_MATCH_MAX_PEERS];
    NativeLobbyMatchSlot slots[4];
    uint8_t content_digest[NATIVE_LOBBY_MATCH_CONTENT_DIGEST_BYTES];
    uint64_t session_id;
    uint64_t stage_id;
    uint64_t item_mask;
    uint8_t peer_count;
    uint8_t mode;
    uint8_t time_limit_minutes;
    uint8_t stock_time_limit_minutes;
    uint8_t stock_count;
    int8_t item_frequency;
    uint8_t handicap;
    uint8_t damage_ratio;
    bool teams;
    bool friendly_fire;
    bool pause;
    bool score_display;
} NativeLobbyMatch;

bool nativeLobbyMatch_Validate(const NativeLobbyMatch* match);
size_t nativeLobbyMatch_Encode(const NativeLobbyMatch* match, uint8_t* output,
                               size_t output_capacity);
bool nativeLobbyMatch_Decode(NativeLobbyMatch* match, const uint8_t* bytes,
                             size_t size);

#endif
#endif
