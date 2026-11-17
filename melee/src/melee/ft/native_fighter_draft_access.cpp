#include "native_fighter_draft_access.h"
#ifdef MELEE_NATIVE
#include "native_fighter_family_project.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace {
bool fail(std::string& error, const std::string& text)
{
    error = text; return false;
}

bool acquire_directory(const char* path, NativeFighterDraftLease& lease, std::string& error)
{
    HANDLE directory = CreateFileA(path, FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (directory == INVALID_HANDLE_VALUE) return fail(error, "Could not identify draft directory.");
    BY_HANDLE_FILE_INFORMATION identity{};
    bool ok = GetFileInformationByHandle(directory, &identity) != 0;
    CloseHandle(directory);
    if (!ok) return fail(error, "Could not identify draft directory.");
    char name[128];
    std::snprintf(name, sizeof(name), "Local\\FreakMeleeDraft-%08lx-%08lx%08lx",
        identity.dwVolumeSerialNumber, identity.nFileIndexHigh, identity.nFileIndexLow);
    HANDLE mutex = CreateMutexA(nullptr, FALSE, name);
    if (!mutex) return fail(error, "Could not open draft authoring lease.");
    DWORD wait = WaitForSingleObject(mutex, 0);
    if (wait != WAIT_OBJECT_0 && wait != WAIT_ABANDONED) {
        CloseHandle(mutex);
        return fail(error, "This draft is owned by an open Workshop or another authoring process.");
    }
    lease.handles[lease.count++] = mutex;
    return true;
}

bool ignored(const std::string& name)
{
    return name == ".workshop-transaction" || name == ".workshop-history" ||
        (name.size() >= 9 && name.compare(name.size() - 9, 9, ".previous") == 0) ||
        (name.size() >= 4 && name.compare(name.size() - 4, 4, ".tmp") == 0) ||
        name.find(".next-") != std::string::npos || name.compare(0, 16, "behavior.editor.") == 0;
}

bool files(const std::string& root, const std::string& relative,
           std::vector<std::string>& result, unsigned depth, std::string& error)
{
    if (depth > 12 || result.size() > 8192) return fail(error, "Draft directory exceeds inspection limits.");
    WIN32_FIND_DATAA info{};
    const std::string parent = root + (relative.empty() ? "" : "/" + relative);
    HANDLE search = FindFirstFileA((parent + "/*").c_str(), &info);
    if (search == INVALID_HANDLE_VALUE) return GetLastError() == ERROR_FILE_NOT_FOUND ||
        fail(error, "Could not enumerate draft files.");
    bool ok = true;
    do {
        const std::string name = info.cFileName;
        if (name == "." || name == ".." || ignored(name)) continue;
        if (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            ok = fail(error, "Reparse points inside a draft are not supported: " + name); break;
        }
        const std::string path = relative.empty() ? name : relative + "/" + name;
        if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ok = files(root, path, result, depth + 1, error);
        else if (result.size() < 8192) result.push_back(path);
        else ok = fail(error, "Draft directory exceeds inspection limits.");
        if (!ok) break;
    } while (FindNextFileA(search, &info));
    if (ok && GetLastError() != ERROR_NO_MORE_FILES) ok = fail(error, "Draft enumeration failed.");
    FindClose(search);
    return ok;
}
}

extern "C" void nativeFighterDraft_Release(NativeFighterDraftLease* lease)
{
    if (!lease) return;
    while (lease->count) {
        HANDLE mutex = lease->handles[--lease->count];
        ReleaseMutex(mutex); CloseHandle(mutex); lease->handles[lease->count] = nullptr;
    }
}

extern "C" bool nativeFighterDraft_Acquire(const char* path, NativeFighterDraftLease* lease,
                                          char* error, size_t capacity)
{
    std::string problem;
    NativeFighterFamilyProjectInfo family{};
    bool ok = path && lease && !lease->count && nativeFighterFamilyProject_ReadInfo(path, &family);
    if (!ok) {
        if (error && capacity) std::snprintf(error, capacity, "A lease needs a valid draft and an empty lease owner.");
        return false;
    }
    if (ok) ok = acquire_directory(path, *lease, problem);
    for (unsigned i = 0; ok && i < family.member_count; ++i)
        ok = acquire_directory(family.paths[i], *lease, problem);
    // Recovery is part of opening an exclusive authoring session, before any
    // asset/source loads. A member opened directly overlaps its family's lease.
    for (unsigned i = 0; ok && i < family.member_count; ++i)
        ok = NativeDraftAccess::recover(family.paths[i], problem);
    if (!ok && lease) nativeFighterDraft_Release(lease);
    if (error && capacity) std::snprintf(error, capacity, "%s", problem.c_str());
    return ok;
}

namespace NativeDraftAccess {
bool Lease::acquire(const char* path, std::string& error)
{
    char message[512]{};
    bool ok = nativeFighterDraft_Acquire(path, &value, message, sizeof(message));
    error = message; return ok;
}

bool revision(const std::string& root, std::string& hash, std::string& error)
{
    std::vector<std::string> names;
    if (!files(root, "", names, 0, error)) return false;
    std::sort(names.begin(), names.end());
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE digest = nullptr;
    bool ok = BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) == 0 &&
        BCryptCreateHash(algorithm, &digest, nullptr, 0, nullptr, 0, 0) == 0;
    unsigned char buffer[65536], bytes[32];
    uint64_t total = 0;
    for (const auto& name : names) {
        if (!ok) break;
        HANDLE file = CreateFileA((root + "/" + name).c_str(), GENERIC_READ, FILE_SHARE_READ,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        LARGE_INTEGER size{};
        ok = file != INVALID_HANDLE_VALUE && GetFileSizeEx(file, &size) && size.QuadPart >= 0;
        uint64_t length = (uint64_t)size.QuadPart;
        total += length;
        if (total > 1024ull * 1024 * 1024) ok = false;
        // Explicit lengths separate file names and content without collisions
        // caused by concatenating different directory/file layouts.
        uint64_t name_length = name.size();
        if (ok) ok = BCryptHashData(digest, (PUCHAR)&name_length, sizeof(name_length), 0) == 0 &&
            BCryptHashData(digest, (PUCHAR)name.data(), (ULONG)name.size(), 0) == 0 &&
            BCryptHashData(digest, (PUCHAR)&length, sizeof(length), 0) == 0;
        uint64_t read = 0;
        while (ok && read < length) {
            DWORD chunk = 0, wanted = (DWORD)std::min<uint64_t>(sizeof(buffer), length - read);
            ok = ReadFile(file, buffer, wanted, &chunk, nullptr) && chunk == wanted &&
                BCryptHashData(digest, buffer, chunk, 0) == 0;
            read += chunk;
        }
        if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    }
    if (ok) ok = BCryptFinishHash(digest, bytes, sizeof(bytes), 0) == 0;
    if (digest) BCryptDestroyHash(digest);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    if (!ok) return fail(error, "Could not hash a stable draft revision; close external writers and retry.");
    static const char hex[] = "0123456789abcdef";
    hash.resize(64);
    for (size_t i = 0; i < 32; ++i) { hash[i * 2] = hex[bytes[i] >> 4]; hash[i * 2 + 1] = hex[bytes[i] & 15]; }
    return true;
}
}
#endif
