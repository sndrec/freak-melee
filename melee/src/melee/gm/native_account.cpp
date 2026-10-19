#include "native_account_internal.h"
#include <commdlg.h>
#include <cstring>
#include <cstdio>
#include <new>

namespace {
    NativeAccountWorker* worker;

    void publish(NativeAccountWorker& w, NativeAccountView& view)
    {
        AcquireSRWLockExclusive(&w.lock);
        view.revision = w.view.revision + 1;
        w.view = view;
        ReleaseSRWLockExclusive(&w.lock);
    }
    bool cancelled(NativeAccountWorker& w)
    {
        AcquireSRWLockShared(&w.lock);
        bool value = w.cancelled || w.stop;
        ReleaseSRWLockShared(&w.lock);
        return value;
    }
    // All WinHTTP handles and buffers belong to this worker. No menu-frame I/O.
}
yyjson_val* nativeAccount_Http(NativeAccountWorker& w, const char* handle, const char* route,
        bool post, bool authorized, NativeAccountView& view, unsigned* status_out)
    {
        if (status_out) *status_out = 0;
        if (cancelled(w)) return nullptr;
        char path[160]; wchar_t object[160], headers[256];
        std::snprintf(path, sizeof(path), "/v1/accounts/%s/%s", handle, route);
        MultiByteToWideChar(CP_UTF8, 0, path, -1, object, 160);
        const char* token = authorized ? w.secrets.token : "";
        swprintf_s(headers, L"Content-Type: application/json\r\nAccept: application/json\r\n%s%hs%s",
            *token ? L"Authorization: Bearer " : L"", token, *token ? L"\r\n" : L"");
        HINTERNET request = w.connection ? WinHttpOpenRequest(w.connection, post ? L"POST" : L"GET", object,
            nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, w.endpoint.secure ? WINHTTP_FLAG_SECURE : 0) : nullptr;
        DWORD redirect = WINHTTP_OPTION_REDIRECT_POLICY_NEVER, disabled = WINHTTP_DISABLE_COOKIES | WINHTTP_DISABLE_AUTHENTICATION;
        bool ok = request && WinHttpSetOption(request, WINHTTP_OPTION_REDIRECT_POLICY, &redirect, sizeof(redirect)) &&
            WinHttpSetOption(request, WINHTTP_OPTION_DISABLE_FEATURE, &disabled, sizeof(disabled));
        DWORD length = post ? DWORD(std::strlen(w.send)) : 0;
        ok = ok && WinHttpSendRequest(request, headers, DWORD(-1), post ? w.send : nullptr, length, length, 0) &&
            WinHttpReceiveResponse(request, nullptr);
        DWORD status = 0, size = sizeof(status);
        ok = ok && WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX);
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
namespace {
    bool profile(yyjson_val* root, NativeAccountView& view)
    {
        NativeSocialProfile profile{};
        if (!nativeAccount_ReadProfile(root, profile)) return false;
        std::memcpy(view.id, profile.id, sizeof(view.id));
        std::memcpy(view.handle, profile.handle, sizeof(view.handle));
        std::memcpy(view.name, profile.name, sizeof(view.name));
        std::memcpy(view.bio, profile.bio, sizeof(view.bio));
        return true;
    }

    bool persist(NativeAccountWorker& w, NativeAccountView& view)
    {
        view.saved = nativeAccount_Store(w.path, w.secrets);
        view.recovery_pending = *w.secrets.recovery != 0;
        if (!view.saved) std::snprintf(view.status, sizeof(view.status),
            "Signed in, but this PC could not save your session. Save your recovery code before closing.");
        return view.saved;
    }
    bool me(NativeAccountWorker& w, NativeAccountView& view)
    {
        if (!*w.secrets.token) return false;
        unsigned status;
        auto* root = nativeAccount_Http(w, w.secrets.handle, "me", false, true, view, &status);
        if (!root) {
            if (status == 401) {
                view.signed_in = false; view.social = {}; view.key_count = view.session_count = 0;
                // Preserve an unexported recovery code even if the session expired.
                if (!*w.secrets.recovery) {
                    nativeAccount_Delete(w.path);
                    AcquireSRWLockExclusive(&w.lock); SecureZeroMemory(&w.secrets, sizeof(w.secrets)); ReleaseSRWLockExclusive(&w.lock);
                    view.saved = false;
                }
            }
            return false;
        }
        NativeAccountView updated = view;
        auto* keys = yyjson_obj_get(root, "passkeys");
        auto* sessions = yyjson_obj_get(root, "sessions");
        bool ok = profile(root, updated) && !std::strcmp(updated.handle, w.secrets.handle) &&
            yyjson_is_arr(keys) && yyjson_arr_size(keys) > 0 && yyjson_arr_size(keys) <= 8 &&
            yyjson_is_arr(sessions) && yyjson_arr_size(sessions) > 0 && yyjson_arr_size(sessions) <= 16;
        updated.key_count = unsigned(yyjson_arr_size(keys)); updated.session_count = unsigned(yyjson_arr_size(sessions));
        for (unsigned i = 0; ok && i < updated.key_count; ++i) {
            auto* key = yyjson_arr_get(keys, i); BYTE id[1050]; DWORD length;
            ok = nativeAccount_String(yyjson_obj_get(key, "id"), updated.keys[i].id, sizeof(updated.keys[i].id)) &&
                nativeAccount_Base64Decode(updated.keys[i].id, id, sizeof(id), length) &&
                nativeAccount_String(yyjson_obj_get(key, "name"), updated.keys[i].name, sizeof(updated.keys[i].name));
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
    bool ceremony(NativeAccountWorker& w, const NativeAccountRequest& request, NativeAccountView& view)
    {
        const auto operation = request.operation;
        const bool registration = operation == NativeAccountOperation::Register || operation == NativeAccountOperation::Recover ||
            operation == NativeAccountOperation::AddPasskey;
        const bool authorized = operation == NativeAccountOperation::Approve || operation == NativeAccountOperation::AddPasskey;
        const char* kind = operation == NativeAccountOperation::Register ? "register" : operation == NativeAccountOperation::Recover ? "recover" :
            operation == NativeAccountOperation::Approve ? "reauth" : operation == NativeAccountOperation::AddPasskey ? "add" : "login";
        char handle[25], route[64], attempt[44];
        if (!nativeAccount_Handle(authorized ? w.secrets.handle : request.handle, handle)) {
            std::snprintf(view.status, sizeof(view.status), "Use 3-24 letters, numbers or underscores for your account name."); return false;
        }
        if (!view.available) { std::snprintf(view.status, sizeof(view.status), "Passkey sign-in requires Windows 10 version 1903 or newer."); return false; }
        if (operation == NativeAccountOperation::Recover && !nativeAccount_Secret(request.recovery)) {
            std::snprintf(view.status, sizeof(view.status), "Enter the complete recovery code you saved when creating your account."); return false;
        }
        auto* body = w.json.object();
        yyjson_mut_obj_add_str(w.json.output, body, "name", *request.name ? request.name : handle);
        if (operation == NativeAccountOperation::Recover) yyjson_mut_obj_add_str(w.json.output, body, "recovery_code", request.recovery);
        if (!w.json.write(w.send, sizeof(w.send))) return false;
        std::snprintf(route, sizeof(route), "%s/options", kind);
        auto* response = nativeAccount_Http(w, handle, route, true, authorized, view);
        if (!response) return false;
        if (!nativeAccount_String(yyjson_obj_get(response, "attempt"), attempt, sizeof(attempt)) || !nativeAccount_Secret(attempt)) {
            std::snprintf(view.status, sizeof(view.status), "The account service returned an invalid sign-in attempt."); return false;
        }
        body = w.json.object();
        yyjson_mut_obj_add_str(w.json.output, body, "attempt", attempt);
        yyjson_mut_obj_add_str(w.json.output, body, "device", "Windows PC");
        yyjson_mut_obj_add_str(w.json.output, body, "passkey_name", "Passkey");
        bool ok = !cancelled(w) && nativeAccount_Passkey(w.passkeys, w.window, response, w.endpoint, registration, w.json, body, view.status);
        if (!ok || cancelled(w)) {
            // Prompt dismissal never consumes the account's recovery code.
            char error[192]; std::memcpy(error, view.status, sizeof(error));
            body = w.json.object(); yyjson_mut_obj_add_str(w.json.output, body, "attempt", attempt);
            if (!cancelled(w) && w.json.write(w.send, sizeof(w.send))) nativeAccount_Http(w, handle, "cancel", true, false, view);
            std::snprintf(view.status, sizeof(view.status), "%s", *error ? error : "Sign-in cancelled.");
            return false;
        }
        if (!w.json.write(w.send, sizeof(w.send))) { std::snprintf(view.status, sizeof(view.status), "The passkey response was too large."); return false; }
        // Once proof is sent, collect and protect the result even if the player
        // backs out: registration/recovery may already have committed remotely.
        AcquireSRWLockExclusive(&w.lock);
        if (w.cancelled || w.stop) { ReleaseSRWLockExclusive(&w.lock); return false; }
        view.cancellable = false;
        w.view.cancellable = false;
        ReleaseSRWLockExclusive(&w.lock);
        std::snprintf(view.status, sizeof(view.status), "Completing sign-in...");
        publish(w, view);
        std::snprintf(route, sizeof(route), "%s/finish", kind);
        response = nativeAccount_Http(w, handle, route, true, authorized, view);
        if (!response) return false;
        if (!authorized) {
            NativeAccountSecrets secrets{};
            bool valid = profile(response, view) && !std::strcmp(view.handle, handle) &&
                nativeAccount_String(yyjson_obj_get(response, "token"), secrets.token, sizeof(secrets.token)) && nativeAccount_Secret(secrets.token);
            if (operation != NativeAccountOperation::Login) valid = valid &&
                nativeAccount_String(yyjson_obj_get(response, "recovery_code"), secrets.recovery, sizeof(secrets.recovery)) && nativeAccount_Secret(secrets.recovery);
            else if (!std::strcmp(w.secrets.handle, handle)) std::memcpy(secrets.recovery, w.secrets.recovery, sizeof(secrets.recovery));
            if (!valid) { SecureZeroMemory(&secrets, sizeof(secrets)); std::snprintf(view.status, sizeof(view.status), "The account service returned an incomplete sign-in."); return false; }
            std::snprintf(secrets.origin, sizeof(secrets.origin), "%s", w.endpoint.origin);
            std::snprintf(secrets.handle, sizeof(secrets.handle), "%s", handle);
            AcquireSRWLockExclusive(&w.lock); w.secrets = secrets; ReleaseSRWLockExclusive(&w.lock);
            SecureZeroMemory(&secrets, sizeof(secrets));
            view.signed_in = true;
            ++view.profile_revision;
            persist(w, view);
        }
        if (!me(w, view)) return false;
        if (view.saved) std::snprintf(view.status, sizeof(view.status), operation == NativeAccountOperation::Approve ? "Account changes approved for five minutes." :
            view.recovery_pending ? "Signed in. Save your recovery code somewhere safe." : "Signed in.");
        return true;
    }
    void run(NativeAccountWorker& w, const NativeAccountRequest& request, NativeAccountView& view)
    {
        if (nativeAccount_RunSocial(w, request, view)) return;
        const auto operation = request.operation;
        if (*w.secrets.recovery && (operation == NativeAccountOperation::Logout || operation == NativeAccountOperation::RevokeAll ||
            operation == NativeAccountOperation::Register || operation == NativeAccountOperation::Recover)) {
            std::snprintf(view.status, sizeof(view.status), "Save your recovery code before signing out or replacing this account."); return;
        }
        if (operation >= NativeAccountOperation::Register && operation <= NativeAccountOperation::AddPasskey) { ceremony(w, request, view); return; }
        if (operation == NativeAccountOperation::Refresh) {
            if (me(w, view)) std::snprintf(view.status, sizeof(view.status), "Signed in.");
            return;
        }
        if (operation == NativeAccountOperation::AcknowledgeRecovery) {
            NativeAccountSecrets previous = w.secrets;
            AcquireSRWLockExclusive(&w.lock); SecureZeroMemory(w.secrets.recovery, sizeof(w.secrets.recovery)); ReleaseSRWLockExclusive(&w.lock);
            if (!persist(w, view)) {
                AcquireSRWLockExclusive(&w.lock); w.secrets = previous; ReleaseSRWLockExclusive(&w.lock);
                view.recovery_pending = true;
            } else std::snprintf(view.status, sizeof(view.status), "Recovery code saved. Keep it somewhere safe.");
            SecureZeroMemory(&previous, sizeof(previous)); return;
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
        case NativeAccountOperation::RemovePasskey:
            yyjson_mut_obj_add_str(w.json.output, body, "id", request.target); route = "passkeys/remove"; break;
        case NativeAccountOperation::RevokeAll: route = "sessions/revoke-all"; break;
        case NativeAccountOperation::RotateRecovery: route = "recovery/rotate"; break;
        default: return;
        }
        if (!w.json.write(w.send, sizeof(w.send))) return;
        auto* response = nativeAccount_Http(w, w.secrets.handle, route, true, true, view);
        const bool remote = response != nullptr;
        if (operation == NativeAccountOperation::Logout || (remote && operation == NativeAccountOperation::RevokeAll)) {
            bool removed = nativeAccount_Delete(w.path);
            AcquireSRWLockExclusive(&w.lock); SecureZeroMemory(&w.secrets, sizeof(w.secrets)); ReleaseSRWLockExclusive(&w.lock);
            view.signed_in = view.saved = view.recovery_pending = false; view.social = {}; view.key_count = view.session_count = 0;
            std::snprintf(view.status, sizeof(view.status), !removed ? "Signed out. The old session file could not be removed." :
                remote ? "Signed out." : "Signed out on this PC. Could not revoke the session online."); return;
        }
        if (!response) return;
        if (operation == NativeAccountOperation::RotateRecovery) {
            char recovery[44];
            if (!nativeAccount_String(yyjson_obj_get(response, "recovery_code"), recovery, sizeof(recovery)) || !nativeAccount_Secret(recovery)) {
                std::snprintf(view.status, sizeof(view.status), "The replacement recovery code could not be read."); return;
            }
            AcquireSRWLockExclusive(&w.lock); std::memcpy(w.secrets.recovery, recovery, sizeof(recovery)); ReleaseSRWLockExclusive(&w.lock);
            SecureZeroMemory(recovery, sizeof(recovery));
            if (persist(w, view)) std::snprintf(view.status, sizeof(view.status), "Save your new recovery code. The previous code has expired."); return;
        }
        if (me(w, view)) std::snprintf(view.status, sizeof(view.status), "Account updated.");
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
            if (cancelled(w)) std::snprintf(view.status, sizeof(view.status), "Request cancelled. Refresh to check your account.");
            SecureZeroMemory(&request, sizeof(request));
            SecureZeroMemory(w.send, sizeof(w.send)); SecureZeroMemory(w.receive, sizeof(w.receive));
            // JSON pools also held copies of tokens and recovery codes.
            yyjson_doc_free(w.json.input); w.json.input = nullptr;
            yyjson_mut_doc_free(w.json.output); w.json.output = nullptr;
            SecureZeroMemory(w.json.read_pool, sizeof(w.json.read_pool));
            SecureZeroMemory(w.json.write_pool, sizeof(w.json.write_pool));
            view.busy = view.cancellable = false;
            publish(w, view);
        }
        return 0;
    }
}
bool nativeAccount_Start(const char* root, const char* endpoint, void* window)
{
    if (worker) return true;
    auto* w = new (std::nothrow) NativeAccountWorker;
    if (!w) return false;
    wchar_t directory[900];
    bool ok = nativeAccount_Endpoint(endpoint, w->endpoint) &&
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, root, -1, directory, 900);
    if (ok) {
        CreateDirectoryW(directory, nullptr);
        ok = swprintf_s(w->path, L"%s/account-session.bin", directory) > 0;
    }
    w->window = static_cast<HWND>(window);
    w->view.available = w->passkeys.load();
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
        std::snprintf(w->view.handle, sizeof(w->view.handle), "%s", w->secrets.handle);
        std::snprintf(w->view.status, sizeof(w->view.status), w->view.saved ? "Checking your saved sign-in..." : "Create an account or sign in with a passkey.");
        w->view.revision = 1;
        w->thread = CreateThread(nullptr, 0, thread_main, w, 0, nullptr);
    }
    if (!w->thread) {
        if (w->wake) CloseHandle(w->wake);
        if (w->connection) WinHttpCloseHandle(w->connection);
        if (w->session) WinHttpCloseHandle(w->session);
        w->passkeys.unload(); delete w; return false;
    }
    worker = w;
    if (w->view.saved) nativeAccount_Submit({NativeAccountOperation::Refresh});
    return true;
}
bool nativeAccount_Submit(const NativeAccountRequest& request)
{
    if (!worker || request.operation > NativeAccountOperation::BlockPlayer ||
        !std::memchr(request.handle, 0, sizeof(request.handle)) || !std::memchr(request.name, 0, sizeof(request.name)) ||
        !std::memchr(request.bio, 0, sizeof(request.bio)) || !std::memchr(request.recovery, 0, sizeof(request.recovery)) ||
        !std::memchr(request.target, 0, sizeof(request.target)) ||
        !std::memchr(request.subject, 0, sizeof(request.subject)) || !std::memchr(request.text, 0, sizeof(request.text)) ||
        !std::memchr(request.nonce, 0, sizeof(request.nonce)) || !std::memchr(request.action, 0, sizeof(request.action))) return false;
    auto& w = *worker;
    AcquireSRWLockExclusive(&w.lock);
    const bool ok = !w.stop && !w.view.busy && !w.queued;
    if (ok) {
        w.request = request; w.queued = true; w.cancelled = false; w.view.busy = true;
        w.view.cancellable = request.operation >= NativeAccountOperation::Register && request.operation <= NativeAccountOperation::AddPasskey;
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
    // Cancellation can race the instant Windows opens its prompt. Repeating it
    // while the cancelled ceremony is active also covers that small interval.
    if (worker->cancelled && worker->view.cancellable && worker->passkeys.cancel)
        worker->passkeys.cancel(&worker->passkeys.cancellation);
    ReleaseSRWLockShared(&worker->lock);
    return changed;
}
void nativeAccount_Cancel()
{
    if (!worker) return;
    AcquireSRWLockExclusive(&worker->lock);
    if (worker->view.cancellable) {
        worker->cancelled = true;
        if (worker->passkeys.cancel) worker->passkeys.cancel(&worker->passkeys.cancellation);
    }
    ReleaseSRWLockExclusive(&worker->lock);
}
void nativeAccount_Shutdown()
{
    if (!worker) return;
    nativeAccount_Cancel();
    AcquireSRWLockExclusive(&worker->lock); worker->stop = true; SetEvent(worker->wake); ReleaseSRWLockExclusive(&worker->lock);
    WaitForSingleObject(worker->thread, INFINITE);
    CloseHandle(worker->thread); CloseHandle(worker->wake);
    WinHttpCloseHandle(worker->connection); WinHttpCloseHandle(worker->session);
    worker->passkeys.unload(); SecureZeroMemory(&worker->secrets, sizeof(worker->secrets));
    delete worker; worker = nullptr;
}
bool nativeAccount_SaveRecovery(void* window)
{
    if (!worker) return false;
    char code[44]{};
    AcquireSRWLockShared(&worker->lock);
    bool ok = !worker->view.busy && *worker->secrets.recovery;
    if (ok) std::memcpy(code, worker->secrets.recovery, sizeof(code));
    ReleaseSRWLockShared(&worker->lock);
    if (!ok) return false;
    wchar_t path[1024] = L"Freak Melee recovery code.txt";
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog); dialog.hwndOwner = static_cast<HWND>(window);
    dialog.lpstrFile = path; dialog.nMaxFile = 1024;
    dialog.lpstrFilter = L"Text files\0*.txt\0\0"; dialog.lpstrDefExt = L"txt";
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;
    ok = GetSaveFileNameW(&dialog) != 0;
    if (ok) {
        HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        char text[256];
        int count = std::snprintf(text, sizeof(text), "Freak Melee recovery code\r\n\r\n%s\r\n\r\nKeep this private. It can replace your account's passkeys.\r\n", code);
        DWORD written;
        ok = file != INVALID_HANDLE_VALUE && WriteFile(file, text, count, &written, nullptr) && written == count && FlushFileBuffers(file);
        if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
        SecureZeroMemory(text, sizeof(text));
    }
    SecureZeroMemory(code, sizeof(code));
    if (ok) nativeAccount_Submit({NativeAccountOperation::AcknowledgeRecovery});
    return ok;
}
