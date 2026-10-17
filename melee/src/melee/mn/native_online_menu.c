#include <melee/ui/native_hub.h>
#include "native_online_menu.h"

#ifdef MELEE_NATIVE
#include <melee/gm/forward.h>

#include <stdio.h>
#include <string.h>

#include <melee/gm/native_home.h>
#include <melee/gm/native_online_session.h>
#include <melee/mn/inlines.h>
#include <melee/ui/native_shell.h>

static struct NativeOnlineMenu {
    NativeOnlineMenuAction pending;
    char name[NATIVE_SIGNALING_MAX_NAME_BYTES + 1];
    char room_code[NATIVE_SIGNALING_ROOM_ID_BYTES + 1];
    uint8_t local_players;
} menu;

bool nativeOnlineMenu_IsAvailable(void)
{
    return nativeHome_IsSelection() || nativeHome_IsPractice();
}

bool nativeOnlineMenu_Request(NativeOnlineMenuAction action, const char* name,
                              const char* code, uint8_t local_players)
{
    if (!nativeOnlineMenu_IsAvailable() ||
        menu.pending != NativeOnlineMenuAction_None)
    {
        return false;
    }
    menu.pending = action;
    menu.local_players = local_players;
    snprintf(menu.name, sizeof(menu.name), "%s",
             name != NULL ? name : "Player");
    snprintf(menu.room_code, sizeof(menu.room_code), "%s",
             code != NULL ? code : "");
    return true;
}

void nativeOnlineMenu_Focus(void)
{
    if (nativeOnlineMenu_IsAvailable()) {
        nativeHub_Open(NativeHubPage_Lobbies);
    }
}

void nativeOnlineMenu_Pump(void)
{
    NativeOnlineMenuAction action = menu.pending;
    menu.pending = NativeOnlineMenuAction_None;
    if (action == NativeOnlineMenuAction_Create ||
        action == NativeOnlineMenuAction_Join)
    {
        if (nativeOnlineSession_State() ==
            NativeLobbySessionState_Disconnected)
        {
            bool started =
                action == NativeOnlineMenuAction_Create
                    ? nativeOnlineSession_CreateRoom(menu.name,
                                                     menu.local_players)
                    : nativeOnlineSession_JoinRoom(menu.room_code, menu.name,
                                                   menu.local_players);
            if (!started) {
                nativeHub_Notice("Could not open the room.");
            } else {
                sfxForward();
            }
        }
    } else if (action == NativeOnlineMenuAction_Leave) {
        nativeOnlineSession_Stop();
        nativeShell_Blur();
        sfxBack();
    } else if (action == NativeOnlineMenuAction_StartMatch) {
        nativeShell_Blur();
        nativeHome_RequestStartMatch();
    }

    const bool active =
        nativeOnlineSession_State() != NativeLobbySessionState_Disconnected;
    if (active) {
        nativeOnlineSession_Pump();
    }
}

#endif
