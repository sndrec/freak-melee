#include <melee/ft/native_workshop_connection.h>
#include <melee/ft/native_workshop_connection_security.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>

#define REQUIRE(expression) do { if (!(expression)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expression); std::abort(); } } while (0)
using namespace NativeWorkshopConnection;

static void echo(const char* method, yyjson_val* arguments, NativeAuthoring::Reply& reply, void*)
{
    if (std::strcmp(method, "echo")) { reply.fail("unknown_tool", "Unknown test operation."); return; }
    auto* value = yyjson_obj_get(arguments, "text");
    if (!yyjson_is_str(value)) { reply.fail("invalid_arguments", "Expected text."); return; }
    yyjson_mut_obj_add_val(reply.document, reply.result, "text", yyjson_val_mut_copy(reply.document, value));
}

static HANDLE raw_connection(const std::string& instance)
{
    for (unsigned i = 0; i < 100; ++i) {
        HANDLE pipe = CreateFileA(pipe_name(instance).c_str(), GENERIC_READ | GENERIC_WRITE, 0,
            nullptr, OPEN_EXISTING, SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS, nullptr);
        if (pipe != INVALID_HANDLE_VALUE) return pipe;
        Sleep(5);
    }
    REQUIRE(false); return INVALID_HANDLE_VALUE;
}
static void bytes(HANDLE pipe, void* memory, DWORD length, bool write)
{
    auto* at = (char*)memory;
    while (length) {
        DWORD count = 0;
        REQUIRE(write ? WriteFile(pipe, at, length, &count, nullptr) : ReadFile(pipe, at, length, &count, nullptr));
        REQUIRE(count && count <= length); at += count; length -= count;
    }
}
static int client(const std::string& instance, const char* mode)
{
    if (!std::strcmp(mode, "large") || !std::strcmp(mode, "wrong-instance")) {
        auto* doc = yyjson_mut_doc_new(nullptr);
        auto* object = yyjson_mut_obj(doc); yyjson_mut_doc_set_root(doc, object);
        yyjson_mut_obj_add_strcpy(doc, object, "instance", !std::strcmp(mode, "wrong-instance")
            ? "1-00000000000000000000000000000000" : instance.c_str());
        std::string text(512 * 1024, 'a');
        for (size_t i = 0; i < text.size(); i += 137) text[i] = char('A' + i % 26);
        yyjson_mut_obj_add_strcpy(doc, object, "text", text.c_str());
        auto* input = yyjson_mut_doc_imut_copy(doc, nullptr);
        NativeAuthoring::Reply reply(doc);
        bool ok = request(instance, "echo", yyjson_doc_get_root(input), reply);
        if (!std::strcmp(mode, "large")) {
            REQUIRE(ok);
            REQUIRE(text == yyjson_mut_get_str(yyjson_mut_obj_get(reply.result, "text")));
            REQUIRE(std::strlen(yyjson_mut_get_str(yyjson_mut_obj_get(reply.result, "request_id"))) == 32);
        } else {
            REQUIRE(!ok);
            REQUIRE(!std::strcmp(yyjson_mut_get_str(yyjson_mut_obj_get(reply.result, "code")), "wrong_instance"));
        }
        yyjson_doc_free(input); yyjson_mut_doc_free(doc); return 0;
    }
    HANDLE pipe = raw_connection(instance);
    if (!std::strcmp(mode, "oversize")) {
        uint32_t size = MaxMessage + 1; bytes(pipe, &size, 4, true); CloseHandle(pipe); return 0;
    }
    if (!std::strcmp(mode, "partial")) {
        uint32_t size = 1024; bytes(pipe, &size, 4, true);
        char fragment[] = "{}"; bytes(pipe, fragment, 2, true); CloseHandle(pipe); return 0;
    }
    const std::string id(32, '1');
    std::string payload = !std::strcmp(mode, "malformed") ? "{" :
        "{\"id\":\"" + id + "\",\"method\":\"echo\",\"arguments\":{\"instance\":\"" + instance +
        "\"},\"build_identity\":\"different-build\"}";
    uint32_t size = (uint32_t)payload.size();
    bytes(pipe, &size, 4, true); bytes(pipe, payload.data(), size, true);
    bytes(pipe, &size, 4, false); REQUIRE(size && size <= MaxMessage);
    std::string response(size, '\0'); bytes(pipe, response.data(), size, false);
    unsigned char ack = 6; bytes(pipe, &ack, 1, true);
    auto* doc = yyjson_read(response.data(), response.size(), 0);
    auto* root = yyjson_doc_get_root(doc);
    REQUIRE(!yyjson_get_bool(yyjson_obj_get(root, "ok")));
    REQUIRE(!std::strcmp(yyjson_get_str(yyjson_obj_get(yyjson_obj_get(root, "error"), "code")),
        !std::strcmp(mode, "malformed") ? "invalid_request" : "build_mismatch"));
    yyjson_doc_free(doc); CloseHandle(pipe); return 0;
}

int main(int argc, char** argv)
{
    if (argc == 4 && !std::strcmp(argv[1], "--client")) return client(argv[2], argv[3]);
    Server server;
    std::string error;
    REQUIRE(start(server, echo, nullptr, error));
    auto found = instances();
    REQUIRE(std::find(found.begin(), found.end(), server.instance) != found.end());
    REQUIRE(process_is_current_user(GetCurrentProcessId()));
    REQUIRE(!process_is_current_user(0));
    char executable[32768];
    REQUIRE(GetModuleFileNameA(nullptr, executable, sizeof(executable)));
    for (const char* mode : {"large", "wrong-instance", "oversize", "large", "partial", "malformed", "mismatch", "large"}) {
        std::string command = '"' + std::string(executable) + "\" --client " + server.instance + ' ' + mode;
        STARTUPINFOA startup{}; startup.cb = sizeof(startup);
        PROCESS_INFORMATION child{};
        REQUIRE(CreateProcessA(executable, command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
            nullptr, nullptr, &startup, &child));
        uint64_t deadline = GetTickCount64() + 15000;
        while (WaitForSingleObject(child.hProcess, 0) != WAIT_OBJECT_0 && GetTickCount64() < deadline) { poll(server); Sleep(1); }
        DWORD code = STILL_ACTIVE;
        REQUIRE(GetExitCodeProcess(child.hProcess, &code));
        if (code == STILL_ACTIVE) { TerminateProcess(child.hProcess, 99); WaitForSingleObject(child.hProcess, INFINITE); }
        if (code) std::fprintf(stderr, "Connection case %s exited %lu\n", mode, code);
        CloseHandle(child.hThread); CloseHandle(child.hProcess); REQUIRE(code == 0);
        for (unsigned i = 0; i < 100; ++i) { poll(server); Sleep(1); }
        REQUIRE(server.phase == Server::Listening);
    }
    auto old = server.instance;
    stop(server);
    for (unsigned i = 0; i < 100 && server.phase != Server::Off; ++i) { poll(server); Sleep(1); }
    REQUIRE(server.phase == Server::Off);
    found = instances(); REQUIRE(std::find(found.begin(), found.end(), old) == found.end());
    REQUIRE(start(server, echo, nullptr, error)); REQUIRE(server.instance != old);
    shutdown(server); REQUIRE(server.phase == Server::Off);
    std::puts("PASS: same-user discovery, framed/partial/large messages, correlation, identity rejection, cancellation and reconnect");
    return 0;
}
