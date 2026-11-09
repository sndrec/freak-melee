#ifndef MELEE_GM_NATIVE_MATCH_LAUNCH_H
#define MELEE_GM_NATIVE_MATCH_LAUNCH_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stdint.h>

#include "native_lobby_match.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NativeNetplayRoom NativeNetplayRoom;

typedef struct NativeNetplayPeerLaunch {
    char ipv4_address[16];
    uint16_t port;
    uint8_t peer;
    uint8_t port_mask;
} NativeNetplayPeerLaunch;

typedef struct NativeNetplayRoomPeerLaunch {
    char peer_id[65];
    uint8_t peer;
    uint8_t port_mask;
} NativeNetplayRoomPeerLaunch;

typedef struct NativeNetplayRoomLaunch {
    NativeNetplayRoomPeerLaunch peers[3];
    char signaling_endpoint[512];
    char room_id[13];
    char local_peer_id[65];
    char local_name[33];
    char build[97];
    char stun_host[256];
    uint16_t stun_port;
    uint8_t peer_count;
    bool enabled;
} NativeNetplayRoomLaunch;

typedef struct NativeNetplayLaunchConfig {
    NativeNetplayPeerLaunch peers[3];
    uint64_t session_id;
    uint16_t listen_port;
    uint8_t local_peer;
    uint8_t local_port_mask;
    uint8_t local_seats[4];
    uint8_t peer_count;
    uint8_t artificial_delay_frames;
    NativeNetplayRoomLaunch room;
    bool enabled;
} NativeNetplayLaunchConfig;

// 1: run, 0: help printed, -1: invalid command line.
int nativeMatchLaunch_Parse(int argc, char** argv, const char** iso_path);
bool nativeMatchLaunch_IsDirect(void);
bool nativeMatchLaunch_RetailMenu(void);
const char* nativeMatchLaunch_ScriptKind(void);
const char* nativeMatchLaunch_ScriptSource(void);
bool nativeMatchLaunch_Hidden(void);
bool nativeMatchLaunch_Muted(void);
// Import direct-match packages after native archives open but before the first
// scene starts. A failure ends launch without reaching fighter registration.
bool nativeMatchLaunch_PrepareDirectFighters(void);
bool nativeMatchLaunch_DirectPreparationFailed(void);
// Local diagnostic outbound-input delay for direct or sidebar room matches.
uint8_t nativeMatchLaunch_ArtificialDelayFrames(void);
uint8_t nativeMatchLaunch_InputDelayFrames(bool online);
// Re-import selected fighters before a tied match enters sudden death, whose
// scene initialization clears the previous fighter registry.
bool nativeMatchLaunch_PrepareSuddenDeathFighters(void);
const char* nativeMatchLaunch_ContentRoot(void);
void nativeMatchLaunch_Apply(void);
// Rebind selected custom fighter bundles after Fighter_FirstInitialize resets
// the decomp tables and before the first Fighter_Create.
bool nativeMatchLaunch_RegisterFighters(void);
// Resolve the installed fighter bundles selected on the home CSS into stable
// runtime character kinds. Empty IDs leave the corresponding output kind None.
bool nativeMatchLaunch_ConfigureLocalFighters(
    const NativeContentId packages[4], uint8_t kinds[4], bool preview);
bool nativeMatchLaunch_ApplyProfiles(void);
const char* nativeMatchLaunch_ReplayPath(void);
bool nativeMatchLaunch_ReplayPlayback(void);
bool nativeMatchLaunch_ReplayInputsOnly(void);
bool nativeMatchLaunch_Fast(void);
bool nativeMatchLaunch_TraceActions(void);
bool nativeMatchLaunch_TraceFighterState(void);
uint64_t nativeMatchLaunch_TraceGeometryFrame(void);
bool nativeMatchLaunch_RollbackCensus(void);
bool nativeMatchLaunch_ObjectCensus(void);
bool nativeMatchLaunch_SnapshotCensus(void);
bool nativeMatchLaunch_RollbackBenchmark(void);
bool nativeMatchLaunch_RollbackProbe(void);
bool nativeMatchLaunch_RollbackCorrectionProbe(void);
unsigned nativeMatchLaunch_RollbackCorrectionFrame(void);
const NativeNetplayLaunchConfig* nativeMatchLaunch_Netplay(void);
void nativeMatchLaunch_ClearNetplay(void);
uint64_t nativeMatchLaunch_Signature(void);
bool nativeMatchLaunch_CreateLobbyMatch(NativeLobbyMatch* match);
bool nativeMatchLaunch_AdoptLobbyRoom(const NativeNetplayRoom* room);
bool nativeMatchLaunch_AdoptSpectatorMatch(const NativeLobbyMatch* match);
bool nativeMatchLaunch_IsSpectating(void);

#ifdef __cplusplus
}
#endif

#endif
#endif
