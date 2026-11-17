#include "native_agent_project.h"
#include <algorithm>
#include <cstring>

namespace NativeAgent {
Project::~Project() { nativeFighterFamilyProject_Close(&family); }

std::unique_ptr<Project> open_project(yyjson_val* arguments, Reply& reply)
{
    auto result = std::make_unique<Project>();
    const char* path = string(arguments, "project");
    if (!nativeFighterFamilyProject_Open(path, &result->family)) {
        reply.fail("invalid_project", "Could not open this draft's owned assets and metadata: " + std::string(path)); return {};
    }
    const char* member = string(arguments, "member");
    if (*member) {
        unsigned index = 0;
        while (index < result->family.member_count && std::strcmp(result->family.keys[index], member)) ++index;
        if (index == result->family.member_count) {
            reply.fail("unknown_member", "Unknown fighter member: " + std::string(member)); return {};
        }
        result->member = index;
    }
    add_string(reply, reply.result, "member", result->family.keys[result->member]);
    add_string(reply, reply.result, "member_path", result->family.paths[result->member]);
    return result;
}

bool load_sources(const Project& project, NativeFighterSources::Session& sources, Reply& reply)
{
    const auto& member = project.family.members[project.member];
    if (!NativeFighterSources::open_project(sources, project.family.paths[project.member], member)) {
        reply.fail("invalid_source_set", sources.error); return false;
    }
    // Import failures remain inspectable. The agent needs the original source
    // and overrides in order to repair them; scripts.check reports the failure.
    NativeFighterSources::import_parameters(sources, member);
    return true;
}

bool source_status(const NativeFighterSources::Session& files, const NativeFighterProject& project, Reply& reply)
{
    std::string combined = NativeFighterSources::compose(files);
    NativeContentId hash{};
    char hex[65];
    if (!nativeContent_Hash(combined.data(), combined.size(), &hash)) {
        reply.fail("hash_failed", "Could not compute the editable source identity."); return false;
    }
    nativeContentStore_IdHex(&hash, hex);
    add_string(reply, reply.result, "editable_source_sha256", hex);
    const char* origins[] = {"validated_source", "saved_draft", "stock_template", "separate_files"};
    add_string(reply, reply.result, "source_origin", origins[files.origin]);
    std::string validated;
    if (project.script_size) validated.assign((const char*)project.script_text, project.script_size);
    validated.erase(std::remove(validated.begin(), validated.end(), '\r'), validated.end());
    yyjson_mut_obj_add_bool(reply.document, reply.result, "matches_validated_source", combined == validated);
    yyjson_mut_obj_add_uint(reply.document, reply.result, "combined_bytes", combined.size());
    yyjson_mut_obj_add_bool(reply.document, reply.result, "imported_parameters", files.imported_parameters);
    if (!files.error.empty()) add_string(reply, reply.result, "parameter_import_error", files.error);
    if (!nativeContent_Hash(files.parameters.data(), files.parameters.size(), &hash)) {
        reply.fail("hash_failed", "Could not compute parameter identity."); return false;
    }
    nativeContentStore_IdHex(&hash, hex);
    add_string(reply, reply.result, "parameter_sha256", hex);
    return true;
}
}
