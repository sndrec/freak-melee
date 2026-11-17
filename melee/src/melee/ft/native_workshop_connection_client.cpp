#include "native_workshop_connection_security.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace NativeWorkshopConnection {
std::vector<std::string> instances()
{
    std::vector<std::string> result;
    WIN32_FIND_DATAA entry{};
    const std::string pattern = std::string("\\\\.\\pipe\\") + Prefix + '*';
    HANDLE search = FindFirstFileA(pattern.c_str(), &entry);
    if (search == INVALID_HANDLE_VALUE) return result;
    do {
        const std::string name = entry.cFileName;
        const size_t prefix = std::strlen(Prefix);
        if (name.compare(0, prefix, Prefix)) continue;
        std::string instance = name.substr(prefix);
        if (instance_valid(instance)) result.push_back(instance);
    } while (FindNextFileA(search, &entry));
    FindClose(search);
    std::sort(result.begin(), result.end());
    return result;
}

namespace {
bool transfer(HANDLE pipe, HANDLE event, void* memory, DWORD size, bool write, uint64_t deadline)
{
    auto* bytes = (unsigned char*)memory;
    while (size) {
        OVERLAPPED io{}; io.hEvent = event; ResetEvent(event);
        DWORD count = 0;
        BOOL done = write ? WriteFile(pipe, bytes, size, &count, &io) : ReadFile(pipe, bytes, size, &count, &io);
        if (!done && GetLastError() == ERROR_IO_PENDING) {
            uint64_t now = GetTickCount64();
            DWORD remaining = now < deadline ? (DWORD)std::min<uint64_t>(deadline - now, MAXDWORD) : 0;
            if (WaitForSingleObject(event, remaining) != WAIT_OBJECT_0) {
                CancelIoEx(pipe, &io);
                // The stack OVERLAPPED and buffer must outlive cancellation.
                GetOverlappedResult(pipe, &io, &count, TRUE);
                return false;
            }
            done = GetOverlappedResult(pipe, &io, &count, FALSE);
        }
        if (!done || !count || count > size) return false;
        bytes += count; size -= count;
    }
    return true;
}
}

bool request(const std::string& instance, const char* method, yyjson_val* arguments,
             NativeAuthoring::Reply& reply, unsigned timeout_ms)
{
    if (!instance_valid(instance)) { reply.fail("invalid_instance", "Use an instance returned by instances.list."); return false; }
    const std::string id = random_id();
    if (id.empty()) { reply.fail("request_id_failed", "Could not create request identity."); return false; }
    auto* doc = yyjson_mut_doc_new(nullptr);
    if (!doc) { reply.fail("out_of_memory", "Could not allocate request."); return false; }
    auto* root = yyjson_mut_obj(doc); yyjson_mut_doc_set_root(doc, root);
    yyjson_mut_obj_add_strcpy(doc, root, "id", id.c_str());
    yyjson_mut_obj_add_strcpy(doc, root, "method", method);
    yyjson_mut_obj_add_str(doc, root, "build_identity", NativeAuthoring::build_identity());
    yyjson_mut_obj_add_val(doc, root, "arguments", yyjson_val_mut_copy(doc, arguments));
    size_t length = 0;
    char* bytes = yyjson_mut_write(doc, 0, &length);
    yyjson_mut_doc_free(doc);
    if (!bytes || length > MaxMessage) {
        std::free(bytes); reply.fail("input_too_large", "Live request exceeds 4 MiB."); return false;
    }
    const auto name = pipe_name(instance);
    const uint64_t deadline = GetTickCount64() + timeout_ms;
    HANDLE pipe = INVALID_HANDLE_VALUE;
    do {
        pipe = CreateFileA(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED | SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS, nullptr);
        if (pipe != INVALID_HANDLE_VALUE || GetLastError() != ERROR_PIPE_BUSY) break;
        WaitNamedPipeA(name.c_str(), std::min(timeout_ms, 50u));
    } while (GetTickCount64() < deadline);
    if (pipe == INVALID_HANDLE_VALUE) {
        std::free(bytes); reply.fail("instance_unavailable", "The selected Workshop connection is unavailable or busy."); return false;
    }
    DWORD pid = 0;
    if (!GetNamedPipeServerProcessId(pipe, &pid) || pid != std::strtoul(instance.c_str(), nullptr, 10) || !process_is_current_user(pid)) {
        CloseHandle(pipe); std::free(bytes); reply.fail("wrong_user", "Could not verify the target process and Windows user."); return false;
    }
    HANDLE event = CreateEventA(nullptr, TRUE, FALSE, nullptr);
    uint32_t size = (uint32_t)length;
    bool header_sent = event && transfer(pipe, event, &size, 4, true, deadline);
    bool sent = header_sent && transfer(pipe, event, bytes, size, true, deadline);
    std::free(bytes);
    std::string response;
    bool received = sent && transfer(pipe, event, &size, 4, false, deadline) && size && size <= MaxMessage;
    if (received) {
        response.resize(size);
        received = transfer(pipe, event, response.data(), size, false, deadline);
    }
    if (received) {
        unsigned char ack = 6;
        transfer(pipe, event, &ack, 1, true, deadline);
    }
    if (event) CloseHandle(event);
    CloseHandle(pipe);
    if (!received) {
        reply.fail(header_sent ? "completion_unknown" : "request_not_sent",
            header_sent ? "The connection closed or timed out before a complete reply. Inspect the live revision before retrying."
                 : "Could not send a complete authoring request.");
        NativeAuthoring::add_string(reply, reply.result, "request_id", id); return false;
    }
    auto* input = yyjson_read(response.data(), response.size(), 0);
    auto* envelope = yyjson_doc_get_root(input);
    const char* actual_id = yyjson_get_str(yyjson_obj_get(envelope, "id"));
    auto* ok = yyjson_obj_get(envelope, "ok");
    if (!actual_id || id != actual_id || !yyjson_is_bool(ok)) {
        yyjson_doc_free(input); reply.fail("invalid_reply", "Target returned an uncorrelated or invalid reply."); return false;
    }
    reply.ok = yyjson_get_bool(ok);
    auto* result = yyjson_obj_get(envelope, reply.ok ? "result" : "error");
    if (!yyjson_is_obj(result)) {
        yyjson_doc_free(input); reply.fail("invalid_reply", "Target reply needs an object result."); return false;
    }
    reply.result = yyjson_val_mut_copy(reply.document, result);
    NativeAuthoring::add_string(reply, reply.result, "request_id", id);
    yyjson_doc_free(input);
    return reply.ok;
}
}
