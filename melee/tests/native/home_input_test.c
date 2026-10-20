#include <melee/gm/native_home.h>
#include <sysdolphin/baselib/controller.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HSD_PadStatus HSD_PadCopyStatus[4];
static bool practice;
static unsigned players, countdown, returned;
static int physical[4];
bool nativeHome_IsPractice(void) { return practice; }
uint16_t nativeOnlineSession_QuickCountdownMs(void) { return countdown; }
uint8_t nativeOnlineSession_LocalPlayers(void) { return players; }
int nativeHome_PhysicalPortForLocalPlayer(unsigned seat) { return physical[seat]; }
bool nativeHome_ReturnToSelection(void) { ++returned; return true; }
#define CHECK(value) do { if (!(value)) { fprintf(stderr, "line %d: %s\n", __LINE__, #value); exit(1); } } while (0)

int main(void)
{
    for (unsigned port = 0; port < 4; ++port) {
        memset(HSD_PadCopyStatus, 0, sizeof(HSD_PadCopyStatus));
        HSD_PadCopyStatus[port].button = HSD_PAD_B | HSD_PAD_A;
        HSD_PadCopyStatus[port].trigger = HSD_PAD_B | HSD_PAD_A;
        HSD_PadCopyStatus[port].repeat = HSD_PAD_B | HSD_PAD_A;
        players = 1; physical[0] = port; returned = 0;
        practice = false; countdown = 3000;
        nativeHome_HandleInput(); CHECK(!returned);
        practice = true; countdown = 0;
        nativeHome_HandleInput(); CHECK(!returned);
        countdown = 3000; HSD_PadCopyStatus[port].err = PAD_ERR_NO_CONTROLLER;
        nativeHome_HandleInput(); CHECK(!returned);
        HSD_PadCopyStatus[port].err = 0;
        physical[0] = -1; nativeHome_HandleInput(); CHECK(!returned);
        physical[0] = port; nativeHome_HandleInput(); CHECK(returned == 1);
        CHECK(HSD_PadCopyStatus[port].trigger == HSD_PAD_A);
        CHECK(HSD_PadCopyStatus[port].repeat == HSD_PAD_A);
        CHECK(HSD_PadCopyStatus[port].button == (HSD_PAD_A | HSD_PAD_B));
        nativeHome_HandleInput(); CHECK(returned == 1);
    }
    players = 2; physical[0] = 3; physical[1] = 1; returned = 0;
    memset(HSD_PadCopyStatus, 0, sizeof(HSD_PadCopyStatus));
    HSD_PadCopyStatus[0].trigger = HSD_PAD_B; // A nonparticipant cannot unready.
    nativeHome_HandleInput(); CHECK(!returned);
    HSD_PadCopyStatus[1].trigger = HSD_PAD_B;
    nativeHome_HandleInput(); CHECK(returned == 1);
    CHECK(HSD_PadCopyStatus[0].trigger == HSD_PAD_B);
    puts("Home input ownership checks passed");
    return 0;
}
