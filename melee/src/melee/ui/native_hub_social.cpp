#include "native_hub_social.h"
#include "native_hub_account.h"
#include "native_hub_lobbies.h"
#include "native_social_presence.h"
#include <melee/ft/native_fighter_runtime.h>
#include <melee/gm/native_home.h>
#include <melee/gm/native_online_session.h>
#include <melee/mn/native_online_menu.h>
#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace {
    unsigned folder, offset, friends_offset, read_scroll, read_lines;
    uint64_t refreshed;
    bool wanted;
    char owner[44], opened[44], nonce[44], invitation_nonce[44], invitation_room[13], invitation_to[25], pending_join[44];
    unsigned join_revision;
    unsigned fighter_request_revision;
    char fighter_nonce[44], sent_fighter[65], sent_friend[25], download_id[44], accepted_gift[44];
    bool sending_fighter;
    constexpr uint32_t gold = 0xD9B754FF, muted = 0xB9ADBDFF;

    const char* presence_label(const char* account_id)
    {
        static const char* labels[] = { "Checking...", "Offline", "Online" };
        return labels[nativeSocialPresence_Account(nativeChat_View(), nativeOnlineSession_Presence(), account_id)];
    }

    bool can_invite(const NativeAccountView& account)
    {
        const auto* presence = nativeOnlineSession_Presence();
        const char* peer = nativeOnlineSession_LocalPeerId();
        if (!account.signed_in || !presence || !peer || !nativeOnlineSession_RoomCode()) return false;
        for (unsigned i = 0; i < presence->participant_count; ++i)
            if (!std::strcmp(peer, presence->participants[i].peer_id))
                return !std::strcmp(account.id, presence->participants[i].account_id);
        return false;
    }

    bool can_create()
    {
        const auto state = nativeOnlineSession_State();
        return nativeOnlineMenu_IsAvailable() && !nativeOnlineSession_Quick() &&
            (state == NativeLobbySessionState_Disconnected || state == NativeLobbySessionState_Failed);
    }

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
        if (operation == NativeAccountOperation::SendInvitation) {
            const char* room = nativeOnlineSession_RoomCode();
            if (!room || !*room || !can_invite(nativeHubAccount_View())) { nativeHub_Notice("Join a lobby with your account before inviting a friend."); return false; }
            if (std::strcmp(invitation_room, room) || std::strcmp(invitation_to, handle)) {
                invitation_nonce[0] = 0;
                std::snprintf(invitation_room, sizeof(invitation_room), "%s", room);
                std::snprintf(invitation_to, sizeof(invitation_to), "%s", handle);
            }
            if (!*invitation_nonce && !nativeAccount_NewNonce(invitation_nonce)) { nativeHub_Notice("Could not prepare the invitation. Try again."); return false; }
            std::snprintf(request.room, sizeof(request.room), "%s", invitation_room);
            std::snprintf(request.nonce, sizeof(request.nonce), "%s", invitation_nonce);
        }
        const bool ok = nativeAccount_Submit(request);
        if (ok && operation == NativeAccountOperation::SendLetter)
            nativeHub_ChangePage(hub.page);
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
        SDL_memset(invitation_nonce, 0, sizeof(invitation_nonce)); pending_join[0] = 0;
        hub.lobby_invite_to[0] = 0; hub.lobby_invite_queued = false;
        std::snprintf(owner, sizeof(owner), "%s", account.signed_in ? account.id : "");
        wanted = true; folder = offset = friends_offset = read_scroll = 0;
        fighter_nonce[0] = sent_fighter[0] = sent_friend[0] = download_id[0] = accepted_gift[0] = 0;
        sending_fighter = false;
    }
    if (*accepted_gift && !account.busy) {
        if (submit(NativeAccountOperation::DeleteLetter, nullptr, accepted_gift)) accepted_gift[0] = 0;
        return;
    }
    if (!account.busy && *nonce && !std::strcmp(nonce, account.social.last_sent_nonce)) {
        clear_draft(); folder = 1; offset = 0; wanted = true;
        if (hub.page == NativeHubPage_ComposeLetter) nativeHub_ChangePage(NativeHubPage_Inbox);
    }
    if (!account.busy && *invitation_nonce && !std::strcmp(invitation_nonce, account.social.last_sent_nonce)) {
        invitation_nonce[0] = 0;
        nativeHub_Notice("Lobby invitation sent.");
    }
    if (!account.busy && sending_fighter && account.revision != fighter_request_revision) {
        sending_fighter = false;
        nativeHub_Notice(account.status);
        if (!std::strcmp(fighter_nonce, account.social.last_sent_nonce)) {
            fighter_nonce[0] = 0;
            if (hub.page == NativeHubPage_Send) nativeHub_ChangePage(NativeHubPage_Workshop);
        }
    }
    if (hub.lobby_invite_queued) {
        const auto state = nativeOnlineSession_State();
        const auto* listing = nativeOnlineSession_Listing();
        if (!account.signed_in || nativeOnlineSession_Quick() || state == NativeLobbySessionState_Failed ||
            (state == NativeLobbySessionState_Disconnected && !nativeOnlineMenu_HasPendingAction())) {
            hub.lobby_invite_to[0] = 0; hub.lobby_invite_queued = false;
            nativeHub_Notice("The lobby was not created. The invitation was not sent.");
        } else if (!account.busy && can_invite(account) && nativeOnlineSession_IsOwner() && !hub.pending_listing && listing &&
            !std::strcmp(listing->title, hub.lobby_title) && listing->listed == hub.lobby_listed &&
            submit(NativeAccountOperation::SendInvitation, hub.lobby_invite_to)) {
            hub.lobby_invite_to[0] = 0; hub.lobby_invite_queued = false;
        }
    }
    if (!account.busy && *pending_join && account.revision != join_revision) {
        const bool checked = !std::strcmp(pending_join, account.social.join_id) && *account.social.join_room;
        pending_join[0] = 0;
        if (checked && nativeOnlineMenu_IsAvailable()) {
            if (nativeOnlineMenu_Request(NativeOnlineMenuAction_Join, hub.public_name, account.social.join_room, uint8_t(hub.local_players), nullptr)) {
                nativeOnlineSession_Stop(); hub.pending_listing = false;
                std::snprintf(hub.code, sizeof(hub.code), "%s", account.social.join_room);
                nativeHub_ChangePage(NativeHubPage_Lobbies);
            } else nativeHub_Notice("Could not join the lobby. Please try again.");
        } else nativeHub_Notice(checked ? "Return to character select or practice to join this lobby." : account.status);
    }
    const auto& social = account.social;
    if (!account.busy && social.loaded) {
        if (social.folder == folder && social.offset == offset && offset && offset >= social.total) {
            offset = social.total ? ((social.total - 1) / 8) * 8 : 0;
            wanted = true;
        }
        if (social.friends_offset == friends_offset && friends_offset && friends_offset >= social.friends_total) {
            friends_offset = social.friends_total ? ((social.friends_total - 1) / 8) * 8 : 0;
            wanted = true;
        }
    }
    const bool picking_friend = hub.page == NativeHubPage_Send && hub.send_to_friends;
    if (account.signed_in && !account.busy && hub.visible &&
        (nativeHubSocial_Page(hub.page) || picking_friend) &&
        hub.page != NativeHubPage_ComposeLetter && hub.page != NativeHubPage_SocialProfile && (wanted || SDL_GetTicks() - refreshed >= 10000)) {
        wanted = !submit(NativeAccountOperation::RefreshSocial);
        refreshed = SDL_GetTicks();
    }
}

void nativeHubSocial_FighterRecipients()
{
    const auto& account = nativeHubAccount_View();
    const auto& social = account.social;
    const bool ready = account.signed_in && !account.busy && social.loaded &&
                       social.friends_offset == friends_offset;
    unsigned rows = 0;
    for (unsigned i = 0; ready && i < social.friend_count; ++i) {
        const auto& entry = social.friends[i];
        if (std::strcmp(entry.state, "friend")) continue;
        char id[32], label[86];
        std::snprintf(id,sizeof(id),"gift-friend-%u",i);
        std::snprintf(label,sizeof(label),"%s (@%s)",entry.name,entry.handle);
        nativeHub_Button(id,label,191,178 + rows++*22,394,20,HubAction::FriendGiftRecipient,
                         0,!std::strcmp(entry.handle,hub.fighter_gift_friend),true,
                         nullptr,entry.name,entry.handle);
    }
    if (!rows) nativeMenuPaint_Text(account.busy || !social.loaded ? "Loading friends…" :
                                    "Add someone in Friends to send them a fighter.",191,194,394,11,muted);
    nativeHub_Button("gift-friends-prev","<",538,151,22,20,HubAction::FriendGiftPage,-8,false,
                     ready && friends_offset > 0);
    nativeHub_Button("gift-friends-next",">",567,151,22,20,HubAction::FriendGiftPage,8,false,
                     ready && friends_offset + 8 < social.friends_total);
    nativeHub_Button("confirm-gift",account.busy && sending_fighter ? "Sending fighter…" : "Send fighter",
                     382,376,223,29,HubAction::ConfirmSend,0,true,ready && *hub.fighter_gift_friend);
}

bool nativeHubSocial_TakeFighter(NativeContentId* id)
{
    if (!*download_id || !nativeHome_IsSelection()) return false;
    const auto& account = nativeHubAccount_View();
    if (account.busy || account.revision == fighter_request_revision) return false;
    const bool downloaded = account.signed_in && !std::strcmp(download_id,account.social.fighter_download_id);
    char root[512];
    if (downloaded && nativeContentStore_DefaultRoot(root,sizeof(root)) &&
        nativeContentStore_IdFromHex(account.social.fighter_download_hash,id) &&
        nativeFighterRuntime_ValidateCached(id,root) && nativeContentStore_Install(root,id)) {
        std::memcpy(accepted_gift, download_id, sizeof(accepted_gift));
        if (hub.page == NativeHubPage_Letter && !std::strcmp(opened, download_id))
            nativeHub_ChangePage(NativeHubPage_Inbox);
        download_id[0] = 0;
        return true;
    }
    download_id[0] = 0;
    nativeHub_Notice(downloaded ? "This fighter could not be validated or installed." : account.status);
    return false;
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
        char handle[64]; std::snprintf(handle, sizeof(handle), "@%s | %s", social.profile.handle, presence_label(social.profile.id));
        nativeMenuPaint_Text(handle, 194, 161, 370, 10, muted);
        nativeHub_TextBlock(*social.profile.bio ? social.profile.bio : "This player has not shared a bio.", 194, 192, 370, 100, 11, muted);
        const bool self = !std::strcmp(account.id, social.profile.id);
        const char* state = social.relationship;
        nativeHub_Button("profile-letter", "Write a letter", 194, 307, 187, 30, HubAction::WriteLetter, 0, true, enabled && !self && !social.profile_blocked);
        const bool friends = !std::strcmp(state, "friend");
        if (friends) {
            nativeHub_Button("profile-invite", can_create() ? "Create & invite" : "Invite to lobby", 395, 307, 189, 30,
                HubAction::SendInvitation, 0, true, enabled && !self && !social.profile_blocked && (can_invite(account) || can_create()));
        }
        nativeHub_Button("profile-friend", !std::strcmp(state, "incoming") ? "Accept friend request" : !std::strcmp(state, "outgoing") ? "Cancel friend request" :
            friends ? "Remove friend" : "Add friend", 395, friends ? 354 : 307, 189, friends ? 25 : 30,
            HubAction::SocialFriend, 0, false, enabled && !self && !social.profile_blocked);
        nativeHub_Button("profile-block", social.profile_blocked ? "Unblock player" : "Block player", 194, 354, 187, 25,
            HubAction::SocialBlock, 0, false, enabled && !self);
        return;
    }
    if (page == NativeHubPage_Letter) {
        nativeHub_Panel(175, 116, 430, 284);
        const auto& message = social.opened;
        if (!*opened || std::strcmp(opened, message.id)) {
            nativeMenuPaint_Text(account.busy ? "Opening…" : "This message could not be opened.", 194, 148, 370, 11, muted); return;
        }
        nativeMenuPaint_Text(message.title, 194, 131, 390, 17, gold, NativeMenuFont_MenuOption);
        char from[128]; std::snprintf(from, sizeof(from), "%s %s (@%s)", message.folder ? "To" : "From", message.name, message.handle);
        nativeMenuPaint_Text(from, 194, 156, 390, 9, muted);
        if (!std::strcmp(message.kind,"fighter_gift")) {
            nativeMenuPaint_Text("Fighter delivery",194,191,390,13,muted);
            nativeHub_Button("install-friend-fighter",account.busy ? "Receiving fighter…" : "Install fighter",
                             194,359,170,28,HubAction::InstallFriendGift,0,true,enabled && nativeHome_IsSelection());
            nativeHub_Button("delete-friend-fighter","Remove delivery",415,359,169,28,HubAction::DeleteLetter,0,false,enabled);
            return;
        }
        read_lines = nativeHub_TextBlock(social.body, 194, 189, 390, 126, 11, 0xF2ECEDFF, read_scroll);
        nativeHub_Button("letter-up", "Up", 506, 320, 35, 22, HubAction::MailPage, -1, false, read_scroll > 0);
        nativeHub_Button("letter-down", "Down", 549, 320, 35, 22, HubAction::MailPage, 1, false, read_scroll + 9 < read_lines);
        bool request = !std::strcmp(message.kind, "friend_request") && !std::strcmp(message.status, "pending");
        bool invitation = !std::strcmp(message.kind, "room_invite");
        SDL_Time now = 0; SDL_GetCurrentTime(&now);
        bool expired = invitation && message.expires <= uint64_t(now / SDL_NS_PER_MS);
        if (invitation) nativeMenuPaint_Text(expired ? "Invitation expired" : "Invitation lasts 15 minutes", 194, 327, 298, 9, muted);
        nativeHub_Button("letter-reply", request ? "Accept" : invitation ? "Join lobby" : "Reply", 194, 359, 120, 28,
            request ? HubAction::SocialFriend : invitation ? HubAction::JoinInvitation : HubAction::ReplyLetter, request ? 1 : 0, true,
            enabled && !message.folder && !expired && (!invitation || nativeOnlineMenu_IsAvailable()));
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
    nativeHub_Panel(175, 152, 430, 216);
    const bool friends = page == NativeHubPage_Friends;
    if (!social.loaded || (friends ? social.friends_offset != friends_offset : social.folder != folder || social.offset != offset)) {
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
                    !std::strcmp(entry.state, "outgoing") ? "Request sent" : presence_label(entry.id));
            } else {
                const auto& entry = social.messages[i];
                std::snprintf(title, sizeof(title), "%s%s", entry.unread ? "* " : "", entry.title);
                std::snprintf(detail, sizeof(detail), "%s %s | %s", folder ? "To" : "From", entry.name,
                              !std::strcmp(entry.kind,"fighter_gift") ? "Fighter delivery" : entry.status);
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
    char count[96]; std::snprintf(count, sizeof(count), "%u %s%s", total,
        friends ? (total == 1 ? "contact" : "contacts") : (total == 1 ? "letter" : "letters"), friends || !social.unread ? "" : " | unread mail");
    nativeMenuPaint_Text(count, 181, 378, 240, 9, muted);
    nativeHub_Button("social-refresh", "Refresh", 431, 373, 82, 22, HubAction::SocialRefresh, 0, false, enabled);
    nativeHub_Button("social-prev", "<", 523, 373, 35, 22, friends ? HubAction::FriendPage : HubAction::MailPage, -8, false, enabled && position > 0);
    nativeHub_Button("social-next", ">", 568, 373, 35, 22, friends ? HubAction::FriendPage : HubAction::MailPage, 8, false, enabled && position + 8 < total);
}
bool nativeHubSocial_Activate(HubAction action, int value)
{
    const auto& account = nativeHubAccount_View(); const auto& social = account.social;
    switch (action) {
    case HubAction::GiftRecipientMode:
        hub.send_to_friends = value != 0;
        if (hub.send_to_friends) { friends_offset = 0; wanted = true; }
        return true;
    case HubAction::FriendGiftRecipient:
        std::snprintf(hub.fighter_gift_friend,sizeof(hub.fighter_gift_friend),"%s",hub.pending_account);
        return true;
    case HubAction::FriendGiftPage:
        friends_offset = unsigned(std::max(0,int(friends_offset) + value)); wanted = true; return true;
    case HubAction::ConfirmSend: {
        if (!hub.send_to_friends || hub.send_from_css) return false;
        if (!account.signed_in || account.busy || !*hub.fighter_gift_friend) return true;
        NativeAccountRequest request{};
        request.operation = NativeAccountOperation::SendFighterGift;
        nativeContentStore_IdHex(&hub.gift_id,request.fighter_hash);
        if (std::strcmp(sent_fighter,request.fighter_hash) || std::strcmp(sent_friend,hub.fighter_gift_friend)) {
            fighter_nonce[0] = 0;
            std::memcpy(sent_fighter,request.fighter_hash,sizeof(sent_fighter));
            std::memcpy(sent_friend,hub.fighter_gift_friend,sizeof(sent_friend));
        }
        if (!*fighter_nonce && !nativeAccount_NewNonce(fighter_nonce)) {
            nativeHub_Notice("Could not prepare this fighter delivery."); return true;
        }
        std::memcpy(request.handle,hub.fighter_gift_friend,sizeof(request.handle));
        std::memcpy(request.nonce,fighter_nonce,sizeof(request.nonce));
        if (nativeAccount_Submit(request)) {
            sending_fighter = true; fighter_request_revision = account.revision;
        } else nativeHub_Notice("Wait for the account request to finish.");
        return true;
    }
    case HubAction::InstallFriendGift:
        if (nativeHome_IsSelection() && submit(NativeAccountOperation::DownloadFighterGift,nullptr,opened)) {
            std::memcpy(download_id,opened,sizeof(download_id)); fighter_request_revision = account.revision;
        }
        return true;
    case HubAction::OpenAccount:
        if (*hub.pending_account) {
            std::snprintf(hub.social_handle, sizeof(hub.social_handle), "%s", hub.pending_account);
            nativeHub_ChangePage(NativeHubPage_SocialProfile);
            submit(NativeAccountOperation::LookupProfile, hub.social_handle);
        }
        return true;
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
    case HubAction::ReplyLetter: compose(social.opened.handle, social.opened.title); return true;
    case HubAction::SendLetter:
        folder = 1; offset = 0; submit(NativeAccountOperation::SendLetter); return true;
    case HubAction::SendInvitation:
        if (!account.signed_in || account.busy || social.profile_blocked ||
            std::strcmp(social.relationship, "friend") || !*social.profile.handle || !std::strcmp(account.id, social.profile.id)) return true;
        if (can_create()) {
            nativeHubLobbies_Activate(HubAction::NewLobby, 0);
            std::snprintf(hub.lobby_invite_to, sizeof(hub.lobby_invite_to), "%s", social.profile.handle);
        } else submit(NativeAccountOperation::SendInvitation, social.profile.handle);
        return true;
    case HubAction::JoinInvitation:
        if (nativeOnlineMenu_IsAvailable() && submit(NativeAccountOperation::JoinInvitation, nullptr, opened)) {
            hub.lobby_invite_to[0] = 0; hub.lobby_invite_queued = false;
            std::snprintf(pending_join, sizeof(pending_join), "%s", opened); join_revision = account.revision;
        }
        return true;
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
    std::fprintf(stderr, "[hub-social] signed-in=%d busy=%d loaded=%d folder=%u offset=%u count=%u total=%u unread=%u friends=%u opened=%d profile=%d profile-handle=%s kind=%s invite-pending=%d create-invite=%s create-queued=%d friends-offset=%u friend-count=%u revision=%u\n",
        account.signed_in, account.busy, social.loaded, social.folder, social.offset, social.count, social.total, social.unread, social.friends_total,
        *opened && !std::strcmp(opened, social.opened.id), *social.profile.id != 0,
        social.profile.handle, social.opened.kind, *invitation_nonce != 0, hub.lobby_invite_to, hub.lobby_invite_queued,
        social.friends_offset, social.friend_count, social.revision);
    for (unsigned i = 0; i < social.friend_count; ++i)
        std::fprintf(stderr, "[hub-friend] handle=%s relationship=%s presence=%s\n", social.friends[i].handle,
            social.friends[i].state, presence_label(social.friends[i].id));
}
void nativeHubSocial_Shutdown()
{
    clear_draft(); SDL_memset(owner, 0, sizeof(owner)); SDL_memset(opened, 0, sizeof(opened));
    SDL_memset(invitation_nonce, 0, sizeof(invitation_nonce)); pending_join[0] = 0;
    invitation_room[0] = invitation_to[0] = 0;
    download_id[0] = accepted_gift[0] = 0;
    hub.lobby_invite_to[0] = 0; hub.lobby_invite_queued = false;
    wanted = false; refreshed = folder = offset = friends_offset = read_scroll = read_lines = 0;
}
