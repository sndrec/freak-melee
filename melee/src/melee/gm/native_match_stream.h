#pragma once
#ifdef MELEE_NATIVE
#include "native_lobby_match.h"
#include <dolphin/pad.h>

enum {
    NATIVE_MATCH_STREAM_RECORD_CAPACITY = 60*60*60+64,
    NATIVE_MATCH_STREAM_RECORD_BYTES = 76,
    NATIVE_MATCH_STREAM_PACKET_HEADER = 20,
    NATIVE_MATCH_STREAM_PACKET_RECORDS = 6,
    NATIVE_MATCH_STREAM_PACKET_BYTES = NATIVE_MATCH_STREAM_PACKET_HEADER +
        NATIVE_MATCH_STREAM_PACKET_RECORDS*NATIVE_MATCH_STREAM_RECORD_BYTES,
    NATIVE_MATCH_STREAM_DESCRIPTION_BYTES = 8+NATIVE_LOBBY_MATCH_MAX_WIRE_BYTES,
};
typedef enum NativeMatchStreamRecordKind {
    NativeMatchStream_Frame,
    NativeMatchStream_PhaseEnd,
} NativeMatchStreamRecordKind;

typedef struct NativeMatchStreamDescription {
    NativeLobbyMatch match;
    uint32_t initial_seed;
} NativeMatchStreamDescription;

typedef struct NativeMatchStreamSample {
    PADStatus pads[4];
    uint64_t state_digest;
    uint32_t frame;
    uint16_t phase;
    uint8_t kind;
    bool continues;
} NativeMatchStreamSample;

// Host-side append-only storage. Records use the portable wire representation;
// rollback never rewinds it. Only corrected, fully confirmed ticks are appended.
typedef struct NativeMatchStream {
    NativeMatchStreamDescription description;
    uint8_t* records;
    uint64_t digest;
    uint32_t capacity, count, next_frame;
    uint16_t phase;
    bool finished, unavailable;
} NativeMatchStream;

bool nativeMatchStream_Bind(NativeMatchStream*, const NativeMatchStreamDescription*,
                            void* records, uint32_t capacity);
bool nativeMatchStream_AppendFrame(NativeMatchStream*, uint64_t frame,
                                   const PADStatus[4], uint64_t state_digest);
bool nativeMatchStream_EndPhase(NativeMatchStream*, bool continues,
                                uint64_t state_digest);
bool nativeMatchStream_Read(const NativeMatchStream*, uint32_t index,
                            NativeMatchStreamSample*);
size_t nativeMatchStream_EncodeDescription(const NativeMatchStreamDescription*,
                                          uint8_t*, size_t);
bool nativeMatchStream_DecodeDescription(NativeMatchStreamDescription*,
                                        const uint8_t*, size_t);
size_t nativeMatchStream_EncodeRecords(const NativeMatchStream*, uint32_t first,
                                      uint8_t*, size_t);
// Contiguous delivery with exact duplicate acceptance. A gap or contradiction
// rejects the complete packet without changing the receiver's prefix.
bool nativeMatchStream_AcceptRecords(NativeMatchStream*, const uint8_t*, size_t);
#endif
