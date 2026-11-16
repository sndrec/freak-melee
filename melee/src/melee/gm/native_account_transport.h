#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Credentials stay in the transport layer. UI views contain public identity only.
bool nativeAccountTransport_Publish(const char* origin, const char* handle, const char* token);
bool nativeAccountTransport_InitGuest(const char* root, const char* origin);
void nativeAccountTransport_Clear(void);
uint64_t nativeAccountTransport_Generation(void);
bool nativeAccountTransport_Headers(const wchar_t* url, wchar_t* headers, size_t capacity);
#ifdef __cplusplus
}
#endif
