#include "native_hub_account.h"
#include "native_hub_startup.h"
#include "native_hub_notice.h"
#include <melee/gm/native_account.h>
#include <melee/gm/native_services.h>
#include <melee/gm/native_online_session.h>
#include <melee/mn/native_online_menu.h>
#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace {
    NativeAccountView account;
    bool started, attempted, leave_lobby;
    bool awaiting_sign_in;
    NativeAccountOperation sign_in_operation;
    void* window;
    unsigned mode, offset, profile_revision;
    uint64_t attach_retry;
    HubAction confirmation = HubAction::None;
    int confirm_index;
    constexpr uint32_t gold = 0xD9B754FF, muted = 0xB9ADBDFF;

    void startup_choices()
    {
        nativeHub_Button("account-create-mode","Sign-Up",74,191,224,42,HubAction::AccountMode,1);
        nativeHub_Button("account-login-mode","Login",97,249,224,42,HubAction::AccountMode,0);
        nativeHub_Button("setup-next","Continue as Guest",74,315,247,36,HubAction::StartupNext);
        nativeHub_Button("account-recover-mode","Recover",488,402,112,25,HubAction::AccountMode,2);
        const char* heading = "Your account";
        const char* text = "Keep friends, letters and invitations with an online account.";
        if (nativeHub_Hover(74,191,224,42)) { heading="Sign-Up"; text="Choose an account name. Freak Melee creates an account key for you and gives you a recovery code to save."; }
        else if (nativeHub_Hover(97,249,224,42)) { heading="Login"; text="Use a key saved on this PC, or import your account-key file. Your existing profile comes with your account."; }
        else if (nativeHub_Hover(74,315,247,36)) { heading="Continue as Guest"; text="Play locally or online and use chat. Set a display name on the next page. You can create an account later."; }
        else if (nativeHub_Hover(488,402,112,25)) { heading="Recover your account"; text="Use your saved recovery code if you have lost your account key. Recovery replaces old keys, signs out other devices and gives you a new key and recovery code."; }
        nativeMenuPaint_Text(heading,364,199,224,18,gold,NativeMenuFont_MenuOption);
        float row_y=240;
        while (*text) {
            while (*text==' ') ++text;
            char row[128]{}; unsigned n=0, space=0;
            while (text[n] && n<126) {
                row[n]=text[n]; row[n+1]=0;
                if (nativeMenuFont_Width(NativeMenuFont_Notice,row,12)>224 && n) { if(space)n=space; break; }
                if (text[n]==' ') space=n;
                ++n;
            }
            row[n]=0; nativeMenuPaint_Text(row,364,row_y,224,12,muted);
            text+=n; row_y+=20;
        }
    }

    void submit(NativeAccountOperation operation, const char* target = nullptr)
    {
        NativeAccountRequest request{};
        request.operation = operation;
        std::snprintf(request.handle, sizeof(request.handle), "%s", hub.account_handle);
        std::snprintf(request.name, sizeof(request.name), "%s", hub.name);
        std::snprintf(request.bio, sizeof(request.bio), "%s", hub.bio);
        std::snprintf(request.recovery, sizeof(request.recovery), "%s", hub.account_recovery);
        if (target) std::snprintf(request.target, sizeof(request.target), "%s", target);
        if (nativeAccount_Submit(request)) {
            if (operation == NativeAccountOperation::Register || operation == NativeAccountOperation::Login ||
                operation == NativeAccountOperation::Recover) {
                awaiting_sign_in = true;
                sign_in_operation = operation;
            }
            nativeHub_ChangePage(hub.page);
        }
        else nativeHub_Notice("An account request is already running. Please wait.");
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
            if (index >= 0 && unsigned(index) < account.key_count) submit(NativeAccountOperation::RemoveKey, account.keys[index].id);
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
        started = root && nativeAccount_Start(root, endpoint && *endpoint ? endpoint : NATIVE_SERVICE_ENDPOINT);
        if (!started) {
            std::snprintf(account.status, sizeof(account.status), "Account sign-in could not start. Check the service address and storage folder.");
            return;
        }
    }
    const bool was_signed_in = account.signed_in;
    char previous[44]; std::snprintf(previous, sizeof(previous), "%s", account.id);
    const bool changed = nativeAccount_Poll(account);
    if (was_signed_in && (!account.signed_in || std::strcmp(previous, account.id))) {
        leave_lobby = true; mode = offset = 0; confirmation = HubAction::None;
    }
    if (leave_lobby && !account.busy && nativeOnlineMenu_IsAvailable()) {
        if (nativeOnlineSession_State() != NativeLobbySessionState_Disconnected) {
            nativeOnlineSession_Stop(); hub.pending_listing = false;
            nativeHub_Notice("You left the lobby because your account signed out.");
        }
        leave_lobby = false;
    }
    if (account.signed_in && !account.busy && !leave_lobby && SDL_GetTicks() >= attach_retry &&
        nativeOnlineMenu_IsAvailable() && nativeOnlineSession_State() == NativeLobbySessionState_Waiting) {
        const auto* presence = nativeOnlineSession_Presence();
        const char* peer = nativeOnlineSession_LocalPeerId();
        const char* connection = nativeOnlineSession_ConnectionId();
        const char* room = nativeOnlineSession_RoomCode();
        if (presence && peer && connection && room && std::strlen(connection) == 36) {
            for (unsigned i = 0; i < presence->participant_count; ++i) {
                const auto& member = presence->participants[i];
                if (std::strcmp(peer, member.peer_id) || !std::strcmp(account.id, member.account_id)) continue;
                NativeAccountRequest request{}; request.operation = NativeAccountOperation::AttachLobby;
                std::snprintf(request.room, sizeof(request.room), "%s", room);
                std::snprintf(request.target, sizeof(request.target), "%s", connection);
                if (nativeAccount_Submit(request)) attach_retry = SDL_GetTicks() + 5000;
                SDL_memset(&request, 0, sizeof(request));
                break;
            }
        }
    }
    if (!changed) return;
    if (awaiting_sign_in && !account.busy) {
        awaiting_sign_in = false;
        if (!account.signed_in) nativeHubNotice_Show(
            sign_in_operation == NativeAccountOperation::Register ? "Could not create account" :
            sign_in_operation == NativeAccountOperation::Recover ? "Could not recover account" : "Could not sign in",
            account.status);
    }
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
    const float x = nativeHubStartup.active ? -70 : -130;
    const float width = nativeHubStartup.active ? 390 : 520;
    const float half = nativeHubStartup.active ? 187 : 246;
    const float right = nativeHubStartup.active ? x+395 : 338;
    nativeMenuPaint_Text(account.signed_in ? account.name : nativeHubStartup.active ? "Online account" : "Your online account",
        nativeHubStartup.active ? 65 : 64, nativeHubStartup.active ? 111 : 133,
        nativeHubStartup.active ? 510 : 340, nativeHubStartup.active ? 33 : 23, gold,
        nativeHubStartup.active ? NativeMenuFont_MenuCaption : NativeMenuFont_MenuOption);
    char handle[64]; std::snprintf(handle, sizeof(handle), "@%s", account.handle);
    if (!nativeHubStartup.active || account.signed_in)
        nativeMenuPaint_Text(account.signed_in ? handle : "Your account key signs you in automatically on this PC.", x + 194, 155, 370, 9, muted);
    if (nativeHubStartup.active && account.signed_in && !account.recovery_pending && !account.busy) {
        nativeMenuPaint_Text("Signed in", 344, 285, 200, 20, gold, NativeMenuFont_MenuCaption);
        return;
    }
    if (nativeHubStartup.active && !account.signed_in && !account.busy && !account.recovery_pending && mode == 3) {
        startup_choices(); return;
    }
    if (!nativeHubStartup.active && !account.signed_in)
        nativeHub_Button("account-profile","Your profile",429,124,155,27,HubAction::Profile);
    if (account.busy) {
        nativeMenuPaint_Text("Contacting the account service...", x + 194, 196, 370, 11, muted);
        if (account.cancellable) nativeHub_Button("account-cancel", "Cancel", x + 194, 236, 170, 31, HubAction::AccountCancel);
    } else if (account.recovery_pending) {
        nativeMenuPaint_Text("Save your recovery code", x + 194, 186, 370, 16, gold);
        nativeMenuPaint_Text("Use it to regain access if you lose your account keys.", x + 194, 216, 370, 10, muted);
        nativeMenuPaint_Text("Keep the file private and somewhere safe.", x + 194, 237, 370, 10, muted);
        nativeHub_Button("account-save-recovery", "Save recovery code...", x + 194, 272, width, 34, HubAction::AccountSaveRecovery, 0, true);
        nativeMenuPaint_Text("The code stays protected on this PC until you save it.", x + 194, 324, 370, 9, muted);
    } else if (confirmation != HubAction::None) {
        const char* title = confirmation == HubAction::AccountLogout ? "Sign out on this PC?" :
            confirmation == HubAction::AccountRevokeAll ? "Sign out on every device?" :
            confirmation == HubAction::AccountRotateRecovery ? "Replace your recovery code?" :
            confirmation == HubAction::AccountRemoveKey ? "Remove this account key?" : "Sign out this device?";
        nativeMenuPaint_Text(title, x + 194, 192, 370, 15, gold);
        nativeMenuPaint_Text(confirmation == HubAction::AccountRotateRecovery ? "Your previous recovery code will expire." :
            confirmation == HubAction::AccountLogout || confirmation == HubAction::AccountRevokeAll ? "You will also leave your current lobby." :
            confirmation == HubAction::AccountRemoveKey ? "You will need another account key to sign in." : "You can sign in again with an account key.",
            x + 194, 225, 370, 10, muted);
        nativeHub_Button("account-confirm", "Confirm", x + 194, 280, half, 32, HubAction::AccountConfirm, 1, true);
        nativeHub_Button("account-keep", "Cancel", right, 280, half, 32, HubAction::AccountConfirm, 0);
    } else if (!account.signed_in) {
        if (nativeHubStartup.active) {
            nativeHub_Button("account-overview","< Account",75,171,124,27,HubAction::AccountMode,3);
            nativeMenuPaint_Text(mode==1?"Sign-Up":mode==2?"Recover":"Login",330,176,250,20,gold,NativeMenuFont_MenuOption);
        } else {
        float tab_height = 26;
        nativeHub_Button("account-login-mode", "Sign in", 64, 179, 162, tab_height, HubAction::AccountMode, 0, mode == 0);
        nativeHub_Button("account-create-mode", "Create account", 244, 179, 162, tab_height, HubAction::AccountMode, 1, mode == 1);
        nativeHub_Button("account-recover-mode", "Recover", 426, 179, 158, tab_height, HubAction::AccountMode, 2, mode == 2);
        }
        if (mode == 0) {
            char saved[80]; std::snprintf(saved, sizeof(saved), "Saved account: @%s", account.handle);
            nativeMenuPaint_Text(account.has_key ? saved : "Have an account on another PC?", x + 194, 226, 370, 12, gold);
            nativeHub_Button("account-import", "Import account-key file...", x + 194, 255, width, 30, HubAction::AccountImportKey);
            if (account.has_key) nativeHub_Button("account-export", "Export saved account key...", x + 194, 296, width, 28, HubAction::AccountExportKey);
        } else {
            nativeMenuPaint_Text("Account name", x + 194, 220, 370, 10, gold);
            nativeHub_Field("account-handle", hub.account_handle, x + 194, 237, width, 27, HubAction::FieldAccountHandle);
            nativeMenuPaint_Text(mode == 1 ? "Display name" : "Recovery code", x + 194, 277, 370, 10, gold);
            if (mode == 1) nativeHub_Field("account-name", hub.name, x + 194, 293, width, 27, HubAction::FieldName);
            else nativeHub_Field("account-recovery", hub.account_recovery, x + 194, 293, width, 27, HubAction::FieldAccountRecovery, true);
        }
        nativeHub_Button("account-submit", mode == 1 ? "Create account" : mode == 2 ? "Recover account" : "Sign in",
            x + 194, 346, width, 32, HubAction::AccountSubmit, 0, true,
            started && account.available && (mode == 0 ? account.has_key : *hub.account_handle != 0));
    } else if (mode == 4 || mode == 5) {
        nativeHub_Button("account-overview", "< Account", x + 194, 178, 125, 25, HubAction::AccountMode, 0);
        const unsigned total = mode == 4 ? account.key_count : account.session_count;
        nativeMenuPaint_Text(mode == 4 ? "Account keys" : "Signed-in devices", x + 337, 185, 240, 11, gold);
        for (unsigned i = offset; i < total && i < offset + 5; ++i) {
            char id[32], label[90];
            std::snprintf(id, sizeof(id), "account-entry-%u", i);
            std::snprintf(label, sizeof(label), "%s [%.7s]%s", mode == 4 ? account.keys[i].name : account.sessions[i].name,
                mode == 4 ? account.keys[i].id : account.sessions[i].id,
                (mode == 4 ? account.keys[i].current : account.sessions[i].current) ? " (current)" : "");
            float y = 219 + (i - offset) * 28;
            nativeMenuPaint_Text(label, x + 194, y + 6, 275, 10, muted);
            nativeHub_Button(id, mode == 4 ? "Remove" : "Sign out", 492, y, 92, 24,
                mode == 4 ? HubAction::AccountRemoveKey : HubAction::AccountRevokeSession, i, false, mode != 4 || total > 1);
        }
        nativeHub_Button("account-prev", "<", 504, 369, 35, 22, HubAction::AccountMode, mode == 4 ? 14 : 15, false, offset > 0);
        nativeHub_Button("account-next", ">", 549, 369, 35, 22, HubAction::AccountMode, mode == 4 ? 24 : 25, false, offset + 5 < total);
    } else {
        nativeHub_Button("account-export", "Export account-key file...", x + 194, 181, width, 30, HubAction::AccountExportKey, 0, true);
        nativeHub_Button("account-add-key", "New key for another PC", x + 194, 221, half, 29, HubAction::AccountAddKey);
        nativeHub_Button("account-keys", "Manage keys", right, 221, half, 29, HubAction::AccountMode, 4);
        nativeHub_Button("account-devices", "Signed-in devices", x + 194, 260, half, 29, HubAction::AccountMode, 5);
        nativeHub_Button("account-all-out", "Sign out all devices", right, 260, half, 29, HubAction::AccountRevokeAll);
        nativeHub_Button("account-rotate", "New recovery code", x + 194, 299, half, 29, HubAction::AccountRotateRecovery);
        nativeHub_Button("account-logout", "Sign out", right, 299, half, 29, HubAction::AccountLogout);
        nativeHub_Button("account-refresh", "Refresh", x + 194, 351, 130, 27, HubAction::AccountRefresh);
        nativeHub_Button("account-profile", "Edit profile", 282, 351, 302, 27, HubAction::Profile);
    }
}
const char* nativeHubAccount_Status()
{
    return account.status;
}
bool nativeHubAccount_Activate(HubAction action, int value)
{
    if (action==HubAction::Back && hub.page==NativeHubPage_OnlineSettings) {
        if (confirmation!=HubAction::None) { confirmation=HubAction::None; return true; }
        if (mode==4 || mode==5 || (!account.signed_in && (mode==1 || mode==2))) { mode=offset=0; return true; }
    }
    switch (action) {
    case HubAction::Account:
        mode = offset = 0; confirmation = HubAction::None;
        nativeHub_ChangePage(nativeHubStartup.active ? NativeHubPage_Account : NativeHubPage_OnlineSettings); return true;
    case HubAction::AccountMode:
        nativeHub_ChangePage(nativeHubStartup.active ? NativeHubPage_Account : NativeHubPage_OnlineSettings);
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
    case HubAction::AccountExportKey: nativeAccount_ChooseFile(NativeAccountOperation::ExportKey, window); return true;
    case HubAction::AccountImportKey: nativeAccount_ChooseFile(NativeAccountOperation::ImportKey, window); return true;
    case HubAction::AccountAddKey: nativeAccount_ChooseFile(NativeAccountOperation::AddKey, window); return true;
    case HubAction::AccountRefresh: submit(NativeAccountOperation::Refresh); return true;
    case HubAction::AccountCancel: awaiting_sign_in = false; nativeAccount_Cancel(); return true;
    case HubAction::AccountSaveRecovery:
        nativeAccount_ChooseFile(NativeAccountOperation::ExportRecovery, window); return true;
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
    if (!nativeHubStartup.active) mode=offset=0;
    SDL_memset(hub.account_recovery, 0, sizeof(hub.account_recovery));
    confirmation = HubAction::None;
}
void nativeHubAccount_BeginStartup() { mode = 3; offset = 0; confirmation = HubAction::None; }
bool nativeHubAccount_BackStartup()
{
    if (account.busy || account.recovery_pending) return true;
    if (mode != 3 && !account.signed_in) { mode=3; nativeHub_ChangePage(NativeHubPage_Account,true); return true; }
    return false;
}
bool nativeHubAccount_StartupContinue() { return account.signed_in; }
void nativeHubAccount_Inspect()
{
    const char* connection = nativeOnlineSession_ConnectionId();
    std::fprintf(stderr, "[hub-account] started=%d available=%d busy=%d signed-in=%d saved=%d recovery-pending=%d mode=%u keys=%u devices=%u lobby-proof-ready=%d lobby-available=%d attach-retry=%d\n",
        started, account.available, account.busy, account.signed_in, account.saved, account.recovery_pending, mode, account.key_count, account.session_count,
        connection && std::strlen(connection) == 36, nativeOnlineMenu_IsAvailable(), SDL_GetTicks() < attach_retry);
}
const NativeAccountView& nativeHubAccount_View() { return account; }
void nativeHubAccount_Shutdown()
{
    nativeAccount_Shutdown(); account = {}; started = attempted = leave_lobby = awaiting_sign_in = false; window = nullptr; mode = offset = profile_revision = 0;
    attach_retry = 0;
    nativeHubAccount_Leave();
}
