#include "native_workshop_connection_security.h"
#include <bcrypt.h>
#include <sddl.h>
#include <cstring>

namespace NativeWorkshopConnection {
bool instance_valid(const std::string& instance)
{
    size_t split = instance.find('-');
    if (!split || split > 10 || split == std::string::npos || instance.size() != split + 33) return false;
    for (size_t i = 0; i < instance.size(); ++i) {
        if (i == split) continue;
        char ch = instance[i];
        if (i < split ? (ch < '0' || ch > '9') : !((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f'))) return false;
    }
    return true;
}
std::string pipe_name(const std::string& instance) { return std::string("\\\\.\\pipe\\") + Prefix + instance; }
std::string random_id()
{
    unsigned char bytes[16];
    if (BCryptGenRandom(nullptr, bytes, sizeof(bytes), BCRYPT_USE_SYSTEM_PREFERRED_RNG)) return {};
    const char* hex = "0123456789abcdef";
    std::string result(32, '0');
    for (size_t i = 0; i < 16; ++i) { result[i * 2] = hex[bytes[i] >> 4]; result[i * 2 + 1] = hex[bytes[i] & 15]; }
    return result;
}
static bool user(HANDLE process, std::vector<unsigned char>& bytes)
{
    HANDLE token = nullptr;
    if (!OpenProcessToken(process, TOKEN_QUERY, &token)) return false;
    DWORD size = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &size);
    bytes.resize(size);
    bool ok = size && GetTokenInformation(token, TokenUser, bytes.data(), size, &size);
    CloseHandle(token); return ok;
}
bool process_is_current_user(DWORD pid)
{
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) return false;
    std::vector<unsigned char> mine, other;
    bool ok = user(GetCurrentProcess(), mine) && user(process, other) &&
        EqualSid(((TOKEN_USER*)mine.data())->User.Sid, ((TOKEN_USER*)other.data())->User.Sid);
    CloseHandle(process); return ok;
}
PSECURITY_DESCRIPTOR owner_descriptor()
{
    std::vector<unsigned char> bytes;
    if (!user(GetCurrentProcess(), bytes)) return nullptr;
    LPSTR sid = nullptr;
    if (!ConvertSidToStringSidA(((TOKEN_USER*)bytes.data())->User.Sid, &sid)) return nullptr;
    const std::string dacl = "D:P(A;;GA;;;" + std::string(sid) + ")";
    LocalFree(sid);
    PSECURITY_DESCRIPTOR result = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorA(dacl.c_str(), SDDL_REVISION_1, &result, nullptr)) return nullptr;
    return result;
}
}
