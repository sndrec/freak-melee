#include "native_authoring_file.h"
#include "native_file_publish.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <filesystem>
#include <string_view>

namespace NativeAuthoring {
namespace {
struct File {
    HANDLE handle;
    ~File() { if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle); }
};

std::filesystem::path file_path(const char* path)
{
    if (!path || !*path) return {};
    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, nullptr, 0);
    if (count <= 1 || count > 32768) return {};
    std::wstring name(count, L'\0');
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, name.data(), count)) return {};
    name.pop_back();
    std::replace(name.begin(), name.end(), L'/', L'\\');
    std::wstring_view local(name);
    std::wstring_view network;
    if (!_wcsnicmp(name.c_str(), L"\\\\?\\UNC\\", 8)) {
        network = local.substr(8);
    } else if (local.rfind(L"\\\\?\\", 0) == 0) {
        local.remove_prefix(4);
        if (local.size() < 3 || local[1] != L':' || local[2] != L'\\') return {};
    } else if (local.rfind(L"\\\\.\\", 0) == 0 || local.rfind(L"\\??\\", 0) == 0) {
        return {};
    } else if (local.rfind(L"\\\\", 0) == 0) {
        network = local.substr(2);
    }
    size_t colon = local.find(L':');
    if (colon != local.npos && (colon != 1 || local.find(L':', 2) != local.npos ||
        !((local[0] >= L'A' && local[0] <= L'Z') || (local[0] >= L'a' && local[0] <= L'z')))) return {};
    if (!network.empty()) {
        size_t server = network.find(L'\\');
        if (server == 0 || server == network.npos) return {};
        const wchar_t* share = network.data() + server + 1;
        if (!_wcsnicmp(share, L"pipe\\", 5) || !_wcsnicmp(share, L"mailslot\\", 9)) return {};
    }
    std::error_code error;
    auto absolute = std::filesystem::absolute(name, error);
    return error || !absolute.has_filename() ? std::filesystem::path{} : absolute;
}
}

bool read_file(const char* path, size_t limit, std::vector<uint8_t>& bytes)
{
    auto source = file_path(path);
    if (source.empty()) return false;
    File file{CreateFileW(source.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                          SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS, nullptr)};
    LARGE_INTEGER size{};
    if (file.handle == INVALID_HANDLE_VALUE || GetFileType(file.handle) != FILE_TYPE_DISK ||
        !GetFileSizeEx(file.handle, &size) || size.QuadPart <= 0 || size.QuadPart > MAXDWORD ||
        static_cast<uint64_t>(size.QuadPart) > limit) return false;
    bytes.resize(static_cast<size_t>(size.QuadPart));
    DWORD read = 0;
    return ReadFile(file.handle, bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr) &&
        read == bytes.size();
}

bool export_file(const char* path, const void* bytes, size_t size, Reply& reply)
{
    auto destination = file_path(path);
    switch (nativeFile_PublishWide(destination.c_str(), bytes, size, NATIVE_FILE_PRESERVE, nullptr)) {
    case NATIVE_FILE_INVALID: reply.fail("invalid_output", "Choose an ordinary output file path."); return false;
    case NATIVE_FILE_UNAVAILABLE: reply.fail("export_unavailable", "Choose a new file in an existing output directory."); return false;
    case NATIVE_FILE_FAILED: reply.fail("export_failed", "Could not publish the complete file."); return false;
    case NATIVE_FILE_PUBLISHED: break;
    }
    auto encoded = destination.u8string();
    add_string(reply, reply.result, "output", std::string(encoded.begin(), encoded.end()));
    yyjson_mut_obj_add_uint(reply.document, reply.result, "bytes", size);
    return true;
}
}
