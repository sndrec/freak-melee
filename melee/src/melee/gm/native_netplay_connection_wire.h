#ifndef MELEE_GM_NATIVE_NETPLAY_CONNECTION_WIRE_H
#define MELEE_GM_NATIVE_NETPLAY_CONNECTION_WIRE_H

#ifdef MELEE_NATIVE

#include <stddef.h>
#include <stdint.h>

#include "native_netplay_ice.h"

size_t nativeNetplayConnectionWire_EncodeDescription(const char* description,
                                                     uint8_t* output,
                                                     size_t output_capacity);
bool nativeNetplayConnectionWire_DecodeDescription(
    const uint8_t* message, size_t message_size, char* description,
    size_t description_capacity);

#endif
#endif
