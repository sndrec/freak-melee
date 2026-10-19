#include <melee/ui/native_social_presence.h>
#include <melee/ui/native_chat.h>
#include <melee/gm/native_signaling_wire.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    static NativeChatView chat;
    static NativeSignalingPresence lobby;
    const char* account = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
    assert(nativeSocialPresence_Account(NULL, NULL, account) == NativeSocialPresence_Unknown);
    chat.state = NativeChat_Online;
    chat.online = 2;
    assert(nativeSocialPresence_Account(&chat, NULL, account) == NativeSocialPresence_Unknown);
    chat.presence_ready = true;
    assert(nativeSocialPresence_Account(&chat, NULL, account) == NativeSocialPresence_Offline);
    chat.player_count = 1;
    strcpy(chat.players[0].name, "Twilight");
    strcpy(chat.players[0].account_handle, "twilight");
    // A guest's matching name/handle cannot supply an account identity.
    assert(nativeSocialPresence_Account(&chat, NULL, account) == NativeSocialPresence_Offline);
    strcpy(chat.players[0].account_id, account);
    assert(nativeSocialPresence_Account(&chat, NULL, account) == NativeSocialPresence_Online);
    chat.player_count = 2; chat.players[1] = chat.players[0];
    chat.players[0].account_id[0] = 0;
    assert(nativeSocialPresence_Account(&chat, NULL, account) == NativeSocialPresence_Online);
    chat.state = NativeChat_Reconnecting;
    assert(nativeSocialPresence_Account(&chat, NULL, account) == NativeSocialPresence_Unknown);
    lobby.participant_count = 1;
    strcpy(lobby.participants[0].account_id, account);
    assert(nativeSocialPresence_Account(&chat, &lobby, account) == NativeSocialPresence_Online);
    assert(nativeSocialPresence_Account(&chat, &lobby, "") == NativeSocialPresence_Unknown);
    assert(nativeSocialPresence_Account(&chat, &lobby, NULL) == NativeSocialPresence_Unknown);
    lobby.participant_count = 0; chat.state = NativeChat_Online;
    chat.presence_ready = false;
    assert(nativeSocialPresence_Account(&chat, &lobby, account) == NativeSocialPresence_Unknown);
    chat.player_count = 0; chat.online = 0; chat.presence_ready = true;
    assert(nativeSocialPresence_Account(&chat, &lobby, account) == NativeSocialPresence_Offline);
    puts("social presence: complete snapshots, reconnects, guest identities, multiple sessions and lobby presence passed");
    return 0;
}
