#pragma once
#include "native_account.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>
#include <yyjson.h>
#include <stddef.h>
#include <cwchar>
#include <vector>

constexpr size_t NativeAccountJsonLimit = 65536;
struct NativeAccountEndpoint {
    wchar_t host[256];
    char origin[512];
    INTERNET_PORT port;
    bool secure;
};
// Fixed JSON pools are owned by the account worker, never shared with rendering.
struct NativeAccountJson {
    alignas(16) char read_pool[1024 * 1024], write_pool[384 * 1024];
    yyjson_alc reader{}, writer{};
    yyjson_doc* input = nullptr;
    yyjson_mut_doc* output = nullptr;
    ~NativeAccountJson();
    yyjson_val* read(char* bytes, size_t length);
    yyjson_mut_val* object();
    bool write(char* bytes, size_t capacity);
};
struct NativeAccountSecrets {
    char origin[512], handle[25], token[44], recovery[44], seed[44];
    bool signed_out;
};
bool nativeAccount_Endpoint(const char* url, NativeAccountEndpoint& result);
bool nativeAccount_Handle(const char* input, char (&output)[25]);
bool nativeAccount_String(yyjson_val* value, char* output, size_t capacity, bool multiline = false);
bool nativeAccount_Secret(const char* text);
bool nativeAccount_Base64Encode(const void* data, size_t length, char* output, size_t capacity);
bool nativeAccount_Base64Decode(const char* text, void* output, size_t capacity, DWORD& length);
bool nativeAccount_Store(const wchar_t* path, const NativeAccountSecrets& secrets);
bool nativeAccount_Load(const wchar_t* path, const char* origin, NativeAccountSecrets& secrets);
bool nativeAccount_Delete(const wchar_t* path);
bool nativeAccount_GenerateKey(char (&encoded)[44]);
bool nativeAccount_KeyPublic(const char* encoded, char (&output)[44]);
bool nativeAccount_KeyProof(yyjson_val* response, const NativeAccountEndpoint& endpoint,
    const char* handle, const char* purpose, const char* encoded, NativeAccountJson& json,
    yyjson_mut_val* body, char (&error)[192]);
bool nativeAccount_ReadKeyFile(const char* path, const char* origin, NativeAccountSecrets& result, NativeAccountJson& json);
bool nativeAccount_WriteKeyFile(const char* path, const NativeAccountSecrets& identity, NativeAccountJson& json);

struct NativeAccountWorker {
    SRWLOCK lock = SRWLOCK_INIT;
    HANDLE wake = nullptr, thread = nullptr;
    bool stop = false, queued = false, cancelled = false;
    NativeAccountEndpoint endpoint{};
    NativeAccountSecrets secrets{};
    NativeAccountRequest request{};
    NativeAccountView view{};
    wchar_t path[1024]{};
    HINTERNET session = nullptr, connection = nullptr;
    NativeAccountJson json;
    char send[NativeAccountJsonLimit + 1], receive[NativeAccountJsonLimit + 1];
};
struct NativeAccountBinary {
    const uint8_t* upload = nullptr;
    size_t upload_size = 0;
    std::vector<uint8_t>* download = nullptr;
};
yyjson_val* nativeAccount_Http(NativeAccountWorker& worker, const char* handle, const char* route,
    bool post, bool authorized, NativeAccountView& view, unsigned* status_out = nullptr,
    NativeAccountBinary binary = {});
bool nativeAccount_ReadProfile(yyjson_val* root, NativeSocialProfile& profile);
bool nativeAccount_RunSocial(NativeAccountWorker& worker, const NativeAccountRequest& request, NativeAccountView& view);

void nativeAccount_Publish(NativeAccountWorker& worker, NativeAccountView& view);
bool nativeAccount_Cancelled(NativeAccountWorker& worker);
bool nativeAccount_SetProfile(yyjson_val* root, NativeAccountView& view);
bool nativeAccount_Persist(NativeAccountWorker& worker, NativeAccountView& view);
bool nativeAccount_Refresh(NativeAccountWorker& worker, NativeAccountView& view);
bool nativeAccount_Login(NativeAccountWorker& worker, const NativeAccountRequest& request, NativeAccountView& view);
void nativeAccount_RunFile(NativeAccountWorker& worker, const NativeAccountRequest& request, NativeAccountView& view);
