#ifndef MELEE_GM_NATIVE_LOBBY_TOPOLOGY_H
#define MELEE_GM_NATIVE_LOBBY_TOPOLOGY_H

#ifdef MELEE_NATIVE

#include <stdbool.h>

#include "native_lobby_match.h"
#include "native_signaling_wire.h"

// Assign active room participants to the match's human controller ports.
// Active participants are ordered by join_order and receive consecutive
// numeric rollback identities. Non-human ports belong to the first active peer
// so every controller history has exactly one sender.
bool nativeLobbyTopology_AssignMatchPeers(
    NativeLobbyMatch* match, const NativeSignalingPresence* presence,
    const char* required_build);

#endif
#endif
