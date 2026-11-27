#include "native_play_menu.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "native_hub.h"
#include "native_hub_hand.h"
#include "native_css_chat.h"
#include "native_hub_rules.h"
#include <aurora/rmlui.hpp>
#include <melee/ft/native_gx_image.h>
#include <melee/gm/native_home.h>
#include <melee/gm/native_online_session.h>
#include <melee/gm/native_spectator_match.h>
#include <melee/mn/native_css_interaction.h>
#include <melee/mn/native_css_original.h>
#include <melee/mn/native_menu_paint.h>
#include <melee/mn/native_menu_portrait.h>
#include <melee/mn/native_menu_skin.h>
#include <RmlUi/Core.h>
#include <SDL3/SDL_clipboard.h>
#include <SDL3/SDL_timer.h>
extern "C" {
#include <melee/lb/lbaudio_ax.h>
#include <sysdolphin/baselib/controller.h>
#include <melee/gm/gm_1A45.h>
#include <melee/gm/gm_1884.h>
#include <melee/gm/gmvs.h>
#include <melee/gm/native_practice.h>
#include <melee/gm/types.h>
}

namespace {
    enum class Action {
        Lobby,
        Ready,
        Watch,
        Accept,
        Decline,
        Queue,
        Stage,
        Settings, ChatInput, ChatSend, ChatHistory, CopyCode, Rotation, Opponent
    };
    struct Button {
        char id[40], label[96];
        int value;
        float x, y, w, h;
        Action action;
        bool enabled, gold, authored;
    };
    struct PlayMenu {
        Button buttons[40];
        unsigned count;
        char status[192], delivery[128];
        NativeNetplayGiftStatus gift;
        float pointer_x = -100, pointer_y = -100;
        bool selection, practice, fonts, ready_requested, show_status;
    } menu;
    struct PackageDownload {
        NativeContentId id;
        NativeMenuPortrait portrait;
        uint64_t started;
        uint32_t bytes_done, bytes_total;
        unsigned index, count;
        bool tried_portrait;
    } download;

    void clear_download()
    {
        nativeMenuPortrait_Free(&download.portrait);
        download = {};
    }

    void update_download(const NativeNetplayContentProgress& progress)
    {
        if (!download.started || std::memcmp(&download.id, &progress.id, sizeof(download.id))) {
            clear_download();
            download.id = progress.id;
            download.started = SDL_GetTicks();
        }
        download.bytes_done = progress.bytes_done;
        download.bytes_total = progress.bytes_total;
        download.index = progress.package_index;
        download.count = progress.package_count;
        if (*progress.preview.title) {
            std::memcpy(download.portrait.title, progress.preview.title,
                        sizeof(progress.preview.title));
        }
        if (progress.preview.portrait_png && !download.tried_portrait) {
            auto& art = download.portrait;
            download.tried_portrait = true;
            art.pixels = nativeGxImage_Decode(progress.preview.portrait_png,
                unsigned(progress.preview.portrait_size), &art.width, &art.height);
        }
    }

    void draw_download()
    {
        if (!download.started) return;
        const float t = std::min(1.f, float(SDL_GetTicks() - download.started) / 220.f);
        const float x = 458.f + 182.f * (1.f - t) * (1.f - t);
        const float y = menu.practice ? 48.f : 115.f;
        const auto& art = download.portrait;
        nativeMenuSkin_Panel(x, y, 170, 120, 0xD8B967FF, 0x09080EF9);
        if (art.pixels) nativeMenuPaint_Image(art.pixels, art.width, art.height,
            x + 59, y + 10, 52, 48, NativeMenuImage_Cover);
        char detail[64];
        if (download.count > 1) {
            std::snprintf(detail, sizeof(detail), "Receiving package %u / %u",
                          download.index, download.count);
        } else std::snprintf(detail, sizeof(detail), "Receiving package");
        nativeMenuPaint_Text(detail, x + 10, y + (art.pixels ? 63 : 22), 150, 8,
                             0xEBDDAEFF, NativeMenuFont_Notice, true);
        nativeMenuPaint_Text(*art.title ? art.title : "Custom package", x + 10,
                             y + (art.pixels ? 76 : 49), 150, 12,
                             0xFFF4E9FF, NativeMenuFont_MenuOption, true);
        if (!download.bytes_total) {
            nativeMenuPaint_Text("Waiting for transfer...", x + 10, y + 101, 150, 8,
                                 0xEBDDAEFF, NativeMenuFont_Notice, true);
            return;
        }
        const float ratio = std::min(1.f, float(download.bytes_done) / download.bytes_total);
        constexpr float mib = 1024.f * 1024.f;
        std::snprintf(detail, sizeof(detail), "%.2f / %.2f MiB  (%u%%)",
                      download.bytes_done / mib, download.bytes_total / mib,
                      unsigned(ratio * 100));
        nativeMenuPaint_Rect(x + 11, y + 92, 148, 4, 0x3C3448FF);
        nativeMenuPaint_Rect(x + 11, y + 92, 148 * ratio, 4, 0xE5C361FF);
        nativeMenuPaint_Text(detail, x + 10, y + 101, 150, 8,
                             0xEBDDAEFF, NativeMenuFont_Notice, true);
    }

    bool hit(const Button& b, float x, float y)
    {
        return x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h;
    }
    void add(const char* id, const char* label, float x, float y, float w,
             float h, Action action, bool enabled = true, bool gold = false,
             bool authored = false, int value = 0)
    {
        if (menu.count == sizeof(menu.buttons) / sizeof(*menu.buttons)) return;
        Button& b = menu.buttons[menu.count++];
        b = {};
        std::snprintf(b.id, sizeof(b.id), "%s", id);
        std::snprintf(b.label, sizeof(b.label), "%s", label);
        b.x = x; b.y = y; b.w = w; b.h = h; b.action = action;
        b.enabled = enabled; b.gold = gold; b.authored = authored; b.value = value;
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
        case Action::ChatInput: nativeCssChat_Focus(true); break;
        case Action::ChatSend: nativeCssChat_Send(); break;
        case Action::ChatHistory: nativeHub_Open(NativeHubPage_RoomChat); break;
        case Action::CopyCode:
            SDL_SetClipboardText(nativeOnlineSession_RoomCode());
            break;
        case Action::Settings: nativeHubRules_Open(); break;
        case Action::Opponent: nativeHub_Open(NativeHubPage_PracticeOpponent); break;
        case Action::Rotation:
            if (!nativeOnlineSession_SetRotation(NativeRoomRotationPolicy(b.value)))
                nativeHub_Notice("Could not change the rotation.");
            break;
        case Action::Stage:
            nativeHub_Open(NativeHubPage_QuickStage);
            break;
        case Action::Ready:
            menu.ready_requested = true;
            break;
        case Action::Watch:
            if (nativeSpectatorMatch_State() == NativeSpectator_Requested)
                nativeSpectatorMatch_Stop();
            else if (!nativeSpectatorMatch_Request())
                nativeHub_Notice("Could not watch this match.");
            break;
        case Action::Queue: {
            const auto* quick = nativeOnlineSession_Quick();
            if (quick && !nativeOnlineSession_SearchQuick(quick->state == NativeQuickState_Idle))
                nativeHub_Notice("Could not change the Quick Play search.");
            break;
        }
        case Action::Accept:
            if (!nativeOnlineSession_RequestGiftAcceptance()) {
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
                if (menu.buttons[i].action != Action::ChatInput &&
                    menu.buttons[i].action != Action::ChatSend) nativeCssChat_Focus(false);
                activate(menu.buttons[i]);
                return true;
            }
        }
        nativeCssChat_Focus(false);
        return false;
    }
    void draw()
    {
        nativePlayMenu_DrawCountdown();
        if (menu.practice && (nativeOnlineSession_Presence() || nativeOnlineSession_Quick())) {
            nativeMenuSkin_Panel(7, 5, 626, 32, 0xACA7B5FF, 0x09080EDC);
            nativeMenuPaint_Text(menu.status,18,16,604,11,0xEBDDAEFF);
            nativeCssChat_DrawPractice();
        } else if (menu.show_status) {
            nativeMenuPaint_Text(menu.status, 451, 263, 177, 8, 0xEBDDAEFF);
        }
        if (menu.selection) nativeCssChat_Draw();
        draw_download();
        if (*menu.delivery) {
            float y = menu.practice ? 44 : 99;
            nativeMenuSkin_Panel(364, y, 262, 78, 0xD8B967FF, 0x09080EF5);
            nativeMenuPaint_Text(menu.delivery, 375, y + 10, 240, 9,
                                 0xF3E6CFFF);
            nativeMenuPaint_Text(menu.gift.title, 375, y + 28, 240, 11,
                                 0xFFF4E9FF, NativeMenuFont_MenuOption);
        }
        for (unsigned i = 0; i < menu.count; ++i) {
            const Button& b = menu.buttons[i];
            if (b.authored) continue;
            bool hover = b.enabled && hit(b, menu.pointer_x, menu.pointer_y);
            nativeMenuSkin_Button(b.label, b.x, b.y, b.w, b.h, 10,
                                   b.gold || hover, b.enabled);
        }
    }
}

extern "C" void nativePlayMenu_Tick(bool selection, bool practice)
{
    if (practice && !nativeHubHand_Ready()) nativeHubHand_Load(nullptr);
    if ((selection || practice) && !menu.fonts) {
        nativeMenuFont_Acquire();
        nativeMenuSkin_Acquire();
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
    menu.show_status = false;
    nativeCssChat_SetVisible(selection && nativeOnlineSession_Presence());
    nativeCssChat_Tick(practice);
    if (!(selection || practice)) {
        if (download.started) clear_download();
        return;
    }
    const auto* quick = nativeOnlineSession_Quick();
    bool searching = quick && quick->state == NativeQuickState_Searching;
    bool assigned = quick && quick->state == NativeQuickState_Assigned;
    bool stage_choice = assigned && quick->phase == 3;
    const char* queue_label = assigned || (quick && quick->state == NativeQuickState_Aborted)
        ? "Leave match" : searching ? "Cancel search" : "Enter queue";
    bool can_queue = quick && (assigned ||
        (nativeOnlineSession_State() == NativeLobbySessionState_Waiting && nativeOnlineSession_IsOwner()));
    const auto* presence = nativeOnlineSession_Presence();
    bool can_watch = !quick && presence && presence->match_size &&
        !nativeOnlineSession_LocalActiveSeats();
    bool requesting_watch = nativeSpectatorMatch_State() == NativeSpectator_Requested;
    if (selection) {
        const bool compact = quick || presence;
        add("play-ready", "Ready / START", compact ? 451 : 14, compact ? 421 : 451,
            compact ? 177 : 126, 24, Action::Ready,
            nativeCssInteraction_CanReady() && !stage_choice, true);
        add("practice-opponent", "Practice opponent", compact ? 14 : 147,
            451, 129, 24, Action::Opponent);
        if (stage_choice) add("play-stage", "Next stage", 451, 451, 177, 24, Action::Stage, true, true);
        else if (quick) add("play-queue", queue_label, 451, 451, 177, 24, Action::Queue, can_queue, !searching);
        else if (compact && (can_watch || requesting_watch))
            add("play-watch", requesting_watch ? "Cancel viewing" : "Watch match",
                451, 451, 177, 24, Action::Watch, true, true);
        else if (presence)
            add("css-chat-code", "Copy connect code", 451, 451, 177, 24, Action::CopyCode);
        if (presence) {
            if (!quick && !presence->match_size) {
                const char* rotation[] = {"Manual", "Winner stays", "Winner out"};
                char label[40];
                const unsigned policy = presence->rotation_policy % 3;
                std::snprintf(label, sizeof(label), "Rotation: %s", rotation[policy]);
                add("css-rotation", label, 451, 246, 177, 20, Action::Rotation,
                    nativeOnlineSession_IsOwner(), false, false, int((policy + 1) % 3));
            }
            add("css-chat-history", "Lobby chat", 455, 278, 98, 17, Action::ChatHistory, true, false, true);
            add("css-chat-input", "Message", 455, 385, 129, 22, Action::ChatInput, true, false, true);
            const auto* chat = nativeOnlineSession_Chat();
            add("css-chat-send", "Send", 589, 385, 35, 22, Action::ChatSend, chat && chat->ready);
        }
        // These are the stock cursor hit regions from mnCharSel_CursorThink.
        // The original scene draws their artwork and hover animations.
        float left, right, top, bottom;
        nativeCssOriginal_Viewport(&left, &right, &top, &bottom);
        const float header_height = (top - 22) / (top - bottom) * 480;
        add("play-lobby", "Back", (17.3f-left)/(right-left)*640, 0,
            (right-17.3f)/(right-left)*640, header_height, Action::Lobby, true, false, true);
        add("play-rules", "Match rules", (-17-left)/(right-left)*640, 0,
            32/(right-left)*640, header_height, Action::Settings, !quick, false, true);
    }
    NativeLobbySessionState state = nativeOnlineSession_State();
    const char* status = selection ? "Choose a fighter. START to ready." : "";
    if (state == NativeLobbySessionState_Connecting) {
        status = "Connecting to lobby...";
    } else if (state == NativeLobbySessionState_Failed) {
        status = "Connection failed. Open Lobbies to reconnect.";
    } else if (state == NativeLobbySessionState_Waiting && practice) {
        const auto* lobby=nativeOnlineSession_LobbyState();
        status=lobby && lobby->phase==1 ? "Everyone ready" : "Waiting for ready players";
    }
    if (quick && state == NativeLobbySessionState_Waiting) {
        status = quick->phase == 3 ? (quick->chooser == quick->team ? "Your side picks the next stage" : "Waiting for the losing side's stage")
                 : assigned ? "Match found. Connecting players..."
                 : searching ? "Searching for players..."
                 : quick->state == NativeQuickState_Aborted ? "Match cancelled. Leave to search again."
                 : "Enter queue, then START to ready and practice.";
        if (*nativeOnlineSession_QuickError()) status = nativeOnlineSession_QuickError();
    }
    if (practice && (quick || presence) && !nativeHome_IsReady()) status="Not ready | START to ready | Exit: L+R+A+START";
    if (quick && state == NativeLobbySessionState_Returning) status = "Agreeing the match result...";
    if (requesting_watch) status = nativeOnlineSession_WatchContentReady()
        ? "Preparing the match..." : "Receiving the match and fighters...";
    const char* code = nativeOnlineSession_RoomCode();
    menu.show_status = state == NativeLobbySessionState_Failed ||
        (quick && *nativeOnlineSession_QuickError());
    if (code && *code && practice && !quick) {
        std::snprintf(menu.status, sizeof(menu.status), "%s  |  %s", code,
                      status);
    } else {
        std::snprintf(menu.status, sizeof(menu.status), "%s", status);
    }
    NativeNetplayGiftStatus outgoing;
    if (nativeOnlineSession_GiftStatus(false, &outgoing)) {
        menu.show_status |= outgoing.state == NativeNetplayGift_Offered ||
            outgoing.state == NativeNetplayGift_Sending || outgoing.state == NativeNetplayGift_AwaitingReceipt;
        if (outgoing.state == NativeNetplayGift_Offered) {
            std::snprintf(menu.status, sizeof(menu.status),
                          "%s offered. Waiting for acceptance.",
                          outgoing.title);
        } else if (outgoing.state == NativeNetplayGift_Sending) {
            std::snprintf(menu.status, sizeof(menu.status), "Sending %s...",
                          outgoing.title);
        } else if (outgoing.state == NativeNetplayGift_AwaitingReceipt) {
            std::snprintf(menu.status, sizeof(menu.status), "Waiting for %s to install...",
                          outgoing.title);
        }
    }
    if (*nativeHub_NoticeText()) {
        menu.show_status = true;
        std::snprintf(menu.status, sizeof(menu.status), "%s",
                      nativeHub_NoticeText());
    }
    NativeNetplayContentProgress progress{};
    const bool transferring = nativeOnlineSession_ContentProgress(&progress);
    bool receiving = transferring && progress.direction == NativeNetplayContent_Receiving;
    if (transferring && progress.direction == NativeNetplayContent_Sending) {
        menu.show_status = true;
        std::snprintf(menu.status, sizeof(menu.status), "Sending package %u / %u",
                      progress.package_index, progress.package_count);
    }
    if (nativeOnlineSession_GiftStatus(true, &menu.gift))
    {
        float y = practice ? 44 : 99;
        const NativeContentId empty{};
        bool includes_fighter = std::memcmp(&menu.gift.dependency_id, &empty, sizeof(empty)) != 0;
        if (menu.gift.state == NativeNetplayGift_Offered) {
            std::snprintf(menu.delivery, sizeof(menu.delivery), "%s",
                menu.gift.kind == NativeNetplayGift_Costume
                    ? (includes_fighter ? "Outfit + fighter delivery" : "Outfit delivery")
                    : "Fighter delivery");
            add("play-accept", menu.gift.kind == NativeNetplayGift_Costume ? "Accept outfit" : "Accept fighter", 375, y + 49, 139, 21,
                Action::Accept, true, true);
            add("play-decline", "No thanks", 522, y + 49, 94, 21,
                Action::Decline);
        } else if (menu.gift.state == NativeNetplayGift_Receiving) {
            const bool parent = includes_fighter &&
                !std::memcmp(&menu.gift.downloading, &menu.gift.dependency_id, sizeof(empty));
            progress = {
                .id = menu.gift.downloading,
                .preview = menu.gift.preview,
                .bytes_done = menu.gift.bytes_done,
                .bytes_total = menu.gift.bytes_total,
                .package_index = uint8_t(parent || !includes_fighter ? 1 : 2),
                .package_count = uint8_t(includes_fighter ? 2 : 1),
                .direction = NativeNetplayContent_Receiving,
            };
            if (!*progress.preview.title && !parent) {
                std::memcpy(progress.preview.title, menu.gift.title, sizeof(progress.preview.title));
            }
            receiving = true;
        }
    }
    if (receiving) update_download(progress);
    else if (download.started) clear_download();
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
    if (!nativeOnlineSession_Quick()) remaining=nativeOnlineSession_LobbyCountdownMs();
    if (!remaining) return;
    char label[64];
    std::snprintf(label, sizeof(label), "Match starts in %u", (remaining + 999) / 1000);
    nativeMenuSkin_Panel(195, 112, 250, 74, 0xD6BC79FF, 0x09080EF5);
    nativeMenuPaint_Text(label, 210, 128, 220, 17, 0xFFF4E9FF, NativeMenuFont_MenuOption, true);
    nativeMenuPaint_Text("START to unready", 210, 158, 220, 10, 0xEBDDAEFF, NativeMenuFont_Notice, true);
}

extern "C" void nativePlayMenu_DrawPractice(void)
{
    if (!menu.practice) return;
    nativeMenuPaint_Begin(nullptr);
    if (gmTraining_LocalMenuOpen()) {
        nativeMenuSkin_Button("Z  Practice opponent", 11, 439, 154, 24, 10, false, true);
        nativeMenuPaint_End();
        return;
    }
    draw();
    if (nativePlayMenu_PointerActive()) {
        nativeHubHand_Draw(menu.pointer_x, menu.pointer_y, false, nullptr);
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
    return (trigger & PAD_BUTTON_A) && click(menu.pointer_x,menu.pointer_y);
}
extern "C" bool nativePlayMenu_Pointer(float u, float v,
                                       NativeCssPointerEvent event)
{
    if (!menu.practice) return false;
    if (gmTraining_LocalMenuOpen()) {
        const bool hovered = u * 640 >= 11 && u * 640 < 165 &&
                             v * 480 >= 439 && v * 480 < 463;
        if (hovered && event == NativeCssPointer_Press)
            nativeHub_Open(NativeHubPage_PracticeOpponent);
        return hovered;
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
    if (!std::strcmp(id,"practice-state")) {
        std::fprintf(stderr,"[practice-state] practice=%u ready=%u paused=%d quick-port=%d buttons=%u hud=%u training=%u row=%u status=%s\n",menu.practice,nativeHome_IsReady(),gm_GetDbPauseFlag(1),nativeHome_QuickSinglesPort(),menu.count,gmVs_GetController_0()->hud_enabled,gmTraining_LocalMenuOpen(),gm_80473814.x00,menu.status);
        if (nativeHome_IsPractice()) nativePractice_InspectOpponent();
        nativeCssChat_Inspect(); return true;
    }

    if (!std::strcmp(id,"quick-play")) {
        const auto* quick = nativeOnlineSession_Quick();
        if (quick) std::fprintf(stderr, "[quick-play] mode=%u state=%u generation=%llu team=%u stage=%u room=%s peer=%s phase=%u countdown=%u completed=%llu chooser=%u votes=%u,%u,%u,%u\n",
            quick->mode, quick->state, (unsigned long long) quick->generation,
            quick->team, quick->stage, nativeOnlineSession_RoomCode(),
            nativeOnlineSession_LocalPeerId(), quick->phase, nativeOnlineSession_QuickCountdownMs(),
            (unsigned long long) quick->completed_epoch, quick->chooser,
            quick->votes[0],quick->votes[1],quick->votes[2],quick->votes[3]);
        else std::fprintf(stderr,"[quick-play] unavailable\n");
        return true;
    }
    if (!std::strcmp(id, "css-chat")) { nativeCssChat_Inspect(); return true; }
    if (!std::strcmp(id, "lobby-listing")) {
        const auto* listing = nativeOnlineSession_Listing();
        std::fprintf(stderr, "[play-menu] lobby-listing available=%d listed=%d owner=%d\n",
            listing != nullptr, listing && listing->listed, nativeOnlineSession_IsOwner());
        return true;
    }
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
                     "enabled=%d value=0 hand=%d\n",
                     id, (size.x - 640 * scale) * .5f + u * 640 * scale,
                     (size.y - 480 * scale) * .5f + v * 480 * scale,
                     (r - u) * 640 * scale, (t - v) * 480 * scale, b.enabled, nativeHubHand_Ready());
        if (const auto* quick = nativeOnlineSession_Quick()) {
            std::fprintf(stderr, "[quick-play] mode=%u state=%u generation=%llu team=%u stage=%u room=%s peer=%s phase=%u countdown=%u completed=%llu chooser=%u votes=%u,%u,%u,%u\n",
                quick->mode, quick->state, (unsigned long long) quick->generation,
                quick->team, quick->stage, nativeOnlineSession_RoomCode(),
                nativeOnlineSession_LocalPeerId(), quick->phase, nativeOnlineSession_QuickCountdownMs(),
                (unsigned long long) quick->completed_epoch, quick->chooser,
                quick->votes[0], quick->votes[1], quick->votes[2], quick->votes[3]);
        }
        return true;
    }
    return false;
}
extern "C" void nativePlayMenu_Shutdown(void)
{
    if (menu.fonts) {
        nativeMenuFont_Release();
        nativeMenuSkin_Release();
    }
    nativeCssChat_SetVisible(false);
    menu = PlayMenu{};
}
