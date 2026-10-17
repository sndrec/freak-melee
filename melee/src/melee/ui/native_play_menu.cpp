#include "native_play_menu.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "native_hub.h"
#include "native_hub_hand.h"
#include <aurora/rmlui.hpp>
#include <melee/gm/native_home.h>
#include <melee/gm/native_online_session.h>
#include <melee/mn/native_css_interaction.h>
#include <melee/mn/native_css_original.h>
#include <melee/mn/native_menu_paint.h>
#include <RmlUi/Core.h>
extern "C" {
#include <melee/lb/lbaudio_ax.h>
#include <sysdolphin/baselib/controller.h>
}

namespace {
    enum class Action {
        Lobby,
        Workshop,
        Ready,
        Start,
        Selection,
        Accept,
        Decline,
        Queue
    };
    struct Button {
        const char* id;
        const char* label;
        float x, y, w, h;
        Action action;
        bool enabled, gold;
    };
    struct PlayMenu {
        Button buttons[8];
        unsigned count;
        char status[192], delivery[128];
        NativeNetplayGiftStatus gift;
        float pointer_x = -100, pointer_y = -100;
        bool selection, practice, fonts, ready_requested;
    } menu;

    bool hit(const Button& b, float x, float y)
    {
        return x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h;
    }
    void add(const char* id, const char* label, float x, float y, float w,
             float h, Action action, bool enabled = true, bool gold = false)
    {
        if (menu.count < sizeof(menu.buttons) / sizeof(*menu.buttons)) {
            menu.buttons[menu.count++] = { id, label,  x,       y,   w,
                                           h,  action, enabled, gold };
        }
    }
    void activate(const Button& b)
    {
        if (!b.enabled) {
            return;
        }
        lbAudioAx_80024030(1);
        switch (b.action) {
        case Action::Lobby:
            nativeHub_Open(NativeHubPage_Lobbies);
            break;
        case Action::Workshop:
            nativeHub_Open(NativeHubPage_Workshop);
            break;
        case Action::Ready:
            menu.ready_requested = true;
            break;
        case Action::Start:
            nativeHome_RequestStartMatch();
            break;
        case Action::Queue: {
            const auto* quick = nativeOnlineSession_Quick();
            if (quick && !nativeOnlineSession_SearchQuick(quick->state == NativeQuickState_Idle))
                nativeHub_Notice("Could not change the Quick Play search.");
            break;
        }
        case Action::Selection:
            nativeHome_ReturnToSelection();
            break;
        case Action::Accept:
            if (!nativeOnlineSession_AcceptGift()) {
                nativeHub_Notice("Could not accept this fighter.");
            }
            break;
        case Action::Decline:
            nativeOnlineSession_DeclineGift();
            break;
        }
    }
    bool click(float x, float y)
    {
        for (unsigned i = 0; i < menu.count; ++i) {
            if (hit(menu.buttons[i], x, y)) {
                activate(menu.buttons[i]);
                return true;
            }
        }
        return false;
    }
    void draw()
    {
        nativePlayMenu_DrawCountdown();
        if (menu.practice) {
            nativeMenuPaint_Round(7, 5, 626, 32, 5, 0x11101DDC);
            nativeMenuPaint_Text(menu.status, 220, 17, 275, 9, 0xEBDDAEFF);
        } else {
            nativeMenuPaint_Text(menu.status, 16, 460, 370, 8, 0xEBDDAEFF);
        }
        if (*menu.delivery) {
            float y = menu.practice ? 44 : 99;
            nativeMenuPaint_Round(364, y, 262, 78, 6, 0xD8B967FF);
            nativeMenuPaint_Round(365, y + 1, 260, 76, 5, 0x191421F5);
            nativeMenuPaint_Text(menu.delivery, 375, y + 10, 240, 9,
                                 0xF3E6CFFF);
            nativeMenuPaint_Text(menu.gift.title, 375, y + 28, 240, 11,
                                 0xFFF4E9FF);
        }
        for (unsigned i = 0; i < menu.count; ++i) {
            const Button& b = menu.buttons[i];
            bool hover = b.enabled && hit(b, menu.pointer_x, menu.pointer_y);
            uint32_t edge = !b.enabled ? 0x817785FF
                            : hover    ? 0xFFF389FF
                                       : 0xD6BC79FF;
            uint32_t fill = !b.enabled        ? 0x24202DDC
                            : b.gold || hover ? 0xDCC77AFF
                                              : 0x211B2BEF;
            nativeMenuPaint_Round(b.x, b.y + 1, b.w, b.h, 4, 0x05040ECC);
            nativeMenuPaint_Round(b.x, b.y, b.w, b.h, 4, edge);
            nativeMenuPaint_Round(b.x + 1, b.y + 1, b.w - 2, b.h - 2, 3, fill);
            nativeMenuPaint_Text(b.label, b.x + 5, b.y + (b.h - 10) * .5f,
                                 b.w - 10, 10,
                                 !b.enabled        ? 0xA19AA9FF
                                 : b.gold || hover ? 0x1E1724FF
                                                   : 0xF2E6CFFF,
                                 NativeMenuFont_Notice, true);
        }
    }
}

extern "C" void nativePlayMenu_Tick(bool selection, bool practice)
{
    if ((selection || practice) && !menu.fonts) {
        nativeMenuFont_Acquire();
        menu.fonts = true;
    }
    if (!(selection || practice)) {
        menu.pointer_x = menu.pointer_y = -100;
        menu.ready_requested = false;
    }
    menu.selection = selection;
    menu.practice = practice;
    menu.count = 0;
    menu.delivery[0] = 0;
    if (!(selection || practice)) {
        return;
    }
    const auto* quick = nativeOnlineSession_Quick();
    bool searching = quick && quick->state == NativeQuickState_Searching;
    bool assigned = quick && quick->state == NativeQuickState_Assigned;
    const char* queue_label = assigned || (quick && quick->state == NativeQuickState_Aborted)
        ? "Leave match" : searching ? "Cancel search" : "Enter queue";
    bool can_queue = quick && nativeOnlineSession_State() == NativeLobbySessionState_Waiting &&
        (assigned || nativeOnlineSession_IsOwner());
    if (selection) {
        add("play-ready", "Ready / START", 35, 207, 118, 27, Action::Ready,
            nativeCssInteraction_CanReady(), true);
        add("play-workshop", "Workshop", 419, 451, 113, 22, Action::Workshop);
        add("play-lobby", "Back", 542, 451, 83, 22, Action::Lobby);
        if (quick) add("play-queue", queue_label, 163, 207, 128, 27, Action::Queue, can_queue, !searching);
    } else {
        add("play-lobby", "Lobby", 13, 10, 88, 22, Action::Lobby);
        if (quick) add("play-queue", queue_label, 109, 10, 104, 22, Action::Queue, can_queue, !searching);
        else add("play-start", "Start match", 109, 10, 104, 22, Action::Start,
                 nativeHome_CanStartMatch(), true);
        add("play-selection", "Character select", 507, 10, 119, 22,
            Action::Selection);
    }
    NativeLobbySessionState state = nativeOnlineSession_State();
    const char* status = selection ? "Choose a fighter. START to ready."
                                   : "Practice | Pause, then Z for lobby";
    if (state == NativeLobbySessionState_Connecting) {
        status = "Connecting to lobby...";
    } else if (state == NativeLobbySessionState_Failed) {
        status = "Connection failed. Open Lobbies to reconnect.";
    } else if (state == NativeLobbySessionState_Waiting && practice) {
        status = nativeHome_CanStartMatch()      ? "Everyone ready"
                 : nativeOnlineSession_IsOwner() ? "Waiting for ready players"
                                                 : "Waiting for the host";
    }
    if (quick && state == NativeLobbySessionState_Waiting) {
        status = assigned ? "Match found. Connecting players..."
                 : searching ? "Searching for players..."
                 : quick->state == NativeQuickState_Aborted ? "Match cancelled. Leave to search again."
                 : "START to practice. Enter queue when ready to search.";
        if (*nativeOnlineSession_QuickError()) status = nativeOnlineSession_QuickError();
    }
    const char* code = nativeOnlineSession_RoomCode();
    if (code && *code) {
        std::snprintf(menu.status, sizeof(menu.status), "%s  |  %s", code,
                      status);
    } else {
        std::snprintf(menu.status, sizeof(menu.status), "%s", status);
    }
    NativeNetplayGiftStatus outgoing;
    if (nativeOnlineSession_GiftStatus(false, &outgoing)) {
        if (outgoing.state == NativeNetplayGift_Offered) {
            std::snprintf(menu.status, sizeof(menu.status),
                          "%s offered. Waiting for acceptance.",
                          outgoing.title);
        } else if (outgoing.state == NativeNetplayGift_Sending) {
            std::snprintf(menu.status, sizeof(menu.status), "Sending %s...",
                          outgoing.title);
        }
    }
    if (*nativeHub_NoticeText()) {
        std::snprintf(menu.status, sizeof(menu.status), "%s",
                      nativeHub_NoticeText());
    }
    NativeNetplayContentProgress progress;
    if (nativeOnlineSession_ContentProgress(&progress)) {
        std::snprintf(menu.status, sizeof(menu.status), "%s package %u / %u",
                      progress.direction == NativeNetplayContent_Receiving
                          ? "Receiving"
                          : "Sending",
                      progress.package_index, progress.package_count);
    }
    if (nativeOnlineSession_GiftStatus(true, &menu.gift) &&
        (menu.gift.state == NativeNetplayGift_Offered ||
         menu.gift.state == NativeNetplayGift_Receiving))
    {
        float y = practice ? 44 : 99;
        std::snprintf(menu.delivery, sizeof(menu.delivery), "%s",
                      menu.gift.state == NativeNetplayGift_Offered
                          ? "Fighter delivery"
                          : "Receiving fighter...");
        if (menu.gift.state == NativeNetplayGift_Offered) {
            add("play-accept", "Accept fighter", 375, y + 49, 139, 21,
                Action::Accept, true, true);
            add("play-decline", "No thanks", 522, y + 49, 94, 21,
                Action::Decline);
        } else if (menu.gift.bytes_total) {
            std::snprintf(menu.delivery, sizeof(menu.delivery),
                          "Receiving fighter... %u%%",
                          unsigned(uint64_t(menu.gift.bytes_done) * 100 /
                                   menu.gift.bytes_total));
        }
    }
}
extern "C" void nativePlayMenu_DrawCss(Mtx view)
{
    if (!menu.selection) {
        return;
    }
    float left, right, top, bottom;
    nativeCssOriginal_Viewport(&left, &right, &top, &bottom);
    Mtx canvas = { { (right - left) / 640, 0, 0, left },
                   { 0, (top - bottom) / 480, 0, top },
                   { 0, 0, 1, 5 } };
    Mtx model;
    C_MTXConcat(view, canvas, model);
    nativeMenuPaint_Begin(model);
    draw();
    nativeMenuPaint_End();
}
extern "C" void nativePlayMenu_DrawCountdown(void)
{
    uint16_t remaining = nativeOnlineSession_QuickCountdownMs();
    if (!remaining) return;
    char label[64];
    std::snprintf(label, sizeof(label), "Match starts in %u", (remaining + 999) / 1000);
    nativeMenuPaint_Round(195, 112, 250, 74, 8, 0xD6BC79FF);
    nativeMenuPaint_Round(197, 114, 246, 70, 7, 0x191421F5);
    nativeMenuPaint_Text(label, 210, 128, 220, 17, 0xFFF4E9FF, NativeMenuFont_Notice, true);
    nativeMenuPaint_Text("B to unready", 210, 158, 220, 10, 0xEBDDAEFF, NativeMenuFont_Notice, true);
}

extern "C" void nativePlayMenu_DrawPractice(void)
{
    if (!menu.practice) {
        return;
    }
    nativeMenuPaint_Begin(nullptr);
    draw();
    if (nativePlayMenu_PointerActive()) {
        nativeHubHand_Draw(menu.pointer_x, menu.pointer_y, false);
    }
    nativeMenuPaint_End();
}
extern "C" bool nativePlayMenu_Hand(float x, float y, uint32_t trigger)
{
    if (!menu.selection) {
        return false;
    }
    float left, right, top, bottom;
    nativeCssOriginal_Viewport(&left, &right, &top, &bottom);
    menu.pointer_x = (x - left) / (right - left) * 640;
    menu.pointer_y = (top - y) / (top - bottom) * 480;
    return (trigger & PAD_BUTTON_A) && click(menu.pointer_x, menu.pointer_y);
}
extern "C" bool nativePlayMenu_Pointer(float u, float v,
                                       NativeCssPointerEvent event)
{
    if (!menu.practice) {
        return false;
    }
    menu.pointer_x = u * 640;
    menu.pointer_y = v * 480;
    if (event == NativeCssPointer_Press) {
        return click(menu.pointer_x, menu.pointer_y);
    }
    return nativePlayMenu_PointerActive();
}
extern "C" bool nativePlayMenu_PointerActive(void)
{
    for (unsigned i = 0; menu.practice && i < menu.count; ++i) {
        if (hit(menu.buttons[i], menu.pointer_x, menu.pointer_y)) {
            return true;
        }
    }
    return false;
}
extern "C" bool nativePlayMenu_TakeReadyRequest(void)
{
    bool value = menu.selection && menu.ready_requested;
    menu.ready_requested = false;
    return value;
}
extern "C" bool nativePlayMenu_Inspect(const char* id)
{
    for (unsigned i = 0; i < menu.count; ++i) {
        const Button& b = menu.buttons[i];
        if (std::strcmp(b.id, id)) {
            continue;
        }
        float u = b.x / 640, v = b.y / 480, r = (b.x + b.w) / 640,
              t = (b.y + b.h) / 480;
        if (menu.selection) {
            float left, right, top, bottom;
            nativeCssOriginal_Viewport(&left, &right, &top, &bottom);
            nativeCssOriginal_PointToCanvas(left + (right - left) * u,
                                            top - (top - bottom) * v, &u, &v);
            nativeCssOriginal_PointToCanvas(left + (right - left) * r,
                                            top - (top - bottom) * t, &r, &t);
        }
        auto size = aurora::rmlui::get_context()->GetDimensions();
        float scale = std::min(size.x / 640.f, size.y / 480.f);
        std::fprintf(stderr,
                     "[play-menu] %s origin=%.1f,%.1f size=%.1f,%.1f "
                     "enabled=%d value=0\n",
                     id, (size.x - 640 * scale) * .5f + u * 640 * scale,
                     (size.y - 480 * scale) * .5f + v * 480 * scale,
                     (r - u) * 640 * scale, (t - v) * 480 * scale, b.enabled);
        if (const auto* quick = nativeOnlineSession_Quick()) {
            std::fprintf(stderr, "[quick-play] mode=%u state=%u generation=%llu team=%u stage=%u room=%s peer=%s phase=%u countdown=%u\n",
                quick->mode, quick->state, (unsigned long long) quick->generation,
                quick->team, quick->stage, nativeOnlineSession_RoomCode(),
                nativeOnlineSession_LocalPeerId(), quick->phase, nativeOnlineSession_QuickCountdownMs());
        }
        return true;
    }
    return false;
}
extern "C" void nativePlayMenu_Shutdown(void)
{
    if (menu.fonts) {
        nativeMenuFont_Release();
    }
    menu = PlayMenu{};
}
