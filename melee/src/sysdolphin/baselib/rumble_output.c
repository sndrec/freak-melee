#include "rumble_output.h"

#ifdef MELEE_NATIVE

#include <dolphin/pad.h>

static bool output_suppressed;
static s8 physical_ports[4] = { 0, 1, 2, 3 };

void HSD_RumbleOutputSetSuppressed(bool suppressed)
{
    output_suppressed = suppressed;
}

bool HSD_RumbleOutputSetPortMapping(const s8 new_ports[4])
{
    if (new_ports == NULL) {
        return false;
    }
    u8 used = 0;
    for (s32 port = 0; port < 4; ++port) {
        s8 physical = new_ports[port];
        if (physical < -1 || physical >= 4 ||
            (physical >= 0 && (used & (1U << physical)) != 0)) {
            return false;
        }
        if (physical >= 0) {
            used |= 1U << physical;
        }
    }
    for (s32 port = 0; port < 4; ++port) {
        if (physical_ports[port] >= 0 &&
            physical_ports[port] != new_ports[port]) {
            PADControlMotor(physical_ports[port], PAD_MOTOR_STOP_HARD);
        }
        physical_ports[port] = new_ports[port];
    }
    return true;
}

void HSD_RumbleOutputCommand(s32 port, u32 command)
{
    if (!output_suppressed && port >= 0 && port < 4 &&
        physical_ports[port] >= 0) {
        PADControlMotor(physical_ports[port], command);
    }
}

#endif
