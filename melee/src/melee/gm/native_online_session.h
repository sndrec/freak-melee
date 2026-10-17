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
bool nativeOnlineSession_IsOwner(void);
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
