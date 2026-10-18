#include "native_content_store.h"
#include "native_content_store_internal.h"

#ifdef MELEE_NATIVE

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

enum {
    COSTUME_HEADER_BYTES = 76,
    CUSTOM_COSTUME_DIRECTORY = COSTUME_HEADER_BYTES + NATIVE_CONTENT_ID_BYTES,
    COSTUME_MEMBER_BYTES = NATIVE_FIGHTER_CODE_KEY_BYTES + 4,
    CUSTOM_COSTUME_HEADER_BYTES = CUSTOM_COSTUME_DIRECTORY +
        NATIVE_FIGHTER_FAMILY_MEMBERS * COSTUME_MEMBER_BYTES,
    INSTALLED_MARKER_BYTES = 4 + NATIVE_CONTENT_ID_BYTES,
};

static const uint8_t package_magic[4] = { 'F', 'M', 'C', 'O' };
static const uint8_t installed_magic[4] = { 'F', 'M', 'I', 'N' };
static LONG temporary_number;

static uint32_t read_u32(const uint8_t* bytes)
{
    return (uint32_t) bytes[0] | ((uint32_t) bytes[1] << 8) |
           ((uint32_t) bytes[2] << 16) | ((uint32_t) bytes[3] << 24);
}

static void write_u32(uint8_t* bytes, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) {
        bytes[i] = (uint8_t) (value >> (i * 8));
    }
}

static bool zero_padding(const uint8_t* bytes, size_t first, size_t end)
{
    for (size_t i = first; i < end; ++i) {
        if (bytes[i] != 0) {
            return false;
        }
    }
    return true;
}

bool nativeContent_Hash(const void* package, size_t size, NativeContentId* id)
{
    if (package == NULL || id == NULL || size > ULONG_MAX) {
        return false;
    }
    BCRYPT_ALG_HANDLE algorithm = NULL;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM,
                                   NULL, 0) < 0) {
        return false;
    }
    bool ok = BCryptHash(algorithm, NULL, 0, (PUCHAR) package, (ULONG) size,
                         id->bytes, sizeof(id->bytes)) >= 0;
    BCryptCloseAlgorithmProvider(algorithm, 0);
    return ok;
}

void nativeContentStore_IdHex(const NativeContentId* id, char hex[65])
{
    static const char digits[] = "0123456789abcdef";
    if (id == NULL || hex == NULL) {
        return;
    }
    for (unsigned i = 0; i < NATIVE_CONTENT_ID_BYTES; ++i) {
        hex[i * 2] = digits[id->bytes[i] >> 4];
        hex[i * 2 + 1] = digits[id->bytes[i] & 15];
    }
    hex[64] = '\0';
}

static int hex_digit(char digit)
{
    if (digit >= '0' && digit <= '9') {
        return digit - '0';
    }
    if (digit >= 'a' && digit <= 'f') {
        return digit - 'a' + 10;
    }
    return -1;
}

bool nativeContentStore_IdFromHex(const char* hex, NativeContentId* id)
{
    if (hex == NULL || id == NULL || strlen(hex) != 64) {
        return false;
    }
    for (unsigned i = 0; i < NATIVE_CONTENT_ID_BYTES; ++i) {
        int high = hex_digit(hex[i * 2]);
        int low = hex_digit(hex[i * 2 + 1]);
        if (high < 0 || low < 0) {
            return false;
        }
        id->bytes[i] = (uint8_t) ((high << 4) | low);
    }
    return true;
}

static bool costume_key(const char* key)
{
    size_t length = strnlen(key, NATIVE_FIGHTER_CODE_KEY_BYTES);
    if (length == 0 || length == NATIVE_FIGHTER_CODE_KEY_BYTES) return false;
    for (size_t i = 0; i < length; ++i) {
        char c = key[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-')) return false;
    }
    return true;
}

bool nativeContentStore_BuildCostume(const NativeCostumePackage* costume,
                                    const NativeCostumePackageData* data,
                                    uint8_t** package, size_t* package_size,
                                    NativeContentId* id)
{
    if (!costume || !data || !package || !package_size || !id) return false;
    const NativeContentId empty_id = {0};
    bool custom = memcmp(&costume->fighter_id, &empty_id, sizeof(empty_id)) != 0;
    unsigned count = costume->member_count;
    size_t header = custom ? CUSTOM_COSTUME_HEADER_BYTES : COSTUME_HEADER_BYTES;
    size_t title = strnlen(costume->title, NATIVE_CONTENT_TITLE_BYTES);
    if (!count || count > NATIVE_FIGHTER_FAMILY_MEMBERS || !title ||
        title == NATIVE_CONTENT_TITLE_BYTES ||
        (custom ? (costume->character || costume->base_costume) : count != 1)) return false;
    size_t total = header;
    for (unsigned m = 0; m < count; ++m) {
        if (!data->members[m].dat || data->members[m].size < 32 ||
            data->members[m].size > NATIVE_CONTENT_MAX_PACKAGE_BYTES - total ||
            (custom && (!costume_key(costume->member_keys[m]) ||
                (m && !strcmp(costume->member_keys[0], costume->member_keys[m]))))) return false;
        total += data->members[m].size;
    }
    for (unsigned m = count; m < NATIVE_FIGHTER_FAMILY_MEMBERS; ++m)
        if (data->members[m].dat || data->members[m].size || costume->member_keys[m][0]) return false;
    if (!custom && costume->member_keys[0][0]) return false;
    uint8_t* built = calloc(1, total);
    if (!built) return false;
    memcpy(built, package_magic, sizeof(package_magic));
    built[4] = custom ? 3 : 1;
    built[5] = custom ? costume->model_variant : costume->character;
    built[6] = custom ? count : costume->base_costume;
    built[7] = title;
    write_u32(built + 8, (uint32_t)(total - header));
    memcpy(built + 12, costume->title, title);
    if (custom) memcpy(built + COSTUME_HEADER_BYTES, costume->fighter_id.bytes, NATIVE_CONTENT_ID_BYTES);
    size_t offset = header;
    for (unsigned m = 0; m < count; ++m) {
        if (custom) {
            uint8_t* entry = built + CUSTOM_COSTUME_DIRECTORY + m * COSTUME_MEMBER_BYTES;
            memcpy(entry, costume->member_keys[m], strlen(costume->member_keys[m]));
            write_u32(entry + NATIVE_FIGHTER_CODE_KEY_BYTES, data->members[m].size);
        }
        memcpy(built + offset, data->members[m].dat, data->members[m].size);
        offset += data->members[m].size;
    }
    if (!nativeContent_Hash(built, total, id)) { free(built); return false; }
    *package = built;
    *package_size = total;
    return true;
}

bool nativeContentStore_ParseCostume(const void* package, size_t package_size,
                                    NativeCostumePackage* costume,
                                    NativeCostumePackageData* data)
{
    if (!package || !costume || !data || package_size < COSTUME_HEADER_BYTES + 32 ||
        package_size > NATIVE_CONTENT_MAX_PACKAGE_BYTES) return false;
    const uint8_t* bytes = package;
    bool custom = bytes[4] == 3;
    size_t header = custom ? CUSTOM_COSTUME_HEADER_BYTES : COSTUME_HEADER_BYTES;
    unsigned count = custom ? bytes[6] : 1;
    unsigned title = bytes[7];
    if (memcmp(bytes, package_magic, sizeof(package_magic)) ||
        (bytes[4] != 1 && !custom) || package_size < header + 32 ||
        !count || count > NATIVE_FIGHTER_FAMILY_MEMBERS || !title ||
        title >= NATIVE_CONTENT_TITLE_BYTES || memchr(bytes + 12, 0, title) ||
        !zero_padding(bytes, 12 + title, COSTUME_HEADER_BYTES) ||
        read_u32(bytes + 8) != package_size - header ||
        (custom && zero_padding(bytes, COSTUME_HEADER_BYTES, CUSTOM_COSTUME_DIRECTORY))) return false;
    NativeCostumePackage parsed = {0};
    NativeCostumePackageData spans = {0};
    parsed.character = custom ? 0 : bytes[5];
    parsed.base_costume = custom ? 0 : bytes[6];
    parsed.model_variant = custom ? bytes[5] : bytes[6];
    parsed.member_count = count;
    memcpy(parsed.title, bytes + 12, title);
    if (custom) memcpy(parsed.fighter_id.bytes, bytes + COSTUME_HEADER_BYTES, NATIVE_CONTENT_ID_BYTES);
    size_t offset = header;
    for (unsigned m = 0; m < count; ++m) {
        size_t size = package_size - header;
        if (custom) {
            const uint8_t* entry = bytes + CUSTOM_COSTUME_DIRECTORY + m * COSTUME_MEMBER_BYTES;
            memcpy(parsed.member_keys[m], entry, NATIVE_FIGHTER_CODE_KEY_BYTES);
            if (!costume_key(parsed.member_keys[m]) ||
                !zero_padding(entry, strlen(parsed.member_keys[m]), NATIVE_FIGHTER_CODE_KEY_BYTES) ||
                (m && !strcmp(parsed.member_keys[0], parsed.member_keys[m]))) return false;
            size = read_u32(entry + NATIVE_FIGHTER_CODE_KEY_BYTES);
        }
        if (size < 32 || size > package_size - offset) return false;
        spans.members[m] = (NativeCostumeMemberData){bytes + offset, size};
        offset += size;
    }
    if (offset != package_size || (custom && !zero_padding(bytes,
        CUSTOM_COSTUME_DIRECTORY + count * COSTUME_MEMBER_BYTES, header))) return false;
    *costume = parsed;
    *data = spans;
    return true;
}

static bool valid_package_structure(const void* package, size_t package_size)
{
    NativeCostumePackage costume;
    NativeFighterPackage fighter;
    NativeFighterPackageData fighter_data;
    NativeCostumePackageData costume_data;
    return nativeContentStore_ParseCostume(package, package_size, &costume,
                                           &costume_data) ||
           nativeContentStore_ParseFighter(package, package_size, &fighter,
                                           &fighter_data);
}

static bool make_path(char path[MAX_PATH], const char* root,
                      const char* directory, const NativeContentId* id,
                      const char* suffix)
{
    if (path == NULL || root == NULL || directory == NULL || id == NULL ||
        suffix == NULL) {
        return false;
    }
    char hex[65];
    nativeContentStore_IdHex(id, hex);
    int length = snprintf(path, MAX_PATH, "%s\\%s\\%s%s", root,
                          directory, hex, suffix);
    return length > 0 && length < MAX_PATH;
}

static bool ensure_directory(const char* path)
{
    char absolute[MAX_PATH];
    DWORD length = GetFullPathNameA(path, MAX_PATH, absolute, NULL);
    if (length < 3 || length >= MAX_PATH || absolute[1] != ':' ||
        absolute[2] != '\\') {
        return false;
    }
    for (size_t i = 3; absolute[i] != '\0'; ++i) {
        if (absolute[i] != '\\') {
            continue;
        }
        absolute[i] = '\0';
        bool ok = CreateDirectoryA(absolute, NULL) ||
                  GetLastError() == ERROR_ALREADY_EXISTS;
        absolute[i] = '\\';
        if (!ok) {
            return false;
        }
    }
    return CreateDirectoryA(absolute, NULL) ||
           GetLastError() == ERROR_ALREADY_EXISTS;
}

static bool ensure_subdirectory(const char* root, const char* name)
{
    char path[MAX_PATH];
    int length = snprintf(path, sizeof(path), "%s\\%s", root, name);
    return length > 0 && length < sizeof(path) && ensure_directory(path);
}

static bool read_file(const char* path, uint8_t** bytes, size_t* size,
                      size_t maximum)
{
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    LARGE_INTEGER length;
    bool ok = GetFileSizeEx(file, &length) && length.QuadPart > 0 &&
              length.QuadPart <= maximum;
    uint8_t* result = ok ? malloc((size_t) length.QuadPart) : NULL;
    ok = ok && result != NULL;
    size_t offset = 0;
    while (ok && offset < (size_t) length.QuadPart) {
        DWORD received = 0;
        DWORD wanted = (DWORD) ((size_t) length.QuadPart - offset);
        ok = ReadFile(file, result + offset, wanted, &received, NULL) &&
             received != 0;
        offset += received;
    }
    ok = CloseHandle(file) && ok;
    if (!ok) {
        free(result);
        return false;
    }
    *bytes = result;
    *size = offset;
    return true;
}

static bool write_once(const char* path, const void* bytes, size_t size)
{
    char temporary[MAX_PATH];
    LONG number = InterlockedIncrement(&temporary_number);
    int length = snprintf(temporary, sizeof(temporary), "%s.%lu.%ld.tmp",
                          path, GetCurrentProcessId(), number);
    if (length <= 0 || length >= sizeof(temporary)) {
        return false;
    }
    HANDLE file = CreateFileA(temporary, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    const uint8_t* source = bytes;
    size_t offset = 0;
    bool ok = true;
    while (ok && offset < size) {
        DWORD written = 0;
        DWORD wanted = (DWORD) (size - offset);
        ok = WriteFile(file, source + offset, wanted, &written, NULL) &&
             written != 0;
        offset += written;
    }
    ok = ok && FlushFileBuffers(file);
    ok = CloseHandle(file) && ok;
    if (ok) {
        ok = MoveFileExA(temporary, path,
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    }
    if (!ok) {
        DeleteFileA(temporary);
    }
    return ok;
}

bool nativeContentStore_Read(const char* root, const NativeContentId* id,
                             uint8_t** package, size_t* package_size)
{
    if (package == NULL || package_size == NULL) {
        return false;
    }
    char path[MAX_PATH];
    if (!make_path(path, root, "cache", id, ".fmc")) {
        return false;
    }
    uint8_t* bytes = NULL;
    size_t size = 0;
    if (!read_file(path, &bytes, &size, NATIVE_CONTENT_MAX_PACKAGE_BYTES)) {
        return false;
    }
    NativeContentId actual;
    bool ok = valid_package_structure(bytes, size) &&
              nativeContent_Hash(bytes, size, &actual) &&
              memcmp(actual.bytes, id->bytes, sizeof(id->bytes)) == 0;
    if (!ok) {
        free(bytes);
        return false;
    }
    *package = bytes;
    *package_size = size;
    return true;
}

bool nativeContentStore_IsCached(const char* root, const NativeContentId* id)
{
    uint8_t* package = NULL;
    size_t size = 0;
    if (!nativeContentStore_Read(root, id, &package, &size)) {
        return false;
    }
    free(package);
    return true;
}

bool nativeContentStore_Cache(const char* root, const void* package,
                              size_t package_size,
                              const NativeContentId* expected_id,
                              NativeContentId* id)
{
    if (root == NULL || id == NULL ||
        !valid_package_structure(package, package_size) ||
        !nativeContent_Hash(package, package_size, id) ||
        (expected_id != NULL &&
         memcmp(id, expected_id, sizeof(*id)) != 0) ||
        !ensure_subdirectory(root, "cache")) {
        return false;
    }
    char path[MAX_PATH];
    if (!make_path(path, root, "cache", id, ".fmc")) {
        return false;
    }
    if (nativeContentStore_IsCached(root, id)) {
        return true;
    }
    return write_once(path, package, package_size) ||
           nativeContentStore_IsCached(root, id);
}

bool nativeContentStore_IsInstalled(const char* root, const NativeContentId* id)
{
    char path[MAX_PATH];
    if (!make_path(path, root, "installed", id, ".fmi")) {
        return false;
    }
    uint8_t* marker = NULL;
    size_t size = 0;
    bool ok = read_file(path, &marker, &size, INSTALLED_MARKER_BYTES) &&
              size == INSTALLED_MARKER_BYTES &&
              memcmp(marker, installed_magic, sizeof(installed_magic)) == 0 &&
              memcmp(marker + 4, id->bytes, sizeof(id->bytes)) == 0;
    free(marker);
    return ok && nativeContentStore_IsCached(root, id);
}

bool nativeContentStore_Install(const char* root, const NativeContentId* id)
{
    uint8_t* bytes = NULL;
    size_t size;
    if (root == NULL || id == NULL ||
        !nativeContentStore_Read(root, id, &bytes, &size)) return false;
    NativeContentId installs[2] = { *id };
    unsigned count = 1;
    NativeCostumePackage costume;
    NativeCostumePackageData costume_data;
    const NativeContentId empty = {0};
    bool dependent = nativeContentStore_ParseCostume(bytes, size, &costume,
        &costume_data) && memcmp(&costume.fighter_id, &empty, sizeof(empty)) != 0;
    free(bytes);
    if (dependent) {
        // Source/asset validation belongs to the accepting caller. The store
        // retains the exact typed dependency before publishing the outfit.
        installs[0] = costume.fighter_id;
        installs[1] = *id;
        count = 2;
        NativeFighterPackage fighter;
        NativeFighterPackageData data;
        if (!nativeContentStore_Read(root, &installs[0], &bytes, &size)) return false;
        bool valid = nativeContentStore_ParseFighter(bytes, size, &fighter, &data);
        free(bytes);
        if (!valid) return false;
    }
    if (!ensure_subdirectory(root, "installed")) return false;
    bool added[2] = {false};
    for (unsigned i = 0; i < count; ++i) {
        if (nativeContentStore_IsInstalled(root, &installs[i])) continue;
        char path[MAX_PATH];
        uint8_t marker[INSTALLED_MARKER_BYTES];
        memcpy(marker, installed_magic, sizeof(installed_magic));
        memcpy(marker + 4, installs[i].bytes, NATIVE_CONTENT_ID_BYTES);
        if (!make_path(path, root, "installed", &installs[i], ".fmi") ||
            (!write_once(path, marker, sizeof(marker)) &&
             !nativeContentStore_IsInstalled(root, &installs[i]))) {
            for (unsigned j = 0; j < i; ++j)
                if (added[j]) nativeContentStore_Uninstall(root, &installs[j]);
            return false;
        }
        added[i] = true;
    }
    return true;
}

bool nativeContentStore_Uninstall(const char* root, const NativeContentId* id)
{
    char path[MAX_PATH];
    if (root == NULL || id == NULL ||
        !make_path(path, root, "installed", id, ".fmi")) {
        return false;
    }
    if (!nativeContentStore_IsInstalled(root, id)) {
        return false;
    }
    return DeleteFileA(path) != 0;
}

bool nativeContentStore_ListInstalled(const char* root, NativeContentId* ids,
                                      size_t capacity, size_t* count)
{
    if (root == NULL || count == NULL || (capacity != 0 && ids == NULL)) {
        return false;
    }
    *count = 0;
    char pattern[MAX_PATH];
    int length = snprintf(pattern, sizeof(pattern), "%s\\installed\\*.fmi",
                          root);
    if (length <= 0 || length >= sizeof(pattern)) {
        return false;
    }
    WIN32_FIND_DATAA entry;
    HANDLE search = FindFirstFileA(pattern, &entry);
    if (search == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
    }
    bool ok = true;
    do {
        if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ||
            strlen(entry.cFileName) != 68 ||
            strcmp(entry.cFileName + 64, ".fmi") != 0) {
            continue;
        }
        char hex[65];
        memcpy(hex, entry.cFileName, 64);
        hex[64] = '\0';
        NativeContentId id;
        if (!nativeContentStore_IdFromHex(hex, &id) ||
            !nativeContentStore_IsInstalled(root, &id)) {
            continue;
        }
        if (*count == capacity) {
            ok = false;
            break;
        }
        ids[(*count)++] = id;
    } while (FindNextFileA(search, &entry));
    if (ok && GetLastError() != ERROR_NO_MORE_FILES) {
        ok = false;
    }
    FindClose(search);
    return ok;
}

bool nativeContentStore_DefaultRoot(char* path, size_t capacity)
{
    char app_data[MAX_PATH];
    DWORD length = GetEnvironmentVariableA("LOCALAPPDATA", app_data,
                                           sizeof(app_data));
    if (path == NULL || capacity == 0 || length == 0 ||
        length >= sizeof(app_data)) {
        return false;
    }
    int written = snprintf(path, capacity, "%s\\Freak Melee\\content",
                           app_data);
    return written > 0 && (size_t) written < capacity;
}

#endif
