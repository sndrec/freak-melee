#include "native_signaling_client.h"
#include "native_account_transport.h"

#ifdef MELEE_NATIVE

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>

enum {
    CONNECT_TIMEOUT_MS = 5000,
    SEND_TIMEOUT_MS = 2000,
    HEARTBEAT_MS = 10000,
    HEARTBEAT_TIMEOUT_MS = 45000,
};

typedef struct SignalingIO {
    HANDLE read_done, write_done, closed;
    _Atomic DWORD error;
    DWORD received;
    WINHTTP_WEB_SOCKET_BUFFER_TYPE type;
} SignalingIO;

typedef struct SignalingConnection {
    HINTERNET session, connection, request, socket;
    SignalingIO http, ws;
    bool request_callback, socket_callback;
    bool send_pending;
    wchar_t headers[192];
    uint8_t receive[NATIVE_SIGNALING_MAX_MESSAGE_BYTES];
    NativeSignalingCommandSlot send;
} SignalingConnection;

static bool copy_bounded(char* output, size_t capacity, const char* input)
{
    if (output == NULL || capacity == 0 || input == NULL) {
        return false;
    }
    size_t length = strnlen(input, capacity);
    if (length == 0 || length >= capacity) {
        return false;
    }
    memcpy(output, input, length + 1);
    return true;
}

static bool valid_room_id(const char* room_id)
{
    size_t length = room_id == NULL ? 0 : strlen(room_id);
    if (length < 6 || length > NATIVE_SIGNALING_ROOM_ID_BYTES) {
        return false;
    }
    for (size_t i = 0; i < length; ++i) {
        if (!(room_id[i] >= 'A' && room_id[i] <= 'Z') &&
            !(room_id[i] >= '0' && room_id[i] <= '9'))
        {
            return false;
        }
    }
    return true;
}

static bool valid_peer_id(const char* peer_id)
{
    size_t length = peer_id == NULL ? 0 : strlen(peer_id);
    if (length < 8 || length > NATIVE_SIGNALING_MAX_PEER_ID_BYTES) {
        return false;
    }
    for (size_t i = 0; i < length; ++i) {
        unsigned char c = (unsigned char) peer_id[i];
        if (!isalnum(c) && c != '_' && c != '-') {
            return false;
        }
    }
    return true;
}

static bool command_push(NativeSignalingCommandQueue* queue,
                         const uint8_t* bytes, size_t size)
{
    if (queue == NULL || bytes == NULL || size == 0 ||
        size > NATIVE_SIGNALING_MAX_MESSAGE_BYTES || size > UINT16_MAX)
    {
        return false;
    }
    uint32_t write = atomic_load_explicit(&queue->write, memory_order_relaxed);
    uint32_t read = atomic_load_explicit(&queue->read, memory_order_acquire);
    if (write - read >= NATIVE_SIGNALING_COMMAND_CAPACITY) {
        return false;
    }
    NativeSignalingCommandSlot* slot =
        &queue->slots[write % NATIVE_SIGNALING_COMMAND_CAPACITY];
    memcpy(slot->bytes, bytes, size);
    slot->size = (uint16_t) size;
    atomic_store_explicit(&queue->write, write + 1, memory_order_release);
    return true;
}

static bool command_pop(NativeSignalingCommandQueue* queue,
                        NativeSignalingCommandSlot* output)
{
    uint32_t read = atomic_load_explicit(&queue->read, memory_order_relaxed);
    uint32_t write = atomic_load_explicit(&queue->write, memory_order_acquire);
    if (read == write) {
        return false;
    }
    NativeSignalingCommandSlot* slot = &queue->slots[read % NATIVE_SIGNALING_COMMAND_CAPACITY];
    *output = *slot;
    SecureZeroMemory(slot->bytes, slot->size);
    atomic_store_explicit(&queue->read, read + 1, memory_order_release);
    return true;
}

static bool event_push(NativeSignalingEventQueue* queue,
                       const NativeSignalingClientEvent* event)
{
    uint32_t write = atomic_load_explicit(&queue->write, memory_order_relaxed);
    uint32_t read = atomic_load_explicit(&queue->read, memory_order_acquire);
    if (write - read >= NATIVE_SIGNALING_EVENT_CAPACITY) {
        return false;
    }
    queue->slots[write % NATIVE_SIGNALING_EVENT_CAPACITY] = *event;
    atomic_store_explicit(&queue->write, write + 1, memory_order_release);
    return true;
}

static bool event_pop(NativeSignalingEventQueue* queue,
                      NativeSignalingClientEvent* output)
{
    uint32_t read = atomic_load_explicit(&queue->read, memory_order_relaxed);
    uint32_t write = atomic_load_explicit(&queue->write, memory_order_acquire);
    if (read == write) {
        return false;
    }
    *output = queue->slots[read % NATIVE_SIGNALING_EVENT_CAPACITY];
    atomic_store_explicit(&queue->read, read + 1, memory_order_release);
    return true;
}

static void push_lifecycle(NativeSignalingClient* client,
                           NativeSignalingClientEventType type,
                           uint32_t error_code, const char* detail)
{
    NativeSignalingClientEvent event = {
        .type = type,
        .error_code = error_code,
        .terminal = error_code == ERROR_ACCESS_DENIED || error_code == ERROR_INVALID_PASSWORD,
    };
    if (detail != NULL) {
        snprintf(event.detail, sizeof(event.detail), "%s", detail);
    }
    event_push(&client->events, &event);
}

static bool percent_encode(const char* input, char* output, size_t capacity)
{
    static const char hex[] = "0123456789ABCDEF";
    size_t write = 0;
    for (const uint8_t* read = (const uint8_t*) input; *read != 0; ++read) {
        uint8_t c = *read;
        bool plain = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                     (c >= '0' && c <= '9') || c == '-' || c == '_' ||
                     c == '.' || c == '~';
        size_t count = plain ? 1 : 3;
        if (write + count >= capacity) {
            return false;
        }
        if (plain) {
            output[write++] = (char) c;
        } else {
            output[write++] = '%';
            output[write++] = hex[c >> 4];
            output[write++] = hex[c & 15];
        }
    }
    output[write] = '\0';
    return true;
}

static bool build_url(const NativeSignalingClient* client, wchar_t* wide_url,
                      size_t wide_capacity)
{
    char name[NATIVE_SIGNALING_MAX_NAME_BYTES * 3 + 1];
    char build[NATIVE_SIGNALING_MAX_BUILD_BYTES * 3 + 1];
    char url[1536];
    if (!percent_encode(client->name, name, sizeof(name)) ||
        !percent_encode(client->build, build, sizeof(build)))
    {
        return false;
    }
    size_t endpoint_length = strlen(client->endpoint);
    int written = client->directory ? snprintf(url, sizeof(url), "%.*s/v1/lobbies/connect?build=%s",
        (int) (endpoint_length - (client->endpoint[endpoint_length - 1] == '/')), client->endpoint, build) : snprintf(
        url, sizeof(url),
        "%.*s/v1/rooms/%s/connect?peer=%s&name=%s&build=%s&players=%u%s%s",
        (int) (endpoint_length - (client->endpoint[endpoint_length - 1] == '/')),
        client->endpoint, client->room_id, client->peer_id, name, build,
        client->local_players, client->admission[0] ? "&admission=" : "",
        client->admission);
    if (written <= 0 || (size_t) written >= sizeof(url)) {
        return false;
    }
    int wide_length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, url, -1,
                                          wide_url, (int) wide_capacity);
    return wide_length > 0;
}

static void CALLBACK http_callback(HINTERNET handle, DWORD_PTR context,
                                   DWORD status, void* info, DWORD size)
{
    (void) handle;
    SignalingIO* io = (SignalingIO*) context;
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
    case WINHTTP_CALLBACK_STATUS_SHUTDOWN_COMPLETE:
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

static bool init_io(SignalingIO* io)
{
    io->read_done = CreateEventW(NULL, FALSE, FALSE, NULL);
    io->write_done = CreateEventW(NULL, FALSE, FALSE, NULL);
    io->closed = CreateEventW(NULL, FALSE, FALSE, NULL);
    return io->read_done && io->write_done && io->closed;
}

static void free_io(SignalingIO* io)
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

static bool set_callback(HINTERNET handle, SignalingIO* io)
{
    DWORD_PTR context = (DWORD_PTR) io;
    return WinHttpSetOption(handle, WINHTTP_OPTION_CONTEXT_VALUE, &context,
                            sizeof(context)) &&
           WinHttpSetStatusCallback(handle, http_callback,
                                    WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS |
                                        WINHTTP_CALLBACK_FLAG_HANDLES,
                                    0) != WINHTTP_INVALID_STATUS_CALLBACK;
}

static void close_connection(SignalingConnection* c)
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

static DWORD wait_write(NativeSignalingClient* client, SignalingIO* io, bool closing)
{
    HANDLE handles[] = { io->write_done, (HANDLE) client->wake_event };
    uint64_t deadline = GetTickCount64() + (closing ? 1000 : 5000);
    for (;;) {
        DWORD event = WaitForMultipleObjects(2, handles, FALSE, 1000);
        DWORD error = atomic_load_explicit(&io->error, memory_order_acquire);
        if (error) return error;
        if (event == WAIT_OBJECT_0) return ERROR_SUCCESS;
        if (event == WAIT_FAILED) return GetLastError();
        if (!closing && atomic_load_explicit(&client->stop_requested, memory_order_acquire)) return ERROR_CANCELLED;
        if (GetTickCount64() >= deadline) return ERROR_TIMEOUT;
    }
}

static DWORD open_connection(NativeSignalingClient* client, SignalingConnection* c)
{
    if (!init_io(&c->http) || !init_io(&c->ws)) return ERROR_NOT_ENOUGH_MEMORY;
    wchar_t url[1536], host[256], path[1536];
    if (!build_url(client, url, sizeof(url) / sizeof(url[0]))) return ERROR_INVALID_PARAMETER;
    URL_COMPONENTS parts = { .dwStructSize = sizeof(parts), .dwHostNameLength = (DWORD) -1,
        .dwUrlPathLength = (DWORD) -1, .dwExtraInfoLength = (DWORD) -1 };
    if (!WinHttpCrackUrl(url, 0, 0, &parts) ||
        (parts.nScheme != INTERNET_SCHEME_HTTP && parts.nScheme != INTERNET_SCHEME_HTTPS) || parts.dwHostNameLength >= 256 ||
        parts.dwUrlPathLength + parts.dwExtraInfoLength >= 1536) return ERROR_INVALID_PARAMETER;
    memcpy(host, parts.lpszHostName, parts.dwHostNameLength * sizeof(wchar_t)); host[parts.dwHostNameLength] = 0;
    memcpy(path, parts.lpszUrlPath, parts.dwUrlPathLength * sizeof(wchar_t));
    if (parts.dwExtraInfoLength) memcpy(path + parts.dwUrlPathLength, parts.lpszExtraInfo, parts.dwExtraInfoLength * sizeof(wchar_t));
    path[parts.dwUrlPathLength + parts.dwExtraInfoLength] = 0;
    c->session = WinHttpOpen(L"Freak Melee Signaling", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC);
    if (!c->session) return GetLastError();
    WinHttpSetTimeouts(c->session, CONNECT_TIMEOUT_MS, CONNECT_TIMEOUT_MS, SEND_TIMEOUT_MS, 5000);
    c->connection = WinHttpConnect(c->session, host, parts.nPort, 0);
    if (!c->connection) return GetLastError();
    c->request = WinHttpOpenRequest(c->connection, L"GET", path, NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, parts.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
    if (!c->request) return GetLastError();
    c->request_callback = set_callback(c->request, &c->http);
    DWORD redirects = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    DWORD disabled = WINHTTP_DISABLE_COOKIES | WINHTTP_DISABLE_AUTHENTICATION;
    if (!c->request_callback ||
        !WinHttpSetOption(c->request, WINHTTP_OPTION_REDIRECT_POLICY, &redirects, sizeof(redirects)) ||
        !WinHttpSetOption(c->request, WINHTTP_OPTION_DISABLE_FEATURE, &disabled, sizeof(disabled)) ||
        !WinHttpSetOption(c->request, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, NULL, 0)) return GetLastError();
    if (!client->directory) nativeAccountTransport_Headers(url, c->headers, 192);
    if (*client->guest_key) swprintf_s(c->headers, 192, L"X-Freak-Guest: %hs\r\n", client->guest_key);
    if (*client->room_password) {
        bool local = !wcscmp(host, L"localhost") || !wcscmp(host, L"127.0.0.1") || !wcscmp(host, L"[::1]");
        if (parts.nScheme != INTERNET_SCHEME_HTTPS && !local) return ERROR_ACCESS_DENIED;
        wchar_t password_header[100];
        swprintf_s(password_header, 100, L"X-Freak-Lobby-Password: %hs\r\n", client->room_password);
        bool added = WinHttpAddRequestHeaders(c->request, password_header, (DWORD) -1, WINHTTP_ADDREQ_FLAG_ADD);
        SecureZeroMemory(password_header, sizeof(password_header));
        if (!added) return GetLastError();
    }
    bool sent = WinHttpSendRequest(c->request, c->headers, (DWORD) -1, WINHTTP_NO_REQUEST_DATA, 0, 0, (DWORD_PTR) &c->http);
    if (!sent) return GetLastError();
    DWORD result = wait_write(client, &c->http, false);
    SecureZeroMemory(c->headers, sizeof(c->headers));
    if (result != ERROR_SUCCESS) return result;
    if (!WinHttpReceiveResponse(c->request, NULL)) return GetLastError();
    result = wait_write(client, &c->http, false);
    if (result != ERROR_SUCCESS) return result;
    DWORD status = 0, size = sizeof(status);
    if (!WinHttpQueryHeaders(c->request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX)) return GetLastError();
    if (status == 423) return ERROR_INVALID_PASSWORD;
    if (status != 101) return status >= 400 && status < 500 ? ERROR_ACCESS_DENIED : ERROR_BAD_NET_RESP;
    c->socket = WinHttpWebSocketCompleteUpgrade(c->request, (DWORD_PTR) &c->ws);
    if (!c->socket) return GetLastError();
    c->socket_callback = set_callback(c->socket, &c->ws);
    return c->socket_callback ? ERROR_SUCCESS : GetLastError();
}

static DWORD dispatch_message(NativeSignalingClient* client, uint8_t* buffer, size_t size)
{
    NativeSignalingClientEvent event = {
        .type = NativeSignalingClientEvent_Message,
    };
    if (!nativeSignalingWire_DecodeServer(buffer, size, &event.message)) {
        return ERROR_INVALID_DATA;
    }
    atomic_store_explicit(&client->last_receive_ms, GetTickCount64(), memory_order_release);
    if (event.message.type == NativeSignalingServerMessage_Chat) {
        NativeSignalingChatQueue* queue = &client->chat;
        uint32_t write = atomic_load_explicit(&queue->write, memory_order_relaxed);
        uint32_t read = atomic_load_explicit(&queue->read, memory_order_acquire);
        if (write - read >= NATIVE_SIGNALING_CHAT_CAPACITY) {
            atomic_store_explicit(&queue->overflow, true, memory_order_release);
        } else {
            queue->slots[write % NATIVE_SIGNALING_CHAT_CAPACITY] = event.message.data.chat;
            atomic_store_explicit(&queue->write, write + 1, memory_order_release);
        }
        return ERROR_SUCCESS;
    }
    if (!event_push(&client->events, &event)) {
        return ERROR_NOT_ENOUGH_QUOTA;
    }
    return ERROR_SUCCESS;
}

static DWORD run_connection(NativeSignalingClient* client, SignalingConnection* c)
{
    uint8_t* buffer = c->receive;
    size_t buffered = 0;
    bool receiving = false;
    uint64_t last_heartbeat = GetTickCount64(), next_ping = last_heartbeat + HEARTBEAT_MS;
    HANDLE handles[] = { c->ws.read_done, (HANDLE) client->wake_event };
    while (!atomic_load_explicit(&client->stop_requested, memory_order_acquire)) {
        if (!receiving) {
            if (buffered == sizeof(c->receive)) return ERROR_INSUFFICIENT_BUFFER;
            DWORD result = WinHttpWebSocketReceive(c->socket, buffer + buffered, (DWORD) (sizeof(c->receive) - buffered), NULL, NULL);
            if (result) return result;
            receiving = true;
        }
        DWORD event = WaitForMultipleObjects(2, handles, FALSE, 1000);
        DWORD error = atomic_load_explicit(&c->ws.error, memory_order_acquire);
        if (error) return error;
        if (event == WAIT_FAILED) return GetLastError();
        if (event == WAIT_OBJECT_0) {
            receiving = false;
            if (c->ws.type == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE) {
                USHORT code = 0; DWORD reason_size = 0; char reason[128];
                if (WinHttpWebSocketQueryCloseStatus(c->socket, &code, reason, sizeof(reason), &reason_size) == ERROR_SUCCESS &&
                    (code == 1000 || code == 1008 || code == 4001 || code == 4002 || code == 4005 || code == 4006)) return ERROR_ACCESS_DENIED;
                return ERROR_CONNECTION_ABORTED;
            }
            if (c->ws.type == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE && !buffered && c->ws.received == 4 && !memcmp(buffer, "pong", 4)) {
                last_heartbeat = GetTickCount64();
                atomic_store_explicit(&client->last_receive_ms, last_heartbeat, memory_order_release);
            } else {
                if (c->ws.type != WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE &&
                    c->ws.type != WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE) return ERROR_INVALID_DATA;
                buffered += c->ws.received;
                if (c->ws.type == WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE) {
                    DWORD result = dispatch_message(client, buffer, buffered);
                    if (result) return result;
                    buffered = 0;
                }
            }
        }
        while (command_pop(&client->commands, &c->send)) {
            DWORD result = WinHttpWebSocketSend(c->socket, WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE, c->send.bytes, c->send.size);
            if (!result) { c->send_pending = true; result = wait_write(client, &c->ws, false); }
            if (result) return result;
            c->send_pending = false;
            SecureZeroMemory(c->send.bytes, c->send.size);
        }
        uint64_t now = GetTickCount64();
        if (now - last_heartbeat >= HEARTBEAT_TIMEOUT_MS) return ERROR_TIMEOUT;
        if (now >= next_ping) {
            DWORD result = WinHttpWebSocketSend(c->socket, WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE, "ping", 4);
            if (!result) { c->send_pending = true; result = wait_write(client, &c->ws, false); }
            if (result) return result;
            c->send_pending = false;
            next_ping = now + HEARTBEAT_MS;
        }
    }
    return ERROR_SUCCESS;
}

static DWORD WINAPI signaling_thread(void* context)
{
    NativeSignalingClient* client = context;
    DWORD result = ERROR_SUCCESS;
    uint64_t recovery_deadline = 0;
    bool had_connection = false;
    for (;;) {
        SignalingConnection connection = { 0 };
        result = open_connection(client, &connection);
        if (result == ERROR_SUCCESS) {
            had_connection = true;
            push_lifecycle(client, NativeSignalingClientEvent_Connected, 0, "Room signaling connected");
            atomic_store_explicit(&client->last_receive_ms, GetTickCount64(), memory_order_release);
            uint64_t connected_at = GetTickCount64();
            result = run_connection(client, &connection);
            if (atomic_load_explicit(&client->last_receive_ms, memory_order_acquire) > connected_at + HEARTBEAT_MS)
                recovery_deadline = 0;
            if (atomic_load_explicit(&client->stop_requested, memory_order_acquire)) {
                if (connection.send_pending) wait_write(client, &connection.ws, true);
                if (WinHttpWebSocketShutdown(connection.socket, WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, NULL, 0) == ERROR_SUCCESS)
                    wait_write(client, &connection.ws, true);
            }
        }
        close_connection(&connection);
        if (atomic_load_explicit(&client->stop_requested, memory_order_acquire)) { result = ERROR_SUCCESS; break; }
        if (!had_connection || client->directory || result == ERROR_ACCESS_DENIED ||
            result == ERROR_INVALID_PASSWORD || result == ERROR_INVALID_DATA) break;
        if (!recovery_deadline) recovery_deadline = GetTickCount64() + 120000;
        if (GetTickCount64() >= recovery_deadline) break;
        if (WaitForSingleObject((HANDLE) client->wake_event, 1000) == WAIT_FAILED) break;
    }
    push_lifecycle(client, NativeSignalingClientEvent_Disconnected, result,
        result == ERROR_SUCCESS ? "Room signaling stopped" :
        result == ERROR_INVALID_PASSWORD ? "Enter the lobby password and try again" :
        result == ERROR_ACCESS_DENIED ? "This lobby connection was closed or replaced" : "Room signaling disconnected");
    atomic_store_explicit(&client->running, false, memory_order_release);
    return result;
}

bool nativeSignalingClient_Start(NativeSignalingClient* client,
                                 const NativeSignalingClientConfig* config)
{
    if (client == NULL || config == NULL || client->thread_handle != 0 ||
        !copy_bounded(client->endpoint, sizeof(client->endpoint),
                      config->endpoint) ||
        (!config->directory && (!valid_room_id(config->room_id) || !valid_peer_id(config->peer_id))) ||
        (!config->directory && (!copy_bounded(client->room_id, sizeof(client->room_id), config->room_id) ||
                               !copy_bounded(client->peer_id, sizeof(client->peer_id), config->peer_id))) ||
        !copy_bounded(client->name, sizeof(client->name), config->name) ||
        !copy_bounded(client->build, sizeof(client->build), config->build) ||
        config->local_players > 4)
    {
        return false;
    }
    client->admission[0] = 0;
    client->room_password[0] = 0;
    if (config->room_password) {
        size_t length = strnlen(config->room_password, sizeof(client->room_password));
        if (length == sizeof(client->room_password)) return false;
        for (size_t i = 0; i < length; ++i)
            if (config->room_password[i] < '!' || config->room_password[i] > '~') return false;
        memcpy(client->room_password, config->room_password, length + 1);
    }
    client->guest_key[0] = 0;
    if (config->guest_key) {
        if (strnlen(config->guest_key, 65) != 64) return false;
        for (unsigned i = 0; i < 64; ++i)
            if (!isxdigit((unsigned char) config->guest_key[i]) ||
                (config->guest_key[i] >= 'A' && config->guest_key[i] <= 'F')) return false;
        memcpy(client->guest_key, config->guest_key, 65);
    }
    if (config->admission && config->admission[0]) {
        if (strnlen(config->admission, 33) != 32) return false;
        for (unsigned i = 0; i < 32; ++i) {
            char c = config->admission[i];
            if (!(c >= 'a' && c <= 'f') && !(c >= '0' && c <= '9')) return false;
        }
        memcpy(client->admission, config->admission, 33);
    }
    memset(&client->commands, 0, sizeof(client->commands));
    memset(&client->events, 0, sizeof(client->events));
    memset(&client->chat, 0, sizeof(client->chat));
    client->local_players = config->local_players;
    client->directory = config->directory;
    if (client->directory) client->room_id[0] = client->peer_id[0] = 0;
    atomic_store_explicit(&client->stop_requested, false, memory_order_relaxed);
    atomic_store_explicit(&client->running, true, memory_order_release);
    HANDLE wake = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (wake == NULL) {
        atomic_store_explicit(&client->running, false, memory_order_release);
        return false;
    }
    client->wake_event = (uintptr_t) wake;
    HANDLE thread = CreateThread(NULL, 0, signaling_thread, client, 0, NULL);
    if (thread == NULL) {
        CloseHandle(wake);
        client->wake_event = 0;
        atomic_store_explicit(&client->running, false, memory_order_release);
        return false;
    }
    client->thread_handle = (uintptr_t) thread;
    return true;
}

void nativeSignalingClient_Stop(NativeSignalingClient* client)
{
    if (client == NULL || client->thread_handle == 0) {
        return;
    }
    atomic_store_explicit(&client->stop_requested, true, memory_order_release);
    SetEvent((HANDLE) client->wake_event);
    HANDLE thread = (HANDLE) client->thread_handle;
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    CloseHandle((HANDLE) client->wake_event);
    client->thread_handle = 0;
    client->wake_event = 0;
    SecureZeroMemory(client->room_password, sizeof(client->room_password));
    SecureZeroMemory(&client->commands, sizeof(client->commands));
}

bool nativeSignalingClient_QueueUpdate(NativeSignalingClient* client,
                                       const NativeSignalingUpdate* update)
{
    uint8_t bytes[NATIVE_SIGNALING_MAX_MESSAGE_BYTES];
    size_t size = nativeSignalingWire_EncodeUpdate(update, bytes,
                                                   sizeof(bytes));
    bool queued = client != NULL && size != 0 &&
                  nativeSignalingClient_IsRunning(client) &&
                  command_push(&client->commands, bytes, size);
    if (queued) SetEvent((HANDLE) client->wake_event);
    return queued;
}

bool nativeSignalingClient_QueueSignal(NativeSignalingClient* client,
                                       const char* target_peer_id,
                                       const uint8_t* payload,
                                       uint16_t payload_size)
{
    uint8_t bytes[NATIVE_SIGNALING_MAX_MESSAGE_BYTES];
    size_t size = nativeSignalingWire_EncodeSignal(
        target_peer_id, payload, payload_size, bytes, sizeof(bytes));
    bool queued = client != NULL && size != 0 &&
                  nativeSignalingClient_IsRunning(client) &&
                  command_push(&client->commands, bytes, size);
    if (queued) SetEvent((HANDLE) client->wake_event);
    return queued;
}

bool nativeSignalingClient_RequestLobbies(NativeSignalingClient* client, uint16_t offset)
{
    uint8_t bytes[10];
    size_t size = nativeSignalingWire_EncodeLobbies(offset, bytes, sizeof(bytes));
    bool queued = client && size && nativeSignalingClient_IsRunning(client) && command_push(&client->commands, bytes, size);
    if (queued) SetEvent((HANDLE) client->wake_event);
    return queued;
}
bool nativeSignalingClient_SetListing(NativeSignalingClient* client, const char* title, bool listed,
                                     uint8_t fighter_limit, const char* password)
{
    uint8_t bytes[144];
    size_t size = nativeSignalingWire_EncodeListing(title, listed, fighter_limit, password, bytes, sizeof(bytes));
    bool queued = client && size && nativeSignalingClient_IsRunning(client) && command_push(&client->commands, bytes, size);
    SecureZeroMemory(bytes, sizeof(bytes));
    if (queued) SetEvent((HANDLE) client->wake_event);
    return queued;
}
bool nativeSignalingClient_RequestPresence(NativeSignalingClient* client)
{
    uint8_t bytes[8];
    size_t size = nativeSignalingWire_EncodeRequestPresence(bytes,
                                                            sizeof(bytes));
    bool queued = client != NULL && size != 0 &&
                  nativeSignalingClient_IsRunning(client) &&
                  command_push(&client->commands, bytes, size);
    if (queued) SetEvent((HANDLE) client->wake_event);
    return queued;
}

bool nativeSignalingClient_SetMatch(NativeSignalingClient* client,
                                    const uint8_t* payload,
                                    uint16_t payload_size)
{
    uint8_t bytes[NATIVE_SIGNALING_MAX_MATCH_BYTES + 10];
    size_t size = nativeSignalingWire_EncodeSetMatch(
        payload, payload_size, bytes, sizeof(bytes));
    bool queued = client != NULL && size != 0 &&
                  nativeSignalingClient_IsRunning(client) &&
                  command_push(&client->commands, bytes, size);
    if (queued) SetEvent((HANDLE) client->wake_event);
    return queued;
}

bool nativeSignalingClient_AssignSeats(NativeSignalingClient* client,
                                       const char* peer_id,
                                       uint8_t active_mask,
                                       uint8_t waiting_mask)
{
    uint8_t bytes[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 12];
    size_t size = nativeSignalingWire_EncodeAssignSeats(
        peer_id, active_mask, waiting_mask, bytes, sizeof(bytes));
    bool queued = client != NULL && size != 0 &&
                  nativeSignalingClient_IsRunning(client) &&
                  command_push(&client->commands, bytes, size);
    if (queued) SetEvent((HANDLE) client->wake_event);
    return queued;
}

bool nativeSignalingClient_SetRotation(NativeSignalingClient* client,
                                       uint8_t policy)
{
    uint8_t bytes[9];
    size_t size = nativeSignalingWire_EncodeSetRotation(
        policy, bytes, sizeof(bytes));
    bool queued = client != NULL && size != 0 &&
                  nativeSignalingClient_IsRunning(client) &&
                  command_push(&client->commands, bytes, size);
    if (queued) SetEvent((HANDLE) client->wake_event);
    return queued;
}

bool nativeSignalingClient_SetLobbyRules(NativeSignalingClient* client, const NativeRoomRules* rules)
{
    uint8_t bytes[NATIVE_ROOM_RULES_BYTES+10];
    size_t size=nativeSignalingWire_EncodeLobbyRules(rules,bytes,sizeof(bytes));
    bool queued=client && size && nativeSignalingClient_IsRunning(client) && command_push(&client->commands,bytes,size);
    if (queued) SetEvent((HANDLE)client->wake_event);
    return queued;
}

bool nativeSignalingClient_StageCursor(NativeSignalingClient* client,
                                       const NativeStageCursor* cursor)
{
    uint8_t bytes[34];
    size_t size = nativeSignalingWire_EncodeStageCursor(cursor, bytes, sizeof(bytes));
    bool queued = client && size && nativeSignalingClient_IsRunning(client) &&
                  command_push(&client->commands, bytes, size);
    if (queued) SetEvent((HANDLE) client->wake_event);
    return queued;
}

bool nativeSignalingClient_RequestLobbyState(NativeSignalingClient* client)
{
    uint8_t bytes[8];
    size_t size=nativeSignalingWire_EncodeRequestLobbyState(bytes,sizeof(bytes));
    bool queued=client && size && nativeSignalingClient_IsRunning(client) && command_push(&client->commands,bytes,size);
    if (queued) SetEvent((HANDLE)client->wake_event);
    return queued;
}

bool nativeSignalingClient_ReportMatch(NativeSignalingClient* client,
                                       uint64_t epoch,
                                       const char* winner_peer_id,
                                       uint8_t winner_seat,
                                       bool eligible, uint8_t loser_port)
{
    uint8_t bytes[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 20];
    size_t size = nativeSignalingWire_EncodeReportMatch(
        epoch, winner_peer_id, winner_seat, eligible, loser_port,
        bytes, sizeof(bytes));
    bool queued = client != NULL && size != 0 &&
                  nativeSignalingClient_IsRunning(client) &&
                  command_push(&client->commands, bytes, size);
    if (queued) SetEvent((HANDLE) client->wake_event);
    return queued;
}

bool nativeSignalingClient_PickLobbyStage(NativeSignalingClient* client,
                                          uint64_t epoch, uint8_t seat,
                                          uint8_t stage)
{
    uint8_t bytes[18];
    size_t size = nativeSignalingWire_EncodePickLobbyStage(epoch, seat, stage,
                                                          bytes, sizeof(bytes));
    bool queued = client && size && nativeSignalingClient_IsRunning(client) &&
                  command_push(&client->commands, bytes, size);
    if (queued) SetEvent((HANDLE) client->wake_event);
    return queued;
}

bool nativeSignalingClient_ConfigureQuick(NativeSignalingClient* client, uint8_t mode)
{
    uint8_t bytes[9];
    size_t size = nativeSignalingWire_EncodeConfigureQuick(mode, bytes, sizeof(bytes));
    bool queued = client && size && nativeSignalingClient_IsRunning(client) &&
        command_push(&client->commands, bytes, size);
    if (queued) SetEvent((HANDLE) client->wake_event);
    return queued;
}

bool nativeSignalingClient_QueueQuick(NativeSignalingClient* client, bool searching)
{
    uint8_t bytes[9];
    size_t size = nativeSignalingWire_EncodeQueueQuick(searching, bytes, sizeof(bytes));
    bool queued = client && size && nativeSignalingClient_IsRunning(client) &&
        command_push(&client->commands, bytes, size);
    if (queued) SetEvent((HANDLE) client->wake_event);
    return queued;
}

bool nativeSignalingClient_ReportQuick(NativeSignalingClient* client, uint64_t epoch,
                                       uint8_t winner)
{
    uint8_t bytes[17];
    size_t size = nativeSignalingWire_EncodeReportQuick(epoch, winner, bytes, sizeof(bytes));
    bool queued = client && size && nativeSignalingClient_IsRunning(client) &&
        command_push(&client->commands, bytes, size);
    if (queued) SetEvent((HANDLE) client->wake_event);
    return queued;
}

bool nativeSignalingClient_VoteQuickStage(NativeSignalingClient* client,
                                          uint64_t epoch, uint8_t seat, uint8_t stage)
{
    uint8_t bytes[18];
    size_t size = nativeSignalingWire_EncodeVoteQuickStage(epoch, seat, stage, bytes, sizeof(bytes));
    bool queued = client && size && nativeSignalingClient_IsRunning(client) &&
        command_push(&client->commands, bytes, size);
    if (queued) SetEvent((HANDLE) client->wake_event);
    return queued;
}

bool nativeSignalingClient_UnreadyQuick(NativeSignalingClient* client,
                                        uint64_t completed_epoch)
{
    uint8_t bytes[16];
    size_t size = nativeSignalingWire_EncodeUnreadyQuick(completed_epoch, bytes, sizeof(bytes));
    bool queued = client && size && nativeSignalingClient_IsRunning(client) &&
        command_push(&client->commands, bytes, size);
    if (queued) SetEvent((HANDLE) client->wake_event);
    return queued;
}

bool nativeSignalingClient_PrepareQuick(NativeSignalingClient* client,
                                        uint64_t revision, bool prepared,
                                        const uint8_t* digest)
{
    uint8_t bytes[51];
    size_t size = nativeSignalingWire_EncodePrepareQuick(revision, prepared, digest, bytes, sizeof(bytes));
    bool queued = client && size && nativeSignalingClient_IsRunning(client) &&
        command_push(&client->commands, bytes, size);
    if (queued) SetEvent((HANDLE) client->wake_event);
    return queued;
}

bool nativeSignalingClient_RequestQuick(NativeSignalingClient* client)
{
    uint8_t bytes[8];
    size_t size = nativeSignalingWire_EncodeRequestQuick(bytes, sizeof(bytes));
    bool queued = client && size && nativeSignalingClient_IsRunning(client) &&
        command_push(&client->commands, bytes, size);
    if (queued) SetEvent((HANDLE) client->wake_event);
    return queued;
}

bool nativeSignalingClient_RequestChat(NativeSignalingClient* client)
{
    uint8_t bytes[8];
    size_t size = nativeSignalingWire_EncodeRequestChat(bytes, sizeof(bytes));
    bool queued = client && size && nativeSignalingClient_IsRunning(client) &&
        command_push(&client->commands, bytes, size);
    if (queued) SetEvent((HANDLE) client->wake_event);
    return queued;
}

bool nativeSignalingClient_PostChat(NativeSignalingClient* client, uint32_t nonce, const char* text)
{
    uint8_t bytes[NATIVE_ROOM_CHAT_TEXT_BYTES + 14];
    size_t size = nativeSignalingWire_EncodePostChat(nonce, text, bytes, sizeof(bytes));
    bool queued = client && size && nativeSignalingClient_IsRunning(client) &&
        command_push(&client->commands, bytes, size);
    if (queued) SetEvent((HANDLE) client->wake_event);
    return queued;
}

bool nativeSignalingClient_PollChat(NativeSignalingClient* client, NativeRoomChatEvent* event)
{
    if (!client || !event) return false;
    NativeSignalingChatQueue* queue = &client->chat;
    uint32_t read = atomic_load_explicit(&queue->read, memory_order_relaxed);
    uint32_t write = atomic_load_explicit(&queue->write, memory_order_acquire);
    if (read == write) return false;
    *event = queue->slots[read % NATIVE_SIGNALING_CHAT_CAPACITY];
    atomic_store_explicit(&queue->read, read + 1, memory_order_release);
    return true;
}

bool nativeSignalingClient_Poll(NativeSignalingClient* client,
                                NativeSignalingClientEvent* event)
{
    return client != NULL && event != NULL && event_pop(&client->events, event);
}

bool nativeSignalingClient_IsRunning(const NativeSignalingClient* client)
{
    return client != NULL &&
           atomic_load_explicit(&client->running, memory_order_acquire);
}

#endif
