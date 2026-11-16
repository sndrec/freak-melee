#include "native_chat.h"
#include <melee/gm/native_account_transport.h>

#define WIN32_LEAN_AND_MEAN
#define _CRT_RAND_S
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winhttp.h>

enum {
    COMMAND_COUNT = 8,
    // Initial history plus a complete 128-player presence snapshot fits even
    // while the presentation thread is constructing a scene.
    EVENT_COUNT = 256
};
typedef struct ChatCommand {
    uint8_t bytes[NATIVE_CHAT_PACKET_BYTES];
    uint16_t size;
} ChatCommand;
typedef struct ChatIO {
    HANDLE read_done, write_done, closed;
    _Atomic DWORD error;
    DWORD received;
    WINHTTP_WEB_SOCKET_BUFFER_TYPE type;
} ChatIO;
typedef struct ChatConnection {
    HINTERNET session, connection, request, socket;
    ChatIO http, ws;
    bool request_callback, socket_callback;
    uint8_t receive[NATIVE_CHAT_PACKET_BYTES];
    ChatCommand send;
} ChatConnection;

// Fixed, single-producer/single-consumer mailboxes. Only the worker touches
// network handles; callbacks complete I/O into worker-owned storage, never UI.
static struct {
    _Alignas(64) _Atomic uint32_t command_write;
    _Alignas(64) _Atomic uint32_t command_read;
    ChatCommand commands[COMMAND_COUNT];
    _Alignas(64) _Atomic uint32_t event_write;
    _Alignas(64) _Atomic uint32_t event_read;
    NativeChatPacket events[EVENT_COUNT];
    _Atomic bool connected;
    HANDLE thread, stop, wake;
    char peer[33];
    char endpoint[512];
    uint64_t account_generation;
    wchar_t url[1024];
} network;
static NativeChatView view;
static uint32_t next_nonce;
static uint64_t pending_since;
static char announced_name[NATIVE_CHAT_NAME_BYTES + 1];
static NativeChatPlayer incoming_players[NATIVE_CHAT_PLAYERS];
static unsigned incoming_count, incoming_expected;
static bool incoming_presence;

static bool push_event(const NativeChatPacket* packet)
{
    uint32_t w =
        atomic_load_explicit(&network.event_write, memory_order_relaxed);
    uint32_t r =
        atomic_load_explicit(&network.event_read, memory_order_acquire);
    if (w - r >= EVENT_COUNT) {
        return false;
    }
    network.events[w % EVENT_COUNT] = *packet;
    atomic_store_explicit(&network.event_write, w + 1, memory_order_release);
    return true;
}

static bool pop_command(ChatCommand* command)
{
    uint32_t r =
        atomic_load_explicit(&network.command_read, memory_order_relaxed);
    uint32_t w =
        atomic_load_explicit(&network.command_write, memory_order_acquire);
    if (r == w) {
        return false;
    }
    *command = network.commands[r % COMMAND_COUNT];
    atomic_store_explicit(&network.command_read, r + 1, memory_order_release);
    return true;
}

static void CALLBACK http_callback(HINTERNET handle, DWORD_PTR context,
                                   DWORD status, void* info, DWORD size)
{
    (void) handle;
    ChatIO* io = (ChatIO*) context;
    if (!io) {
        return;
    }
    switch (status) {
    case WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING:
        SetEvent(io->closed);
        break;
    case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
    case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
    case WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE:
        SetEvent(io->write_done);
        break;
    case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
        if (size == sizeof(WINHTTP_WEB_SOCKET_STATUS)) {
            WINHTTP_WEB_SOCKET_STATUS* result = info;
            io->received = result->dwBytesTransferred;
            io->type = result->eBufferType;
            SetEvent(io->read_done);
        }
        break;
    case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
        if (size >= sizeof(WINHTTP_ASYNC_RESULT)) {
            atomic_store_explicit(&io->error,
                                  ((WINHTTP_ASYNC_RESULT*) info)->dwError,
                                  memory_order_release);
            SetEvent(io->read_done);
            SetEvent(io->write_done);
        }
        break;
    }
}

static bool init_io(ChatIO* io)
{
    io->read_done = CreateEventW(NULL, FALSE, FALSE, NULL);
    io->write_done = CreateEventW(NULL, FALSE, FALSE, NULL);
    io->closed = CreateEventW(NULL, FALSE, FALSE, NULL);
    return io->read_done && io->write_done && io->closed;
}

static void free_io(ChatIO* io)
{
    if (io->read_done) {
        CloseHandle(io->read_done);
    }
    if (io->write_done) {
        CloseHandle(io->write_done);
    }
    if (io->closed) {
        CloseHandle(io->closed);
    }
}

static bool set_callback(HINTERNET handle, ChatIO* io)
{
    DWORD_PTR context = (DWORD_PTR) io;
    return WinHttpSetOption(handle, WINHTTP_OPTION_CONTEXT_VALUE, &context,
                            sizeof(context)) &&
           WinHttpSetStatusCallback(handle, http_callback,
                                    WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS |
                                        WINHTTP_CALLBACK_FLAG_HANDLES,
                                    0) != WINHTTP_INVALID_STATUS_CALLBACK;
}

static void close_connection(ChatConnection* c)
{
    // Async handle closure cancels pending operations. HANDLE_CLOSING is the
    // last callback, so buffers and events remain alive until it has arrived.
    if (c->socket) {
        WinHttpCloseHandle(c->socket);
        if (c->socket_callback) {
            WaitForSingleObject(c->ws.closed, INFINITE);
        }
    }
    if (c->request) {
        WinHttpCloseHandle(c->request);
        if (c->request_callback) {
            WaitForSingleObject(c->http.closed, INFINITE);
        }
    }
    if (c->connection) {
        WinHttpCloseHandle(c->connection);
    }
    if (c->session) {
        WinHttpCloseHandle(c->session);
    }
    free_io(&c->ws);
    free_io(&c->http);
}

static bool wait_write(ChatIO* io)
{
    HANDLE handles[] = { network.stop, io->write_done };
    return WaitForMultipleObjects(2, handles, FALSE, 5000) ==
               WAIT_OBJECT_0 + 1 &&
           atomic_load_explicit(&io->error, memory_order_acquire) == 0;
}

static bool open_connection(ChatConnection* c)
{
    if (!init_io(&c->http) || !init_io(&c->ws)) {
        return false;
    }
    URL_COMPONENTS parts = { .dwStructSize = sizeof(parts),
                             .dwHostNameLength = (DWORD) -1,
                             .dwUrlPathLength = (DWORD) -1,
                             .dwExtraInfoLength = (DWORD) -1 };
    if (!WinHttpCrackUrl(network.url, 0, 0, &parts) ||
        (parts.nScheme != INTERNET_SCHEME_HTTP &&
         parts.nScheme != INTERNET_SCHEME_HTTPS))
    {
        return false;
    }
    wchar_t host[256], path[1024];
    if (parts.dwHostNameLength >= 256 ||
        parts.dwUrlPathLength + parts.dwExtraInfoLength >= 1024)
    {
        return false;
    }
    memcpy(host, parts.lpszHostName, parts.dwHostNameLength * sizeof(wchar_t));
    host[parts.dwHostNameLength] = 0;
    memcpy(path, parts.lpszUrlPath, parts.dwUrlPathLength * sizeof(wchar_t));
    if (parts.dwExtraInfoLength) {
        memcpy(path + parts.dwUrlPathLength, parts.lpszExtraInfo,
               parts.dwExtraInfoLength * sizeof(wchar_t));
    }
    path[parts.dwUrlPathLength + parts.dwExtraInfoLength] = 0;
    c->session = WinHttpOpen(
        L"Freak Melee Chat", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC);
    if (!c->session) {
        return false;
    }
    WinHttpSetTimeouts(c->session, 5000, 5000, 5000, 5000);
    c->connection = WinHttpConnect(c->session, host, parts.nPort, 0);
    if (!c->connection) {
        return false;
    }
    c->request = WinHttpOpenRequest(
        c->connection, L"GET", path, NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        parts.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
    if (!c->request) {
        return false;
    }
    c->request_callback = set_callback(c->request, &c->http);
    wchar_t headers[192] = { 0 };
    nativeAccountTransport_Headers(network.url, headers, 192);
    DWORD redirects = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    DWORD disabled = WINHTTP_DISABLE_COOKIES | WINHTTP_DISABLE_AUTHENTICATION;
    bool policy = WinHttpSetOption(c->request, WINHTTP_OPTION_REDIRECT_POLICY, &redirects, sizeof(redirects)) &&
        WinHttpSetOption(c->request, WINHTTP_OPTION_DISABLE_FEATURE, &disabled, sizeof(disabled));
    if (!c->request_callback ||
        !policy ||
        !WinHttpSetOption(c->request, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET,
                          NULL, 0) ||
        !WinHttpSendRequest(c->request, headers, (DWORD) -1,
                            WINHTTP_NO_REQUEST_DATA, 0, 0,
                            (DWORD_PTR) &c->http) ||
        !wait_write(&c->http) || !WinHttpReceiveResponse(c->request, NULL) ||
        !wait_write(&c->http))
    {
        SecureZeroMemory(headers, sizeof(headers));
        return false;
    }
    SecureZeroMemory(headers, sizeof(headers));
    DWORD status = 0, size = sizeof(status);
    if (!WinHttpQueryHeaders(
            c->request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &status, &size,
            WINHTTP_NO_HEADER_INDEX) ||
        status != 101)
    {
        return false;
    }
    c->socket =
        WinHttpWebSocketCompleteUpgrade(c->request, (DWORD_PTR) &c->ws);
    if (!c->socket) {
        return false;
    }
    c->socket_callback = set_callback(c->socket, &c->ws);
    return c->socket_callback;
}

static void run_connection(ChatConnection* c)
{
    size_t buffered = 0;
    bool receiving = false;
    uint64_t last_receive = GetTickCount64(), next_ping = last_receive + 10000;
    HANDLE handles[] = { network.stop, c->ws.read_done, network.wake };
    for (;;) {
        if (!receiving) {
            if (buffered == sizeof(c->receive) ||
                WinHttpWebSocketReceive(
                    c->socket, c->receive + buffered,
                    (DWORD) (sizeof(c->receive) - buffered), NULL, NULL))
            {
                return;
            }
            receiving = true;
        }
        DWORD event = WaitForMultipleObjects(3, handles, FALSE, 1000);
        if (event == WAIT_OBJECT_0 || event == WAIT_FAILED ||
            atomic_load_explicit(&c->ws.error, memory_order_acquire))
        {
            return;
        }
        uint64_t now = GetTickCount64();
        if (event == WAIT_OBJECT_0 + 1) {
            receiving = false;
            if (c->ws.type == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE) {
                USHORT code = 0; DWORD reason_size = 0; char reason[128];
                if (WinHttpWebSocketQueryCloseStatus(c->socket, &code, reason, sizeof(reason), &reason_size) == ERROR_SUCCESS &&
                    (code == 4001 || code == 4002 || code == 1008 || code == 4006)) SetEvent(network.stop);
                return;
            }
            if (c->ws.type != WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE &&
                c->ws.type != WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE)
            {
                return;
            }
            buffered += c->ws.received;
            if (c->ws.type == WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE) {
                NativeChatPacket packet;
                if (!nativeChatWire_Decode(c->receive, buffered, &packet)) {
                    return;
                }
                if (packet.type == 0x82) {
                    atomic_store_explicit(&network.connected, true,
                                          memory_order_release);
                }
                if (packet.type != 0x84 && !push_event(&packet)) {
                    return;
                }
                if (packet.type == 0x84) last_receive = now;
                buffered = 0;
            }
        }
        if (now - last_receive > 45000) {
            return;
        }
        while (pop_command(&c->send)) {
            if (WinHttpWebSocketSend(
                    c->socket, WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE,
                    c->send.bytes, c->send.size) ||
                !wait_write(&c->ws))
            {
                return;
            }
        }
        if (now >= next_ping) {
            c->send.bytes[0] = 2;
            if (WinHttpWebSocketSend(
                    c->socket, WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE,
                    c->send.bytes, 1) ||
                !wait_write(&c->ws))
            {
                return;
            }
            next_ping = now + 10000;
        }
    }
}

static DWORD WINAPI chat_worker(void* unused)
{
    (void) unused;
    DWORD retry_ms = 1000;
    while (WaitForSingleObject(network.stop, 0) != WAIT_OBJECT_0) {
        ChatConnection connection = { 0 };
        if (open_connection(&connection)) {
            run_connection(&connection);
        }
        bool was_connected = atomic_exchange_explicit(
            &network.connected, false, memory_order_acq_rel);
        close_connection(&connection);
        ChatCommand discarded;
        while (pop_command(&discarded)) {
        }
        NativeChatPacket disconnected = { .type = 0xff };
        push_event(&disconnected);
        if (was_connected) {
            retry_ms = 1000;
        }
        if (WaitForSingleObject(network.stop, retry_ms) == WAIT_OBJECT_0) {
            break;
        }
        if (retry_ms < 30000) {
            retry_ms = retry_ms > 15000 ? 30000 : retry_ms * 2;
        }
    }
    return 0;
}

bool nativeChat_Start(const char* endpoint)
{
    if (network.thread || !endpoint) {
        return false;
    }
    memset(&network, 0, sizeof(network));
    if (strlen(endpoint) >= sizeof(network.endpoint)) return false;
    memcpy(network.endpoint, endpoint, strlen(endpoint) + 1);
    network.account_generation = nativeAccountTransport_Generation();
    memset(&view, 0, sizeof(view));
    view.state = NativeChat_Offline;
    next_nonce = 0;
    announced_name[0] = 0;
    incoming_presence = false;
    for (size_t i = 0; i < 4; ++i) {
        unsigned int random;
        if (rand_s(&random)) {
            return false;
        }
        snprintf(network.peer + i * 8, 9, "%08x", random);
    }
    char url[1024];
    size_t length = strlen(endpoint);
    if (length == 0) {
        return false;
    }
    int written = snprintf(url, sizeof(url), "%.*s/chat/connect?peer=%s",
                           (int) (length - (endpoint[length - 1] == '/')),
                           endpoint, network.peer);
    if (written <= 0 || written >= sizeof(url) ||
        !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, url, -1,
                             network.url, 1024))
    {
        return false;
    }
    network.stop = CreateEventW(NULL, TRUE, FALSE, NULL);
    network.wake = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (network.stop && network.wake) {
        network.thread = CreateThread(NULL, 0, chat_worker, NULL, 0, NULL);
    }
    if (!network.thread) {
        nativeChat_Stop();
        return false;
    }
    view.state = NativeChat_Connecting;
    return true;
}

void nativeChat_Update(void)
{
    if (network.thread && network.account_generation != nativeAccountTransport_Generation()) {
        char endpoint[512]; memcpy(endpoint, network.endpoint, sizeof(endpoint));
        nativeChat_Stop(); nativeChat_Start(endpoint);
    }
    uint32_t r =
        atomic_load_explicit(&network.event_read, memory_order_relaxed);
    uint32_t w =
        atomic_load_explicit(&network.event_write, memory_order_acquire);
    for (; r != w; ++r) {
        const NativeChatPacket* packet = &network.events[r % EVENT_COUNT];
        if (packet->type == 0xff) {
            view.state = NativeChat_Reconnecting;
            view.online = view.player_count = 0;
            view.presence_ready = false;
            incoming_presence = false;
            announced_name[0] = 0;
            if (view.delivery == NativeChatDelivery_Sending) {
                view.delivery = NativeChatDelivery_Unconfirmed;
            }
        } else if (packet->type == 0x82) {
            view.online = packet->online;
            view.state = NativeChat_Online;
        } else if (packet->type == 0x85) {
            incoming_presence = true;
            incoming_count = 0;
            incoming_expected = packet->online;
        } else if (packet->type == 0x86 && incoming_presence) {
            for (unsigned i = 0; i < incoming_count; ++i) {
                if (!strcmp(incoming_players[i].peer, packet->player.peer)) {
                    incoming_presence = false;
                    break;
                }
            }
            if (incoming_presence && incoming_count < incoming_expected) {
                incoming_players[incoming_count++] = packet->player;
            } else {
                incoming_presence = false;
            }
        } else if (packet->type == 0x87) {
            if (incoming_presence && incoming_count == incoming_expected) {
                memcpy(view.players, incoming_players,
                       sizeof(incoming_players[0]) * incoming_count);
                view.online = view.player_count = (uint16_t) incoming_count;
                view.presence_ready = true;
            }
            incoming_presence = false;
        } else if (packet->type == 0x81) {
            const NativeChatMessage* m = &packet->message;
            if (m->nonce == view.pending.nonce &&
                strcmp(m->peer, network.peer) == 0)
            {
                view.delivery = NativeChatDelivery_None;
            }
            if (view.count == 0 || m->id > view.messages[view.count - 1].id) {
                if (view.count == NATIVE_CHAT_HISTORY) {
                    memmove(view.messages, view.messages + 1,
                            sizeof(view.messages[0]) *
                                (NATIVE_CHAT_HISTORY - 1));
                    --view.count;
                }
                view.messages[view.count++] = *m;
            }
        }
        ++view.revision;
    }
    atomic_store_explicit(&network.event_read, r, memory_order_release);
    if (view.state == NativeChat_Online &&
        !atomic_load_explicit(&network.connected, memory_order_acquire))
    {
        // A full event queue must not leave the UI claiming a lost connection
        // is usable while the worker reconnects and fetches recent history.
        view.state = NativeChat_Reconnecting;
        view.online = view.player_count = 0;
        view.presence_ready = false;
        incoming_presence = false;
        announced_name[0] = 0;
        if (view.delivery == NativeChatDelivery_Sending) {
            view.delivery = NativeChatDelivery_Unconfirmed;
        }
        ++view.revision;
    }
    if (view.delivery == NativeChatDelivery_Sending &&
        GetTickCount64() - pending_since >= 10000)
    {
        view.delivery = NativeChatDelivery_Unconfirmed;
        ++view.revision;
    }
}

bool nativeChat_Identify(const char* name)
{
    if (!network.thread || !name || view.state != NativeChat_Online ||
        !atomic_load_explicit(&network.connected, memory_order_acquire))
    {
        return false;
    }
    if (!strcmp(announced_name, name)) {
        return true;
    }
    uint32_t w =
        atomic_load_explicit(&network.command_write, memory_order_relaxed);
    uint32_t r =
        atomic_load_explicit(&network.command_read, memory_order_acquire);
    if (w - r >= COMMAND_COUNT) {
        return false;
    }
    ChatCommand* command = &network.commands[w % COMMAND_COUNT];
    size_t size =
        nativeChatWire_Identify(name, command->bytes, sizeof(command->bytes));
    if (!size) {
        return false;
    }
    command->size = (uint16_t) size;
    atomic_store_explicit(&network.command_write, w + 1, memory_order_release);
    strcpy(announced_name, name);
    SetEvent(network.wake);
    return true;
}

bool nativeChat_Send(const char* name, const char* text)
{
    if (!network.thread || !name || !text || view.state != NativeChat_Online ||
        view.delivery == NativeChatDelivery_Sending ||
        !atomic_load_explicit(&network.connected, memory_order_acquire))
    {
        return false;
    }
    uint32_t nonce = next_nonce + 1;
    if (view.delivery == NativeChatDelivery_Unconfirmed &&
        strcmp(name, view.pending.name) == 0 &&
        strcmp(text, view.pending.text) == 0)
    {
        nonce = view.pending.nonce;
    }
    uint32_t w =
        atomic_load_explicit(&network.command_write, memory_order_relaxed);
    uint32_t r =
        atomic_load_explicit(&network.command_read, memory_order_acquire);
    if (w - r >= COMMAND_COUNT) {
        return false;
    }
    ChatCommand* command = &network.commands[w % COMMAND_COUNT];
    size_t size = nativeChatWire_Post(nonce, name, text, command->bytes,
                                      sizeof(command->bytes));
    if (!size) {
        return false;
    }
    command->size = (uint16_t) size;
    atomic_store_explicit(&network.command_write, w + 1, memory_order_release);
    SetEvent(network.wake);
    next_nonce = nonce;
    view.pending.nonce = nonce;
    strcpy(view.pending.name, name);
    strcpy(view.pending.text, text);
    view.delivery = NativeChatDelivery_Sending;
    pending_since = GetTickCount64();
    ++view.revision;
    return true;
}

const NativeChatView* nativeChat_View(void)
{
    return &view;
}
const char* nativeChat_LocalPeerId(void)
{
    return network.peer;
}

void nativeChat_Stop(void)
{
    if (network.thread) {
        SetEvent(network.stop);
        WaitForSingleObject(network.thread, INFINITE);
        CloseHandle(network.thread);
    }
    if (network.stop) {
        CloseHandle(network.stop);
    }
    if (network.wake) {
        CloseHandle(network.wake);
    }
    network.thread = network.stop = network.wake = NULL;
    view.state = NativeChat_Offline;
    view.online = view.player_count = 0;
    view.presence_ready = false;
    incoming_presence = false;
    announced_name[0] = 0;
}
