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
    RECEIVE_TIMEOUT_MS = 50,
    CONNECT_TIMEOUT_MS = 5000,
    SEND_TIMEOUT_MS = 2000,
};

typedef struct SignalingConnection {
    HINTERNET session;
    HINTERNET connection;
    HINTERNET request;
    HINTERNET socket;
} SignalingConnection;

typedef struct SignalingSenderContext {
    NativeSignalingClient* client;
    HINTERNET socket;
} SignalingSenderContext;

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
    *output = queue->slots[read % NATIVE_SIGNALING_COMMAND_CAPACITY];
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

static void close_connection(SignalingConnection* connection)
{
    if (connection->socket != NULL) {
        WinHttpCloseHandle(connection->socket);
    }
    if (connection->request != NULL) {
        WinHttpCloseHandle(connection->request);
    }
    if (connection->connection != NULL) {
        WinHttpCloseHandle(connection->connection);
    }
    if (connection->session != NULL) {
        WinHttpCloseHandle(connection->session);
    }
    *connection = (SignalingConnection) { 0 };
}

static DWORD open_connection(const NativeSignalingClient* client,
                             SignalingConnection* output)
{
    wchar_t url[1536];
    if (!build_url(client, url, sizeof(url) / sizeof(url[0]))) {
        return ERROR_INVALID_PARAMETER;
    }
    URL_COMPONENTS components = {
        .dwStructSize = sizeof(components),
        .dwHostNameLength = (DWORD) -1,
        .dwUrlPathLength = (DWORD) -1,
        .dwExtraInfoLength = (DWORD) -1,
    };
    if (!WinHttpCrackUrl(url, 0, 0, &components)) {
        DWORD error = GetLastError();
        return error == ERROR_SUCCESS ? ERROR_INVALID_PARAMETER : error;
    }
    if (components.nScheme != INTERNET_SCHEME_HTTP &&
        components.nScheme != INTERNET_SCHEME_HTTPS)
    {
        return ERROR_INVALID_PARAMETER;
    }
    wchar_t host[256];
    wchar_t object[1280];
    if (components.dwHostNameLength >= sizeof(host) / sizeof(host[0]) ||
        components.dwUrlPathLength + components.dwExtraInfoLength >=
            sizeof(object) / sizeof(object[0]))
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }
    memcpy(host, components.lpszHostName,
           components.dwHostNameLength * sizeof(wchar_t));
    host[components.dwHostNameLength] = L'\0';
    memcpy(object, components.lpszUrlPath,
           components.dwUrlPathLength * sizeof(wchar_t));
    if (components.dwExtraInfoLength != 0) {
        memcpy(object + components.dwUrlPathLength, components.lpszExtraInfo,
               components.dwExtraInfoLength * sizeof(wchar_t));
    }
    object[components.dwUrlPathLength + components.dwExtraInfoLength] = L'\0';

    SignalingConnection connection = { 0 };
    connection.session = WinHttpOpen(
        L"Freak Melee/0.1", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (connection.session == NULL) {
        return GetLastError();
    }
    if (!WinHttpSetTimeouts(connection.session, CONNECT_TIMEOUT_MS,
                            CONNECT_TIMEOUT_MS, SEND_TIMEOUT_MS,
                            RECEIVE_TIMEOUT_MS))
    {
        DWORD error = GetLastError();
        close_connection(&connection);
        return error;
    }
    connection.connection =
        WinHttpConnect(connection.session, host, components.nPort, 0);
    if (connection.connection == NULL) {
        DWORD error = GetLastError();
        close_connection(&connection);
        return error;
    }
    DWORD flags = components.nScheme == INTERNET_SCHEME_HTTPS
                      ? WINHTTP_FLAG_SECURE
                      : 0;
    connection.request = WinHttpOpenRequest(
        connection.connection, L"GET", object, NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    wchar_t headers[192] = { 0 };
    if (!client->directory) nativeAccountTransport_Headers(url, headers, 192);
    DWORD redirects = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    DWORD disabled = WINHTTP_DISABLE_COOKIES | WINHTTP_DISABLE_AUTHENTICATION;
    if (connection.request == NULL ||
        !WinHttpSetOption(connection.request, WINHTTP_OPTION_REDIRECT_POLICY, &redirects, sizeof(redirects)) ||
        !WinHttpSetOption(connection.request, WINHTTP_OPTION_DISABLE_FEATURE, &disabled, sizeof(disabled)) ||
        !WinHttpSetOption(connection.request, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET,
                          NULL, 0) ||
        !WinHttpSendRequest(connection.request, headers, (DWORD) -1,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(connection.request, NULL))
    {
        DWORD error = GetLastError();
        SecureZeroMemory(headers, sizeof(headers));
        close_connection(&connection);
        return error;
    }
    SecureZeroMemory(headers, sizeof(headers));
    DWORD status = 0;
    DWORD status_size = sizeof(status);
    if (!WinHttpQueryHeaders(
            connection.request,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size,
            WINHTTP_NO_HEADER_INDEX) ||
        status != 101)
    {
        DWORD error = status == 0 ? GetLastError() : ERROR_BAD_NET_RESP;
        close_connection(&connection);
        return error;
    }
    connection.socket =
        WinHttpWebSocketCompleteUpgrade(connection.request, 0);
    if (connection.socket == NULL) {
        DWORD error = GetLastError();
        close_connection(&connection);
        return error;
    }
    WinHttpCloseHandle(connection.request);
    connection.request = NULL;
    *output = connection;
    return ERROR_SUCCESS;
}

static DWORD send_commands(NativeSignalingClient* client, HINTERNET socket)
{
    NativeSignalingCommandSlot command;
    while (command_pop(&client->commands, &command)) {
        DWORD result = WinHttpWebSocketSend(
            socket, WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE,
            command.bytes, command.size);
        if (result != ERROR_SUCCESS) {
            return result;
        }
    }
    return ERROR_SUCCESS;
}

static DWORD receive_message(NativeSignalingClient* client, HINTERNET socket,
                             uint8_t* buffer, size_t* buffered)
{
    if (*buffered >= NATIVE_SIGNALING_MAX_MESSAGE_BYTES) {
        return ERROR_INSUFFICIENT_BUFFER;
    }
    DWORD received = 0;
    WINHTTP_WEB_SOCKET_BUFFER_TYPE type;
    DWORD result = WinHttpWebSocketReceive(
        socket, buffer + *buffered,
        (DWORD) (NATIVE_SIGNALING_MAX_MESSAGE_BYTES - *buffered), &received,
        &type);
    if (result != ERROR_SUCCESS) {
        return result;
    }
    if (type == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE) {
        return ERROR_CONNECTION_ABORTED;
    }
    if (type != WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE &&
        type != WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE)
    {
        return ERROR_INVALID_DATA;
    }
    *buffered += received;
    if (type == WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE) {
        return ERROR_MORE_DATA;
    }
    NativeSignalingClientEvent event = {
        .type = NativeSignalingClientEvent_Message,
    };
    if (!nativeSignalingWire_DecodeServer(buffer, *buffered, &event.message)) {
        return ERROR_INVALID_DATA;
    }
    *buffered = 0;
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

static DWORD WINAPI signaling_sender_thread(void* context)
{
    SignalingSenderContext* sender = context;
    NativeSignalingClient* client = sender->client;
    HANDLE wake = (HANDLE) client->wake_event;
    DWORD result = ERROR_SUCCESS;
    for (;;) {
        WaitForSingleObject(wake, INFINITE);
        bool stopping = atomic_load_explicit(&client->stop_requested,
                                             memory_order_acquire);
        result = send_commands(client, sender->socket);
        if (result != ERROR_SUCCESS) {
            atomic_store_explicit(&client->worker_error, result,
                                  memory_order_release);
            atomic_store_explicit(&client->stop_requested, true,
                                  memory_order_release);
            CancelSynchronousIo((HANDLE) client->thread_handle);
            break;
        }
        if (stopping) {
            break;
        }
    }
    return result;
}

static DWORD WINAPI signaling_thread(void* context)
{
    NativeSignalingClient* client = context;
    SignalingConnection connection = { 0 };
    DWORD result = open_connection(client, &connection);
    if (result != ERROR_SUCCESS) {
        push_lifecycle(client, NativeSignalingClientEvent_Disconnected, result,
                       "WebSocket connection failed");
        atomic_store_explicit(&client->running, false, memory_order_release);
        return result;
    }
    push_lifecycle(client, NativeSignalingClientEvent_Connected, 0,
                   "Room signaling connected");

    SignalingSenderContext sender_context = {
        .client = client,
        .socket = connection.socket,
    };
    HANDLE sender = CreateThread(NULL, 0, signaling_sender_thread,
                                 &sender_context, 0, NULL);
    if (sender == NULL) {
        result = GetLastError();
        close_connection(&connection);
        push_lifecycle(client, NativeSignalingClientEvent_Disconnected, result,
                       "Could not start signaling send lane");
        atomic_store_explicit(&client->running, false, memory_order_release);
        return result;
    }
    SetEvent((HANDLE) client->wake_event);

    uint8_t receive_buffer[NATIVE_SIGNALING_MAX_MESSAGE_BYTES];
    size_t buffered = 0;
    while (!atomic_load_explicit(&client->stop_requested,
                                 memory_order_acquire))
    {
        result = receive_message(client, connection.socket, receive_buffer,
                                 &buffered);
        if (result == ERROR_MORE_DATA) {
            continue;
        }
        if (result != ERROR_SUCCESS) {
            break;
        }
    }
    bool requested = atomic_load_explicit(&client->stop_requested,
                                          memory_order_acquire);
    atomic_store_explicit(&client->stop_requested, true, memory_order_release);
    SetEvent((HANDLE) client->wake_event);
    WaitForSingleObject(sender, INFINITE);
    CloseHandle(sender);
    uint32_t sender_error = atomic_load_explicit(&client->worker_error,
                                                 memory_order_acquire);
    if (sender_error != ERROR_SUCCESS) {
        result = sender_error;
        requested = false;
    }
    close_connection(&connection);
    push_lifecycle(client, NativeSignalingClientEvent_Disconnected,
                   requested ? 0 : result,
                   requested ? "Room signaling stopped"
                             : "Room signaling disconnected");
    atomic_store_explicit(&client->running, false, memory_order_release);
    return requested ? ERROR_SUCCESS : result;
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
    atomic_store_explicit(&client->worker_error, ERROR_SUCCESS,
                          memory_order_relaxed);
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
    uint8_t wake_message[8];
    size_t wake_size = nativeSignalingWire_EncodeRequestPresence(
        wake_message, sizeof(wake_message));
    if (wake_size != 0) {
        command_push(&client->commands, wake_message, wake_size);
    }
    atomic_store_explicit(&client->stop_requested, true, memory_order_release);
    SetEvent((HANDLE) client->wake_event);
    HANDLE thread = (HANDLE) client->thread_handle;
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    CloseHandle((HANDLE) client->wake_event);
    client->thread_handle = 0;
    client->wake_event = 0;
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
bool nativeSignalingClient_SetListing(NativeSignalingClient* client, const char* title, bool listed)
{
    uint8_t bytes[80];
    size_t size = nativeSignalingWire_EncodeListing(title, listed, bytes, sizeof(bytes));
    bool queued = client && size && nativeSignalingClient_IsRunning(client) && command_push(&client->commands, bytes, size);
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
