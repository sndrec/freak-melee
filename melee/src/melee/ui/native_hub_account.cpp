#include "native_hub_account.h"
#include <melee/gm/native_account.h>
#include <melee/gm/native_services.h>
#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace {
    NativeAccountView account;
    bool started, attempted;
    void* window;
    unsigned mode, offset, profile_revision;
    HubAction confirmation = HubAction::None;
    int confirm_index;
    constexpr uint32_t gold = 0xD9B754FF, muted = 0xB9ADBDFF;

    void submit(NativeAccountOperation operation, const char* target = nullptr)
    {
        NativeAccountRequest request{};
        request.operation = operation;
        std::snprintf(request.handle, sizeof(request.handle), "%s", hub.account_handle);
        std::snprintf(request.name, sizeof(request.name), "%s", hub.name);
        std::snprintf(request.bio, sizeof(request.bio), "%s", hub.bio);
        std::snprintf(request.recovery, sizeof(request.recovery), "%s", hub.account_recovery);
        if (target) std::snprintf(request.target, sizeof(request.target), "%s", target);
        nativeHub_ChangePage(hub.page);
        if (!nativeAccount_Submit(request)) nativeHub_Notice("An account request is already running. Please wait.");
        SDL_memset(hub.account_recovery, 0, sizeof(hub.account_recovery));
        SDL_memset(&request, 0, sizeof(request));
    }
    void security_action(HubAction action, int index)
    {
        switch (action) {
        case HubAction::AccountLogout: submit(NativeAccountOperation::Logout); break;
        case HubAction::AccountRevokeAll: submit(NativeAccountOperation::RevokeAll); break;
        case HubAction::AccountRotateRecovery: submit(NativeAccountOperation::RotateRecovery); break;
        case HubAction::AccountRemoveKey:
            if (index >= 0 && unsigned(index) < account.key_count) submit(NativeAccountOperation::RemovePasskey, account.keys[index].id);
            break;
        case HubAction::AccountRevokeSession:
            if (index >= 0 && unsigned(index) < account.session_count) submit(NativeAccountOperation::RevokeSession, account.sessions[index].id);
            break;
        default: break;
        }
    }
}
void nativeHubAccount_Tick()
{
    if (!attempted && hub.initialized) {
        attempted = true;
        char buffer[512]; const char* root = nativeHubCatalog_Root(buffer);
        int count = 0; SDL_Window** windows = SDL_GetWindows(&count);
        if (windows && count) window = SDL_GetPointerProperty(SDL_GetWindowProperties(windows[0]), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        SDL_free(windows);
        const char* endpoint = std::getenv("FREAK_MELEE_SERVICE_ENDPOINT");
        started = root && nativeAccount_Start(root, endpoint && *endpoint ? endpoint : NATIVE_SERVICE_ENDPOINT, window);
        if (!started) {
            std::snprintf(account.status, sizeof(account.status), "Account sign-in could not start. Check the service address and storage folder.");
            return;
        }
    }
    if (!nativeAccount_Poll(account)) return;
    if (!*hub.account_handle && *account.handle) std::snprintf(hub.account_handle, sizeof(hub.account_handle), "%s", account.handle);
    if (account.signed_in && !account.busy && account.profile_revision != profile_revision) {
        profile_revision = account.profile_revision;
        std::snprintf(hub.public_name, sizeof(hub.public_name), "%s", account.name);
        // Do not overwrite a profile that the player is actively typing.
        if (hub.field != hub.name) std::snprintf(hub.name, sizeof(hub.name), "%s", account.name);
        if (hub.field != hub.bio) std::snprintf(hub.bio, sizeof(hub.bio), "%s", account.bio);
    }
    if (offset >= (mode == 4 ? account.key_count : account.session_count)) offset = 0;
}
void nativeHubAccount_Render()
{
    nativeHub_Panel(175, 116, 430, 286);
    nativeMenuPaint_Text(account.signed_in ? account.name : "Your online account", 194, 133, 370, 17, gold, NativeMenuFont_MenuOption);
    char handle[64]; std::snprintf(handle, sizeof(handle), "@%s", account.handle);
    nativeMenuPaint_Text(account.signed_in ? handle : "Sign in here using a Windows passkey prompt.", 194, 155, 370, 9, muted);
    if (account.busy) {
        nativeMenuPaint_Text("Waiting for the account service or passkey approval...", 194, 196, 370, 11, muted);
        if (account.cancellable) nativeHub_Button("account-cancel", "Cancel", 194, 236, 170, 31, HubAction::AccountCancel);
    } else if (account.recovery_pending) {
        nativeMenuPaint_Text("Save your recovery code", 194, 186, 370, 16, gold);
        nativeMenuPaint_Text("Use it to regain access if you lose your passkeys.", 194, 216, 370, 10, muted);
        nativeMenuPaint_Text("Keep the file private and somewhere safe.", 194, 237, 370, 10, muted);
        nativeHub_Button("account-save-recovery", "Save recovery code...", 194, 272, 390, 34, HubAction::AccountSaveRecovery, 0, true);
        nativeMenuPaint_Text("The code stays protected on this PC until you save it.", 194, 324, 370, 9, muted);
    } else if (confirmation != HubAction::None) {
        const char* title = confirmation == HubAction::AccountLogout ? "Sign out on this PC?" :
            confirmation == HubAction::AccountRevokeAll ? "Sign out on every device?" :
            confirmation == HubAction::AccountRotateRecovery ? "Replace your recovery code?" :
            confirmation == HubAction::AccountRemoveKey ? "Remove this passkey?" : "Sign out this device?";
        nativeMenuPaint_Text(title, 194, 192, 370, 15, gold);
        nativeMenuPaint_Text(confirmation == HubAction::AccountRotateRecovery ? "Your previous recovery code will expire." :
            confirmation == HubAction::AccountRemoveKey ? "You will need another passkey to sign in." : "You can sign in again with a passkey.",
            194, 225, 370, 10, muted);
        nativeHub_Button("account-confirm", "Confirm", 194, 280, 187, 32, HubAction::AccountConfirm, 1, true);
        nativeHub_Button("account-keep", "Cancel", 395, 280, 189, 32, HubAction::AccountConfirm, 0);
    } else if (!account.signed_in) {
        nativeHub_Button("account-login-mode", "Sign in", 194, 179, 120, 26, HubAction::AccountMode, 0, mode == 0);
        nativeHub_Button("account-create-mode", "Create account", 322, 179, 135, 26, HubAction::AccountMode, 1, mode == 1);
        nativeHub_Button("account-recover-mode", "Recover", 465, 179, 119, 26, HubAction::AccountMode, 2, mode == 2);
        nativeMenuPaint_Text("Account name", 194, 220, 370, 10, gold);
        nativeHub_Field("account-handle", hub.account_handle, 194, 237, 390, 27, HubAction::FieldAccountHandle);
        if (mode == 1) {
            nativeMenuPaint_Text("Display name", 194, 277, 370, 10, gold);
            nativeHub_Field("account-name", hub.name, 194, 293, 390, 27, HubAction::FieldName);
        } else if (mode == 2) {
            nativeMenuPaint_Text("Recovery code", 194, 277, 370, 10, gold);
            nativeHub_Field("account-recovery", hub.account_recovery, 194, 293, 390, 27, HubAction::FieldAccountRecovery, true);
        } else nativeMenuPaint_Text("3-24 letters, numbers or underscores.", 194, 280, 370, 9, muted);
        nativeHub_Button("account-submit", mode == 1 ? "Create account" : mode == 2 ? "Recover with a new passkey" : "Sign in with a passkey",
            194, 346, 390, 32, HubAction::AccountSubmit, 0, true, started && account.available && *hub.account_handle);
    } else if (mode == 4 || mode == 5) {
        nativeHub_Button("account-overview", "< Account", 194, 178, 125, 25, HubAction::AccountMode, 0);
        const unsigned total = mode == 4 ? account.key_count : account.session_count;
        nativeMenuPaint_Text(mode == 4 ? "Passkeys" : "Signed-in devices", 337, 185, 240, 11, gold);
        for (unsigned i = offset; i < total && i < offset + 5; ++i) {
            char id[32], label[90];
            std::snprintf(id, sizeof(id), "account-entry-%u", i);
            std::snprintf(label, sizeof(label), "%s%s", mode == 4 ? account.keys[i].name : account.sessions[i].name,
                mode == 5 && account.sessions[i].current ? " (this PC)" : "");
            float y = 219 + (i - offset) * 28;
            nativeMenuPaint_Text(label, 194, y + 6, 275, 10, muted);
            nativeHub_Button(id, mode == 4 ? "Remove" : "Sign out", 492, y, 92, 24,
                mode == 4 ? HubAction::AccountRemoveKey : HubAction::AccountRevokeSession, i, false, mode != 4 || total > 1);
        }
        nativeHub_Button("account-prev", "<", 504, 369, 35, 22, HubAction::AccountMode, mode == 4 ? 14 : 15, false, offset > 0);
        nativeHub_Button("account-next", ">", 549, 369, 35, 22, HubAction::AccountMode, mode == 4 ? 24 : 25, false, offset + 5 < total);
    } else {
        nativeHub_Button("account-approve", "Approve security changes", 194, 181, 390, 30, HubAction::AccountApprove, 0, true);
        nativeHub_Button("account-add-key", "Add passkey", 194, 221, 187, 29, HubAction::AccountAddKey);
        nativeHub_Button("account-keys", "Manage passkeys", 395, 221, 189, 29, HubAction::AccountMode, 4);
        nativeHub_Button("account-devices", "Signed-in devices", 194, 260, 187, 29, HubAction::AccountMode, 5);
        nativeHub_Button("account-all-out", "Sign out all devices", 395, 260, 189, 29, HubAction::AccountRevokeAll);
        nativeHub_Button("account-rotate", "New recovery code", 194, 299, 187, 29, HubAction::AccountRotateRecovery);
        nativeHub_Button("account-logout", "Sign out", 395, 299, 189, 29, HubAction::AccountLogout);
        nativeHub_Button("account-refresh", "Refresh", 194, 351, 130, 27, HubAction::AccountRefresh);
        nativeHub_Button("account-profile", "Edit profile", 338, 351, 246, 27, HubAction::Profile);
    }
}
const char* nativeHubAccount_Status()
{
    return account.status;
}
bool nativeHubAccount_Activate(HubAction action, int value)
{
    switch (action) {
    case HubAction::Account:
        mode = offset = 0; confirmation = HubAction::None;
        nativeHub_ChangePage(NativeHubPage_Account); return true;
    case HubAction::AccountMode:
        nativeHub_ChangePage(NativeHubPage_Account);
        SDL_memset(hub.account_recovery, 0, sizeof(hub.account_recovery));
        if (value >= 20) offset += 5;
        else if (value >= 10) offset = offset >= 5 ? offset - 5 : 0;
        else { mode = value; offset = 0; }
        return true;
    case HubAction::AccountSubmit:
        submit(account.signed_in ? NativeAccountOperation::SaveProfile : mode == 1 ? NativeAccountOperation::Register :
            mode == 2 ? NativeAccountOperation::Recover : NativeAccountOperation::Login); return true;
    case HubAction::SaveProfile:
        if (!account.signed_in) return false;
        submit(NativeAccountOperation::SaveProfile); return true;
    case HubAction::AccountApprove: submit(NativeAccountOperation::Approve); return true;
    case HubAction::AccountAddKey: submit(NativeAccountOperation::AddPasskey); return true;
    case HubAction::AccountRefresh: submit(NativeAccountOperation::Refresh); return true;
    case HubAction::AccountCancel: nativeAccount_Cancel(); return true;
    case HubAction::AccountSaveRecovery:
        nativeHub_Notice(nativeAccount_SaveRecovery(window) ? "Recovery code saved." : "Recovery code has not been saved. Try again when ready."); return true;
    case HubAction::AccountLogout: case HubAction::AccountRevokeAll: case HubAction::AccountRotateRecovery:
    case HubAction::AccountRemoveKey: case HubAction::AccountRevokeSession:
        confirmation = action; confirm_index = value; return true;
    case HubAction::AccountConfirm:
        if (value) security_action(confirmation, confirm_index);
        confirmation = HubAction::None; return true;
    default: return false;
    }
}
void nativeHubAccount_Leave()
{
    SDL_memset(hub.account_recovery, 0, sizeof(hub.account_recovery));
    confirmation = HubAction::None;
}
void nativeHubAccount_Inspect()
{
    std::fprintf(stderr, "[hub-account] started=%d available=%d busy=%d signed-in=%d saved=%d recovery-pending=%d mode=%u keys=%u devices=%u\n",
        started, account.available, account.busy, account.signed_in, account.saved, account.recovery_pending, mode, account.key_count, account.session_count);
}
void nativeHubAccount_Shutdown()
{
    nativeAccount_Shutdown(); account = {}; started = attempted = false; window = nullptr; mode = offset = profile_revision = 0;
    nativeHubAccount_Leave();
}
