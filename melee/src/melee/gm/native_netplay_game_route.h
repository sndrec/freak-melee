#ifndef MELEE_GM_NATIVE_NETPLAY_GAME_ROUTE_H
#define MELEE_GM_NATIVE_NETPLAY_GAME_ROUTE_H
#ifdef MELEE_NATIVE
#include "native_netplay_connection_wire.h"
#include "native_signaling_wire.h"

struct NativeNetplayConnection;
typedef struct NativeNetplayConnectionPeer {
    NativeNetplayIcePeer ice;
    char peer_id[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
    uint64_t join_order;
    NativeRouteGeneration local, remote;
    uint64_t restart_after, negotiate_until, hello_after, description_after;
    unsigned retry_delay_ms;
    bool active, local_restart, remote_restart, hello_requested;
    bool local_description_sent, remote_description_applied, description_requested;
    bool was_connected;
} NativeNetplayConnectionPeer;

bool nativeNetplayGameRoute_Ready(const NativeNetplayConnectionPeer*);
void nativeNetplayGameRoute_Pump(struct NativeNetplayConnection*, NativeNetplayConnectionPeer*, uint64_t now);
void nativeNetplayGameRoute_Apply(NativeNetplayConnectionPeer*, const NativeConnectionMessage*);
#endif
#endif
