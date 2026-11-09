#ifndef MELEE_GM_NATIVE_PROFILE_INPUT_H
#define MELEE_GM_NATIVE_PROFILE_INPUT_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stdint.h>

#include "native_profile.h"
#include <dolphin/pad.h>

enum {
    NATIVE_PROFILE_EXT_SHORT_HOP = UINT32_C(0x80000000),
    NATIVE_PROFILE_EXT_TAP_JUMP_DISABLED = UINT32_C(0x40000000),
    NATIVE_PROFILE_EXT_CSTICK_TILT = UINT32_C(0x20000000),
    NATIVE_PROFILE_EXT_ACTION_BUFFER_SHIFT = 24,
    NATIVE_PROFILE_EXT_ACTION_BUFFER_MASK = UINT32_C(0x07000000),
};

#ifdef __cplusplus
extern "C" {
#endif

bool nativeProfileInput_Assign(uint32_t port,
                               const NativeProfile* profile);
void nativeProfileInput_Clear(uint32_t port);
void nativeProfileInput_ClearAll(void);
const NativeProfile* nativeProfileInput_Assigned(uint32_t port);
void nativeProfileInput_Apply(PADStatus pads[4]);

#ifdef __cplusplus
}
#endif

#endif
#endif
