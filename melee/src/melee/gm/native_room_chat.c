#include "native_room_chat.h"
#ifdef MELEE_NATIVE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <stdio.h>
#include <string.h>
#include <windows.h>

void nativeRoomChat_Init(NativeRoomChat* chat, NativeSignalingClient* client)
{
    memset(chat, 0, sizeof(*chat));
    chat->client = client;
}

static void acknowledge(NativeRoomChat* chat, const NativeRoomChatEntry* entry)
{
    if (chat->delivery != NativeRoomChatDelivery_None &&
        entry->nonce == chat->nonce &&
        entry->sequence > chat->pending_after_sequence &&
        !strcmp(entry->peer_id, chat->client->peer_id) &&
        !strcmp(entry->text, chat->pending))
    {
        chat->delivery = NativeRoomChatDelivery_None;
        chat->failure[0] = 0;
    }
}

void nativeRoomChat_Update(NativeRoomChat* chat, bool connected)
{
    uint64_t now = GetTickCount64();
    chat->connected = connected;
    if (!connected) {
        chat->ready = chat->loading = chat->requested = false;
        if (chat->delivery == NativeRoomChatDelivery_Sending) {
            chat->delivery = NativeRoomChatDelivery_Unconfirmed;
        }
        return;
    }
    NativeRoomChatEvent event;
    if (atomic_exchange_explicit(&chat->client->chat.overflow, false,
                                 memory_order_acq_rel))
    {
        // Chat loss never interrupts gameplay signaling. Replace the whole
        // chat view from a new history response instead of displaying a
        // partial one.
        while (nativeSignalingClient_PollChat(chat->client, &event)) {
        }
        chat->ready = chat->loading = chat->requested = false;
    }
    while (nativeSignalingClient_PollChat(chat->client, &event)) {
        switch (event.kind) {
        case NativeRoomChatEvent_Begin:
            chat->expected = event.count;
            chat->staging_count = 0;
            chat->loading = true;
            chat->ready = false;
            break;
        case NativeRoomChatEvent_Entry:
            if (chat->loading) {
                if (chat->staging_count < chat->expected &&
                    (!chat->staging_count ||
                     event.entry.sequence >
                         chat->staging[chat->staging_count - 1].sequence))
                {
                    chat->staging[chat->staging_count++] = event.entry;
                } else {
                    chat->requested = chat->loading = false;
                }
            } else if (chat->ready) {
                acknowledge(chat, &event.entry);
                if (event.entry.sequence <= chat->last_sequence) {
                    break;
                }
                if (chat->count == NATIVE_ROOM_CHAT_HISTORY) {
                    memmove(chat->messages, chat->messages + 1,
                            (NATIVE_ROOM_CHAT_HISTORY - 1) *
                                sizeof(chat->messages[0]));
                    --chat->count;
                }
                chat->messages[chat->count++] = event.entry;
                chat->last_sequence = event.entry.sequence;
            }
            break;
        case NativeRoomChatEvent_End:
            if (chat->loading && chat->staging_count == chat->expected &&
                event.entry.sequence ==
                    (chat->staging_count
                         ? chat->staging[chat->staging_count - 1].sequence
                         : 0))
            {
                memcpy(chat->messages, chat->staging,
                       chat->staging_count * sizeof(chat->messages[0]));
                chat->count = chat->staging_count;
                chat->last_sequence = event.entry.sequence;
                chat->ready = true;
                for (unsigned i = 0; i < chat->count; ++i) {
                    acknowledge(chat, &chat->messages[i]);
                }
            }
            chat->loading = false;
            chat->requested = chat->ready;
            break;
        case NativeRoomChatEvent_Rejected:
            if (event.entry.nonce == chat->nonce &&
                chat->delivery != NativeRoomChatDelivery_None)
            {
                chat->delivery = NativeRoomChatDelivery_Unconfirmed;
                snprintf(chat->failure, sizeof(chat->failure), "%s",
                         event.reason);
            } else if (!event.entry.nonce && !chat->ready) {
                chat->loading = chat->requested = false;
            }
            break;
        }
    }
    if (!chat->ready && chat->requested &&
        now - chat->history_since_ms >= 10000)
    {
        chat->requested = chat->loading = false;
    }
    if (!chat->requested &&
        (!chat->history_since_ms || now - chat->history_since_ms >= 1200) &&
        nativeSignalingClient_RequestChat(chat->client))
    {
        chat->requested = true;
        chat->history_since_ms = now;
    }
    if (chat->delivery == NativeRoomChatDelivery_Sending &&
        now - chat->pending_since_ms >= 10000)
    {
        chat->delivery = NativeRoomChatDelivery_Unconfirmed;
    }
}

bool nativeRoomChat_Post(NativeRoomChat* chat, const char* text)
{
    if (!chat || !chat->ready || !chat->connected ||
        chat->delivery == NativeRoomChatDelivery_Sending)
    {
        return false;
    }
    uint32_t nonce = chat->nonce + 1;
    if (!nonce) {
        nonce = 1;
    }
    if (!nativeSignalingClient_PostChat(chat->client, nonce, text)) {
        return false;
    }
    chat->nonce = nonce;
    snprintf(chat->pending, sizeof(chat->pending), "%s", text);
    chat->pending_after_sequence = chat->last_sequence;
    chat->pending_since_ms = GetTickCount64();
    chat->delivery = NativeRoomChatDelivery_Sending;
    chat->failure[0] = 0;
    return true;
}

bool nativeRoomChat_Retry(NativeRoomChat* chat)
{
    if (!chat || !chat->ready || !chat->connected ||
        chat->delivery != NativeRoomChatDelivery_Unconfirmed ||
        !nativeSignalingClient_PostChat(chat->client, chat->nonce,
                                        chat->pending))
    {
        return false;
    }
    chat->pending_since_ms = GetTickCount64();
    chat->delivery = NativeRoomChatDelivery_Sending;
    chat->failure[0] = 0;
    return true;
}
#endif
