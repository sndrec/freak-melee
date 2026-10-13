#include "stage_hazard_state_internal.h"
#include <sysdolphin/baselib/debug.h>
RainbowCruiseState rainbow_cruise_state;
FountainState fountain_state;
void nativeFountain_ReleaseReflection(void* data)
{
    HSD_ASSERT(0, data==&fountain_state.reflection);
    fountain_state.owner=NULL;
}

// Actual simulation storage; not MELEE_ROLLBACK_HOST_STATE. All original
// zero initializers and the shared debug conversation's -1 are preserved.
CorneriaState corneria_state;
VenomState venom_state;
GreensState greens_state;
KraidState kraid_state;
BrinstarState brinstar_state;
int arwing_conversation_selection=-1;

KongoState kongo_state={ .links={
    { 8, 0, NULL, 0.06981317f, 0.0f, 0.0f },
    { 9, 0, NULL, 0.0f, 0.0f, 0.0f },
    { 0xA, 0, NULL, -0.05235988f, 0.0f, 0.0f },
    { 0x17, 0, NULL, -0.10471976f, 0.0f, 0.0f },
    { 0x18, 0, NULL, -0.13962634f, 0.0f, 0.0f },
    { 0x19, 0, NULL, -0.15707964f, 0.0f, 0.0f },
    { 0x1A, 0, NULL, -0.20943952f, 0.0f, 0.0f },
    { 0x1B, 0, NULL, -0.23561944f, 0.0f, 0.0f },
    { 0x1C, 0, NULL, -0.18325958f, 0.0f, 0.0f },
    { 0x1D, 0, NULL, -0.1308997f, 0.0f, 0.0f },
    { 0x1E, 0, NULL, -0.07853982f, 0.0f, 0.0f },
    { 0x1F, 0, NULL, -0.02617994f, 0.0f, 0.0f },
    { 0x20, 0, NULL, 0.0f, 0.0f, 0.0f },
    { 7, 0, NULL, 0.05235988f, 0.0f, 0.0f },
    { 0x2D, 0, NULL, 0.10471976f, 0.0f, 0.0f },
} };
IceMountainState ice_mountain_state={ .rows={
    { 1, 180.0f, -180.0f }, { 2, 190.0f, -180.0f }, { 3, 190.0f, -195.0f },
    { 4, 195.0f, -185.0f }, { 5, 190.0f, -200.0f }, { 6, 180.0f, -190.0f },
} };

MuteCityState mute_city_state;
CastleState castle_state={ .wind_angle_step=0.017453292f, .strong_wind=1 };

BattlefieldState battlefield_state;
PuraState pura_state={ .maps={
    { { 20, 2, 21 }, 0, NULL },
    { { 21, 2, 22 }, 0, NULL },
    { { 22, 2, 23 }, 0, NULL },
    { { 23, 2, 24 }, 0, NULL },
    { { 24, 2, 25 }, 0, NULL },
    { { 25, 2, 26 }, 0, NULL },
    { { 26, 2, 27 }, 0, NULL },
    { { 39, 2, 28 }, 0, NULL },
    { { 284, 12, 21 }, 0, NULL },
    { { 285, 12, 23 }, 0, NULL },
    { { 267, 12, 24 }, 0, NULL },
    { { 261, 12, 25 }, 0, NULL },
    { { 262, 12, 26 }, 0, NULL },
    { { 275, 12, 27 }, 0, NULL },
    { { 276, 12, 28 }, 0, NULL },
    { { 277, 12, 29 }, 0, NULL },
    { { 269, 12, 30 }, 0, NULL },
    { { 270, 12, 31 }, 0, NULL },
    { { 272, 12, 32 }, 0, NULL },
    { { 286, 12, 33 }, 0, NULL },
    { { 180, 8, 23 }, 0, NULL },
    { { 181, 8, 51 }, 0, NULL },
    { { 182, 8, 27 }, 0, NULL },
    { { 183, 8, 30 }, 0, NULL },
    { { 184, 8, 31 }, 0, NULL },
    { { 513, 25, 10 }, 0, NULL },
    { { 514, 25, 13 }, 0, NULL },
    { { 520, 25, 16 }, 0, NULL },
    { { 521, 25, 19 }, 0, NULL },
    { { 522, 25, 22 }, 0, NULL },
    { { 496, 25, 25 }, 0, NULL },
    { { 497, 25, 28 }, 0, NULL },
    { { 500, 25, 31 }, 0, NULL },
    { { 523, 25, 32 }, 0, NULL },
    { { 524, 25, 29 }, 0, NULL },
    { { 527, 25, 26 }, 0, NULL },
    { { 528, 25, 23 }, 0, NULL },
    { { 525, 25, 20 }, 0, NULL },
    { { 526, 25, 17 }, 0, NULL },
    { { 494, 25, 14 }, 0, NULL },
    { { 495, 25, 11 }, 0, NULL },
    { { -1, 0, 0 }, 0, NULL },
    { 0 },
    { 0 },
} };

BigBlueState big_blue_state={ .tracks={
    { 4, 6, 5, 0, { 0, 0, 0 } },    { 19, 21, 20, 0, { 0, 0, 0 } },
    { 7, 9, 8, 0, { 0, 0, 0 } },    { 10, 12, 11, 0, { 0, 0, 0 } },
    { 44, 46, 45, 0, { 0, 0, 0 } }, { 1, 3, 2, 0, { 0, 0, 0 } },
    { 32, 34, 33, 0, { 0, 0, 0 } }, { 35, 37, 36, 0, { 0, 0, 0 } },
    { 38, 40, 39, 0, { 0, 0, 0 } }, { 41, 43, 42, 0, { 0, 0, 0 } },
    { 29, 31, 30, 0, { 0, 0, 0 } }, { 13, 18, 17, 0, { 0, 0, 0 } },
} };
