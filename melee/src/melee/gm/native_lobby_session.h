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

typedef enum NativeLobbyMatchSource {
    NativeLobbyMatchSource_Lobby,
    NativeLobbyMatchSource_QuickPlay,
    NativeLobbyMatchSource_Direct,
} NativeLobbyMatchSource;

typedef struct NativeLobbySession {
    NativeNetplayRoom room;
    NativeLobbySessionState state;
    NativeLobbyMatchSource match_source;
    uint64_t observed_match_epoch;
    uint64_t finished_match_epoch;
    char failure[NATIVE_NETPLAY_CONNECTION_FAILURE_BYTES];
} NativeLobbySession;

// Session storage must remain at a stable address until Shutdown returns.
bool nativeLobbySession_Start(NativeLobbySession* session,
                              const NativeNetplayConnectionConfig* config,
                              NativeLobbyMatchSource source);
void nativeLobbySession_Shutdown(NativeLobbySession* session);
bool nativeLobbySession_Pump(NativeLobbySession* session);
bool nativeLobbySession_FinishMatch(NativeLobbySession* session);
bool nativeLobbySession_UpdateParticipant(NativeLobbySession* session,
                                          const NativeSignalingUpdate* update);
const NativeSignalingPresence*
nativeLobbySession_Presence(const NativeLobbySession* session);
const char* nativeLobbySession_Failure(const NativeLobbySession* session);

#endif
#endif
