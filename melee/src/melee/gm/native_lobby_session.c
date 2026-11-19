#include "native_lobby_session.h"

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

bool nativeLobbySession_Start(NativeLobbySession* session,
                              const NativeNetplayConnectionConfig* config)
{
    if (session == NULL || config == NULL) {
        return false;
    }
    memset(session, 0, sizeof(*session));
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
    if (presence->match_size != 0) {
        NativeLobbyMatch match;
        if (!canonical_match(presence, &match)) {
            return fail(session, "room published an invalid match proposal");
        }
        const char* local_peer_id =
            session->room.connection.local_peer_id;
        bool active_local = false;
        bool included = false;
        for (uint32_t index = 0; index < presence->participant_count;
             ++index) {
            const NativeSignalingParticipant* participant =
                &presence->participants[index];
            if (strcmp(participant->peer_id, local_peer_id) == 0) {
                active_local = participant->active_mask != 0;
                break;
            }
        }
        for (uint32_t index = 0; index < match.peer_count; ++index) {
            if (strcmp(match.peers[index].peer_id, local_peer_id) == 0) {
                included = true;
                break;
            }
        }
        if (!active_local || !included) {
            return true;
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
    if (!session->room.connection.quick.mode &&
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

bool nativeLobbySession_BeginMatch(NativeLobbySession* session,
                                   const NativeLobbyMatch* selection,
                                   uint8_t artificial_delay_frames)
{
    if (session == NULL || selection == NULL ||
        session->state != NativeLobbySessionState_Waiting ||
        session->room.match_configured ||
        !session->room.connection.has_presence)
    {
        return false;
    }
    const NativeSignalingPresence* presence =
        &session->room.connection.presence;
    if (strcmp(presence->owner_peer_id,
               session->room.connection.local_peer_id) != 0 ||
        presence->match_size != 0)
    {
        return false;
    }
    NativeLobbyMatch match = *selection;
    if (!nativeLobbyTopology_AssignMatchPeers(
            &match, presence, session->room.connection.local_build))
    {
        return false;
    }
    bool local_in_match = false;
    for (uint32_t peer = 0; peer < match.peer_count; ++peer) {
        local_in_match |= strcmp(match.peers[peer].peer_id,
                                 session->room.connection.local_peer_id) == 0;
    }
    if (!local_in_match) {
        // The lobby owner can organize a match while continuing local practice.
        // Active peers adopt this proposal through the ordinary lobby pump.
        uint8_t bytes[NATIVE_LOBBY_MATCH_MAX_WIRE_BYTES];
        size_t size = nativeLobbyMatch_Encode(&match, bytes, sizeof(bytes));
        if (session->room.publish_request_epoch == presence->match_epoch || !size ||
            !nativeSignalingClient_SetMatch(&session->room.connection.signaling,
                                             bytes, size)) return false;
        session->room.publish_request_epoch = presence->match_epoch;
        return true;
    }
    if (!nativeNetplayRoom_ConfigureMatch(&session->room, &match,
                                          artificial_delay_frames)) {
        return false;
    }
    session->state = NativeLobbySessionState_Starting;
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
