#include "native_hub_internal.h"
#include "native_hub_quick_stage.h"
#include "native_hub_lobbies.h"
#include <melee/ft/native_fighter_runtime.h>
#include <melee/gm/native_online_session.h>
#include <melee/gm/native_home.h>
#include <SDL3/SDL_timer.h>
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace {
    constexpr uint32_t gold = 0xE5C361FF, muted = 0xA6A1AFFF;
    struct MessageLines {
        char lines[32][128];
        unsigned count;
    };
    MessageLines wrap_message(const char* text)
    {
        MessageLines result{};
        while (*text && result.count < 32) {
            while (*text == ' ') {
                ++text;
            }
            if (!*text) {
                break;
            }
            unsigned n = 0, last_space = 0;
            char* line = result.lines[result.count];
            while (text[n] && text[n] != '\n' && n < 126) {
                line[n] = text[n];
                line[n + 1] = 0;
                if (nativeMenuFont_Width(NativeMenuFont_Notice, line, 8) >
                        270 &&
                    n)
                {
                    if (last_space) {
                        n = last_space;
                    }
                    break;
                }
                if (text[n] == ' ') {
                    last_space = n;
                }
                ++n;
            }
            if (!n && *text != '\n') {
                n = 1;
            }
            std::memcpy(line, text, n);
            line[n] = 0;
            text += n;
            if (*text == '\n') {
                ++text;
            }
            ++result.count;
        }
        return result;
    }
    void description(const char* subtitle)
    {
        if (subtitle) {
            nativeMenuPaint_Text(subtitle, 175, 94, 420, 8, muted);
        }
    }
    void fighter_tile(unsigned index, float x, float y)
    {
        const NativeHubFighter& f = nativeHubFighters[index];
        bool selected = int(index) == hub.selected,
             hover = nativeHub_Hover(x, y, 49, 49);
        nativeMenuPaint_Round(x - 1, y - 1, 51, 52, 3,
                             selected || hover ? gold : 0xBAB6BFFF);
        nativeMenuPaint_Round(x + 1, y + 1, 47, 48, 2, 0x17151FFF);
        char lines[2][128];
        unsigned count = nativeMenuFont_LayoutTileName(f.title, 45, 8, lines);
        float name_y = 49 - (count == 2 ? 18 : 10);
        if (f.pixels) {
            nativeMenuPaint_Image(f.pixels, f.width, f.height, x + 2, y + 2, 45,
                                 name_y - 2);
        } else {
            nativeMenuPaint_Text(f.title, x + 5, y + 12, 40, 13, muted,
                                NativeMenuFont_MenuOption, true);
        }
        nativeMenuPaint_Rect(x + 2, y + name_y, 45, 49 - name_y, 0x111019EE);
        for (unsigned line = 0; line < count; ++line) {
            nativeMenuPaint_Text(lines[line], x + 2, y + name_y + 1 + line * 8,
                                45, 8, 0xFFF4E9FF, NativeMenuFont_Tile, true);
        }
        nativeHub_Region("fighter", x, y, 49, 49, HubAction::Fighter,
                         int(index));
    }
    void collection()
    {
        description("Clone a fighter. Make it yours.");
        nativeHub_Button("clone-fighter", "Clone fighter", 175, 110, 100, 26,
                         HubAction::NewClone, 0, true, nativeHome_IsSelection());
        nativeHub_Button("import-fighter", "Import model / fighter", 282, 110,
                         135, 26, HubAction::OpenImport, 0, false, nativeHome_IsSelection());
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
            nativeMenuPaint_Text(f.installed
                                    ? "Moves - Scripts - Animation - Model"
                                    : "Editable draft",
                                194, 313, 240, 8, muted);
            nativeHub_Button("edit-fighter", "Edit fighter", 465, 285, 122, 36,
                             HubAction::Edit, 0, true, nativeHome_IsSelection());
            nativeHub_Button("copy-fighter", "Clone", 194, 332, 49, 21,
                             HubAction::CopyFighter, 0, false, f.installed && nativeHome_IsSelection());
            nativeHub_Button("rename-fighter", "Rename", 249, 332, 64, 21,
                             HubAction::RenameFighter, 0, false, nativeHome_IsSelection());
            nativeHub_Button("send-fighter", "Send fighter", 320, 332, 81, 21,
                             HubAction::SendFighter, 0, false, f.installed);
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
        if (hub.rename) {
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
        description("Your copy owns its moves, scripts, animations and model.");
        nativeHub_Panel(175, 115, 430, 228);
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
            float x = 186 + (number % 3) * 137, y = 126 + (number / 3) * 24;
            nativeHub_Button("template", name, x, y, 128, 20,
                             HubAction::StockTemplate, k,
                             k == hub.template_kind);
            ++number;
        }
        nativeMenuPaint_Text("Name your fighter", 180, 355, 200, 9, muted);
        nativeHub_Field("clone-name", hub.title, 175, 372, 266, 26,
                        HubAction::FieldTitle);
        nativeHub_Button("create-clone", "Clone fighter", 452, 372, 153, 26,
                         HubAction::CreateClone, 0, true, *hub.title);
    }
    void quick_play()
    {
        nativeHub_Button("quick-singles", "1 vs 1", 190, 157, 190, 109,
                         HubAction::Singles, 0, true);
        nativeHub_Button("quick-doubles", "2 vs 2", 397, 157, 190, 109,
                         HubAction::Doubles, 0, true);
    }
    void lobbies()
    {
        description("Play together. Share your fighters.");
        auto state = nativeOnlineSession_State();
        const char* code = nativeOnlineSession_RoomCode();
        bool connected = state != NativeLobbySessionState_Disconnected &&
                         state != NativeLobbySessionState_Failed;
        if (!connected) {
            nativeHubLobbies_Render(false);
            return;
        }
        char heading[96];
        const auto* listing = nativeOnlineSession_Listing();
        std::snprintf(heading, sizeof(heading), "%s", listing && listing->title[0] ? listing->title :
                      code && *code ? code : "Connecting...");
        nativeMenuPaint_Text(heading, 181, 119, 194, 17, gold,
                            NativeMenuFont_MenuOption);
        nativeHub_Button("lobby-chat", "Chat", 388, 115, 94, 25, HubAction::RoomChat);
        nativeHub_Button("copy-code", "Copy code", 490, 115, 114, 25,
                         HubAction::CopyCode);
        nativeHub_Panel(175, 150, 430, 170);
        if (listing && listing->title[0] && nativeOnlineSession_IsOwner() && !nativeOnlineSession_Quick())
            nativeHub_Button("change-listing", listing->listed ? "Listed" : "Unlisted", 175, 400, 141, 17, HubAction::ChangeListing);
        const NativeSignalingPresence* p = nativeOnlineSession_Presence();
        const NativeQuickAssignment* quick = nativeOnlineSession_Quick();
        float y = 161;
        hub.lobby_scroll = std::clamp(hub.lobby_scroll, 0, p && p->participant_count ? int((p->participant_count - 1) / 6 * 6) : 0);
        if (p) {
            for (unsigned i = unsigned(hub.lobby_scroll); i < p->participant_count && i < unsigned(hub.lobby_scroll + 6); ++i) {
                const auto& person = p->participants[i];
                nativeHub_Button("lobby-player", person.name, 187, y, 182, 19,
                                 HubAction::ShowProfile, 0, false, true,
                                 person.peer_id, person.name);
                for (unsigned seat = 0;
                     seat < person.local_players && seat < 4; ++seat)
                {
                    const char* label =
                        (person.active_mask & (1U << seat))    ? "Fighting"
                        : (person.waiting_mask & (1U << seat)) ? "Waiting"
                                                               : "Spectating";
                    char seat_id[32];
                    std::snprintf(seat_id, sizeof(seat_id), "lobby-seat-%u-%u", i, seat);
                    nativeHub_Button(seat_id, label, 381 + seat * 52, y,
                                     50, 19, HubAction::Seat, int(seat), false,
                                     true, person.peer_id, person.name);
                }
                y += 24;
            }
        }
        if (!p || !p->participant_count) {
            nativeMenuPaint_Text("Connecting to the lobby...", 190, 171, 380,
                                11, muted);
        }
        if (p && p->participant_count > 6) {
            char page[40];
            std::snprintf(page, sizeof(page), "%d-%u / %u", hub.lobby_scroll + 1,
                std::min(unsigned(hub.lobby_scroll + 6), unsigned(p->participant_count)), p->participant_count);
            nativeMenuPaint_Text(page, 241, 308, 290, 8, muted);
            nativeHub_Button("lobby-prev", "<", 187, 303, 35, 17, HubAction::LobbyScroll, -6, false, hub.lobby_scroll > 0);
            nativeHub_Button("lobby-next", ">", 558, 303, 35, 17, HubAction::LobbyScroll, 6, false, hub.lobby_scroll + 6 < p->participant_count);
        }
        nativeHub_Button("lobby-ready", nativeHome_IsPractice() ? "Return to practice" : "Choose fighter", 175, 337, 141, 31,
                         HubAction::Practice, 0, true);
        const char* start_label = !quick ? "Start match"
            : quick->state == NativeQuickState_Searching ? "Cancel search"
            : quick->state == NativeQuickState_Idle ? "Enter queue" : "Leave match";
        nativeHub_Button("start-match", start_label, 325, 337, 141, 31,
                         HubAction::StartMatch, 0, false,
                         quick ? (quick->state == NativeQuickState_Assigned || nativeOnlineSession_IsOwner())
                               : nativeHome_CanStartMatch());
        nativeHub_Button("leave-lobby", quick && quick->state >= NativeQuickState_Assigned
            ? "Leave match" : "Leave lobby", 476, 337, 129, 31,
                         HubAction::Leave);
        nativeHub_Button("lobby-selection", "Character select", 175, 376, 141, 22,
                         HubAction::Selection, 0, false, nativeHome_IsPractice());
        const char* rotations[] = { "Manual", "Winner stays", "Winner out" };
        nativeHub_Button("rotation-policy", quick ? "Quick Play" : p ? rotations[unsigned(p->rotation_policy)%3] : "Manual", 325, 376, 141, 22,
                         HubAction::Rotation, 0, false, !quick && nativeOnlineSession_IsOwner());
        nativeHub_Button("match-rules", "Match rules", 476, 376, 129, 22,
                         HubAction::Rules, 0, false, !quick && nativeHome_IsSelection() && nativeOnlineSession_IsOwner());
    }
    void chat(bool room = false)
    {
        const auto* global = nativeChat_View();
        const auto* lobby = nativeOnlineSession_Chat();
        const auto* presence = nativeOnlineSession_Presence();
        unsigned people = room ? (presence ? presence->participant_count : 0) : global->player_count;
        unsigned count = room ? (lobby ? lobby->count : 0) : global->count;
        bool ready = room ? lobby && lobby->connected && lobby->ready : global->state == NativeChat_Online;
        char label[80];
        if (room) std::snprintf(label, sizeof(label), "Lobby %s - %u players", nativeOnlineSession_RoomCode() ? nativeOnlineSession_RoomCode() : "", people);
        else std::snprintf(label, sizeof(label), "Global chat - %u online", global->online);
        description(label);
        nativeHub_Panel(175, 116, 296, 234);
        nativeHub_Panel(479, 116, 126, 234);
        nativeMenuPaint_Text("Players", 489, 127, 103, 12, gold, NativeMenuFont_MenuOption);
        int& scroll = room ? hub.room_chat_scroll : hub.chat_scroll;
        unsigned total_lines = 0;
        for (unsigned i = 0; i < count; ++i)
            total_lines += wrap_message(room ? lobby->messages[i].text : global->messages[i].text).count + 2;
        scroll = std::clamp(scroll, 0, std::max(0, int(total_lines) - 17));
        nativeMenuPaint_Text("Messages", 186, 127, 172, 12, gold, NativeMenuFont_MenuOption);
        nativeHub_Button("chat-older", "Older", 371, 122, 43, 18,
            HubAction::ChatScroll, 16, false, scroll + 17 < int(total_lines));
        nativeHub_Button("chat-newer", "Newer", 420, 122, 43, 18,
            HubAction::ChatScroll, -16, false, scroll > 0);
        unsigned skip = unsigned(scroll);
        float y = 335;
        for (unsigned i = count; i > 0 && y >= 151; --i) {
            const char* text = room ? lobby->messages[i - 1].text : global->messages[i - 1].text;
            const char* name = room ? lobby->messages[i - 1].name : global->messages[i - 1].name;
            MessageLines lines = wrap_message(text);
            for (int row = int(lines.count) + 1; row >= 0 && y >= 151; --row) {
                if (skip) { --skip; continue; }
                if (!row) nativeMenuPaint_Text(name, 186, y, 270, 9, gold);
                else if (unsigned(row) <= lines.count) nativeMenuPaint_Text(lines.lines[row - 1], 186, y, 270, 8);
                y -= 11;
            }
        }
        if (!count) nativeMenuPaint_Text(ready ? "Be the first to say hello." : room ? "Connecting to lobby chat..." : "Connecting to global chat...", 186, 155, 272, 10, muted);
        int& people_scroll = room ? hub.room_people_scroll : hub.people_scroll;
        people_scroll = std::clamp(people_scroll, 0, std::max(0, int(people) - 12));
        unsigned last = std::min(unsigned(people_scroll + 12), people);
        for (unsigned i = unsigned(people_scroll); i < last; ++i) {
            const char* name = room ? presence->participants[i].name : global->players[i].name;
            const char* peer = room ? presence->participants[i].peer_id : global->players[i].peer;
            nativeHub_Button("chat-player", name, 487, 149 + (i - people_scroll) * 14, 108, 13,
                HubAction::ShowProfile, 0, false, true, peer, name);
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
        const char* status = sending ? "Sending..." : unconfirmed ? "Delivery is unconfirmed." : ready ? "Connected" : "Connecting...";
        if (room && lobby && lobby->failure[0]) status = lobby->failure;
        nativeMenuPaint_Text(status, 181, 396, room && unconfirmed ? 325 : 416, 8, muted);
        if (room && unconfirmed) nativeHub_Button("retry-lobby-chat", "Retry", 536, 393, 69, 14, HubAction::RetryRoomChat, 0, false, ready);
    }
    void settings()
    {
        description("Your controls and profile.");
        nativeHub_Panel(175, 116, 430, 256);
        nativeHub_Button("controls", "Controls & bindings", 193, 135, 392, 36,
                         HubAction::Controls, 0, true, nativeHome_IsSelection());
        nativeHub_Button("settings-rules", "Match rules", 193, 310, 187, 31,
                         HubAction::Rules, 0, false, nativeHome_IsSelection());
        nativeHub_Button("settings-costumes", "Costumes", 391, 310, 194, 31,
                         HubAction::Costumes, 0, false, nativeHome_IsSelection());
        nativeHub_Button("settings-packages", "Installed packages", 193, 347, 392, 21,
                         HubAction::Packages, 0, false, nativeHome_IsSelection());
        nativeMenuPaint_Text("Display name", 194, 196, 360, 10, gold);
        nativeHub_Field("display-name", hub.name, 193, 216, 392, 27,
                        HubAction::FieldName);
        nativeHub_Button("edit-profile", "Your profile", 193, 264, 187, 31,
                         HubAction::Profile);
        nativeHub_Button("save-profile", "Save settings", 391, 264, 194, 31,
                         HubAction::SaveProfile, 0, true, *hub.name);
    }
    void profile()
    {
        description(hub.profile_self ? "Your profile" : "Player profile");
        nativeHub_Panel(175, 118, 430, 240);
        nativeMenuPaint_Text("About", 194, 138, 370, 15, gold,
                            NativeMenuFont_MenuOption);
        if (hub.profile_self) {
            nativeHub_Field("profile-bio", hub.bio, 194, 171, 390, 68,
                            HubAction::FieldBio);
            nativeHub_Button("save-bio", "Save profile", 194, 263, 165, 30,
                             HubAction::SaveProfile, 0, true);
        } else {
            nativeMenuPaint_Text("This player has not shared a bio.", 194, 175,
                                380, 10, muted);
            nativeHub_Button("write-letter", "Write a letter", 193, 251, 120,
                             30, HubAction::WriteLetter);
            nativeHub_Button("add-friend", "Add friend", 321, 251, 120, 30,
                             HubAction::AddFriend);
            nativeHub_Button("invite-player", "Invite to lobby", 449, 251, 138,
                             30, HubAction::Invite);
        }
    }
    void social(bool inbox)
    {
        description(inbox ? "Letters, friend requests and invitations."
                    : "Find your people. Invite them to play.");
        nativeHub_Panel(175, 119, 430, 226);
        nativeMenuPaint_Text(inbox ? "Your mailbox" : "Your friends", 194, 143,
                            370, 20, gold, NativeMenuFont_MenuOption);
        nativeMenuPaint_Text(
            "Account sign-in and offline delivery are being connected.", 194,
            187, 382, 10, muted);
        nativeMenuPaint_Text("Global chat and lobbies are available now.", 194,
                            211, 382, 10);
        nativeHub_Button("go-chat", "Open Chatroom", 194, 263, 172, 32,
                         HubAction::Chat, 0, true);
        nativeHub_Button("go-lobbies", "Open Lobbies", 379, 263, 205, 32,
                         HubAction::Lobbies);
    }
    void send()
    {
        description("Choose someone in your lobby.");
        nativeHub_Panel(175, 117, 430, 252);
        nativeMenuPaint_Text(hub.gift_title, 191, 136, 335, 18, gold,
                            NativeMenuFont_MenuOption);
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
                        "gift-recipient", person.name, 191, 172 + rows * 24,
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
    case NativeHubPage_Workshop:
        collection();
        break;
    case NativeHubPage_Clone:
        clone_page();
        break;
    case NativeHubPage_QuickPlay:
        quick_play();
        break;
    case NativeHubPage_CreateLobby:
        nativeHubLobbies_Render(true);
        break;
    case NativeHubPage_Lobbies:
        lobbies();
        break;
    case NativeHubPage_RoomChat:
        chat(true);
        break;
    case NativeHubPage_Chat:
        chat();
        break;
    case NativeHubPage_Friends:
        social(false);
        break;
    case NativeHubPage_Inbox:
        social(true);
        break;
    case NativeHubPage_Settings:
        settings();
        break;
    case NativeHubPage_Profile:
        profile();
        break;
    case NativeHubPage_Send:
        send();
        break;
    case NativeHubPage_QuickStage:
        nativeHubQuickStage_Render();
        break;
    }
}
