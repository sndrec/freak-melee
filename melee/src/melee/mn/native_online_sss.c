#include "native_online_sss.h"

#ifdef MELEE_NATIVE

#include <string.h>

#include <melee/gm/gmvsmelee.h>
#include <melee/gm/native_home.h>
#include <melee/gm/native_match_selection.h>
#include <melee/gm/native_online_session.h>
#include <melee/mn/types.h>

static struct NativeOnlineSssState {
    bool active;
    bool match_submitted;
} online_sss;

void nativeOnlineSss_Prepare(SSSData* sss)
{
    memset(&online_sss, 0, sizeof(online_sss));
    online_sss.active =
        sss != NULL &&
        (nativeOnlineSession_State() == NativeLobbySessionState_Waiting ||
         (nativeOnlineSession_Quick() &&
          (nativeOnlineSession_State() == NativeLobbySessionState_Starting ||
           nativeOnlineSession_State() == NativeLobbySessionState_Ready)));
}

bool nativeOnlineSss_IsActive(void)
{
    return online_sss.active;
}

NativeOnlineSssAction nativeOnlineSss_Pump(SSSData* sss,
                                           bool stage_selected,
                                           uint8_t stage)
{
    if (!online_sss.active || sss == NULL) {
        return NativeOnlineSssAction_Offline;
    }
    if (!nativeOnlineSession_Pump()) {
        NativeLobbySessionState state = nativeOnlineSession_State();
        return state == NativeLobbySessionState_Failed ||
                       state == NativeLobbySessionState_Disconnected
                   ? NativeOnlineSssAction_Failed
                   : NativeOnlineSssAction_Wait;
    }

    switch (nativeOnlineSession_State()) {
    case NativeLobbySessionState_Ready:
        // Adoption applies the canonical proposal to the persistent VS data.
        // Carry that exact data through this already-entered SSS scene.
        sss->vs = *gmVsMelee_GetVsData();
        online_sss.active = false;
        return NativeOnlineSssAction_Start;
    case NativeLobbySessionState_Starting:
        return NativeOnlineSssAction_Wait;
    case NativeLobbySessionState_Waiting:
        {
            bool owner_requested;
            if (!nativeHome_RoomRosterReady(&owner_requested, NULL, NULL) ||
                !owner_requested) {
                return NativeOnlineSssAction_Cancelled;
            }
        }
        break;
    case NativeLobbySessionState_Connecting:
    case NativeLobbySessionState_Returning:
        return NativeOnlineSssAction_Wait;
    case NativeLobbySessionState_Disconnected:
    case NativeLobbySessionState_Failed:
        return NativeOnlineSssAction_Failed;
    }

    if (!nativeOnlineSession_IsOwner()) {
        return NativeOnlineSssAction_Wait;
    }
    if (!stage_selected) {
        return NativeOnlineSssAction_OwnerInput;
    }
    if (!online_sss.match_submitted) {
        NativeLobbyMatch match;
        NativeContentId costume_packages[4];
        NativeContentId fighter_packages[4];
        bool owner_requested;
        if (!nativeHome_RoomRosterReady(&owner_requested, costume_packages,
                                        fighter_packages) ||
            !owner_requested ||
            !nativeMatchSelection_Build(&match, &sss->vs, stage,
                                        costume_packages, fighter_packages) ||
            !nativeOnlineSession_BeginMatch(&match))
        {
            return NativeOnlineSssAction_Failed;
        }
        online_sss.match_submitted = true;
    }
    return NativeOnlineSssAction_Wait;
}

void nativeOnlineSss_Reset(void)
{
    memset(&online_sss, 0, sizeof(online_sss));
}

#endif
