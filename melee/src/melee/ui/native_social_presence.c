#include "native_social_presence.h"
#include "native_chat.h"
#include <melee/gm/native_signaling_wire.h>
#include <string.h>

NativeSocialPresence nativeSocialPresence_Account(const NativeChatView* chat,
    const NativeSignalingPresence* lobby, const char* account_id)
{
    if (!account_id || !*account_id) return NativeSocialPresence_Unknown;
    if (lobby) {
        for (unsigned i = 0; i < lobby->participant_count; ++i)
            if (!strcmp(account_id, lobby->participants[i].account_id))
                return NativeSocialPresence_Online;
    }
    if (!chat || chat->state != NativeChat_Online || !chat->presence_ready)
        return NativeSocialPresence_Unknown;
    for (unsigned i = 0; i < chat->player_count; ++i)
        if (!strcmp(account_id, chat->players[i].account_id))
            return NativeSocialPresence_Online;
    return NativeSocialPresence_Offline;
}
