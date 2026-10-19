#include <melee/gm/native_account_internal.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>

static void wait(NativeAccountView& view)
{
    const auto deadline = GetTickCount64() + 30000;
    do {
        Sleep(10);
        nativeAccount_Poll(view);
        assert(GetTickCount64() < deadline);
    } while (view.busy || !view.revision);
}
// Integration driver: a real service-issued grant arrives through stdin, never
// command-line arguments or logs. It exercises the normal saved-session path.
static int client(const char* root, const char* endpoint, bool seed)
{
    auto json = std::make_unique<NativeAccountJson>();
    char line[8192];
    NativeAccountEndpoint service{}; assert(nativeAccount_Endpoint(endpoint, service));
    if (seed) {
        assert(std::fgets(line, sizeof(line), stdin));
        auto* grant = json->read(line, std::strlen(line)); assert(grant);
        assert(nativeAccount_PasskeyOptions(yyjson_obj_get(grant, "registration"), service, true));
        assert(nativeAccount_PasskeyOptions(yyjson_obj_get(grant, "authentication"), service, false));
        NativeAccountSecrets secrets{};
        std::snprintf(secrets.origin, sizeof(secrets.origin), "%s", service.origin);
        assert(nativeAccount_String(yyjson_obj_get(grant, "handle"), secrets.handle, sizeof(secrets.handle)));
        assert(nativeAccount_String(yyjson_obj_get(grant, "token"), secrets.token, sizeof(secrets.token)));
        wchar_t path[1024], folder[900];
        assert(MultiByteToWideChar(CP_UTF8, 0, root, -1, folder, 900));
        assert(swprintf_s(path, L"%s/account-session.bin", folder) > 0);
        assert(nativeAccount_Store(path, secrets));
        SecureZeroMemory(&secrets, sizeof(secrets)); SecureZeroMemory(line, sizeof(line));
    }
    assert(nativeAccount_Start(root, endpoint, nullptr));
    NativeAccountView view{}; wait(view);
    std::printf("ready signed-in=%u pending=%u keys=%u sessions=%u\n", view.signed_in, view.recovery_pending, view.key_count, view.session_count); std::fflush(stdout);
    while (std::fgets(line, sizeof(line), stdin)) {
        auto* object = json->read(line, std::strlen(line)); assert(object);
        NativeAccountRequest request{};
        request.operation = NativeAccountOperation(yyjson_get_uint(yyjson_obj_get(object, "operation")));
        auto* name = yyjson_obj_get(object, "name"), *bio = yyjson_obj_get(object, "bio"), *target = yyjson_obj_get(object, "target");
        if (name) assert(nativeAccount_String(name, request.name, sizeof(request.name)));
        if (bio) assert(nativeAccount_String(bio, request.bio, sizeof(request.bio), true));
        if (target) assert(nativeAccount_String(target, request.target, sizeof(request.target)));
        assert(nativeAccount_Submit(request)); wait(view);
        std::printf("done signed-in=%u saved=%u pending=%u keys=%u sessions=%u name=%s bio=%s status=%s\n",
            view.signed_in, view.saved, view.recovery_pending, view.key_count, view.session_count, view.name, view.bio, view.status);
        std::fflush(stdout);
    }
    nativeAccount_Shutdown();
    return 0;
}
int main(int argc, char** argv)
{
    if (argc == 4 && (!std::strcmp(argv[1], "--client") || !std::strcmp(argv[1], "--resume"))) return client(argv[2], argv[3], !std::strcmp(argv[1], "--client"));
    NativeAccountEndpoint endpoint{};
    assert(nativeAccount_Endpoint("https://EXAMPLE.com:443/", endpoint));
    assert(!std::strcmp(endpoint.origin, "https://example.com"));
    assert(nativeAccount_Endpoint("http://localhost:8799", endpoint) && endpoint.port == 8799);
    for (const char* url : {"http://example.com", "http://127.0.0.1", "https://user:pass@example.com", "https://example.com/a",
        "https://example.com/?token=x", "https://example.com/#x", "https://bad\".com", "file:///C:/test"})
        assert(!nativeAccount_Endpoint(url, endpoint));
    assert(nativeAccount_Endpoint("https://example.com", endpoint));
    char handle[25];
    assert(nativeAccount_Handle("TWILIGHT_2", handle) && !std::strcmp(handle, "twilight_2"));
    for (const char* value : {"ab", "ab/c", "a b", ".........................."}) assert(!nativeAccount_Handle(value, handle));
    BYTE bytes[256];
    for (unsigned i = 0; i < sizeof(bytes); ++i) bytes[i] = BYTE(i);
    char encoded[512]; BYTE decoded[256]; DWORD length;
    assert(nativeAccount_Base64Encode(bytes, sizeof(bytes), encoded, sizeof(encoded)));
    assert(nativeAccount_Base64Decode(encoded, decoded, sizeof(decoded), length) && length == sizeof(bytes) && !memcmp(bytes, decoded, sizeof(bytes)));
    for (const char* value : {"a", "AA=", "AA+", "AA/", "AA\n", "AB"}) assert(!nativeAccount_Base64Decode(value, decoded, sizeof(decoded), length));
    auto json = std::make_unique<NativeAccountJson>();
    char options[] = R"({"origin":"https://example.com","options":{"rpId":"example.com","challenge":"AAAAAAAAAAAAAAAAAAAAAA","allowCredentials":[{"id":"AAAA","type":"public-key"}],"userVerification":"required"}})";
    auto* value = json->read(options, strlen(options));
    assert(value && nativeAccount_PasskeyOptions(value, endpoint, false));
    assert(nativeAccount_Endpoint("https://other.example.com", endpoint));
    assert(!nativeAccount_PasskeyOptions(value, endpoint, false));
    char invalid[] = R"({"a":"\u0000"})";
    value = json->read(invalid, strlen(invalid));
    assert(value && !nativeAccount_String(yyjson_obj_get(value, "a"), encoded, sizeof(encoded)));
    wchar_t temporary[MAX_PATH], path[MAX_PATH];
    assert(GetTempPathW(MAX_PATH, temporary) && GetTempFileNameW(temporary, L"fma", 0, path));
    NativeAccountSecrets original{}, loaded{};
    std::snprintf(original.origin, sizeof(original.origin), "https://example.com");
    std::snprintf(original.handle, sizeof(original.handle), "twilight");
    assert(nativeAccount_Base64Encode(bytes, 32, original.token, sizeof(original.token)));
    assert(nativeAccount_Base64Encode(bytes + 32, 32, original.recovery, sizeof(original.recovery)));
    assert(nativeAccount_Store(path, original));
    assert(nativeAccount_Load(path, original.origin, loaded) && !memcmp(&loaded, &original, sizeof(original)));
    assert(!nativeAccount_Load(path, "https://other.example.com", loaded));
    HANDLE file = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    assert(file != INVALID_HANDLE_VALUE);
    char encrypted[4096]; DWORD count;
    assert(ReadFile(file, encrypted, sizeof(encrypted) - 1, &count, nullptr));
    encrypted[count] = 0;
    for (DWORD i = 0; i + 43 <= count; ++i) {
        assert(memcmp(encrypted + i, original.token, 43));
        assert(memcmp(encrypted + i, original.recovery, 43));
    }
    SetFilePointer(file, 0, nullptr, FILE_BEGIN);
    encrypted[0] ^= 1;
    assert(WriteFile(file, encrypted, count, &length, nullptr)); CloseHandle(file);
    assert(!nativeAccount_Load(path, original.origin, loaded));
    assert(nativeAccount_Delete(path));
    NativeAccountPasskeys passkeys;
    bool available = passkeys.load();
    std::printf("account contracts passed; Windows passkey API available=%u (no prompt opened)\n", available);
    passkeys.unload();
    return 0;
}
