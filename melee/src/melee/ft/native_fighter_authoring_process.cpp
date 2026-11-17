#include "native_fighter_authoring_process.h"
#include "native_fighter_build.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace NativeFighterAuthoringProcess {
namespace {
using NativeAuthoring::Reply;
using NativeAuthoring::add_string;
constexpr DWORD Deadline = 15000;
constexpr DWORD MaxReply = 1024 * 1024;

struct Isolated {
    std::string root;
    std::vector<std::string> files;
    bool ordinary(const std::string& relative) const {
        std::string path = root;
        size_t start = 0;
        while (start < relative.size()) {
            size_t end = relative.find('/', start);
            if (end == std::string::npos) end = relative.size();
            path += '/' + relative.substr(start, end - start);
            DWORD flags = GetFileAttributesA(path.c_str());
            if (flags != INVALID_FILE_ATTRIBUTES && (flags & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
            start = end + 1;
        }
        return true;
    }
    ~Isolated() {
        if (root.empty()) return;
        DWORD flags = GetFileAttributesA(root.c_str());
        if (flags == INVALID_FILE_ATTRIBUTES || (flags & FILE_ATTRIBUTE_REPARSE_POINT)) return;
        std::vector<std::string> directories{"draft"};
        for (const auto& file : files) {
            if (!ordinary(file)) continue;
            DeleteFileA((root + '/' + file).c_str());
            for (size_t end = file.find('/'); end != std::string::npos; end = file.find('/', end + 1))
                directories.push_back(file.substr(0, end));
        }
        std::sort(directories.begin(), directories.end(), [](const auto& a, const auto& b) { return a.size() > b.size(); });
        directories.erase(std::unique(directories.begin(), directories.end()), directories.end());
        for (const auto& path : directories) if (ordinary(path)) RemoveDirectoryA((root + '/' + path).c_str());
        if (!RemoveDirectoryA(root.c_str()))
            std::fprintf(stderr, "[workshop-validation] Retained isolated validation files at %s\n", root.c_str());
    }
    bool create(const NativeFighterSnapshot::Capture& capture, std::string& error) {
        char temporary[MAX_PATH];
        DWORD length = GetTempPathA(sizeof(temporary), temporary);
        unsigned char random[16]; char hex[33];
        if (!length || length >= sizeof(temporary) ||
            BCryptGenRandom(nullptr, random, sizeof(random), BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0) {
            error = "Could not create isolated validation storage."; return false;
        }
        for (unsigned i = 0; i < sizeof(random); ++i) std::snprintf(hex + i * 2, 3, "%02x", random[i]);
        std::string path = std::string(temporary) + "FreakMeleeValidation-" + std::to_string(GetCurrentProcessId()) + '-' + hex;
        if (path.size() > 350 || !CreateDirectoryA(path.c_str(), nullptr)) {
            error = "Could not reserve a private validation directory."; return false;
        }
        root = std::move(path);
        const std::string draft = root + "/draft";
        if (!CreateDirectoryA(draft.c_str(), nullptr)) {
            error = "Could not create the isolated draft directory."; return false;
        }
        // This new directory is exclusively owned by this capture. apply uses
        // the same path checks and complete-batch staging as normal draft saves.
        std::string before, after;
        if (!NativeDraftAccess::revision(draft, before, error) ||
            !NativeDraftAccess::apply(draft, before, capture.files, after, error)) return false;
        for (const auto& file : capture.files) if (!file.remove) files.push_back("draft/" + file.file);
        return true;
    }
};

bool read(const std::string& path, std::string& bytes, DWORD limit)
{
    HANDLE file = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{};
    DWORD count = 0;
    bool ok = GetFileSizeEx(file, &size) && size.QuadPart >= 0 && size.QuadPart <= limit;
    if (ok) { bytes.resize((size_t)size.QuadPart); ok = ReadFile(file, bytes.data(), (DWORD)bytes.size(), &count, nullptr) && count == bytes.size(); }
    CloseHandle(file); return ok;
}

bool run(Isolated& isolated, const char* member, const char* content_root, std::string& output, std::string& error)
{
    char executable[32768];
    DWORD length = GetModuleFileNameA(nullptr, executable, sizeof(executable));
    char* basename = length && length < sizeof(executable) ? std::strrchr(executable, '\\') : nullptr;
    if (!basename) { error = "Could not locate the installed native validator."; return false; }
    std::string tool(executable, basename + 1);
    tool += "native_content_tool.exe";
    auto* arguments = yyjson_mut_doc_new(nullptr);
    if (!arguments) { error = "Could not allocate the validation request."; return false; }
    auto* object = yyjson_mut_obj(arguments);
    yyjson_mut_doc_set_root(arguments, object);
    const std::string draft = isolated.root + "/draft";
    yyjson_mut_obj_add_strcpy(arguments, object, "project", draft.c_str());
    if (member) yyjson_mut_obj_add_strcpy(arguments, object, "member", member);
    else {
        std::string revision;
        if (!NativeDraftAccess::revision(draft, revision, error)) { yyjson_mut_doc_free(arguments); return false; }
        yyjson_mut_obj_add_strcpy(arguments, object, "revision", revision.c_str());
        yyjson_mut_obj_add_strcpy(arguments, object, "content_root", content_root);
    }
    size_t size = 0;
    char* request = yyjson_mut_write(arguments, 0, &size);
    yyjson_mut_doc_free(arguments);
    SECURITY_ATTRIBUTES inherit{sizeof(inherit), nullptr, TRUE};
    const char* names[] = {"request.json", "response.json", "stderr.log"};
    HANDLE handles[3] = {INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE};
    bool ready = request != nullptr;
    for (unsigned i = 0; ready && i < 3; ++i) {
        isolated.files.push_back(names[i]);
        handles[i] = CreateFileA((isolated.root + '/' + names[i]).c_str(), i ? GENERIC_WRITE : GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ, &inherit, CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY, nullptr);
        ready = handles[i] != INVALID_HANDLE_VALUE;
    }
    DWORD written = 0; LARGE_INTEGER zero{};
    ready = ready && WriteFile(handles[0], request, (DWORD)size, &written, nullptr) && written == size &&
        SetFilePointerEx(handles[0], zero, nullptr, FILE_BEGIN);
    std::free(request);
    SIZE_T attribute_bytes = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &attribute_bytes);
    std::vector<unsigned char> attributes(attribute_bytes);
    STARTUPINFOEXA startup{};
    startup.StartupInfo.cb = sizeof(startup);
    startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    startup.StartupInfo.hStdInput = handles[0];
    startup.StartupInfo.hStdOutput = handles[1];
    startup.StartupInfo.hStdError = handles[2];
    startup.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributes.data());
    bool initialized = ready && InitializeProcThreadAttributeList(startup.lpAttributeList, 1, 0, &attribute_bytes);
    ready = initialized && UpdateProcThreadAttribute(startup.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
        handles, sizeof(handles), nullptr, nullptr);
    std::string command = '"' + tool + "\" agent call " + (member ? "scripts.check" : "project.build") + " --stdin";
    PROCESS_INFORMATION child{};
    bool started = ready && CreateProcessA(tool.c_str(), command.data(), nullptr, nullptr, TRUE,
        CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT, nullptr, isolated.root.c_str(), &startup.StartupInfo, &child);
    if (initialized) DeleteProcThreadAttributeList(startup.lpAttributeList);
    DWORD wait = WAIT_FAILED, code = 1;
    if (started) {
        CloseHandle(child.hThread);
        wait = WaitForSingleObject(child.hProcess, Deadline);
        if (wait != WAIT_OBJECT_0) {
            // This handle belongs only to the validator we just created.
            TerminateProcess(child.hProcess, 1);
            WaitForSingleObject(child.hProcess, 5000);
        }
        GetExitCodeProcess(child.hProcess, &code);
        CloseHandle(child.hProcess);
    }
    for (HANDLE handle : handles) if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
    if (!started || wait != WAIT_OBJECT_0 || !read(isolated.root + "/response.json", output, MaxReply)) {
        error = !started ? "Could not start the installed native validator." : wait == WAIT_TIMEOUT
            ? "Validation exceeded its 15-second limit." : "Could not read the native validation result.";
        std::string log;
        if (read(isolated.root + "/stderr.log", log, 4096) && !log.empty()) error += " " + log;
        return false;
    }
    return code <= 1; // Both success and structured invalid-request replies are JSON.
}

void import_result(const Isolated& isolated, const std::string& bytes, const char* const* fields, size_t field_count, Reply& reply)
{
    auto* document = yyjson_read(bytes.data(), bytes.size(), 0);
    auto* root = yyjson_doc_get_root(document);
    auto* result = yyjson_obj_get(root, "result");
    const char* build = yyjson_get_str(yyjson_obj_get(result, "build_identity"));
    if (!document || !yyjson_is_bool(yyjson_obj_get(root, "ok")) || !yyjson_is_obj(result)) {
        auto* failure = yyjson_obj_get(root, "error");
        const char* message = yyjson_get_str(yyjson_obj_get(failure, "message"));
        std::string text = message ? message : "The native validator returned an invalid structured result.";
        for (size_t at = text.find(isolated.root); at != std::string::npos; at = text.find(isolated.root, at + 17))
            text.replace(at, isolated.root.size(), "<validation copy>");
        reply.fail("validation_unavailable", text);
    } else if (!build || std::strcmp(build, NativeAuthoring::build_identity())) {
        reply.fail("validator_mismatch", "The installed validator belongs to another build. Use native_content_tool.exe from the running game's installation.");
    } else {
        // Keep temporary paths and their disk revision private. Source paths in
        // diagnostics are relative and resolve against the live member instead.
        for (size_t i = 0; i < field_count; ++i) {
            const char* field = fields[i];
            auto* value = yyjson_obj_get(result, field);
            if (value) yyjson_mut_obj_add_val(reply.document, reply.result, field, yyjson_val_mut_copy(reply.document, value));
        }
        add_string(reply, reply.result, "draft_view", "live_snapshot");
    }
    yyjson_doc_free(document);
}

void locate_diagnostics(Reply& reply, yyjson_mut_val* object)
{
        auto* diagnostics = yyjson_mut_obj_get(object, "diagnostics");
        size_t index, count; yyjson_mut_val* diagnostic;
        yyjson_mut_arr_foreach(diagnostics, index, count, diagnostic) {
            if (yyjson_mut_obj_get(diagnostic, "source_view")) {
                yyjson_mut_obj_remove_str(diagnostic, "source_view");
                yyjson_mut_obj_add_str(reply.document, diagnostic, "source_view", "session.scripts.read");
            }
        }
}
}

void check(const NativeFighterFamilyProject& family,
    NativeFighterSources::Session* const sources[NATIVE_FIGHTER_FAMILY_MEMBERS],
    unsigned member, Reply& reply)
{
    if (member >= family.member_count) { reply.fail("unknown_member", "Choose an open fighter member."); return; }
    NativeFighterSnapshot::Capture capture;
    Isolated isolated;
    std::string error, bytes;
    if (!NativeFighterSnapshot::capture(family, sources, capture, error) || !isolated.create(capture, error) ||
        !run(isolated, family.keys[member], nullptr, bytes, error)) {
        reply.fail("validation_unavailable", error.empty() ? "The isolated validator exited unexpectedly." : error); return;
    }
    const char* fields[] = {"accepted", "diagnostics", "diagnostic_count", "diagnostics_truncated", "metadata_complete",
        "bodies_complete", "package_diagnostic", "functions_checked", "statistics", "editable_source_sha256",
        "parameter_sha256", "matches_validated_source", "combined_bytes", "parameter_import_error"};
    import_result(isolated, bytes, fields, sizeof(fields) / sizeof(*fields), reply);
    if (reply.ok) locate_diagnostics(reply, reply.result);
}

void build(const NativeFighterFamilyProject& family,
    NativeFighterSources::Session* const sources[NATIVE_FIGHTER_FAMILY_MEMBERS],
    const void* portrait, size_t portrait_size, const char* content_root, Reply& reply)
{
    if (!portrait || !portrait_size || !content_root) { reply.fail("build_inputs", "Choose a portrait and a local content cache."); return; }
    NativeFighterSnapshot::Capture capture;
    Isolated isolated;
    std::string error, bytes, revision;
    if (!NativeFighterBuild::revision(family, sources, revision, error) ||
        !NativeFighterSnapshot::capture(family, sources, capture, error)) {
        reply.fail("build_unavailable", error); return;
    }
    capture.files.push_back({"portrait.png", std::string((const char*)portrait, portrait_size)});
    if (!isolated.create(capture, error) || !run(isolated, nullptr, content_root, bytes, error)) {
        reply.fail("build_unavailable", error.empty() ? "The isolated builder exited unexpectedly." : error); return;
    }
    const char* fields[] = {"accepted", "member_reports", "diagnostic_count", "package_id", "package_bytes", "cached", "saved", "installed"};
    import_result(isolated, bytes, fields, sizeof(fields) / sizeof(*fields), reply);
    if (!reply.ok) return;
    auto* members = yyjson_mut_obj_get(reply.result, "member_reports");
    size_t index, count; yyjson_mut_val* member;
    yyjson_mut_arr_foreach(members, index, count, member) {
        for (const char* name : {"member_path", "revision", "draft_view"}) yyjson_mut_obj_remove_str(member, name);
        locate_diagnostics(reply, member);
    }
    add_string(reply, reply.result, "fighter_revision", revision);
    if (!yyjson_mut_get_bool(yyjson_mut_obj_get(reply.result, "accepted"))) return;
    NativeContentId id{};
    const char* hex = yyjson_mut_get_str(yyjson_mut_obj_get(reply.result, "package_id"));
    if (!hex || !nativeContentStore_IdFromHex(hex, &id) || !NativeFighterBuild::verify_cached(family, sources, content_root, id, error))
        reply.fail("build_mismatch", error.empty() ? "The builder returned an invalid package identity." : error);
}
}
