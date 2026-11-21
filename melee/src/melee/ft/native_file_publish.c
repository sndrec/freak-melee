#include "native_file_publish.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

NativeFileResult nativeFile_PublishWide(const wchar_t* path, const void* bytes, size_t size,
                                        NativeFileExisting existing, NativeFileIdentity* identity)
{
    if (!path || !*path || (!bytes && size) || size > MAXDWORD)
        return NATIVE_FILE_INVALID;
    DWORD count = GetFullPathNameW(path, 0, NULL, NULL);
    if (!count || count > 32768) return NATIVE_FILE_INVALID;
    size_t capacity = offsetof(FILE_RENAME_INFO, FileName) + count * sizeof(wchar_t);
    if (capacity < sizeof(FILE_RENAME_INFO)) capacity = sizeof(FILE_RENAME_INFO);
    FILE_RENAME_INFO* rename = calloc(1, capacity);
    if (!rename) return NATIVE_FILE_FAILED;

    NativeFileResult result = NATIVE_FILE_FAILED;
    HANDLE file = INVALID_HANDLE_VALUE;
    wchar_t* temporary = NULL;
    wchar_t* leaf = NULL;
    DWORD length = GetFullPathNameW(path, count, rename->FileName, &leaf);
    if (!length || length >= count || !leaf || !*leaf) {
        result = NATIVE_FILE_INVALID;
        goto done;
    }
    rename->ReplaceIfExists = existing == NATIVE_FILE_REPLACE;
    rename->FileNameLength = length * sizeof(wchar_t);
    if (!rename->ReplaceIfExists && GetFileAttributesW(rename->FileName) != INVALID_FILE_ATTRIBUTES) {
        result = NATIVE_FILE_UNAVAILABLE;
        goto done;
    }
    static const wchar_t hex[] = L"0123456789abcdef";
    wchar_t name[] = L".freak-melee-00000000000000000000000000000000.tmp";
    unsigned char random[16];
    if (BCryptGenRandom(NULL, random, sizeof(random), BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0)
        goto done;
    for (unsigned i = 0; i < sizeof(random); ++i) {
        size_t n = (sizeof(L".freak-melee-") / sizeof(wchar_t) - 1) + 2 * i;
        name[n] = hex[random[i] >> 4];
        name[n + 1] = hex[random[i] & 15];
    }
    size_t prefix = (size_t)(leaf - rename->FileName);
    temporary = malloc(prefix * sizeof(wchar_t) + sizeof(name));
    if (!temporary) goto done;
    memcpy(temporary, rename->FileName, prefix * sizeof(wchar_t));
    memcpy(temporary + prefix, name, sizeof(name));
    file = CreateFileW(temporary, GENERIC_WRITE | FILE_READ_ATTRIBUTES | DELETE, 0, NULL, CREATE_NEW,
                       FILE_ATTRIBUTE_NORMAL | SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        result = NATIVE_FILE_UNAVAILABLE;
        goto done;
    }
    DWORD written = 0;
    BY_HANDLE_FILE_INFORMATION info;
    if ((!identity || GetFileInformationByHandle(file, &info)) &&
        WriteFile(file, bytes, (DWORD)size, &written, NULL) && written == size && FlushFileBuffers(file)) {
        if (SetFileInformationByHandle(file, FileRenameInfo, rename, (DWORD)capacity)) {
            if (identity) *identity = (NativeFileIdentity){info.dwVolumeSerialNumber, info.nFileIndexHigh,
                info.nFileIndexLow, info.ftCreationTime.dwHighDateTime, info.ftCreationTime.dwLowDateTime, true};
            result = NATIVE_FILE_PUBLISHED;
        } else if (existing == NATIVE_FILE_PRESERVE &&
                   GetFileAttributesW(rename->FileName) != INVALID_FILE_ATTRIBUTES) {
            result = NATIVE_FILE_UNAVAILABLE;
        }
    }
    if (result != NATIVE_FILE_PUBLISHED) {
        FILE_DISPOSITION_INFO discard = {TRUE};
        SetFileInformationByHandle(file, FileDispositionInfo, &discard, sizeof(discard));
    }
done:
    if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    free(temporary);
    free(rename);
    return result;
}

bool nativeFile_PublishLocal(const char* path, const void* bytes, size_t size,
                             NativeFileExisting existing, NativeFileIdentity* identity)
{
    if (!path || !*path) return false;
    int count = MultiByteToWideChar(CP_ACP, 0, path, -1, NULL, 0);
    if (count <= 1 || count > 32768) return false;
    wchar_t* wide = malloc((size_t)count * sizeof(wchar_t));
    if (!wide) return false;
    bool ok = MultiByteToWideChar(CP_ACP, 0, path, -1, wide, count) == count &&
        nativeFile_PublishWide(wide, bytes, size, existing, identity) == NATIVE_FILE_PUBLISHED;
    free(wide);
    return ok;
}

void nativeFile_RemoveOwned(const char* path, const NativeFileIdentity* identity)
{
    if (!identity || !identity->valid) return;
    HANDLE file = CreateFileA(path, FILE_READ_ATTRIBUTES | DELETE, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                              FILE_FLAG_OPEN_REPARSE_POINT | SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS, NULL);
    if (file == INVALID_HANDLE_VALUE) return;
    BY_HANDLE_FILE_INFORMATION info;
    if (GetFileInformationByHandle(file, &info) && !(info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
        info.dwVolumeSerialNumber == identity->volume && info.nFileIndexHigh == identity->index_hi &&
        info.nFileIndexLow == identity->index_lo && info.ftCreationTime.dwHighDateTime == identity->created_hi &&
        info.ftCreationTime.dwLowDateTime == identity->created_lo) {
        FILE_DISPOSITION_INFO remove = {TRUE};
        SetFileInformationByHandle(file, FileDispositionInfo, &remove, sizeof(remove));
    }
    CloseHandle(file);
}
