#include "native_agent_project.h"
#include <melee/ft/native_model_authoring.h>
#include <melee/ft/native_fighter_snapshot.h>

namespace NativeAgent {
void scan_model(const Context&, yyjson_val* args, Reply& reply) {NativeModelAuthoring::scan(args, reply);}
void read_models(const Context&, yyjson_val* args, Reply& reply)
{
    auto project = open_project(args, reply);
    if (project) NativeModelAuthoring::read(project->family.members[project->member], args, reply);
}
void edit_models(const Context&, yyjson_val* args, Reply& reply)
{
    NativeDraftAccess::Lease lease; std::string error;
    if (!lease.acquire(string(args, "project"), error)) {reply.fail("draft_busy", error); return;}
    auto project = open_project(args, reply); if (!project) return;
    if (project->revision != string(args, "revision")) {reply.fail("revision_conflict", "Inspect the changed draft before editing its model."); return;}
    if (!NativeModelAuthoring::edit(project->family.members[project->member], args, reply)) return;
    if (yyjson_mut_get_bool(yyjson_mut_obj_get(reply.result, "changed"))) {
        // Model operations can change draw selectors, rig facts, animation
        // bindings and source-aware metadata together. Save the actual member's
        // complete assets through the existing family transaction.
        NativeFighterSnapshot::Capture capture;
        if (!NativeFighterSnapshot::capture(project->family, nullptr, capture, error)) {reply.fail("capture_failed", error); return;}
        std::string after;
        if (!NativeDraftAccess::apply(project->family.paths[0], project->revision, capture.files, after, error)) {reply.fail("save_failed", error); return;}
        project->revision = after;
    }
    yyjson_mut_obj_remove_str(reply.result, "revision"); add_string(reply, reply.result, "revision", project->revision);
    yyjson_mut_obj_remove_str(reply.result, "saved"); yyjson_mut_obj_add_bool(reply.document, reply.result, "saved", true);
}
}
