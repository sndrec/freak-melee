#include "native_turn_credentials.h"

#ifdef MELEE_NATIVE

#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <winhttp.h>

enum {
    TURN_WIRE_HEADER_BYTES = 20,
    TURN_WIRE_VERSION = 1,
    TURN_FLAG_RELAY = 1,
    HTTP_TIMEOUT_MS = 5000,
};

static uint16_t read_u16_be(const uint8_t* bytes)
{
    return (uint16_t) ((uint16_t) bytes[0] << 8 | bytes[1]);
}

static uint32_t read_u32_be(const uint8_t* bytes)
{
    return (uint32_t) bytes[0] << 24 | (uint32_t) bytes[1] << 16 |
           (uint32_t) bytes[2] << 8 | bytes[3];
}

static bool copy_wire_text(char* output, size_t capacity, const uint8_t* input,
                           size_t length)
{
    if (length >= capacity || memchr(input, '\0', length) != NULL) {
        return false;
    }
    memcpy(output, input, length);
    output[length] = '\0';
    return true;
}

bool nativeTurnCredentials_Decode(const uint8_t* bytes, size_t size,
                                  NativeTurnCredentials* credentials)
{
    if (bytes == NULL || credentials == NULL ||
        size < TURN_WIRE_HEADER_BYTES || memcmp(bytes, "FMT1", 4) != 0 ||
        bytes[4] != TURN_WIRE_VERSION || (bytes[5] & ~TURN_FLAG_RELAY) != 0 ||
        bytes[6] == 0 || bytes[10] != 0 || bytes[11] != 0)
    {
        return false;
    }
    const bool has_turn = (bytes[5] & TURN_FLAG_RELAY) != 0;
    const size_t stun_length = bytes[6];
    const size_t turn_length = bytes[7];
    const size_t username_length = bytes[8];
    const size_t password_length = bytes[9];
    const size_t payload_size =
        stun_length + turn_length + username_length + password_length;
    const uint16_t stun_port = read_u16_be(bytes + 16);
    const uint16_t turn_port = read_u16_be(bytes + 18);
    if (size != TURN_WIRE_HEADER_BYTES + payload_size || stun_port == 0 ||
        (has_turn && (turn_length == 0 || username_length == 0 ||
                      password_length == 0 || turn_port == 0)) ||
        (!has_turn && (turn_length != 0 || username_length != 0 ||
                       password_length != 0 || turn_port != 0)))
    {
        return false;
    }

    NativeTurnCredentials decoded = {
        .ttl_seconds = read_u32_be(bytes + 12),
        .stun_port = stun_port,
        .turn_port = turn_port,
        .has_turn = has_turn,
    };
    size_t offset = TURN_WIRE_HEADER_BYTES;
    if (!copy_wire_text(decoded.stun_host, sizeof(decoded.stun_host),
                        bytes + offset, stun_length))
    {
        return false;
    }
    offset += stun_length;
    if (!copy_wire_text(decoded.turn_host, sizeof(decoded.turn_host),
                        bytes + offset, turn_length))
    {
        return false;
    }
    offset += turn_length;
    if (!copy_wire_text(decoded.username, sizeof(decoded.username),
                        bytes + offset, username_length))
    {
        return false;
    }
    offset += username_length;
    if (!copy_wire_text(decoded.password, sizeof(decoded.password),
                        bytes + offset, password_length))
    {
        return false;
    }
    *credentials = decoded;
    return true;
}

static bool build_url(const char* endpoint, wchar_t* output, size_t capacity)
{
    if (endpoint == NULL || output == NULL || capacity == 0) {
        return false;
    }
    size_t endpoint_length = strnlen(endpoint, 512);
    if (endpoint_length == 0 || endpoint_length >= 512) {
        return false;
    }
    char url[560];
    const bool trailing_slash = endpoint[endpoint_length - 1] == '/';
    int written =
        snprintf(url, sizeof(url), "%.*s%sv1/turn-credentials",
                 (int) endpoint_length, endpoint, trailing_slash ? "" : "/");
    if (written <= 0 || (size_t) written >= sizeof(url)) {
        return false;
    }
    return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, url, -1, output,
                               (int) capacity) > 0;
}

static void close_handle(HINTERNET* handle)
{
    if (*handle != NULL) {
        WinHttpCloseHandle(*handle);
        *handle = NULL;
    }
}

bool nativeTurnCredentials_Fetch(const char* signaling_endpoint,
                                 NativeTurnCredentials* credentials)
{
    if (credentials == NULL) {
        return false;
    }
    wchar_t url[560];
    if (!build_url(signaling_endpoint, url, sizeof(url) / sizeof(url[0]))) {
        return false;
    }
    URL_COMPONENTS components = {
        .dwStructSize = sizeof(components),
        .dwHostNameLength = (DWORD) -1,
        .dwUrlPathLength = (DWORD) -1,
        .dwExtraInfoLength = (DWORD) -1,
    };
    if (!WinHttpCrackUrl(url, 0, 0, &components) ||
        (components.nScheme != INTERNET_SCHEME_HTTP &&
         components.nScheme != INTERNET_SCHEME_HTTPS))
    {
        return false;
    }
    wchar_t host[256];
    wchar_t object[560];
    if (components.dwHostNameLength == 0 ||
        components.dwHostNameLength >= sizeof(host) / sizeof(host[0]) ||
        components.dwUrlPathLength + components.dwExtraInfoLength >=
            sizeof(object) / sizeof(object[0]))
    {
        return false;
    }
    memcpy(host, components.lpszHostName,
           components.dwHostNameLength * sizeof(wchar_t));
    host[components.dwHostNameLength] = L'\0';
    memcpy(object, components.lpszUrlPath,
           components.dwUrlPathLength * sizeof(wchar_t));
    if (components.dwExtraInfoLength != 0) {
        memcpy(object + components.dwUrlPathLength, components.lpszExtraInfo,
               components.dwExtraInfoLength * sizeof(wchar_t));
    }
    object[components.dwUrlPathLength + components.dwExtraInfoLength] = L'\0';

    HINTERNET session =
        WinHttpOpen(L"Freak Melee/0.1", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    HINTERNET connection = NULL;
    HINTERNET request = NULL;
    bool success = false;
    if (session == NULL ||
        !WinHttpSetTimeouts(session, HTTP_TIMEOUT_MS, HTTP_TIMEOUT_MS,
                            HTTP_TIMEOUT_MS, HTTP_TIMEOUT_MS))
    {
        goto cleanup;
    }
    connection = WinHttpConnect(session, host, components.nPort, 0);
    if (connection == NULL) {
        goto cleanup;
    }
    const DWORD flags =
        components.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
    request = WinHttpOpenRequest(connection, L"GET", object, NULL,
                                 WINHTTP_NO_REFERER,
                                 WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (request == NULL ||
        !WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(request, NULL))
    {
        goto cleanup;
    }
    DWORD status = 0;
    DWORD status_size = sizeof(status);
    if (!WinHttpQueryHeaders(
            request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size,
            WINHTTP_NO_HEADER_INDEX) ||
        status != 200)
    {
        goto cleanup;
    }

    uint8_t bytes[NATIVE_TURN_WIRE_MAX_BYTES];
    size_t total = 0;
    for (;;) {
        uint8_t overflow;
        void* destination = total < sizeof(bytes) ? bytes + total : &overflow;
        DWORD capacity_left =
            total < sizeof(bytes) ? (DWORD) (sizeof(bytes) - total) : 1;
        DWORD received = 0;
        if (!WinHttpReadData(request, destination, capacity_left, &received)) {
            goto cleanup;
        }
        if (received == 0) {
            break;
        }
        if (total == sizeof(bytes)) {
            goto cleanup;
        }
        total += received;
    }
    success = nativeTurnCredentials_Decode(bytes, total, credentials);

cleanup:
    close_handle(&request);
    close_handle(&connection);
    close_handle(&session);
    return success;
}

#endif
