#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <melee/gm/native_room_chat.h>

static NativeSignalingClient client;
static NativeRoomChat chat;

static void receive(NativeRoomChatEventKind kind, uint64_t sequence,
                    uint32_t nonce, const char* text, uint8_t count)
{
    uint32_t write = atomic_load(&client.chat.write);
    NativeRoomChatEvent* event =
        &client.chat.slots[write % NATIVE_SIGNALING_CHAT_CAPACITY];
    memset(event, 0, sizeof(*event));
    event->kind = kind;
    event->count = count;
    event->entry.sequence = sequence;
    event->entry.nonce = nonce;
    strcpy(event->entry.peer_id, "local_peer");
    strcpy(event->entry.name, "Tester");
    if (text) {
        strcpy(event->entry.text, text);
    }
    atomic_store(&client.chat.write, write + 1);
}

int main(void)
{
    strcpy(client.peer_id, "local_peer");
    atomic_store(&client.running, true);
    nativeRoomChat_Init(&chat, &client);
    nativeRoomChat_Update(&chat, true);
    assert(chat.requested && !chat.ready);
    receive(NativeRoomChatEvent_Begin, 0, 0, NULL, 1);
    receive(NativeRoomChatEvent_Entry, 10, 9, "Previous message", 0);
    nativeRoomChat_Update(&chat, true);
    assert(chat.loading && !chat.ready && chat.count == 0);
    receive(NativeRoomChatEvent_End, 10, 0, NULL, 0);
    nativeRoomChat_Update(&chat, true);
    assert(chat.ready && chat.count == 1 && chat.last_sequence == 10);

    assert(!nativeRoomChat_Post(&chat, "bad\ntext"));
    assert(nativeRoomChat_Post(&chat, "New message"));
    assert(!nativeRoomChat_Post(&chat, "Cannot replace pending send"));
    assert(chat.delivery == NativeRoomChatDelivery_Sending);
    receive(NativeRoomChatEvent_Entry, 11, chat.nonce, "New message", 0);
    nativeRoomChat_Update(&chat, true);
    assert(chat.count == 2 && chat.delivery == NativeRoomChatDelivery_None);
    receive(NativeRoomChatEvent_Entry, 11, chat.nonce, "New message", 0);
    nativeRoomChat_Update(&chat, true);
    assert(chat.count == 2);

    assert(nativeRoomChat_Post(&chat, "Retry this"));
    nativeRoomChat_Update(&chat, false);
    assert(!chat.ready && chat.delivery == NativeRoomChatDelivery_Unconfirmed);
    assert(!nativeRoomChat_Retry(&chat));
    chat.history_since_ms = 0;
    nativeRoomChat_Update(&chat, true);
    receive(NativeRoomChatEvent_Begin, 0, 0, NULL, 1);
    receive(NativeRoomChatEvent_Entry, 12, chat.nonce, "Retry this", 0);
    receive(NativeRoomChatEvent_End, 12, 0, NULL, 0);
    nativeRoomChat_Update(&chat, true);
    assert(chat.count == 1 && chat.delivery == NativeRoomChatDelivery_None);

    // A partial history response never replaces the visible conversation.
    receive(NativeRoomChatEvent_Begin, 0, 0, NULL, 2);
    receive(NativeRoomChatEvent_Entry, 13, 5, "Incomplete", 0);
    receive(NativeRoomChatEvent_End, 13, 0, NULL, 0);
    nativeRoomChat_Update(&chat, true);
    assert(!chat.ready && chat.count == 1 && chat.last_sequence == 12);
    assert(!strcmp(chat.messages[0].text, "Retry this"));

    atomic_store(&client.chat.overflow, true);
    chat.history_since_ms = 0;
    nativeRoomChat_Update(&chat, true);
    assert(chat.requested && !chat.ready && atomic_load(&client.running));
    assert(atomic_load(&client.events.write) == 0);
    receive(NativeRoomChatEvent_Begin, 0, 0, NULL, 0);
    receive(NativeRoomChatEvent_End, 0, 0, NULL, 0);
    nativeRoomChat_Update(&chat, true);
    assert(chat.ready && chat.count == 0);
    puts(
        "native room chat staging, delivery, retry and overflow tests passed");
    return 0;
}
