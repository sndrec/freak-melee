#include "native_account_internal.h"
#include <wincrypt.h>
#include <cstring>

bool nativeAccount_Delete(const wchar_t* path)
{
    return DeleteFileW(path) || GetLastError() == ERROR_FILE_NOT_FOUND;
}
bool nativeAccount_Store(const wchar_t* path, const NativeAccountSecrets& secrets)
{
    DATA_BLOB plain{sizeof(secrets), reinterpret_cast<BYTE*>(const_cast<NativeAccountSecrets*>(&secrets))}, encrypted{};
    if (!CryptProtectData(&plain, L"Freak Melee account", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &encrypted)) return false;
    wchar_t temporary[1024]{};
    bool ok = swprintf_s(temporary, L"%s.tmp", path) > 0;
    HANDLE file = ok ? CreateFileW(temporary, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, nullptr) : INVALID_HANDLE_VALUE;
    DWORD written = 0;
    ok = file != INVALID_HANDLE_VALUE && WriteFile(file, encrypted.pbData, encrypted.cbData, &written, nullptr) &&
        written == encrypted.cbData && FlushFileBuffers(file);
    if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    if (ok) ok = MoveFileExW(temporary, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    if (!ok && *temporary) DeleteFileW(temporary);
    SecureZeroMemory(encrypted.pbData, encrypted.cbData);
    LocalFree(encrypted.pbData);
    return ok;
}
bool nativeAccount_Load(const wchar_t* path, const char* origin, NativeAccountSecrets& secrets)
{
    BYTE bytes[4096];
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{}; DWORD read = 0;
    bool ok = GetFileSizeEx(file, &size) && size.QuadPart > 0 && size.QuadPart <= sizeof(bytes) &&
        ReadFile(file, bytes, DWORD(size.QuadPart), &read, nullptr) && read == size.QuadPart;
    CloseHandle(file);
    if (!ok) return false;
    DATA_BLOB encrypted{read, bytes}, plain{};
    ok = CryptUnprotectData(&encrypted, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &plain) && plain.cbData == sizeof(secrets);
    NativeAccountSecrets loaded{};
    if (ok) {
        std::memcpy(&loaded, plain.pbData, sizeof(loaded));
        char canonical[25];
        ok = std::memchr(loaded.origin, 0, sizeof(loaded.origin)) && !std::strcmp(loaded.origin, origin) &&
            std::memchr(loaded.handle, 0, sizeof(loaded.handle)) && nativeAccount_Handle(loaded.handle, canonical) &&
            !std::strcmp(canonical, loaded.handle) && loaded.token[43] == 0 && nativeAccount_Secret(loaded.token) &&
            loaded.recovery[43] == 0 && (!*loaded.recovery || nativeAccount_Secret(loaded.recovery));
    }
    if (ok) secrets = loaded;
    SecureZeroMemory(&loaded, sizeof(loaded));
    if (plain.pbData) { SecureZeroMemory(plain.pbData, plain.cbData); LocalFree(plain.pbData); }
    SecureZeroMemory(bytes, sizeof(bytes));
    return ok;
}
