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
    std::vector<size_t> selected;
    yyjson_arr_foreach(yyjson_obj_get(arguments, "files"), index, count, item) {
        std::string requested = string(item, "file");
        std::replace(requested.begin(), requested.end(), '\\', '/');
        size_t file_index = 0;
        for (; file_index < sources.files.size(); ++file_index) {
            auto relative = sources.files[file_index].path.substr(sources.root.size() + 1);
            std::replace(relative.begin(), relative.end(), '\\', '/');
            if (requested == relative) break;
        }
        if (file_index == sources.files.size()) { reply.fail("unknown_source", "Use a file returned by scripts.list: " + requested); return; }
        if (std::find(selected.begin(), selected.end(), file_index) != selected.end()) {
            reply.fail("duplicate_source", "A batch may edit each source once."); return;
        }
        selected.push_back(file_index);
        sources.files[file_index].text = string(item, "text");
    }
    auto* parameters = yyjson_obj_get(arguments, "parameters");
    if (selected.empty() && !parameters) { reply.fail("empty_edit", "Supply source edits or parameter text."); return; }
    if (parameters) sources.parameters = string(arguments, "parameters");
    // Re-run imports after the proposed edits, allowing the agent to repair an
    // invalid override instead of making that broken file impossible to edit.
    const auto& member = project->family.members[project->member];
    if (!NativeFighterSources::import_parameters(sources, member)) {
        reply.fail("parameter_import", sources.error); return;
    }
    std::string parameter_text = sources.parameters;
    NativeFighterParameters parsed{}; unsigned line = 0;
    if (!nativeFighterParameters_Parse(parameter_text.data(), parameter_text.size(), &parsed, &line)) {
        reply.fail("invalid_parameters", "Invalid parameter text.");
        yyjson_mut_obj_add_uint(reply.document, reply.result, "line", line); return;
    }
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
    yyjson_mut_obj_add_uint(reply.document, reply.result, "edited_files", selected.size());
    yyjson_mut_obj_add_bool(reply.document, reply.result, "validated", false);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "installed", false);
    source_status(sources, project->family.members[project->member], reply);
}
}
