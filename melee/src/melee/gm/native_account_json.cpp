#include "native_account_internal.h"
#include <wincrypt.h>
#include <bcrypt.h>
#include <cstring>
#include <cstdio>

bool nativeAccount_NewNonce(char (&nonce)[44])
{
    BYTE bytes[32];
    const bool ok = BCryptGenRandom(nullptr, bytes, sizeof(bytes), BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0 &&
        nativeAccount_Base64Encode(bytes, sizeof(bytes), nonce, sizeof(nonce));
    SecureZeroMemory(bytes, sizeof(bytes));
    return ok;
}
bool nativeAccount_ReadProfile(yyjson_val* root, NativeSocialProfile& profile)
{
    auto* value = yyjson_obj_get(root, "profile");
    char canonical[25];
    return nativeAccount_String(yyjson_obj_get(value, "id"), profile.id, sizeof(profile.id)) && nativeAccount_Secret(profile.id) &&
        nativeAccount_String(yyjson_obj_get(value, "handle"), profile.handle, sizeof(profile.handle)) &&
        nativeAccount_Handle(profile.handle, canonical) && !std::strcmp(canonical, profile.handle) &&
        nativeAccount_String(yyjson_obj_get(value, "name"), profile.name, sizeof(profile.name)) && *profile.name &&
        nativeAccount_String(yyjson_obj_get(value, "bio"), profile.bio, sizeof(profile.bio), true);
}

NativeAccountJson::~NativeAccountJson()
{
    yyjson_doc_free(input);
    yyjson_mut_doc_free(output);
    SecureZeroMemory(read_pool, sizeof(read_pool));
    SecureZeroMemory(write_pool, sizeof(write_pool));
}
yyjson_val* NativeAccountJson::read(char* bytes, size_t length)
{
    yyjson_doc_free(input); input = nullptr;
    if (!length || length > NativeAccountJsonLimit) return nullptr;
    yyjson_alc_pool_init(&reader, read_pool, sizeof(read_pool));
    input = yyjson_read_opts(bytes, length, 0, &reader, nullptr);
    auto* root = yyjson_doc_get_root(input);
    return yyjson_is_obj(root) ? root : nullptr;
}
yyjson_mut_val* NativeAccountJson::object()
{
    yyjson_mut_doc_free(output);
    yyjson_alc_pool_init(&writer, write_pool, sizeof(write_pool));
    output = yyjson_mut_doc_new(&writer);
    auto* root = yyjson_mut_obj(output);
    yyjson_mut_doc_set_root(output, root);
    return root;
}
bool NativeAccountJson::write(char* bytes, size_t capacity)
{
    size_t length = 0;
    char* value = yyjson_mut_write_opts(output, 0, &writer, &length, nullptr);
    const bool ok = value && length < capacity;
    if (ok) std::memcpy(bytes, value, length + 1);
    if (value) { SecureZeroMemory(value, length); writer.free(writer.ctx, value); }
    return ok;
}
bool nativeAccount_String(yyjson_val* value, char* output, size_t capacity, bool multiline)
{
    if (!yyjson_is_str(value) || yyjson_get_len(value) >= capacity) return false;
    const char* text = yyjson_get_str(value);
    for (size_t i = 0; i < yyjson_get_len(value); ++i) {
        unsigned char c = text[i];
        if (c == 127 || (c < 32 && !(multiline && (c == '\n' || c == '\t')))) return false;
    }
    std::memcpy(output, text, yyjson_get_len(value) + 1);
    return true;
}
bool nativeAccount_Handle(const char* input, char (&output)[25])
{
    const size_t length = strnlen(input, 25);
    if (length < 3 || length > 24) return false;
    for (size_t i = 0; i < length; ++i) {
        char c = input[i];
        if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
        if (!(c >= 'a' && c <= 'z') && !(c >= '0' && c <= '9') && c != '_') return false;
        output[i] = c;
    }
    output[length] = 0;
    return true;
}
bool nativeAccount_Base64Encode(const void* data, size_t length, char* output, size_t capacity)
{
    const size_t count = (length / 3) * 4 + (length % 3 ? length % 3 + 1 : 0);
    if (length > 49152 || count >= capacity) return false;
    constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    const auto* bytes = static_cast<const BYTE*>(data);
    size_t at = 0;
    for (size_t i = 0; i < length; i += 3) {
        const unsigned remaining = unsigned(length - i);
        const unsigned value = (unsigned(bytes[i]) << 16) | (remaining > 1 ? unsigned(bytes[i + 1]) << 8 : 0) |
            (remaining > 2 ? bytes[i + 2] : 0);
        output[at++] = alphabet[value >> 18];
        output[at++] = alphabet[(value >> 12) & 63];
        if (remaining > 1) output[at++] = alphabet[(value >> 6) & 63];
        if (remaining > 2) output[at++] = alphabet[value & 63];
    }
    output[at] = 0;
    return true;
}
bool nativeAccount_Base64Decode(const char* text, void* output, size_t capacity, DWORD& length)
{
    char encoded[2048];
    const size_t size = strnlen(text, sizeof(encoded));
    if (!size || size >= sizeof(encoded) - 4 || size % 4 == 1 || capacity > MAXDWORD) return false;
    for (size_t i = 0; i < size; ++i) {
        char c = text[i];
        if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') &&
            !(c >= '0' && c <= '9') && c != '-' && c != '_') return false;
        encoded[i] = c == '-' ? '+' : c == '_' ? '/' : c;
    }
    size_t padded = size;
    while (padded % 4) encoded[padded++] = '=';
    encoded[padded] = 0;
    length = DWORD(capacity);
    if (!CryptStringToBinaryA(encoded, DWORD(padded), CRYPT_STRING_BASE64 | CRYPT_STRING_STRICT,
        static_cast<BYTE*>(output), &length, nullptr, nullptr)) return false;
    char canonical[2048];
    return nativeAccount_Base64Encode(output, length, canonical, sizeof(canonical)) && !std::strcmp(canonical, text);
}
bool nativeAccount_Secret(const char* text)
{
    BYTE bytes[32]; DWORD length;
    const bool ok = std::strlen(text) == 43 && nativeAccount_Base64Decode(text, bytes, sizeof(bytes), length) && length == 32;
    SecureZeroMemory(bytes, sizeof(bytes));
    return ok;
}
bool nativeAccount_Endpoint(const char* url, NativeAccountEndpoint& result)
{
    wchar_t wide[512];
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, url, -1, wide, 512)) return false;
    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    parts.dwHostNameLength = parts.dwUrlPathLength = parts.dwExtraInfoLength =
        parts.dwUserNameLength = parts.dwPasswordLength = DWORD(-1);
    if (!WinHttpCrackUrl(wide, 0, 0, &parts) || !parts.dwHostNameLength || parts.dwHostNameLength >= 256 ||
        parts.dwUserNameLength || parts.dwPasswordLength || parts.dwExtraInfoLength ||
        (parts.dwUrlPathLength && !(parts.dwUrlPathLength == 1 && *parts.lpszUrlPath == '/'))) return false;
    result = {};
    wmemcpy(result.host, parts.lpszHostName, parts.dwHostNameLength);
    const bool local = !wcscmp(result.host, L"localhost");
    if (parts.nScheme != INTERNET_SCHEME_HTTPS && !(parts.nScheme == INTERNET_SCHEME_HTTP && local)) return false;
    result.port = parts.nPort;
    result.secure = parts.nScheme == INTERNET_SCHEME_HTTPS;
    char host[256];
    if (!WideCharToMultiByte(CP_UTF8, 0, result.host, -1, host, sizeof(host), nullptr, nullptr)) return false;
    for (char* c = host; *c; ++c) {
        if (*c >= 'A' && *c <= 'Z') *c += 'a' - 'A';
        if (!(*c >= 'a' && *c <= 'z') && !(*c >= '0' && *c <= '9') && *c != '.' && *c != '-') return false;
    }
    const unsigned normal = result.secure ? 443 : 80;
    if (result.port == normal) std::snprintf(result.origin, sizeof(result.origin), "%s://%s", result.secure ? "https" : "http", host);
    else std::snprintf(result.origin, sizeof(result.origin), "%s://%s:%u", result.secure ? "https" : "http", host, result.port);
    return true;
}
