#pragma once
#include "native_account.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>
#include <webauthn.h>
#include <yyjson.h>
#include <stddef.h>
#include <cwchar>

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
    char origin[512], handle[25], token[44], recovery[44];
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

struct NativeAccountPasskeys {
    HMODULE library = nullptr;
    decltype(&WebAuthNGetApiVersionNumber) version = nullptr;
    decltype(&WebAuthNGetCancellationId) cancellation_id = nullptr;
    decltype(&WebAuthNCancelCurrentOperation) cancel = nullptr;
    decltype(&WebAuthNAuthenticatorMakeCredential) create = nullptr;
    decltype(&WebAuthNAuthenticatorGetAssertion) get = nullptr;
    decltype(&WebAuthNFreeCredentialAttestation) free_attestation = nullptr;
    decltype(&WebAuthNFreeAssertion) free_assertion = nullptr;
    GUID cancellation{};
    bool load();
    void unload();
};
// Validates trusted origin/RP/options before showing any security prompt.
bool nativeAccount_PasskeyOptions(yyjson_val* response, const NativeAccountEndpoint& endpoint, bool registration);
bool nativeAccount_Passkey(NativeAccountPasskeys& api, HWND window,
    yyjson_val* response, const NativeAccountEndpoint& endpoint, bool registration,
    NativeAccountJson& json, yyjson_mut_val* body, char (&error)[192]);

struct NativeAccountWorker {
    SRWLOCK lock = SRWLOCK_INIT;
    HANDLE wake = nullptr, thread = nullptr;
    bool stop = false, queued = false, cancelled = false;
    HWND window = nullptr;
    NativeAccountEndpoint endpoint{};
    NativeAccountSecrets secrets{};
    NativeAccountPasskeys passkeys;
    NativeAccountRequest request{};
    NativeAccountView view{};
    wchar_t path[1024]{};
    HINTERNET session = nullptr, connection = nullptr;
    NativeAccountJson json;
    char send[NativeAccountJsonLimit + 1], receive[NativeAccountJsonLimit + 1];
};
yyjson_val* nativeAccount_Http(NativeAccountWorker& worker, const char* handle, const char* route,
    bool post, bool authorized, NativeAccountView& view, unsigned* status_out = nullptr);
bool nativeAccount_ReadProfile(yyjson_val* root, NativeSocialProfile& profile);
bool nativeAccount_RunSocial(NativeAccountWorker& worker, const NativeAccountRequest& request, NativeAccountView& view);
