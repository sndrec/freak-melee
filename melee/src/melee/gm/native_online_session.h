#ifndef MELEE_GM_NATIVE_ONLINE_SESSION_H
#define MELEE_GM_NATIVE_ONLINE_SESSION_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stdint.h>

#include "native_lobby_session.h"
#include "native_lobby_selection.h"

#ifdef __cplusplus
extern "C" {
#endif

bool nativeOnlineSession_CreateRoom(const char* local_name,
                                    uint8_t local_players);
const char* nativeOnlineSession_Build(void);
const NativeLobbyListing* nativeOnlineSession_Listing(void);
bool nativeOnlineSession_SetListing(const char* title, bool listed);
bool nativeOnlineSession_JoinRoom(const char* room_code,
                                  const char* local_name,
                                  uint8_t local_players);
bool nativeOnlineSession_EnterQuick(uint8_t mode, const char* local_name,
                                    uint8_t local_players);
bool nativeOnlineSession_SearchQuick(bool searching);
bool nativeOnlineSession_PickQuickStage(uint8_t local_seat, uint8_t stage);
const NativeQuickAssignment* nativeOnlineSession_Quick(void);
const char* nativeOnlineSession_QuickError(void);
uint16_t nativeOnlineSession_QuickCountdownMs(void);
bool nativeOnlineSession_UnreadyQuick(void);
bool nativeOnlineSession_TakeQuickUnready(void);
void nativeOnlineSession_Stop(void);
bool nativeOnlineSession_Pump(void);
bool nativeOnlineSession_BeginMatch(const NativeLobbyMatch* selection);
NativeLobbySessionState nativeOnlineSession_State(void);
const NativeSignalingPresence* nativeOnlineSession_Presence(void);
const NativeLobbyMatch* nativeOnlineSession_Match(void);
NativeRoomChat* nativeOnlineSession_Chat(void);
uint32_t nativeOnlineSession_CacheRevision(void);
const char* nativeOnlineSession_Failure(void);
const char* nativeOnlineSession_RoomCode(void);
const char* nativeOnlineSession_LocalPeerId(void);
// Private per-connection proof for the account worker. Keep out of diagnostics.
const char* nativeOnlineSession_ConnectionId(void);
void nativeOnlineSession_Inspect(void);
bool nativeOnlineSession_WatchMatch(void);
void nativeOnlineSession_StopWatching(void);
const NativeRoomWatchReceive* nativeOnlineSession_Watch(void);
bool nativeOnlineSession_WatchContentReady(void);
const NativeRoomWatchReceive* nativeOnlineSession_ClaimWatch(void);
void nativeOnlineSession_ReleaseWatch(void);
bool nativeOnlineSession_ContentProgress(
    NativeNetplayContentProgress* progress);
bool nativeOnlineSession_OfferGift(const char* peer_id,
                                   const NativeContentId* id);
bool nativeOnlineSession_AcceptGift(void);
void nativeOnlineSession_DeclineGift(void);
bool nativeOnlineSession_GiftStatus(bool incoming,
                                    NativeNetplayGiftStatus* status);
bool nativeOnlineSession_UpdateSelection(
    const NativeLobbySelection* selection);
typedef enum NativeRoomSeatState {
    NativeRoomSeatState_Fighting,
    NativeRoomSeatState_Waiting,
    NativeRoomSeatState_Spectating,
} NativeRoomSeatState;
bool nativeOnlineSession_ChooseSeat(const char* peer_id, uint8_t seat,
                                    NativeRoomSeatState state);
uint8_t nativeOnlineSession_LocalPlayers(void);
uint8_t nativeOnlineSession_LocalActiveSeats(void);
bool nativeOnlineSession_IsOwner(void);
int nativeOnlineSession_StagePort(void);
bool nativeOnlineSession_PickStage(uint8_t stage);
bool nativeOnlineSession_SetRotation(NativeRoomRotationPolicy policy);
struct MatchEnd;
bool nativeOnlineSession_ReportMatch(const struct MatchEnd* result);

// A ready room remains at one process-lifetime address while the match borrows
// it. FinishMatch resets match state while preserving room membership.
NativeNetplayRoom* nativeOnlineSession_ClaimMatchRoom(void);
void nativeOnlineSession_FinishMatch(void);

#ifdef __cplusplus
}
#endif

#endif
#endif
