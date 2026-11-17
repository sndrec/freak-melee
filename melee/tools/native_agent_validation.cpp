#include "native_agent_project.h"
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/native_fighter_validation.h>
#include <cstring>

namespace NativeAgent {
static void diagnostic(Reply& reply, yyjson_mut_val* array,
                       const NativeFighterSources::Session& files,
                       const NativeScriptDiagnostic& entry)
{
    auto* object = yyjson_mut_obj(reply.document);
    yyjson_mut_arr_add_val(array, object);
    add_string(reply, object, "severity", "error");
    add_string(reply, object, "message", entry.message);
    size_t file = 0;
    unsigned line = 0;
    if (NativeFighterSources::locate_line(files, entry.source.line, &file, &line)) {
        add_string(reply, object, "file", files.files[file].path.substr(files.root.size() + 1));
        yyjson_mut_obj_add_uint(reply.document, object, "line", line);
        yyjson_mut_obj_add_uint(reply.document, object, "column", entry.source.column);
        add_string(reply, object, "source_view", "scripts.read");
    } else {
        yyjson_mut_obj_add_null(reply.document, object, "file");
        yyjson_mut_obj_add_null(reply.document, object, "line");
        yyjson_mut_obj_add_null(reply.document, object, "column");
    }
}

void check_scripts(const Context&, yyjson_val* arguments, Reply& reply)
{
    auto project = open_project(arguments, reply);
    NativeFighterSources::Session files;
    if (!project || !load_sources(*project, files, reply)) return;
    const auto& member = project->family.members[project->member];
    if (!source_status(files, member, reply)) return;
    auto* diagnostics = yyjson_mut_arr(reply.document);
    yyjson_mut_obj_add_val(reply.document, reply.result, "diagnostics", diagnostics);
    if (!files.error.empty()) {
        auto* object = yyjson_mut_obj(reply.document);
        yyjson_mut_arr_add_val(diagnostics, object);
        add_string(reply, object, "severity", "error");
        add_string(reply, object, "phase", "parameter_import");
        add_string(reply, object, "message", files.error);
        if (files.error_file < files.files.size())
            add_string(reply, object, "file", files.files[files.error_file].path.substr(files.root.size() + 1));
        yyjson_mut_obj_add_uint(reply.document, object, "line", files.error_line);
        yyjson_mut_obj_add_bool(reply.document, reply.result, "accepted", false);
        yyjson_mut_obj_add_uint(reply.document, reply.result, "diagnostic_count", 1);
        yyjson_mut_obj_add_bool(reply.document, reply.result, "metadata_complete", false);
        yyjson_mut_obj_add_bool(reply.document, reply.result, "bodies_complete", false);
        return;
    }
    std::string combined = NativeFighterSources::compose(files);
    std::vector<NativeScriptDiagnostic> entries(256);
    NativeFighterValidation report{};
    report.entries = entries.data(); report.capacity = (uint32_t)entries.size();
    if (!nativeFighterProject_CheckSource(&member, combined.data(), combined.size(),
        files.parameters.data(), files.parameters.size(), project->member, &report)) {
        reply.fail("validation_unavailable", "The validator could not prepare and release an independent candidate. No draft files were changed.");
        return;
    }
    for (unsigned i = 0; i < report.count; ++i) diagnostic(reply, diagnostics, files, entries[i]);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "accepted", report.accepted);
    yyjson_mut_obj_add_uint(reply.document, reply.result, "diagnostic_count", report.total);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "diagnostics_truncated", report.total > report.count);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "metadata_complete", report.metadata_complete);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "package_diagnostic", report.package_diagnostic);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "bodies_complete", report.bodies_complete);
    yyjson_mut_obj_add_uint(reply.document, reply.result, "functions_checked", report.functions_checked);
    if (report.accepted) {
        auto* statistics = yyjson_mut_obj(reply.document);
        yyjson_mut_obj_add_uint(reply.document, statistics, "functions", report.functions);
        yyjson_mut_obj_add_uint(reply.document, statistics, "instructions", report.instructions);
        yyjson_mut_obj_add_uint(reply.document, statistics, "parameters", report.parameters);
        yyjson_mut_obj_add_val(reply.document, reply.result, "statistics", statistics);
    }
}
}
