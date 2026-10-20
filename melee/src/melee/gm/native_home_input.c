#include "native_home.h"
#include "native_online_session.h"
#include <sysdolphin/baselib/controller.h>

void nativeHome_HandleInput(void)
{
    if (!nativeHome_IsPractice() || !nativeOnlineSession_QuickCountdownMs()) return;
    for (unsigned seat = 0; seat < nativeOnlineSession_LocalPlayers(); ++seat) {
        int port = nativeHome_PhysicalPortForLocalPlayer(seat);
        if (port < 0 || port >= 4) continue;
        HSD_PadStatus* pad = &HSD_PadCopyStatus[port];
        if (!pad->err && (pad->trigger & HSD_PAD_B)) {
            // Unready owns B before the hub or practice can use this sample.
            pad->trigger &= ~HSD_PAD_B;
            pad->repeat &= ~HSD_PAD_B;
            nativeHome_ReturnToSelection();
            return;
        }
    }
}
