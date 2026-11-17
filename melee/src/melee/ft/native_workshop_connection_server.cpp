#include "native_workshop_connection_security.h"
#include <algorithm>
#include <cstring>
#include <cstdlib>

namespace NativeWorkshopConnection {
namespace {
void clear_io(Server& s)
{
    HANDLE event = s.io.hEvent;
    s.io = {}; s.io.hEvent = event;
    ResetEvent(event); s.pending = false; s.immediate = 0;
}
void close_pipe(Server& s)
{
    if (s.pipe != INVALID_HANDLE_VALUE) { DisconnectNamedPipe(s.pipe); CloseHandle(s.pipe); }
    s.pipe = INVALID_HANDLE_VALUE; s.phase = Server::Off; s.enabled = false;
}
void listen(Server& s)
{
    clear_io(s);
    s.phase = Server::Listening; s.offset = s.length = 0;
    if (ConnectNamedPipe(s.pipe, &s.io)) { s.immediate = 1; return; }
    DWORD code = GetLastError();
    if (code == ERROR_IO_PENDING) s.pending = true;
    else if (code == ERROR_PIPE_CONNECTED) s.immediate = 1;
    else close_pipe(s);
}
void reset(Server& s)
{
    if (s.pending) {
        CancelIoEx(s.pipe, &s.io); s.phase = Server::Closing;
    } else {
        DisconnectNamedPipe(s.pipe);
        if (s.enabled) listen(s); else close_pipe(s);
    }
}
void transfer(Server& s, Server::Phase phase)
{
    clear_io(s); s.phase = phase;
    void* data; DWORD length;
    if (phase == Server::Header) { data = (char*)&s.length + s.offset; length = 4 - s.offset; }
    else if (phase == Server::Body) { data = s.buffer.data() + s.offset; length = s.length - s.offset; }
    else if (phase == Server::Acknowledgment) { data = &s.acknowledgment; length = 1; }
    else { data = s.buffer.data() + s.offset; length = s.length - s.offset; }
    BOOL done = phase == Server::Writing
        ? WriteFile(s.pipe, data, length, &s.immediate, &s.io)
        : ReadFile(s.pipe, data, length, &s.immediate, &s.io);
    if (!done) {
        if (GetLastError() == ERROR_IO_PENDING) s.pending = true;
        else reset(s);
    }
}
const char* text(yyjson_val* object, const char* key)
{
    auto* value = yyjson_obj_get(object, key);
    const char* string = yyjson_get_str(value);
    return string && std::strlen(string) == yyjson_get_len(value) ? string : "";
}
void dispatch(Server& s)
{
    auto* request = yyjson_read(s.buffer.data(), s.length, 0);
    auto* root = yyjson_doc_get_root(request);
    auto* output = yyjson_mut_doc_new(nullptr);
    if (!output) { yyjson_doc_free(request); reset(s); return; }
    auto* envelope = yyjson_mut_obj(output); yyjson_mut_doc_set_root(output, envelope);
    NativeAuthoring::Reply reply(output);
    DWORD client = 0;
    const char* id = text(root, "id");
    const char* method = text(root, "method");
    if (!GetNamedPipeClientProcessId(s.pipe, &client) || !process_is_current_user(client))
        reply.fail("wrong_user", "Authoring requests must come from the same Windows user.");
    else if (!request || !yyjson_is_obj(root) || std::strlen(id) != 32 || !*method ||
             !yyjson_is_obj(yyjson_obj_get(root, "arguments")))
        reply.fail("invalid_request", "Expected a correlated authoring request and argument object.");
    else if (std::strcmp(text(yyjson_obj_get(root, "arguments"), "instance"), s.instance.c_str()))
        reply.fail("wrong_instance", "Request instance does not match this connection.");
    else if (!std::strcmp(method, "connection.describe")) {
        char executable[32768];
        DWORD size = GetModuleFileNameA(nullptr, executable, sizeof(executable));
        NativeAuthoring::add_string(reply, reply.result, "instance", s.instance);
        yyjson_mut_obj_add_str(output, reply.result, "build_identity", NativeAuthoring::build_identity());
        yyjson_mut_obj_add_str(output, reply.result, "source_revision", NativeAuthoring::source_revision());
        yyjson_mut_obj_add_str(output, reply.result, "build", NativeAuthoring::release_label());
        yyjson_mut_obj_add_uint(output, reply.result, "process_id", GetCurrentProcessId());
        if (size && size < sizeof(executable)) NativeAuthoring::add_string(reply, reply.result, "executable", executable);
    }
    else if (std::strcmp(text(root, "build_identity"), NativeAuthoring::build_identity()))
        reply.fail("build_mismatch", "The tool and running game are from different builds. Use the tool beside this game's executable, listed by instances.list.");
    else {
        s.request_id = id;
        s.handler(method, yyjson_obj_get(root, "arguments"), reply, s.context);
    }
    yyjson_mut_obj_add_strcpy(output, envelope, "id", id);
    yyjson_mut_obj_add_str(output, envelope, "build_identity", NativeAuthoring::build_identity());
    yyjson_mut_obj_add_bool(output, envelope, "ok", reply.ok);
    yyjson_mut_obj_add_val(output, envelope, reply.ok ? "result" : "error", reply.result);
    size_t size = 0;
    char* bytes = yyjson_mut_write(output, 0, &size);
    if (!bytes || size > MaxMessage) {
        std::free(bytes); yyjson_mut_doc_free(output); yyjson_doc_free(request); reset(s); return;
    }
    uint32_t length = (uint32_t)size;
    std::memcpy(s.buffer.data(), &length, 4);
    std::memcpy(s.buffer.data() + 4, bytes, size);
    std::free(bytes); yyjson_mut_doc_free(output); yyjson_doc_free(request);
    s.offset = 0; s.length = length + 4;
    transfer(s, Server::Writing);
}
}

bool start(Server& s, Handler handler, void* context, std::string& error)
{
    if (s.phase != Server::Off || !handler) { error = "Authoring connection is already open."; return false; }
    std::string random = random_id();
    if (random.empty()) { error = "Could not create the authoring instance identity."; return false; }
    s.instance = std::to_string(GetCurrentProcessId()) + '-' + random;
    auto* descriptor = owner_descriptor();
    if (!descriptor) { error = "Could not restrict the authoring connection to this Windows user."; return false; }
    SECURITY_ATTRIBUTES security{sizeof(security), descriptor, FALSE};
    s.pipe = CreateNamedPipeA(pipe_name(s.instance).c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
        1, 65536, 65536, 0, &security);
    LocalFree(descriptor);
    if (s.pipe == INVALID_HANDLE_VALUE) { error = "Could not create the local authoring connection."; return false; }
    if (!s.io.hEvent) s.io.hEvent = CreateEventA(nullptr, TRUE, FALSE, nullptr);
    if (!s.io.hEvent) { close_pipe(s); error = "Could not create the authoring I/O event."; return false; }
    s.buffer.resize(MaxMessage + 4);
    s.handler = handler; s.context = context; s.enabled = true;
    listen(s);
    if (s.phase == Server::Off) { error = "Could not listen for authoring requests."; return false; }
    return true;
}

void poll(Server& s)
{
    // Bound work in a tick even if a client keeps a stream of requests ready.
    for (unsigned transitions = 0; transitions < 8 && s.phase != Server::Off; ++transitions) {
        if (s.phase != Server::Listening && s.phase != Server::Closing && GetTickCount64() > s.deadline) { reset(s); return; }
        DWORD count = s.immediate;
        if (s.pending) {
            if (WaitForSingleObject(s.io.hEvent, 0) != WAIT_OBJECT_0) return;
            bool done = GetOverlappedResult(s.pipe, &s.io, &count, FALSE) != 0;
            s.pending = false;
            if (s.phase == Server::Closing) {
                if (s.enabled) { DisconnectNamedPipe(s.pipe); listen(s); } else close_pipe(s);
                continue;
            }
            if (!done) { reset(s); return; }
        }
        if (s.phase == Server::Listening) {
            s.deadline = GetTickCount64() + 15000; s.offset = s.length = 0;
            transfer(s, Server::Header); continue;
        }
        if (!count) { reset(s); return; }
        s.offset += count;
        if (s.phase == Server::Header) {
            if (s.offset < 4) { transfer(s, Server::Header); continue; }
            if (!s.length || s.length > MaxMessage) { reset(s); return; }
            s.offset = 0; transfer(s, Server::Body);
        } else if (s.phase == Server::Body) {
            if (s.offset < s.length) { transfer(s, Server::Body); continue; }
            dispatch(s);
        } else if (s.phase == Server::Writing) {
            if (s.offset < s.length) { transfer(s, Server::Writing); continue; }
            s.offset = 0; transfer(s, Server::Acknowledgment);
        } else if (s.phase == Server::Acknowledgment) {
            // The client acknowledges a complete reply before disconnection;
            // DisconnectNamedPipe otherwise discards bytes not yet read.
            reset(s); return;
        }
    }
}
void stop(Server& s) { s.enabled = false; if (s.phase != Server::Off) reset(s); }
void shutdown(Server& s)
{
    stop(s);
    if (s.pending) {
        // Shutdown is an explicit transition, never the frame polling path.
        DWORD ignored; GetOverlappedResult(s.pipe, &s.io, &ignored, TRUE);
        s.pending = false;
    }
    close_pipe(s);
    if (s.io.hEvent) { CloseHandle(s.io.hEvent); s.io.hEvent = nullptr; }
    s.buffer.clear();
}
}
