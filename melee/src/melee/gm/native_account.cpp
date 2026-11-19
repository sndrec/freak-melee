#include "native_account_internal.h"
#include "native_account_transport.h"
#include "native_content_store.h"
#include <cstring>
#include <cstdio>
#include <new>

namespace {
    NativeAccountWorker* worker;
}

void nativeAccount_Publish(NativeAccountWorker& w, NativeAccountView& view)
{
    AcquireSRWLockExclusive(&w.lock);
    view.revision = w.view.revision + 1;
    w.view = view;
    if (!view.signed_in || !nativeAccountTransport_Publish(w.secrets.origin, w.secrets.handle, w.secrets.token)) nativeAccountTransport_Clear();
    ReleaseSRWLockExclusive(&w.lock);
}
bool nativeAccount_Cancelled(NativeAccountWorker& w)
{
    AcquireSRWLockShared(&w.lock);
    bool value = w.cancelled || w.stop;
    ReleaseSRWLockShared(&w.lock);
    return value;
}

yyjson_val* nativeAccount_Http(NativeAccountWorker& w, const char* handle, const char* route,
    bool post, bool authorized, NativeAccountView& view, unsigned* status_out, NativeAccountBinary binary)
{
    if (status_out) *status_out = 0;
    if (nativeAccount_Cancelled(w)) return nullptr;
    char path[512]; wchar_t object[512], headers[256];
    int path_length = std::snprintf(path, sizeof(path), "/v1/accounts/%s/%s", handle, route);
    if (path_length < 0 || unsigned(path_length) >= sizeof(path) ||
        !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, object, 512))
        return nullptr;
    const char* token = authorized ? w.secrets.token : "";
    swprintf_s(headers, L"Content-Type: %ls\r\nAccept: %ls\r\n%s%hs%s",
        binary.upload ? L"application/octet-stream" : L"application/json",
        binary.download ? L"application/octet-stream" : L"application/json",
        *token ? L"Authorization: Bearer " : L"", token, *token ? L"\r\n" : L"");
    HINTERNET request = w.connection ? WinHttpOpenRequest(w.connection, post ? L"POST" : L"GET", object,
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, w.endpoint.secure ? WINHTTP_FLAG_SECURE : 0) : nullptr;
    DWORD redirect = WINHTTP_OPTION_REDIRECT_POLICY_NEVER, disabled = WINHTTP_DISABLE_COOKIES | WINHTTP_DISABLE_AUTHENTICATION;
    bool ok = request && WinHttpSetOption(request, WINHTTP_OPTION_REDIRECT_POLICY, &redirect, sizeof(redirect)) &&
        WinHttpSetOption(request, WINHTTP_OPTION_DISABLE_FEATURE, &disabled, sizeof(disabled));
    DWORD length = binary.upload ? DWORD(binary.upload_size) : post ? DWORD(std::strlen(w.send)) : 0;
    void* body = binary.upload ? static_cast<void*>(const_cast<uint8_t*>(binary.upload))
                               : post ? static_cast<void*>(w.send) : nullptr;
    ok = ok && WinHttpSendRequest(request, headers, DWORD(-1), body, length, length, 0) &&
        WinHttpReceiveResponse(request, nullptr);
    DWORD status = 0, size = sizeof(status);
    ok = ok && WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX);
    if (binary.download && status == 200) {
        DWORD expected = 0; size = sizeof(expected);
        ok = ok && WinHttpQueryHeaders(request, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &expected, &size, WINHTTP_NO_HEADER_INDEX) &&
            expected > 0 && expected <= NATIVE_CONTENT_MAX_PACKAGE_BYTES;
        auto& data = *binary.download;
        if (ok) {
            data.resize(expected);
            DWORD received = 0;
            while (received < expected && ok) {
                DWORD count = 0;
                ok = !nativeAccount_Cancelled(w) &&
                     WinHttpReadData(request, data.data()+received, expected-received, &count) && count > 0;
                received += count;
            }
        }
        if (!ok) data.clear();
        if (request) WinHttpCloseHandle(request);
        SecureZeroMemory(headers, sizeof(headers));
        if (status_out) *status_out = ok ? status : 0;
        if (!ok) std::snprintf(view.status,sizeof(view.status),"Could not download this fighter. Try again shortly.");
        return nullptr;
    }
    size_t received = 0;
    while (ok) {
        DWORD available = 0, count = 0;
        if (!WinHttpQueryDataAvailable(request, &available)) { ok = false; break; }
        if (!available) break;
        if (available > NativeAccountJsonLimit - received ||
            !WinHttpReadData(request, w.receive + received, available, &count) || !count) { ok = false; break; }
        received += count;
    }
    if (request) WinHttpCloseHandle(request);
    SecureZeroMemory(headers, sizeof(headers));
    SecureZeroMemory(w.send, sizeof(w.send));
    w.receive[received] = 0;
    if (!ok) { std::snprintf(view.status, sizeof(view.status), "Could not reach the account service. Try again shortly."); return nullptr; }
    if (status_out) *status_out = status;
    auto* root = w.json.read(w.receive, received);
    if (!root) { std::snprintf(view.status, sizeof(view.status), "The account service returned an invalid response."); return nullptr; }
    if (status < 200 || status >= 300) {
        if (authorized && status == 401) { view.signed_in = false; view.social = {}; }
        if (!nativeAccount_String(yyjson_obj_get(root, "error"), view.status, sizeof(view.status)))
            std::snprintf(view.status, sizeof(view.status), "The account service could not complete this request.");
        return nullptr;
    }
    return root;
}
bool nativeAccount_SetProfile(yyjson_val* root, NativeAccountView& view)
{
    NativeSocialProfile profile{};
    if (!nativeAccount_ReadProfile(root, profile)) return false;
    std::memcpy(view.id, profile.id, sizeof(view.id));
    std::memcpy(view.handle, profile.handle, sizeof(view.handle));
    std::memcpy(view.name, profile.name, sizeof(view.name));
    std::memcpy(view.bio, profile.bio, sizeof(view.bio));
    return true;
}

bool nativeAccount_Persist(NativeAccountWorker& w, NativeAccountView& view)
{
    view.saved = nativeAccount_Store(w.path, w.secrets);
    view.recovery_pending = *w.secrets.recovery != 0;
    view.has_key = *w.secrets.seed != 0;
    if (!view.saved) std::snprintf(view.status, sizeof(view.status),
        "Could not save your account key on this PC. Export your key before closing.");
    return view.saved;
}
bool nativeAccount_Refresh(NativeAccountWorker& w, NativeAccountView& view)
{
    if (!*w.secrets.token) return false;
    unsigned status;
    auto* root = nativeAccount_Http(w, w.secrets.handle, "me", false, true, view, &status);
    if (!root) {
        if (status == 401) {
            view.signed_in = false; view.social = {}; view.key_count = view.session_count = 0;
            SecureZeroMemory(w.secrets.token, sizeof(w.secrets.token));
            w.secrets.signed_out = true;
            nativeAccount_Persist(w, view);
        }
        return false;
    }
    NativeAccountView updated = view;
    auto* keys = yyjson_obj_get(root, "keys");
    auto* sessions = yyjson_obj_get(root, "sessions");
    bool ok = nativeAccount_SetProfile(root, updated) && !std::strcmp(updated.handle, w.secrets.handle) &&
        yyjson_is_arr(keys) && yyjson_arr_size(keys) > 0 && yyjson_arr_size(keys) <= 8 &&
        yyjson_is_arr(sessions) && yyjson_arr_size(sessions) > 0 && yyjson_arr_size(sessions) <= 16;
    updated.key_count = unsigned(yyjson_arr_size(keys)); updated.session_count = unsigned(yyjson_arr_size(sessions));
    for (unsigned i = 0; ok && i < updated.key_count; ++i) {
        auto* key = yyjson_arr_get(keys, i); char own_key[44];
        ok = nativeAccount_String(yyjson_obj_get(key, "id"), updated.keys[i].id, sizeof(updated.keys[i].id)) &&
            nativeAccount_Secret(updated.keys[i].id) &&
            nativeAccount_String(yyjson_obj_get(key, "name"), updated.keys[i].name, sizeof(updated.keys[i].name));
        updated.keys[i].current = nativeAccount_KeyPublic(w.secrets.seed, own_key) && !std::strcmp(own_key, updated.keys[i].id);
    }
    for (unsigned i = 0; ok && i < updated.session_count; ++i) {
        auto* session = yyjson_arr_get(sessions, i);
        ok = nativeAccount_String(yyjson_obj_get(session, "id"), updated.sessions[i].id, sizeof(updated.sessions[i].id)) &&
            nativeAccount_Secret(updated.sessions[i].id) &&
            nativeAccount_String(yyjson_obj_get(session, "name"), updated.sessions[i].name, sizeof(updated.sessions[i].name)) &&
            yyjson_is_bool(yyjson_obj_get(session, "current"));
        updated.sessions[i].current = yyjson_get_bool(yyjson_obj_get(session, "current"));
    }
    if (!ok) { std::snprintf(view.status, sizeof(view.status), "The account details could not be read."); return false; }
    updated.signed_in = true;
    ++updated.profile_revision;
    view = updated;
    return true;
}
namespace {
    void run(NativeAccountWorker& w, const NativeAccountRequest& request, NativeAccountView& view)
    {
        if (request.operation == NativeAccountOperation::AttachLobby) {
            if (!view.signed_in) return;
            auto* body = w.json.object();
            yyjson_mut_obj_add_str(w.json.output, body, "room", request.room);
            yyjson_mut_obj_add_str(w.json.output, body, "connection", request.target);
            if (!w.json.write(w.send, sizeof(w.send))) return;
            if (nativeAccount_Http(w, w.secrets.handle, "lobby/attach", true, true, view))
                std::snprintf(view.status, sizeof(view.status), "Account connected to lobby.");
            return;
        }
        if (request.operation >= NativeAccountOperation::ImportKey) { nativeAccount_RunFile(w, request, view); return; }
        if (nativeAccount_RunSocial(w, request, view)) return;
        const auto operation = request.operation;
        if (*w.secrets.recovery && (operation == NativeAccountOperation::Logout || operation == NativeAccountOperation::RevokeAll ||
            operation == NativeAccountOperation::Register || operation == NativeAccountOperation::Recover)) {
            std::snprintf(view.status, sizeof(view.status), "Save your recovery code before signing out or replacing this account."); return;
        }
        if (operation >= NativeAccountOperation::Register && operation <= NativeAccountOperation::AddKey) { nativeAccount_Login(w, request, view); return; }
        if (operation == NativeAccountOperation::Refresh) {
            if (!*w.secrets.token && *w.secrets.seed && !w.secrets.signed_out) {
                NativeAccountRequest login{}; login.operation = NativeAccountOperation::Login;
                nativeAccount_Login(w, login, view); return;
            }
            if (nativeAccount_Refresh(w, view)) std::snprintf(view.status, sizeof(view.status), "Signed in.");
            return;
        }
        if (operation == NativeAccountOperation::RemoveKey || operation == NativeAccountOperation::RevokeAll ||
            operation == NativeAccountOperation::RotateRecovery) {
            NativeAccountRequest approval{}; approval.operation = NativeAccountOperation::Approve;
            if (!nativeAccount_Login(w, approval, view)) return;
        }
        auto* body = w.json.object();
        const char* route = nullptr;
        switch (operation) {
        case NativeAccountOperation::SaveProfile:
            yyjson_mut_obj_add_str(w.json.output, body, "name", request.name);
            yyjson_mut_obj_add_str(w.json.output, body, "bio", request.bio);
            route = "profile"; break;
        case NativeAccountOperation::Logout: route = "logout"; break;
        case NativeAccountOperation::RevokeSession:
            yyjson_mut_obj_add_str(w.json.output, body, "id", request.target); route = "sessions/revoke"; break;
        case NativeAccountOperation::RemoveKey:
            yyjson_mut_obj_add_str(w.json.output, body, "id", request.target); route = "keys/remove"; break;
        case NativeAccountOperation::RevokeAll: route = "sessions/revoke-all"; break;
        case NativeAccountOperation::RotateRecovery: route = "recovery/rotate"; break;
        default: return;
        }
        if (!w.json.write(w.send, sizeof(w.send))) return;
        auto* response = nativeAccount_Http(w, w.secrets.handle, route, true, true, view);
        const bool remote = response != nullptr;
        if (operation == NativeAccountOperation::Logout || (remote && operation == NativeAccountOperation::RevokeAll)) {
            SecureZeroMemory(w.secrets.token, sizeof(w.secrets.token)); w.secrets.signed_out = true;
            view.signed_in = false; view.social = {}; view.key_count = view.session_count = 0;
            if (nativeAccount_Persist(w, view)) std::snprintf(view.status, sizeof(view.status),
                remote ? "Signed out. Your account key is saved on this PC." : "Signed out on this PC. Could not revoke the session online.");
            return;
        }
        if (!response) return;
        if (operation == NativeAccountOperation::RotateRecovery) {
            char recovery[44];
            if (!nativeAccount_String(yyjson_obj_get(response, "recovery_code"), recovery, sizeof(recovery)) || !nativeAccount_Secret(recovery)) {
                std::snprintf(view.status, sizeof(view.status), "The replacement recovery code could not be read."); return;
            }
            AcquireSRWLockExclusive(&w.lock); std::memcpy(w.secrets.recovery, recovery, sizeof(recovery)); ReleaseSRWLockExclusive(&w.lock);
            SecureZeroMemory(recovery, sizeof(recovery));
            if (nativeAccount_Persist(w, view)) std::snprintf(view.status, sizeof(view.status), "Save your new recovery code. The previous code has expired."); return;
        }
        if (nativeAccount_Refresh(w, view)) std::snprintf(view.status, sizeof(view.status), "Account updated.");
    }
    DWORD WINAPI thread_main(void* data)
    {
        auto& w = *static_cast<NativeAccountWorker*>(data);
        for (;;) {
            WaitForSingleObject(w.wake, INFINITE);
            AcquireSRWLockExclusive(&w.lock);
            if (w.stop) { ReleaseSRWLockExclusive(&w.lock); break; }
            NativeAccountRequest request = w.request;
            NativeAccountView view = w.view;
            SecureZeroMemory(&w.request, sizeof(w.request)); w.queued = false;
            ReleaseSRWLockExclusive(&w.lock);
            run(w, request, view);
            if (nativeAccount_Cancelled(w)) std::snprintf(view.status, sizeof(view.status), "Request cancelled. Refresh to check your account.");
            SecureZeroMemory(&request, sizeof(request));
            SecureZeroMemory(w.send, sizeof(w.send)); SecureZeroMemory(w.receive, sizeof(w.receive));
            // JSON pools also held copies of tokens and recovery codes.
            yyjson_doc_free(w.json.input); w.json.input = nullptr;
            yyjson_mut_doc_free(w.json.output); w.json.output = nullptr;
            SecureZeroMemory(w.json.read_pool, sizeof(w.json.read_pool));
            SecureZeroMemory(w.json.write_pool, sizeof(w.json.write_pool));
            view.busy = view.cancellable = false;
            nativeAccount_Publish(w, view);
        }
        return 0;
    }
}
bool nativeAccount_Start(const char* root, const char* endpoint)
{
    if (worker) return true;
    auto* w = new (std::nothrow) NativeAccountWorker;
    if (!w) return false;
    wchar_t directory[900];
    bool ok = nativeAccount_Endpoint(endpoint, w->endpoint) &&
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, root, -1, directory, 900);
    if (ok) {
        CreateDirectoryW(directory, nullptr);
        ok = nativeAccountTransport_InitGuest(root, endpoint) &&
            swprintf_s(w->path, L"%s/account-session.bin", directory) > 0;
    }
    w->view.available = true;
    w->session = WinHttpOpen(L"FreakMelee/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (w->session) {
        WinHttpSetTimeouts(w->session, 5000, 5000, 10000, 10000);
        DWORD protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
        WinHttpSetOption(w->session, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));
    }
    if (ok && w->session) w->connection = WinHttpConnect(w->session, w->endpoint.host, w->endpoint.port, 0);
    w->wake = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (ok && w->wake && w->connection) {
        w->view.saved = nativeAccount_Load(w->path, w->endpoint.origin, w->secrets);
        w->view.recovery_pending = *w->secrets.recovery != 0;
        w->view.has_key = *w->secrets.seed != 0;
        std::snprintf(w->view.handle, sizeof(w->view.handle), "%s", w->secrets.handle);
        std::snprintf(w->view.status, sizeof(w->view.status), w->view.saved && !w->secrets.signed_out ? "Checking your saved sign-in..." : "Create an account or import your account-key file.");
        w->view.revision = 1;
        w->thread = CreateThread(nullptr, 0, thread_main, w, 0, nullptr);
    }
    if (!w->thread) {
        if (w->wake) CloseHandle(w->wake);
        if (w->connection) WinHttpCloseHandle(w->connection);
        if (w->session) WinHttpCloseHandle(w->session);
        delete w; return false;
    }
    worker = w;
    if (w->view.saved && !w->secrets.signed_out) nativeAccount_Submit({NativeAccountOperation::Refresh});
    return true;
}
bool nativeAccount_Submit(const NativeAccountRequest& request)
{
    if (!worker || unsigned(request.operation) >= unsigned(NativeAccountOperation::Count) ||
        !std::memchr(request.handle, 0, sizeof(request.handle)) || !std::memchr(request.name, 0, sizeof(request.name)) ||
        !std::memchr(request.bio, 0, sizeof(request.bio)) || !std::memchr(request.recovery, 0, sizeof(request.recovery)) ||
        !std::memchr(request.target, 0, sizeof(request.target)) ||
        !std::memchr(request.subject, 0, sizeof(request.subject)) || !std::memchr(request.text, 0, sizeof(request.text)) ||
        !std::memchr(request.nonce, 0, sizeof(request.nonce)) || !std::memchr(request.action, 0, sizeof(request.action)) ||
        !std::memchr(request.room, 0, sizeof(request.room)) || !std::memchr(request.path, 0, sizeof(request.path))) return false;
    auto& w = *worker;
    AcquireSRWLockExclusive(&w.lock);
    const bool ok = !w.stop && !w.view.busy && !w.queued;
    if (ok) {
        w.request = request; w.queued = true; w.cancelled = false; w.view.busy = true;
        w.view.cancellable = request.operation >= NativeAccountOperation::Register && request.operation <= NativeAccountOperation::AddKey;
        std::snprintf(w.view.status, sizeof(w.view.status), "Working..."); ++w.view.revision;
        SetEvent(w.wake);
    }
    ReleaseSRWLockExclusive(&w.lock);
    return ok;
}
bool nativeAccount_Poll(NativeAccountView& view)
{
    if (!worker) return false;
    AcquireSRWLockShared(&worker->lock);
    bool changed = view.revision != worker->view.revision;
    if (changed) view = worker->view;
    ReleaseSRWLockShared(&worker->lock);
    return changed;
}
void nativeAccount_Cancel()
{
    if (!worker) return;
    AcquireSRWLockExclusive(&worker->lock);
    if (worker->view.cancellable) {
        worker->cancelled = true;
    }
    ReleaseSRWLockExclusive(&worker->lock);
}
void nativeAccount_Shutdown()
{
    nativeAccountTransport_Clear();
    if (!worker) return;
    nativeAccount_Cancel();
    AcquireSRWLockExclusive(&worker->lock); worker->stop = true; SetEvent(worker->wake); ReleaseSRWLockExclusive(&worker->lock);
    WaitForSingleObject(worker->thread, INFINITE);
    nativeAccountTransport_Clear();
    CloseHandle(worker->thread); CloseHandle(worker->wake);
    WinHttpCloseHandle(worker->connection); WinHttpCloseHandle(worker->session);
    SecureZeroMemory(&worker->secrets, sizeof(worker->secrets));
    delete worker; worker = nullptr;
}
