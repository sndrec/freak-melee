#ifndef SYSDOLPHIN_BASELIB_RUMBLE_OUTPUT_H
#define SYSDOLPHIN_BASELIB_RUMBLE_OUTPUT_H

#ifdef MELEE_NATIVE

#include <Runtime/platform.h>

#include <stdbool.h>

void HSD_RumbleOutputSetSuppressed(bool suppressed);
bool HSD_RumbleOutputSetPortMapping(const s8 physical_ports[4]);
void HSD_RumbleOutputCommand(s32 port, u32 command);

#endif
#endif
