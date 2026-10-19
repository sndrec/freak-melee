#include "native_hub_social.h"
#include "native_hub_account.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace {
    unsigned folder, offset, friends_offset, read_scroll, read_lines;
    uint64_t refreshed;
    bool wanted;
    char owner[44], opened[44], nonce[44];
    constexpr uint32_t gold = 0xD9B754FF, muted = 0xB9ADBDFF;

    void clear_draft()
    {
        SDL_memset(hub.letter_to, 0, sizeof(hub.letter_to));
        SDL_memset(hub.letter_subject, 0, sizeof(hub.letter_subject));
        SDL_memset(hub.letter_body, 0, sizeof(hub.letter_body));
        SDL_memset(nonce, 0, sizeof(nonce));
    }
    bool submit(NativeAccountOperation operation, const char* handle = nullptr, const char* target = nullptr, const char* action = nullptr, bool blocked = false)
    {
        NativeAccountRequest request{}; request.operation = operation;
        request.folder = folder; request.offset = offset; request.friends_offset = friends_offset;
        if (handle) std::snprintf(request.handle, sizeof(request.handle), "%s", handle);
        if (target) std::snprintf(request.target, sizeof(request.target), "%s", target);
        if (action) std::snprintf(request.action, sizeof(request.action), "%s", action);
        request.blocked = blocked;
        if (operation == NativeAccountOperation::SendLetter) {
            if (!*nonce && !nativeAccount_NewNonce(nonce)) { nativeHub_Notice("Could not prepare the letter. Try again."); return false; }
            std::snprintf(request.handle, sizeof(request.handle), "%s", hub.letter_to);
            std::snprintf(request.subject, sizeof(request.subject), "%s", hub.letter_subject);
            std::snprintf(request.text, sizeof(request.text), "%s", hub.letter_body);
            std::snprintf(request.nonce, sizeof(request.nonce), "%s", nonce);
        }
        nativeHub_ChangePage(hub.page);
        const bool ok = nativeAccount_Submit(request);
        if (!ok) nativeHub_Notice("An account request is still running. Please wait.");
        SDL_memset(&request, 0, sizeof(request));
        return ok;
    }
    void compose(const char* to, const char* title)
    {
        if (!*hub.letter_body && !*hub.letter_subject) {
            if (to) std::snprintf(hub.letter_to, sizeof(hub.letter_to), "%s", to);
            if (title) std::snprintf(hub.letter_subject, sizeof(hub.letter_subject), "Re: %.60s", title);
        } else if (to && std::strcmp(to, hub.letter_to)) nativeHub_Notice("Your current draft is still here. Send it or clear it to start another.");
        nativeHub_ChangePage(NativeHubPage_ComposeLetter);
    }
}
bool nativeHubSocial_Page(NativeHubPage page)
{
    return page == NativeHubPage_Friends || page == NativeHubPage_Inbox || page == NativeHubPage_Letter ||
        page == NativeHubPage_ComposeLetter || page == NativeHubPage_SocialProfile;
}
void nativeHubSocial_Tick()
{
    const auto& account = nativeHubAccount_View();
    if ((!account.signed_in && *owner) || (account.signed_in && std::strcmp(owner, account.id))) {
        clear_draft(); SDL_memset(opened, 0, sizeof(opened));
        std::snprintf(owner, sizeof(owner), "%s", account.signed_in ? account.id : "");
        wanted = true; folder = offset = friends_offset = read_scroll = 0;
    }
    if (!account.busy && *nonce && !std::strcmp(nonce, account.social.last_sent_nonce)) {
        clear_draft(); folder = 1; offset = 0; wanted = true;
        if (hub.page == NativeHubPage_ComposeLetter) nativeHub_ChangePage(NativeHubPage_Inbox);
    }
    if (account.signed_in && !account.busy && hub.visible && nativeHubSocial_Page(hub.page) &&
        hub.page != NativeHubPage_ComposeLetter && hub.page != NativeHubPage_SocialProfile && (wanted || SDL_GetTicks() - refreshed >= 10000)) {
        wanted = !submit(NativeAccountOperation::RefreshSocial);
        refreshed = SDL_GetTicks();
    }
}
void nativeHubSocial_Render(NativeHubPage page)
{
    const auto& account = nativeHubAccount_View(); const auto& social = account.social;
    const bool enabled = !account.busy;
    if (!account.signed_in) {
        nativeHub_Panel(175, 118, 430, 225);
        nativeMenuPaint_Text("Sign in to your account", 194, 143, 370, 18, gold, NativeMenuFont_MenuOption);
        nativeHub_TextBlock("Your friends and letters follow your account. Letters can arrive while you are offline.",
            194, 186, 370, 64, 11, muted);
        nativeHub_Button("social-sign-in", "Open account", 194, 274, 390, 32, HubAction::Account, 0, true);
        return;
    }
    if (page == NativeHubPage_ComposeLetter) {
        nativeHub_Panel(175, 116, 430, 284);
        nativeMenuPaint_Text("To (account name)", 194, 130, 370, 10, gold);
        nativeHub_Field("letter-to", hub.letter_to, 194, 146, 390, 26, HubAction::FieldLetterTo);
        nativeMenuPaint_Text("Subject", 194, 184, 370, 10, gold);
        nativeHub_Field("letter-subject", hub.letter_subject, 194, 200, 390, 26, HubAction::FieldLetterSubject);
        nativeHub_Field("letter-body", hub.letter_body, 194, 240, 390, 107, HubAction::FieldLetterBody);
        nativeHub_Button("letter-clear", "Clear draft", 194, 362, 130, 27, HubAction::ComposeLetter, 1, false, enabled);
        nativeHub_Button("letter-send", "Send letter", 338, 362, 246, 27, HubAction::SendLetter, 0, true,
            enabled && *hub.letter_to && *hub.letter_subject && *hub.letter_body);
        return;
    }
    if (page == NativeHubPage_SocialProfile) {
        nativeHub_Panel(175, 116, 430, 284);
        if (!*social.profile.id || std::strcmp(social.profile.handle, hub.social_handle)) {
            nativeMenuPaint_Text(account.busy ? "Loading player profile..." : "Enter an account name in Friends to find a player.", 194, 148, 370, 11, muted);
            return;
        }
        nativeMenuPaint_Text(social.profile.name, 194, 134, 370, 19, gold, NativeMenuFont_MenuOption);
        char handle[64]; std::snprintf(handle, sizeof(handle), "@%s", social.profile.handle);
        nativeMenuPaint_Text(handle, 194, 161, 370, 10, muted);
        nativeHub_TextBlock(*social.profile.bio ? social.profile.bio : "This player has not shared a bio.", 194, 192, 370, 100, 11, muted);
        const bool self = !std::strcmp(account.id, social.profile.id);
        const char* state = social.relationship;
        nativeHub_Button("profile-letter", "Write a letter", 194, 307, 187, 30, HubAction::WriteLetter, 0, true, enabled && !self);
        nativeHub_Button("profile-friend", !std::strcmp(state, "incoming") ? "Accept friend request" : !std::strcmp(state, "outgoing") ? "Cancel friend request" :
            !std::strcmp(state, "friend") ? "Remove friend" : "Add friend", 395, 307, 189, 30, HubAction::SocialFriend, 0, false, enabled && !self && !social.profile_blocked);
        nativeHub_Button("profile-block", social.profile_blocked ? "Unblock player" : "Block player", 194, 354, 187, 25,
            HubAction::SocialBlock, 0, false, enabled && !self);
        return;
    }
    if (page == NativeHubPage_Letter) {
        nativeHub_Panel(175, 116, 430, 284);
        const auto& message = social.opened;
        if (!*opened || std::strcmp(opened, message.id)) {
            nativeMenuPaint_Text(account.busy ? "Opening letter..." : "This letter could not be opened.", 194, 148, 370, 11, muted); return;
        }
        nativeMenuPaint_Text(message.title, 194, 131, 390, 17, gold, NativeMenuFont_MenuOption);
        char from[128]; std::snprintf(from, sizeof(from), "%s %s (@%s)", message.folder ? "To" : "From", message.name, message.handle);
        nativeMenuPaint_Text(from, 194, 156, 390, 9, muted);
        read_lines = nativeHub_TextBlock(social.body, 194, 189, 390, 126, 11, 0xF2ECEDFF, read_scroll);
        nativeHub_Button("letter-up", "Up", 506, 320, 35, 22, HubAction::MailPage, -1, false, read_scroll > 0);
        nativeHub_Button("letter-down", "Down", 549, 320, 35, 22, HubAction::MailPage, 1, false, read_scroll + 9 < read_lines);
        bool request = !std::strcmp(message.kind, "friend_request") && !std::strcmp(message.status, "pending");
        nativeHub_Button("letter-reply", request ? "Accept" : "Reply", 194, 359, 120, 28,
            request ? HubAction::SocialFriend : HubAction::ReplyLetter, request ? 1 : 0, true, enabled && !message.folder);
        if (request) nativeHub_Button("letter-decline", "Decline", 325, 359, 120, 28, HubAction::SocialFriend, 2, false, enabled);
        nativeHub_Button("letter-delete", "Delete letter", 459, 359, 125, 28, HubAction::DeleteLetter, 0, false, enabled);
        return;
    }
    if (page == NativeHubPage_Friends) {
        nativeHub_Field("social-account-name", hub.social_handle, 175, 116, 310, 27, HubAction::FieldSocialHandle);
        nativeHub_Button("find-account", "Find account", 494, 116, 111, 27, HubAction::SocialLookup, 0, true, enabled && *hub.social_handle);
    } else {
        nativeHub_Button("mail-inbox", "Inbox", 175, 116, 103, 27, HubAction::MailFolder, 0, folder == 0);
        nativeHub_Button("mail-sent", "Sent", 288, 116, 103, 27, HubAction::MailFolder, 1, folder == 1);
        nativeHub_Button("new-letter", "Write a letter", 403, 116, 202, 27, HubAction::ComposeLetter, 0, true, enabled);
    }
    nativeHub_Panel(175, 152, 430, 226);
    const bool friends = page == NativeHubPage_Friends;
    if (!social.loaded || (!friends && (social.folder != folder || social.offset != offset))) {
        nativeMenuPaint_Text(account.busy ? "Loading..." : "Refresh to load this page.", 194, 178, 370, 11, muted);
    } else {
        unsigned count = friends ? social.friend_count : social.count;
        for (unsigned i = 0; i < count; ++i) {
            float y = 162 + i * 25; char id[40], title[192], detail[96];
            std::snprintf(id, sizeof(id), friends ? "social-friend-%u" : "mail-entry-%u", i);
            if (friends) {
                const auto& entry = social.friends[i];
                std::snprintf(title, sizeof(title), "%s", entry.name);
                std::snprintf(detail, sizeof(detail), "@%s | %s", entry.handle, !std::strcmp(entry.state, "incoming") ? "Friend request" :
                    !std::strcmp(entry.state, "outgoing") ? "Request sent" : "Friend");
            } else {
                const auto& entry = social.messages[i];
                std::snprintf(title, sizeof(title), "%s%s", entry.unread ? "* " : "", entry.title);
                std::snprintf(detail, sizeof(detail), "%s %s | %s", folder ? "To" : "From", entry.name, entry.status);
            }
            bool hover = nativeHub_Hover(185, y - 2, 409, 25) && enabled;
            if (hover) nativeMenuPaint_Rect(185, y - 2, 409, 25, 0x72573055);
            nativeMenuPaint_Text(title, 194, y, 387, 11, gold);
            nativeMenuPaint_Text(detail, 194, y + 12, 387, 8, muted);
            nativeHub_Region(id, 185, y - 2, 409, 25, friends ? HubAction::SocialLookup : HubAction::OpenLetter, friends ? i + 1 : i, enabled);
        }
        if (!count) nativeMenuPaint_Text(friends ? "Find someone by account name to add a friend." : folder ? "Your sent letters appear here." : "Your mailbox is empty.",
            194, 183, 370, 11, muted);
    }
    unsigned position = friends ? friends_offset : offset, total = friends ? social.friends_total : social.total;
    char count[96]; std::snprintf(count, sizeof(count), "%u %s%s", total, friends ? "contacts" : "letters", friends || !social.unread ? "" : " | unread mail");
    nativeMenuPaint_Text(count, 181, 389, 255, 9, muted);
    nativeHub_Button("social-refresh", "Refresh", 431, 384, 82, 22, HubAction::SocialRefresh, 0, false, enabled);
    nativeHub_Button("social-prev", "<", 523, 384, 35, 22, friends ? HubAction::FriendPage : HubAction::MailPage, -8, false, enabled && position > 0);
    nativeHub_Button("social-next", ">", 568, 384, 35, 22, friends ? HubAction::FriendPage : HubAction::MailPage, 8, false, enabled && position + 8 < total);
}
bool nativeHubSocial_Activate(HubAction action, int value)
{
    const auto& account = nativeHubAccount_View(); const auto& social = account.social;
    switch (action) {
    case HubAction::Friends: case HubAction::Inbox:
        nativeHub_ChangePage(action == HubAction::Friends ? NativeHubPage_Friends : NativeHubPage_Inbox);
        wanted = true; return true;
    case HubAction::SocialLookup:
        if (value > 0 && unsigned(value) <= social.friend_count) std::snprintf(hub.social_handle, sizeof(hub.social_handle), "%s", social.friends[value - 1].handle);
        nativeHub_ChangePage(NativeHubPage_SocialProfile);
        submit(NativeAccountOperation::LookupProfile, hub.social_handle); return true;
    case HubAction::ComposeLetter:
        if (value) clear_draft();
        compose(nullptr, nullptr); return true;
    case HubAction::WriteLetter:
        if (hub.page != NativeHubPage_SocialProfile) { nativeHub_Notice("Find this player's account in Friends to write a letter."); return true; }
        compose(social.profile.handle, nullptr); return true;
    case HubAction::AddFriend:
        nativeHub_ChangePage(NativeHubPage_Friends); wanted = true;
        nativeHub_Notice("Find this player's account name. Display names can be different."); return true;
    case HubAction::ReplyLetter: compose(social.opened.handle, social.opened.title); return true;
    case HubAction::SendLetter:
        folder = 1; offset = 0; submit(NativeAccountOperation::SendLetter); return true;
    case HubAction::OpenLetter:
        if (value >= 0 && unsigned(value) < social.count) {
            std::snprintf(opened, sizeof(opened), "%s", social.messages[value].id); read_scroll = 0;
            nativeHub_ChangePage(NativeHubPage_Letter); submit(NativeAccountOperation::ReadLetter, nullptr, opened);
        }
        return true;
    case HubAction::DeleteLetter:
        submit(NativeAccountOperation::DeleteLetter, nullptr, opened);
        nativeHub_ChangePage(NativeHubPage_Inbox); return true;
    case HubAction::MailFolder: folder = unsigned(value); offset = 0; wanted = true; return true;
    case HubAction::MailPage:
        if (hub.page == NativeHubPage_Letter) nativeHubSocial_Scroll(value * 4);
        else { offset = unsigned(std::max(0, int(offset) + value)); wanted = true; }
        return true;
    case HubAction::FriendPage: friends_offset = unsigned(std::max(0, int(friends_offset) + value)); wanted = true; return true;
    case HubAction::SocialRefresh: wanted = true; return true;
    case HubAction::SocialFriend:
        if (hub.page == NativeHubPage_Letter) {
            submit(NativeAccountOperation::ChangeFriend, social.opened.handle, nullptr, value == 2 ? "decline" : "accept");
            nativeHub_ChangePage(NativeHubPage_Inbox);
        } else {
            const char* state = social.relationship;
            submit(NativeAccountOperation::ChangeFriend, social.profile.handle, nullptr, !std::strcmp(state, "incoming") ? "accept" :
                !std::strcmp(state, "none") ? "request" : "remove");
        }
        return true;
    case HubAction::SocialBlock:
        submit(NativeAccountOperation::BlockPlayer, social.profile.handle, nullptr, nullptr, !social.profile_blocked); return true;
    default: return false;
    }
}
void nativeHubSocial_Scroll(int amount)
{
    read_scroll = unsigned(std::clamp(int(read_scroll) + amount, 0, std::max(0, int(read_lines) - 9)));
}
void nativeHubSocial_Inspect()
{
    const auto& account = nativeHubAccount_View(); const auto& social = account.social;
    std::fprintf(stderr, "[hub-social] signed-in=%d busy=%d loaded=%d folder=%u offset=%u count=%u total=%u unread=%u friends=%u opened=%d profile=%d\n",
        account.signed_in, account.busy, social.loaded, social.folder, social.offset, social.count, social.total, social.unread, social.friends_total,
        *opened && !std::strcmp(opened, social.opened.id), *social.profile.id != 0);
}
void nativeHubSocial_Shutdown()
{
    clear_draft(); SDL_memset(owner, 0, sizeof(owner)); SDL_memset(opened, 0, sizeof(opened));
    wanted = false; refreshed = folder = offset = friends_offset = read_scroll = read_lines = 0;
}
