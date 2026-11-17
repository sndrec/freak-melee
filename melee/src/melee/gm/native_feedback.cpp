#include "native_feedback.h"
#include "native_feedback_context.h"
#include "native_feedback_input.h"
#include "native_community.h"
#include "native_online_session.h"
#include <melee/ui/native_feedback_view.h>
#include <melee/ui/native_hub.h>
#include <melee/mn/native_css_interaction.h>
extern "C" {
#include <melee/lb/lb_0195.h>
#include <sysdolphin/baselib/controller.h>
}
#include <aurora/rmlui.hpp>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_keyboard.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
NativeFeedbackDraft draft;
NativeCommunityUpload upload;
NativeCommunityReceipt receipt;
bool open, online, context_ready, context_complete, submitted, sending, completed, old_cursor;
bool suppressed_keys[SDL_SCANCODE_COUNT];
unsigned suppressed_mouse;
unsigned shown_revision;
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
    if (!context_ready && !(context_ready = NativeFeedbackCapture::fallback(draft))) {
        nativeFeedbackView_Status("Could not save the diagnostic capture. Free some disk space, then press Enter to retry. Your screenshot and notes are retained.", false, false, false); return;
    }
    if (capture.capture_id != draft.image.capture_id || capture.status != AURORA_CAPTURE_READY) {
        if (capture.capture_id == draft.image.capture_id && capture.status == AURORA_CAPTURE_FAILED &&
            aurora_write_presented_capture(draft.image_path)) {
            nativeFeedbackView_Status("Saving the original screenshot again. Press Enter to send when it is ready.", false, false, false); return;
        }
        nativeFeedbackView_Status(capture.status == AURORA_CAPTURE_FAILED ? capture.error : "The screenshot is still being saved. Please try again in a moment.", false, false, false);
        return;
    }
    if (!submitted && !NativeFeedbackCapture::notes(draft, upload, context_complete)) {
        nativeFeedbackView_Status("Could not save your notes. Check free disk space, then try again.", false, false, false); return;
    }
    // The first send seals the metadata too. A retry reuses all three files
    // and the same report ID; it never reads the current simulation again.
    submitted = true;
    if (!NativeFeedbackCapture::submit(upload)) {
        nativeFeedbackView_Status("The feedback service is busy or unavailable. Your report is saved on this PC; retry sends the same capture.", true, true, false); return;
    }
    sending = true; shown_revision = 0;
    nativeFeedbackView_Status("Uploading the captured screenshot, notes, and gameplay diagnostics…", true, false, false);
}
}

extern "C" bool nativeFeedback_Open(void)
{
    if (open) return false;
    receipt = {};
    if (!NativeFeedbackCapture::create(draft, upload, true, context_complete)) return false;
    context_ready = context_complete || NativeFeedbackCapture::fallback(draft);
    if (!nativeFeedbackView_Open(draft)) {
        aurora_release_presented_capture();
        DeleteFileA(draft.context_path); DeleteFileA(draft.image_path);
        return false;
    }
    if (!context_complete) nativeFeedbackView_Status(
        "Gameplay diagnostics could not be captured. You can still send this screenshot and your notes.", false, false, false);
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
