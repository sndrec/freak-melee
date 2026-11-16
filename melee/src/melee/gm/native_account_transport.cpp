#include "native_account_transport.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <cwchar>

namespace {
    SRWLOCK lock = SRWLOCK_INIT;
    std::atomic<uint64_t> generation;
    struct Credentials {
        wchar_t host[256]; INTERNET_PORT port; INTERNET_SCHEME scheme;
        char handle[25], token[44];
    } credentials;
    struct GuestIdentity {
        wchar_t host[256]; INTERNET_PORT port; INTERNET_SCHEME scheme;
        char key[65];
    } guest;
    bool authority(const wchar_t* url, wchar_t (&host)[256], INTERNET_PORT& port, INTERNET_SCHEME& scheme)
    {
        URL_COMPONENTS parts{}; parts.dwStructSize = sizeof(parts);
        parts.dwHostNameLength = parts.dwUserNameLength = parts.dwPasswordLength = DWORD(-1);
        if (!WinHttpCrackUrl(url, 0, 0, &parts) || !parts.dwHostNameLength || parts.dwHostNameLength >= 256 ||
            parts.dwUserNameLength || parts.dwPasswordLength) return false;
        std::memcpy(host, parts.lpszHostName, parts.dwHostNameLength * sizeof(wchar_t)); host[parts.dwHostNameLength] = 0;
        if (parts.nScheme != INTERNET_SCHEME_HTTPS && !(parts.nScheme == INTERNET_SCHEME_HTTP &&
            (!_wcsicmp(host, L"localhost") || !_wcsicmp(host, L"127.0.0.1") || !_wcsicmp(host, L"[::1]")))) return false;
        port = parts.nPort; scheme = parts.nScheme; return true;
    }
}
bool nativeAccountTransport_InitGuest(const char* root, const char* origin)
{
    GuestIdentity next{}; wchar_t wide[1024], path[1024];
    if (!root || !origin || !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, origin, -1, wide, 1024) ||
        !authority(wide, next.host, next.port, next.scheme) ||
        !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, root, -1, wide, 1024) ||
        swprintf_s(path, L"%s/guest-identity.bin", wide) < 0) return false;
    CreateDirectoryW(wide, nullptr);
    HANDLE file = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    OVERLAPPED region{}; unsigned char key[32]{}; DWORD transferred = 0; LARGE_INTEGER size{};
    bool locked = LockFileEx(file, LOCKFILE_EXCLUSIVE_LOCK, 0, sizeof(key), 0, &region) != FALSE;
    bool ok = locked && GetFileSizeEx(file, &size);
    if (ok && size.QuadPart == 0) {
        ok = BCryptGenRandom(nullptr, key, sizeof(key), BCRYPT_USE_SYSTEM_PREFERRED_RNG) >= 0 &&
            WriteFile(file, key, sizeof(key), &transferred, nullptr) && transferred == sizeof(key) && FlushFileBuffers(file);
    } else if (ok) {
        ok = size.QuadPart == sizeof(key) && ReadFile(file, key, sizeof(key), &transferred, nullptr) && transferred == sizeof(key);
    }
    if (locked) UnlockFileEx(file, 0, sizeof(key), 0, &region);
    CloseHandle(file);
    if (ok) {
        const char hex[] = "0123456789abcdef";
        for (unsigned i = 0; i < sizeof(key); ++i) {
            next.key[i * 2] = hex[key[i] >> 4]; next.key[i * 2 + 1] = hex[key[i] & 15];
        }
        AcquireSRWLockExclusive(&lock);
        if (std::memcmp(&guest, &next, sizeof(next))) {
            guest = next; generation.fetch_add(1, std::memory_order_release);
        }
        ReleaseSRWLockExclusive(&lock);
    }
    SecureZeroMemory(key, sizeof(key)); SecureZeroMemory(&next, sizeof(next));
    return ok;
}
bool nativeAccountTransport_Publish(const char* origin, const char* handle, const char* token)
{
    Credentials next; SecureZeroMemory(&next, sizeof(next)); wchar_t wide[512];
    const size_t size = strnlen(handle, sizeof(next.handle));
    if (size < 3 || size > 24 || strnlen(token, 44) != 43 ||
        !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, origin, -1, wide, 512) ||
        !authority(wide, next.host, next.port, next.scheme)) return false;
    for (size_t i = 0; i < size; ++i)
        if (!(handle[i] >= 'a' && handle[i] <= 'z') && !(handle[i] >= '0' && handle[i] <= '9') && handle[i] != '_') return false;
    for (unsigned i = 0; i < 43; ++i)
        if (!(token[i] >= 'a' && token[i] <= 'z') && !(token[i] >= 'A' && token[i] <= 'Z') &&
            !(token[i] >= '0' && token[i] <= '9') && token[i] != '_' && token[i] != '-') return false;
    std::memcpy(next.handle, handle, size + 1); std::memcpy(next.token, token, 44);
    AcquireSRWLockExclusive(&lock);
    if (std::memcmp(&next, &credentials, sizeof(next))) {
        SecureZeroMemory(&credentials, sizeof(credentials)); credentials = next;
        generation.fetch_add(1, std::memory_order_release);
    }
    ReleaseSRWLockExclusive(&lock);
    SecureZeroMemory(&next, sizeof(next)); return true;
}
void nativeAccountTransport_Clear()
{
    AcquireSRWLockExclusive(&lock);
    if (*credentials.token) {
        SecureZeroMemory(&credentials, sizeof(credentials)); generation.fetch_add(1, std::memory_order_release);
    }
    ReleaseSRWLockExclusive(&lock);
}
uint64_t nativeAccountTransport_Generation() { return generation.load(std::memory_order_acquire); }
bool nativeAccountTransport_Headers(const wchar_t* url, wchar_t* headers, size_t capacity)
{
    if (!headers || !capacity) return false;
    *headers = 0;
    wchar_t host[256]; INTERNET_PORT port; INTERNET_SCHEME scheme;
    if (!authority(url, host, port, scheme)) return false;
    AcquireSRWLockShared(&lock);
    bool ok = *credentials.token && !_wcsicmp(host, credentials.host) && port == credentials.port && scheme == credentials.scheme;
    if (ok) ok = capacity >= 128 && swprintf_s(headers, capacity, L"Authorization: Bearer %hs\r\nX-Freak-Account: %hs\r\n", credentials.token, credentials.handle) > 0;
    else if (*guest.key && !_wcsicmp(host, guest.host) && port == guest.port && scheme == guest.scheme)
        ok = swprintf_s(headers, capacity, L"X-Freak-Guest: %hs\r\n", guest.key) > 0;
    ReleaseSRWLockShared(&lock); return ok;
}
