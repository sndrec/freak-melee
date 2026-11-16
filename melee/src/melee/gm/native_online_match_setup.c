#include "native_online_match_setup.h"

#include "gm_1A3F.h"
#include "gm_1A45.h"
#include "gmvsmode.h"
#include "native_online_session.h"
#include <melee/ui/native_hub.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/controller.h>
#include <SDL3/SDL.h>

static uint64_t setup_started_ms;

void nativeOnlineMatchSetup_Load(void* unused)
{
    (void) unused;
    setup_started_ms = SDL_GetTicks();
    OSReport("[match-setup] Automatic stage; preparing match without stage select.\n");
}

void nativeOnlineMatchSetup_Tick(void)
{
    // No fighter objects survive into this scene. Installing the agreed assets
    // here cannot invalidate actors from the preceding practice scene.
    bool ok = nativeOnlineSession_Pump();
    NativeLobbySessionState state = nativeOnlineSession_State();
    if (ok && state == NativeLobbySessionState_Ready) {
        gm_SetNextGameModeStateId(gmVsMode_State_Vs);
        gm_801A4B74();
        return;
    }
    const NativeRoomLobbyState* lobby = nativeOnlineSession_LobbyState();
    bool cancelled = state == NativeLobbySessionState_Waiting && lobby &&
                     lobby->phase != 2;
    bool back = false;
    for (unsigned port = 0; port < 4; ++port)
        back |= !HSD_PadCopyStatus[port].err &&
                (HSD_PadCopyStatus[port].trigger & HSD_PAD_B) != 0;
    if (!ok || cancelled || back || state == NativeLobbySessionState_Failed ||
        state == NativeLobbySessionState_Disconnected ||
        SDL_GetTicks() - setup_started_ms > 45000) {
        nativeHub_Notice(cancelled || back ? "Match start cancelled."
                                  : "Could not prepare the online match.");
        if (!cancelled) nativeOnlineSession_Stop();
        gm_SetNextGameModeStateId(gmVsMode_State_Css);
        gm_801A4B74();
    }
}
