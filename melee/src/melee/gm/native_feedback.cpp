#include "native_feedback.h"
#include "native_feedback_context.h"
#include "native_feedback_input.h"
#include "native_community.h"
#include "native_services.h"
#include "native_online_session.h"
#include <melee/ui/native_feedback_view.h>
#include <melee/ui/native_hub_catalog.h>
#include <melee/ui/native_hub.h>
#include <melee/ui/native_shell_workshop.h>
#include <melee/ui/native_shell_workshop_editor.h>
#include <melee/mn/native_menu_scene.h>
#include <melee/mn/native_css_interaction.h>
extern "C" {
#include <melee/lb/lb_0195.h>
#include <sysdolphin/baselib/controller.h>
}
#include <aurora/rmlui.hpp>
#include <RmlUi/Core/Element.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_keyboard.h>
#include <yyjson.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
NativeFeedbackDraft draft;
NativeCommunityUpload upload;
NativeCommunityReceipt receipt;
bool open, online, context_ready, submitted, sending, completed, old_cursor;
bool suppressed_keys[SDL_SCANCODE_COUNT];
unsigned suppressed_mouse;
unsigned shown_revision;

bool directory(const char* path)
{ return CreateDirectoryA(path, nullptr) || GetLastError() == ERROR_ALREADY_EXISTS; }
bool new_id(char (&id)[37])
{
    unsigned char bytes[16];
    if (BCryptGenRandom(nullptr, bytes, sizeof(bytes), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) return false;
    bytes[6] = (bytes[6] & 15) | 0x40; bytes[8] = (bytes[8] & 63) | 0x80;
    std::snprintf(id, sizeof(id), "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        bytes[0],bytes[1],bytes[2],bytes[3],bytes[4],bytes[5],bytes[6],bytes[7],
        bytes[8],bytes[9],bytes[10],bytes[11],bytes[12],bytes[13],bytes[14],bytes[15]);
    return true;
}
bool metadata()
{
    auto* doc = yyjson_mut_doc_new(nullptr);
    if (!doc) return false;
    auto* root = yyjson_mut_obj(doc); yyjson_mut_doc_set_root(doc, root);
    yyjson_mut_obj_add_strcpy(doc, root, "id", draft.id);
    yyjson_mut_obj_add_strcpy(doc, root, "capture_at", draft.captured_at);
#ifdef NATIVE_NETPLAY_DIAGNOSTICS_BUILD
    yyjson_mut_obj_add_str(doc, root, "build", NATIVE_NETPLAY_DIAGNOSTICS_BUILD);
#else
    yyjson_mut_obj_add_str(doc, root, "build", "development");
#endif
    auto* pins = yyjson_mut_arr(doc); yyjson_mut_obj_add_val(doc, root, "pins", pins);
    for (unsigned i = 0; i < draft.count; ++i) {
        const auto& pin = draft.pins[i]; auto* row = yyjson_mut_obj(doc); yyjson_mut_arr_append(pins, row);
        yyjson_mut_obj_add_uint(doc, row, "id", pin.id);
        yyjson_mut_obj_add_real(doc, row, "x", pin.x); yyjson_mut_obj_add_real(doc, row, "y", pin.y);
        yyjson_mut_obj_add_strcpy(doc, row, "text", pin.text);
    }
    size_t length = 0; char* bytes = yyjson_mut_write(doc, 0, &length);
    FILE* file = bytes && length <= 512 * 1024 ? std::fopen(upload.metadata, "wb") : nullptr;
    bool ok = file && std::fwrite(bytes, 1, length, file) == length;
    if (file) ok = std::fclose(file) == 0 && ok;
    std::free(bytes); yyjson_mut_doc_free(doc); return ok;
}
void close(bool discard)
{
    // Release RmlUi's pressed state while the feedback document still owns
    // focus. Consume the real releases later, after the underlying UI returns.
    SDL_Event release{}; release.type = SDL_EVENT_KEY_UP;
    for (unsigned code = 0; code < SDL_SCANCODE_COUNT; ++code) if (suppressed_keys[code]) {
        release.key.scancode = SDL_Scancode(code);
        release.key.key = SDL_GetKeyFromScancode(release.key.scancode, SDL_KMOD_NONE, false);
        aurora::rmlui::handle_event(release);
    }
    release = {}; release.type = SDL_EVENT_MOUSE_BUTTON_UP;
    release.button.x = release.button.y = -100;
    for (unsigned button = 1; button <= 5; ++button) if (suppressed_mouse & SDL_BUTTON_MASK(button)) {
        release.button.button = Uint8(button); aurora::rmlui::handle_event(release);
    }
    nativeFeedbackView_Close(); aurora_release_presented_capture();
    nativeFeedbackInput_SetOpen(false); open = false;
    if (!old_cursor) SDL_HideCursor();
    if (!online) {
        HSD_PadFlushQueue(HSD_PAD_FLUSH_QUEUE_THROWAWAY);
        lbInput_RebaseClock(SDL_GetTicksNS());
    }
    if (discard && !submitted) {
        DeleteFileA(draft.image_path); DeleteFileA(draft.context_path); DeleteFileA(upload.metadata);
    }
}
void send()
{
    AuroraPresentedCapture capture{}; aurora_presented_capture_status(&capture);
    if (!context_ready) {
        nativeFeedbackView_Status("Could not save the gameplay context. Cancel this capture and try F2 again.", false, false, false); return;
    }
    if (capture.capture_id != draft.image.capture_id || capture.status != AURORA_CAPTURE_READY) {
        if (capture.capture_id == draft.image.capture_id && capture.status == AURORA_CAPTURE_FAILED &&
            aurora_write_presented_capture(draft.image_path)) {
            nativeFeedbackView_Status("Saving the original screenshot again. Press Enter to send when it is ready.", false, false, false); return;
        }
        nativeFeedbackView_Status(capture.status == AURORA_CAPTURE_FAILED ? capture.error : "The screenshot is still being saved. Please try again in a moment.", false, false, false);
        return;
    }
    if (!submitted && !metadata()) {
        nativeFeedbackView_Status("Could not save your notes. Check free disk space, then try again.", false, false, false); return;
    }
    // The first send seals the metadata too. A retry reuses all three files
    // and the same report ID; it never reads the current simulation again.
    submitted = true;
    char root[512]; const char* content = nativeHubCatalog_Root(root);
    const char* endpoint = std::getenv("FREAK_MELEE_COMMUNITY_ENDPOINT");
    if (!content || !nativeCommunity_Start(content, endpoint && *endpoint ? endpoint : NATIVE_COMMUNITY_ENDPOINT) ||
        !nativeCommunity_RequestUpload(upload)) {
        nativeFeedbackView_Status("The feedback service is busy or unavailable. Your report is saved on this PC; retry sends the same capture.", true, true, false); return;
    }
    sending = true; shown_revision = 0;
    nativeFeedbackView_Status("Uploading the captured screenshot, notes, and gameplay diagnostics…", true, false, false);
}
}

extern "C" bool nativeFeedback_Open(void)
{
    if (open) return false;
    draft = {}; upload = {}; receipt = {};
    char root[512], folder[1024];
    const char* content = nativeHubCatalog_Root(root);
    if (!content || !new_id(draft.id) || std::snprintf(folder, sizeof(folder), "%s/feedback", content) >= sizeof(folder) || !directory(folder)) return false;
    std::snprintf(draft.image_path, sizeof(draft.image_path), "%s/%s.png", folder, draft.id);
    std::snprintf(draft.context_path, sizeof(draft.context_path), "%s/%s.context.json", folder, draft.id);
    std::snprintf(upload.metadata, sizeof(upload.metadata), "%s/%s.notes.json", folder, draft.id);
    std::snprintf(upload.id, sizeof(upload.id), "%s", draft.id);
    std::snprintf(upload.image, sizeof(upload.image), "%s", draft.image_path);
    std::snprintf(upload.context, sizeof(upload.context), "%s", draft.context_path);
    SYSTEMTIME time; GetSystemTime(&time);
    std::snprintf(draft.captured_at, sizeof(draft.captured_at), "%04u-%02u-%02uT%02u:%02u:%02u.%03uZ",
        time.wYear,time.wMonth,time.wDay,time.wHour,time.wMinute,time.wSecond,time.wMilliseconds);
    // Reserve the displayed pixels and collect diagnostics before hiding UI,
    // changing input, pausing, or letting another simulation tick run.
    if (!aurora_capture_presented(draft.image_path, true, &draft.image)) return false;
    auto* context = aurora::rmlui::get_context();
    const auto* focused = context ? context->GetFocusElement() : nullptr;
    const bool workshop = nativeShellWorkshop_IsOpen();
    NativeFeedbackInterface interface_state{};
    interface_state.workspace = workshop ? "Workshop" : nativeHub_IsVisible() ? "Hub" : "Game";
    interface_state.mode = !workshop ? "" : nativeShellWorkshop_IsTesting() ? "Test Character" :
        nativeShellWorkshopEditor_IsModelMode() ? "Model" : nativeShellWorkshopEditor_IsAnimationMode() ? "Animation" :
        nativeShellWorkshopEditor_IsScriptMode() ? "Scripting" : "Action Editor";
    interface_state.hub_page = nativeHub_IsVisible() ? int(nativeHub_CurrentPage()) : -1;
    interface_state.menu_transition = nativeMenuScene_Active();
    interface_state.focused_control = focused ? focused->GetId().c_str() : "";
    context_ready = nativeFeedbackContext_Write(draft.context_path, draft.id, draft.captured_at, &draft.image,
        &interface_state);
    if (!nativeFeedbackView_Open(draft)) { aurora_release_presented_capture(); return false; }
    const char* room = nativeOnlineSession_RoomCode();
    online = nativeFeedbackContext_Online() || (room && *room);
    submitted = sending = completed = false; shown_revision = 0;
    old_cursor = SDL_CursorVisible(); SDL_ShowCursor();
    nativeHub_ReleasePointer(); nativeCssInteraction_CancelPointer();
    open = true; nativeFeedbackInput_SetOpen(true); aurora_set_capture_backdrop(true);
    return true;
}
extern "C" bool nativeFeedback_IsOpen(void) { return open; }
extern "C" bool nativeFeedback_PausesSimulation(void) { return open && !online; }
extern "C" bool nativeFeedback_Event(const SDL_Event* event)
{
    if (!event) return false;
    if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP) {
        const unsigned code = unsigned(event->key.scancode);
        if (code < SDL_SCANCODE_COUNT) {
            if (!open && suppressed_keys[code]) {
                if (event->type == SDL_EVENT_KEY_UP) suppressed_keys[code] = false;
                return true;
            }
            if (open) suppressed_keys[code] = event->type == SDL_EVENT_KEY_DOWN;
        }
        if (event->type == SDL_EVENT_KEY_DOWN && event->key.scancode == SDL_SCANCODE_F2) {
            if (!event->key.repeat && !open && !nativeFeedback_Open())
                std::fprintf(stderr, "[feedback] Could not capture the displayed interface.\n");
            if (code < SDL_SCANCODE_COUNT) suppressed_keys[code] = true;
            return true;
        }
    }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN || event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        const unsigned mask = SDL_BUTTON_MASK(event->button.button);
        if (!open && (suppressed_mouse & mask)) {
            if (event->type == SDL_EVENT_MOUSE_BUTTON_UP) suppressed_mouse &= ~mask;
            return true;
        }
        if (open) {
            if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) suppressed_mouse |= mask;
            else suppressed_mouse &= ~mask;
        }
    }
    if (!open) return false;
    const bool input = (event->type >= SDL_EVENT_KEY_DOWN && event->type <= SDL_EVENT_KEYBOARD_REMOVED) ||
        (event->type >= SDL_EVENT_MOUSE_MOTION && event->type <= SDL_EVENT_MOUSE_REMOVED) ||
        (event->type >= SDL_EVENT_FINGER_DOWN && event->type <= SDL_EVENT_FINGER_CANCELED);
    if (!input) return false;
    if (event->type == SDL_EVENT_KEY_DOWN && event->key.repeat &&
        (event->key.scancode == SDL_SCANCODE_ESCAPE ||
         (!(event->key.mod & SDL_KMOD_SHIFT) && (event->key.scancode == SDL_SCANCODE_RETURN ||
           event->key.scancode == SDL_SCANCODE_KP_ENTER)))) return true;
    if (event->type == SDL_EVENT_KEY_DOWN && nativeFeedbackView_Key(event->key.scancode,
        (event->key.mod & SDL_KMOD_SHIFT) != 0)) return true;
    SDL_Event copy = *event; aurora::rmlui::handle_event(copy);
    // An action may close the overlay in this dispatch. The closing event
    // still belongs to feedback, never to the menu underneath it.
    return true;
}
extern "C" void nativeFeedback_Update(void)
{
    if (!open) return;
    nativeFeedbackView_Update();
    switch (nativeFeedbackView_Action()) {
    case NativeFeedbackAction::ConfirmSend: nativeFeedbackView_Confirm(); break;
    case NativeFeedbackAction::ConfirmDiscard: nativeFeedbackView_Confirm(true); break;
    case NativeFeedbackAction::Send: case NativeFeedbackAction::Retry: if (!sending && !completed) send(); break;
    case NativeFeedbackAction::Discard: if (!sending) close(true); return;
    case NativeFeedbackAction::Close: if (!sending) close(false); return;
    default: break;
    }
    if (sending && nativeCommunity_PollReceipt(receipt) && !std::strcmp(receipt.id, draft.id) && shown_revision != receipt.revision) {
        shown_revision = receipt.revision;
        if (receipt.busy) {
            char message[160]; std::snprintf(message, sizeof(message), "Uploading feedback… %llu / %llu KB",
                (unsigned long long)(receipt.sent / 1024), (unsigned long long)(receipt.total / 1024));
            nativeFeedbackView_Status(message, true, false, false);
        } else {
            sending = false; completed = receipt.stored;
            nativeFeedbackView_Status(completed ? "Thank you! Your feedback has been received." : receipt.error,
                true, !completed, completed);
            if (completed) {
                DeleteFileA(upload.metadata); DeleteFileA(draft.context_path); DeleteFileA(draft.image_path);
            }
        }
    }
}
extern "C" void nativeFeedback_Shutdown(void) { if (open) close(false); }
