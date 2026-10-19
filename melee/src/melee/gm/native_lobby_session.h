#ifndef MELEE_GM_NATIVE_LOBBY_SESSION_H
#define MELEE_GM_NATIVE_LOBBY_SESSION_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stdint.h>

#include "native_lobby_topology.h"
#include "native_netplay_room.h"

typedef enum NativeLobbySessionState {
    NativeLobbySessionState_Disconnected,
    NativeLobbySessionState_Connecting,
    NativeLobbySessionState_Waiting,
    NativeLobbySessionState_Starting,
    NativeLobbySessionState_Ready,
    NativeLobbySessionState_Returning,
    NativeLobbySessionState_Failed,
} NativeLobbySessionState;

typedef struct NativeLobbySession {
    NativeNetplayRoom room;
    NativeLobbySessionState state;
    uint64_t finished_match_epoch;
    char failure[NATIVE_NETPLAY_CONNECTION_FAILURE_BYTES];
} NativeLobbySession;

// Session storage must remain at a stable address until Shutdown returns.
bool nativeLobbySession_Start(NativeLobbySession* session,
                              const NativeNetplayConnectionConfig* config);
void nativeLobbySession_Shutdown(NativeLobbySession* session);
bool nativeLobbySession_Pump(NativeLobbySession* session);
bool nativeLobbySession_FinishMatch(NativeLobbySession* session);
// The room owner publishes the active roster. A fighting owner enters setup;
// a spectating owner stays in Waiting and can continue practicing or watching.
bool nativeLobbySession_BeginMatch(NativeLobbySession* session,
                                   const NativeLobbyMatch* selection,
                                   uint8_t artificial_delay_frames);
bool nativeLobbySession_UpdateParticipant(NativeLobbySession* session,
                                          const NativeSignalingUpdate* update);
bool nativeLobbySession_AssignSeats(NativeLobbySession* session,
                                    const char* peer_id,
                                    uint8_t active_mask,
                                    uint8_t waiting_mask);
const NativeSignalingPresence*
nativeLobbySession_Presence(const NativeLobbySession* session);
const char* nativeLobbySession_Failure(const NativeLobbySession* session);

#endif
#endif
