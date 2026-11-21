#include "native_lobby_session.h"
#include "native_stock_content.h"

#ifdef MELEE_NATIVE

#include <stdio.h>
#include <string.h>

static bool fail(NativeLobbySession* session, const char* reason)
{
    if (reason == NULL || reason[0] == '\0') {
        reason = "lobby operation failed";
    }
    snprintf(session->failure, sizeof(session->failure), "%s", reason);
    session->state = NativeLobbySessionState_Failed;
    return false;
}

static bool canonical_match(const NativeSignalingPresence* presence,
                            NativeLobbyMatch* match)
{
    uint8_t encoded[NATIVE_LOBBY_MATCH_MAX_WIRE_BYTES];
    if (presence->match_size == 0 ||
        !nativeLobbyMatch_Decode(match, presence->match, presence->match_size))
    {
        return false;
    }
    size_t size = nativeLobbyMatch_Encode(match, encoded, sizeof(encoded));
    return size == presence->match_size &&
           memcmp(encoded, presence->match, size) == 0;
}

static bool can_start_match(const NativeLobbySession* session)
{
    const NativeNetplayConnection* connection = &session->room.connection;
    switch (session->match_source) {
    case NativeLobbyMatchSource_Lobby:
        return connection->has_lobby && connection->lobby.has_rules &&
               connection->lobby.phase == 2 && connection->presence.stage != 255;
    case NativeLobbyMatchSource_QuickPlay:
        return connection->quick.state == NativeQuickState_Assigned &&
               connection->quick.phase == 2;
    case NativeLobbyMatchSource_Direct:
        return true;
    }
    return false;
}

bool nativeLobbySession_Start(NativeLobbySession* session,
                              const NativeNetplayConnectionConfig* config,
                              NativeLobbyMatchSource source)
{
    if (session == NULL || config == NULL) {
        return false;
    }
    memset(session, 0, sizeof(*session));
    session->match_source = source;
    if (!nativeNetplayRoom_Connect(&session->room, config)) {
        return fail(session, "could not connect to the room service");
    }
    session->state = NativeLobbySessionState_Connecting;
    return true;
}

void nativeLobbySession_Shutdown(NativeLobbySession* session)
{
    if (session == NULL) {
        return;
    }
    nativeNetplayRoom_Shutdown(&session->room);
    session->state = NativeLobbySessionState_Disconnected;
}

bool nativeLobbySession_Pump(NativeLobbySession* session)
{
    if (session == NULL ||
        session->state == NativeLobbySessionState_Disconnected ||
        session->state == NativeLobbySessionState_Failed)
    {
        return false;
    }
    if (session->state == NativeLobbySessionState_Returning) {
        if (!nativeNetplayRoom_PumpLobby(&session->room)) {
            return fail(session, nativeNetplayConnection_Failure(
                                     &session->room.connection));
        }
        const NativeSignalingPresence* presence =
            &session->room.connection.presence;
        if (session->room.connection.has_presence &&
            presence->match_epoch > session->finished_match_epoch &&
            presence->match_size == 0)
        {
            session->state = NativeLobbySessionState_Waiting;
        }
        return true;
    }
    if (session->room.match_configured) {
        session->state = NativeLobbySessionState_Starting;
        if (!nativeNetplayRoom_PumpSetup(&session->room)) {
            const char* reason = session->room.content_failure[0] != '\0'
                                     ? session->room.content_failure
                                     : nativeNetplayConnection_Failure(
                                           &session->room.connection);
            return fail(session, reason);
        }
        if (session->room.ready) {
            session->state = NativeLobbySessionState_Ready;
        }
        return true;
    }
    if (!nativeNetplayRoom_PumpLobby(&session->room)) {
        return fail(session, nativeNetplayConnection_Failure(
                                 &session->room.connection));
    }
    if (!session->room.connection.has_presence) {
        session->state = NativeLobbySessionState_Connecting;
        return true;
    }
    session->state = NativeLobbySessionState_Waiting;
    const NativeSignalingPresence* presence =
        &session->room.connection.presence;
    if (presence->match_size != 0 && presence->match_epoch != session->observed_match_epoch &&
        can_start_match(session)) {
        session->observed_match_epoch = presence->match_epoch;
        const char* local_peer_id =
            session->room.connection.local_peer_id;
        bool active_local = false;
        for (uint32_t index = 0; index < presence->participant_count;
             ++index) {
            const NativeSignalingParticipant* participant =
                &presence->participants[index];
            if (strcmp(participant->peer_id, local_peer_id) == 0) {
                active_local = participant->active_mask != 0;
                break;
            }
        }
        if (!active_local) {
            return true;
        }
        NativeLobbyMatch match;
        if (!canonical_match(presence, &match)) {
            return fail(session, "room published an invalid match proposal");
        }
        if (session->match_source == NativeLobbyMatchSource_Direct) {
            uint32_t index = 0;
            while (index < match.peer_count &&
                   strcmp(match.peers[index].peer_id, local_peer_id) != 0) ++index;
            if (index == match.peer_count) return true;
        } else if (memcmp(match.content_digest, nativeStockContent_Digest(), sizeof(match.content_digest))) {
            return fail(session, "room uses incompatible game content");
        }
        if (!nativeNetplayRoom_ConfigureMatch(
                &session->room, &match,
                session->room.artificial_delay_frames)) {
            return fail(session, "room published an invalid match proposal");
        }
        session->state = NativeLobbySessionState_Starting;
    }
    return true;
}

bool nativeLobbySession_FinishMatch(NativeLobbySession* session)
{
    if (session == NULL || session->state != NativeLobbySessionState_Ready) {
        return false;
    }
    session->finished_match_epoch = session->room.agreed_match_epoch;
    if (!nativeNetplayRoom_FinishMatch(&session->room)) {
        return fail(session, "could not keep the room open after the match");
    }
    session->state = NativeLobbySessionState_Returning;
    const NativeSignalingPresence* presence =
        &session->room.connection.presence;
    if (session->match_source == NativeLobbyMatchSource_Direct &&
        strcmp(presence->owner_peer_id,
               session->room.connection.local_peer_id) == 0 &&
        presence->match_epoch == session->finished_match_epoch &&
        presence->match_size != 0 &&
        !nativeSignalingClient_SetMatch(&session->room.connection.signaling,
                                        NULL, 0))
    {
        return fail(session, "could not clear the completed match");
    }
    return true;
}

bool nativeLobbySession_UpdateParticipant(NativeLobbySession* session,
                                          const NativeSignalingUpdate* update)
{
    return session != NULL && update != NULL &&
           session->state == NativeLobbySessionState_Waiting &&
           nativeSignalingClient_QueueUpdate(
               &session->room.connection.signaling, update);
}

const NativeSignalingPresence*
nativeLobbySession_Presence(const NativeLobbySession* session)
{
    return session != NULL && session->room.connection.has_presence
               ? &session->room.connection.presence
               : NULL;
}

const char* nativeLobbySession_Failure(const NativeLobbySession* session)
{
    return session == NULL || session->failure[0] == '\0' ? NULL
                                                          : session->failure;
}

#endif
