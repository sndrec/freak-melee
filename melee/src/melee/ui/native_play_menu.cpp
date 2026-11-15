#include "native_play_menu.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "native_hub.h"
#include "native_hub_hand.h"
#include "native_css_chat.h"
#include "native_shell_rules.h"
#include <aurora/rmlui.hpp>
#include <melee/gm/native_home.h>
#include <melee/gm/native_online_session.h>
#include <melee/gm/native_spectator_match.h>
#include <melee/mn/native_css_interaction.h>
#include <melee/mn/native_css_original.h>
#include <melee/mn/native_menu_paint.h>
#include <melee/mn/native_menu_skin.h>
#include <RmlUi/Core.h>
#include <SDL3/SDL_clipboard.h>
extern "C" {
#include <melee/lb/lbaudio_ax.h>
#include <sysdolphin/baselib/controller.h>
}

namespace {
    enum class Action {
        Lobby,
        Ready,
        Start,
        Watch,
        Selection,
        Accept,
        Decline,
        Queue,
        Stage,
        Settings, ChatInput, ChatSend, ChatHistory, CopyCode, Listing, Rotation, MatchRules, Seat, PeoplePage
    };
    struct Button {
        char id[40], label[96], peer[65];
        int value;
        float x, y, w, h;
        Action action;
        bool enabled, gold, authored;
    };
    struct PlayMenu {
        Button buttons[40];
        unsigned count, people_offset;
        char status[192], delivery[128];
        NativeNetplayGiftStatus gift;
        float pointer_x = -100, pointer_y = -100;
        bool selection, practice, fonts, ready_requested, show_status, rules;
    } menu;

    bool hit(const Button& b, float x, float y)
    {
        return x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h;
    }
    void add(const char* id, const char* label, float x, float y, float w,
             float h, Action action, bool enabled = true, bool gold = false,
             bool authored = false, int value = 0, const char* peer = nullptr)
    {
        if (menu.count == sizeof(menu.buttons) / sizeof(*menu.buttons)) return;
        Button& b = menu.buttons[menu.count++];
        b = {};
        std::snprintf(b.id, sizeof(b.id), "%s", id);
        std::snprintf(b.label, sizeof(b.label), "%s", label);
        if (peer) std::snprintf(b.peer, sizeof(b.peer), "%s", peer);
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
            if (menu.rules) menu.rules = false;
            else nativeHub_Open(NativeHubPage_Lobbies);
            break;
        case Action::ChatInput: nativeCssChat_Focus(true); break;
        case Action::ChatSend: nativeCssChat_Send(); break;
        case Action::ChatHistory: nativeHub_Open(NativeHubPage_RoomChat); break;
        case Action::CopyCode:
            SDL_SetClipboardText(nativeOnlineSession_RoomCode());
            break;
        case Action::Settings:
            if (nativeOnlineSession_Presence() && !nativeOnlineSession_Quick())
                menu.rules = !menu.rules;
            else if (!nativeOnlineSession_Quick()) nativeShellRules_Open();
            break;
        case Action::Listing: {
            const auto* listing = nativeOnlineSession_Listing();
            char title[65]; std::snprintf(title, sizeof(title), "%.32s's lobby", nativeHub_LocalName());
            if (!listing || !nativeOnlineSession_SetListing(listing->title[0] ? listing->title : title, !listing->listed))
                nativeHub_Notice("Could not change the lobby listing.");
            break;
        }
        case Action::Rotation: {
            const auto* p = nativeOnlineSession_Presence();
            if (p) nativeOnlineSession_SetRotation(NativeRoomRotationPolicy((p->rotation_policy + 1) % 3));
            break;
        }
        case Action::MatchRules: nativeShellRules_Open(); break;
        case Action::Seat:
            if (!nativeOnlineSession_ChooseSeat(b.peer, uint8_t(b.value & 3), NativeRoomSeatState(b.value >> 2)))
                nativeHub_Notice("Could not change this player's seat.");
            else nativeHome_CancelStartMatch();
            break;
        case Action::PeoplePage:
            menu.people_offset = unsigned(std::max(0, int(menu.people_offset) + b.value));
            break;
        case Action::Stage:
            nativeHub_Open(NativeHubPage_QuickStage);
            break;
        case Action::Ready:
            menu.ready_requested = true;
            break;
        case Action::Start:
            nativeHome_RequestStartMatch();
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
        case Action::Selection:
            nativeHome_ReturnToSelection();
            break;
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
        if (menu.rules) {
            nativeMenuPaint_Rect(0, 85, 640, 395, 0x00000080);
            nativeMenuSkin_Panel(145, 122, 482, 310, 0xC2BEC7FF, 0x09080EF0);
            nativeMenuPaint_Text("Lobby rules", 160, 138, 290, 21, 0xF5EFEAFF, NativeMenuFont_Heading);
            const auto* p = nativeOnlineSession_Presence();
            for (unsigned i = menu.people_offset; p && i < p->participant_count && i < menu.people_offset + 6; ++i)
                nativeMenuPaint_Text(p->participants[i].name, 160, 194 + (i - menu.people_offset) * 25, 190, 11);
            nativeMenuPaint_Text("Rotation", 160, 396, 75, 10, 0xE5C361FF);
            if (menu.show_status) nativeMenuPaint_Text(menu.status, 160, 366, 450, 9, 0xEBDDAEFF);
        } else if (menu.practice) {
            nativeMenuSkin_Panel(7, 5, 626, 32, 0xACA7B5FF, 0x09080EDC);
            nativeMenuPaint_Text(menu.status, 220, 17, 275, 9, 0xEBDDAEFF);
        } else if (menu.show_status) {
            nativeMenuPaint_Text(menu.status, 451, 263, 177, 8, 0xEBDDAEFF);
        }
        if (menu.selection && !menu.rules) nativeCssChat_Draw();
        if (*menu.delivery && !menu.rules) {
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
    if (!selection || !nativeOnlineSession_Presence() || nativeOnlineSession_Quick()) menu.rules = false;
    menu.count = 0;
    menu.delivery[0] = 0;
    menu.show_status = false;
    nativeCssChat_SetVisible(selection && !menu.rules && nativeOnlineSession_Presence());
    if (!(selection || practice)) {
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
        if (menu.rules) {
            const auto* listing = nativeOnlineSession_Listing();
            const bool owner = nativeOnlineSession_IsOwner();
            const char* local = nativeOnlineSession_LocalPeerId();
            menu.people_offset = std::min(menu.people_offset,
                presence && presence->participant_count ? unsigned((presence->participant_count - 1) / 6 * 6) : 0);
            add("change-listing", listing && listing->listed ? "Listed" : "Unlisted", 480, 137, 130, 26,
                Action::Listing, owner);
            for (unsigned i = menu.people_offset; presence && i < presence->participant_count && i < menu.people_offset + 6; ++i) {
                const auto& person = presence->participants[i];
                for (unsigned seat = 0; seat < person.local_players; ++seat) {
                    const auto state = person.active_mask & (1U << seat) ? NativeRoomSeatState_Fighting
                        : person.waiting_mask & (1U << seat) ? NativeRoomSeatState_Waiting : NativeRoomSeatState_Spectating;
                    const auto next = state == NativeRoomSeatState_Fighting ? NativeRoomSeatState_Waiting
                        : state == NativeRoomSeatState_Waiting ? NativeRoomSeatState_Spectating : NativeRoomSeatState_Fighting;
                    char id[40]; std::snprintf(id, sizeof(id), "lobby-seat-%u-%u", i, seat);
                    add(id, state == NativeRoomSeatState_Fighting ? "Fighting" : state == NativeRoomSeatState_Waiting ? "Waiting" : "Spectating",
                        365 + seat * 60, 188 + (i - menu.people_offset) * 25, 57, 23,
                        Action::Seat, owner || (local && !std::strcmp(person.peer_id, local)), false, false,
                        int(next) * 4 + int(seat), person.peer_id);
                }
            }
            if (presence && presence->participant_count > 6) {
                add("lobby-prev", "<", 550, 343, 25, 22, Action::PeoplePage, menu.people_offset > 0, false, false, -6);
                add("lobby-next", ">", 585, 343, 25, 22, Action::PeoplePage,
                    menu.people_offset + 6 < presence->participant_count, false, false, 6);
            }
            const char* policies[] = {"Manual", "Winner stays", "Winner out"};
            add("rotation-policy", presence ? policies[presence->rotation_policy % 3] : "Manual",
                240, 387, 135, 27, Action::Rotation, owner);
            add("match-rules", "Match rules", 390, 387, 220, 27, Action::MatchRules, owner);
        } else {
        const bool compact = quick || presence;
        add("play-ready", "Ready / START", compact ? 451 : 14, compact ? 421 : 451,
            compact ? 177 : 126, 24, Action::Ready,
            nativeCssInteraction_CanReady() && !stage_choice, true);
        if (stage_choice) add("play-stage", "Next stage", 451, 451, 177, 24, Action::Stage, true, true);
        else if (quick) add("play-queue", queue_label, 451, 451, 177, 24, Action::Queue, can_queue, !searching);
        else if (compact && (can_watch || requesting_watch))
            add("play-watch", requesting_watch ? "Cancel viewing" : "Watch match",
                451, 451, 177, 24, Action::Watch, true, true);
        else if (compact) add("play-start", "Start match", 451, 451, 177, 24, Action::Start,
                              nativeHome_CanStartMatch(), true);
        if (presence) {
            add("css-chat-history", "Lobby chat", 455, 278, 98, 17, Action::ChatHistory, true, false, true);
            add("css-chat-code", "Copy code", 565, 278, 59, 17, Action::CopyCode);
            add("css-chat-input", "Message", 455, 385, 129, 22, Action::ChatInput, true, false, true);
            const auto* chat = nativeOnlineSession_Chat();
            add("css-chat-send", "Send", 589, 385, 35, 22, Action::ChatSend, chat && chat->ready);
        }
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
    } else {
        add("play-lobby", "Lobby", 13, 10, 88, 22, Action::Lobby);
        if (quick) add("play-queue", queue_label, 109, 10, 104, 22, Action::Queue, can_queue, !searching);
        else if (can_watch || requesting_watch)
            add("play-watch", requesting_watch ? "Cancel viewing" : "Watch match",
                109, 10, 104, 22, Action::Watch, true, true);
        else add("play-start", "Start match", 109, 10, 104, 22, Action::Start,
                 nativeHome_CanStartMatch(), true);
        add("play-selection", "Character select", 507, 10, 119, 22,
            Action::Selection);
        if (stage_choice) add("play-stage", "Next stage", 302, 10, 120, 22, Action::Stage, true, true);
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
        status = quick->phase == 3 ? (quick->chooser == quick->team ? "Your side picks the next stage" : "Waiting for the losing side's stage")
                 : assigned ? "Match found. Connecting players..."
                 : searching ? "Searching for players..."
                 : quick->state == NativeQuickState_Aborted ? "Match cancelled. Leave to search again."
                 : "START to practice. Enter queue when ready to search.";
        if (*nativeOnlineSession_QuickError()) status = nativeOnlineSession_QuickError();
    }
    if (quick && state == NativeLobbySessionState_Returning) status = "Agreeing the match result...";
    if (requesting_watch) status = nativeOnlineSession_WatchContentReady()
        ? "Preparing the match..." : "Receiving the match and fighters...";
    const char* code = nativeOnlineSession_RoomCode();
    menu.show_status = state == NativeLobbySessionState_Failed ||
        (quick && *nativeOnlineSession_QuickError());
    if (code && *code && practice) {
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
    NativeNetplayContentProgress progress;
    if (nativeOnlineSession_ContentProgress(&progress)) {
        menu.show_status = true;
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
        const NativeContentId empty{};
        bool includes_fighter = std::memcmp(&menu.gift.dependency_id, &empty, sizeof(empty)) != 0;
        std::snprintf(menu.delivery, sizeof(menu.delivery), "%s",
                      menu.gift.state == NativeNetplayGift_Offered
                          ? (menu.gift.kind == NativeNetplayGift_Costume
                             ? (includes_fighter ? "Outfit + fighter delivery" : "Outfit delivery")
                             : "Fighter delivery")
                          : "Receiving delivery...");
        if (menu.gift.state == NativeNetplayGift_Offered) {
            add("play-accept", menu.gift.kind == NativeNetplayGift_Costume ? "Accept outfit" : "Accept fighter", 375, y + 49, 139, 21,
                Action::Accept, true, true);
            add("play-decline", "No thanks", 522, y + 49, 94, 21,
                Action::Decline);
        } else if (menu.gift.bytes_total) {
            std::snprintf(menu.delivery, sizeof(menu.delivery),
                          "Receiving delivery... %u%%",
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
    nativeMenuSkin_Panel(195, 112, 250, 74, 0xD6BC79FF, 0x09080EF5);
    nativeMenuPaint_Text(label, 210, 128, 220, 17, 0xFFF4E9FF, NativeMenuFont_MenuOption, true);
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
    if (menu.rules && (trigger & PAD_BUTTON_B)) { menu.rules = false; return true; }
    return (trigger & PAD_BUTTON_A) ? (click(menu.pointer_x, menu.pointer_y) || menu.rules) : menu.rules;
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
extern "C" bool nativePlayMenu_RulesOpen(void)
{
    return menu.selection && menu.rules;
}
extern "C" bool nativePlayMenu_TakeReadyRequest(void)
{
    bool value = menu.selection && !menu.rules && menu.ready_requested;
    menu.ready_requested = false;
    return value;
}
extern "C" bool nativePlayMenu_Inspect(const char* id)
{
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
        std::fprintf(stderr, "[play-menu] lobby-listing available=%d listed=%d owner=%d rules=%d\n",
            listing != nullptr, listing && listing->listed, nativeOnlineSession_IsOwner(), menu.rules);
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
