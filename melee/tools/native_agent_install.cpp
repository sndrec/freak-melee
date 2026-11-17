#include "native_agent_project.h"
#include <melee/ft/native_fighter_build.h>
#include <array>
#include <cstring>

namespace NativeAgent {
void install_project(const Context& context, yyjson_val* args, Reply& reply)
{
    std::string error;
    NativeDraftAccess::Lease lease;
    if (!lease.acquire(string(args, "project"), error)) { reply.fail("draft_busy", error); return; }
    auto project = open_project(args, reply);
    if (!project) return;
    if (project->revision != string(args, "revision")) { reply.fail("revision_conflict", "Inspect the changed draft before installing a build."); return; }
    const std::string root = *string(args, "content_root") ? string(args, "content_root") : context.content_root;
    NativeContentId id{}, replaced{};
    if (!nativeContentStore_IdFromHex(string(args, "package_id"), &id)) { reply.fail("invalid_package", "Choose the package_id returned by project.build."); return; }
    std::array<NativeFighterSources::Session, NATIVE_FIGHTER_FAMILY_MEMBERS> sessions;
    NativeFighterSources::Session* sources[NATIVE_FIGHTER_FAMILY_MEMBERS]{};
    for (unsigned i = 0; i < project->family.member_count; ++i) {
        if (!NativeFighterSources::open_project(sessions[i], project->family.paths[i], project->family.members[i]) ||
            !NativeFighterSources::import_parameters(sessions[i], project->family.members[i])) {
            reply.fail("source_unavailable", sessions[i].error); return;
        }
        sources[i] = &sessions[i];
    }
    // Acceptance verifies the complete cached artifact against current editable
    // sources and owned assets before publishing its behavior to the draft.
    std::string after;
    if (!NativeFighterBuild::accept(project->family, sources, root.c_str(), id, project->revision, after, error)) {
        reply.fail("build_not_accepted", error); return;
    }
    add_string(reply, reply.result, "revision", after);
    add_string(reply, reply.result, "package_id", string(args, "package_id"));
    yyjson_mut_obj_add_bool(reply.document, reply.result, "accepted", true);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "saved", true);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "installed", false);
    if (!nativeFighterProject_Install(project->family.paths[0], root.c_str(), &id, &replaced)) {
        reply.fail("install_failed", "Behavior was accepted and saved, but installing this package failed. The previous installation is retained.");
        add_string(reply, reply.result, "revision", after);
        add_string(reply, reply.result, "package_id", string(args, "package_id"));
        yyjson_mut_obj_add_bool(reply.document, reply.result, "accepted", true);
        yyjson_mut_obj_add_bool(reply.document, reply.result, "saved", true);
        yyjson_mut_obj_add_bool(reply.document, reply.result, "installed", false);
        return;
    }
    yyjson_mut_obj_remove_str(reply.result, "installed");
    yyjson_mut_obj_add_bool(reply.document, reply.result, "installed", true);
    add_string(reply, reply.result, "installed_package_id", string(args, "package_id"));
    add_string(reply, reply.result, "status", "installed");
    NativeContentId empty{};
    if (std::memcmp(&replaced, &empty, sizeof(empty))) {
        char hex[65]; nativeContentStore_IdHex(&replaced, hex);
        add_string(reply, reply.result, "replaced_package_id", hex);
    } else yyjson_mut_obj_add_null(reply.document, reply.result, "replaced_package_id");
}
}
