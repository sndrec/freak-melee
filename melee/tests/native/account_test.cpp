#include <melee/gm/native_account_internal.h>
#include <melee/gm/native_account_transport.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <monocypher-ed25519.h>

static void wait(NativeAccountView& view)
{
    const auto deadline = GetTickCount64() + 30000;
    do {
        Sleep(10);
        nativeAccount_Poll(view);
        assert(GetTickCount64() < deadline);
    } while (view.busy || !view.revision);
}
// Drives the real native key generation, signatures, HTTP and protected store.
static int client(const char* root, const char* endpoint)
{
    auto json = std::make_unique<NativeAccountJson>();
    char line[8192];
    assert(nativeAccount_Start(root, endpoint));
    NativeAccountView view{}; wait(view);
    std::printf("ready signed-in=%u pending=%u keys=%u sessions=%u\n", view.signed_in, view.recovery_pending, view.key_count, view.session_count); std::fflush(stdout);
    while (std::fgets(line, sizeof(line), stdin)) {
        auto* object = json->read(line, std::strlen(line)); assert(object);
        NativeAccountRequest request{};
        const char* names[] = {"Refresh", "Register", "Login", "Recover", "Approve", "AddKey", "SaveProfile",
            "Logout", "RevokeSession", "RevokeAll", "RemoveKey", "RotateRecovery", "RefreshSocial", "LookupProfile",
            "SendLetter", "ReadLetter", "DeleteLetter", "ChangeFriend", "BlockPlayer", "SendInvitation", "JoinInvitation",
            "ImportKey", "ExportKey", "ExportRecovery"};
        unsigned operation = 0;
        for (; operation < sizeof(names) / sizeof(names[0]); ++operation)
            if (yyjson_equals_str(yyjson_obj_get(object, "operation"), names[operation])) break;
        assert(operation < sizeof(names) / sizeof(names[0])); request.operation = NativeAccountOperation(operation);
        auto* path = yyjson_obj_get(object, "path"), *recovery = yyjson_obj_get(object, "recovery");
        if (path) assert(nativeAccount_String(path, request.path, sizeof(request.path)));
        if (recovery) assert(nativeAccount_String(recovery, request.recovery, sizeof(request.recovery)));
        auto* name = yyjson_obj_get(object, "name"), *bio = yyjson_obj_get(object, "bio"), *target = yyjson_obj_get(object, "target");
        if (name) assert(nativeAccount_String(name, request.name, sizeof(request.name)));
        if (bio) assert(nativeAccount_String(bio, request.bio, sizeof(request.bio), true));
        if (target) assert(nativeAccount_String(target, request.target, sizeof(request.target)));
        auto* handle = yyjson_obj_get(object, "handle"), *subject = yyjson_obj_get(object, "subject"), *text = yyjson_obj_get(object, "text");
        auto* nonce = yyjson_obj_get(object, "nonce"), *action = yyjson_obj_get(object, "action");
        if (handle) assert(nativeAccount_String(handle, request.handle, sizeof(request.handle)));
        if (subject) assert(nativeAccount_String(subject, request.subject, sizeof(request.subject)));
        if (text) assert(nativeAccount_String(text, request.text, sizeof(request.text), true));
        if (nonce) assert(nativeAccount_String(nonce, request.nonce, sizeof(request.nonce)));
        if (action) assert(nativeAccount_String(action, request.action, sizeof(request.action)));
        auto* room = yyjson_obj_get(object, "room");
        if (room) assert(nativeAccount_String(room, request.room, sizeof(request.room)));
        request.folder = unsigned(yyjson_get_uint(yyjson_obj_get(object, "folder")));
        request.offset = unsigned(yyjson_get_uint(yyjson_obj_get(object, "offset")));
        request.friends_offset = unsigned(yyjson_get_uint(yyjson_obj_get(object, "friends_offset")));
        request.blocked = yyjson_get_bool(yyjson_obj_get(object, "blocked"));
        assert(nativeAccount_Submit(request)); wait(view);
        std::printf("done signed-in=%u saved=%u pending=%u keys=%u sessions=%u name=%s bio=%s status=%s social-loaded=%u mail=%u total=%u unread=%u friends=%u opened=%u lookup=%s relationship=%s handle=%s key0=%s\n",
            view.signed_in, view.saved, view.recovery_pending, view.key_count, view.session_count, view.name, view.bio, view.status,
            view.social.loaded, view.social.count, view.social.total, view.social.unread, view.social.friends_total,
            *view.social.opened.id != 0, view.social.profile.handle, view.social.relationship, view.handle, view.key_count ? view.keys[0].id : "");
        std::fflush(stdout);
    }
    nativeAccount_Shutdown();
    return 0;
}
int main(int argc, char** argv)
{
    if (argc == 4 && !std::strcmp(argv[1], "--service")) return client(argv[2], argv[3]);
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
    // RFC 8032 test vector 1, checked independently of the service implementation.
    auto unhex = [](const char* text, BYTE* output, unsigned size) {
        assert(std::strlen(text) == size * 2);
        for (unsigned i = 0; i < size; ++i) { unsigned value; assert(std::sscanf(text + i * 2, "%2x", &value) == 1); output[i] = BYTE(value); }
    };
    BYTE seed[32], secret_key[64], public_key[32], expected[64], signature[64];
    unhex("9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60", seed, 32);
    crypto_ed25519_key_pair(secret_key, public_key, seed);
    unhex("d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a", expected, 32);
    assert(!std::memcmp(public_key, expected, 32));
    crypto_ed25519_sign(signature, secret_key, nullptr, 0);
    unhex("e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e065224901555fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b", expected, 64);
    assert(!std::memcmp(signature, expected, 64) && crypto_ed25519_check(signature, public_key, nullptr, 0) == 0);
    crypto_wipe(secret_key, sizeof(secret_key));
    char generated[44], derived[44];
    assert(nativeAccount_GenerateKey(generated) && nativeAccount_KeyPublic(generated, derived));
    assert(std::strcmp(generated, derived));
    yyjson_val* value;
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
    std::memcpy(original.seed, generated, sizeof(generated));
    nativeAccountTransport_Clear();
    const auto generation = nativeAccountTransport_Generation();
    assert(nativeAccountTransport_Publish(original.origin, original.handle, original.token));
    assert(nativeAccountTransport_Generation() == generation + 1);
    assert(nativeAccountTransport_Publish(original.origin, original.handle, original.token));
    assert(nativeAccountTransport_Generation() == generation + 1);
    wchar_t headers[192];
    assert(nativeAccountTransport_Headers(L"https://EXAMPLE.com:443/chat/connect", headers, 192));
    assert(wcsstr(headers, L"Authorization: Bearer ") == headers && wcsstr(headers, L"X-Freak-Account: twilight\r\n"));
    for (const wchar_t* url : {L"https://example.com.evil/chat/connect", L"https://elsewhere.test/chat/connect", L"http://example.com/chat/connect",
        L"https://example.com:444/chat/connect", L"https://user@example.com/chat/connect"}) {
        assert(!nativeAccountTransport_Headers(url, headers, 192) && !*headers);
    }
    nativeAccountTransport_Clear();
    assert(nativeAccountTransport_Generation() == generation + 2);
    assert(!nativeAccountTransport_Headers(L"https://example.com/chat/connect", headers, 192));
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
    char portable[1024]; assert(WideCharToMultiByte(CP_UTF8, 0, path, -1, portable, sizeof(portable), nullptr, nullptr));
    assert(nativeAccount_WriteKeyFile(portable, original, *json));
    loaded = {};
    assert(nativeAccount_ReadKeyFile(portable, original.origin, loaded, *json));
    assert(!std::strcmp(loaded.seed, original.seed) && !std::strcmp(loaded.handle, original.handle) && !*loaded.token && !*loaded.recovery);
    assert(!nativeAccount_ReadKeyFile(portable, "https://other.example.com", loaded, *json));
    assert(nativeAccount_Delete(path));
    original.token[0] = 0; original.signed_out = true;
    assert(nativeAccount_Store(path, original) && nativeAccount_Load(path, original.origin, loaded));
    assert(loaded.signed_out && !*loaded.token && !std::strcmp(loaded.seed, original.seed));
    assert(nativeAccount_Delete(path));
    std::printf("account contracts passed: RFC 8032, portable key file, protected sign-in, endpoint isolation\n");
    return 0;
}
