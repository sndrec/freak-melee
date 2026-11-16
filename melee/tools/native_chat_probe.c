#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include <melee/ui/native_chat.h>
#include <melee/gm/native_account_transport.h>

int main(int argc, char** argv)
{
    if (argc != 6) {
        fprintf(stderr, "Usage: native_chat_probe ENDPOINT NAME MESSAGE "
                        "EXPECTED_MESSAGE SECONDS\n"
                        "   or: native_chat_probe --presence ENDPOINT NAME "
                        "EXPECTED_NAME SECONDS\n");
        return 2;
    }
    const bool presence = strcmp(argv[1], "--presence") == 0;
    char temporary[MAX_PATH], identity_root[MAX_PATH];
    if (!GetTempPathA(sizeof(temporary), temporary)) return 2;
    snprintf(identity_root, sizeof(identity_root), "%sFreakMeleeChatProbe-%lu", temporary, (unsigned long) GetCurrentProcessId());
    if (!nativeAccountTransport_InitGuest(identity_root, presence ? argv[2] : argv[1])) return 2;
    const char* name = presence ? argv[3] : argv[2];
    if (!nativeChat_Start(presence ? argv[2] : argv[1])) {
        return 2;
    }
    uint64_t deadline = GetTickCount64() + (uint64_t) atoi(argv[5]) * 1000;
    uint64_t last = 0;
    NativeChatState previous = NativeChat_Offline;
    uint64_t presence_revision = 0;
    bool sent = false, acknowledged = false, received = false;
    while (GetTickCount64() < deadline) {
        nativeChat_Update();
        const NativeChatView* view = nativeChat_View();
        if (view->state != previous) {
            printf("state=%d online=%u\n", view->state, view->online);
            previous = view->state;
        }
        if (presence && view->state == NativeChat_Online) {
            nativeChat_Identify(name);
            if (presence_revision != view->revision) {
                for (unsigned i = 0; i < view->player_count; ++i) {
                    const NativeChatPlayer* player = &view->players[i];
                    printf("player=%s name=%s\n", player->peer, player->name);
                    acknowledged |= strcmp(player->name, name) == 0;
                    received |= strcmp(player->name, argv[4]) == 0;
                }
                presence_revision = view->revision;
            }
        } else if (!presence && !sent && view->state == NativeChat_Online) {
            sent = nativeChat_Send(argv[2], argv[3]);
        }
        if (!presence && sent && view->delivery == NativeChatDelivery_None) {
            acknowledged = true;
        }
        for (uint32_t i = 0; i < view->count; ++i) {
            const NativeChatMessage* message = &view->messages[i];
            if (message->id <= last) {
                continue;
            }
            printf("message=%llu name=%s text=%s\n",
                   (unsigned long long) message->id, message->name,
                   message->text);
            if (strcmp(message->text, argv[4]) == 0) {
                received = true;
            }
            last = message->id;
        }
        fflush(stdout);
        Sleep(10);
    }
    uint64_t stopped = GetTickCount64();
    nativeChat_Stop();
    printf("stop_ms=%llu acknowledged=%u received=%u\n",
           (unsigned long long) (GetTickCount64() - stopped), acknowledged,
           received);
    return acknowledged && received ? 0 : 1;
}
