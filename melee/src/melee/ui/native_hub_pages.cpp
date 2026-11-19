#include "native_hub_rules.h"
#include "native_hub_costumes.h"
#include "native_hub_packages.h"
#include "native_hub_internal.h"
#include "native_hub_news.h"
#include <melee/gm/native_presentation.h>
#include "native_hub_quick_stage.h"
#include "native_hub_lobbies.h"
#include "native_hub_profiles.h"
#include "native_hub_settings.h"
#include "native_hub_account.h"
#include "native_hub_social.h"
#include "native_hub_startup.h"
#include "native_hub_startup_roster.h"
#include "native_hub_portrait.h"
#include <melee/mn/native_menu_tile.h>
#include <melee/mn/native_menu_skin.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/gm/native_online_session.h>
#include <melee/gm/native_home.h>
#include <SDL3/SDL_timer.h>
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace {
    constexpr uint32_t gold = 0xE5C361FF, muted = 0xA6A1AFFF;
    void fighter_tile(unsigned index, float x, float y)
    {
        const NativeHubFighter& f = nativeHubFighters[index];
        bool selected = int(index) == hub.selected,
             hover = nativeHub_Hover(x, y, 49, 49);
        nativeMenuPaint_Round(x - 1, y - 1, 51, 52, 3,
                             selected || hover ? gold : 0xBAB6BFFF);
        nativeMenuTile_Draw(f.pixels, f.width, f.height, f.tile_name, x, y, 49, 49*56.f/64);
        nativeHub_Region("fighter", x, y, 49, 49, HubAction::Fighter,
                         int(index));
    }
    void collection()
    {
        nativeHub_Button("clone-fighter", "Clone Vanilla Fighter", 175, 110, 175, 26,
                         HubAction::NewClone, 0, true, nativeHome_IsSelection());
        nativeHub_Panel(175, 148, 430, 210);
        unsigned end =
            std::min(nativeHubFighterCount, unsigned(hub.first_fighter + 14));
        for (unsigned i = hub.first_fighter; i < end; ++i) {
            fighter_tile(i, 189 + ((i - hub.first_fighter) % 7) * 57,
                         162 + ((i - hub.first_fighter) / 7) * 58);
        }
        if (!nativeHubFighterCount) {
            nativeMenuPaint_Text("Start your roster", 200, 179, 380, 20, gold,
                                NativeMenuFont_MenuOption);
            nativeMenuPaint_Text(
                "Choose a vanilla fighter to make your first editable copy.",
                200, 212, 380, 9);
            nativeHub_Button("first-clone", "Choose a fighter", 200, 240, 140,
                             30, HubAction::NewClone, 0, true);
        } else if (hub.selected >= 0 &&
                   unsigned(hub.selected) < nativeHubFighterCount)
        {
            const NativeHubFighter& f = nativeHubFighters[hub.selected];
            nativeMenuPaint_Text(f.title, 194, 290, 260, 17, 0xF5EFEAFF,
                                NativeMenuFont_MenuOption);
            if (!f.installed)
                nativeMenuPaint_Text("Draft", 194, 313, 240, 8, muted);
            nativeHub_Button("edit-fighter", "Edit fighter", 465, 285, 122, 36,
                             HubAction::Edit, 0, true, nativeHome_IsSelection());
            nativeHub_Button("copy-fighter", "Duplicate", 194, 332, 62, 21,
                             HubAction::CopyFighter, 0, false, f.installed && nativeHome_IsSelection());
            nativeHub_Button("rename-fighter", "Rename", 262, 332, 56, 21,
                             HubAction::RenameFighter, 0, false, nativeHome_IsSelection());
            nativeHub_Button("send-fighter", "Send fighter", 320, 332, 81, 21,
                             HubAction::SendFighter, 0, false, f.installed);
            nativeHub_Button("portrait-fighter","Portrait & tile",410,332,110,23,
                             HubAction::EditPortrait,0,false,nativeHome_IsSelection() && f.draft[0]);
            nativeHub_Button("delete-fighter", "Delete", 526, 332, 61, 23,
                             HubAction::DeleteFighter, 0, false, nativeHome_IsSelection());
        }
        if (nativeHubFighterCount > 14) {
            nativeHub_Button("roster-prev", "<", 539, 115, 27, 21,
                             HubAction::Previous, 0, false,
                             hub.first_fighter > 0);
            nativeHub_Button("roster-next", ">", 572, 115, 27, 21,
                             HubAction::Next, 0, false,
                             end < nativeHubFighterCount);
        }
        nativeHub_Button("edit-layout", "Edit CSS layout", 175, 372, 100, 25,
                         HubAction::EditLayout, 0, false, nativeHome_IsSelection());
        nativeHub_Button("local-practice", "Local practice", 282, 372, 100, 25,
                         HubAction::Practice);
        nativeHub_Button("defaults-open", "Add Melee roster", 457, 372, 148, 25,
                         HubAction::DefaultsOpen, 0, false, nativeHome_IsSelection());
        if (nativeHome_IsPractice()) nativeMenuPaint_Text(
            "Return to character select to edit fighters.", 179, 385, 420, 9, muted);
        NativeNetplayGiftStatus gift{};
        if (nativeOnlineSession_GiftStatus(true, &gift) &&
            gift.state == NativeNetplayGift_Offered)
        {
            nativeHub_Panel(185, 225, 410, 58);
            nativeMenuPaint_Text("Fighter delivery", 197, 233, 200, 10, gold);
            nativeMenuPaint_Text(gift.title, 197, 253, 200, 11);
            nativeHub_Button("accept-fighter", "Accept fighter", 414, 235, 103,
                             27, HubAction::AcceptGift, 0, true);
            nativeHub_Button("decline-fighter", "Decline", 523, 235, 60, 27,
                             HubAction::DeclineGift);
        }
        if (hub.delete_confirmation && hub.selected >= 0 &&
            unsigned(hub.selected) < nativeHubFighterCount) {
            hub.region_count = 0;
            nativeMenuPaint_Rect(0, 0, 640, 480, 0x00000090);
            nativeHub_Panel(190, 181, 400, 123);
            nativeMenuPaint_Text("Delete fighter?", 207, 195, 366, 18, gold,
                                 NativeMenuFont_MenuOption);
            nativeMenuPaint_Text(nativeHubFighters[hub.selected].title,
                                 207, 222, 366, 12, 0xF5EFEAFF);
            nativeMenuPaint_Text("Remove its project and Character Select tiles.",
                                 207, 243, 366, 9, muted);
            nativeHub_Button("delete-cancel", "Cancel", 207, 266, 120, 24,
                             HubAction::Cancel, 0, true);
            nativeHub_Button("delete-confirm", "Delete fighter", 397, 266, 175, 24,
                             HubAction::ConfirmDeleteFighter);
        } else if (hub.rename) {
            nativeHub_Panel(194, 196, 390, 91);
            nativeMenuPaint_Text("Fighter name", 208, 207, 200, 10, gold);
            nativeHub_Field("fighter-title", hub.title, 208, 227, 360, 23,
                            HubAction::FieldTitle);
            nativeHub_Button("apply-name", "Save name", 389, 257, 91, 22,
                             HubAction::ApplyRename, 0, true);
            nativeHub_Button("cancel-name", "Cancel", 487, 257, 81, 22,
                             HubAction::Cancel);
        }
    }
    void clone_page()
    {
        const float offset = nativeHubStartup.active ? -70 : 0;
        if (nativeHubStartup.active)
            nativeMenuPaint_Text("Choose your first fighter", 105, 98, 430, 18, gold, NativeMenuFont_MenuOption, true);
        else {
            nativeHub_Panel(175, 115, 430, 228);
        }
        unsigned number = 0;
        for (int k = Ft_Kind_Mario; k < Ft_Kind_StockCount; ++k) {
            auto kind = static_cast<FighterKind>(k);
            const char* name = nativeFighterProject_StockBaseName(kind);
            if (!name || kind == Ft_Kind_Kirby || kind == Ft_Kind_Nana ||
                kind == Ft_Kind_Seak ||
                !nativeFighterRuntime_FindStockDefinition(kind))
            {
                continue;
            }
            float x = offset + 186 + (number % 3) * 137, y = 126 + (number / 3) * 24;
            nativeHub_Button("template", name, x, y, 128, 20,
                             HubAction::StockTemplate, k,
                             k == hub.template_kind);
            ++number;
        }
        nativeMenuPaint_Text("Name your fighter", offset + 180, 355, 200, 9, muted);
        nativeHub_Field("clone-name", hub.title, offset + 175, 372, 266, 26,
                        HubAction::FieldTitle);
        nativeHub_Button("create-clone", "Clone fighter", offset + 452, 372, 153, 26,
                         HubAction::CreateClone, 0, true, *hub.title);
    }
    void quick_play()
    {
        const char* ids[] = {"quick-singles", "quick-doubles"};
        const char* labels[] = {"1 vs 1", "2 vs 2"};
        for (unsigned i = 0; i < 2; ++i) {
            const float x = 185 + i * 211;
            const bool hover = nativeHub_Hover(x, 175, 195, 82);
            nativeMenuPaint_Text(labels[i], x, 193, 195, 37,
                                 hover ? 0xFFE36BFF : 0xD5CED9FF,
                                 NativeMenuFont_MenuCaption, true, false);
            nativeHub_Region(ids[i], x, 175, 195, 82,
                             i ? HubAction::Doubles : HubAction::Singles);
        }
    }
    void chat(bool room = false)
    {
        const auto* global = nativeChat_View();
        const auto* lobby = nativeOnlineSession_Chat();
        const auto* presence = nativeOnlineSession_Presence();
        unsigned people = room ? (presence ? presence->participant_count : 0) : global->player_count;
        unsigned count = room ? (lobby ? lobby->count : 0) : global->count;
        bool ready = room ? lobby && lobby->connected && lobby->ready : global->state == NativeChat_Online;
        nativeHub_Panel(175, 116, 296, 234);
        nativeHub_Panel(479, 116, 126, 234);
        nativeMenuPaint_Text("Players", 489, 127, 103, 12, gold, NativeMenuFont_MenuOption);
        constexpr float font_size = 7.5f;
        constexpr float line_height = font_size + 3;
        constexpr float width = 270;
        constexpr float bottom = 335;
        constexpr unsigned visible_lines = 1 + unsigned((bottom - 151) / line_height);
        char message[640];
        auto format_message = [&](unsigned i) {
            const char* name = room ? lobby->messages[i].name : global->messages[i].name;
            const char* content = room ? lobby->messages[i].text : global->messages[i].text;
            std::snprintf(message, sizeof(message), "%s: %s", name, content);
            return unsigned(std::strlen(name)) + 1;
        };
        int& scroll = room ? hub.room_chat_scroll : hub.chat_scroll;
        unsigned total_lines = 0;
        for (unsigned i = 0; i < count; ++i) {
            format_message(i);
            total_lines += nativeHub_TextBlock(message, 0, 0, width, 0, font_size, 0xFFFFFFFF);
        }
        scroll = std::clamp(scroll, 0, std::max(0, int(total_lines) - int(visible_lines)));
        nativeMenuPaint_Text("Messages", 186, 127, 172, 12, gold, NativeMenuFont_MenuOption);
        nativeHub_Button("chat-older", "Older", 371, 122, 43, 18,
            HubAction::ChatScroll, visible_lines - 1, false,
            unsigned(scroll) + visible_lines < total_lines);
        nativeHub_Button("chat-newer", "Newer", 420, 122, 43, 18,
            HubAction::ChatScroll, 1 - int(visible_lines), false, scroll > 0);
        unsigned skip = unsigned(scroll);
        unsigned remaining = visible_lines;
        for (unsigned i = count; i > 0 && remaining; --i) {
            const unsigned name_length = format_message(i - 1);
            const unsigned lines = nativeHub_TextBlock(message, 0, 0, width, 0, font_size, 0xFFFFFFFF);
            if (skip >= lines) {
                skip -= lines;
                continue;
            }
            const unsigned end = lines - skip;
            const unsigned rows = std::min(remaining, end);
            const float y = bottom - (visible_lines - remaining + rows - 1) * line_height;
            nativeHub_TextBlock(message, 186, y, width, rows * line_height - 3,
                                font_size, 0xFFFFFFFF, end - rows, name_length, gold);
            remaining -= rows;
            skip = 0;
        }
        if (!count) nativeMenuPaint_Text(ready ? "Be the first to say hello." : room ? "Connecting to lobby chat..." : "Connecting to global chat...", 186, 155, 272, 10, muted);
        int& people_scroll = room ? hub.room_people_scroll : hub.people_scroll;
        people_scroll = std::clamp(people_scroll, 0, std::max(0, int(people) - 12));
        unsigned last = std::min(unsigned(people_scroll + 12), people);
        for (unsigned i = unsigned(people_scroll); i < last; ++i) {
            const char* name = room ? presence->participants[i].name : global->players[i].name;
            const char* peer = room ? presence->participants[i].peer_id : global->players[i].peer;
            const char* account = room ? presence->participants[i].account_handle : global->players[i].account_handle;
            const float row_y = 149 + (i - people_scroll) * 14;
            const bool hover = nativeHub_Hover(487, row_y, 108, 14);
            nativeMenuPaint_Text(name, 487, row_y, 108, 10,
                                 hover ? 0xFFE36BFF : gold,
                                 NativeMenuFont_Notice, false, false);
            nativeHub_Region("chat-player", 487, row_y, 108, 14,
                *account ? HubAction::OpenAccount : HubAction::ShowProfile, 0, true, peer, name, account);
        }
        if (people > 12) {
            nativeHub_Button("people-prev", "<", 489, 324, 28, 18, HubAction::PeopleScroll, -12, false, people_scroll > 0);
            nativeHub_Button("people-next", ">", 567, 324, 28, 18, HubAction::PeopleScroll, 12, false, last < people);
        }
        nativeHub_Field(room ? "lobby-message" : "chat-message", room ? hub.room_chat : hub.chat,
            175, 362, 328, 28, room ? HubAction::FieldRoomChat : HubAction::FieldChat);
        bool sending = room ? lobby && lobby->delivery == NativeRoomChatDelivery_Sending : global->delivery == NativeChatDelivery_Sending;
        bool unconfirmed = room ? lobby && lobby->delivery == NativeRoomChatDelivery_Unconfirmed : global->delivery == NativeChatDelivery_Unconfirmed;
        nativeHub_Button(room ? "send-lobby-chat" : "send-chat", "Send", 512, 362, 93, 28,
            room ? HubAction::SendRoomChat : HubAction::SendChat, 0, true,
            *(room ? hub.room_chat : hub.chat) && ready && !sending);
        const char* status = sending ? "Sending..." : unconfirmed ? "Delivery is unconfirmed." : ready ? "" : "Connecting...";
        if (room && lobby && lobby->failure[0]) status = lobby->failure;
        if (*status) nativeMenuPaint_Text(status, 181, 396, room && unconfirmed ? 325 : 416, 8, muted);
        if (room && unconfirmed) nativeHub_Button("retry-lobby-chat", "Retry", 536, 393, 69, 14, HubAction::RetryRoomChat, 0, false, ready);
    }
    void profile()
    {
        if (hub.profile_self) {
            nativeMenuPaint_Text("Display name", 65, 127, 518, 16, gold,
                                NativeMenuFont_MenuOption);
            nativeHub_Field("profile-display-name", hub.name, 65, 153, 518, 30,
                            HubAction::FieldName, false, "Player");
            nativeMenuPaint_Text("About", 65, 205, 518, 16, gold,
                                NativeMenuFont_MenuOption);
            nativeHub_Field("profile-bio", hub.bio, 65, 231, 518, 103,
                            HubAction::FieldBio);
            nativeHub_Button("save-bio", "Save profile", 398, 353, 185, 30,
                             HubAction::SaveProfile, 0, true);
            nativeHub_Button("profile-back", "< Back", 40, 402, 110, 28,
                             HubAction::Back);
            return;
        }
        nativeHub_Panel(175, 118, 430, 240);
        nativeMenuPaint_Text("About", 194, 138, 370, 15, gold,
                            NativeMenuFont_MenuOption);
        nativeMenuPaint_Text("This guest has no linked online account.", 194, 175,
                            380, 10, muted);
        nativeHub_Button("guest-find-account", "Find an account", 193, 251, 394,
                         30, HubAction::Friends);
    }
    void send()
    {
        nativeHub_Panel(175, 117, 430, 252);
        nativeMenuPaint_Text(hub.gift_title, 191, 136, 335, 18, gold,
                            NativeMenuFont_MenuOption);
        if (!hub.send_from_css && nativeHubAccount_View().signed_in) {
            nativeHub_Button("gift-lobby-tab", "Lobby", 191, 153, 77, 20,
                             HubAction::GiftRecipientMode, 0, !hub.send_to_friends);
            nativeHub_Button("gift-friends-tab", "Friends", 276, 153, 77, 20,
                             HubAction::GiftRecipientMode, 1, hub.send_to_friends);
        }
        if (hub.send_to_friends) { nativeHubSocial_FighterRecipients(); return; }
        const auto* p = nativeOnlineSession_Presence();
        unsigned rows = 0, total = 0;
        unsigned available = 0;
        for (unsigned i = 0; p && i < p->participant_count; ++i) {
            if (std::strcmp(p->participants[i].peer_id,
                             nativeOnlineSession_LocalPeerId())) {
                ++available;
            }
        }
        hub.recipient_scroll =
            std::min(hub.recipient_scroll,
                     int(available ? (available - 1) / 7 * 7 : 0));
        bool recipient_available = false;
        if (p) {
            for (unsigned i = 0; i < p->participant_count; ++i) {
                const auto& person = p->participants[i];
                if (!std::strcmp(person.peer_id,
                                 nativeOnlineSession_LocalPeerId()))
                {
                    continue;
                }
                if (total >= unsigned(hub.recipient_scroll) && rows < 7) {
                    nativeHub_Button(
                        "gift-recipient", person.name, 191, 178 + rows * 24,
                        394, 21, HubAction::Recipient, int(i),
                        !std::strcmp(hub.recipient, person.peer_id), true,
                        person.peer_id, person.name);
                    ++rows;
                }
                if (!std::strcmp(hub.recipient, person.peer_id)) {
                    recipient_available = true;
                }
                ++total;
            }
        }
        nativeHub_Button("recipients-prev", "<", 538, 136, 22, 20,
                         HubAction::RecipientScroll, -7, false,
                         hub.recipient_scroll > 0);
        nativeHub_Button("recipients-next", ">", 567, 136, 22, 20,
                         HubAction::RecipientScroll, 7, false,
                         unsigned(hub.recipient_scroll + 7) < total);
        if (!rows) {
            nativeMenuPaint_Text(
                "Join a lobby with another player to send your fighter.", 191,
                193, 393, 11, muted);
        }
        if (*hub.recipient && !recipient_available) {
            nativeMenuPaint_Text("That player has left the lobby.",
                                191, 342, 394, 8, muted);
        }
        nativeHub_Button("confirm-gift", "Send fighter", 382, 376, 223, 29,
                         HubAction::ConfirmSend, 0, true,
                         recipient_available);
    }
}
void nativeHub_RenderPage(NativeHubPage page)
{
    switch (page) {
    case NativeHubPage_MatchRules:
    case NativeHubPage_ItemSwitch:
    case NativeHubPage_AdditionalRules:
    case NativeHubPage_LobbyRules:
        nativeHubRules_Render(page); break;
    case NativeHubPage_Costumes:
        nativeHubCostumes_Render(); break;
    case NativeHubPage_Packages:
    case NativeHubPage_PackageRecipient:
        nativeHubPackages_Render(page); break;
    case NativeHubPage_PlayerProfiles:
    case NativeHubPage_ControllerCalibration:
    case NativeHubPage_ControllerPicker:
        nativeHubProfiles_Render(); break;
    case NativeHubPage_Controls:
    case NativeHubPage_Video:
    case NativeHubPage_Audio:
    case NativeHubPage_Game:
    case NativeHubPage_OnlineSettings:
        nativeHubSettings_Render(page); break;
    case NativeHubPage_News:
        nativeHubNews_Render(); break;
    case NativeHubPage_Portrait:
        nativeHubPortrait_Render(); break;
    case NativeHubPage_Startup:
        nativeHubStartup_Render();
        break;
    case NativeHubPage_Workshop:
        collection();
        break;
    case NativeHubPage_Clone:
        if (nativeHubStartup.active) nativeHubStartupRoster_Render();
        else clone_page();
        break;
    case NativeHubPage_QuickPlay:
        quick_play();
        break;
    case NativeHubPage_CreateLobby:
        nativeHubLobbies_Render(true);
        break;
    case NativeHubPage_Lobbies:
        nativeHubLobbies_Render(false);
        break;
    case NativeHubPage_RoomChat:
        chat(true);
        break;
    case NativeHubPage_Chat:
        chat();
        break;
    case NativeHubPage_Friends:
    case NativeHubPage_Inbox:
    case NativeHubPage_Letter:
    case NativeHubPage_ComposeLetter:
    case NativeHubPage_SocialProfile:
        nativeHubSocial_Render(page);
        break;
    case NativeHubPage_Settings:
        nativeHubSettings_Render(page);
        break;
    case NativeHubPage_Profile:
        profile();
        break;
    case NativeHubPage_Account:
        nativeHubAccount_Render();
        break;
    case NativeHubPage_Send:
        send();
        break;
    case NativeHubPage_QuickStage:
        nativeHubQuickStage_Render();
        break;
    }
}

void nativeHub_RenderSession()
{
    const auto state = nativeOnlineSession_State();
    if (state == NativeLobbySessionState_Disconnected || state == NativeLobbySessionState_Failed) return;
    const auto* quick = nativeOnlineSession_Quick();
    const auto* listing = nativeOnlineSession_Listing();
    const char* code = nativeOnlineSession_RoomCode();
    char label[112];
    if (quick) std::snprintf(label, sizeof(label), "Quick Play %s%s",
        quick->mode == NativeQuickMode_Doubles ? "2 vs 2" : "1 vs 1",
        quick->state == NativeQuickState_Searching ? " - Searching" : "");
    else std::snprintf(label, sizeof(label), "%s%s%s",
        listing && *listing->title ? listing->title : "Lobby",
        code && *code ? " - " : "", code ? code : "");
    nativeMenuSkin_Panel(38, 437, 567, 33, 0xC2BEC7FF, 0x09080EDC);
    nativeMenuPaint_Text(label, 48, 448, 260, 10, gold);
    nativeHub_Button("return-session", quick ? "Return to Quick Play" : "Return to lobby",
                     322, 442, 145, 24, HubAction::Selection);
    nativeHub_Button("leave-session", quick ? "Leave queue" : "Leave lobby",
                     476, 442, 119, 24, HubAction::Leave);
}
