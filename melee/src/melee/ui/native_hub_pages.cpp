#include "native_hub_internal.h"
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
    void title(const char* text, const char* subtitle)
    {
        nativeHubPaint_Text(text, 175, 70, 420, 17, 0xF5EFEAFF,
                            NativeMenuFont_Heading);
        if (subtitle) {
            nativeHubPaint_Text(subtitle, 175, 94, 420, 8, muted);
        }
    }
    void fighter_tile(unsigned index, float x, float y)
    {
        const NativeHubFighter& f = nativeHubFighters[index];
        bool selected = int(index) == hub.selected,
             hover = nativeHub_Hover(x, y, 49, 49);
        nativeHubPaint_Round(x - 1, y - 1, 51, 52, 3,
                             selected || hover ? gold : 0xBAB6BFFF);
        nativeHubPaint_Round(x + 1, y + 1, 47, 48, 2, 0x17151FFF);
        if (f.pixels) {
            nativeHubPaint_Image(f.pixels, f.width, f.height, x + 2, y + 2, 45,
                                 37);
        } else {
            nativeHubPaint_Text(f.title, x + 5, y + 12, 40, 13, muted,
                                NativeMenuFont_Heading, true);
        }
        nativeHubPaint_Rect(x + 2, y + 39, 45, 10, 0x111019EE);
        nativeHubPaint_Text(f.title, x + 2, y + 40, 45, 8, 0xFFF4E9FF,
                            NativeMenuFont_Tile, true);
        nativeHub_Region("fighter", x, y, 49, 49, HubAction::Fighter,
                         int(index));
    }
    void collection()
    {
        title("Your fighters", "Clone a fighter. Make it yours.");
        nativeHub_Button("clone-fighter", "Clone fighter", 175, 110, 100, 26,
                         HubAction::NewClone, 0, true);
        nativeHub_Button("import-fighter", "Import model / fighter", 282, 110,
                         135, 26, HubAction::OpenImport);
        nativeHub_Panel(175, 148, 430, 210);
        unsigned end =
            std::min(nativeHubFighterCount, unsigned(hub.first_fighter + 14));
        for (unsigned i = hub.first_fighter; i < end; ++i) {
            fighter_tile(i, 189 + ((i - hub.first_fighter) % 7) * 57,
                         162 + ((i - hub.first_fighter) / 7) * 58);
        }
        if (!nativeHubFighterCount) {
            nativeHubPaint_Text("Start your roster", 200, 179, 380, 20, gold,
                                NativeMenuFont_Heading);
            nativeHubPaint_Text(
                "Choose a vanilla fighter to make your first editable copy.",
                200, 212, 380, 9);
            nativeHub_Button("first-clone", "Choose a fighter", 200, 240, 140,
                             30, HubAction::NewClone, 0, true);
        } else if (hub.selected >= 0 &&
                   unsigned(hub.selected) < nativeHubFighterCount)
        {
            const NativeHubFighter& f = nativeHubFighters[hub.selected];
            nativeHubPaint_Text(f.title, 194, 290, 260, 17, 0xF5EFEAFF,
                                NativeMenuFont_Heading);
            nativeHubPaint_Text(f.installed
                                    ? "Moves - Scripts - Animation - Model"
                                    : "Editable draft",
                                194, 313, 240, 8, muted);
            nativeHub_Button("edit-fighter", "Edit fighter", 465, 285, 122, 36,
                             HubAction::Edit, 0, true);
            nativeHub_Button("copy-fighter", "Clone", 194, 332, 49, 21,
                             HubAction::CopyFighter, 0, false, f.installed);
            nativeHub_Button("rename-fighter", "Rename", 249, 332, 64, 21,
                             HubAction::RenameFighter);
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
                         HubAction::EditLayout);
        nativeHub_Button("local-practice", "Local practice", 282, 372, 100, 25,
                         HubAction::Practice);
        NativeNetplayGiftStatus gift{};
        if (nativeOnlineSession_GiftStatus(true, &gift) &&
            gift.state == NativeNetplayGift_Offered)
        {
            nativeHub_Panel(185, 225, 410, 58);
            nativeHubPaint_Text("Fighter delivery", 197, 233, 200, 10, gold);
            nativeHubPaint_Text(gift.title, 197, 253, 200, 11);
            nativeHub_Button("accept-fighter", "Accept fighter", 414, 235, 103,
                             27, HubAction::AcceptGift, 0, true);
            nativeHub_Button("decline-fighter", "Decline", 523, 235, 60, 27,
                             HubAction::DeclineGift);
        }
        if (hub.rename) {
            nativeHub_Panel(194, 196, 390, 91);
            nativeHubPaint_Text("Fighter name", 208, 207, 200, 10, gold);
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
        title("Clone a fighter",
              "Your copy owns its moves, scripts, animations and model.");
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
        nativeHubPaint_Text("Name your fighter", 180, 355, 200, 9, muted);
        nativeHub_Field("clone-name", hub.title, 175, 372, 266, 26,
                        HubAction::FieldTitle);
        nativeHub_Button("create-clone", "Clone fighter", 452, 372, 153, 26,
                         HubAction::CreateClone, 0, true, *hub.title);
    }
    void quick_play()
    {
        title("Quick Play", nullptr);
        nativeHub_Button("quick-singles", "1 vs 1", 190, 157, 190, 109,
                         HubAction::Singles, 0, true);
        nativeHub_Button("quick-doubles", "2 vs 2", 397, 157, 190, 109,
                         HubAction::Doubles, 0, true);
        nativeHubPaint_Text(
            "Matchmaking is being connected to the online service.", 188, 299,
            405, 9, muted, NativeMenuFont_Notice, true);
    }
    void lobbies()
    {
        title("Lobbies", "Play together. Share your fighters.");
        auto state = nativeOnlineSession_State();
        const char* code = nativeOnlineSession_RoomCode();
        bool connected = state != NativeLobbySessionState_Disconnected &&
                         state != NativeLobbySessionState_Failed;
        if (!connected) {
            nativeHub_Panel(175, 118, 430, 210);
            nativeHubPaint_Text("Create a lobby", 192, 137, 350, 15, gold,
                                NativeMenuFont_Heading);
            nativeHubPaint_Text("Invite friends with your lobby code.", 192,
                                161, 350, 9, muted);
            nativeHub_Button("create-lobby", "Create lobby", 192, 185, 148, 30,
                             HubAction::Create, 0, true);
            nativeHubPaint_Text("Join a lobby", 192, 239, 350, 15, gold,
                                NativeMenuFont_Heading);
            nativeHub_Field("lobby-code", hub.code, 192, 267, 240, 27,
                            HubAction::FieldCode);
            nativeHub_Button("join-lobby", "Join", 444, 267, 140, 27,
                             HubAction::Join, 0, true,
                             std::strlen(hub.code) >= 6);
            const char* failure = nativeOnlineSession_Failure();
            if (failure && *failure) {
                nativeHubPaint_Text(failure, 190, 342, 400, 10, 0xF4A49DFF);
            }
            return;
        }
        char heading[96];
        std::snprintf(heading, sizeof(heading), "Lobby %s",
                      code && *code ? code : "connecting...");
        nativeHubPaint_Text(heading, 181, 119, 289, 17, gold,
                            NativeMenuFont_Heading);
        nativeHub_Button("copy-code", "Copy code", 490, 115, 114, 25,
                         HubAction::CopyCode);
        nativeHub_Panel(175, 150, 430, 170);
        const NativeSignalingPresence* p = nativeOnlineSession_Presence();
        float y = 161;
        if (p) {
            for (unsigned i = 0; i < p->participant_count && y < 294; ++i) {
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
                    nativeHub_Button("lobby-seat", label, 381 + seat * 52, y,
                                     50, 19, HubAction::Seat, int(seat), false,
                                     true, person.peer_id, person.name);
                }
                y += 24;
            }
        }
        if (!p || !p->participant_count) {
            nativeHubPaint_Text("Connecting to the lobby...", 190, 171, 380,
                                11, muted);
        }
        nativeHub_Button("lobby-ready", "Choose fighter", 175, 337, 141, 31,
                         HubAction::Practice, 0, true);
        nativeHub_Button("start-match", "Start match", 325, 337, 141, 31,
                         HubAction::StartMatch, 0, false,
                         nativeHome_CanStartMatch());
        nativeHub_Button("leave-lobby", "Leave lobby", 476, 337, 129, 31,
                         HubAction::Leave);
        nativeHubPaint_Text(
            "Choose Fighting, Waiting or Spectating for each local player.",
            179, 385, 420, 9, muted);
    }
    void chat()
    {
        const NativeChatView* view = nativeChat_View();
        char label[80];
        std::snprintf(label, sizeof(label), "Global chat - %u online",
                      view->online);
        title("Chatroom", label);
        nativeHub_Panel(175, 116, 296, 234);
        nativeHub_Panel(479, 116, 126, 234);
        nativeHubPaint_Text("Recent speakers", 489, 127, 103, 12, gold,
                            NativeMenuFont_Heading);
        unsigned count = view->count;
        unsigned end =
            count > unsigned(hub.chat_scroll) ? count - hub.chat_scroll : 0;
        unsigned start = end > 8 ? end - 8 : 0;
        float bottom = 340;
        for (unsigned i = end; i > start; --i) {
            const NativeChatMessage& msg = view->messages[i - 1];
            MessageLines lines = wrap_message(msg.text);
            float top = bottom - 16 - lines.count * 11;
            if (top < 126) {
                break;
            }
            nativeHubPaint_Text(msg.name, 186, top, 270, 9, gold);
            for (unsigned row = 0; row < lines.count; ++row) {
                nativeHubPaint_Text(lines.lines[row], 186, top + 14 + row * 11,
                                    270, 8);
            }
            bottom = top - 5;
        }
        if (!count) {
            nativeHubPaint_Text(view->state == NativeChat_Online
                                    ? "Be the first to say hello."
                                    : "Connecting to global chat...",
                                186, 139, 272, 10, muted);
        }
        // The service currently reports a count and message authors, rather
        // than a live directory. Show the authors that actually appear in this
        // history.
        char seen[12][33]{};
        unsigned seen_count = 0;
        for (unsigned i = count; i > 0 && seen_count < 12; --i) {
            const auto& msg = view->messages[i - 1];
            bool found = false;
            for (unsigned j = 0; j < seen_count; ++j) {
                if (!std::strcmp(seen[j], msg.peer)) {
                    found = true;
                }
            }
            if (found) {
                continue;
            }
            std::snprintf(seen[seen_count], sizeof(seen[0]), "%s", msg.peer);
            nativeHub_Button(
                "chat-author", msg.name, 487, 149 + seen_count * 15, 108, 14,
                HubAction::ShowProfile, 0, false, true, msg.peer, msg.name);
            ++seen_count;
        }
        nativeHubPaint_Text("Recent speakers", 489, 331, 103, 7, muted);
        nativeHub_Field("chat-message", hub.chat, 175, 362, 328, 28,
                        HubAction::FieldChat);
        nativeHub_Button("send-chat", "Send", 512, 362, 93, 28,
                         HubAction::SendChat, 0, true,
                         *hub.chat && view->state == NativeChat_Online);
        const char* status = view->state == NativeChat_Online ? "Connected"
                             : view->state == NativeChat_Offline
                                 ? "Offline"
                                 : "Connecting...";
        if (view->delivery == NativeChatDelivery_Sending) {
            status = "Sending...";
        }
        if (view->delivery == NativeChatDelivery_Unconfirmed) {
            status = "Delivery is unconfirmed.";
        }
        nativeHubPaint_Text(status, 181, 401, 416, 8, muted);
    }
    void settings()
    {
        title("Settings", "Your controls and profile.");
        nativeHub_Panel(175, 116, 430, 256);
        nativeHub_Button("controls", "Controls & bindings", 193, 135, 392, 36,
                         HubAction::Controls, 0, true);
        nativeHubPaint_Text("Display name", 194, 196, 360, 10, gold);
        nativeHub_Field("display-name", hub.name, 193, 216, 392, 27,
                        HubAction::FieldName);
        nativeHub_Button("edit-profile", "Your profile", 193, 264, 187, 31,
                         HubAction::Profile);
        nativeHub_Button("save-profile", "Save settings", 391, 264, 194, 31,
                         HubAction::SaveProfile, 0, true, *hub.name);
    }
    void profile()
    {
        title(hub.profile_self ? hub.name : hub.viewed_name,
              hub.profile_self ? "Your profile" : "Player profile");
        nativeHub_Panel(175, 118, 430, 240);
        nativeHubPaint_Text("About", 194, 138, 370, 15, gold,
                            NativeMenuFont_Heading);
        if (hub.profile_self) {
            nativeHub_Field("profile-bio", hub.bio, 194, 171, 390, 68,
                            HubAction::FieldBio);
            nativeHub_Button("save-bio", "Save profile", 194, 263, 165, 30,
                             HubAction::SaveProfile, 0, true);
        } else {
            nativeHubPaint_Text("This player has not shared a bio.", 194, 175,
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
        title(inbox ? "Inbox" : "Friends",
              inbox ? "Letters, friend requests and invitations."
                    : "Find your people. Invite them to play.");
        nativeHub_Panel(175, 119, 430, 226);
        nativeHubPaint_Text(inbox ? "Your mailbox" : "Your friends", 194, 143,
                            370, 20, gold, NativeMenuFont_Heading);
        nativeHubPaint_Text(
            "Account sign-in and offline delivery are being connected.", 194,
            187, 382, 10, muted);
        nativeHubPaint_Text("Global chat and lobbies are available now.", 194,
                            211, 382, 10);
        nativeHub_Button("go-chat", "Open Chatroom", 194, 263, 172, 32,
                         HubAction::Chat, 0, true);
        nativeHub_Button("go-lobbies", "Open Lobbies", 379, 263, 205, 32,
                         HubAction::Lobbies);
    }
    void send()
    {
        title("Send a fighter", "Choose someone in your lobby.");
        nativeHub_Panel(175, 117, 430, 252);
        nativeHubPaint_Text(hub.gift_title, 191, 136, 390, 18, gold,
                            NativeMenuFont_Heading);
        const auto* p = nativeOnlineSession_Presence();
        unsigned rows = 0;
        if (p) {
            for (unsigned i = 0; i < p->participant_count && rows < 7; ++i) {
                const auto& person = p->participants[i];
                if (!std::strcmp(person.peer_id,
                                 nativeOnlineSession_LocalPeerId()))
                {
                    continue;
                }
                nativeHub_Button("gift-recipient", person.name, 191,
                                 172 + rows * 24, 394, 21,
                                 HubAction::Recipient, int(i),
                                 !std::strcmp(hub.recipient, person.peer_id),
                                 true, person.peer_id, person.name);
                ++rows;
            }
        }
        if (!rows) {
            nativeHubPaint_Text(
                "Join a lobby with another player to send your fighter.", 191,
                193, 393, 11, muted);
        }
        nativeHub_Button("confirm-gift", "Send fighter", 382, 376, 223, 29,
                         HubAction::ConfirmSend, 0, true, *hub.recipient);
    }
}
void nativeHub_RenderPage(void)
{
    switch (hub.page) {
    case HubPage::Workshop:
        collection();
        break;
    case HubPage::Clone:
        clone_page();
        break;
    case HubPage::QuickPlay:
        quick_play();
        break;
    case HubPage::Lobbies:
        lobbies();
        break;
    case HubPage::Chat:
        chat();
        break;
    case HubPage::Friends:
        social(false);
        break;
    case HubPage::Inbox:
        social(true);
        break;
    case HubPage::Settings:
        settings();
        break;
    case HubPage::Profile:
        profile();
        break;
    case HubPage::Send:
        send();
        break;
    }
}
