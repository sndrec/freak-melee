#include "native_agent_project.h"
#include <melee/ft/native_fighter_draft_access.h>
#include <melee/ft/native_fighter_parameters.h>
#include <algorithm>
#include <cstring>

namespace NativeAgent {
void update_scripts(const Context&, yyjson_val* arguments, Reply& reply)
{
    NativeDraftAccess::Lease lease;
    std::string error;
    const char* root = string(arguments, "project");
    if (!lease.acquire(root, error)) { reply.fail("draft_busy", error); return; }
    auto project = open_project(arguments, reply);
    if (!project) return;
    const char* expected = string(arguments, "revision");
    if (project->revision != expected) {
        reply.fail("revision_conflict", "The draft has changed. Inspect it before editing.");
        add_string(reply, reply.result, "revision", project->revision); return;
    }
    NativeFighterSources::Session sources;
    if (!load_sources(*project, sources, reply)) return;
    size_t index, count; yyjson_val* item;
    std::vector<NativeFighterSources::SourceEdit> edits;
    yyjson_arr_foreach(yyjson_obj_get(arguments, "files"), index, count, item) {
        edits.push_back({string(item, "file"), string(item, "text")});
    }
    auto* parameters = yyjson_obj_get(arguments, "parameters");
    std::string parameter_text = parameters ? string(arguments, "parameters") : "";
    auto staged = NativeFighterSources::edit(sources, project->family.members[project->member], edits, parameters ? &parameter_text : nullptr);
    if (!staged.ok) { reply.fail(staged.code, staged.message); return; }
    parameter_text = sources.parameters;
    // A selected member has its own transaction root; the supplied revision
    // still describes the complete family, so an edit to either member conflicts.
    const std::string member_root = project->family.paths[project->member];
    std::string member_revision;
    if (!NativeDraftAccess::revision(member_root, member_revision, error)) {
        reply.fail("revision_unavailable", error); return;
    }
    std::string current;
    if (!NativeDraftAccess::revision(root, current, error) || current != expected) {
        reply.fail("revision_conflict", "Draft changed while preparing this batch."); return;
    }
    std::string after;
    if (!NativeFighterSources::save_batch(sources, member_revision, &parameter_text, after)) {
        reply.fail("edit_failed", sources.error); return;
    }
    add_string(reply, reply.result, "previous_revision", expected);
    yyjson_mut_obj_remove_str(reply.result, "revision");
    if (NativeDraftAccess::revision(root, after, error)) add_string(reply, reply.result, "revision", after);
    else {
        yyjson_mut_obj_add_null(reply.document, reply.result, "revision");
        add_string(reply, reply.result, "revision_diagnostic", error);
    }
    add_string(reply, reply.result, "status", "saved_draft");
    yyjson_mut_obj_add_bool(reply.document, reply.result, "committed", true);
    yyjson_mut_obj_add_uint(reply.document, reply.result, "edited_files", edits.size());
    yyjson_mut_obj_add_bool(reply.document, reply.result, "validated", false);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "installed", false);
    source_status(sources, project->family.members[project->member], reply);
}
}
