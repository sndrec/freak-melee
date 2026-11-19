#include <melee/ui/native_hub.h>
#include "native_online_menu.h"

#ifdef MELEE_NATIVE
#include <melee/gm/forward.h>

#include <stdio.h>
#include <string.h>
#include <windows.h>

#include <melee/gm/native_home.h>
#include <melee/gm/native_online_session.h>
#include <melee/mn/inlines.h>
#include <melee/mn/native_css_interaction.h>
#include <melee/ui/native_shell.h>

static struct NativeOnlineMenu {
    NativeOnlineMenuAction pending;
    char name[NATIVE_SIGNALING_MAX_NAME_BYTES + 1];
    char room_code[NATIVE_SIGNALING_ROOM_ID_BYTES + 1];
    char password[65];
    uint8_t local_players;
} menu;

bool nativeOnlineMenu_IsAvailable(void)
{
    return nativeHome_IsSelection() || nativeHome_IsPractice();
}

bool nativeOnlineMenu_HasPendingAction(void)
{
    return menu.pending != NativeOnlineMenuAction_None;
}

bool nativeOnlineMenu_Request(NativeOnlineMenuAction action, const char* name,
                              const char* code, uint8_t local_players, const char* password)
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
    snprintf(menu.password, sizeof(menu.password), "%s", password ? password : "");
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
        // A new player-created room replaces the current session, including a
        // Quick Play assignment. The pending operation owns its copied inputs.
        nativeHome_CancelStartMatch();
        if (nativeOnlineSession_State() != NativeLobbySessionState_Disconnected)
            nativeOnlineSession_Stop();
        if (nativeOnlineSession_State() ==
            NativeLobbySessionState_Disconnected)
        {
            bool started =
                action == NativeOnlineMenuAction_Create
                    ? nativeOnlineSession_CreateRoom(menu.name,
                                                     menu.local_players)
                    : nativeOnlineSession_JoinRoom(menu.room_code, menu.name,
                                                   menu.local_players, menu.password);
            if (!started) {
                nativeHub_Notice("Could not open the room.");
            } else {
                // The configured local party also applies when joining from
                // an already-open CSS, including mouse-only menu sessions.
                nativeCssInteraction_SetLocalPlayers(menu.local_players);
                sfxForward();
            }
        }
        SecureZeroMemory(menu.password, sizeof(menu.password));
    } else if (action == NativeOnlineMenuAction_Leave) {
        const NativeQuickAssignment* quick = nativeOnlineSession_Quick();
        if (quick && (quick->state == NativeQuickState_Assigned ||
                      quick->state == NativeQuickState_Aborted)) {
            nativeOnlineSession_SearchQuick(false);
        } else {
            nativeOnlineSession_Stop();
        }
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
