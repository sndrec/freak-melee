#include "native_agent_project.h"
#include <melee/ft/native_fighter_build.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <array>
#include <cstdlib>
#include <cstring>

namespace NativeAgent {
namespace {
bool portrait(const Context& context, const std::string& project, yyjson_val* arguments, std::string& bytes)
{
    std::string path = string(arguments, "portrait");
    if (path.empty()) {
        path = project + "/portrait.png";
        if (GetFileAttributesA(path.c_str()) == INVALID_FILE_ATTRIBUTES)
            path = context.installation + "/ui/workshop_draft_portrait.png";
    }
    HANDLE file = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{}; DWORD count = 0;
    bool ok = GetFileSizeEx(file, &size) && size.QuadPart >= 32 && size.QuadPart <= 16 * 1024 * 1024;
    if (ok) { bytes.resize((size_t)size.QuadPart); ok = ReadFile(file, bytes.data(), (DWORD)bytes.size(), &count, nullptr) && count == bytes.size(); }
    CloseHandle(file); return ok;
}
}

void build_project(const Context& context, yyjson_val* arguments, Reply& reply)
{
    std::string error;
    NativeDraftAccess::Lease lease;
    if (!lease.acquire(string(arguments, "project"), error)) { reply.fail("draft_busy", error); return; }
    auto project = open_project(arguments, reply);
    if (!project) return;
    if (project->revision != string(arguments, "revision")) {
        reply.fail("revision_conflict", "Draft changed. Inspect it before building."); return;
    }
    add_string(reply, reply.result, "build_identity", NativeAuthoring::build_identity());
    auto* reports = yyjson_mut_arr(reply.document);
    yyjson_mut_obj_add_val(reply.document, reply.result, "member_reports", reports);
    bool accepted = true;
    unsigned total = 0;
    std::array<NativeFighterSources::Session, NATIVE_FIGHTER_FAMILY_MEMBERS> sessions;
    NativeFighterSources::Session* sources[NATIVE_FIGHTER_FAMILY_MEMBERS]{};
    std::string combined[NATIVE_FIGHTER_FAMILY_MEMBERS];
    auto candidate = project->family;
    for (unsigned i = 0; i < candidate.member_count; ++i) {
        auto* input = yyjson_mut_doc_new(nullptr);
        auto* values = yyjson_mut_obj(input);
        yyjson_mut_doc_set_root(input, values);
        yyjson_mut_obj_add_strcpy(input, values, "project", project->family.paths[0]);
        yyjson_mut_obj_add_strcpy(input, values, "member", project->family.keys[i]);
        auto* immutable = yyjson_mut_doc_imut_copy(input, nullptr);
        Reply checked(reply.document);
        check_scripts(context, yyjson_doc_get_root(immutable), checked);
        yyjson_doc_free(immutable); yyjson_mut_doc_free(input);
        add_string(checked, checked.result, "key", project->family.keys[i]);
        yyjson_mut_arr_add_val(reports, checked.result);
        bool valid = checked.ok && yyjson_mut_get_bool(yyjson_mut_obj_get(checked.result, "accepted"));
        accepted &= valid;
        total += checked.ok ? (unsigned)yyjson_mut_get_uint(yyjson_mut_obj_get(checked.result, "diagnostic_count")) : 1;
        if (!valid) continue;
        auto& member = candidate.members[i];
        if (!NativeFighterSources::open_project(sessions[i], project->family.paths[i], member) ||
            !NativeFighterSources::import_parameters(sessions[i], member)) {
            reply.fail("source_unavailable", sessions[i].error); return;
        }
        sources[i] = &sessions[i];
        combined[i] = NativeFighterSources::compose(sessions[i]);
        member.script_text = (uint8_t*)combined[i].data(); member.script_size = combined[i].size();
        member.parameter_text = (uint8_t*)sessions[i].parameters.data(); member.parameter_size = sessions[i].parameters.size();
    }
    yyjson_mut_obj_add_bool(reply.document, reply.result, "accepted", accepted);
    yyjson_mut_obj_add_uint(reply.document, reply.result, "diagnostic_count", total);
    if (!accepted) return;
    std::string current;
    if (!NativeDraftAccess::revision(project->family.paths[0], current, error) || current != project->revision) {
        reply.fail("revision_conflict", "Draft changed while preparing its source members. Inspect before building."); return;
    }
    std::string image;
    if (!portrait(context, project->family.paths[0], arguments, image)) {
        reply.fail("portrait_unavailable", "Choose a readable portrait PNG, or use the installed default portrait."); return;
    }
    uint8_t* bytes = nullptr; size_t size = 0; NativeContentId id{};
    if (!nativeFighterFamilyProject_Build(&candidate, image.data(), image.size(), &bytes, &size, &id)) {
        reply.fail("family_prepare_failed", "The complete family or portrait could not be prepared. Member diagnostics describe the source checks."); return;
    }
    std::string cache = string(arguments, "content_root");
    if (cache.empty()) cache = context.content_root;
    const NativeContentId expected = id;
    bool cached = nativeContentStore_Cache(cache.c_str(), bytes, size, &expected, &id);
    std::free(bytes);
    if (!cached) { reply.fail("cache_failed", "Could not cache this complete fighter package."); return; }
    char hex[65]; nativeContentStore_IdHex(&id, hex);
    add_string(reply, reply.result, "package_id", hex);
    add_string(reply, reply.result, "content_root", cache);
    std::string fighter_revision;
    if (!NativeFighterBuild::revision(project->family, sources, fighter_revision, error)) {
        reply.fail("revision_unavailable", error); return;
    }
    add_string(reply, reply.result, "fighter_revision", fighter_revision);
    yyjson_mut_obj_add_uint(reply.document, reply.result, "package_bytes", size);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "cached", true);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "saved", false);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "installed", false);
}

void export_package(const Context& context, yyjson_val* arguments, Reply& reply)
{
    NativeContentId expected{}, actual{};
    std::string root = string(arguments, "content_root");
    if (root.empty()) root = context.content_root;
    uint8_t* bytes = nullptr; size_t size = 0;
    bool valid = nativeContentStore_IdFromHex(string(arguments, "package_id"), &expected) &&
        nativeContentStore_Read(root.c_str(), &expected, &bytes, &size) && nativeContent_Hash(bytes, size, &actual) &&
        !std::memcmp(&actual, &expected, sizeof(actual));
    if (!valid) { std::free(bytes); reply.fail("invalid_package", "Choose a readable cached package ID."); return; }
    const char* output = string(arguments, "output");
    const size_t path_size = std::strlen(output);
    bool stream = false;
    for (size_t i = 0; i < path_size; ++i) if (output[i] == ':' && i != 1) stream = true;
    if (path_size < 5 || _stricmp(output + path_size - 4, ".fmc") || stream) {
        std::free(bytes); reply.fail("invalid_output", "Choose a new .fmc package file."); return;
    }
    HANDLE file = CreateFileA(output, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) { std::free(bytes); reply.fail("export_unavailable", "Choose a new file in an existing output directory."); return; }
    DWORD written = 0;
    bool ok = size <= MAXDWORD && WriteFile(file, bytes, (DWORD)size, &written, nullptr) && written == size && FlushFileBuffers(file);
    ok = CloseHandle(file) && ok;
    std::free(bytes);
    if (!ok) { DeleteFileA(output); reply.fail("export_failed", "Could not write the complete package."); return; }
    char full[32768]; DWORD length = GetFullPathNameA(output, sizeof(full), full, nullptr);
    add_string(reply, reply.result, "output", length && length < sizeof(full) ? full : output);
    add_string(reply, reply.result, "package_id", string(arguments, "package_id"));
    yyjson_mut_obj_add_uint(reply.document, reply.result, "bytes", size);
}
}
