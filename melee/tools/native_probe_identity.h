#pragma once
#include <stdbool.h>
#include <windows.h>
#include <bcrypt.h>

// Separate simulated players in a probe process need separate guest keys.
static bool nativeProbeGuestKey(char output[65])
{
    unsigned char bytes[32];
    if (BCryptGenRandom(NULL, bytes, sizeof(bytes), BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0) return false;
    const char hex[] = "0123456789abcdef";
    for (unsigned i = 0; i < sizeof(bytes); ++i) {
        output[2 * i] = hex[bytes[i] >> 4];
        output[2 * i + 1] = hex[bytes[i] & 15];
    }
    output[64] = 0;
    SecureZeroMemory(bytes, sizeof(bytes));
    return true;
}
