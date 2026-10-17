#pragma once
#include <melee/gm/native_lobby_selection.h>
#include <melee/gm/native_signaling_wire.h>

enum {
    NATIVE_CSS_NO_PORT = 255
};
typedef struct NativeCssLocalSlot {
    NativeLobbyPlayerSelection choice;
    uint8_t kind; // Gm_PKind: human, CPU or closed.
} NativeCssLocalSlot;
typedef struct NativeCssParticipant {
    NativeLobbyPlayerSelection choice;
    char peer[65], name[33];
    uint8_t seat, physical_port, canonical_port, kind;
    bool local, ready;
} NativeCssParticipant;
typedef struct NativeCssParticipants {
    NativeCssParticipant cards[4];
    unsigned count;
    bool online;
} NativeCssParticipants;

#ifdef __cplusplus
extern "C" {
#endif
// Presentation order changes independently of match ports and physical inputs.
bool nativeCssParticipants_Build(const NativeSignalingPresence* presence,
                                 const char* local_peer,
                                 const char* local_name,
                                 const NativeCssLocalSlot local[4],
                                 NativeCssParticipants* result);
#ifdef __cplusplus
}
#endif
