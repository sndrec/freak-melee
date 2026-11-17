#include "native_agent_project.h"
#include <melee/ft/native_attribute_authoring.h>
#include <melee/ft/native_fighter_draft_access.h>

namespace NativeAgent {
void edit_attributes(const Context&, yyjson_val* args, Reply& reply)
{
    NativeDraftAccess::Lease lease;
    std::string error;
    if (!lease.acquire(string(args, "project"), error)) {
        reply.fail("draft_busy", error); return;
    }
    auto project = open_project(args, reply);
    if (!project) return;
    if (project->revision != string(args, "revision")) {
        reply.fail("revision_conflict", "Inspect the changed draft before editing its attributes."); return;
    }
    auto& member = project->family.members[project->member];
    if (!NativeAttributeAuthoring::edit(member, args, reply)) return;
    if (yyjson_mut_get_bool(yyjson_mut_obj_get(reply.result, "changed"))) {
        const std::string prefix = project->member ? "partner/" : "";
        std::vector<NativeDraftAccess::FileEdit> files{{prefix + "fighter.dat",
            std::string((const char*) member.fighter_dat, member.fighter_size)}};
        std::string after;
        if (!NativeDraftAccess::apply(project->family.paths[0], project->revision, files, after, error)) {
            reply.fail("save_failed", error); return;
        }
        project->revision = after;
    }
    yyjson_mut_obj_remove_str(reply.result, "revision");
    add_string(reply, reply.result, "revision", project->revision);
    yyjson_mut_obj_remove_str(reply.result, "saved");
    yyjson_mut_obj_add_bool(reply.document, reply.result, "saved", true);
}
}
