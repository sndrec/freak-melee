#ifndef MELEE_GM_NATIVE_NETPLAY_ROOM_CONTENT_H
#define MELEE_GM_NATIVE_NETPLAY_ROOM_CONTENT_H

#ifdef MELEE_NATIVE

#include "native_netplay_room.h"

void nativeNetplayRoom_ClearContent(NativeNetplayRoom* room, bool keep_match_files);
bool nativeNetplayRoom_PrepareContent(NativeNetplayRoom* room);
bool nativeNetplayRoom_ContentPacket(
    NativeNetplayRoom* room,
    const NativeNetplayConnectionDatagram* datagram);
bool nativeNetplayRoom_PumpContent(NativeNetplayRoom* room, bool* ready);

#endif
#endif
